#include "adc_measure.h"

void ADC_ConvertRaw(uint16_t raw_vh, uint16_t raw_vl,
                    uint16_t raw_il, AdcMeasurements *out)
{
    out->raw_vh = raw_vh;
    out->raw_vl = raw_vl;
    out->raw_il = raw_il;
    out->v_high = (double)raw_vh * ADC_LEFT_VOLTS_PER_COUNT;
    out->v_low = (double)raw_vl * ADC_RIGHT_VOLTS_PER_COUNT;
    out->il = (double)raw_il * ADC_CURRENT_AMPS_PER_COUNT - ADC_CURRENT_ZERO_AMPS;
}
