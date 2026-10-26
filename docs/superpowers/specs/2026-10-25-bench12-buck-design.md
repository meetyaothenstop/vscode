# 12 V forward Buck bench build

User chose to begin at 12 V using existing ADC current protection without
adding PB1/PA7 comparator wiring. Supply is adjustable; user proposes 0.2 A
input current limit. This change prepares firmware only, without downloading
it or starting hardware. ADC current protection does not replace fast
hardware fault shutdown, and the input current limit does not bound current
from charged bus capacitors.

Approaches: retain the default hardware requirement (user declined for this
test), remove it globally (would also permit unintended higher-power modes),
or create an explicitly enabled, restricted test build. Use the last approach.

Implementation choices: POWER_BENCH_12V_BUCK_TEST defaults to 0; a separate
Keil project/output enables it. The default image still rejects missing
hardware readiness. The test image leaves protection_ready=0 and comparator
GPIOs untouched, permitting only mode 1 forward Buck, measured input
10.5-13.5 V and output target 1-6 V. Enforce the input window during startup
and operation. Current reference limit is 0.5 A; absolute sampled trip is
1.5 A. Retain ADC rail, sample loss, timing, polarity, output overvoltage,
cold-start, stop and fault-latch behavior. Never auto-start or auto-restart.

OLED test default is forward 12->6 V, with a visible SW build label. The
existing default UI and limits stay unchanged. First proposed load is 100
ohm (6 V gives 0.36 W); use a resistor rated at least 1 W. Begin at STOPPED
and verify the source maintains 12 V without constant-current indication.
Actual gate/deadtime, current sensing, sensor transient and ISR timing are
not established by host tests. No full-power or hardware-protection claim.

Verification-driven adjustment: bench-only forward Buck subminimum pulses
must use HRTIMv2 null duty, matching the existing reverse implementation;
the default forward mapper stays unchanged. Fresh source/current/overvoltage
checks precede any pending-start output enable. Cold output and current
conditions are rechecked throughout cold arming in both builds (excluding
charged-output HOLD resume).
