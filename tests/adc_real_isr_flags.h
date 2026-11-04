#include "power_hal/stm32g4xx_hal.h"
/* This experiment uses the real STM32G474 ISR masks rather than the isolated
 * fixture's synthetic bit assignments. No firmware source is modified. */
#undef ADC_ISR_JEOS
#undef ADC_ISR_JEOC
#undef ADC_ISR_JQOVF
#undef ADC_ISR_EOC
#undef ADC_ISR_EOS
#undef ADC_ISR_OVR
#define ADC_ISR_ADRDY 0x1U
#define ADC_ISR_JEOS 0x40U
#define ADC_ISR_JEOC 0x20U
#define ADC_ISR_JQOVF 0x400U
#define ADC_ISR_EOC 0x4U
#define ADC_ISR_EOS 0x8U
#define ADC_ISR_OVR 0x10U
