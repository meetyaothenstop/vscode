#ifndef FORWARD_MAIN_BOOST_H
#define FORWARD_MAIN_BOOST_H
#include "forward.h"
#include "power_build_config.h"
#if !POWER_MAIN_APPLICATION
#error Protected forward Boost entry requires the MAIN application.
#endif
/* Backend-only contract: initialized mode2 and fresh finite ADC inputs with
 * positive left-input voltage, already qualified by Power_TimerIRQ protections.
 * The public forward_step() retains its general compatibility behavior. */
void forward_main_boost_step(void);
#endif
