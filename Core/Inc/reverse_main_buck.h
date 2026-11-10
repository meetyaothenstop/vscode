#ifndef REVERSE_MAIN_BUCK_H
#define REVERSE_MAIN_BUCK_H
#include "reverse.h"
#include "power_build_config.h"
#if !POWER_MAIN_APPLICATION
#error Protected reverse Buck entry points require the MAIN application.
#endif
/* Backend-only contract: initialized mode3, fresh finite ADC inputs and
 * positive right-input voltage qualified by Power_TimerIRQ protections. */
void reverse_main_buck_step(void);
/* Compatibility normal-step alias; MAIN production does not call HOLD. */
void reverse_main_buck_hold_step(void);
#endif
