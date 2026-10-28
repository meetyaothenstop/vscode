#ifndef POWER_TEST_STM32G4XX_HAL_H
#define POWER_TEST_STM32G4XX_HAL_H

/* Isolated register/HAL fixture for the real power backend/protection tests.
 * This header must precede Core/Inc on that executable's include path.
 * Other host test families use tests/stm32g4xx_hal.h. */
#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
typedef struct { volatile uint32_t ISR, JDR1, JDR2, JDR3, DR; } ADC_TypeDef;
typedef struct {
    /* Synthetic W1C semantics: clearing REP writes zero to this shared cell.
     * Real silicon has separate ISR/ICR addresses and writes one to clear.
     * Aliasing only these fixture registers lets the unchanged production
     * write clear REP, including its end-of-ISR pending-period check. */
    union { volatile uint32_t TIMxISR; volatile uint32_t TIMxICR; };
    volatile uint32_t TIMxDIER, CNTxR, CMP1xR, CMP2xR, CMP3xR, CMP4xR, OUTxR, FLTxR;
} HRTIM_Timerx_TypeDef;
typedef struct {
    volatile uint32_t ODISR, OENR, CR1, CR2, ISR, ICR, IER;
    volatile uint32_t FLTINR1, FLTINR2, FLTINR3, FLTINR4;
} HRTIM_Common_TypeDef;
typedef struct {
    HRTIM_Timerx_TypeDef sTimerxRegs[2];
    HRTIM_Common_TypeDef sCommonRegs;
} HRTIM_TypeDef;
typedef struct { volatile uint32_t APB2ENR, AHB2ENR, APB1ENR1, APB1ENR2; } RCC_TypeDef;
typedef struct { volatile uint32_t MODER, PUPDR; } GPIO_TypeDef;
typedef struct { volatile uint32_t CR, MCR, DHR12R1, DHR12R2, DOR1, DOR2; } DAC_TypeDef;
typedef struct { volatile uint32_t CSR; } COMP_TypeDef;
typedef struct { volatile uint32_t DEMCR; } CoreDebug_Type;
typedef struct { volatile uint32_t CTRL, CYCCNT; } DWT_Type;
typedef struct {
    uint32_t ClockPrescaler, Resolution, DataAlign, GainCompensation, ScanConvMode;
    uint32_t EOCSelection, LowPowerAutoWait, ContinuousConvMode, NbrOfConversion;
    uint32_t DiscontinuousConvMode, ExternalTrigConv, ExternalTrigConvEdge;
    uint32_t DMAContinuousRequests, Overrun, OversamplingMode;
} ADC_InitTypeDef;
typedef struct { ADC_TypeDef *Instance; ADC_InitTypeDef Init; } ADC_HandleTypeDef;
typedef struct {
    uint32_t Channel, Rank, SamplingTime, SingleDiff, OffsetNumber, Offset;
} ADC_ChannelConfTypeDef;
typedef struct { HRTIM_TypeDef *Instance; } HRTIM_HandleTypeDef;
typedef struct {
    uint32_t InjectedNbrOfConversion, InjectedSamplingTime, InjectedSingleDiff;
    uint32_t InjectedOffsetNumber, ExternalTrigInjecConv, ExternalTrigInjecConvEdge;
    uint32_t QueueInjectedContext, AutoInjectedConv, InjectedChannel, InjectedRank;
} ADC_InjectionConfTypeDef;
typedef struct { uint32_t UpdateSource, Trigger; } HRTIM_ADCTriggerCfgTypeDef;
typedef struct { uint32_t CompareValue; } HRTIM_CompareCfgTypeDef;
typedef struct { uint32_t Pin, Mode, Pull, Speed, Alternate; } GPIO_InitTypeDef;
typedef struct {
    uint32_t InputPlus, InputMinus, Hysteresis, OutputPol, BlankingSrce, TriggerMode;
} COMP_InitTypeDef;
typedef struct { COMP_TypeDef *Instance; COMP_InitTypeDef Init; uint32_t State, Lock, ErrorCode; } COMP_HandleTypeDef;
typedef struct { DAC_TypeDef *Instance; uint32_t State, Lock, ErrorCode; } DAC_HandleTypeDef;
typedef struct { uint32_t DAC_SampleTime, DAC_HoldTime, DAC_RefreshTime; } DAC_SampleAndHoldConfTypeDef;
typedef struct {
    uint32_t DAC_HighFrequency, DAC_DMADoubleDataMode, DAC_SignedFormat;
    uint32_t DAC_SampleAndHold, DAC_Trigger, DAC_Trigger2, DAC_OutputBuffer;
    uint32_t DAC_ConnectOnChipPeripheral, DAC_UserTrimming, DAC_TrimmingValue;
    DAC_SampleAndHoldConfTypeDef DAC_SampleAndHoldConfig;
} DAC_ChannelConfTypeDef;
typedef struct { uint32_t Source, Polarity, Filter, Lock; } HRTIM_FaultCfgTypeDef;

extern ADC_TypeDef test_adc1;
extern ADC_TypeDef test_adc2;
extern HRTIM_TypeDef test_hrtim1;
extern RCC_TypeDef test_rcc;
extern CoreDebug_Type test_core_debug;
extern DWT_Type test_dwt;
extern uint32_t test_primask;
extern GPIO_TypeDef test_gpioa, test_gpiob;
extern DAC_TypeDef test_dac3;
extern COMP_TypeDef test_comp1, test_comp2;
#define ADC1 (&test_adc1)
#define ADC2 (&test_adc2)
#define HRTIM1 (&test_hrtim1)
#define RCC (&test_rcc)
#define CoreDebug (&test_core_debug)
#if defined(POWER_UNIFIED_DIAGNOSTICS) && POWER_UNIFIED_DIAGNOSTICS
DWT_Type *test_dwt_access(void);
#define DWT test_dwt_access()
#else
#define DWT (&test_dwt)
#endif
#define GPIOA (&test_gpioa)
#define GPIOB (&test_gpiob)
#define DAC3 (&test_dac3)
#define COMP1 (&test_comp1)
#define COMP2 (&test_comp2)

/* Values below match the STM32G474 CMSIS definitions. */
#define GPIO_PIN_1 (1U << 1)
#define GPIO_PIN_2 (1U << 2)
#define GPIO_PIN_7 (1U << 7)
#define GPIO_MODE_ANALOG 3U
#define GPIO_NOPULL 0U
#define GPIO_SPEED_FREQ_LOW 0U
#define DAC_CHANNEL_1 0U
#define DAC_CHANNEL_2 16U
#define DAC_ALIGN_12B_R 0U
#define DAC_TRIGGER_NONE 0U
#define DAC_SAMPLEANDHOLD_DISABLE 0U
#define DAC_OUTPUTBUFFER_DISABLE 2U
#define DAC_CHIPCONNECT_INTERNAL 2U
#define DAC_TRIMMING_FACTORY 0U
#define DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC 2U
#define DAC_CR_EN1 (1U << 0)
#define DAC_CR_EN2 (1U << 16)
#define DAC_MCR_MODE1 7U
#define DAC_MCR_MODE1_0 1U
#define DAC_MCR_MODE1_1 2U
#define DAC_MCR_MODE1_2 4U
#define DAC_MCR_MODE2 (7U << 16)
#define DAC_MCR_MODE2_0 (1U << 16)
#define DAC_MCR_MODE2_1 (2U << 16)
#define DAC_MCR_MODE2_2 (4U << 16)
#define COMP_CSR_EN (1U << 0)
#define COMP_CSR_INMSEL_2 (1U << 6)
#define COMP_CSR_INPSEL (1U << 8)
#define COMP_CSR_POLARITY (1U << 15)
#define COMP_CSR_HYST_0 (1U << 16)
#define COMP_CSR_VALUE (1U << 30)
#define COMP_CSR_LOCK (1U << 31)
#define COMP_INPUT_PLUS_IO1 0U
#define COMP_INPUT_PLUS_IO2 COMP_CSR_INPSEL
#define COMP_INPUT_MINUS_DAC3_CH1 COMP_CSR_INMSEL_2
#define COMP_INPUT_MINUS_DAC3_CH2 COMP_CSR_INMSEL_2
#define COMP_OUTPUTPOL_NONINVERTED 0U
#define COMP_OUTPUTPOL_INVERTED COMP_CSR_POLARITY
#define COMP_HYSTERESIS_10MV COMP_CSR_HYST_0
#define COMP_BLANKINGSRC_NONE 0U
#define COMP_TRIGGERMODE_NONE 0U
#define COMP_OUTPUT_LEVEL_LOW 0U
#define COMP_OUTPUT_LEVEL_HIGH 1U
#define HRTIM_FAULT_1 1U
#define HRTIM_FAULT_4 8U
#define HRTIM_FAULTSOURCE_INTERNAL 1U
#define HRTIM_FAULTPOLARITY_HIGH 2U
#define HRTIM_FAULTFILTER_NONE 0U
#define HRTIM_FAULTLOCK_READWRITE 0U
#define HRTIM_FAULTLOCK_READONLY (1U << 7)
#define HRTIM_FAULTMODECTL_ENABLED 1U
#define HRTIM_FAULTMODECTL_DISABLED 0U
#define HRTIM_FLTINR1_FLT1E (1U << 0)
#define HRTIM_FLTINR1_FLT1P (1U << 1)
#define HRTIM_FLTINR1_FLT1SRC_0 (1U << 2)
#define HRTIM_FLTINR1_FLT1LCK (1U << 7)
#define HRTIM_FLTINR1_FLT4E (1U << 24)
#define HRTIM_FLTINR1_FLT4P (1U << 25)
#define HRTIM_FLTINR1_FLT4SRC_0 (1U << 26)
#define HRTIM_FLTINR1_FLT4LCK (1U << 31)
#define HRTIM_FLTR_FLT1EN (1U << 0)
#define HRTIM_FLTR_FLT4EN (1U << 3)
#define HRTIM_FLTR_FLTLCK (1U << 31)
#define HRTIM_OUTR_POL1 (1U << 1)
#define HRTIM_OUTR_POL2 (1U << 17)
#define HRTIM_OUTR_FAULT1 (3U << 4)
#define HRTIM_OUTR_FAULT1_0 (1U << 4)
#define HRTIM_OUTR_FAULT1_1 (2U << 4)
#define HRTIM_OUTR_FAULT2 (3U << 20)
#define HRTIM_OUTR_FAULT2_0 (1U << 20)
#define HRTIM_OUTR_FAULT2_1 (2U << 20)
#define HRTIM_ISR_FLT1 1U
#define HRTIM_ISR_FLT4 8U
#define HRTIM_ICR_FLT1C 1U
#define HRTIM_ICR_FLT4C 8U
#define HRTIM_IER_FLT1 1U
#define HRTIM_IER_FLT4 8U
#define HRTIM_FLAG_FLT1 HRTIM_ISR_FLT1
#define HRTIM_FLAG_FLT4 HRTIM_ISR_FLT4
#define HRTIM_IT_FLT1 HRTIM_IER_FLT1
#define HRTIM_IT_FLT4 HRTIM_IER_FLT4
#define HRTIM1_FLT_IRQn 2

void test_power_clock_enable(uint32_t);
void test_hrtim_clear_flag(HRTIM_HandleTypeDef *, uint32_t);
#define __HAL_RCC_GPIOA_CLK_ENABLE() test_power_clock_enable(1U)
#define __HAL_RCC_GPIOB_CLK_ENABLE() test_power_clock_enable(2U)
#define __HAL_RCC_DAC3_CLK_ENABLE() test_power_clock_enable(4U)
#define __HAL_RCC_SYSCFG_CLK_ENABLE() test_power_clock_enable(8U)
#define __HAL_HRTIM_CLEAR_FLAG(handle, flags) test_hrtim_clear_flag((handle), (flags))
#define __HAL_HRTIM_ENABLE_IT(handle, flags) ((handle)->Instance->sCommonRegs.IER |= (flags))
#define __HAL_HRTIM_DISABLE_IT(handle, flags) ((handle)->Instance->sCommonRegs.IER &= ~(flags))
#define READ_REG(reg) (reg)
#define WRITE_REG(reg, value) ((reg) = (value))
#define SET_BIT(reg, bits) ((reg) |= (bits))
#define CLEAR_BIT(reg, bits) ((reg) &= ~(bits))
#define MODIFY_REG(reg, clear, set) ((reg) = (((reg) & ~(clear)) | (set)))

#define RCC_APB2ENR_HRTIM1EN (1U << 0)
#define HRTIM_OUTPUT_TA1 (1U << 0)
#define HRTIM_OUTPUT_TA2 (1U << 1)
#define HRTIM_OUTPUT_TB1 (1U << 2)
#define HRTIM_OUTPUT_TB2 (1U << 3)
#define HRTIM_TIMERINDEX_TIMER_A 0U
#define HRTIM_TIMERINDEX_TIMER_B 1U
#define HRTIM_TIMERID_TIMER_A (1U << 0)
#define HRTIM_TIMERID_TIMER_B (1U << 1)
#define HRTIM_CR1_TAUDIS (1U << 0)
#define HRTIM_CR1_TBUDIS (1U << 1)
#define HRTIM_CR2_TASWU (1U << 0)
#define HRTIM_CR2_TBSWU (1U << 1)
#define HRTIM_CR2_TARST (1U << 2)
#define HRTIM_CR2_TBRST (1U << 3)
#define HRTIM_TIMISR_REP (1U << 0)
#define HRTIM_TIMICR_REPC 0U
#define HRTIM_TIMDIER_REPIE (1U << 0)
#define HRTIM_COMPAREUNIT_3 3U
#define HRTIM_COMPAREUNIT_2 2U
#define HRTIM_COMPAREUNIT_4 4U
#define HRTIM_ADCTRIGGERUPDATE_TIMER_A 1U
#define HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP3 1U
#define HRTIM_ADCTRIGGEREVENT24_TIMERA_CMP2 2U
#define HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP4 4U
#define HRTIM_ADCTRIGGER_1 1U
#define HRTIM_ADCTRIGGER_2 2U
#define HRTIM1_TIMB_IRQn 1
#define ADC_ISR_JEOS (1U << 0)
#define ADC_ISR_JEOC (1U << 1)
#define ADC_ISR_JQOVF (1U << 2)
#define ADC_ISR_EOC (1U << 3)
#define ADC_ISR_EOS (1U << 4)
#define ADC_ISR_OVR (1U << 5)
#define ADC_CLOCK_ASYNC_DIV4 4U
#define ADC_RESOLUTION_12B 12U
#define ADC_DATAALIGN_RIGHT 0U
#define ADC_SCAN_DISABLE 0U
#define ADC_EOC_SINGLE_CONV 1U
#define ADC_EXTERNALTRIG_HRTIM_TRG1 21U
#define ADC_EXTERNALTRIGCONVEDGE_RISING 1U
#define ADC_OVR_DATA_PRESERVED 0U
#define ADC_REGULAR_RANK_1 1U
#define ADC_SAMPLETIME_24CYCLES_5 245U
#define ADC_SINGLE_ENDED 0U
#define ADC_OFFSET_NONE 0U
#define ADC_EXTERNALTRIGINJEC_HRTIM_TRG1 1U
#define ADC_EXTERNALTRIGINJEC_HRTIM_TRG2 2U
#define ADC_EXTERNALTRIGINJECCONV_EDGE_RISING 1U
#define ADC_CHANNEL_1 1U
#define ADC_CHANNEL_2 2U
#define ADC_CHANNEL_3 3U
#define ADC_INJECTED_RANK_1 1U
#define ADC_INJECTED_RANK_2 2U
#define ADC_INJECTED_RANK_3 3U
#define DISABLE 0U
#define ENABLE 1U
#define CoreDebug_DEMCR_TRCENA_Msk (1U << 0)
#define DWT_CTRL_CYCCNTENA_Msk (1U << 0)

static inline uint32_t __get_PRIMASK(void) { return test_primask; }
static inline void __disable_irq(void) { test_primask = 1U; }
static inline void __set_PRIMASK(uint32_t value) { test_primask = value; }
static inline void __DMB(void) { }
uint32_t HAL_RCC_GetHCLKFreq(void);
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *, ADC_ChannelConfTypeDef *);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_InjectedConfigChannel(ADC_HandleTypeDef *, ADC_InjectionConfTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_InjectedStart(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_HRTIM_WaveformCompareConfig(HRTIM_HandleTypeDef *, uint32_t, uint32_t, HRTIM_CompareCfgTypeDef *);
HAL_StatusTypeDef HAL_HRTIM_ADCTriggerConfig(HRTIM_HandleTypeDef *, uint32_t, HRTIM_ADCTriggerCfgTypeDef *);
HAL_StatusTypeDef HAL_HRTIM_WaveformCounterStart(HRTIM_HandleTypeDef *, uint32_t);
void HAL_NVIC_SetPriority(int, uint32_t, uint32_t);
void HAL_NVIC_ClearPendingIRQ(int);
void HAL_NVIC_EnableIRQ(int);
void HAL_NVIC_DisableIRQ(int);
void HAL_GPIO_Init(GPIO_TypeDef *, GPIO_InitTypeDef *);
HAL_StatusTypeDef HAL_DAC_Init(DAC_HandleTypeDef *);
HAL_StatusTypeDef HAL_DAC_ConfigChannel(DAC_HandleTypeDef *, DAC_ChannelConfTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *, uint32_t, uint32_t, uint32_t);
HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_COMP_Init(COMP_HandleTypeDef *);
HAL_StatusTypeDef HAL_COMP_Start(COMP_HandleTypeDef *);
HAL_StatusTypeDef HAL_COMP_Lock(COMP_HandleTypeDef *);
uint32_t HAL_COMP_GetOutputLevel(const COMP_HandleTypeDef *);
HAL_StatusTypeDef HAL_HRTIM_FaultConfig(HRTIM_HandleTypeDef *, uint32_t, HRTIM_FaultCfgTypeDef *);
void HAL_HRTIM_FaultModeCtl(HRTIM_HandleTypeDef *, uint32_t, uint32_t);
#endif
