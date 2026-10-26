# Hardware Overcurrent Implementation Plan

> Execution: native implementation with independent hardware-route audit and final fresh code review. User explicitly requested adding the software; no extra approval handoff for the authorized code work. Physical pin/wiring confirmation remains pending and does not authorize enabling an unverified board route.

**Goal:** Add a real bidirectional COMP/DAC-to-HRTIM shutdown path, safe readiness gating and latched firmware handling.

**Architecture:** Dedicated power_protection.c/.h owns DAC3, COMP1/2, HRTIM FAULT1/4 and hardware status; power_control.c consumes its readiness, source/latched state and safe acknowledge API. Default unconfirmed build refuses power startup. GPIO proposal PB1/PA7 is isolated in that module pending board confirmation.

**Tech Stack:** STM32G474CBT6, existing STM32G4 HAL/CMSIS, Keil armclang, GCC host tests.

**Spec:** docs/superpowers/specs/2026-10-23-hardware-overcurrent-design.md

## Global constraints

- Outer project only, no Git repository: back up changed source under .superpowers/hardware-overcurrent/originals. Do not change original exports or nested project.
- No physical hardware operation or flashing. Default POWER_OCP_WIRING_CONFIRMED=0 until actual wiring verification.
- Preserve TimerB-left/TimerA-right and signed IL; faults disable all four outputs, never auto restart. Existing state/command/fault bits stay compatible; add256hardware and512not-ready.

## Review focus

- Transient comparator trip disappears before ISR: sticky HRTIM fault remains latched.
- Fault during bootstrap, changeover, HOLD or a pending start must cancel all enables.
- Clear/start races with active comparator or newly latched fault must remain off.
- Default unconfirmed image must not silently retain unprotected power operation.
- Correct internal mapping is COMP1→FLT4, COMP2→FLT1, both timers and both complementary outputs; no ADC IRQ substitute.

## Task1: backend protection contract

Files: Core/Inc/power_protection.h, Core/Src/power_control.c, Core/Inc/power_control.h, Core/Src/stm32g4xx_it.c, Core/Inc/stm32g4xx_it.h, tests/power_control_test.c.

Interfaces: PowerProtection_Init(HRTIM_HandleTypeDef*), PowerProtection_IsReady(void), PowerProtection_FaultPending(void), PowerProtection_Acknowledge(void); public Power_FaultIRQ(void).

- [x] Add fake protection API and tests for Init, readiness/start refusal, both hardware faults, transient sticky fault, clear refusal, state/command compatibility and no restart. Observe RED against unchanged backend.
- [x] Implement calls in Init/start/clear/timer IRQ plus fault IRQ and256/512bits/readiness status. Observe GREEN.

## Task2: actual hardware module

Files: Core/Src/power_protection.c, Core/Inc/power_protection.h, Core/Inc/stm32g4xx_hal_conf.h, MDK-ARM/bidreaction.uvprojx, isolated tests/protection_hal fixture and tests/power_protection_test.c.

- [x] Pin down RM0440 fault/DAC settings and actual hardware latching behavior with independent audit.
- [x] Write meaningful HAL/register tests for configured and unconfirmed builds, analog threshold codes, COMP polarity/inputs, FLT1/4 onA/B, output levels, initialization errors and acknowledge semantics. Observe RED.
- [x] Implement module, add HAL COMP/DAC driver sources to Keil, test GREEN.

## Task3: verification and handoff

Files: tests/run_tests.ps1, PWM使用说明.md, new hardware-overcurrent wiring/commissioning guide, this ledger/plan.

- [x] Add both protection module configurations to runner; run full regression suite and inspect all output.
- [x] Keil full rebuild, inspect timing/stack implications, produce HEX. Default image must remain gated until actual board confirmation.
- [x] Independent final review; fix concrete blockers with failing test then fix; rerun required checks.
- [x] Document exact added wiring, approximate thresholds, protection-not-ready512/hardware256, confirmation macro, injection checks and verification limitations. Report actual code readiness separately from board readiness.
