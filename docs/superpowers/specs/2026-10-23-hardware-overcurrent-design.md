# Bidirectional hardware overcurrent protection

User request: add the code needed for hardware-fast overcurrent shutdown. No board flashing, wiring changes or power-stage operation are authorized/performed by this software task.

## Approach

Use STM32G474CBT6 internal COMP1/COMP2 and DAC3, not an ADC interrupt as the fast shutdown path. INA240A1 OUT remains on PA2 for ADC and additionally fans out to PB1 (COMP1 plus IO2) and PA7 (COMP2 plus IO1). Both pins are unconfigured in the current project; actual board availability is pending the user's answer. G4 has no shared-input window mode, so both physical comparator connections are required. An external window comparator feeding a fault pin is a possible alternative if these pins cannot be used, but its board connection is not established.

COMP1 compares the current signal to an upper DAC3CH1 reference; COMP2 compares the same signal to a lower DAC3CH2 reference and inverts its output. RM0440 Table228 routes COMP1 to internal FAULT4 and COMP2 to internal FAULT1. Enable both faults on BOTH Timer A and B, with all four fault output levels inactive/low. Keep physical PWM and ADC mapping unchanged.

Initial configurable test limits: approximately ±7A, corresponding to1.8V/3.2V with current2.5V zero and0.1V/A gain. These are commissioning values within the current ADC range, not a verified hardware rating. DAC reference assumed3.3V, same as current ADC conversion. Set small10mV comparator hysteresis and no digital fault filter/blanking initially; validate noise and actual shutdown delay on the board before changing these.

## Readiness and fault behavior

- Default POWER_OCP_WIRING_CONFIRMED=0. Do not claim hardware protection active or configure unknown board pins in this default build. Power startup is refused with protection-not-ready fault512. OLED, keys and ADC remain usable.
- After physical wiring and injection verification, set this compile-time confirmation to1; configure DAC/COMP/Fault before control timers start. The readiness state is observable.
- Hardware fault shuts off outputs independently of CPU; the fault IRQ and timer IRQ also disable all outputs, cancel pending starts and latch software fault256.
- A short pulse which has already disappeared remains captured by the HRTIM sticky fault flag. No automatic restart after a fault clears.
- Explicit fault clear is permitted only when the hardware source is inactive. Clear fault flags with outputs disabled, recheck, remain stopped, never enable gates. Start also checks protection ready and fault source/flags.
- Preserve existing software current/voltage/ADC/timing protection, menu behavior, controller mathematics, current sign, and existing command numbers.

## Verification and limits

Host tests must cover both configured/unconfirmed readiness, comparator/DAC/Fault settings, both fault sources, short-pulse latch, active-fault clear refusal, start/stop/fault behavior in all active states, and no automatic restart. Keil full rebuild required. Host register tests cannot prove real analog thresholds, noise immunity or propagation delay. Board validation requires injected voltages with power stage isolated; never validate by deliberately shorting the power stage.
