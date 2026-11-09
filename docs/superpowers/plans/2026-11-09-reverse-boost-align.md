# Reverse Boost Alignment Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan inline; perform one independent final review.

**Goal:** Align MAIN reverse Boost electrical checks and25/5kHz cadence with forward Boost.
**Architecture:** Reuse MAIN electrical limits and boost_control_rates.h; add template-generated mode4 step/hold entries while preserving public compatibility rates.
**Tech Stack:** STM32G474 C, GCC host fixtures, ARMCLANG6.18/Keil.
**Spec:** ../specs/2026-11-09-reverse-boost-align-design.md

## Global Constraints

- Existing main and sole bench projects; no download/board operations.
- Preserve18→3V forward and12→6V reverse defaults, IDE state and unrelated scratch files.
- PWM/ADC/fast safety150kHz; Buck50/10kHz; reverse sign/ADC and continuous PI unchanged.
- Baseline tag uploaded before edits; final source commit/tag pushed and remotely verified.

## Review Focus

- Wrong bus or ADC2 fallback in reverse: existing input-phase fixture plus mode4 limit cases.
- Cold checks only changed at request: inject limits independently in prepare/bootstrap/arming.
- Slow cadence skipping safety: inject faults in all five inter-request periods.
- HOLD/resume or changeover still using50kHz coefficients: independent old-source same-rate trajectory and real switched backend.
- Public/Buck rates accidentally changed: existing public equivalence tests and four-mode cadence assertions.

### Task1: Shared reverse Boost policy and rate profile

Files: power_control.c, power_multirate.inc, reverse.c/reverse_step_impl.inc,
reverse_main_boost.h, rate header comments, existing limits/rates/backend tests,
new reverse_boost_equivalence_test.c, run_tests.ps1 and image verification.

- [x] Update actual-backend tests to expect mode4 input9..42V, cold1A/3V, capped1.5xOVP and6-PWM cadence; run RED against baseline.
- [x] Include mode4 in MAIN shared policy; generate mode4 rate-selected step/hold; dispatch worker and deadline by actual mode.
- [x] Pin reverse reference to pre-reverse-boost-align-20261109, compile its unmodified algorithm at matching25/5kHz, compare complete B/X/Y through precharge, random samples, HOLD/resume and reinitialization.
- [x] Run complete tests/run_tests.ps1; rebuild both existing Keil projects with no download; verify AXF/HEX/CCM/integer IRQ and unchanged bench HEX.
- [x] Independent read-only review without blockers; record final verification in docs and README.

Finalization: commit and tag main-reverse-boost-align-20261109, push existing branch/tag,
and verify remote SHA before reporting upload complete. Source baseline70e5a84 is already remote.
Evidence:86/86groups,720000bitwise steps, ARM main/bench0/0, four image checks and independent review.
Detailed outcome and exact artifact hashes are in docs/反向Boost对齐测试-20261109.md.
