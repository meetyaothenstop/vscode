# OPT1 self-review

Reviewed candidate against87fe1b5 and the reported855/29248 T3 point.
All numerical guard and electrical-protection settings remain unchanged.
The qualified IRQ path has fresh uint16-derived finite values, positive
protected right input and mode3 assigned, and exits on ADC/input/output/
current/start faults before direct controller calls. Internal entry contract
is isolated in reverse_main_buck.h; it is not an unvalidated public API.

Public reverse_step still rejects NaN/infinity/nonpositive source and clears
outputs as before, with mode0/Boost behavior retained. Dedicated HOLD entry
freezes voltage integration and clears current history exactly as the public
path. Active math remains bitwise equal over600000 cases against the full
path and a separately compiled b99b2b7 original source oracle. Mapper shortcut
uses identical float multiplication/threshold and constant rounding;200k+
generic/specialized comparisons, edge cases and invalid-output checks pass.

48 full host groups passed, including the actual first-control855/29248
event still rejecting as T3/no-write/max0. Main/bench ARM0errors/0warnings;
actual HEX CCM/copy/vector/no-probes/DMA/export checks pass. Native code
confirms direct calls at0x10000aa2 and0x10000b8c, no public validation on that
protected path, and null compare branch at0x10000c0c..c12 skipping conversion.
New HOLD entry stays in CCM; controller body788->656bytes. Banner/parameter
identity and artifact fingerprints match records; bench HEX is unchanged.
No blocking issue found in this self-review; no independent reviewer was used.

Not judged as verified: actual saved cycles, first-step T3/T4 resolution,
hardware worst-case timing, electrical behavior and OCP readiness. Those
require the user's next board run. No board actions performed. Preexisting
MDK project/IDE state edits are outside this change.
