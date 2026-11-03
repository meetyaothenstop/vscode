# EVT1 fault and foreground diagnostics implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Deliver the user-approved diagnostic image identifying the pre-fault state and foreground/OLED progress without changing control policy.

**Architecture:** A separate `power_event` symbol holds double-buffered foreground/UI progress and an immutable first-fault record. Progress publishes from foreground with DMB and an atomic slot index, without masking interrupts. Fault capture runs only after emergency output disable under the existing fault lock, before cancellation destroys state.

**Tech Stack:** Existing STM32G474 C/ARMCLANG6.18 main project; GCC register fixtures and linked HEX/disassembly verification.

**Spec:** User-approved two-part diagnostic proposal immediately before “你继续”; this document records implementation choices, not additional user requirements.

## Global constraints

- Keep existing main/four modes/sole bench; PWM/protection150kHz, current50kHz, voltage10kHz, PI and thresholds unchanged.
- Default diagnostics only for RAW MAIN; explicitly disableable for equivalence comparison. No extra normal fast-IRQ probes.
- Cold capture stores prior state/mode/target/pending/bootstrap/arming/polarity count, last committed duty/reference/compares, progress snapshots and ages.
- Progress records service entry count/time/max gap and last UI stage/page/HAL result. Use DWT and HAL tick; DWT differences wrap modulo32 bits (28.63s at150MHz).
- First fault freezes until successful explicit clear; failed clear/repeated trips/foreground after fault cannot overwrite it. Per-run progress resets at accepted start, within existing stopped-state lock.
- Test synthetic interrupt preemption during slot publication; fault receives the old complete slot. Call counts are entry counts, not stage counts.
- Identity EVT1 distinguishes instrumentation from RAW1; no board/UI operations during build. Old AXF addresses no longer valid.
- Backup tag pre-eventdiag-20261103 remotely verified at9652d4b before edits. Preserve original unrelated IDE/scratch changes; work inline in active user project on codex/main-buck.

## Review focus

- Fault before first foreground/UI entry: validity/count0 and explicit age0, no invented elapsed time.
- Fault interrupts unpublished progress: coherent old slot, no half-published timing pair.
- Initial BOOTSTRAP/ARMING/HOLD/recovered RUNNING: record original state and counters before cancel.
- Repeated fault/failed acknowledgement: first packet immutable; successful clear removes it.
- DMA busy/startup mute/render/page submission: distinguish UI service liveness from display progress; unchanged transfer policy.

## Task 1: backend and publication

- [x] Add failing `tests/event_backend_test.c` on real backend for phase/context lock, immutability/clear, DWT wrap, tick/cycle ages, no heartbeat and mid-publish IRQ. Run and observe missing diagnostic API failure.
- [x] Add `Core/Inc/power_event_diag.h` and `Core/Src/power_event_diag.inc`; integrate init/start/clear/fault/Power_Service only, default RAW MAIN.
- [x] Run tests to GREEN; existing no-event build remains supported.

## Task 2: UI stages and identity

- [x] Add failing event stage assertions to main UI fixture (entry/wait DMA/startup/render/submit/error), observe failure before hooks.
- [x] Add foreground-only marks in `power_ui.c`, no scheduling changes. EVT1 menu identity and matching expectations.
- [x] Run real backend and UI regressions, then full suite.

## Task 3: artifact and delivery

- [x] Main and bench rebuild0/0. Check actual vector/CCM/stack/DMA/HEX-AXF and linked healthy functions have no FP. Bench hash must remain A9BA257B...D451.
- [x] One fresh independent read-only review; fix important findings with RED→GREEN. No false board-fix claim.
- [x] Update README and Chinese test handoff with field interpretation and host timing limits; commit/tag/push via existing HTTPS credentials and verify remote SHA. User downloads.
