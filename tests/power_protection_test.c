/* Host tests link the real protection module against synthetic peripherals.
 * Build twice with POWER_OCP_WIRING_CONFIRMED=0 and =1. No hardware is accessed.
 */
#include "power_protection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expression) do { if (!(expression)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
    exit(EXIT_FAILURE); \
} } while (0)
#define ALL_OUTPUTS 15U
#define FAULTS 9U
#define OTHER_IRQ (1U << 10)
#define OTHER_FLAG (1U << 8)
#define OTHER_OUTR 0x10000100U
#define COMMON_ENABLES (HRTIM_FLTINR1_FLT1E | HRTIM_FLTINR1_FLT4E)
#define COMMON_LOCKS (HRTIM_FLTINR1_FLT1LCK | HRTIM_FLTINR1_FLT4LCK)

ADC_TypeDef test_adc1;
HRTIM_TypeDef test_hrtim1;
RCC_TypeDef test_rcc;
CoreDebug_Type test_core_debug;
DWT_Type test_dwt;
uint32_t test_primask;
GPIO_TypeDef test_gpioa, test_gpiob;
DAC_TypeDef test_dac3;
COMP_TypeDef test_comp1, test_comp2;

static unsigned clocks, gpio_calls, dac_init_calls, status_calls, fail_call;
static unsigned clear_calls, nvic_clear_calls, nvic_enable_calls, nvic_disable_calls;
static unsigned comp_init_mask, comp_start_mask, comp_lock_mask, fault_config_mask;
static unsigned dac_config_mask, dac_value_mask, dac_start_mask;
static unsigned inject_active_on_clear, inject_flag_on_clear;
static HAL_StatusTypeDef failure_status = HAL_ERROR;
static DAC_ChannelConfTypeDef dac_cfg[2];
static COMP_InitTypeDef comp_cfg[2];
static HRTIM_FaultCfgTypeDef fault_cfg[2];
static uint32_t gpio_pins_a, gpio_pins_b, dac_values[2];
static HRTIM_HandleTypeDef timer = { HRTIM1 };

static void outputs_stay_off(void)
{
    CHECK((HRTIM1->sCommonRegs.ODISR & ALL_OUTPUTS) == ALL_OUTPUTS);
    CHECK(HRTIM1->sCommonRegs.OENR == 0U);
}

static HAL_StatusTypeDef hal_step(void)
{
    outputs_stay_off();
    CHECK((HRTIM1->sCommonRegs.IER & FAULTS) == 0U);
    CHECK(!PowerProtection_IsReady());
    ++status_calls;
    return status_calls == fail_call ? failure_status : HAL_OK;
}

void test_power_clock_enable(uint32_t which) { outputs_stay_off(); clocks |= which; }
void HAL_GPIO_Init(GPIO_TypeDef *gpio, GPIO_InitTypeDef *cfg)
{
    outputs_stay_off();
    CHECK(cfg->Mode == GPIO_MODE_ANALOG && cfg->Pull == GPIO_NOPULL);
    if (gpio == GPIOA) gpio_pins_a |= cfg->Pin;
    else { CHECK(gpio == GPIOB); gpio_pins_b |= cfg->Pin; }
    ++gpio_calls;
}

static unsigned dac_channel(uint32_t channel)
{ CHECK(channel == DAC_CHANNEL_1 || channel == DAC_CHANNEL_2); return channel == DAC_CHANNEL_2; }

HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef *dac)
{
    CHECK(dac->Instance == DAC3);
    ++dac_init_calls;
    return hal_step();
}
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef *dac, DAC_ChannelConfTypeDef *cfg, uint32_t channel)
{
    unsigned index = dac_channel(channel);
    HAL_StatusTypeDef result = hal_step();
    CHECK(dac->Instance == DAC3);
    if (result != HAL_OK) return result;
    dac_cfg[index] = *cfg;
    dac_config_mask |= 1U << index;
    /* Mirror HAL's normal, internal-only, unbuffered MODE=3 path. */
    uint32_t mode = cfg->DAC_SampleAndHold | cfg->DAC_OutputBuffer;
    if (cfg->DAC_ConnectOnChipPeripheral == DAC_CHIPCONNECT_INTERNAL) mode |= 1U;
    DAC3->MCR = (DAC3->MCR & ~(7U << channel)) | (mode << channel);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t align, uint32_t value)
{
    unsigned index = dac_channel(channel);
    HAL_StatusTypeDef result = hal_step();
    CHECK(dac->Instance == DAC3 && align == DAC_ALIGN_12B_R && value <= 4095U);
    if (result != HAL_OK) return result;
    dac_values[index] = value;
    dac_value_mask |= 1U << index;
    if (index) DAC3->DHR12R2 = value; else DAC3->DHR12R1 = value;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *dac, uint32_t channel)
{
    unsigned index = dac_channel(channel);
    HAL_StatusTypeDef result = hal_step();
    CHECK(dac->Instance == DAC3);
    if (result != HAL_OK) return result;
    CHECK((dac_config_mask & dac_value_mask & (1U << index)) != 0U);
    DAC3->CR |= DAC_CR_EN1 << channel;
    if (index) DAC3->DOR2 = DAC3->DHR12R2; else DAC3->DOR1 = DAC3->DHR12R1;
    dac_start_mask |= 1U << index;
    return HAL_OK;
}

static unsigned comp_index(const COMP_HandleTypeDef *comp)
{ CHECK(comp->Instance == COMP1 || comp->Instance == COMP2); return comp->Instance == COMP2; }
HAL_StatusTypeDef HAL_COMP_Init(COMP_HandleTypeDef *comp)
{
    unsigned index = comp_index(comp);
    HAL_StatusTypeDef result = hal_step();
    if (result != HAL_OK) return result;
    CHECK((comp->Instance->CSR & COMP_CSR_LOCK) == 0U);
    comp_cfg[index] = comp->Init;
    comp->Instance->CSR = (comp->Instance->CSR & COMP_CSR_VALUE) | comp->Init.InputPlus |
        comp->Init.InputMinus | comp->Init.Hysteresis | comp->Init.OutputPol;
    comp_init_mask |= 1U << index;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_COMP_Start(COMP_HandleTypeDef *comp)
{
    unsigned index = comp_index(comp);
    HAL_StatusTypeDef result = hal_step();
    if (result != HAL_OK) return result;
    CHECK((comp_init_mask & (1U << index)) != 0U);
    CHECK((comp->Instance->CSR & COMP_CSR_LOCK) == 0U); /* Locking first prevents Start. */
    CHECK(dac_start_mask == 3U);
    comp->Instance->CSR |= COMP_CSR_EN;
    comp_start_mask |= 1U << index;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_COMP_Lock(COMP_HandleTypeDef *comp)
{
    unsigned index = comp_index(comp);
    HAL_StatusTypeDef result = hal_step();
    if (result != HAL_OK) return result;
    CHECK((comp_start_mask & (1U << index)) != 0U);
    comp->Instance->CSR |= COMP_CSR_LOCK;
    comp_lock_mask |= 1U << index;
    return HAL_OK;
}
uint32_t HAL_COMP_GetOutputLevel(const COMP_HandleTypeDef *comp)
{
    (void)comp_index(comp);
    /* G4 VALUE is before POLARITY; do not invert this fixture value. */
    return (comp->Instance->CSR & COMP_CSR_VALUE) ? COMP_OUTPUT_LEVEL_HIGH : COMP_OUTPUT_LEVEL_LOW;
}

HAL_StatusTypeDef HAL_HRTIM_FaultConfig(HRTIM_HandleTypeDef *hrtim, uint32_t fault,
                                       HRTIM_FaultCfgTypeDef *cfg)
{
    HAL_StatusTypeDef result = hal_step();
    CHECK(hrtim->Instance == HRTIM1 && (fault == HRTIM_FAULT_1 || fault == HRTIM_FAULT_4));
    if (result != HAL_OK) return result;
    CHECK(cfg->Lock == HRTIM_FAULTLOCK_READWRITE);
    unsigned index = fault == HRTIM_FAULT_4;
    unsigned shift = index ? 24U : 0U;
    fault_cfg[index] = *cfg;
    CHECK((HRTIM1->sCommonRegs.FLTINR1 & (HRTIM_FLTINR1_FLT1LCK << shift)) == 0U);
    HRTIM1->sCommonRegs.FLTINR1 = (HRTIM1->sCommonRegs.FLTINR1 & ~(0xfeU << shift)) |
        (((cfg->Source << 2U) | cfg->Polarity | cfg->Filter | cfg->Lock) << shift);
    fault_config_mask |= fault;
    return HAL_OK;
}
void HAL_HRTIM_FaultModeCtl(HRTIM_HandleTypeDef *hrtim, uint32_t faults, uint32_t enable)
{
    outputs_stay_off();
    CHECK(hrtim->Instance == HRTIM1 && (faults & ~FAULTS) == 0U);
    CHECK(enable == HRTIM_FAULTMODECTL_ENABLED);
    CHECK(fault_config_mask == FAULTS);
    CHECK((HRTIM1->sCommonRegs.FLTINR1 & COMMON_LOCKS) == 0U);
    if (faults & HRTIM_FAULT_1) HRTIM1->sCommonRegs.FLTINR1 |= HRTIM_FLTINR1_FLT1E;
    if (faults & HRTIM_FAULT_4) HRTIM1->sCommonRegs.FLTINR1 |= HRTIM_FLTINR1_FLT4E;
}
void test_hrtim_clear_flag(HRTIM_HandleTypeDef *hrtim, uint32_t flags)
{
    CHECK(hrtim->Instance == HRTIM1 && (flags & ~FAULTS) == 0U);
    HRTIM1->sCommonRegs.ICR = flags;
    HRTIM1->sCommonRegs.ISR &= ~flags; /* W1C emulation is confined to the test. */
    ++clear_calls;
    if (inject_active_on_clear) COMP1->CSR |= COMP_CSR_VALUE;
    if (inject_flag_on_clear) HRTIM1->sCommonRegs.ISR |= HRTIM_ISR_FLT4;
}
void HAL_NVIC_SetPriority(int irq, uint32_t preempt, uint32_t sub)
{ CHECK(irq == HRTIM1_FLT_IRQn && preempt == 0U && sub == 0U); }
void HAL_NVIC_ClearPendingIRQ(int irq)
{ CHECK(irq == HRTIM1_FLT_IRQn); ++nvic_clear_calls; }
void HAL_NVIC_EnableIRQ(int irq)
{ CHECK(irq == HRTIM1_FLT_IRQn); ++nvic_enable_calls; }
void HAL_NVIC_DisableIRQ(int irq)
{ CHECK(irq == HRTIM1_FLT_IRQn); ++nvic_disable_calls; }
uint32_t HAL_RCC_GetHCLKFreq(void) { return 150000000U; }

static void reset_fixture(void)
{
    memset(&test_hrtim1, 0, sizeof(test_hrtim1));
    memset(&test_dac3, 0, sizeof(test_dac3));
    memset(&test_comp1, 0, sizeof(test_comp1));
    memset(&test_comp2, 0, sizeof(test_comp2));
    memset(&test_gpioa, 0, sizeof(test_gpioa));
    memset(&test_gpiob, 0, sizeof(test_gpiob));
    memset(dac_cfg, 0, sizeof(dac_cfg));
    memset(comp_cfg, 0, sizeof(comp_cfg));
    memset(fault_cfg, 0, sizeof(fault_cfg));
    clocks = gpio_calls = dac_init_calls = status_calls = fail_call = 0U;
    clear_calls = nvic_clear_calls = nvic_enable_calls = nvic_disable_calls = 0U;
    comp_init_mask = comp_start_mask = comp_lock_mask = fault_config_mask = 0U;
    dac_config_mask = dac_value_mask = dac_start_mask = 0U;
    inject_active_on_clear = inject_flag_on_clear = 0U;
    gpio_pins_a = gpio_pins_b = 0U;
    failure_status = HAL_ERROR;
    RCC->APB2ENR = RCC_APB2ENR_HRTIM1EN;
    COMP2->CSR = COMP_CSR_VALUE;
    HRTIM1->sCommonRegs.IER = OTHER_IRQ | FAULTS;
    HRTIM1->sCommonRegs.ISR = OTHER_FLAG;
    HRTIM1->sCommonRegs.FLTINR3 = 0xFFFFFFFFU;
    for (unsigned index = 0U; index < 2U; ++index) {
        HRTIM1->sTimerxRegs[index].OUTxR = OTHER_OUTR | HRTIM_OUTR_FAULT1 | HRTIM_OUTR_FAULT2;
        HRTIM1->sTimerxRegs[index].FLTxR = 1U << 2; /* Preserve independent fault selection. */
    }
}

static void wrong_handle_rejected(void)
{
    HRTIM_TypeDef other_hrtim = {0};
    HRTIM_HandleTypeDef wrong = { &other_hrtim };
    HRTIM_HandleTypeDef empty = { NULL };
    HRTIM_HandleTypeDef *cases[] = { NULL, &wrong, &empty };
    for (unsigned index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        reset_fixture();
        CHECK(PowerProtection_Init(cases[index]) == HAL_ERROR);
        outputs_stay_off();
        CHECK(!PowerProtection_IsReady());
        CHECK((HRTIM1->sCommonRegs.IER & FAULTS) == 0U);
        CHECK(gpio_calls == 0U && status_calls == 0U && clocks == 0U);
    }
    reset_fixture();
    RCC->APB2ENR = 0U;
    CHECK(PowerProtection_Init(&timer) == HAL_ERROR);
    CHECK(!PowerProtection_IsReady() && gpio_calls == 0U && status_calls == 0U);
    puts("Invalid handle and absent HRTIM clock never configure protection: PASS");
}

#if POWER_OCP_WIRING_CONFIRMED
static unsigned confirmed_init(void)
{
    reset_fixture();
    CHECK(PowerProtection_Init(&timer) == HAL_OK);
    outputs_stay_off();
    CHECK(PowerProtection_IsReady());
    CHECK((clocks & 15U) == 15U);
    CHECK(gpio_pins_a == GPIO_PIN_7 && gpio_pins_b == GPIO_PIN_1);
    CHECK(dac_init_calls == 1U && dac_config_mask == 3U && dac_start_mask == 3U);
    /* 2.49 V zero +/- 0.70 V, with measured 3.31 V DAC reference. */
    CHECK(dac_values[0] == 3948U && dac_values[1] == 2215U);
    for (unsigned index = 0U; index < 2U; ++index) {
        CHECK(dac_cfg[index].DAC_SampleAndHold == DAC_SAMPLEANDHOLD_DISABLE);
        CHECK(dac_cfg[index].DAC_OutputBuffer == DAC_OUTPUTBUFFER_DISABLE);
        CHECK(dac_cfg[index].DAC_ConnectOnChipPeripheral == DAC_CHIPCONNECT_INTERNAL);
        CHECK(dac_cfg[index].DAC_Trigger == DAC_TRIGGER_NONE);
        CHECK(dac_cfg[index].DAC_HighFrequency == DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC);
        CHECK(dac_cfg[index].DAC_UserTrimming == DAC_TRIMMING_FACTORY);
        CHECK(comp_cfg[index].Hysteresis == COMP_HYSTERESIS_10MV);
        CHECK(comp_cfg[index].BlankingSrce == COMP_BLANKINGSRC_NONE);
        CHECK(comp_cfg[index].TriggerMode == COMP_TRIGGERMODE_NONE);
        CHECK(fault_cfg[index].Source == HRTIM_FAULTSOURCE_INTERNAL);
        CHECK(fault_cfg[index].Polarity == HRTIM_FAULTPOLARITY_HIGH);
        CHECK(fault_cfg[index].Filter == HRTIM_FAULTFILTER_NONE);
        CHECK(fault_cfg[index].Lock == HRTIM_FAULTLOCK_READWRITE);
        CHECK(HRTIM1->sTimerxRegs[index].OUTxR ==
              (OTHER_OUTR | HRTIM_OUTR_FAULT1_1 | HRTIM_OUTR_FAULT2_1));
        CHECK(HRTIM1->sTimerxRegs[index].FLTxR ==
              ((1U << 2) | HRTIM_FLTR_FLT1EN | HRTIM_FLTR_FLT4EN | HRTIM_FLTR_FLTLCK));
    }
    CHECK(comp_cfg[0].InputPlus == COMP_INPUT_PLUS_IO2);
    CHECK(comp_cfg[1].InputPlus == COMP_INPUT_PLUS_IO1);
    CHECK(comp_cfg[0].InputMinus == COMP_INPUT_MINUS_DAC3_CH1);
    CHECK(comp_cfg[1].InputMinus == COMP_INPUT_MINUS_DAC3_CH2);
    CHECK(comp_cfg[0].OutputPol == COMP_OUTPUTPOL_NONINVERTED);
    CHECK(comp_cfg[1].OutputPol == COMP_OUTPUTPOL_INVERTED);
    CHECK(comp_init_mask == 3U && comp_start_mask == 3U && comp_lock_mask == 3U);
    CHECK((HRTIM1->sCommonRegs.FLTINR1 & (COMMON_ENABLES | COMMON_LOCKS)) ==
          (COMMON_ENABLES | COMMON_LOCKS));
    CHECK(HRTIM1->sCommonRegs.FLTINR3 == 0x00FFFF00U);
    CHECK(HRTIM1->sCommonRegs.IER == (OTHER_IRQ | FAULTS));
    CHECK(HRTIM1->sCommonRegs.ISR == OTHER_FLAG);
    CHECK(nvic_enable_calls > 0U);
    CHECK(PowerProtection_FaultPending() == 0U);
    return status_calls;
}

static void raw_levels_and_transient_latches(void)
{
    (void)confirmed_init();
    const struct { unsigned high, low; uint32_t expected; } cases[] = {
        {0U, 1U, 0U}, {1U, 1U, 8U}, {0U, 0U, 1U}, {1U, 0U, 9U}
    };
    unsigned old_clear = clear_calls;
    for (unsigned index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        COMP1->CSR = (COMP1->CSR & ~COMP_CSR_VALUE) | (cases[index].high ? COMP_CSR_VALUE : 0U);
        COMP2->CSR = (COMP2->CSR & ~COMP_CSR_VALUE) | (cases[index].low ? COMP_CSR_VALUE : 0U);
        CHECK(PowerProtection_FaultPending() == cases[index].expected);
    }
    COMP1->CSR &= ~COMP_CSR_VALUE;
    COMP2->CSR |= COMP_CSR_VALUE;
    HRTIM1->sCommonRegs.ISR |= HRTIM_ISR_FLT4;
    CHECK(PowerProtection_FaultPending() == 8U);
    HRTIM1->sCommonRegs.ISR |= HRTIM_ISR_FLT1;
    CHECK(PowerProtection_FaultPending() == 9U);
    CHECK(clear_calls == old_clear);
    outputs_stay_off();
    reset_fixture();
    HRTIM1->sCommonRegs.ISR |= FAULTS;
    CHECK(PowerProtection_Init(&timer) == HAL_OK);
    CHECK(PowerProtection_FaultPending() == FAULTS);
    CHECK(clear_calls == 0U);
    puts("Raw comparator levels use correct polarity; transient events stay latched: PASS");
}

static void mask_and_acknowledge(void)
{
    (void)confirmed_init();
    HRTIM1->sCommonRegs.ISR |= FAULTS;
    uint32_t hardware_enables = HRTIM1->sCommonRegs.FLTINR1;
    PowerProtection_MaskIRQ();
    CHECK(HRTIM1->sCommonRegs.IER == OTHER_IRQ);
    CHECK(HRTIM1->sCommonRegs.FLTINR1 == hardware_enables);
    CHECK(PowerProtection_FaultPending() == FAULTS);
    unsigned old_clear = clear_calls, old_nvic_clear = nvic_clear_calls;
    COMP1->CSR |= COMP_CSR_VALUE;
    CHECK(!PowerProtection_Acknowledge());
    CHECK(clear_calls == old_clear && nvic_clear_calls == old_nvic_clear);
    CHECK(HRTIM1->sCommonRegs.ISR == (OTHER_FLAG | FAULTS));
    CHECK(HRTIM1->sCommonRegs.IER == OTHER_IRQ);
    COMP1->CSR &= ~COMP_CSR_VALUE;
    COMP2->CSR &= ~COMP_CSR_VALUE;
    CHECK(!PowerProtection_Acknowledge());
    CHECK(clear_calls == old_clear && nvic_clear_calls == old_nvic_clear);
    COMP2->CSR |= COMP_CSR_VALUE;
    CHECK(PowerProtection_Acknowledge());
    CHECK(clear_calls > old_clear && nvic_clear_calls > old_nvic_clear);
    CHECK(HRTIM1->sCommonRegs.ISR == OTHER_FLAG);
    CHECK(HRTIM1->sCommonRegs.IER == (OTHER_IRQ | FAULTS));
    CHECK(PowerProtection_FaultPending() == 0U);
    CHECK(HRTIM1->sCommonRegs.FLTINR1 == hardware_enables);
    outputs_stay_off();
    puts("IRQ masking preserves shutdown; acknowledgement waits for both safe levels: PASS");
}

static void acknowledge_rechecks_races(void)
{
    for (unsigned kind = 0U; kind < 2U; ++kind) {
        (void)confirmed_init();
        PowerProtection_MaskIRQ();
        HRTIM1->sCommonRegs.ISR |= FAULTS;
        inject_active_on_clear = kind == 0U;
        inject_flag_on_clear = kind == 1U;
        CHECK(!PowerProtection_Acknowledge());
        CHECK(HRTIM1->sCommonRegs.IER == OTHER_IRQ);
        CHECK(PowerProtection_FaultPending() == 8U);
        outputs_stay_off();
    }
    puts("A fault arriving during acknowledgement remains unarmed: PASS");
}

static void initialization_failure_never_arms(void)
{
    unsigned operations = confirmed_init();
    CHECK(operations >= 15U);
    for (unsigned index = 1U; index <= operations; ++index) {
        reset_fixture();
        fail_call = index;
        failure_status = index % 3U == 0U ? HAL_BUSY : index % 3U == 1U ? HAL_TIMEOUT : HAL_ERROR;
        CHECK(PowerProtection_Init(&timer) == HAL_ERROR);
        CHECK(status_calls == index);
        CHECK(!PowerProtection_IsReady());
        CHECK((HRTIM1->sCommonRegs.IER & FAULTS) == 0U);
        CHECK(nvic_enable_calls == 0U);
        outputs_stay_off();
    }
    for (unsigned index = 0U; index < 2U; ++index) {
        reset_fixture();
        HRTIM1->sTimerxRegs[index].OUTxR |= index ? HRTIM_OUTR_POL2 : HRTIM_OUTR_POL1;
        CHECK(PowerProtection_Init(&timer) == HAL_ERROR);
        CHECK(!PowerProtection_IsReady());
        CHECK((HRTIM1->sCommonRegs.IER & FAULTS) == 0U);
        outputs_stay_off();
    }
    puts("Every HAL failure and unsafe output polarity leaves protection unready: PASS");
}
#else
static void unconfirmed_wiring_stays_unconfigured(void)
{
    reset_fixture();
    CHECK(PowerProtection_Init(&timer) == HAL_OK);
    outputs_stay_off();
    CHECK(!PowerProtection_IsReady());
    CHECK(clocks == 0U && gpio_calls == 0U && status_calls == 0U);
    CHECK(DAC3->CR == 0U && DAC3->MCR == 0U);
    CHECK(COMP1->CSR == 0U && COMP2->CSR == COMP_CSR_VALUE);
    CHECK(HRTIM1->sCommonRegs.IER == OTHER_IRQ);
    CHECK(HRTIM1->sCommonRegs.FLTINR1 == 0U);
    COMP1->CSR |= COMP_CSR_VALUE;
    COMP2->CSR &= ~COMP_CSR_VALUE;
    HRTIM1->sCommonRegs.ISR |= FAULTS;
    CHECK(PowerProtection_FaultPending() == 0U);
    CHECK(PowerProtection_Acknowledge());
    CHECK(HRTIM1->sCommonRegs.IER == OTHER_IRQ);
    CHECK(HRTIM1->sCommonRegs.ISR == (OTHER_FLAG | FAULTS));
    CHECK(clear_calls == 0U && nvic_enable_calls == 0U);
    PowerProtection_MaskIRQ();
    outputs_stay_off();
    puts("Unconfirmed wiring leaves analog peripherals unconfigured and PWM disabled: PASS");
}
#endif

int main(void)
{
    wrong_handle_rejected();
#if POWER_OCP_WIRING_CONFIRMED
    (void)confirmed_init();
    puts("Confirmed GPIO, threshold DACs, comparators and both HRTIM fault routes: PASS");
    raw_levels_and_transient_latches();
    mask_and_acknowledge();
    acknowledge_rechecks_races();
    initialization_failure_never_arms();
#else
    unconfirmed_wiring_stays_unconfigured();
#endif
    return EXIT_SUCCESS;
}
