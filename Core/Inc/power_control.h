#ifndef POWER_CONTROL_H
#define POWER_CONTROL_H
#include "stm32g4xx_hal.h"
#include "adc_measure.h"
#include "pwm_plan.h"
#include "power_build_config.h"

typedef enum {
    POWER_STOPPED, POWER_BOOTSTRAP, POWER_ARMING, POWER_RUNNING, POWER_FAULT,
    POWER_CHANGEOVER, POWER_HOLD
} PowerState;
enum {
    POWER_FAULT_ADC = 1U, POWER_FAULT_INPUT = 2U,
    POWER_FAULT_OUTPUT = 4U, POWER_FAULT_CURRENT = 8U,
    POWER_FAULT_REVERSE = 16U, POWER_FAULT_CONTROLLER = 32U,
    POWER_FAULT_TIMING = 64U, POWER_FAULT_START = 128U,
    POWER_FAULT_HARDWARE = 256U, POWER_FAULT_PROTECTION = 512U
};
enum {
    POWER_CMD_NONE, POWER_CMD_START_BOOST, POWER_CMD_STOP,
    POWER_CMD_CLEAR_FAULT, POWER_CMD_START_BUCK,
    POWER_CMD_START_REVERSE_BUCK, POWER_CMD_START_REVERSE_BOOST
};
typedef struct {
    PowerState state;
    uint32_t fault;
    uint32_t protection_ready;
    uint32_t samples;
    uint32_t isr_cycles_max;
    uint16_t raw_vh, raw_vl, raw_il;
    float v_left, v_right, il;
    float d_cmd, v_reference;
    uint32_t left_compare, right_compare;
    /* First-fault diagnostics. S1..S9 are start rejections; T1..T6 timing.
     * Timing values are CPU cycles / raw HRTIM counter ticks, latched on fault. */
    uint32_t fault_detail;
    uint32_t fault_elapsed_cycles, fault_spacing_cycles, fault_counter_ticks;
    float fault_v_left, fault_v_right, fault_il, fault_input_volts;
    uint16_t fault_raw_vh, fault_raw_vl, fault_raw_il;
} PowerStatus;

#if POWER_UNIFIED_DIAGNOSTICS
enum {
    POWER_DIAG_CONTEXT_INIT = 1U, POWER_DIAG_CONTEXT_START_REQUEST,
    POWER_DIAG_CONTEXT_TIMER_IRQ, POWER_DIAG_CONTEXT_HARDWARE_IRQ,
    POWER_DIAG_CONTEXT_OTHER
};
enum {
    POWER_DIAG_STAGE_NONE, POWER_DIAG_STAGE_ENTRY, POWER_DIAG_STAGE_ADC,
    POWER_DIAG_STAGE_CHECKS, POWER_DIAG_STAGE_STATE, POWER_DIAG_STAGE_CONTROLLER,
    POWER_DIAG_STAGE_MAP, POWER_DIAG_STAGE_PREWRITE, POWER_DIAG_STAGE_WRITE,
    POWER_DIAG_STAGE_EXIT
};
enum {
    POWER_DIAG_CP_ENTRY, POWER_DIAG_CP_ADC, POWER_DIAG_CP_CHECKS,
    POWER_DIAG_CP_STATE, POWER_DIAG_CP_CONTROLLER, POWER_DIAG_CP_MAP,
    POWER_DIAG_CP_WRITE, POWER_DIAG_CHECKPOINT_COUNT
};
typedef struct {
    uint32_t stage;
    PowerState entry_state;
    uint32_t entry_samples, entry_spacing_cycles, entry_counter_ticks;
    /* Completed boundaries only; offsets include instrumentation overhead. */
    uint32_t valid_mask, checkpoint_cycles[POWER_DIAG_CHECKPOINT_COUNT];
} PowerDiagProfile;
typedef struct {
    uint32_t fault, detail;
    PowerState state; /* Before trip replaced state with POWER_FAULT. */
    uint32_t context, start_pending, bootstrap_remaining, arming_remaining;
    uint32_t bootstrap_resume, mode;
    float target_volts;
    uint32_t samples, sample_fresh;
    uint16_t raw_vh, raw_vl, raw_il;
    float v_left, v_right, il;
    /* Capture follows output disable. Only timing_* is the exact deadline
     * measurement; those fields are zero for non-timing first faults. */
    uint32_t capture_elapsed_cycles;
    uint32_t timing_elapsed_cycles, timing_spacing_cycles, timing_counter_ticks;
    PowerDiagProfile profile;
} PowerDiagFault;
typedef struct {
    uint32_t version, cpu_hz, pwm_hz, write_budget_cycles, exit_budget_cycles;
    PowerDiagProfile irq; /* Live, may change after fault; use fault.profile. */
    PowerDiagFault fault; /* Immutable until successful explicit clear/reset. */
} PowerDiagnostics;
extern volatile PowerDiagnostics power_diag;
#endif

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
void Power_FaultIRQ(void);
/* May be called from fatal handlers. Does not require HAL handle state. */
void Power_EmergencyOff(void);
#endif
