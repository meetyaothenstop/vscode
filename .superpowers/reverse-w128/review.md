# Reverse W128 self-review

Reviewed against3b77c22. Only MAIN mode3 cached write phase changes26880->27904,
plus identity/records/tests. Mode1 retains27904; modes2/4 and bench retain26880.
DWT840, exit936/29952 for Buck, T1/T2/T6 and electrical guards remain intact.
ARM uses(mode &~2)==1 to select0x6D00 and caches before pending publication.
Common paired write block is identical13 instruction encodings to OPT1;
the IRQ has no new window-selection work. No callback/PI/ADC change.

Captured792/27232 first-step fixture was RED on old policy and GREEN on the
candidate.48 groups pass, including boundary/no-write, T3 priority, T5,
wrapped-low-counter pending REP/T6, cache changes and immutable faults.
Both ARM builds report0errors/0warnings. Actual HEX CCM/copy/vector/no-probes/
DMA and AXF export pass; identities fit21 columns and match actual image.
Bench SHA is identical. No blocking issue found in this self-review; no
independent reviewer was used.

Not judged verified: board T4 resolution, all-load stability, true normal/
HOLD-resume tail and exception-return WCET or hardware OCP readiness.128/64
are engineering reserves and T5 cannot undo a missed boundary. Prior/new
snapshots are different captures and do not establish exact optimized WCET.
No board actions performed; preexisting MDK project/IDE changes excluded.
