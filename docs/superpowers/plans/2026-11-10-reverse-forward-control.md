# Reverse Forward Control Implementation Plan

> **For agentic workers:** Use executing-plans inline, followed by one independent read-only final review.

**Goal:** Replace both MAIN reverse controllers with the exact forward controller algorithm and make backups distinguishable.
**Architecture:** Two independent instances of shared initialization/step templates; MAIN reverse is a physical I/O adapter, with continuous regulation and mirrored PWM. NON-MAIN legacy references remain explicit.
**Tech Stack:** STM32G474 C, GCC actual-backend fixtures, Keil ARMCLANG6.18.
**Spec:** ../specs/2026-11-10-reverse-forward-control-design.md

## Global Constraints

- Baseline tag pre-reverse-forward-control-20261110 already remote→d000bfb; record in docs/版本与备份对照.md.
- Main and sole bench unchanged; no board actions; preserve existing IDE/scratch changes.
- Preserve voltage defaults, electrical/ADC/sign/timing protections and all rates; mode4UV7 remains a separate test policy.
- Forward behavior and bench image must remain; reverse algorithm/HOLD change intentionally needs new behavioral tests.

## Review Focus

- State or public namespace shared by directions: interleave steps/configuration and compare independent reference state.
- Physical current sign or swapped PWM wrong: mirrored fixtures and exact compare-pair swapping over legal duties.
- Hidden old reverse HOLD/resume still active: real backend target-crossing/spike cases with OVP and STOP retained.
- Compatibility entry silently at25kHz vs public50kHz: separate same-rate pinned original-forward references.
- New core executes outside CCM or adds fast IRQ FP: linked symbols/call paths/actual HEX/image checks.

### Task1: Shared controller and MAIN adapter

- [x] Add failing reverse-vs-pinned-forward tests; observe baseline divergence before implementation.
- [x] Extract reusable forward controller template without math/order changes; instantiate independent MAIN reverse I/O/state, retaining old public physical interface and explicit NON-MAIN implementation.
- [x] Remove MAIN reverse HOLD/resume lifecycle and use continuous regulation; update MAIN expectations and retain legacy references.
- [x] Prove B/X/output equality at all relevant rates, state isolation, signed-zero/error handling, STOP/reinit and exact PWM mirroring.

### Task2: Integrated verification and handoff

- [x] Complete meaningful backend fault/transition/no-HOLD and four-mode plant tests; full89-group suite green.
- [x] Existing main/bench ARM rebuild0/0, exact bench HEX; inspect actual image/CCM/AXF/fast IRQ/worker and UI FCTRL1.
- [x] Read-only independent review, noCritical/Important; strengthen comparison from first sample and rerun both720000-step profiles.
- [x] Update README, test record, persistent AGENTS rule and version table.

Finalization: commit/tag main-reverse-forward-control-20261110, push and verify branch/tag SHA before reporting upload complete.
Ruling: requested exact forward behavior inherits its24/35→36V startup OCP envelope. Replace the old MAIN-only unprotected reverse plant assumption with actual protected production-backend assertions for both directions; keep7.5A threshold and report the limitation. Legacy NON-MAIN models remain unchanged.
