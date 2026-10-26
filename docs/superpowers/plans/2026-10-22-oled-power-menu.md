# OLED Power Menu Implementation Plan

> **For agentic workers:** Execute inline with superpowers:executing-plans and test-driven-development; review the whole change after verification.

**Goal:** Add the approved forward/reverse OLED menu with editable nominal input/output voltage, automatic Buck/Boost choice, telemetry and start/stop.

**Architecture:** A HAL-independent menu state machine produces actions and an eight-row text frame. A foreground adapter consumes real key events, dispatches existing power APIs, and renders/transmits one SSD1306 page per loop.

**Tech Stack:** C11 host tests, STM32G4 HAL, Keil ARMClang 6.18, SSD1306 128×64.

**Spec:** ../specs/2026-10-22-oled-power-menu-design.md

## Global Constraints

- Only the outer bidreaction project changes. No firmware download or real power-stage start.
- KEY1 up/+1 V; KEY2 down/−1 V; KEY3 select/commit/action; KEY4 cancel/back and stop on return.
- Compare committed output/input settings: lower=Buck, higher=Boost, equal=reject. Preserve all measured startup checks.
- Reverse UI only; no reverse start. Input 6–40 V, output 1–36 V, RAM persistence per direction.
- Preserve controller timing, gains, protections, and default disabled PWM outputs.

## Review Focus

- Accepted start pending before the first IRQ must already lock editing and offer stop.
- Fault or ADC invalidity must not cause a false running indication or implicit restart.
- Input setting and measured source disagree: retain backend startup rejection.
- All 10 characters of intput_ret/output_ret must remain visible in 128 pixels.
- Repeated UI refreshes must leave key scanning and control IRQ service available.

### Task 1: Menu behavior and rendering

**Files:** Core/Inc/power_menu.h, Core/Src/power_menu.c, tests/power_menu_test.c.

**Interfaces:** Menu_Init, Menu_HandleKey, Menu_StartResult, Menu_Mode, Menu_Render; MenuTelemetry and MenuRequest are HAL-independent.

- [x] Write behavior tests; run against minimal scaffold and observe missing navigation fail.
- [x] Implement navigation, transactional edits, auto mode, action gating, notices and text frame.
- [x] Verify menu tests including invalid data, pending startup, direction mapping and boundaries.

### Task 2: Hardware adapters and integration

**Files:** power_ui.c/.h, OLED_Function.c/.h, OLED_IIC_Config.c/.h, power_control.c/.h, main.c, MDK-ARM/bidreaction.uvprojx, tests/power_ui_test.c, tests/oled_render_test.c, tests/stm32g4xx_hal.h.

**Interfaces:** PowerUI_Init and PowerUI_Service(const AdcMeasurements *); OLED_DrawStr draws RAM only, OLED_RefreshPage sends one page; two power queries expose pending/valid flags.

- [x] Write adapter and real-font rendering tests, observe failure before implementation.
- [x] Implement adapter and OLED helpers, integrate in main and Keil project.
- [x] Verify boundary commands, key priority, bounded page service, real glyph/pixel bounds; export preview PBM files.

### Task 3: Verification and handoff

**Files:** tests/run_tests.ps1, PWM使用说明.md, this plan and progress ledger.

- [x] Add host tests to suite and document menu operation and reverse limitation.
- [x] Run complete host suite and Keil rebuild; inspect results and actual-font previews.
- [x] Review the complete change, fix material findings with regressions, update ledger and report artifacts.

