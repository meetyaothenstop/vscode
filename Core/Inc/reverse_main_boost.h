#ifndef REVERSE_MAIN_BOOST_H
#define REVERSE_MAIN_BOOST_H
#include "reverse.h"
#include "power_build_config.h"
#include "boost_control_rates.h"
#if !POWER_MAIN_APPLICATION
#error Protected reverse Boost entry requires the MAIN application.
#endif
/* Backend-only: initialized mode4, fresh finite ADC1 inputs with positive
 * right-input voltage qualified by Power_TimerIRQ. Physical IL is negative.
 * Multirate cadence25kHz current/5kHz voltage, one request every6PWM periods.
 * Public reverse_step/hold retain50/10kHz (150kHz in single-rate builds).
 * Reinitialize while stopped before switching mode or entry profile. */
void reverse_main_boost_step(void);
/* Compatibility normal-step alias; MAIN production does not call HOLD. */
void reverse_main_boost_hold_step(void);
#endif
