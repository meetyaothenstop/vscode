#include "power_protection.h"

#define OCP_OUTPUTS (HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2)
#define OCP_FLAGS (HRTIM_FLAG_FLT1 | HRTIM_FLAG_FLT4)
#define OCP_IRQS (HRTIM_IT_FLT1 | HRTIM_IT_FLT4)
#define OCP_TIMER_FAULTS (HRTIM_FLTR_FLT1EN | HRTIM_FLTR_FLT4EN)
static volatile unsigned configured;

#if POWER_OCP_WIRING_CONFIRMED
static DAC_HandleTypeDef ocp_dac;
static COMP_HandleTypeDef upper_comp, lower_comp;
static HRTIM_HandleTypeDef *ocp_timer;

static uint32_t analog_faults(void)
{
    uint32_t faults = 0U;
    /* G4 CSR.VALUE is BEFORE output inversion (RM0440 COMP_CSR).
     * COMP2 is inverted for the lower threshold, so raw LOW means fault. */
    if (COMP1->CSR & COMP_CSR_VALUE) faults |= HRTIM_FLAG_FLT4;
    if (!(COMP2->CSR & COMP_CSR_VALUE)) faults |= HRTIM_FLAG_FLT1;
    return faults;
}
#endif

int PowerProtection_IsReady(void)
{
    return configured != 0U;
}

uint32_t PowerProtection_FaultPending(void)
{
#if POWER_OCP_WIRING_CONFIRMED
    if (configured)
        return (HRTIM1->sCommonRegs.ISR & OCP_FLAGS) | analog_faults();
#endif
    return 0U;
}

void PowerProtection_MaskIRQ(void)
{
    if (RCC->APB2ENR & RCC_APB2ENR_HRTIM1EN)
        HRTIM1->sCommonRegs.IER &= ~OCP_IRQS;
}

int PowerProtection_Acknowledge(void)
{
#if POWER_OCP_WIRING_CONFIRMED
    if (configured) {
        if (analog_faults()) return 0;
        __HAL_HRTIM_CLEAR_FLAG(ocp_timer, OCP_FLAGS);
        HAL_NVIC_ClearPendingIRQ(HRTIM1_FLT_IRQn);
        /* Recheck a concurrent event without ever touching OENR. */
        if (PowerProtection_FaultPending()) return 0;
        HRTIM1->sCommonRegs.IER |= OCP_IRQS;
        if (PowerProtection_FaultPending()) {
            PowerProtection_MaskIRQ();
            return 0;
        }
    }
#endif
    return 1;
}

HAL_StatusTypeDef PowerProtection_Init(HRTIM_HandleTypeDef *hrtim)
{
    configured = 0U;
    HAL_NVIC_DisableIRQ(HRTIM1_FLT_IRQn);
    if (RCC->APB2ENR & RCC_APB2ENR_HRTIM1EN)
        HRTIM1->sCommonRegs.ODISR = OCP_OUTPUTS;
    PowerProtection_MaskIRQ();
    if (hrtim == 0 || hrtim->Instance != HRTIM1 ||
        !(RCC->APB2ENR & RCC_APB2ENR_HRTIM1EN)) return HAL_ERROR;
#if !POWER_OCP_WIRING_CONFIRMED
    /* The existing board connection is ADC-only. Until the two additional
     * comparator inputs are verified, leave those GPIOs untouched and let
     * Power_RequestStart report POWER_FAULT_PROTECTION (512). */
    return HAL_OK;
#else
    ocp_timer = hrtim;
    GPIO_InitTypeDef gpio = {0};
    DAC_ChannelConfTypeDef dac = {0};
    HRTIM_FaultCfgTypeDef fault = {0};
    uint32_t delta_mv = POWER_OCP_LIMIT_MA * POWER_OCP_MV_PER_AMP / 1000U;
    uint32_t high_mv = POWER_OCP_ZERO_MV + delta_mv;
    if (delta_mv >= POWER_OCP_ZERO_MV || high_mv >= POWER_OCP_DAC_REFERENCE_MV)
        goto fail; /* Refuse an unrepresentable threshold; never clamp it. */
    uint32_t low_mv = POWER_OCP_ZERO_MV - delta_mv;
    /* DAC transfer is VREF+ * DOR /4096, unlike the ADC /4095 conversion. */
    uint32_t high_code = (high_mv * 4096U + POWER_OCP_DAC_REFERENCE_MV / 2U) /
                         POWER_OCP_DAC_REFERENCE_MV;
    uint32_t low_code = (low_mv * 4096U + POWER_OCP_DAC_REFERENCE_MV / 2U) /
                        POWER_OCP_DAC_REFERENCE_MV;
    if (high_code > 4095U || low_code == 0U) goto fail;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_DAC3_CLK_ENABLE();
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    gpio.Pin = GPIO_PIN_1; /* PB1 = COMP1_INP IO2. PA2 ADC remains connected. */
    HAL_GPIO_Init(GPIOB, &gpio);
    gpio.Pin = GPIO_PIN_7; /* PA7 = COMP2_INP IO1. Same INA240 OUT signal. */
    HAL_GPIO_Init(GPIOA, &gpio);

    ocp_dac.Instance = DAC3;
    if (HAL_DAC_Init(&ocp_dac) != HAL_OK) goto fail;
    dac.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
    dac.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    dac.DAC_Trigger = DAC_TRIGGER_NONE;
    dac.DAC_Trigger2 = DAC_TRIGGER_NONE;
    dac.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    dac.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_INTERNAL;
    dac.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    if (HAL_DAC_ConfigChannel(&ocp_dac, &dac, DAC_CHANNEL_1) != HAL_OK ||
        HAL_DAC_ConfigChannel(&ocp_dac, &dac, DAC_CHANNEL_2) != HAL_OK ||
        HAL_DAC_SetValue(&ocp_dac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, high_code) != HAL_OK ||
        HAL_DAC_SetValue(&ocp_dac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, low_code) != HAL_OK ||
        HAL_DAC_Start(&ocp_dac, DAC_CHANNEL_1) != HAL_OK ||
        HAL_DAC_Start(&ocp_dac, DAC_CHANNEL_2) != HAL_OK) goto fail;
    /* HAL Start allows DAC startup/settling before COMP enable. MODE3 is
     * normal, buffer disabled, internal connection (not sample/hold MODE4/5). */
    if ((DAC3->CR & (DAC_CR_EN1 | DAC_CR_EN2)) != (DAC_CR_EN1 | DAC_CR_EN2) ||
        (DAC3->MCR & (DAC_MCR_MODE1 | DAC_MCR_MODE2)) != (3U | (3U << 16U)) ||
        DAC3->DOR1 != high_code || DAC3->DOR2 != low_code) goto fail;

    upper_comp.Instance = COMP1;
    upper_comp.Init.InputPlus = COMP_INPUT_PLUS_IO2;
    upper_comp.Init.InputMinus = COMP_INPUT_MINUS_DAC3_CH1;
    upper_comp.Init.Hysteresis = COMP_HYSTERESIS_10MV;
    upper_comp.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
    upper_comp.Init.BlankingSrce = COMP_BLANKINGSRC_NONE;
    upper_comp.Init.TriggerMode = COMP_TRIGGERMODE_NONE;
    lower_comp.Instance = COMP2;
    lower_comp.Init = upper_comp.Init;
    lower_comp.Init.InputPlus = COMP_INPUT_PLUS_IO1;
    lower_comp.Init.InputMinus = COMP_INPUT_MINUS_DAC3_CH2;
    lower_comp.Init.OutputPol = COMP_OUTPUTPOL_INVERTED;
    if (HAL_COMP_Init(&upper_comp) != HAL_OK || HAL_COMP_Init(&lower_comp) != HAL_OK ||
        HAL_COMP_Start(&upper_comp) != HAL_OK || HAL_COMP_Start(&lower_comp) != HAL_OK ||
        HAL_COMP_Lock(&upper_comp) != HAL_OK || HAL_COMP_Lock(&lower_comp) != HAL_OK)
        goto fail;
    if ((COMP1->CSR & (COMP_CSR_EN | COMP_CSR_LOCK)) != (COMP_CSR_EN | COMP_CSR_LOCK) ||
        (COMP2->CSR & (COMP_CSR_EN | COMP_CSR_LOCK)) != (COMP_CSR_EN | COMP_CSR_LOCK))
        goto fail;

    /* RM0440 Table228: COMP1→FAULT4, COMP2→FAULT1. Both must feed A AND B. */
    fault.Source = HRTIM_FAULTSOURCE_INTERNAL;
    fault.Polarity = HRTIM_FAULTPOLARITY_HIGH;
    fault.Filter = HRTIM_FAULTFILTER_NONE;
    fault.Lock = HRTIM_FAULTLOCK_READWRITE;
    if (HAL_HRTIM_FaultConfig(hrtim, HRTIM_FAULT_1, &fault) != HAL_OK ||
        HAL_HRTIM_FaultConfig(hrtim, HRTIM_FAULT_4, &fault) != HAL_OK) goto fail;
    /* Each FLTINR3 byte controls one fault. Disable blanking/counters for
     * Fault1 (low byte) and Fault4 (high byte); preserve Fault2/3 fields. */
    HRTIM1->sCommonRegs.FLTINR3 &= ~0xFF0000FFU;
    for (unsigned timer = HRTIM_TIMERINDEX_TIMER_A; timer <= HRTIM_TIMERINDEX_TIMER_B; ++timer) {
        if (HRTIM1->sTimerxRegs[timer].OUTxR & (HRTIM_OUTR_POL1 | HRTIM_OUTR_POL2))
            goto fail; /* The board expects active-high gate drive. */
        HRTIM1->sTimerxRegs[timer].OUTxR =
            (HRTIM1->sTimerxRegs[timer].OUTxR & ~(HRTIM_OUTR_FAULT1 | HRTIM_OUTR_FAULT2)) |
            HRTIM_OUTR_FAULT1_1 | HRTIM_OUTR_FAULT2_1;
        HRTIM1->sTimerxRegs[timer].FLTxR |= OCP_TIMER_FAULTS | HRTIM_FLTR_FLTLCK;
        if ((HRTIM1->sTimerxRegs[timer].FLTxR & (OCP_TIMER_FAULTS | HRTIM_FLTR_FLTLCK)) !=
            (OCP_TIMER_FAULTS | HRTIM_FLTR_FLTLCK)) goto fail;
    }
    HAL_HRTIM_FaultModeCtl(hrtim, HRTIM_FAULT_1 | HRTIM_FAULT_4, HRTIM_FAULTMODECTL_ENABLED);
    /* Common locks also freeze enable bits. Enable FIRST, lock SECOND. */
    HRTIM1->sCommonRegs.FLTINR1 |= HRTIM_FLTINR1_FLT1LCK | HRTIM_FLTINR1_FLT4LCK;
    if ((HRTIM1->sCommonRegs.FLTINR1 & (HRTIM_FLTINR1_FLT1E | HRTIM_FLTINR1_FLT4E)) !=
        (HRTIM_FLTINR1_FLT1E | HRTIM_FLTINR1_FLT4E)) goto fail;
    configured = 1U;
    __DMB();
    HAL_NVIC_SetPriority(HRTIM1_FLT_IRQn, 0U, 0U);
    HAL_NVIC_ClearPendingIRQ(HRTIM1_FLT_IRQn);
    HRTIM1->sCommonRegs.IER |= OCP_IRQS;
    HAL_NVIC_EnableIRQ(HRTIM1_FLT_IRQn);
    return HAL_OK;
fail:
    configured = 0U;
    HRTIM1->sCommonRegs.ODISR = OCP_OUTPUTS;
    PowerProtection_MaskIRQ();
    return HAL_ERROR;
#endif
}
