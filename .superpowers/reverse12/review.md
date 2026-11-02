# Reverse12V Buck self-review

Reviewed changes against b99b2b7. MAIN mode3 owns the new Buck path, mode4
retains full Boost. One template preserves original active PI/ramp/filter
expressions;300000 bitwise cases pass against both the non-MAIN full build
and an independent object compiled from b99b2b7 source. HOLD/resume and stopped
reinitialization are included. No algorithm or gain retuning is claimed.

Shared Buck protection preserves forward numeric policy and mirrors bus/sign
selection correctly for reverse. Boost and bench policies remain separate.
Reverse UI12/6 and mailbox5 are tested. Fresh ADC1 remains the reverse input/
output source; no incorrect reuse of PA0 ADC2 as right input. New mapper is
the original generic implementation factored into inline expressions;200k+
edge/random comparisons per direction pass.

Reverse T4 remains26880, DWT840; new absolute exit29952 is tighter than the
former32000 bound, preserving DWT936/T6. Synthetic phase tests verify exact
edges and no T3/T4 writes, postwrite T5/T6 shutdown and first-fault latching.
Actual ARM mode3 branch at0x10000b96 bypasses generic mapper at0x10000c54;
both new controller bodies and dispatch/HOLD/resume reside in CCM. Buck has
788 bytes and one VDIV, full has1572 bytes and two VDIV; these are not WCET.

48 full host groups passed, main/bench ARM0 errors/0 warnings; actual image
CCM/copy/vector/no-probes/DMA/export checks and identities pass. Main HEX/AXF
match manifest. Bench HEX is identical to base. Historical documentation is
marked and current README/handoff points to the new candidate. No blocking
issue found in this self-review; no independent reviewer was used.

Declined to judge: actual board stability, high/load range, calibration, true
IRQ worst-case timing and hardware OCP readiness; no hardware evidence.
Preexisting MDK project/IDE files are excluded. No board operations performed.
