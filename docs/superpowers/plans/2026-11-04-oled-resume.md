# MAIN resumable OLED frames and long-run observations

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans for inline implementation and one fresh read-only final review.

**Goal:** Fix reproduced OLED page starvation under transient startup/run transitions and make minute-long observations interpretable.

**Architecture:** MAIN-only frame completion handshake. A started page owns its immutable DMA snapshot until completed; startup suspends unsent data without starting transfers, then resumes it. UI finishes a frame across transient active-state changes, coalescing telemetry; keys/STOP/fault can request a fresh frame after the packet drains. Existing sample counter supplies additional elapsed-period ages without adding fast-IRQ probes. FRM1 identifies this version.

**Spec:** Ongoing user-authorized troubleshooting; latest correction is 2.49V sensor output both before/after fault and 1–2 minutes of apparent OLED freeze. The trigger16 remains unexplained physically. This change fixes only the reproduced display scheduling defect and observability; frequency/PI/safety remain intact.

## Constraints and review focus

- Preserve main/four modes/sole bench and user-operated downloads. No board operations.
- PWM/ADC/protection150kHz, current50kHz, voltage10kHz and fault16 unchanged.
- Baseline14b4a76, backup pre-oledframe-20261104 pushed/remote SHA verified before edits; preserve unrelated IDE/scratch changes.
- Default frame resume only when MAIN event diagnostics is active; disabled/bench builds retain old driver/UI behavior and bench HEX hash.
- Startup starts neither command nor data packets; previously active packet may drain. Deliberately paused unsent data does not hit the retry timeout; BUSY retry while allowed and active DMA still time out.
- Frame page advances only when data completion is consumed. Busy submits do not advance; repeated startup/run transitions cannot rewind completed pages. Screen RAM remains frozen through a frame unless an urgent redraw is requested after draining.
- User keys/STOP/fault override a stale frame once pending packet completes; startup mute remains. No target variable or flash writes.
- Samples are accepted ADC periods, not an independent wall clock; 32-bit period differences cover about7.95h at150kHz. Missing samples/debug halt affect time meaning. DWT/ticks remain for comparison, not guaranteed wall time.
- Counts/ages/frame-completion records stay first-fault immutable and coherent under unpublished-slot preemption.

## Task1: driver completion and pause

- [x] Add MAIN driver test for command-done→long startup pause→resume exact snapshot→one-shot data completion; retain timeout tests.
- [x] Observe RED on old driver, implement main-only paused phase and OLED_TakePageComplete handshake, GREEN.

## Task2: UI frame scheduling

- [x] Add regression covering 400 startup/run transitions with pending command/data, immutable frame, progress to all8 pages, STOP/fault replacement and errors.
- [x] Observe page7 starvation RED, implement completion-based advance, coalesced transient changes and urgent redraw; GREEN with legacy UI contract.
- [x] Publish complete-frame event and extend age snapshots with existing ADC periods; test120s duration despite wrapped DWT and skewed HAL tick, immutable freeze.

## Task3: delivery

- [x] Full regression, MAIN/bench ARM rebuild0/0, actual image/stack/CCM/DMA/AXF, integer fast IRQ and no event probes; exact bench hash.
- [x] Fresh independent review, one TDD fix pass for important findings. Document limits and measured-vs-sampled distinction.
- [x] README/test handoff/manifest; commit/tag/push and verify expected remote branch+tag. User downloads.
