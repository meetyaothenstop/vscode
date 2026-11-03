# Fast IRQ raw ADC optimization

User approved the four-point proposal. Execute inline with TDD and one fresh final review.
Baseline040164c and backup tag pre-fastirq-raw-20261103 remotely verified before edits.

Keep PWM/ADC/software safety150kHz, current50kHz, voltage10kHz and T1..T7 unchanged.
Preserve main project/four modes/sole bench and exact calibrated protection decisions.

1. Add independently tested raw ADC windows, built in stopped foreground by
   evaluating the existing float predicates for every12-bit ADC code. Exhaustive
   equivalence tests cover both bus gains, modes, limits, inclusive/strict edges.
2. MAIN multirate fast IRQ reads/stores raw samples and checks integer windows;
   move conversions to worker/STOP telemetry. Request acceptance snapshots fresh
   raw data; first-fault latch always converts its own raw snapshot. Preserve
   ADC1/late ADC2 mode selection, current direction16 samples, HOLD and30+2 timing.
3. Worker precomputes integer pulse-density increment; fast path uses integer
   accumulate/carry and paired compares. Preserve source minimum/null pulse policy.
4. Separate healthy fast-IRQ peak from fault-inclusive peak, worker start delay,
   and worker body wall time (includes preemption; never claim pure PI cycles).
   Add normal fast C-cycle accounting if needed; exception/other IRQ overhead must
   be explicitly excluded from any preemption estimate.
5. Run all existing and new regressions, main/bench ARM builds, actual-image
   checks and disassembly for FP instructions in fast healthy functions. Review
   cancellation, raw boundary equivalence, fresh fault snapshots and transitions.
   Record limits, commit/tag/push and verify remote. No board operation.

Do not claim77% sustained utilization from the prior769-cycle historical peak:
it may include the T7 fault-latch path. Board screenshot: elapsed3435,
spacing1000, phase15968, worker count171, voltage35, completed response max2903.
