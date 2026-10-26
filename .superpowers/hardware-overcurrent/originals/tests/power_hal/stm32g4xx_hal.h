#ifndef POWER_TEST_STM32G4XX_HAL_H
#define POWER_TEST_STM32G4XX_HAL_H

/* Isolated register/HAL fixture for the real power_control.c executable.
 * This header must precede Core/Inc on that executable's include path.
 * No other host test should include it. */
#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
typedef struct { volatile uint32_t ISR, JDR1, JDR2, JDR3; } ADC_TypeDef;
typedef struct {
    /* Synthetic W1C semantics: clearing REP writes zero to this shared cell.
     * Real silicon has separate ISR/ICR addresses and writes one to clear.
     * Aliasing only these fixture registers lets the unchanged production
     * write clear REP, including its end-of-ISR pending-period check. */
    union { volatile uint32_t TIMxISR; volatile uint32_t TIMxICR; };
    volatile uint32_t TIMxDIER, CNTxR, CMP1xR, CMP3xR;
} HRTIM_Timerx_TypeDef;
typedef struct { volatile uint32_t ODISR, OENR, CR1, CR2; } HRTIM_Common_TypeDef;
typedef struct {
    HRTIM_Timerx_TypeDef sTimerxRegs[2];
    HRTIM_Common_TypeDef sCommonRegs;
} HRTIM_TypeDef;
typedef struct { volatile uint32_t APB2ENR; } RCC_TypeDef;
typedef struct { volatile uint32_t DEMCR; } CoreDebug_Type;
typedef struct { volatile uint32_t CTRL, CYCCNT; } DWT_Type;
typedef struct { ADC_TypeDef *Instance; } ADC_HandleTypeDef;
typedef struct { HRTIM_TypeDef *Instance; } HRTIM_HandleTypeDef;
typedef struct {
    uint32_t InjectedNbrOfConversion, InjectedSamplingTime, InjectedSingleDiff;
    uint32_t InjectedOffsetNumber, ExternalTrigInjecConv, ExternalTrigInjecConvEdge;
    uint32_t QueueInjectedContext, AutoInjectedConv, InjectedChannel, InjectedRank;
} ADC_InjectionConfTypeDef;
typedef struct { uint32_t UpdateSource, Trigger; } HRTIM_ADCTriggerCfgTypeDef;
typedef struct { uint32_t CompareValue; } HRTIM_CompareCfgTypeDef;

extern ADC_TypeDef test_adc1;
extern HRTIM_TypeDef test_hrtim1;
extern RCC_TypeDef test_rcc;
extern CoreDebug_Type test_core_debug;
extern DWT_Type test_dwt;
extern uint32_t test_primask;
#define ADC1 (&test_adc1)
#define HRTIM1 (&test_hrtim1)
#define RCC (&test_rcc)
#define CoreDebug (&test_core_debug)
#define DWT (&test_dwt)

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
#define HRTIM_ADCTRIGGERUPDATE_TIMER_A 1U
#define HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP3 1U
#define HRTIM_ADCTRIGGER_1 1U
#define HRTIM1_TIMB_IRQn 1
#define ADC_ISR_JEOS (1U << 0)
#define ADC_ISR_JEOC (1U << 1)
#define ADC_ISR_JQOVF (1U << 2)
#define ADC_SAMPLETIME_24CYCLES_5 245U
#define ADC_SINGLE_ENDED 0U
#define ADC_OFFSET_NONE 0U
#define ADC_EXTERNALTRIGINJEC_HRTIM_TRG1 1U
#define ADC_EXTERNALTRIGINJECCONV_EDGE_RISING 1U
#define ADC_CHANNEL_1 1U
#define ADC_CHANNEL_2 2U
#define ADC_CHANNEL_3 3U
#define ADC_INJECTED_RANK_1 1U
#define ADC_INJECTED_RANK_2 2U
#define ADC_INJECTED_RANK_3 3U
#define DISABLE 0U
#define CoreDebug_DEMCR_TRCENA_Msk (1U << 0)
#define DWT_CTRL_CYCCNTENA_Msk (1U << 0)

static inline uint32_t __get_PRIMASK(void) { return test_primask; }
static inline void __disable_irq(void) { test_primask = 1U; }
static inline void __set_PRIMASK(uint32_t value) { test_primask = value; }
static inline void __DMB(void) { }
uint32_t HAL_RCC_GetHCLKFreq(void);
HAL_StatusTypeDef HAL_ADCEx_InjectedConfigChannel(ADC_HandleTypeDef *, ADC_InjectionConfTypeDef *);
HAL_StatusTypeDef HAL_ADCEx_InjectedStart(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_HRTIM_WaveformCompareConfig(HRTIM_HandleTypeDef *, uint32_t, uint32_t, HRTIM_CompareCfgTypeDef *);
HAL_StatusTypeDef HAL_HRTIM_ADCTriggerConfig(HRTIM_HandleTypeDef *, uint32_t, HRTIM_ADCTriggerCfgTypeDef *);
HAL_StatusTypeDef HAL_HRTIM_WaveformCounterStart(HRTIM_HandleTypeDef *, uint32_t);
void HAL_NVIC_SetPriority(int, uint32_t, uint32_t);
void HAL_NVIC_ClearPendingIRQ(int);
void HAL_NVIC_EnableIRQ(int);
#endif
