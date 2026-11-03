# Reverse Buck staged HOLD resume — 2026-11-03

Continue the authorized reverse12->6V timing investigation on the existing
MAIN project. W128 fault64/T5 recorded907/1000/31072; target6V, reference and
fault output4.38342094V. User then read the current MAP-verified addresses:
bootstrap_resume=1 and bootstrap_cycles=30. This identifies the late HOLD
resume branch: prepare/reset/controller/map followed by low-side enabling
and its two hardware-fault polls in the same IRQ. Phase31072 exceeds29952
by1120ticks=35CPU clocks. Why HOLD occurred/output fell is not established.

Approaches: further deadline relaxation loses already-small return margin;
controller arithmetic changes would disturb regulation. Split mode3 MAIN
resume preparation from enabling by one IRQ. This is a local implementation
choice within the existing authorized timing work, preserving policy.

Cache the resume compares while all gates remain off, mark private pending
and POWER_BOOTSTRAP, then on the next fresh protected sample recheck output
<.99*target and |IL|<.25A before begin_bootstrap(1). If invalid, return HOLD
without enabling. Preserve30 complete low-side intervals and2 arming periods.
The early enable/cancel path joins the common T5/T6 exit, without controller
work. STOP/zero/clear/fault/new request cancel pending. No new public state.

Keep840/27904 prewrite,936/29952 exit, T1/T2/T6, PI, sampling/PWM, all voltage
and current protections, forward/Boost/bench behavior and existing projects.
One extra off period is6.667us; cached plan is one sample older, already held
during bootstrap. No board operation; real timing/regulation remains to test.

Plan (single dependent change, executed inline):
- [x] Add real backend tests: all-off prepare; fresh recheck/cancel;30+2
  periods; retained plan; STOP/zero/clear/ADC/current/input/output/HW faults;
  T5/T6 on both prepare and early enable, including old907/31072 rejection.
- [x] Observe RED, implement private pending in power_control.c, observe GREEN.
- [x] Model the extra MAIN mode3 off interval, update identity to RES1,
  run complete suite, rebuild MAIN/bench, verify actual images/CCM/ARM.
- [x] Review, record fingerprints/limits and update README/test handoff.
- Delivery: commit/tag/push, verify remote SHA and report the outcome.
  Keep unrelated dirty files untouched.
