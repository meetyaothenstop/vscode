#ifndef POWER_PROTECTION_H
#define POWER_PROTECTION_H
#include "stm32g4xx_hal.h"
#include "adc_measure.h"

/* Set to1 only AFTER confirming INA240 OUT also reaches PB1 and PA7, then
 * validate both analog trips with the power stage isolated. The default
 * image deliberately refuses power startup; PA2 ADC remains unchanged. */
#ifndef POWER_OCP_WIRING_CONFIRMED
#define POWER_OCP_WIRING_CONFIRMED 0U
#endif
#define POWER_OCP_LIMIT_MA        7000U
#define POWER_OCP_ZERO_MV          ADC_CURRENT_ZERO_MV
#define POWER_OCP_MV_PER_AMP        ADC_CURRENT_MV_PER_AMP
#define POWER_OCP_DAC_REFERENCE_MV  ADC_REFERENCE_MV

HAL_StatusTypeDef PowerProtection_Init(HRTIM_HandleTypeDef *hrtim);
int PowerProtection_IsReady(void);
/* Includes persistent HRTIM event flags AND current analog fault levels. */
uint32_t PowerProtection_FaultPending(void);
/* Mask only fault IRQ requests; hardware shutdown remains enabled. */
void PowerProtection_MaskIRQ(void);
/* Foreground, IRQs masked, all outputs off. Never rearms PWM outputs. */
int PowerProtection_Acknowledge(void);
#endif
