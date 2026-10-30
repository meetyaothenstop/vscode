# Main write-window candidate Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement inline.

**Goal:** Prepare a bounded forward-Buck-only phase-window candidate with tail protection.
**Architecture:** Cache per-mode phase deadlines at request acceptance; retain
existing timing protections and add an absolute postwrite T5 check in MAIN.
**Tech Stack:** STM32G474, bare-metal C, ARM6.18, GCC host integration fixture.
**Spec:** docs/superpowers/specs/2026-10-30-main-write-budget-design.md

## Global Constraints

Same bidreaction.uvprojx/branch, four modes, user downloads; no control/ADC/PWM
frequency or PI/enable order changes. Main mode1 T4=27904 and absolute T5=29952;
other modes T4=26880. DWT write840/exit936 and T1/T2/T6 remain. No board claims.

## Review Focus

- Main mode switching must publish the correct deadline before start_pending.
- HOLD/bootstrap/other modes retain existing behavior and phase policy.
- T3 wins when both prewrite checks fail; first-fault data must not be overwritten.
- New absolute T5 must not disable original elapsed or next-REP checks.
- Actual ARM tail code and data placement must match the reviewed source.

### Task 1: Candidate and verification

Files: tests/power_hal/stm32g4xx_hal.h, tests/power_control_test.c,
tests/run_tests.ps1, tests/main_menu_test.c, tests/main_ui_test.c,
Core/Src/power_control.c, Core/Src/power_menu.c, README.md and test/handoff docs.
Interfaces: retain PowerStatus and existing public API; test-only DWT accessor
adds deterministic prewrite/exit time and timer events, with no production hooks.

- [ ] Run46-group baseline; inspect actual OPT2 ARM tail and note timing limits.
- [ ] RED: add a dedicated MAIN timing-fixture variant exercising792/27232
  acceptance,27904/27905 and29952/29953 boundaries,841-cycle T3,
 937-cycle T5, REP T6, mode2/3/4 original T4 and immutable first fault.
- [ ] Implement cached main-mode phase limits and postwrite absolute T5; change
  non-diagnostic software identity to W128. Keep restricted bench policy.
- [ ] GREEN: run47 host groups including diagnostic/guard variants.
- [ ] ARM rebuild main and bench, actual image checks and fromelf inspection;
  record new hashes and explain that return-time margin remains unmeasured.
- [ ] Fresh independent code review; fix material findings with regression tests.
- [ ] Update README/test/handoff records and manifest; commit/tag/push and verify
  remote branch/tag. Hand off main HEX path/identity/low-power test observations.
