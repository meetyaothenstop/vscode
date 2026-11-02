# Reverse Buck W128 window — 2026-11-02

AfterOPT1 the user's T4 snapshot reports792/1000/27232, reference.1/max0.
T3 now passes; phase exceeds26880 by352ticks=11CPU clocks, leaving149clocks
before32000. Adopt the existing MAIN forward Buck W128 policy for mode3.
Only request-time cached T4 changes to27904, retaining DWT840, exit936/29952,
T1/T2/T6, Boost/bench deadlines and all electrical/control behavior.
Identity MAIN ADC RBUCK W128 / M-RW128; same project and user-only download.

Normal paired write uses the identical13-instruction common sequence with
no call.128/64 reserves remain engineering settings, not measured hardware
WCET, especially for HOLD resume and exception return. Keep explicit T5/T6.
RED/GREEN captured first-step event,27904/27905, T3 priority, exit edges,
wrapped-low-counter pending REP, mode cache and first-fault retention.
48 groups, both ARM builds, actual image/CCM/DMA/export/identity, self-review,
docs/fingerprints, commit/tag/push and remote SHA check. No board operations.
