#include "adc_acquire.h"

#include <assert.h>

static const uint16_t samples[3] = {675, 1400, 3081};
static unsigned index;
static int fail_on_poll;

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    index = 0;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t timeout)
{
    (void)hadc;
    (void)timeout;
    return (fail_on_poll && index == 1) ? HAL_TIMEOUT : HAL_OK;
}

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return samples[index++];
}

HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return HAL_OK;
}

int main(void)
{
    ADC_HandleTypeDef adc = {0};
    AdcMeasurements out = {0};

    assert(ADC_ReadMeasurements(&adc, 2, &out) == HAL_OK);
    assert(out.raw_vh == 675);
    assert(out.raw_vl == 1400);
    assert(out.raw_il == 3081);
    assert(out.v_high > 11.81 && out.v_high < 11.85);

    fail_on_poll = 1;
    out.raw_vh = 123;
    assert(ADC_ReadMeasurements(&adc, 2, &out) == HAL_TIMEOUT);
    assert(out.raw_vh == 123);
    return 0;
}
