# MAIN 50 kHz current / 10 kHz voltage control

User authorized implementation on 2026-11-03 after the frequency discussion.
Before any source change, baseline32a9bc8 and annotated tag
`pre-control-multirate-20261103` were pushed by HTTPS and remotely verified.
The SSH origin is unchanged. Original unrelated IDE/workspace edits stay untouched.

## Design

Keep PWM, ADC triggers, fast protection and state timing at150kHz. Schedule a
preemptible PendSV control calculation every third PWM period (50kHz). Run the
voltage PI and its feedback filter every fifth control step (10kHz), holding
the current reference between voltage steps. Current PI and ramps use20us;
voltage PI/filter use100us. Preserve continuous gains, protection thresholds,
directions, bootstrap200us and paired preload updates. The Boost transition
delay remains20us (one new control step). No RTOS is introduced.

The fast IRQ snapshots one control request; the worker publishes one coherent
result with a generation identifier. Consume completed work at the next safe
PWM IRQ and latch both compares together at the following PWM update. Request
cadence remains50kHz; missing completion at the next request boundary trips
a new T7 deadline fault. Fast IRQ
T1..T6 and phase limits remain; they measure the short fast IRQ, not the worker.
Stop, fault and new start invalidate outstanding work. The worker never writes
PWM registers, enables outputs, or changes the public power state.

For subminimum Buck/precharge duties, distribute minimum/null source pulses
over individual PWM periods with a first-order pulse-density accumulator. This
prevents grouping minimum-energy packets for three cycles at low output. PI
still executes50/10kHz; it does not run in the modulator. Both forward/reverse
Boost stay all-off for at least three complete PWM periods (20us) and until the Boost plan has latched, regardless of worker completion latency. MAIN reserves2KiB stack for nested UI/PendSV/fast/DMA exceptions; bench keeps1KiB.

Reverse HOLD/recovery and Boost changeover are commanded only by the fast IRQ
using published controller flags plus fresh protection/recovery samples. Retain
the RES1 all-off preparation and full30+2 PWM-period bootstrap/arming intervals.

Only MAIN defaults to the new architecture. The sole restricted bench and
legacy controller configurations stay at150kHz. Keep existing main project,
four operating modes and explicitly authorized ADC-only protection policy.

## Approaches considered

Chosen: separate fast IRQ and preemptible worker, with split loop rates.
Simply skipping calls inside the existing IRQ reduces average load but does
not increase its worst-case time budget. Reducing PWM would change power-stage
ripple and is outside the requested control-frequency change.

## Verification and limits

Test real controllers and backend with synthetic registers: rates, new Ts and
filter response, all four modes, fast protection between control steps,
coherent publication, delayed/stale results, stop/fault cancellation, HOLD
recovery and Boost changeover. Run legacy regression, ARM main/bench builds,
CCM/vector/HEX checks and an independent code review. Host evidence cannot
establish board WCET, stability, electrical waveforms or hardware OCP readiness.
Deliver a clearly identified candidate for user download, commit and push it.
