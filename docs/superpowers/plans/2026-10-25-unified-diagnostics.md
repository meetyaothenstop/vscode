# Unified Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Capture startup stage, coherent first-fault samples and IRQ section timing in one restricted diagnostic firmware.

**Architecture:** Compile-time opt-in probes in the real power backend. One first-fault object is latched after output disable, with timing metadata committed atomically. Default and existing bench builds have no profiler storage or probes.

**Tech Stack:** STM32G474, C11, DWT CYCCNT, Keil Armclang 6.18, gcc host fixtures.

**Spec:** `docs/superpowers/specs/2026-10-25-unified-diagnostics-design.md`

## Global Constraints

- Preserve 150 MHz CPU, 150 kHz PWM, 840/936-cycle deadlines and all existing protection thresholds.
- Preserve ADC1 HRTIM TRG2/CMP2 and cold-start/enable sequencing.
- Opt-in requires restricted bench and OLED-mute; no download/start commands.
- This workspace has no Git: preserve originals and review filesystem changes against them.

## Review Focus

- Early T1/T2/ADC faults must identify that current IRQ samples are unavailable.
- S8/S9 before bootstrap, in bootstrap and in arming must be distinguishable.
- Later ADC/hardware faults must not corrupt an earlier sample/timing record.
- Missing sections and CYCCNT wrap must not produce misleading section times.
- Probe overhead must remain inside deadlines and be documented, not treated as a fix.

### Task 1: Backend record and timing probes

**Files:** Modify `Core/Inc/power_control.h`, `Core/Inc/power_build_config.h`, `Core/Src/power_control.c`, `tests/power_control_test.c`, `tests/power_hal/stm32g4xx_hal.h`, `tests/run_tests.ps1`.

**Interfaces:** `volatile PowerDiagnostics power_diag`; `.irq` is live, `.fault` is first-fault latched. Enums describe context, stage and completed checkpoints. Existing PowerStatus ABI remains unchanged.

- [x] Add real-backend tests for stage/freshness/latching/clear, T3 partial profile and wrap; run diagnostic case and observe RED.
- [x] Implement gated probes and atomic capture; run diagnostic tests GREEN.
- [x] Run entire `tests/run_tests.ps1`; all cases pass.

### Task 2: Reviewable bench delivery

**Files:** Create `MDK-ARM/bench12_unified_diag.uvprojx`, `.uvoptx`, `docs/统一诊断版测试.md`; modify `Core/Src/power_menu.c`, `tests/bench12_menu_test.c`, test runner.

**Interfaces:** target `bench12_unified_diag`, home `12V UNIFIED DIAG`, HEX `MDK-ARM/bench12_unified_diag/bidreaction_bench12_unified_diag.hex`.

- [x] Test identity/home and restricted menu start/stop; observe RED then implement GREEN.
- [x] Prefill Watch1 with `power_diag.fault` and existing canonical fields; keep breakpoints empty.
- [x] Compile diagnostic target and an uninstrumented comparison; inspect errors/warnings and binary symbols.
- [x] Document field meanings, overhead, unchanged limits and one-run capture instructions.
- [x] Fresh review of backups/current files, then required final verification; record unresolved board validation honestly.
