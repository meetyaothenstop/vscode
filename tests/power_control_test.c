/* Host integration of the real backend, controllers, PWM mapper and ADC
 * conversion. Register events are synthetic; no hardware is accessed.
 * Example build:
 * gcc -std=c11 -O2 -Wall -Wextra -Werror -Itests/power_hal -ICore/Inc 
 *   tests/power_control_test.c Core/Src/power_control.c Core/Src/forward.c
 *   Core/Src/reverse.c Core/Src/pwm_plan.c Core/Src/adc_measure.c -lm
 */
#include "power_control.h"
#include "forward.h"
#include "reverse.h"
#include "power_build_config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(expression) do { if (!(expression)) { \
    fprintf(stderr, "%s:%d: %s (state=%u, fault=0x%lx, outputs=0x%lx)\n", \
        __FILE__, __LINE__, #expression, (unsigned)power_status.state, \
        (unsigned long)power_status.fault, (unsigned long)enabled_outputs); \
    exit(EXIT_FAILURE); \
} } while (0)
#define ALL_OUTPUTS (HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2)
#define LOW_OUTPUTS (HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB2)
#define TIMER_A HRTIM_TIMERINDEX_TIMER_A
#define TIMER_B HRTIM_TIMERINDEX_TIMER_B

ADC_TypeDef test_adc1;
ADC_TypeDef test_adc2;
HRTIM_TypeDef test_hrtim1;
RCC_TypeDef test_rcc;
CoreDebug_Type test_core_debug;
DWT_Type test_dwt;
uint32_t test_primask;
static uint32_t enabled_outputs, active_left_compare, active_right_compare;
static unsigned adc_channels[3], adc_config_count, counters_started;
static unsigned protection_init_calls, protection_ready = 1U, protection_irq_masked;
static uint32_t protection_latched, protection_active;
static unsigned protection_fault_on_poll;
static unsigned protection_fault_after_safe_poll;
static uint32_t advance_cycles_on_poll;

#if POWER_BENCH_LATE_INPUT_ADC
static unsigned input_init_step, input_fail_step, input_started;
static uint32_t next_input_flags = ADC_ISR_EOC | ADC_ISR_EOS;
static uint16_t early_input_raw;
static unsigned override_early_input;
static HAL_StatusTypeDef input_init_result(void)
{ return ++input_init_step == input_fail_step ? HAL_ERROR : HAL_OK; }

HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *adc)
{
    CHECK(adc->Instance == ADC2);
    CHECK(adc->Init.ClockPrescaler == ADC_CLOCK_ASYNC_DIV4);
    CHECK(adc->Init.Resolution == ADC_RESOLUTION_12B);
    CHECK(adc->Init.DataAlign == ADC_DATAALIGN_RIGHT);
    CHECK(adc->Init.ScanConvMode == ADC_SCAN_DISABLE && adc->Init.NbrOfConversion == 1U);
    CHECK(adc->Init.ContinuousConvMode == DISABLE && adc->Init.LowPowerAutoWait == DISABLE);
    CHECK(adc->Init.DMAContinuousRequests == DISABLE && adc->Init.OversamplingMode == DISABLE);
    CHECK(adc->Init.ExternalTrigConv == ADC_EXTERNALTRIG_HRTIM_TRG1);
    CHECK(adc->Init.ExternalTrigConvEdge == ADC_EXTERNALTRIGCONVEDGE_RISING);
    CHECK(adc->Init.Overrun == ADC_OVR_DATA_PRESERVED);
    return input_init_result();
}
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *adc, uint32_t mode)
{
    CHECK(adc->Instance == ADC2 && mode == ADC_SINGLE_ENDED && input_init_step == 1U);
    return input_init_result();
}
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *adc, ADC_ChannelConfTypeDef *cfg)
{
    CHECK(adc->Instance == ADC2 && input_init_step == 2U);
    CHECK(cfg->Channel == ADC_CHANNEL_1 && cfg->Rank == ADC_REGULAR_RANK_1);
    CHECK(cfg->SamplingTime == ADC_SAMPLETIME_24CYCLES_5);
    CHECK(cfg->SingleDiff == ADC_SINGLE_ENDED && cfg->OffsetNumber == ADC_OFFSET_NONE);
    return input_init_result();
}
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *adc)
{
    CHECK(adc->Instance == ADC2 && input_init_step == 5U);
    HAL_StatusTypeDef result = input_init_result();
    if (result == HAL_OK) input_started = 1U;
    return result;
}
#endif

#if defined(POWER_UNIFIED_DIAGNOSTICS) && POWER_UNIFIED_DIAGNOSTICS
/* Hardware-only double: advance the real backend's CYCCNT reads. A delayed
 * checkpoint models time spent in that section; controllers remain real. */
static uint32_t profile_tick_per_read, profile_delay_stage, profile_delay_cycles;
DWT_Type *test_dwt_access(void)
{
    test_dwt.CYCCNT += profile_tick_per_read;
    if (profile_delay_cycles && power_diag.irq.stage == profile_delay_stage) {
        test_dwt.CYCCNT += profile_delay_cycles;
        profile_delay_cycles = 0U;
    }
    return &test_dwt;
}
#endif

HAL_StatusTypeDef PowerProtection_Init(HRTIM_HandleTypeDef *timer)
{
    CHECK(timer->Instance == HRTIM1);
    ++protection_init_calls;
    return HAL_OK;
}
int PowerProtection_IsReady(void) { return (int)protection_ready; }
uint32_t PowerProtection_FaultPending(void)
{
    if (advance_cycles_on_poll) {
        DWT->CYCCNT += advance_cycles_on_poll;
        advance_cycles_on_poll = 0U;
    }
    if (protection_fault_on_poll && --protection_fault_on_poll == 0U)
        protection_latched |= 8U;
    uint32_t result = protection_latched | protection_active;
    if (protection_fault_after_safe_poll && --protection_fault_after_safe_poll == 0U) {
        protection_latched |= 8U;
        enabled_outputs = 0U; /* Hardware reacted after the register snapshot. */
    }
    return result;
}
void PowerProtection_MaskIRQ(void) { protection_irq_masked = 1U; }
int PowerProtection_Acknowledge(void)
{
    if (protection_active) return 0;
    protection_latched = 0U;
    protection_irq_masked = 0U;
    return 1;
}

uint32_t HAL_RCC_GetHCLKFreq(void) { return 150000000U; }
HAL_StatusTypeDef HAL_ADCEx_InjectedConfigChannel(ADC_HandleTypeDef *adc, ADC_InjectionConfTypeDef *cfg)
{
    CHECK(adc->Instance == ADC1 && cfg->InjectedRank >= 1U && cfg->InjectedRank <= 3U);
    CHECK(cfg->InjectedNbrOfConversion == 3U);
    /* RM0440: ADC1 injected conversions cannot use HRTIM_TRG1 (ADC3/4/5
     * only). Reject that route even though the host can synthesize JEOS. */
    CHECK(cfg->ExternalTrigInjecConv == ADC_EXTERNALTRIGINJEC_HRTIM_TRG2);
    CHECK(cfg->ExternalTrigInjecConvEdge == ADC_EXTERNALTRIGINJECCONV_EDGE_RISING);
    adc_channels[cfg->InjectedRank - 1U] = cfg->InjectedChannel;
    ++adc_config_count;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_ADCEx_InjectedStart(ADC_HandleTypeDef *adc)
{ CHECK(adc->Instance == ADC1); return HAL_OK; }
HAL_StatusTypeDef HAL_HRTIM_WaveformCompareConfig(HRTIM_HandleTypeDef *timer, uint32_t index,
    uint32_t unit, HRTIM_CompareCfgTypeDef *cfg)
{
    CHECK(timer->Instance == HRTIM1 && index == TIMER_A);
#if POWER_BENCH_LATE_INPUT_ADC
    if (unit == HRTIM_COMPAREUNIT_4) {
        CHECK(input_init_step == 3U && cfg->CompareValue == 23040U);
        HRTIM1->sTimerxRegs[index].CMP4xR = cfg->CompareValue;
        return input_init_result();
    }
#endif
    CHECK(unit == HRTIM_COMPAREUNIT_2);
    HRTIM1->sTimerxRegs[index].CMP2xR = cfg->CompareValue;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_HRTIM_ADCTriggerConfig(HRTIM_HandleTypeDef *timer, uint32_t index,
    HRTIM_ADCTriggerCfgTypeDef *cfg)
{
    CHECK(timer->Instance == HRTIM1);
    CHECK(cfg->UpdateSource == HRTIM_ADCTRIGGERUPDATE_TIMER_A);
#if POWER_BENCH_LATE_INPUT_ADC
    if (index == HRTIM_ADCTRIGGER_1) {
        CHECK(input_init_step == 4U && cfg->Trigger == HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP4);
        return input_init_result();
    }
#endif
    CHECK(index == HRTIM_ADCTRIGGER_2);
    CHECK(cfg->Trigger == HRTIM_ADCTRIGGEREVENT24_TIMERA_CMP2);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_HRTIM_WaveformCounterStart(HRTIM_HandleTypeDef *timer, uint32_t counters)
{
    CHECK(timer->Instance == HRTIM1);
#if POWER_BENCH_LATE_INPUT_ADC
    CHECK(input_started && input_init_step == 6U);
#endif
    counters_started = counters;
    return HAL_OK;
}
void HAL_NVIC_SetPriority(int irq, uint32_t preempt, uint32_t sub)
{ CHECK(irq == HRTIM1_TIMB_IRQn && preempt == 0U && sub == 0U); }
void HAL_NVIC_ClearPendingIRQ(int irq) { CHECK(irq == HRTIM1_TIMB_IRQn); }
void HAL_NVIC_EnableIRQ(int irq) { CHECK(irq == HRTIM1_TIMB_IRQn); }

/* OENR/ODISR are write strobes on silicon. Clear their last observed writes
 * before each API/IRQ and apply the new writes to a separate effective mask.
 * A simultaneous disable always wins in this fixture. */
static void before_call(void)
{
    HRTIM1->sCommonRegs.OENR = 0U;
    HRTIM1->sCommonRegs.ODISR = 0U;
}
static void after_call(void)
{
    enabled_outputs |= HRTIM1->sCommonRegs.OENR;
    enabled_outputs &= ~HRTIM1->sCommonRegs.ODISR;
    CHECK(test_primask == 0U);
    CHECK((HRTIM1->sCommonRegs.CR1 & (HRTIM_CR1_TAUDIS | HRTIM_CR1_TBUDIS)) == 0U);
}
static uint16_t raw_voltage(float volts, float gain)
{ return (uint16_t)lroundf(volts * 4095.0f * gain / 3.31f); }
static uint16_t raw_current(float amps)
{ return (uint16_t)lroundf((amps * 0.1f + 2.49f) * 4095.0f / 3.31f); }

static void irq(float left, float right, float il, uint32_t adc_flags,
                uint32_t elapsed_cycles, uint32_t counter_ticks)
{
    /* The next period's hardware preload update happens before its IRQ.
     * CYCCNT moves by one 150 kHz period, while execution itself is free
     * in this register fixture; elapsed ISR timing needs board validation. */
    active_left_compare = HRTIM1->sTimerxRegs[TIMER_B].CMP1xR;
    active_right_compare = HRTIM1->sTimerxRegs[TIMER_A].CMP1xR;
    DWT->CYCCNT += elapsed_cycles;
    HRTIM1->sTimerxRegs[TIMER_B].CNTxR = counter_ticks;
    HRTIM1->sTimerxRegs[TIMER_B].TIMxISR = HRTIM_TIMISR_REP;
    ADC1->JDR1 = raw_current(il);
    ADC1->JDR2 = raw_voltage(left, 0.0461f);
#if POWER_BENCH_LATE_INPUT_ADC
    if (override_early_input) {
        ADC1->JDR2 = early_input_raw;
        override_early_input = 0U;
    }
    ADC2->DR = raw_voltage(left, 0.0461f);
    ADC2->ISR = next_input_flags;
    next_input_flags = ADC_ISR_EOC | ADC_ISR_EOS;
#endif
    ADC1->JDR3 = raw_voltage(right, 0.04666f);
    /* ADC ISR is supplied afresh each time. The ADC W1C write itself does
     * not need emulation because the next period replaces this register. */
    ADC1->ISR = adc_flags;
    before_call();
    Power_TimerIRQ();
    after_call();
}
static void period(float left, float right, float il)
{ irq(left, right, il, ADC_ISR_JEOS, 1000U, 640U); }
static int request(unsigned mode, float target)
{
    int accepted;
    before_call();
    accepted = Power_RequestStart(mode, target);
    after_call();
    return accepted;
}
static void stop(void)
{ before_call(); Power_Stop(); after_call(); }
static void clear_fault(void)
{ before_call(); Power_ClearFault(); after_call(); }
static void service(void)
{
    AdcMeasurements measurements;
    before_call();
    Power_Service(&measurements);
    after_call();
    CHECK(power_command == POWER_CMD_NONE);
    CHECK(fabs(measurements.v_high - power_status.v_left) < 0.0001);
    CHECK(fabs(measurements.v_low - power_status.v_right) < 0.0001);
    CHECK(fabs(measurements.il - power_status.il) < 0.0001);
}
static void stopped_samples(float left, float right, float il)
{
    stop();
    clear_fault();
    CHECK(power_status.state == POWER_STOPPED && power_status.fault == 0U);
    CHECK(!Power_IsStartPending() && enabled_outputs == 0U);
    period(left, right, il);
    period(left, right, il);
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    CHECK(Power_SamplesAreValid());
}
static void check_plan(unsigned mode)
{
    PwmPlan plan;
    if (mode >= 3U) {
        CHECK(PWM_MakeReversePlan(reverse_Y.D_cm_R, reverse_Y.BOOST_RUN_R, &plan));
        CHECK(fabsf(power_status.d_cmd - reverse_Y.D_cm_R) < 0.000001f);
    } else {
        CHECK(PWM_MakePlan(forward_Y.D_cmd, forward_Y.BOOST_RUN, &plan));
        CHECK(fabsf(power_status.d_cmd - forward_Y.D_cmd) < 0.000001f);
    }
    CHECK(power_status.left_compare == plan.left_compare);
    CHECK(power_status.right_compare == plan.right_compare);
    CHECK(HRTIM1->sTimerxRegs[TIMER_B].CMP1xR == plan.left_compare);
    CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP1xR == plan.right_compare);
}
static void arm_pending(unsigned mode, float left, float right)
{
    CHECK(Power_IsStartPending() && enabled_outputs == 0U);
    for (unsigned n = 1U; n <= 30U; ++n) {
        period(left, right, 0.0f);
        CHECK(power_status.state == POWER_BOOTSTRAP);
        CHECK(enabled_outputs == LOW_OUTPUTS);
        CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP1xR == PWM_MIN_COMPARE_TICKS);
        CHECK(HRTIM1->sTimerxRegs[TIMER_B].CMP1xR == PWM_MIN_COMPARE_TICKS);
    }
    period(left, right, 0.0f);
    CHECK(power_status.state == POWER_ARMING && enabled_outputs == LOW_OUTPUTS);
    /* Initial preload must be mirrored before any high side is enabled. */
    if (mode >= 3U) {
        CHECK(HRTIM1->sTimerxRegs[TIMER_B].CMP1xR == 28160U);
        CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP1xR == 0U);
    } else {
        CHECK(HRTIM1->sTimerxRegs[TIMER_B].CMP1xR ==
              (POWER_BENCH_12V_BUCK_TEST ? 0U : PWM_MIN_COMPARE_TICKS));
        CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP1xR == 28160U);
    }
    period(left, right, 0.0f);
    CHECK(power_status.state == POWER_ARMING && enabled_outputs == LOW_OUTPUTS);
    period(left, right, 0.0f);
    CHECK(power_status.state == POWER_RUNNING && enabled_outputs == ALL_OUTPUTS);
    CHECK(!Power_IsStartPending() && power_status.fault == 0U);
    check_plan(mode);
}
static void start_running(unsigned mode, float source, float target)
{
    float left = mode >= 3U ? 0.0f : source;
    float right = mode >= 3U ? source : 0.0f;
    stopped_samples(left, right, 0.0f);
    CHECK(request(mode, target));
    arm_pending(mode, left, right);
}
static void expect_fault(uint32_t fault)
{
    CHECK(power_status.state == POWER_FAULT);
    CHECK((power_status.fault & fault) != 0U);
    CHECK(enabled_outputs == 0U && !Power_IsStartPending());
}

#if POWER_BENCH_LATE_INPUT_ADC
static void late_input_sampling(void)
{
    CHECK(adc_config_count == 3U && adc_channels[0] == ADC_CHANNEL_3);
    CHECK(adc_channels[1] == ADC_CHANNEL_1 && adc_channels[2] == ADC_CHANNEL_2);
    CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP2xR == 8000U);
    CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP4xR == 23040U);
    stopped_samples(12.0f, 0.0f, 0.0f);
    early_input_raw = raw_voltage(16.0f, 0.0461f);
    override_early_input = 1U;
    period(12.0f, 0.0f, 0.0f);
    CHECK(fabsf(power_status.v_left - 12.0f) < 0.02f);
    CHECK(request(1U, 6.0f));
    arm_pending(1U, 12.0f, 0.0f);
    override_early_input = 1U;
    period(12.0f, 0.2f, 0.1f);
    CHECK(power_status.state == POWER_RUNNING && power_status.fault == 0U);
    CHECK(fabsf(forward_U.V_left - 12.0f) < 0.02f);
    CHECK(fabsf(forward_U.V_right - 0.2f) < 0.02f);
    CHECK(fabsf(forward_U.IL - 0.1f) < 0.01f);
    CHECK(ADC2->ISR == (ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR)); /* W1C strobe */
    const float bad_volts[] = {9.9f, 14.1f};
    for (unsigned n = 0; n < 2U; ++n) {
        start_running(1U, 12.0f, 6.0f);
        early_input_raw = raw_voltage(12.0f, 0.0461f);
        override_early_input = 1U;
        period(bad_volts[n], 0.0f, 0.0f);
        expect_fault(POWER_FAULT_INPUT);
        CHECK(fabsf(power_status.fault_input_volts - bad_volts[n]) < 0.02f);
        CHECK(power_status.fault_raw_vh == raw_voltage(bad_volts[n], 0.0461f));
    }
    const uint32_t invalid[] = {0U, ADC_ISR_EOC, ADC_ISR_EOS,
                               ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR};
    const unsigned prep_cycles[] = {0U, 1U, 31U, 33U};
    for (unsigned bad = 0U; bad < 4U; ++bad) {
        for (unsigned phase = 0U; phase < 4U; ++phase) {
            stopped_samples(12.0f, 0.0f, 0.0f);
            CHECK(request(1U, 6.0f));
            for (unsigned n = 0; n < prep_cycles[phase]; ++n) period(12.0f, 0.0f, 0.0f);
            next_input_flags = invalid[bad];
            period(12.0f, 0.0f, 0.0f);
            expect_fault(POWER_FAULT_ADC);
            CHECK(!Power_SamplesAreValid());
        }
        stopped_samples(12.0f, 0.0f, 0.0f);
        next_input_flags = invalid[bad];
        period(12.0f, 0.0f, 0.0f);
        CHECK(!Power_SamplesAreValid() && enabled_outputs == 0U);
        CHECK(!request(1U, 6.0f));
    }
    stopped_samples(12.0f, 0.0f, 0.0f);
    puts("Late input ADC2: independent phase/source, no stale fallback, startup/run guards: PASS");
}

static void late_input_init_failures(void)
{
    ADC_HandleTypeDef adc = {.Instance = ADC1};
    HRTIM_HandleTypeDef timer = {HRTIM1};
    for (unsigned fail = 1U; fail <= 6U; ++fail) {
        input_init_step = input_started = counters_started = 0U;
        input_fail_step = fail;
        enabled_outputs = ALL_OUTPUTS;
        before_call();
        CHECK(Power_Init(&adc, &timer) == HAL_ERROR);
        after_call();
        CHECK(input_init_step == fail && counters_started == 0U && enabled_outputs == 0U);
    }
    input_fail_step = 0U;
    puts("Late input ADC2: all six HAL setup/arm failures keep counters and outputs off: PASS");
}
#endif

static void cold_start_samples_rechecked(void)
{
#if defined(POWER_BENCH_FAST_BUCK) && POWER_BENCH_FAST_BUCK
    const float output[] = {1.05f, 0.0f};
    const float current[] = {0.0f, 0.55f};
    /* New limits apply consistently to request, bootstrap and arming. */
    for (unsigned sign = 0; sign < 2; ++sign) {
        float il = sign ? -0.48f : 0.48f;
        stopped_samples(12.0f, 0.9f, il);
        CHECK(request(1U, 6.0f));
        for (unsigned cycle = 0; cycle < 33; ++cycle) {
            period(12.0f, 0.9f, il);
            CHECK(power_status.fault == 0U);
        }
        CHECK(power_status.state == POWER_RUNNING);
    }
    const float rejected_current[] = {-0.55f, 0.55f};
    for (unsigned sign = 0; sign < 2; ++sign) {
        stopped_samples(12.0f, 0.0f, rejected_current[sign]);
        CHECK(!request(1U, 6.0f));
        CHECK(power_status.fault_detail == 5U);
    }
    /* Direct physical values check the exact comparison boundary independently
     * of ADC quantization; period tests above exercise real conversion. */
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.il = 0.5f;
    CHECK(!request(1U, 6.0f) && power_status.fault_detail == 5U);
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.il = -0.5f;
    CHECK(!request(1U, 6.0f) && power_status.fault_detail == 5U);
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_right = 1.0f;
    CHECK(request(1U, 6.0f));
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_right = 1.001f;
    CHECK(!request(1U, 6.0f) && power_status.fault_detail == 6U);
#else
    const float output[] = {0.4f, 0.0f};
    const float current[] = {0.0f, 0.3f};
#endif
    for (unsigned n = 0U; n < 2U; ++n) {
        stopped_samples(12.0f, 0.0f, 0.0f);
        CHECK(request(1U, 6.0f));
        period(12.0f, output[n], current[n]);
        expect_fault(POWER_FAULT_START);
        CHECK(power_status.fault_detail == (n == 0U ? 9U : 8U));
        CHECK(fabsf(power_status.fault_v_left - 12.0f) < 0.02f);
        CHECK(fabsf(power_status.fault_v_right - output[n]) < 0.02f);
        CHECK(fabsf(power_status.fault_il - current[n]) < 0.02f);
        /* OENR retains any enable strobe even if ODISR follows in this IRQ. */
        CHECK(HRTIM1->sCommonRegs.OENR == 0U);
        stopped_samples(12.0f, 0.0f, 0.0f);
        CHECK(request(1U, 6.0f));
        for (unsigned cycle = 0U; cycle < 31U; ++cycle)
            period(12.0f, 0.0f, 0.0f);
        CHECK(power_status.state == POWER_ARMING);
        period(12.0f, output[n], current[n]);
        expect_fault(POWER_FAULT_START);
        CHECK(power_status.fault_detail == (n == 0U ? 9U : 8U));
        CHECK(HRTIM1->sCommonRegs.OENR == 0U);
    }
    puts("Cold output/current rechecked before bootstrap and high-side arming: PASS");
}

static void cold_boot(void)
{
    ADC_HandleTypeDef adc = {.Instance = ADC1};
    HRTIM_HandleTypeDef timer = {HRTIM1};
    RCC->APB2ENR = RCC_APB2ENR_HRTIM1EN;
    enabled_outputs = ALL_OUTPUTS; /* Init must force even a stale enable off. */
    before_call();
    CHECK(Power_Init(&adc, &timer) == HAL_OK);
    after_call();
    CHECK(power_status.state == POWER_STOPPED && power_status.samples == 0U);
    CHECK(enabled_outputs == 0U && !Power_IsStartPending() && !Power_SamplesAreValid());
    CHECK(protection_init_calls == 1U);
    CHECK(adc_config_count == 3U && adc_channels[0] == ADC_CHANNEL_3);
    CHECK(adc_channels[1] == ADC_CHANNEL_1 && adc_channels[2] == ADC_CHANNEL_2);
    CHECK(counters_started == (HRTIM_TIMERID_TIMER_A | HRTIM_TIMERID_TIMER_B));
    CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP2xR == PWM_PERIOD_TICKS / 4U);
    CHECK((HRTIM1->sTimerxRegs[TIMER_B].TIMxDIER & HRTIM_TIMDIER_REPIE) != 0U);
    period(24.0f, 0.0f, 0.0f);
    CHECK(!request(1U, 12.0f)); /* One valid conversion cannot arm. */
    expect_fault(POWER_FAULT_START);
    stopped_samples(24.0f, 0.0f, 0.0f);
    for (unsigned i = 0U; i < 5U; ++i) period(24.0f, 0.0f, 0.0f);
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    puts("Cold boot, sample qualification and idle outputs: PASS");
}
static void forward_regressions(void)
{
    _Static_assert(POWER_STOPPED == 0 && POWER_BOOTSTRAP == 1 && POWER_ARMING == 2 &&
                   POWER_RUNNING == 3 && POWER_FAULT == 4, "existing state values changed");
    _Static_assert(POWER_CMD_START_BOOST == 1 && POWER_CMD_STOP == 2 &&
                   POWER_CMD_CLEAR_FAULT == 3 && POWER_CMD_START_BUCK == 4,
                   "existing debugger commands changed");
    start_running(1U, 24.0f, 12.0f);
    CHECK(power_status.right_compare == 28160U);
    CHECK(power_status.left_compare < power_status.right_compare);
    period(24.0f, 0.0f, 2.0f);
    CHECK(power_status.state == POWER_RUNNING && enabled_outputs == ALL_OUTPUTS);
    period(24.0f, 0.0f, -1.2f);
    expect_fault(POWER_FAULT_REVERSE);
    start_running(2U, 12.0f, 36.0f);
    CHECK(forward_Y.EN_F_BOOST && !forward_Y.EN_F_BUCK);
    CHECK(!forward_Y.BOOST_RUN); /* Cold Boost starts in Buck precharge. */
    period(12.0f, 0.0f, 7.6f);
    expect_fault(POWER_FAULT_CURRENT);
    stopped_samples(24.0f, 0.4f, 0.0f);
    CHECK(!request(1U, 12.0f));
    expect_fault(POWER_FAULT_START);
    puts("Forward Buck/Boost startup and polarity regressions: PASS");
}
static void reverse_startup_and_current(void)
{
    start_running(3U, 24.0f, 12.0f);
    CHECK(reverse_Y.EN_R_BUCK && !reverse_Y.EN_R_BOOST);
    CHECK(power_status.left_compare == 28160U);
    CHECK(power_status.right_compare < power_status.left_compare);
    period(0.0f, 24.0f, -2.0f);
    CHECK(power_status.state == POWER_RUNNING && power_status.fault == 0U);
    CHECK(enabled_outputs == ALL_OUTPUTS && reverse_U.IL < -1.9f);
    check_plan(3U);
    period(0.0f, 24.0f, -7.6f);
    expect_fault(POWER_FAULT_CURRENT);
    start_running(3U, 24.0f, 12.0f);
    /* Reverse polarity is qualified for 16 periods so fixed-phase current
     * ripple at light load does not latch a directional fault. A healthy
     * sample must reset that counter. Absolute current trips stay immediate. */
    for (unsigned n = 0U; n < 15U; ++n) {
        period(0.0f, 24.0f, 1.2f);
        CHECK(power_status.state == POWER_RUNNING && power_status.fault == 0U);
    }
    period(0.0f, 24.0f, -0.2f);
    for (unsigned n = 0U; n < 15U; ++n) {
        period(0.0f, 24.0f, 1.2f);
        CHECK(power_status.state == POWER_RUNNING && power_status.fault == 0U);
    }
    period(0.0f, 24.0f, 1.2f);
    expect_fault(POWER_FAULT_REVERSE);
    start_running(4U, 12.0f, 36.0f);
    CHECK(reverse_Y.EN_R_BOOST && !reverse_Y.EN_R_BUCK);
    CHECK(!reverse_Y.BOOST_RUN_R);
    period(0.0f, 12.0f, -2.0f);
    CHECK(power_status.state == POWER_RUNNING && power_status.fault == 0U);
    check_plan(4U);
    period(0.0f, 12.0f, -7.6f);
    expect_fault(POWER_FAULT_CURRENT);
    puts("Reverse Buck/Boost mirrored startup and physical current trips: PASS");
}
static void reverse_bus_protection(void)
{
    start_running(3U, 24.0f, 12.0f);
    period(13.5f, 24.0f, -0.2f);
    expect_fault(POWER_FAULT_OUTPUT);
    CHECK((power_status.fault & POWER_FAULT_INPUT) == 0U);
    start_running(3U, 24.0f, 12.0f);
    period(0.0f, 5.8f, -0.2f);
    expect_fault(POWER_FAULT_INPUT);
    CHECK((power_status.fault & POWER_FAULT_OUTPUT) == 0U);
    start_running(3U, 24.0f, 12.0f);
    period(0.0f, 40.2f, -0.2f);
    expect_fault(POWER_FAULT_INPUT);
    puts("Reverse left output OV and right source range: PASS");
}
static void start_rejections(void)
{
    const struct { unsigned mode; float left, right, il, target; } cases[] = {
        {3U, 0.4f, 24.0f, 0.0f, 12.0f},
        {4U, 0.4f, 12.0f, 0.0f, 36.0f},
        {3U, 0.0f, 5.8f, 0.0f, 1.0f},
        {3U, 0.0f, 40.2f, 0.0f, 12.0f},
        {3U, 0.0f, 24.0f, -0.3f, 12.0f},
        {3U, 0.0f, 24.0f, 0.3f, 12.0f},
        {3U, 0.0f, 24.0f, 0.0f, 0.9f},
        {4U, 0.0f, 12.0f, 0.0f, 36.1f},
        {3U, 0.0f, 12.0f, 0.0f, 24.0f},
        {4U, 0.0f, 24.0f, 0.0f, 12.0f},
        {0U, 24.0f, 0.0f, 0.0f, 12.0f},
        {5U, 0.0f, 24.0f, 0.0f, 12.0f},
        {1U, 12.0f, 0.0f, 0.0f, 24.0f},
        {2U, 24.0f, 0.0f, 0.0f, 12.0f}
    };
    for (unsigned i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        stopped_samples(cases[i].left, cases[i].right, cases[i].il);
        CHECK(!request(cases[i].mode, cases[i].target));
        expect_fault(POWER_FAULT_START);
    }
    stopped_samples(0.0f, 6.1f, 0.0f);
    CHECK(request(4U, 12.0f));
    stop();
    stopped_samples(0.0f, 39.9f, 0.0f);
    CHECK(request(3U, 1.0f));
    stop();
    puts("Start qualification: bus direction, residual voltage, current, targets and modes: PASS");
}
static void boost_preload_changeover(void)
{
    unsigned observed_blank = 0U, observed_boost = 0U;
    int previous_step_boost = 0;
    start_running(4U, 12.0f, 36.0f);
    /* A synthesized safe precharged output drives the real controller's
     * filter and three-period delay, without changing its internal state. */
    for (unsigned n = 0U; n < 1000U; ++n) {
        uint32_t before_outputs = enabled_outputs;
        previous_step_boost = reverse_Y.BOOST_RUN_R;
        period(12.1f, 12.0f, -0.2f);
        CHECK(power_status.state != POWER_FAULT);
        if (reverse_Y.D_cm_R4 && !reverse_Y.BOOST_RUN_R) {
            ++observed_blank;
            CHECK((unsigned)power_status.state == 5U); /* appended POWER_CHANGEOVER */
            CHECK(enabled_outputs == 0U);
        }
        if (reverse_Y.BOOST_RUN_R) {
            check_plan(4U);
            if (before_outputs == 0U && enabled_outputs == ALL_OUTPUTS) {
                CHECK(previous_step_boost); /* first Boost preload has latched */
                CHECK(active_right_compare == 28160U);
                CHECK(active_left_compare >= PWM_MIN_COMPARE_TICKS);
                CHECK(active_left_compare <= 28160U);
                ++observed_boost;
                CHECK(power_status.state == POWER_RUNNING);
                break;
            }
            if (!previous_step_boost) CHECK(enabled_outputs == 0U);
        }
    }
    CHECK(observed_blank > 0U && observed_boost == 1U);
    puts("Reverse Boost all-off changeover waits for actual preload latch: PASS");
}
static void adc_and_timing_faults(void)
{
    stopped_samples(0.0f, 24.0f, 0.0f);
    CHECK(request(3U, 12.0f));
    irq(0.0f, 24.0f, 0.0f, 0U, 1000U, 640U);
    expect_fault(POWER_FAULT_ADC);
    CHECK(!Power_SamplesAreValid());
    period(0.0f, 24.0f, 0.0f);
    expect_fault(POWER_FAULT_ADC);
    clear_fault();
    irq(0.0f, 24.0f, 0.0f, 0U, 1000U, 640U);
    CHECK(power_status.state == POWER_STOPPED && !Power_SamplesAreValid());
    CHECK(!request(3U, 12.0f));
    expect_fault(POWER_FAULT_START);
    start_running(3U, 24.0f, 12.0f);
    irq(0.0f, 24.0f, -0.2f, ADC_ISR_JEOS | ADC_ISR_JQOVF, 1000U, 640U);
    expect_fault(POWER_FAULT_ADC);
    start_running(4U, 12.0f, 36.0f);
    irq(0.0f, 12.0f, -0.2f, ADC_ISR_JEOS, 2000U, 640U);
    expect_fault(POWER_FAULT_TIMING);
    start_running(1U, 24.0f, 12.0f);
    irq(24.0f, 0.0f, 0.0f, ADC_ISR_JEOS, 1000U, 9000U);
    expect_fault(POWER_FAULT_TIMING);
    puts("ADC loss/overflow cancel starts; missed/late IRQ trips: PASS");
}
static void reverse_output_hold(void)
{
    unsigned waited, bootstrap_periods = 0U;
    PwmPlan held_plan;
    float entry_reference, resume_reference;
    start_running(4U, 12.0f, 36.0f);
    /* Exercise the real reference ramp and precharge delay. A sampled
     * output above the final setpoint models the unloaded output condition
     * that requires all-off regulation instead of an overvoltage trip. */
    for (waited = 0U; waited < 40000U && (unsigned)power_status.state != 6U; ++waited) {
        period(36.3f, 12.0f, 0.0f);
        CHECK(power_status.state != POWER_FAULT);
    }
    CHECK(waited < 40000U && (unsigned)power_status.state == 6U); /* POWER_HOLD */
    CHECK(enabled_outputs == 0U && !Power_IsStartPending());
    CHECK(reverse_Y.BOOST_RUN_R);
    entry_reference = power_status.v_reference;
    CHECK(entry_reference < 35.0f); /* Overshoot protection also applies during the ramp. */
    CHECK(!request(3U, 12.0f)); /* A held controller is still an active run. */
    CHECK((unsigned)power_status.state == 6U && power_status.fault == 0U);
    for (unsigned n = 0U; n < 128U; ++n) {
        period(36.3f, 12.0f, 0.0f);
        CHECK((unsigned)power_status.state == 6U && enabled_outputs == 0U);
    }
    CHECK(power_status.v_reference > entry_reference + 0.05f);
    CHECK(power_status.v_reference == reverse_B.Saturation4);
    for (unsigned n = 0U; n < 3U; ++n) {
        period(36.0f, 12.0f, 0.0f);
        CHECK((unsigned)power_status.state == 6U && enabled_outputs == 0U);
    }
    for (unsigned n = 0U; n < 3U; ++n) {
        period(35.0f, 12.0f, -0.5f);
        CHECK((unsigned)power_status.state == 6U && enabled_outputs == 0U);
    }
    period(35.0f, 12.0f, 0.0f);
    CHECK(power_status.state == POWER_BOOTSTRAP && enabled_outputs == LOW_OUTPUTS);
    resume_reference = power_status.v_reference;
    CHECK(resume_reference > entry_reference);
    /* HOLD continues to step the controller; capture its newest plan at
     * the resume decision rather than freezing the plan at HOLD entry. */
    CHECK(PWM_MakeReversePlan(reverse_Y.D_cm_R, 1, &held_plan));
    ++bootstrap_periods;
    while (power_status.state == POWER_BOOTSTRAP && bootstrap_periods <= 31U) {
        period(35.0f, 12.0f, 0.0f);
        if (power_status.state == POWER_BOOTSTRAP) {
            ++bootstrap_periods;
            CHECK(enabled_outputs == LOW_OUTPUTS);
        }
    }
    CHECK(bootstrap_periods >= 29U && bootstrap_periods <= 30U);
    CHECK(power_status.state == POWER_ARMING && enabled_outputs == LOW_OUTPUTS);
    CHECK(reverse_Y.BOOST_RUN_R && power_status.v_reference == resume_reference);
    /* A resume must retain the settled Boost duty. A cold Buck plan here
     * would connect the bridges with the wrong voltage ratio. */
    CHECK(HRTIM1->sTimerxRegs[TIMER_A].CMP1xR == held_plan.right_compare);
    CHECK(HRTIM1->sTimerxRegs[TIMER_B].CMP1xR == held_plan.left_compare);
    period(35.0f, 12.0f, 0.0f);
    CHECK(power_status.state == POWER_ARMING && enabled_outputs == LOW_OUTPUTS);
    period(35.0f, 12.0f, 0.0f);
    CHECK(power_status.state == POWER_RUNNING && enabled_outputs == ALL_OUTPUTS);
    CHECK(reverse_Y.BOOST_RUN_R && power_status.v_reference >= resume_reference);
    CHECK(power_status.v_reference < resume_reference + 0.01f);
    CHECK(active_right_compare == held_plan.right_compare);
    CHECK(active_left_compare == held_plan.left_compare);
    check_plan(4U);
    period(36.3f, 12.0f, 0.0f);
    CHECK((unsigned)power_status.state == 6U && enabled_outputs == 0U);
    stop();
    CHECK(power_status.state == POWER_STOPPED && !Power_IsStartPending());
    for (unsigned n = 0U; n < 3U; ++n) period(35.0f, 12.0f, 0.0f);
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    /* Protection remains live while the output is held off. */
    start_running(4U, 12.0f, 36.0f);
    for (waited = 0U; waited < 40000U && (unsigned)power_status.state != 6U; ++waited) {
        period(36.3f, 12.0f, 0.0f);
        CHECK(power_status.state != POWER_FAULT);
    }
    CHECK((unsigned)power_status.state == 6U);
    period(36.3f, 12.0f, -7.6f);
    expect_fault(POWER_FAULT_CURRENT);
    puts("Reverse output hold, residual current guard, Boost resume and Stop: PASS");
}
static void commands_stop_and_clear(void)
{
    const struct { uint32_t command; unsigned mode; float source, target; } cases[] = {
        {1U, 2U, 12.0f, 36.0f}, {4U, 1U, 24.0f, 12.0f},
        {5U, 3U, 24.0f, 12.0f}, {6U, 4U, 12.0f, 36.0f}
    };
    for (unsigned i = 0U; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        float left = cases[i].mode >= 3U ? 0.0f : cases[i].source;
        float right = cases[i].mode >= 3U ? cases[i].source : 0.0f;
        stopped_samples(left, right, 0.0f);
        power_target_volts = cases[i].target;
        power_command = cases[i].command;
        service();
        arm_pending(cases[i].mode, left, right);
        power_command = 2U;
        service();
        CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
        for (unsigned n = 0U; n < 3U; ++n) period(left, right, 0.0f);
        CHECK(power_status.state == POWER_STOPPED && !Power_IsStartPending());
    }
    stopped_samples(0.0f, 24.0f, 0.0f);
    CHECK(request(3U, 12.0f));
    stop(); /* Pending start is cancelled without an IRQ ever enabling lows. */
    period(0.0f, 24.0f, 0.0f);
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    start_running(3U, 24.0f, 12.0f);
    period(0.0f, 24.0f, -7.6f);
    expect_fault(POWER_FAULT_CURRENT);
    power_command = 2U;
    service();
    expect_fault(POWER_FAULT_CURRENT); /* Stop preserves a latched fault. */
    power_command = 3U;
    service();
    CHECK(power_status.state == POWER_STOPPED && power_status.fault == 0U);
    for (unsigned n = 0U; n < 3U; ++n) period(0.0f, 24.0f, 0.0f);
    CHECK(enabled_outputs == 0U && !Power_IsStartPending());
    puts("Debugger commands 1-6, stop/fault clear and no automatic restart: PASS");
}
static void hardware_fault_latch_and_readiness(void)
{
    stopped_samples(0.0f, 24.0f, 0.0f);
    protection_ready = 0U;
    CHECK(!request(3U, 12.0f));
    expect_fault(512U); /* Hardware wiring has not been confirmed. */
    CHECK(enabled_outputs == 0U);
    clear_fault();
    protection_ready = 1U;
    protection_fault_on_poll = 2U; /* Fault interrupts foreground controller init. */
    CHECK(!request(3U, 12.0f));
    expect_fault(256U);
    CHECK(!Power_IsStartPending());
    clear_fault();
    /* A captured pulse remains a fault even after the comparator returns safe. */
    protection_latched = 8U;
    CHECK(!request(3U, 12.0f));
    expect_fault(256U);
    CHECK(protection_irq_masked);
    clear_fault();
    CHECK(power_status.state == POWER_STOPPED && !Power_IsStartPending());
    for (unsigned phase = 0U; phase < 3U; ++phase) {
        stopped_samples(0.0f, 12.0f, 0.0f);
        CHECK(request(4U, 36.0f));
        if (phase < 2U) {
            for (unsigned n = 0; n < (phase ? 31U : 1U); ++n)
                period(0.0f, 12.0f, 0.0f);
            CHECK(power_status.state == (phase ? POWER_ARMING : POWER_BOOTSTRAP));
        } else {
            arm_pending(4U, 0.0f, 12.0f);
            unsigned n;
            for (n = 0U; n < 1000U && power_status.state != POWER_CHANGEOVER; ++n)
                period(12.2f, 12.0f, -0.2f);
            CHECK(power_status.state == POWER_CHANGEOVER);
        }
        protection_latched = phase % 2U ? 8U : 1U;
        before_call();
        Power_FaultIRQ();
        after_call();
        expect_fault(256U);
        clear_fault();
        CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    }
    for (unsigned mode = 1U; mode <= 4U; ++mode) {
        start_running(mode, mode % 2U ? 24.0f : 12.0f, mode % 2U ? 12.0f : 36.0f);
        protection_active = mode % 2U ? 1U : 8U;
        before_call();
        Power_FaultIRQ();
        after_call();
        expect_fault(256U);
        clear_fault();
        expect_fault(256U); /* An active analog fault cannot be acknowledged. */
        protection_active = 0U;
        clear_fault();
        CHECK(power_status.state == POWER_STOPPED && !Power_IsStartPending());
        CHECK(enabled_outputs == 0U);
    }
    stopped_samples(0.0f, 12.0f, 0.0f);
    CHECK(request(4U, 36.0f));
    protection_latched = 1U;
    /* Timer poll catches a fault even if common fault IRQ has not run and
     * the injected ADC sequence is absent. Pending startup must be cancelled. */
    irq(0.0f, 12.0f, 0.0f, 0U, 1000U, 640U);
    expect_fault(256U);
    clear_fault();
    start_running(4U, 12.0f, 36.0f);
    unsigned guard;
    for (guard = 0; guard < 1000 && power_status.state != POWER_HOLD; ++guard)
        period(36.3f, 12.0f, 0.0f);
    CHECK(power_status.state == POWER_HOLD);
    protection_latched = 8U;
    period(36.3f, 12.0f, 0.0f);
    expect_fault(256U);
    clear_fault();
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    puts("Hardware protection readiness, pulse capture, active clear refusal and no restart: PASS");
}
static void hardware_enable_interleavings(void)
{
    /* A transient fault can arrive after the safe entry poll. It must not
     * be overridden by an OENR later in that same (equal priority) IRQ. */
    for (unsigned poll = 1U; poll <= 2U; ++poll) {
        stopped_samples(0.0f, 24.0f, 0.0f);
        CHECK(request(3U, 12.0f));
        for (unsigned n = 0U; n < 32U; ++n) period(0.0f, 24.0f, 0.0f);
        CHECK(power_status.state == POWER_ARMING);
        protection_fault_after_safe_poll = poll;
        period(0.0f, 24.0f, 0.0f);
        CHECK(protection_fault_after_safe_poll == 0U);
        expect_fault(256U);
        clear_fault();
        /* Pending cold start: the low-side bootstrap enable is guarded too. */
        stopped_samples(0.0f, 12.0f, 0.0f);
        CHECK(request(4U, 36.0f));
        protection_fault_after_safe_poll = poll;
        period(0.0f, 12.0f, 0.0f);
        CHECK(protection_fault_after_safe_poll == 0U);
        expect_fault(256U);
        clear_fault();
        /* Ready Boost compare plan: a disappeared fault must not be rearmed
         * by the CHANGEOVER -> RUNNING transition. */
        start_running(4U, 12.0f, 36.0f);
        unsigned guard;
        for (guard = 0U; guard < 1000U &&
             !(power_status.state == POWER_CHANGEOVER && reverse_Y.BOOST_RUN_R); ++guard)
            period(12.2f, 12.0f, -0.2f);
        CHECK(power_status.state == POWER_CHANGEOVER && reverse_Y.BOOST_RUN_R);
        protection_fault_after_safe_poll = poll;
        period(12.2f, 12.0f, -0.2f);
        CHECK(protection_fault_after_safe_poll == 0U);
        expect_fault(256U);
        clear_fault();
        /* HOLD resume also enables the low sides after the entry poll. */
        start_running(4U, 12.0f, 36.0f);
        for (guard = 0U; guard < 1000U && power_status.state != POWER_HOLD; ++guard)
            period(36.3f, 12.0f, 0.0f);
        CHECK(power_status.state == POWER_HOLD);
        protection_fault_after_safe_poll = poll;
        period(35.0f, 12.0f, 0.0f);
        CHECK(protection_fault_after_safe_poll == 0U);
        expect_fault(256U);
        clear_fault();
    }
    puts("Short faults around cold/bootstrap/arming/changeover/HOLD enables remain latched: PASS");
}
#if defined(POWER_UNIFIED_DIAGNOSTICS) && POWER_UNIFIED_DIAGNOSTICS
static void unified_first_fault_diagnostics(void)
{
    CHECK(power_diag.version == 1U);
    CHECK(power_diag.cpu_hz == 150000000U && power_diag.pwm_hz == 150000U);
    CHECK(power_diag.write_budget_cycles == 840U && power_diag.exit_budget_cycles == 936U);
    /* The same S9 must identify whether lows have ever been requested. */
    const unsigned prep_periods[] = {0U, 1U, 31U};
    const PowerState states[] = {POWER_STOPPED, POWER_BOOTSTRAP, POWER_ARMING};
    for (unsigned phase = 0U; phase < 3U; ++phase) {
        stopped_samples(12.0f, 0.0f, 0.0f);
        CHECK(request(1U, 6.0f));
        for (unsigned n = 0U; n < prep_periods[phase]; ++n) period(12.0f, 0.0f, 0.0f);
        period(12.0f, 0.589f, -0.23f);
        expect_fault(128U);
        CHECK(power_diag.fault.fault == 128U && power_diag.fault.detail == 9U);
        CHECK(power_diag.fault.state == states[phase]);
        CHECK(power_diag.fault.profile.entry_state == states[phase]);
        CHECK(power_diag.fault.context == POWER_DIAG_CONTEXT_TIMER_IRQ);
        CHECK(power_diag.fault.profile.stage == POWER_DIAG_STAGE_CHECKS);
        CHECK(power_diag.fault.start_pending == (phase == 0U));
        CHECK(power_diag.fault.sample_fresh == 1U);
        CHECK(power_diag.fault.profile.valid_mask == 3U); /* Entry + ADC. */
        CHECK(power_diag.fault.raw_vl == 34U);
        CHECK(fabsf(power_diag.fault.v_right - 0.5889905f) < 0.00001f);
        CHECK(fabsf(power_diag.fault.il - (-0.23059845f)) < 0.00001f);
        CHECK(power_diag.fault.mode == 1U && power_diag.fault.target_volts == 6.0f);
        for (unsigned n = 2U; n < POWER_DIAG_CHECKPOINT_COUNT; ++n)
            CHECK(power_diag.fault.profile.checkpoint_cycles[n] == 0U);
        uint32_t captured_samples = power_diag.fault.samples;
        period(12.0f, 0.0f, 0.0f);
        CHECK(power_status.v_right == 0.0f && power_diag.fault.raw_vl == 34U);
        CHECK(!request(2U, 5.0f)); /* Rejected requests cannot change old evidence. */
        protection_latched = 8U;
        before_call(); Power_FaultIRQ(); after_call();
        CHECK(power_diag.fault.fault == 128U && power_diag.fault.detail == 9U);
        CHECK(power_diag.fault.samples == captured_samples && power_diag.fault.raw_vl == 34U);
        CHECK(power_diag.fault.target_volts == 6.0f);
        clear_fault();
        CHECK(power_diag.fault.fault == 0U && power_diag.fault.profile.valid_mask == 0U);
        CHECK(power_status.state == POWER_STOPPED && !Power_IsStartPending());
    }
    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(1U, 6.0f));
    period(12.0f, 0.0f, 0.3f);
    CHECK(power_diag.fault.detail == 8U && power_diag.fault.state == POWER_STOPPED);
    CHECK(power_diag.fault.start_pending == 1U && power_diag.fault.sample_fresh == 1U);
    stopped_samples(12.0f, 0.4f, 0.0f);
    CHECK(!request(1U, 6.0f));
    CHECK(power_diag.fault.detail == 6U);
    CHECK(power_diag.fault.context == POWER_DIAG_CONTEXT_START_REQUEST);
    CHECK(power_diag.fault.profile.valid_mask == 0U && power_diag.fault.sample_fresh == 0U);
    CHECK(power_diag.fault.mode == 1U && power_diag.fault.target_volts == 6.0f);

    start_running(1U, 12.0f, 6.0f);
    uint32_t old_samples = power_status.samples;
    irq(12.0f, 0.9f, 0.0f, ADC_ISR_JEOS, 2000U, 640U);
    CHECK(power_diag.fault.detail == 1U && power_diag.fault.fault == 64U);
    CHECK(power_diag.fault.sample_fresh == 0U && power_diag.fault.samples == old_samples);
    CHECK(power_diag.fault.profile.valid_mask == 0U && power_diag.fault.raw_vl == 0U);
    CHECK(power_diag.fault.timing_spacing_cycles == 2000U);
    start_running(1U, 12.0f, 6.0f);
    irq(12.0f, 0.9f, 0.0f, 0U, 1000U, 640U);
    CHECK(power_diag.fault.fault == 1U && power_diag.fault.sample_fresh == 0U);
    CHECK(power_diag.fault.profile.valid_mask == 1U && power_diag.fault.raw_vl == 0U);

    start_running(1U, 12.0f, 6.0f);
    profile_delay_stage = POWER_DIAG_STAGE_CONTROLLER;
    profile_delay_cycles = 900U;
    period(12.0f, 0.0f, 0.0f);
    expect_fault(64U);
    CHECK(power_diag.fault.detail == 3U);
    CHECK(power_diag.fault.state == POWER_RUNNING && power_diag.fault.sample_fresh == 1U);
    CHECK(power_diag.fault.profile.stage == POWER_DIAG_STAGE_PREWRITE);
    CHECK(power_diag.fault.timing_elapsed_cycles == 900U);
    CHECK(power_status.fault_elapsed_cycles == 900U);
    CHECK(power_diag.fault.profile.valid_mask == 63U); /* No register-write checkpoint. */
    CHECK(power_diag.fault.profile.checkpoint_cycles[POWER_DIAG_CP_STATE] == 0U);
    CHECK(power_diag.fault.profile.checkpoint_cycles[POWER_DIAG_CP_CONTROLLER] == 900U);
    CHECK(power_diag.fault.profile.checkpoint_cycles[POWER_DIAG_CP_MAP] == 900U);
    CHECK(power_diag.fault.profile.checkpoint_cycles[POWER_DIAG_CP_WRITE] == 0U);
    period(12.0f, 0.0f, 0.0f);
    CHECK(power_diag.fault.timing_elapsed_cycles == 900U);

    /* Make the sampled CYCCNT wrap inside the next profiled control IRQ. */
    stop(); clear_fault();
    test_dwt.CYCCNT = UINT32_MAX - 36010U;
    start_running(1U, 12.0f, 6.0f); /* 2 idle + 33 preparation IRQs. */
    profile_tick_per_read = 3U;
    profile_delay_cycles = 900U;
    period(12.0f, 0.0f, 0.0f);
    profile_tick_per_read = 0U;
    CHECK(test_dwt.CYCCNT < 2000U);
    CHECK(power_diag.fault.detail == 3U);
    CHECK(power_diag.fault.timing_elapsed_cycles >= 900U &&
          power_diag.fault.timing_elapsed_cycles < 1100U);
    for (unsigned n = 1U; n <= POWER_DIAG_CP_MAP; ++n)
        CHECK(power_diag.fault.profile.checkpoint_cycles[n] >=
              power_diag.fault.profile.checkpoint_cycles[n - 1U]);

    start_running(1U, 12.0f, 6.0f);
    protection_active = 8U;
    before_call(); Power_FaultIRQ(); after_call();
    CHECK(power_diag.fault.fault == 256U);
    CHECK(power_diag.fault.context == POWER_DIAG_CONTEXT_HARDWARE_IRQ);
    CHECK(power_diag.fault.profile.valid_mask == 0U && power_diag.fault.sample_fresh == 0U);
    clear_fault(); /* Active comparator must preserve the original record. */
    CHECK(power_diag.fault.context == POWER_DIAG_CONTEXT_HARDWARE_IRQ);
    protection_active = 0U;
    clear_fault();
    CHECK(power_diag.fault.fault == 0U && enabled_outputs == 0U);
    puts("Unified first-fault stages, fresh/stale samples, section delays, wrap and clear: PASS");
}
#endif

#if defined(POWER_BENCH_12V_BUCK_TEST) && POWER_BENCH_12V_BUCK_TEST
int main(void)
{
    ADC_HandleTypeDef adc = {.Instance = ADC1};
    HRTIM_HandleTypeDef timer = {HRTIM1};
    protection_ready = 0U;
    RCC->APB2ENR = RCC_APB2ENR_HRTIM1EN;
    before_call();
    CHECK(Power_Init(&adc, &timer) == HAL_OK);
    after_call();
    CHECK(power_status.protection_ready == 0U);
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
#if POWER_BENCH_LATE_INPUT_ADC
    late_input_sampling();
#endif
#if defined(POWER_UNIFIED_DIAGNOSTICS) && POWER_UNIFIED_DIAGNOSTICS
    unified_first_fault_diagnostics();
#endif
    cold_start_samples_rechecked();
#if defined(POWER_BENCH_FAST_BUCK) && POWER_BENCH_FAST_BUCK
    /* Reported 10.31/13.53V samples must pass request, preparation
     * and running. Still shut off on the first sample above the new limit. */
    const float accepted_inputs[] = {10.05f, 10.31f, 13.53f, 13.9f, 14.0f};
    for (unsigned n = 0; n < sizeof(accepted_inputs)/sizeof(accepted_inputs[0]); ++n) {
        float vin = accepted_inputs[n];
        stopped_samples(vin, 0.0f, 0.0f);
        CHECK(request(1U, 6.0f));
        arm_pending(1U, vin, 0.0f);
        for (unsigned cycle = 0; cycle < 10; ++cycle) {
            period(vin, 0.0f, 0.0f);
            CHECK(power_status.state == POWER_RUNNING && power_status.fault == 0U);
        }
        period(14.1f, 0.0f, 0.0f);
        expect_fault(POWER_FAULT_INPUT);
        CHECK(fabsf(power_status.fault_input_volts - 14.1f) < 0.02f);
        stopped_samples(vin, 0.0f, 0.0f);
        CHECK(request(1U, 6.0f));
        arm_pending(1U, vin, 0.0f);
        period(9.9f, 0.0f, 0.0f);
        expect_fault(POWER_FAULT_INPUT);
        CHECK(fabsf(power_status.fault_input_volts - 9.9f) < 0.02f);
    }
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_left = 14.0f;
    CHECK(request(1U, 6.0f));
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_left = 14.001f;
    CHECK(!request(1U, 6.0f) && power_status.fault_detail == 4U);
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_left = 10.0f;
    CHECK(request(1U, 6.0f));
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_left = 9.999f;
    CHECK(!request(1U, 6.0f) && power_status.fault_detail == 4U);
    puts("FAST input window: 10..14V accepted; outside request/first-sample trip: PASS");
#endif
    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(1U, 6.0f));
    CHECK(power_status.protection_ready == 0U && enabled_outputs == 0U);
    arm_pending(1U, 12.0f, 0.0f);
    for (unsigned n = 0U; n < 16000U; ++n) {
        period(12.0f, 0.0f, 0.0f);
        CHECK(power_status.state == POWER_RUNNING);
        CHECK(forward_B.Saturation <= 0.500001f);
    }
    CHECK(forward_B.Saturation > 0.49f);
    period(12.0f, 0.0f, 1.6f);
    expect_fault(POWER_FAULT_CURRENT);
    period(12.0f, 0.0f, 0.0f);
    expect_fault(POWER_FAULT_CURRENT);
    clear_fault();
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    CHECK(!Power_IsStartPending());
    CHECK(power_status.fault_detail == 0U);
    puts("Bench valid Buck starts without fabricated hardware readiness; 0.5 A reference and 1.5 A trip: PASS");

    const unsigned modes[] = {0U, 2U, 3U, 4U, 5U};
    for (unsigned n = 0U; n < sizeof(modes) / sizeof(modes[0]); ++n) {
        unsigned mode = modes[n];
        stopped_samples(mode >= 3U ? 0.0f : 12.0f,
                        mode >= 3U ? 12.0f : 0.0f, 0.0f);
        CHECK(!request(mode, 6.0f));
        expect_fault(POWER_FAULT_START);
        CHECK(power_status.fault_detail == 1U);
    }
    const float targets[] = {0.0f, 6.1f, 12.0f, 36.0f, NAN, INFINITY};
    for (unsigned n = 0U; n < sizeof(targets) / sizeof(targets[0]); ++n) {
        stopped_samples(12.0f, 0.0f, 0.0f);
        CHECK(!request(1U, targets[n]));
        expect_fault(POWER_FAULT_START);
        CHECK(power_status.fault_detail == 3U);
    }
    const float sources[] = {9.9f, 14.1f, 24.0f};
    for (unsigned n = 0U; n < sizeof(sources) / sizeof(sources[0]); ++n) {
        stopped_samples(sources[n], 0.0f, 0.0f);
        CHECK(!request(1U, 6.0f));
        expect_fault(POWER_FAULT_START);
        CHECK(power_status.fault_detail == 4U);
    }
#if defined(POWER_BENCH_FAST_BUCK) && POWER_BENCH_FAST_BUCK
    stopped_samples(12.0f, 1.05f, 0.0f);
#else
    stopped_samples(12.0f, 0.4f, 0.0f);
#endif
    CHECK(!request(1U, 6.0f));
    expect_fault(POWER_FAULT_START);
    CHECK(power_status.fault_detail == 6U);
#if defined(POWER_BENCH_FAST_BUCK) && POWER_BENCH_FAST_BUCK
    stopped_samples(12.0f, 0.0f, 0.55f);
#else
    stopped_samples(12.0f, 0.0f, 0.3f);
#endif
    CHECK(!request(1U, 6.0f));
    expect_fault(POWER_FAULT_START);
    CHECK(power_status.fault_detail == 5U);

    /* Source limits apply after acceptance and throughout arming/running. */
    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(1U, 6.0f));
    period(14.1f, 0.0f, 0.0f);
    expect_fault(POWER_FAULT_INPUT);
    CHECK(HRTIM1->sCommonRegs.OENR == 0U);
    start_running(1U, 12.0f, 6.0f);
    period(9.9f, 0.0f, 0.0f);
    expect_fault(POWER_FAULT_INPUT);
    CHECK(fabsf(power_status.fault_input_volts - 9.9f) < 0.02f);
    CHECK(power_status.fault_raw_vh == raw_voltage(9.9f, 0.0461f));
    period(12.0f, 0.0f, 0.0f);
    CHECK(fabsf(power_status.v_left - 12.0f) < 0.02f);
    CHECK(fabsf(power_status.fault_input_volts - 9.9f) < 0.02f);
    start_running(1U, 12.0f, 6.0f);
    period(12.0f, 6.7f, 0.0f);
    expect_fault(POWER_FAULT_OUTPUT);
    start_running(1U, 12.0f, 6.0f);
    period(12.0f, 0.0f, -1.2f);
    expect_fault(POWER_FAULT_REVERSE);
    start_running(1U, 12.0f, 6.0f);
    period(12.0f, 0.0f, 8.2f); /* ADC upper rail. */
    expect_fault(POWER_FAULT_CURRENT);
    start_running(1U, 12.0f, 6.0f);
    irq(12.0f, 0.0f, 0.0f, 0U, 1000U, 640U);
    expect_fault(POWER_FAULT_ADC);
    start_running(1U, 12.0f, 6.0f);
    irq(12.0f, 0.0f, 0.0f, ADC_ISR_JEOS, 2000U, 640U);
    expect_fault(POWER_FAULT_TIMING);
    CHECK(power_status.fault_detail == 1U);
    CHECK(power_status.fault_spacing_cycles == 2000U);
    CHECK(power_status.fault_counter_ticks == 640U);
    period(12.0f, 0.0f, 0.0f);
    CHECK(power_status.fault_detail == 1U && power_status.fault_spacing_cycles == 2000U);
    start_running(1U, 12.0f, 6.0f);
    irq(12.0f, 0.0f, 0.0f, ADC_ISR_JEOS, 1000U, 9000U);
    expect_fault(POWER_FAULT_TIMING);
    CHECK(power_status.fault_detail == 2U && power_status.fault_counter_ticks == 9000U);
    start_running(1U, 12.0f, 6.0f);
    advance_cycles_on_poll = 900U;
    period(12.0f, 0.0f, 0.0f);
    expect_fault(POWER_FAULT_TIMING);
    CHECK(power_status.fault_detail == 3U && power_status.fault_elapsed_cycles == 900U);
    CHECK(power_status.fault_spacing_cycles == 1000U);
    clear_fault();
    CHECK(power_status.fault_detail == 0U && power_status.fault_elapsed_cycles == 0U);
    CHECK(power_status.fault_spacing_cycles == 0U && power_status.fault_counter_ticks == 0U);
    CHECK(power_status.fault_v_left == 0.0f && power_status.fault_raw_vh == 0U);
    puts("Bench mode/voltage restrictions and existing startup/current/ADC/timing protections: PASS");

    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(1U, 6.0f));
    stop();
    period(12.0f, 0.0f, 0.0f);
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    CHECK(!Power_IsStartPending());
    CHECK(request(1U, 6.0f));
    clear_fault();
    period(12.0f, 0.0f, 0.0f);
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    CHECK(!Power_IsStartPending());

    stopped_samples(12.0f, 0.0f, 0.0f);
    power_target_volts = 6.0f;
    power_command = POWER_CMD_START_BUCK;
    service();
    CHECK(Power_IsStartPending() && power_status.protection_ready == 0U);
    arm_pending(1U, 12.0f, 0.0f);
    power_command = POWER_CMD_STOP;
    service();
    CHECK(power_status.state == POWER_STOPPED && enabled_outputs == 0U);
    const uint32_t commands[] = {POWER_CMD_START_BOOST,
        POWER_CMD_START_REVERSE_BUCK, POWER_CMD_START_REVERSE_BOOST};
    for (unsigned n = 0U; n < sizeof(commands) / sizeof(commands[0]); ++n) {
        stopped_samples(12.0f, 0.0f, 0.0f);
        power_command = commands[n];
        service();
        expect_fault(POWER_FAULT_START);
    }
    puts("Bench debugger start/stop follows the same restricted backend: PASS");
#if POWER_BENCH_LATE_INPUT_ADC
    late_input_init_failures();
#endif
    return EXIT_SUCCESS;
}
#else
int main(void)
{
    cold_boot(); /* Power_Init runs exactly once, like the firmware. */
    cold_start_samples_rechecked();
    stopped_samples(11.83f, 23.9f, 0.0f);
    CHECK(fabsf(power_status.v_left - 11.83f) < 0.02f);
    CHECK(fabsf(power_status.v_right - 23.9f) < 0.02f);
    CHECK(fabsf(power_status.il) < 0.01f);
    service(); /* UI/foreground must match the controller's calibration. */
    puts("Calibrated left/right buses and shared telemetry: PASS");
    forward_regressions();
    reverse_startup_and_current();
    reverse_bus_protection();
    start_rejections();
    boost_preload_changeover();
    adc_and_timing_faults();
    reverse_output_hold();
    commands_stop_and_clear();
    hardware_fault_latch_and_readiness();
    hardware_enable_interleavings();
    puts("Real power backend host integration: PASS");
    return EXIT_SUCCESS;
}
#endif
