#ifndef FORWARD_MAIN_BOOST_H
#define FORWARD_MAIN_BOOST_H
#include "forward.h"
#include "power_build_config.h"
#include "boost_control_rates.h"
#if !POWER_MAIN_APPLICATION
#error Protected forward Boost entry requires the MAIN application.
#endif
/* Backend-only contract: initialized mode2 and fresh finite ADC inputs with
 * positive left-input voltage, already qualified by Power_TimerIRQ protections.
 * Multirate cadence:25kHz current and5kHz voltage; caller requests every6PWM.
 * The public forward_step() retains its general50/10kHz compatibility behavior
 * (150kHz when POWER_MULTIRATE_CONTROL=0). Reinitialize before entry changes. */
void forward_main_boost_step(void);
#endif
