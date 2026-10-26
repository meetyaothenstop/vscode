#include "power_control.h"
#include "forward.h"
#include <math.h>

#define ALL_OUTPUTS (HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2)
#define LOW_OUTPUTS (HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB2)
#define TIMER_A HRTIM_TIMERINDEX_TIMER_A
#define TIMER_B HRTIM_TIMERINDEX_TIMER_B
#define CURRENT_REFERENCE_LIMIT 6.0f /* leave margin below the +8 A ADC rail */
#define CURRENT_TRIP_AMPS 7.5f
#define INPUT_MIN_VOLTS 6.0f
#define INPUT_MAX_VOLTS 40.0f

volatile uint32_t power_command;
volatile float power_target_volts = 36.0f;
volatile PowerStatus power_status;
static volatile unsigned start_pending;
static unsigned active_mode, bootstrap_cycles, arming_cycles;
static float active_target;
static uint32_t cycle_budget;
static uint32_t previous_entry;
static unsigned have_previous_entry;
static volatile unsigned sample_valid;

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

static void trip(uint32_t fault)
{
    Power_EmergencyOff();
    power_status.fault |= fault;
    power_status.state = POWER_FAULT;
    start_pending = 0U;
}

void Power_Stop(void)
{
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    Power_EmergencyOff();
    start_pending = 0U;
    if (power_status.state != POWER_FAULT) power_status.state = POWER_STOPPED;
    __set_PRIMASK(saved);
}

void Power_ClearFault(void)
{
    uint32_t saved = __get_PRIMASK();
    __disable_irq();
    Power_EmergencyOff();
    start_pending = 0U;
    power_status.fault = 0U;
    power_status.state = POWER_STOPPED;
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

HAL_StatusTypeDef Power_Init(ADC_HandleTypeDef *adc, HRTIM_HandleTypeDef *hrtim)
{
    ADC_InjectionConfTypeDef config = {0};
    HRTIM_ADCTriggerCfgTypeDef trigger = {0};
    HRTIM_CompareCfgTypeDef compare = {0};
    Power_EmergencyOff();
    if (adc->Instance != ADC1 || hrtim->Instance != HRTIM1 ||
        HAL_RCC_GetHCLKFreq() != 150000000U) return HAL_ERROR;

    /* Injected order differs from the legacy regular scan: IL first,
     * then VH, VL. 3 x (24.5 + 12.5) / 37.5 MHz = 2.96 us.
     * Trigger at 25% period; data is read at the next period interrupt. */
    config.InjectedNbrOfConversion = 3U;
    config.InjectedSamplingTime = ADC_SAMPLETIME_24CYCLES_5;
    config.InjectedSingleDiff = ADC_SINGLE_ENDED;
    config.InjectedOffsetNumber = ADC_OFFSET_NONE;
    config.ExternalTrigInjecConv = ADC_EXTERNALTRIGINJEC_HRTIM_TRG1;
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
    if (HAL_HRTIM_WaveformCompareConfig(hrtim, TIMER_A, HRTIM_COMPAREUNIT_3,
                                       &compare) != HAL_OK) return HAL_ERROR;
    trigger.UpdateSource = HRTIM_ADCTRIGGERUPDATE_TIMER_A;
    trigger.Trigger = HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP3;
    if (HAL_HRTIM_ADCTriggerConfig(hrtim, HRTIM_ADCTRIGGER_1, &trigger) != HAL_OK)
        return HAL_ERROR;
    /* Flush initial preloads before starting both counters in one MCR write. */
    HRTIM1->sCommonRegs.CR2 = HRTIM_CR2_TASWU | HRTIM_CR2_TBSWU |
                             HRTIM_CR2_TARST | HRTIM_CR2_TBRST;
    ADC1->ISR = ADC_ISR_JEOS | ADC_ISR_JEOC | ADC_ISR_JQOVF;
    if (HAL_ADCEx_InjectedStart(adc) != HAL_OK) return HAL_ERROR;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    cycle_budget = HAL_RCC_GetHCLKFreq() / PWM_FREQUENCY_HZ;
    power_status.state = POWER_STOPPED;
    HRTIM1->sTimerxRegs[TIMER_B].TIMxICR = HRTIM_TIMICR_REPC;
    HRTIM1->sTimerxRegs[TIMER_B].TIMxDIER |= HRTIM_TIMDIER_REPIE;
    HAL_NVIC_SetPriority(HRTIM1_TIMB_IRQn, 0U, 0U);
    HAL_NVIC_ClearPendingIRQ(HRTIM1_TIMB_IRQn);
    HAL_NVIC_EnableIRQ(HRTIM1_TIMB_IRQn);
    return HAL_HRTIM_WaveformCounterStart(hrtim, HRTIM_TIMERID_TIMER_A |
                                        HRTIM_TIMERID_TIMER_B);
}

int Power_RequestStart(unsigned mode, float target_volts)
{
    float vin = power_status.v_left;
    if (power_status.state != POWER_STOPPED || start_pending != 0U) return 0;
    if ((mode != 1U && mode != 2U) || power_status.samples < 2U ||
        !sample_valid ||
        !(target_volts >= 1.0f && target_volts <= 36.0f) ||
        !(vin >= INPUT_MIN_VOLTS && vin <= INPUT_MAX_VOLTS) ||
        !(fabsf(power_status.il) < 0.25f) ||
        /* Original precharge starts with a tiny left duty. A charged output
         * would drive reverse current before the voltage filter catches up. */
        power_status.v_right > 0.25f ||
        (mode == 2U && vin >= target_volts) ||
        (mode == 1U && vin <= target_volts)) {
        trip(POWER_FAULT_START);
        return 0;
    }
    active_mode = mode;
    active_target = target_volts;
    forward_configure(mode == 2U ? vin : 0.1f, target_volts, CURRENT_REFERENCE_LIMIT);
    forward_initialize(0.0f);
    forward_U.mode = (float)mode;
    power_status.isr_cycles_max = 0U;
    __DMB();
    start_pending = 1U;
    return 1;
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
}

void Power_TimerIRQ(void)
{
    const uint32_t begin = DWT->CYCCNT;
    uint32_t flags = ADC1->ISR;
    if (!(HRTIM1->sTimerxRegs[TIMER_B].TIMxISR & HRTIM_TIMISR_REP)) return;
    HRTIM1->sTimerxRegs[TIMER_B].TIMxICR = HRTIM_TIMICR_REPC;
    /* REP and JEOS are sticky: they cannot count missed periods. Compare
     * successive entry timestamps as well as checking execution duration. */
    uint32_t spacing = begin - previous_entry;
    previous_entry = begin;
    if (have_previous_entry && power_status.state != POWER_STOPPED &&
        power_status.state != POWER_FAULT &&
        !PWM_EntryIsTimely(spacing, HRTIM1->sTimerxRegs[TIMER_B].CNTxR)) {
        trip(POWER_FAULT_TIMING);
        return;
    }
    have_previous_entry = 1U;
    if (!(flags & ADC_ISR_JEOS) || (flags & ADC_ISR_JQOVF)) {
        sample_valid = 0U;
        if (start_pending || (power_status.state != POWER_STOPPED &&
                              power_status.state != POWER_FAULT))
            trip(POWER_FAULT_ADC);
        ADC1->ISR = ADC_ISR_JQOVF | ADC_ISR_JEOS | ADC_ISR_JEOC;
        return;
    }
    power_status.raw_il = (uint16_t)ADC1->JDR1;
    power_status.raw_vh = (uint16_t)ADC1->JDR2;
    power_status.raw_vl = (uint16_t)ADC1->JDR3;
    ADC1->ISR = ADC_ISR_JEOS | ADC_ISR_JEOC;
    power_status.il = (float)power_status.raw_il * (3.3f / 4095.0f / 0.1f) - 25.0f;
    power_status.v_left = (float)power_status.raw_vh * (3.3f / 4095.0f / 0.047f);
    power_status.v_right = (float)power_status.raw_vl * (3.3f / 4095.0f / 0.047f);
    ++power_status.samples;
    sample_valid = 1U;
    if (power_status.state == POWER_FAULT) return;
    if (start_pending) {
        start_pending = 0U;
        bootstrap_cycles = 30U; /* 200 us with high outputs disabled */
        HRTIM1->sTimerxRegs[TIMER_A].CMP1xR = PWM_MIN_COMPARE_TICKS;
        HRTIM1->sTimerxRegs[TIMER_B].CMP1xR = PWM_MIN_COMPARE_TICKS;
        power_status.state = POWER_BOOTSTRAP;
        HRTIM1->sCommonRegs.OENR = LOW_OUTPUTS;
    }
    if (power_status.state == POWER_STOPPED) return;
    if (!(power_status.v_left >= INPUT_MIN_VOLTS && power_status.v_left <= INPUT_MAX_VOLTS))
        trip(POWER_FAULT_INPUT);
    if (power_status.v_right > active_target * 1.1f)
        trip(POWER_FAULT_OUTPUT);
    if (power_status.il >= CURRENT_TRIP_AMPS || power_status.raw_il >= 4090U ||
        power_status.raw_il <= 5U) trip(POWER_FAULT_CURRENT);
    if (power_status.il < -1.0f) trip(POWER_FAULT_REVERSE);
    if (power_status.state == POWER_FAULT) return;

    if (power_status.state == POWER_BOOTSTRAP) {
        if (--bootstrap_cycles == 0U) {
            PwmPlan initial;
            (void)PWM_MakePlan(0.0f, 0, &initial);
            write_plan(&initial);
            arming_cycles = 2U;
            power_status.state = POWER_ARMING;
        }
        return;
    }
    if (power_status.state == POWER_ARMING) {
        if (--arming_cycles != 0U) return;
        HRTIM1->sCommonRegs.OENR = ALL_OUTPUTS;
        power_status.state = POWER_RUNNING;
    }
    forward_U.IL = power_status.il;
    forward_U.V_left = power_status.v_left;
    forward_U.V_right = power_status.v_right;
    forward_U.mode = (float)active_mode;
    forward_step();
    PwmPlan plan;
    if (forward_errorStatus || !PWM_MakePlan(forward_Y.D_cmd,
                                             forward_Y.BOOST_RUN, &plan)) {
        trip(POWER_FAULT_CONTROLLER);
        return;
    }
    /* Do not write either preload close to the common update boundary. */
    if (DWT->CYCCNT - begin > cycle_budget - 160U ||
        HRTIM1->sTimerxRegs[TIMER_B].CNTxR > PWM_PERIOD_TICKS - 5120U) {
        trip(POWER_FAULT_TIMING);
        return;
    }
    write_plan(&plan);
    power_status.d_cmd = forward_Y.D_cmd;
    power_status.v_reference = forward_B.Saturation4;
    uint32_t elapsed = DWT->CYCCNT - begin;
    if (elapsed > power_status.isr_cycles_max) power_status.isr_cycles_max = elapsed;
    if (elapsed > cycle_budget - 64U ||
        (HRTIM1->sTimerxRegs[TIMER_B].TIMxISR & HRTIM_TIMISR_REP))
        trip(POWER_FAULT_TIMING);
}
