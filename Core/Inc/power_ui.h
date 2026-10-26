#ifndef POWER_UI_H
#define POWER_UI_H
#include "adc_measure.h"
/* Foreground only. Init does not start the power stage or transfer OLED data. */
void PowerUI_Init(void);
void PowerUI_Service(const AdcMeasurements *measurements);
#endif
