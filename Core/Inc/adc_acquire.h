#ifndef ADC_ACQUIRE_H
#define ADC_ACQUIRE_H

#include "adc_measure.h"
#include "stm32g4xx_hal.h"

/* ADC1 regular sequence: PA0/ADC_VH, PA1/ADC_VL, PA2/ADC_L. */
HAL_StatusTypeDef ADC_ReadMeasurements(ADC_HandleTypeDef *hadc,
                                       uint32_t timeout_ms,
                                       AdcMeasurements *out);

#endif
