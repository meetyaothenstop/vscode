#include "adc_acquire.h"

HAL_StatusTypeDef ADC_ReadMeasurements(ADC_HandleTypeDef *hadc,
                                       uint32_t timeout_ms,
                                       AdcMeasurements *out)
{
    uint16_t raw[3];
    HAL_StatusTypeDef status = HAL_ADC_Start(hadc);
    if (status != HAL_OK)
    {
        return status;
    }

    for (unsigned rank = 0; rank < 3; ++rank)
    {
        status = HAL_ADC_PollForConversion(hadc, timeout_ms);
        if (status != HAL_OK)
        {
            (void)HAL_ADC_Stop(hadc);
            return status;
        }
        raw[rank] = (uint16_t)HAL_ADC_GetValue(hadc);
    }

    status = HAL_ADC_Stop(hadc);
    if (status == HAL_OK)
    {
        ADC_ConvertRaw(raw[0], raw[1], raw[2], out);
    }
    return status;
}
