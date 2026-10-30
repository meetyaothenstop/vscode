#include "power_control.h"
#include "forward.h"
#include "reverse.h"
#include "power_protection.h"
#include "power_build_config.h"
#include <math.h>

#define ALL_OUTPUTS (HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2)
#define LOW_OUTPUTS (HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB2)
#define TIMER_A HRTIM_TIMERINDEX_TIMER_A
#define TIMER_B HRTIM_TIMERINDEX_TIMER_B

volatile uint32_t power_command;
volatile float power_target_volts = POWER_FORWARD_DEFAULT_OUTPUT_VOLTS;
volatile PowerStatus power_status;
static volatile unsigned start_pending;
static unsigned active_mode, bootstrap_cycles, arming_cycles;
static unsigned bootstrap_resume, transition_ready;
static unsigned reverse_polarity_cycles;
static float active_target;
static uint32_t cycle_budget;
static uint32_t previous_entry;
static unsigned have_previous_entry;
static volatile unsigned sample_valid;
static PwmPlan resume_plan;

#if POWER_MAIN_APPLICATION
/* STOP telemetry uses the late input sample. Other modes must qualify and
 * run on the original ADC1 phase, even if the unused ADC2 sample is lost. */
static volatile unsigned early_sample_valid;
static volatile float early_input_volts;
#define MODE_LIMIT(mode, name) (((mode) == 1U || (mode) == 3U) ? \
    POWER_MAIN_BUCK_##name : POWER_##name)
/* Fixed for an accepted request. Publish with active_mode/target before
 * start_pending, rather than choosing Buck/Boost constants in every IRQ. */
static struct {
    float INPUT_MIN_VOLTS, INPUT_MAX_VOLTS;
    float START_CURRENT_LIMIT_AMPS, START_OUTPUT_MAX_VOLTS;
    float output_trip_volts;
} active_limits;
#define IRQ_LIMIT(name) active_limits.name
#else
#define MODE_LIMIT(mode, name) POWER_##name
#define IRQ_LIMIT(name) POWER_##name
#endif

#if POWER_LATE_INPUT_ADC
static ADC_HandleTypeDef input_adc;
_Static_assert(POWER_INPUT_ADC_PHASE_TICKS > PWM_PERIOD_TICKS / 4U + 3U * 37U * 128U,
               "Input phase must follow the original ADC1 sequence");
_Static_assert(POWER_INPUT_ADC_PHASE_TICKS + 40U * 128U < PWM_PERIOD_TICKS,
               "Input conversion plus rounded trigger latency must fit before REP");

static HAL_StatusTypeDef configure_late_input(HRTIM_HandleTypeDef *hrtim)
{
    /* ADC1 MSP already enabled the shared ADC12 clock and PA0 analog input.
     * PA0 is ADC12_IN1: no GPIO remap, shared-clock reset or ADC1 deinit. */
    ADC_ChannelConfTypeDef channel = {0};
    HRTIM_CompareCfgTypeDef compare = {0};
    HRTIM_ADCTriggerCfgTypeDef trigger = {0};
    input_adc = (ADC_HandleTypeDef){0};
    input_adc.Instance = ADC2;
    input_adc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
    input_adc.Init.Resolution = ADC_RESOLUTION_12B;
    input_adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    input_adc.Init.ScanConvMode = ADC_SCAN_DISABLE;
    input_adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    input_adc.Init.NbrOfConversion = 1U;
    input_adc.Init.ExternalTrigConv = ADC_EXTERNALTRIG_HRTIM_TRG1;
    input_adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    input_adc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&input_adc) != HAL_OK ||
        HAL_ADCEx_Calibration_Start(&input_adc, ADC_SINGLE_ENDED) != HAL_OK)
        return HAL_ERROR;
    channel.Channel = ADC_CHANNEL_1;
    channel.Rank = ADC_REGULAR_RANK_1;
    channel.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
    channel.SingleDiff = ADC_SINGLE_ENDED;
    channel.OffsetNumber = ADC_OFFSET_NONE;
    if (HAL_ADC_ConfigChannel(&input_adc, &channel) != HAL_OK) return HAL_ERROR;
    compare.CompareValue = POWER_INPUT_ADC_PHASE_TICKS;
    if (HAL_HRTIM_WaveformCompareConfig(hrtim, TIMER_A, HRTIM_COMPAREUNIT_4,
                                       &compare) != HAL_OK) return HAL_ERROR;
    trigger.UpdateSource = HRTIM_ADCTRIGGERUPDATE_TIMER_A;
    trigger.Trigger = HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP4;
    if (HAL_HRTIM_ADCTriggerConfig(hrtim, HRTIM_ADCTRIGGER_1, &trigger) != HAL_OK)
        return HAL_ERROR;
    ADC2->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
    return HAL_ADC_Start(&input_adc);
}
#endif

#if POWER_UNIFIED_DIAGNOSTICS
volatile PowerDiagnostics power_diag;
static uint32_t diag_begin, diag_timer_active, diag_requested_mode;
static uint32_t diag_context = POWER_DIAG_CONTEXT_INIT;
static float diag_requested_target;

static inline void diag_checkpoint(unsigned checkpoint)
{
    power_diag.irq.checkpoint_cycles[checkpoint] = DWT->CYCCNT - diag_begin;
    power_diag.irq.valid_mask |= 1UL << checkpoint;
}

static void diag_capture(uint32_t fault, uint32_t detail, uint32_t elapsed,
                         uint32_t spacing, uint32_t counter)
{
    volatile PowerDiagFault *record = &power_diag.fault;
    uint32_t context = diag_context == POWER_DIAG_CONTEXT_HARDWARE_IRQ ?
        POWER_DIAG_CONTEXT_HARDWARE_IRQ :
        (diag_timer_active ? POWER_DIAG_CONTEXT_TIMER_IRQ : diag_context);
    record->capture_elapsed_cycles = diag_timer_active ? DWT->CYCCNT - diag_begin : 0U;
    record->fault = fault;
    record->detail = detail;
    record->state = power_status.state;
    record->context = context;
    record->start_pending = start_pending;
    record->bootstrap_remaining = power_status.state == POWER_BOOTSTRAP ? bootstrap_cycles : 0U;
    record->arming_remaining = power_status.state == POWER_ARMING ? arming_cycles : 0U;
    record->bootstrap_resume = bootstrap_resume;
    record->mode = context == POWER_DIAG_CONTEXT_START_REQUEST ? diag_requested_mode : active_mode;
    record->target_volts = context == POWER_DIAG_CONTEXT_START_REQUEST ? diag_requested_target : active_target;
    record->samples = power_status.samples;
    record->sample_fresh = diag_timer_active &&
        (power_diag.irq.valid_mask & (1UL << POWER_DIAG_CP_ADC)) ? 1U : 0U;
    record->raw_vh = power_status.raw_vh;
    record->raw_vl = power_status.raw_vl;
    record->raw_il = power_status.raw_il;
    record->v_left = power_status.v_left;
    record->v_right = power_status.v_right;
    record->il = power_status.il;
    record->timing_elapsed_cycles = elapsed;
    record->timing_spacing_cycles = spacing;
    record->timing_counter_ticks = counter;
    record->profile.stage = diag_timer_active ? power_diag.irq.stage : POWER_DIAG_STAGE_NONE;
    record->profile.entry_state = diag_timer_active ? power_diag.irq.entry_state : power_status.state;
    record->profile.entry_samples = diag_timer_active ? power_diag.irq.entry_samples : power_status.samples;
    record->profile.entry_spacing_cycles = diag_timer_active ? power_diag.irq.entry_spacing_cycles : 0U;
    record->profile.entry_counter_ticks = diag_timer_active ? power_diag.irq.entry_counter_ticks : 0U;
    uint32_t mask = diag_timer_active ? power_diag.irq.valid_mask : 0U;
    record->profile.valid_mask = mask;
    for (unsigned n = 0U; n < POWER_DIAG_CHECKPOINT_COUNT; ++n)
        record->profile.checkpoint_cycles[n] = (mask & (1UL << n)) ?
            power_diag.irq.checkpoint_cycles[n] : 0U;
}

#define DIAG_STAGE(value) (power_diag.irq.stage = (value))
#define DIAG_CHECKPOINT(value) diag_checkpoint(value)
#define DIAG_IRQ_RETURN() do { diag_timer_active = 0U; return; } while (0)
#define DIAG_REQUEST_RETURN(value) do { diag_context = POWER_DIAG_CONTEXT_OTHER; return (value); } while (0)
#else
#define DIAG_STAGE(value) ((void)0)
#define DIAG_CHECKPOINT(value) ((void)0)
#define DIAG_IRQ_RETURN() return
#define DIAG_REQUEST_RETURN(value) return (value)
#endif

int Power_IsStartPending(void)
{
    return start_pending != 0U;
}

int Power_SamplesAreValid(void)
{
    return sample_valid != 0U;
}

void Power_EmergencyOff(void)
{
    if ((RCC->APB2ENR & RCC_APB2ENR_HRTIM1EN) != 0U)
        HRTIM1->sCommonRegs.ODISR = ALL_OUTPUTS;
}

static void trip_record(uint32_t fault, uint32_t detail, uint32_t elapsed,
                        uint32_t spacing, uint32_t counter)
{
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    Power_EmergencyOff();
    if (power_status.fault == 0U) {
        power_status.fault_detail = detail;
        /* Retain the last complete sample at shutdown. Later fault-state
         * ADC/UI refresh must not erase transient evidence. */
        power_status.fault_v_left = power_status.v_left;
        power_status.fault_v_right = power_status.v_right;
        power_status.fault_il = power_status.il;
        power_status.fault_raw_vh = power_status.raw_vh;
        power_status.fault_raw_vl = power_status.raw_vl;
        power_status.fault_raw_il = power_status.raw_il;
        power_status.fault_input_volts = (fault & POWER_FAULT_INPUT) ?
            (active_mode >= 3U ? power_status.v_right : power_status.v_left) : 0.0f;
        power_status.fault_elapsed_cycles = elapsed;
        power_status.fault_spacing_cycles = spacing;
        power_status.fault_counter_ticks = counter;
#if POWER_UNIFIED_DIAGNOSTICS
        diag_capture(fault, detail, elapsed, spacing, counter);
#endif
    }
    power_status.fault |= fault;
    power_status.state = POWER_FAULT;
    start_pending = 0U;
    __set_PRIMASK(saved);
}

static void trip_detail(uint32_t fault, uint32_t detail)
{
    trip_record(fault, detail, 0U, 0U, 0U);
}

static void trip(uint32_t fault)
{
    trip_detail(fault, 0U);
}

static void timing_trip(uint32_t detail, uint32_t elapsed,
                        uint32_t spacing, uint32_t counter)
{
    /* Commit timing and sample evidence under the same first-fault lock. */
    trip_record(POWER_FAULT_TIMING, detail, elapsed, spacing, counter);
}

static void hardware_trip(void)
{
    /* Keep the sticky fault flag until an explicit, safe acknowledge. */
    PowerProtection_MaskIRQ();
    trip(POWER_FAULT_HARDWARE);
}

void Power_FaultIRQ(void)
{
#if POWER_UNIFIED_DIAGNOSTICS
    uint32_t interrupted_context = diag_context;
    diag_context = POWER_DIAG_CONTEXT_HARDWARE_IRQ;
#endif
    if (PowerProtection_FaultPending()) hardware_trip();
#if POWER_UNIFIED_DIAGNOSTICS
    diag_context = interrupted_context;
#endif
}

void Power_Stop(void)
{
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    Power_EmergencyOff();
    start_pending = 0U;
#if POWER_MAIN_APPLICATION
    power_status.d_cmd = power_status.v_reference = 0.0f;
    /* A later start must qualify on a fresh STOP sample, rather than the
     * preceding mode's ADC1-only phase or its ignored ADC2 status. */
    sample_valid = early_sample_valid = 0U;
#endif
    if (power_status.state != POWER_FAULT) power_status.state = POWER_STOPPED;
    __set_PRIMASK(saved);
}

void Power_ClearFault(void)
{
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    Power_EmergencyOff();
    start_pending = 0U;
#if POWER_MAIN_APPLICATION
    sample_valid = early_sample_valid = 0U;
#endif
    if (!PowerProtection_Acknowledge()) hardware_trip();
    else {
        power_status.fault = 0U;
        power_status.fault_detail = 0U;
        power_status.fault_elapsed_cycles = 0U;
        power_status.fault_spacing_cycles = 0U;
        power_status.fault_counter_ticks = 0U;
        power_status.fault_v_left = power_status.fault_v_right = power_status.fault_il = 0.0f;
        power_status.fault_input_volts = 0.0f;
        power_status.fault_raw_vh = power_status.fault_raw_vl = power_status.fault_raw_il = 0U;
        power_status.state = POWER_STOPPED;
#if POWER_UNIFIED_DIAGNOSTICS
        power_diag.fault = (PowerDiagFault){0};
#endif
    }
    __set_PRIMASK(saved);
}

static void write_plan(const PwmPlan *plan)
{
    /* Block both preload transfers during the paired write. No software
     * update in the running loop: both compare values latch at next period. */
    HRTIM1->sCommonRegs.CR1 |= HRTIM_CR1_TAUDIS | HRTIM_CR1_TBUDIS;
    HRTIM1->sTimerxRegs[TIMER_A].CMP1xR = plan->right_compare;
    HRTIM1->sTimerxRegs[TIMER_B].CMP1xR = plan->left_compare;
    HRTIM1->sCommonRegs.CR1 &= ~(HRTIM_CR1_TAUDIS | HRTIM_CR1_TBUDIS);
    power_status.left_compare = plan->left_compare;
    power_status.right_compare = plan->right_compare;
}

static int enable_outputs(uint32_t outputs)
{
    if (power_status.state == POWER_FAULT || power_status.fault) {
        Power_EmergencyOff();
        return 0;
    }
    if (PowerProtection_FaultPending()) {
        hardware_trip();
        return 0;
    }
    HRTIM1->sCommonRegs.OENR = outputs;
    /* A short pulse can finish after the precheck but before OEN. Its sticky
     * flag must close the outputs immediately, before any controller work
     * or state transition can proceed. An active hardware fault always wins. */
    if (PowerProtection_FaultPending()) {
        hardware_trip();
        return 0;
    }
    if (power_status.state == POWER_FAULT || power_status.fault) {
        Power_EmergencyOff();
        return 0;
    }
    return 1;
}

static int begin_bootstrap(unsigned resume)
{
    /* Both callers already have all outputs disabled (stopped or HOLD). */
    bootstrap_resume = resume;
    bootstrap_cycles = 30U; /* 200 us with high outputs disabled */
    HRTIM1->sTimerxRegs[TIMER_A].CMP1xR = PWM_MIN_COMPARE_TICKS;
    HRTIM1->sTimerxRegs[TIMER_B].CMP1xR = PWM_MIN_COMPARE_TICKS;
    power_status.state = POWER_BOOTSTRAP;
    return enable_outputs(LOW_OUTPUTS);
}

HAL_StatusTypeDef Power_Init(ADC_HandleTypeDef *adc, HRTIM_HandleTypeDef *hrtim)
{
    ADC_InjectionConfTypeDef config = {0};
    HRTIM_ADCTriggerCfgTypeDef trigger = {0};
    HRTIM_CompareCfgTypeDef compare = {0};
    Power_EmergencyOff();
#if POWER_UNIFIED_DIAGNOSTICS
    power_diag = (PowerDiagnostics){0};
    diag_timer_active = 0U;
    diag_context = POWER_DIAG_CONTEXT_INIT;
    power_diag.version = 1U;
    power_diag.cpu_hz = HAL_RCC_GetHCLKFreq();
    power_diag.pwm_hz = PWM_FREQUENCY_HZ;
    power_diag.write_budget_cycles = power_diag.cpu_hz / PWM_FREQUENCY_HZ - 160U;
    power_diag.exit_budget_cycles = power_diag.cpu_hz / PWM_FREQUENCY_HZ - 64U;
#endif
    if (adc->Instance != ADC1 || hrtim->Instance != HRTIM1 ||
        HAL_RCC_GetHCLKFreq() != 150000000U) return HAL_ERROR;
    power_status.state = POWER_STOPPED;
#if POWER_MAIN_APPLICATION
    sample_valid = early_sample_valid = 0U;
#endif
    if (PowerProtection_Init(hrtim) != HAL_OK) {
        trip(POWER_FAULT_HARDWARE);
        return HAL_ERROR;
    }
    power_status.protection_ready = (uint32_t)PowerProtection_IsReady();

    /* Injected order differs from the legacy regular scan: IL first,
     * then VH, VL. 3 x (24.5 + 12.5) / 37.5 MHz = 2.96 us.
     * Trigger at 25% period; data is read at the next period interrupt. */
    config.InjectedNbrOfConversion = 3U;
    config.InjectedSamplingTime = ADC_SAMPLETIME_24CYCLES_5;
    config.InjectedSingleDiff = ADC_SINGLE_ENDED;
    config.InjectedOffsetNumber = ADC_OFFSET_NONE;
    /* ADC1/2 injected groups support HRTIM_TRG2, not HRTIM_TRG1.
     * TRG1 selects TIM16_CC1 on ADC1/2 (RM0440 Table 167). */
    config.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJEC_HRTIM_TRG2;
    config.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONV_EDGE_RISING;
    config.QueueInjectedContext = DISABLE;
    config.AutoInjectedConv = DISABLE;
    config.InjectedChannel = ADC_CHANNEL_3;
    config.InjectedRank = ADC_INJECTED_RANK_1;
    if (HAL_ADCEx_InjectedConfigChannel(adc, &config) != HAL_OK) return HAL_ERROR;
    config.InjectedChannel = ADC_CHANNEL_1;
    config.InjectedRank = ADC_INJECTED_RANK_2;
    if (HAL_ADCEx_InjectedConfigChannel(adc, &config) != HAL_OK) return HAL_ERROR;
    config.InjectedChannel = ADC_CHANNEL_2;
    config.InjectedRank = ADC_INJECTED_RANK_3;
    if (HAL_ADCEx_InjectedConfigChannel(adc, &config) != HAL_OK) return HAL_ERROR;

    compare.CompareValue = PWM_PERIOD_TICKS / 4U;
    if (HAL_HRTIM_WaveformCompareConfig(hrtim, TIMER_A, HRTIM_COMPAREUNIT_2,
                                       &compare) != HAL_OK) return HAL_ERROR;
    trigger.UpdateSource = HRTIM_ADCTRIGGERUPDATE_TIMER_A;
    trigger.Trigger = HRTIM_ADCTRIGGEREVENT24_TIMERA_CMP2;
    if (HAL_HRTIM_ADCTriggerConfig(hrtim, HRTIM_ADCTRIGGER_2, &trigger) != HAL_OK)
        return HAL_ERROR;
#if POWER_LATE_INPUT_ADC
    if (configure_late_input(hrtim) != HAL_OK) return HAL_ERROR;
#endif
    /* Flush initial preloads before starting both counters in one MCR write. */
    HRTIM1->sCommonRegs.CR2 = HRTIM_CR2_TASWU | HRTIM_CR2_TBSWU |
                             HRTIM_CR2_TARST | HRTIM_CR2_TBRST;
    ADC1->ISR = ADC_ISR_JEOS | ADC_ISR_JEOC | ADC_ISR_JQOVF;
    if (HAL_ADCEx_InjectedStart(adc) != HAL_OK) return HAL_ERROR;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    cycle_budget = HAL_RCC_GetHCLKFreq() / PWM_FREQUENCY_HZ;
    HRTIM1->sTimerxRegs[TIMER_B].TIMxICR = HRTIM_TIMICR_REPC;
    HRTIM1->sTimerxRegs[TIMER_B].TIMxDIER |= HRTIM_TIMDIER_REPIE;
    HAL_NVIC_SetPriority(HRTIM1_TIMB_IRQn, 0U, 0U);
    HAL_NVIC_ClearPendingIRQ(HRTIM1_TIMB_IRQn);
    HAL_NVIC_EnableIRQ(HRTIM1_TIMB_IRQn);
    HAL_StatusTypeDef result = HAL_HRTIM_WaveformCounterStart(hrtim, HRTIM_TIMERID_TIMER_A |
                                                            HRTIM_TIMERID_TIMER_B);
#if POWER_UNIFIED_DIAGNOSTICS
    diag_context = POWER_DIAG_CONTEXT_OTHER;
#endif
    return result;
}

int Power_RequestStart(unsigned mode, float target_volts)
{
#if POWER_UNIFIED_DIAGNOSTICS
    diag_context = POWER_DIAG_CONTEXT_START_REQUEST;
    diag_requested_mode = mode;
    diag_requested_target = target_volts;
#endif
#if POWER_MAIN_APPLICATION
    /* A zero Buck target is an explicit all-gates-off request, including
     * during preparation/running/fault. It never acknowledges a fault. */
    if ((mode == 1U || mode == 3U) && target_volts == 0.0f) {
        Power_Stop();
        DIAG_REQUEST_RETURN(1);
    }
#endif
    int reverse = mode == 3U || mode == 4U;
    int boost = mode == 2U || mode == 4U;
    float left = power_status.v_left;
#if POWER_MAIN_APPLICATION
    if (mode != 1U) left = early_input_volts;
#endif
    float vin = reverse ? power_status.v_right : left;
    float vout = reverse ? left : power_status.v_right;
    if (power_status.state != POWER_STOPPED || start_pending != 0U) DIAG_REQUEST_RETURN(0);
    power_status.protection_ready = (uint32_t)PowerProtection_IsReady();
    if (!power_status.protection_ready && !POWER_BENCH_12V_BUCK_TEST &&
        !(POWER_MAIN_APPLICATION && POWER_MAIN_ADC_SOFTWARE_ONLY)) {
        trip(POWER_FAULT_PROTECTION);
        DIAG_REQUEST_RETURN(0);
    }
    if (PowerProtection_FaultPending()) {
        hardware_trip();
        DIAG_REQUEST_RETURN(0);
    }
    uint32_t rejection = 0U;
    if (mode < 1U || mode > 4U || (POWER_BENCH_12V_BUCK_TEST && mode != 1U))
        rejection = 1U;
    else if (power_status.samples < 2U ||
#if POWER_MAIN_APPLICATION
             !(mode == 1U ? sample_valid : early_sample_valid)
#else
             !sample_valid
#endif
            ) rejection = 2U;
    else if (!(target_volts >= 1.0f && target_volts <= MODE_LIMIT(mode, MAX_TARGET_VOLTS))) rejection = 3U;
    else if (!(vin >= MODE_LIMIT(mode, INPUT_MIN_VOLTS) && vin <= MODE_LIMIT(mode, INPUT_MAX_VOLTS))) rejection = 4U;
    else if (!(fabsf(power_status.il) < MODE_LIMIT(mode, START_CURRENT_LIMIT_AMPS))) rejection = 5U;
    /* A charged output can drive reverse current at cold-start duty. */
    else if (vout > MODE_LIMIT(mode, START_OUTPUT_MAX_VOLTS)) rejection = 6U;
    else if ((boost && vin >= target_volts) || (!boost && vin <= target_volts)) rejection = 7U;
#if POWER_MAIN_APPLICATION
    else if (mode == 1U && target_volts > vin * POWER_MAIN_BUCK_MAX_DUTY) rejection = 7U;
#endif
    if (rejection) {
        trip_detail(POWER_FAULT_START, rejection);
        DIAG_REQUEST_RETURN(0);
    }
    active_mode = mode;
    active_target = target_volts;
#if POWER_MAIN_APPLICATION
    active_limits.INPUT_MIN_VOLTS = MODE_LIMIT(mode, INPUT_MIN_VOLTS);
    active_limits.INPUT_MAX_VOLTS = MODE_LIMIT(mode, INPUT_MAX_VOLTS);
    active_limits.START_CURRENT_LIMIT_AMPS = MODE_LIMIT(mode, START_CURRENT_LIMIT_AMPS);
    active_limits.START_OUTPUT_MAX_VOLTS = MODE_LIMIT(mode, START_OUTPUT_MAX_VOLTS);
    active_limits.output_trip_volts = target_volts * MODE_LIMIT(mode, OUTPUT_OVERVOLTAGE_RATIO);
#endif
    if (reverse) {
        reverse_configure(boost ? vin : 0.1f, target_volts, POWER_CURRENT_REFERENCE_LIMIT);
        reverse_initialize(0.0f);
        reverse_U.mode = (float)mode;
    } else {
        forward_configure(boost ? vin : 0.1f, target_volts, POWER_CURRENT_REFERENCE_LIMIT);
        forward_initialize(0.0f);
        forward_U.mode = (float)mode;
    }
    bootstrap_resume = transition_ready = 0U;
    reverse_polarity_cycles = 0U;
    power_status.isr_cycles_max = 0U;
    /* A hardware IRQ may have interrupted the foreground controller init.
     * Commit the request only if the stopped state and fault source remain safe. */
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    if (PowerProtection_FaultPending()) hardware_trip();
    if (power_status.state != POWER_STOPPED) {
        __set_PRIMASK(saved);
        DIAG_REQUEST_RETURN(0);
    }
    __DMB();
    start_pending = 1U;
    __set_PRIMASK(saved);
    DIAG_REQUEST_RETURN(1);
}

void Power_Service(AdcMeasurements *measurements)
{
    uint16_t vh, vl, il;
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    vh = power_status.raw_vh;
    vl = power_status.raw_vl;
    il = power_status.raw_il;
    __set_PRIMASK(saved);
    ADC_ConvertRaw(vh, vl, il, measurements);
    uint32_t command = power_command;
    power_command = POWER_CMD_NONE;
    if (command == POWER_CMD_STOP) Power_Stop();
    else if (command == POWER_CMD_CLEAR_FAULT) Power_ClearFault();
    else if (command == POWER_CMD_START_BOOST)
        (void)Power_RequestStart(2U, power_target_volts);
    else if (command == POWER_CMD_START_BUCK)
        (void)Power_RequestStart(1U, power_target_volts);
    else if (command == POWER_CMD_START_REVERSE_BUCK)
        (void)Power_RequestStart(3U, power_target_volts);
    else if (command == POWER_CMD_START_REVERSE_BOOST)
        (void)Power_RequestStart(4U, power_target_volts);
}

void Power_TimerIRQ(void)
{
    const uint32_t begin = DWT->CYCCNT;
#if POWER_UNIFIED_DIAGNOSTICS
    diag_begin = begin;
    diag_timer_active = 1U;
    power_diag.irq.stage = POWER_DIAG_STAGE_ENTRY;
    power_diag.irq.entry_state = power_status.state;
    power_diag.irq.entry_samples = power_status.samples;
    power_diag.irq.entry_spacing_cycles = 0U;
    power_diag.irq.entry_counter_ticks = 0U;
    power_diag.irq.valid_mask = 0U;
#endif
    uint32_t flags = ADC1->ISR;
    if (!(HRTIM1->sTimerxRegs[TIMER_B].TIMxISR & HRTIM_TIMISR_REP)) DIAG_IRQ_RETURN();
    HRTIM1->sTimerxRegs[TIMER_B].TIMxICR = HRTIM_TIMICR_REPC;
#if POWER_MAIN_APPLICATION
    uint32_t input_flags = ADC2->ISR;
    uint16_t late_input_raw = (uint16_t)ADC2->DR;
    unsigned late_valid = (input_flags & (ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR)) ==
                          (ADC_ISR_EOC | ADC_ISR_EOS);
    unsigned use_late_input = (power_status.state == POWER_STOPPED && !start_pending) ||
                              (active_mode == 1U && power_status.state != POWER_FAULT);
    /* ADC2 runs in every mode. Drain it even on timing/ADC1 failure, so a
     * later Buck request cannot accept an earlier period's sticky result. */
    ADC2->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
#endif
    /* Analog hardware has already forced gates off. Catch even a short
     * pulse before sampling/arming if its dedicated IRQ has not run yet. */
    if (PowerProtection_FaultPending()) hardware_trip();
    /* REP and JEOS are sticky: they cannot count missed periods. Compare
     * successive entry timestamps as well as checking execution duration. */
    uint32_t spacing = begin - previous_entry;
    previous_entry = begin;
    uint32_t entry_counter = HRTIM1->sTimerxRegs[TIMER_B].CNTxR;
#if POWER_UNIFIED_DIAGNOSTICS
    power_diag.irq.entry_spacing_cycles = spacing;
    power_diag.irq.entry_counter_ticks = entry_counter;
#endif
    if (have_previous_entry && power_status.state != POWER_STOPPED &&
        power_status.state != POWER_FAULT &&
        !PWM_EntryIsTimely(spacing, entry_counter)) {
        /* Same 1256-cycle spacing threshold as PWM_EntryIsTimely(). */
        timing_trip(spacing > 1256U ? 1U : 2U,
                    DWT->CYCCNT - begin, spacing, entry_counter);
        DIAG_IRQ_RETURN();
    }
    have_previous_entry = 1U;
    DIAG_CHECKPOINT(POWER_DIAG_CP_ENTRY);
    DIAG_STAGE(POWER_DIAG_STAGE_ADC);
    if (!(flags & ADC_ISR_JEOS) || (flags & ADC_ISR_JQOVF)
#if POWER_LATE_INPUT_ADC && !POWER_MAIN_APPLICATION
        || (ADC2->ISR & (ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR)) !=
           (ADC_ISR_EOC | ADC_ISR_EOS)
#endif
       ) {
        sample_valid = 0U;
#if POWER_MAIN_APPLICATION
        early_sample_valid = 0U;
#endif
        if (start_pending || (power_status.state != POWER_STOPPED &&
                              power_status.state != POWER_FAULT))
            trip(POWER_FAULT_ADC);
        ADC1->ISR = ADC_ISR_JQOVF | ADC_ISR_JEOS | ADC_ISR_JEOC;
#if POWER_LATE_INPUT_ADC && !POWER_MAIN_APPLICATION
        ADC2->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
#endif
        DIAG_IRQ_RETURN();
    }
#if POWER_MAIN_APPLICATION
    early_input_volts = (float)(uint16_t)ADC1->JDR2 * ADC_LEFT_VOLTS_PER_COUNT;
    early_sample_valid = 1U;
    if (use_late_input && !late_valid &&
        (start_pending || (power_status.state != POWER_STOPPED && power_status.state != POWER_FAULT))) {
        sample_valid = 0U;
        trip(POWER_FAULT_ADC);
        ADC1->ISR = ADC_ISR_JEOS | ADC_ISR_JEOC;
        DIAG_IRQ_RETURN();
    }
#endif
    power_status.raw_il = (uint16_t)ADC1->JDR1;
#if POWER_MAIN_APPLICATION
    /* Invalid STOP late input stays visibly invalid. Preserve the previous
     * VH value while retaining fresh ADC1 current/right bus for other modes. */
    if (!use_late_input) power_status.raw_vh = (uint16_t)ADC1->JDR2;
    else if (late_valid) power_status.raw_vh = late_input_raw;
#elif POWER_LATE_INPUT_ADC
    power_status.raw_vh = (uint16_t)ADC2->DR;
    /* DR consumes EOC; clear EOS as well so no previous result is accepted.
     * Never fall back to the earlier ADC1 JDR2 value if this sample is lost. */
    ADC2->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
#else
    power_status.raw_vh = (uint16_t)ADC1->JDR2;
#endif
    power_status.raw_vl = (uint16_t)ADC1->JDR3;
    ADC1->ISR = ADC_ISR_JEOS | ADC_ISR_JEOC;
    power_status.il = (float)power_status.raw_il * ADC_CURRENT_AMPS_PER_COUNT -
                      ADC_CURRENT_ZERO_AMPS;
    power_status.v_left = (float)power_status.raw_vh * ADC_LEFT_VOLTS_PER_COUNT;
    power_status.v_right = (float)power_status.raw_vl * ADC_RIGHT_VOLTS_PER_COUNT;
    ++power_status.samples;
#if POWER_MAIN_APPLICATION
    sample_valid = !use_late_input || late_valid;
#else
    sample_valid = 1U;
#endif
    DIAG_CHECKPOINT(POWER_DIAG_CP_ADC);
    if (power_status.state == POWER_FAULT) DIAG_IRQ_RETURN();
    unsigned starting = start_pending;
    if (power_status.state == POWER_STOPPED && !starting) DIAG_IRQ_RETURN();
    DIAG_STAGE(POWER_DIAG_STAGE_CHECKS);
    int reverse = active_mode >= 3U;
    float vin = reverse ? power_status.v_right : power_status.v_left;
    float vout = reverse ? power_status.v_left : power_status.v_right;
    if (!(vin >= IRQ_LIMIT(INPUT_MIN_VOLTS) &&
          vin <= IRQ_LIMIT(INPUT_MAX_VOLTS)))
        trip(POWER_FAULT_INPUT);
#if POWER_MAIN_APPLICATION
    if (active_mode == 1U && active_target > vin * POWER_MAIN_BUCK_MAX_DUTY)
        trip(POWER_FAULT_INPUT);
#endif
#if POWER_MAIN_APPLICATION
    if (vout > active_limits.output_trip_volts)
#else
    if (vout > active_target * POWER_OUTPUT_OVERVOLTAGE_RATIO)
#endif
        trip(POWER_FAULT_OUTPUT);
    if (fabsf(power_status.il) >= POWER_CURRENT_TRIP_AMPS || power_status.raw_il >= 4090U ||
        power_status.raw_il <= 5U) trip(POWER_FAULT_CURRENT);
    /* Synchronous reverse Boost can briefly return ripple current to its
     * source when resuming from zero current. Qualify direction over 16
     * samples (107 us); absolute overcurrent above remains immediate. */
    if (reverse) {
        reverse_polarity_cycles = power_status.il > 1.0f ? reverse_polarity_cycles + 1U : 0U;
        if (reverse_polarity_cycles >= 16U) trip(POWER_FAULT_REVERSE);
    } else if (power_status.il < -1.0f) trip(POWER_FAULT_REVERSE);
    /* Request acceptance precedes several fresh samples. Revalidate cold
     * output/current before ANY initial enable and throughout cold arming.
     * HOLD resume deliberately has charged output and is excluded. */
    if ((starting || (!bootstrap_resume &&
         (power_status.state == POWER_BOOTSTRAP || power_status.state == POWER_ARMING))) &&
        (!(fabsf(power_status.il) < IRQ_LIMIT(START_CURRENT_LIMIT_AMPS)) ||
         vout > IRQ_LIMIT(START_OUTPUT_MAX_VOLTS)))
        trip_detail(POWER_FAULT_START,
                    !(fabsf(power_status.il) < IRQ_LIMIT(START_CURRENT_LIMIT_AMPS)) ? 8U : 9U);
    if (power_status.state == POWER_FAULT) DIAG_IRQ_RETURN();
    DIAG_CHECKPOINT(POWER_DIAG_CP_CHECKS);
    DIAG_STAGE(POWER_DIAG_STAGE_STATE);
    if (starting) {
        start_pending = 0U;
        if (!begin_bootstrap(0U)) DIAG_IRQ_RETURN();
    }

    if (power_status.state == POWER_BOOTSTRAP) {
        /* The IRQ which enables the low sides starts interval one; do not
         * count an interval before it has elapsed. HOLD resume is started
         * at IRQ end and follows the same 30 complete period requirement. */
        if (starting) {
            DIAG_CHECKPOINT(POWER_DIAG_CP_STATE);
            DIAG_IRQ_RETURN();
        }
        if (--bootstrap_cycles == 0U) {
            PwmPlan initial;
            if (bootstrap_resume) initial = resume_plan;
            else if (reverse) (void)PWM_MakeReversePlan(0.0f, 0, &initial);
            else (void)PWM_MakePlan(0.0f, 0, &initial);
            write_plan(&initial);
            arming_cycles = 2U;
            power_status.state = POWER_ARMING;
        }
        DIAG_CHECKPOINT(POWER_DIAG_CP_STATE);
        DIAG_IRQ_RETURN();
    }
    if (power_status.state == POWER_ARMING) {
        if (--arming_cycles != 0U) {
            DIAG_CHECKPOINT(POWER_DIAG_CP_STATE);
            DIAG_IRQ_RETURN();
        }
        if (!enable_outputs(ALL_OUTPUTS)) DIAG_IRQ_RETURN();
        power_status.state = POWER_RUNNING;
    }
    if (power_status.state == POWER_CHANGEOVER && transition_ready) {
        /* A valid boost plan was written on the previous IRQ and has now
         * crossed the common preload boundary before enabling all gates. */
        if (!enable_outputs(ALL_OUTPUTS)) DIAG_IRQ_RETURN();
        power_status.state = POWER_RUNNING;
    }
    if (reverse && power_status.state == POWER_RUNNING && reverse_Y.CTRL_RUN_R &&
        vout >= active_target * 1.005f) {
        Power_EmergencyOff();
        power_status.state = POWER_HOLD;
    }
    PwmPlan plan;
    int resuming = power_status.state == POWER_HOLD &&
                   vout < active_target * 0.99f && fabsf(power_status.il) < 0.25f;
    DIAG_CHECKPOINT(POWER_DIAG_CP_STATE);
    DIAG_STAGE(POWER_DIAG_STAGE_CONTROLLER);
    if (reverse) {
        reverse_U.IL = power_status.il;
        reverse_U.V_left = power_status.v_left;
        reverse_U.V_right = power_status.v_right;
        reverse_U.mode = (float)active_mode;
        if (resuming) reverse_prepare_resume(vout);
        if (power_status.state == POWER_HOLD && !resuming) reverse_hold_step();
        else reverse_step();
        DIAG_CHECKPOINT(POWER_DIAG_CP_CONTROLLER);
        DIAG_STAGE(POWER_DIAG_STAGE_MAP);
        if (reverse_errorStatus || !PWM_MakeReversePlan(reverse_Y.D_cm_R,
                                                        reverse_Y.BOOST_RUN_R, &plan)) {
            trip(POWER_FAULT_CONTROLLER);
            DIAG_IRQ_RETURN();
        }
        if (active_mode == 4U && reverse_Y.D_cm_R4 && !reverse_Y.BOOST_RUN_R) {
            Power_EmergencyOff();
            power_status.state = POWER_CHANGEOVER;
            transition_ready = 0U;
        }
        if (power_status.state == POWER_CHANGEOVER && reverse_Y.BOOST_RUN_R)
            transition_ready = 1U;
        power_status.d_cmd = reverse_Y.D_cm_R;
        power_status.v_reference = reverse_B.Saturation4;
    } else {
        forward_U.IL = power_status.il;
        forward_U.V_left = power_status.v_left;
        forward_U.V_right = power_status.v_right;
        forward_U.mode = (float)active_mode;
        forward_step();
        DIAG_CHECKPOINT(POWER_DIAG_CP_CONTROLLER);
        DIAG_STAGE(POWER_DIAG_STAGE_MAP);
        if (forward_errorStatus ||
#if POWER_MAIN_APPLICATION
            !(active_mode == 1U ? PWM_MakeBuckCompares(forward_Y.D_cmd, &plan.left_compare, &plan.right_compare) :
              PWM_MakePlan(forward_Y.D_cmd, forward_Y.BOOST_RUN, &plan))) {
#else
            !PWM_MakePlan(forward_Y.D_cmd, forward_Y.BOOST_RUN, &plan)) {
#endif
            trip(POWER_FAULT_CONTROLLER);
            DIAG_IRQ_RETURN();
        }
        power_status.d_cmd = forward_Y.D_cmd;
        power_status.v_reference = forward_B.Saturation4;
    }
    DIAG_CHECKPOINT(POWER_DIAG_CP_MAP);
    DIAG_STAGE(POWER_DIAG_STAGE_PREWRITE);
    /* Do not write either preload close to the common update boundary. */
    uint32_t before_write = DWT->CYCCNT - begin;
    uint32_t write_counter = HRTIM1->sTimerxRegs[TIMER_B].CNTxR;
    if (before_write > cycle_budget - 160U || write_counter > PWM_PERIOD_TICKS - 5120U) {
        timing_trip(before_write > cycle_budget - 160U ? 3U : 4U,
                    before_write, spacing, write_counter);
        DIAG_IRQ_RETURN();
    }
    DIAG_STAGE(POWER_DIAG_STAGE_WRITE);
    if (power_status.state == POWER_HOLD) {
        /* Keep reference/filters alive without winding up PIs while off.
         * Refresh bootstrap before resuming the retained operating mode. */
        if (resuming) {
            /* Only compare registers are consumed on resume. Do not retain
             * unused duty fields: that makes them escape the inline mapper
             * and forces intermediate stores on the normal Buck path too. */
            resume_plan.left_compare = plan.left_compare;
            resume_plan.right_compare = plan.right_compare;
            if (!begin_bootstrap(1U)) DIAG_IRQ_RETURN();
        }
    } else write_plan(&plan);
    DIAG_CHECKPOINT(POWER_DIAG_CP_WRITE);
    DIAG_STAGE(POWER_DIAG_STAGE_EXIT);
    uint32_t elapsed = DWT->CYCCNT - begin;
    if (elapsed > power_status.isr_cycles_max) power_status.isr_cycles_max = elapsed;
    if (elapsed > cycle_budget - 64U)
        timing_trip(5U, elapsed, spacing, HRTIM1->sTimerxRegs[TIMER_B].CNTxR);
    else if (HRTIM1->sTimerxRegs[TIMER_B].TIMxISR & HRTIM_TIMISR_REP)
        timing_trip(6U, elapsed, spacing, HRTIM1->sTimerxRegs[TIMER_B].CNTxR);
#if POWER_UNIFIED_DIAGNOSTICS
    diag_timer_active = 0U;
#endif
}
