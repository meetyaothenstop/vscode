# 12 V unified first-fault diagnostics

User-approved scope: build a diagnostic firmware before attempting performance or
sampling fixes. Keep 12 V/.2 A/100-ohm/6 V bench operation, all existing protection
thresholds, clocks, ADC triggers, controller parameters and output-enable sequencing.
No flashing, automatic start, DMA conversion or unverified root-cause fix is included.

Expose one debugger object, `power_diag`, only in an opt-in restricted bench build.
Its first-fault record pairs the existing ADC snapshot with pre-fault state, IRQ
entry state, request/IRQ/fault-IRQ context, pending flag and bootstrap/arming counts.
An explicit freshness flag distinguishes a sample read in this IRQ from the previous
complete sample used by an early timing/ADC/hardware fault.

Record cumulative CYCCNT offsets at entry-check, ADC, protection, state-machine,
controller, PWM-map and register-write boundaries. A validity mask identifies
completed boundaries; unvisited values in a fault record are zero. Unsigned deltas
handle counter wrap. Capture the measured deadline value separately from the time
at diagnostic capture, since capture follows emergency output disable.

The probes add real execution overhead. Keep that overhead inside the unchanged
deadline; do not subtract an invented estimate or claim a measured performance fix.
Use this build to identify expensive regions. Any later optimization must also be
validated on an uninstrumented build and on the board.

Latch all first-fault fields and timing metadata in the same critical section,
after emergency output disable. Later faults, live ADC updates and rejected start
requests cannot overwrite the record. Successful explicit clear removes it without
starting outputs; failed hardware acknowledgement preserves it.

Approaches considered: DMA-first does not locate the already observed T3; blanket
threshold increases hide evidence. Selected approach is one instrumented OLED-mute
build, preserving the existing builds as uninstrumented comparisons.

Deliver one distinct Keil project/target/HEX and OLED identity, prefilled Watch1,
Chinese field/measurement guide, host regression evidence and Keil build evidence.
