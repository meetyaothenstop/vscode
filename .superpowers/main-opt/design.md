# Main timing optimization candidate — 2026-10-30

Base8dc59c7. Same active bidreaction project/checkout; preserve user IDE edits.
User supplied first-control diagnostic screenshot; reading the expanded Watch
with computer-use confirmed fault64/detail3/mode1/target12, ARMING entry and
RUNNING captured state. No run/halt/reset/download commands were sent.

Native baseline:858cycles, spacing1000, phase29344, reference0.10000001.
Diagnostic:952cycles, spacing1000, entry counter4800, write counter352,
capture1147, mask63. Cumulative checkpoints145/249/416/533/808/938/invalid.
Deltas: entry145, ADC104, checks167, state117, controller275, map130.
Probe/layout overhead means these are not native section timings. The low
counter352 must not be interpreted as safe phase: counters wrap each period.

Goal: reduce repeated per-period work without changing equations, thresholds,
sample phases, gate enable sequencing, public commands, or four-mode behavior.
Cache main per-mode fixed protection limits and target×OVP once at accepted
request publication; retain all per-sample comparisons, .95 reachability and
first-fault behavior. Add a Buck-specialized entry to the existing forward PWM
mapper, generated from the same implementation with boost_run=0; generic and
reverse mapping remain. Use it only for main mode1; other builds remain intact.
No PI edits or timing-limit increase. Restore non-probed main build (explicit
POWER_UNIFIED_DIAGNOSTICS=0), identity MAIN ADC OPT 6A / M-OPT.

Plan: RED missing specialized API and new identity; GREEN differential mapper
boundary/random-float equivalence and existing mode/target/fault regressions;
ARM main+bench rebuild and actual CCM/HEX checks; independent review; docs,
commit/tag/push with remote SHA verification. Cache refactor relies on existing
cross-mode adjacent-ADC-boundary regressions, not a host timing assertion.
Native board timing and T3 elimination must be measured after user download.
