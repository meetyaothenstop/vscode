# Reverse first-control timing OPT1 — 2026-11-02

After87fe1b5 user screenshot:64 T3,855cycles/1000spacing/29248phase,
reference.1V/max0, output2.47224951/input11.9357195/current+.108888626A.
Both840 and26880 prewrite limits exceeded. Optimize computation and retain
all guard values; no measured cycle reduction may be claimed from code size.

MAIN mode3 fresh ADC values already pass input/output/current/startup checks.
Let that protected IRQ call Buck and Buck-HOLD entries directly through an
internal header. Keep public API finite/positive-input validation, mode0,
Boost behavior and active PI/ramp/filter math. Fast entry still checks mode3
and latched controller error. Add a null-pulse compare shortcut using the
generic mapper's exact expression, preserving every output and rejection.

Verify600000 bitwise samples, separate original-source oracle, public invalid
inputs, both mappers, actual855/29248 event remains T3/no-write and original
timing edges.48 groups, both ARM builds, actual image/CCM/DMA/export/identity,
self-review, records, commit/tag/push and remote SHA check. User alone flashes.
