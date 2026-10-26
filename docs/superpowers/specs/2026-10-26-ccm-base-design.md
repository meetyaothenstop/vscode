# Uninstrumented CCM validation

New CCM profile evidence: valid_mask63; cumulative101,184,289,396,890,1012 cycles;
section deltas101,83,105,107,494,122. entry_counter0xE60. Current screenshot does
not show entry_state, so do not claim a strict same-phase benchmark against the
earlier ARMING Flash record. Controller region is smaller, but T3 still fires.

Remove optional unified section probes in a distinct CCM build before changing
control math. Use POWER_UNIFIED_DIAGNOSTICS=0, POWER_CCM_CONTROL=1, restricted
bench and OLED-mute. Keep canonical PowerStatus first-fault ADC/timing snapshots,
all protection checks/deadlines, clocks, ADC route, algorithm and output sequence.
No DMA, new auto-zero, threshold relaxation or hardware operation.

Share the existing verified CCM scatter layout and standard startup copy. Verify
the actual linked image is in CCM and has no power_diag/private probe symbols.
Give the build a distinct12V CCM BASE / SW-BASE identity and matching prefilled
Watch1. Capture fault_elapsed_cycles (exact timing) and v_reference if T3; on a
normal K4 stop use isr_cycles_max. In restricted cold Buck, reference starts0.1V
and advances120V/s; T3 occurs after it is assigned, so it helps identify first vs
later control step without extra probes. No timing improvement is claimed before
the board test. Preserve the profiled CCM HEX as reference.
