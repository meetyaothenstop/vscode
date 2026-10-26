# CCM Timing Comparison Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver a behavior-preserving CCM execution comparison for the measured first-control T3.

**Architecture:** Separate restricted target and explicit scatter file; unchanged control sources. Verify placement and startup copy using linked map and HEX.

**Tech Stack:** STM32G474CB, Armclang6.18/armlink, PowerShell, gcc fixtures.

**Spec:** `docs/superpowers/specs/2026-10-26-ccm-timing-design.md`

## Global Constraints

- Preserve all840/936-cycle and ADC/current/voltage protections, clocks and enable sequencing.
- CCM0x10000000..0x10007FFF; ordinary SRAM0x20000000..0x20017FFF; Flash128KiB.
- Standard scatter initialization; reset and emergency-off remain in Flash.
- No hardware download/start, control math rewrite or DMA conversion.

## Review Focus

- CCM code must have a Flash load image and standard startup copy entry, including cold boot.
- Ordinary RW/stack must not overlap the CCM alias.
- IRQ vector and far calls must resolve correctly, without moving emergency-off into uninitialized RAM.
- Diagnostic identities/flags must correspond to actual linked placement.
- Board speedup must not be claimed from host or linker validation alone.

### Task1: Placement verification and independent target

**Files:** `.superpowers/ccm-timing/verify_build.ps1`, `MDK-ARM/bench12_ccm_diag.sct`, `MDK-ARM/bench12_ccm_diag.uvprojx/.uvoptx`.
**Interfaces:** verifier takes MapPath/HexPath; exits nonzero on wrong layout/copy. Target=bench12_ccm_diag.

- [x] Write placement/copy verifier and observe RED on Flash unified baseline.
- [x] Create CCM scatter/target; build ARM image and verify GREEN.

### Task2: Identity, regressions and delivery

**Files:** `Core/Inc/power_build_config.h`, `Core/Src/power_menu.c`, `tests/bench12_menu_test.c`, `tests/run_tests.ps1`, `docs/CCM耗时对照测试.md`.
**Interfaces:** POWER_CCM_CONTROL=1 requires unified bench/mute; home=12V CCM DIAG; HEX=MDK-ARM/bench12_ccm_diag/bidreaction_bench12_ccm_diag.hex.

- [x] Add menu identity test, observe RED, then implement identity/guard.
- [x] Complete host regressions and inspect standard scatter/vector placement in linked artifacts.
- [x] Independent final review; document remaining board checks, baseline and one-attempt test instructions.
