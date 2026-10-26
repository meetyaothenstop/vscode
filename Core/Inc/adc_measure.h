#ifndef ADC_MEASURE_H
#define ADC_MEASURE_H

#include <stdint.h>

/* Preliminary board calibration, 2026-10-24/25 (power stage unpopulated).
 * VDDA/VREF+=3.31 V; INA240 OUT/PA2=2.49 V at idle.
 * Left bus: 11.83 V measured / old display ~11.57 V.
 * Right bus: 11.9 V / old display ~11.8 V; 23.9 V / 23.70-23.77 V.
 * Right correction is provisional because the reported readings are coarse.
 * Recheck both right-bus points after download and verify a known shunt
 * current before powered closed-loop commissioning. */
#define ADC_REFERENCE_MV           3310U
#define ADC_CURRENT_ZERO_MV        2490U
#define ADC_CURRENT_MV_PER_AMP      100U /* INA240A1 x20, 5 mOhm; not gain-calibrated. */
#define ADC_LEFT_VOLTAGE_GAIN      0.0461f
#define ADC_RIGHT_VOLTAGE_GAIN     0.04666f
#define ADC_FULL_SCALE_COUNTS      4095.0f

/* Keep these single precision: the 150 kHz ISR uses the same calibration
 * as foreground telemetry without introducing double-precision arithmetic. */
#define ADC_LEFT_VOLTS_PER_COUNT \
    ((float)ADC_REFERENCE_MV / (1000.0f * ADC_FULL_SCALE_COUNTS * ADC_LEFT_VOLTAGE_GAIN))
#define ADC_RIGHT_VOLTS_PER_COUNT \
    ((float)ADC_REFERENCE_MV / (1000.0f * ADC_FULL_SCALE_COUNTS * ADC_RIGHT_VOLTAGE_GAIN))
#define ADC_CURRENT_AMPS_PER_COUNT \
    ((float)ADC_REFERENCE_MV / (ADC_FULL_SCALE_COUNTS * (float)ADC_CURRENT_MV_PER_AMP))
#define ADC_CURRENT_ZERO_AMPS \
    ((float)ADC_CURRENT_ZERO_MV / (float)ADC_CURRENT_MV_PER_AMP)

typedef struct
{
    uint16_t raw_vh;
    uint16_t raw_vl;
    uint16_t raw_il;
    double v_high;
    double v_low;
    double il;
} AdcMeasurements;

/* Convert ADC1 ranks PA0/ADC_VH, PA1/ADC_VL, PA2/ADC_L. */
void ADC_ConvertRaw(uint16_t raw_vh, uint16_t raw_vl,
                    uint16_t raw_il, AdcMeasurements *out);

#endif
