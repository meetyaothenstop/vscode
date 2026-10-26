#ifndef POWER_CONTROL_H
#define POWER_CONTROL_H
#include "stm32g4xx_hal.h"
#include "adc_measure.h"
#include "pwm_plan.h"

typedef enum {
    POWER_STOPPED, POWER_BOOTSTRAP, POWER_ARMING, POWER_RUNNING, POWER_FAULT,
    POWER_CHANGEOVER, POWER_HOLD
} PowerState;
enum {
    POWER_FAULT_ADC = 1U, POWER_FAULT_INPUT = 2U,
    POWER_FAULT_OUTPUT = 4U, POWER_FAULT_CURRENT = 8U,
    POWER_FAULT_REVERSE = 16U, POWER_FAULT_CONTROLLER = 32U,
    POWER_FAULT_TIMING = 64U, POWER_FAULT_START = 128U
};
enum {
    POWER_CMD_NONE, POWER_CMD_START_BOOST, POWER_CMD_STOP,
    POWER_CMD_CLEAR_FAULT, POWER_CMD_START_BUCK,
    POWER_CMD_START_REVERSE_BUCK, POWER_CMD_START_REVERSE_BOOST
};
typedef struct {
    PowerState state;
    uint32_t fault;
    uint32_t samples;
    uint32_t isr_cycles_max;
    uint16_t raw_vh, raw_vl, raw_il;
    float v_left, v_right, il;
    float d_cmd, v_reference;
    uint32_t left_compare, right_compare;
} PowerStatus;

/* Debugger mailbox. Main loop consumes the command once. */
extern volatile uint32_t power_command;
extern volatile float power_target_volts; /* default 36 V */
extern volatile PowerStatus power_status;

/* Call after ADC calibration and MX_HRTIM1_Init. Outputs remain disabled. */
HAL_StatusTypeDef Power_Init(ADC_HandleTypeDef *adc, HRTIM_HandleTypeDef *hrtim);
/* Foreground-only: 1/2 forward buck/boost, 3/4 reverse buck/boost.
 * Reverse source is the right bus; physical IL stays signed left-to-right. */
int Power_RequestStart(unsigned mode, float target_volts);
void Power_Stop(void);
void Power_ClearFault(void);
int Power_IsStartPending(void);
int Power_SamplesAreValid(void);
void Power_Service(AdcMeasurements *measurements);
void Power_TimerIRQ(void);
/* May be called from fatal handlers. Does not require HAL handle state. */
void Power_EmergencyOff(void);
#endif
