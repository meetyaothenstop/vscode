# Multirate Control Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans inline; perform one independent final review.

**Goal:** MAIN PWM/protection150kHz, current50kHz, voltage10kHz.
**Architecture:** Fast IRQ owns safety/state/PWM; PendSV owns controller arithmetic; generation-checked request/result pipeline.
**Tech Stack:** STM32G474, C11, ARMClang6.18, GCC host fixtures, PowerShell.
**Spec:** docs/superpowers/specs/2026-11-03-multirate-design.md

## Global Constraints

Keep current main project and all four modes; sole bench remains unchanged.
Preserve protection thresholds and actual bootstrap/arming times; no board operation.
Baseline tag must be remotely verified before production edits (done).
Retain unrelated IDE edits; no binaries/UI state in commits; push verified result.

## Review Focus

- Fast fault preempts worker publication: stale result must never re-enable/write outputs.
- Worker misses50kHz deadline: T7 must trip instead of silently reusing stale duty.
- Reverse HOLD recovery fails fresh qualification: remain all-off.
- Boost transition changes outer-loop eligibility: refresh voltage PI promptly and retain20us delay.
- Sample time changes must preserve ramp speed, filter corner and PI gain units.

### Task1: Resample/split controller arithmetic

**Files:** Core/Inc/control_rates.h, Core/Src/forward.c, reverse.c, *_step_impl.inc; tests/multirate_controller_test.c.
**Interfaces:** normal public controller step now means20us in MAIN; expose step counters for cadence tests; legacy opt-out preserves bit equivalence.
- [x] Write/run failing real-controller cadence, integral/ramp/filter tests (RED).
- [x] Implement50kHz inner and10kHz outer with correct discrete coefficients and20us Boost delay.
- [x] Run new controller tests and legacy reference comparisons (GREEN).

### Task2: Fast protection and PendSV pipeline

**Files:** Core/Src/power_control.c, stm32g4xx_it.c, Core/Inc/power_control.h; MDK-ARM/bidreaction_ccm.sct; tests/power_control_test.c and fixture.
**Interfaces:** Power_ControlIRQ(); fast IRQ publishes request/consumes result every3 periods; worker publishes generation-stamped immutable result.
- [x] Add/run failing integration tests for cadence, protection, T7, cancellation, state transitions (RED).
- [x] Implement pipeline and PendSV priority/vector/CCM placement; preserve legacy backend via compile-time option.
- [x] Run new integration scenarios and full legacy suite (GREEN).

### Task3: Delivery verification and preservation

**Files:** tests/run_tests.ps1, UI identity/tests, README.md, docs test record.
**Interfaces:** existing main/bench binaries and source history.
- [x] Run full suite and both existing ARM rebuilds; verify linked CCM/vector and generated HEX.
- [x] Have independent reviewer inspect diff for all Review Focus conditions; address material findings with reproducing tests.
- [x] Record exact results/limits and candidate identity; commit, tag, push and verify remote SHAs.
