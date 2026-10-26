# Reverse Controller Integration Plan

**Goal:** Audit the supplied PLECS reverse controller and integrate working reverse Buck/Boost into the existing outer STM32G474 project and automatic OLED menu.

**Authorization:** User requested inspection followed by assembly into this project. Necessary controller adaptations and regression tests are included; no firmware download or real power-stage start.

**Architecture:** Keep physical left/right ADC and Timer B-left/Timer A-right wiring. Reverse modes 3/4 use negative physical IL, swapped source/output voltage checks, and mirrored bootstrap PWM duties. Adapt only project copies of the export to single precision, configurable bounded reference/current, and conditional PI integration.

## Constraints and interfaces

- Supplied reverse.c/h remain unchanged under Desktop/plecs/四开关代码.
- Modes 1/2 remain forward Buck/Boost; add 3/4 for reverse Buck/Boost. Existing debugger command values 1–4 retain meaning; new commands 5/6 start reverse Buck/Boost.
- Source input 6–40 V, output target 1–36 V, cold output ≤0.25 V, stopped current <0.25 A, reference 6 A and absolute trip 7.5 A retained.
- Reverse source is V_right, output is V_left; negative IL is expected. Forward wrong polarity is unchanged; reverse IL>+1 A must persist for16samples (107us) to tolerate brief synchronous ripple. Absolute7.5A trips immediately in either direction.
- One controller executes per IRQ; preserve timing checks, paired preload update, startup low-side bootstrap and default disabled outputs.
- Existing OLED chooses Buck/Boost using committed input/output settings. Reverse requests map that choice to backend modes 3/4; actual measured startup checks remain authoritative.

## Review focus

- Reverse export reference/precharge are unbounded; controller copies must bound both and maintain final target.
- PWM mirrors effective duties, not physical timer indices; arming plan must also mirror.
- Reverse output residual/overvoltage checks must inspect left bus, source faults right bus.
- Negative current is normal in reverse but ±7.5 A and wrong polarity remain faults.
- Test real power_control.c register interactions as well as menu boundary stubs; verify ADC loss and skipped IRQs still turn outputs off.

## Tasks

1. [x] Audit original export with probe and independent math review; record required adaptations.
2. [x] Write failing reverse-controller/PWM tests, adapt project copies, verify average and ideal switching cases for reverse Buck/Boost plus reinitialization.
3. [x] Add failing real-backend startup/protection tests and reverse UI tests; integrate directional control, PWM mapping, commands and menu.
4. [x] Update Keil project and documentation; run all host tests, Keil full build, inspect ISR implementation/stack and complete independent review.

## Decisions verified during implementation

- Correct Boost feedforward to1−Vin/Vref; use single precision, bounded configurable reference/current and conditional integration. Probe confirmed original1s reference144V and precharge500A.
- Reverse Buck/precharge command limit1 with both physical bridge highs<=.85 permits99% precharge completion. Null CMP1 duty on subminimum source pulses prevents forced-energy cold-start oscillation; HRTIMv2 continuous/equality mode per ST AN4539 A.1.9.
- Reverse Boost switching gap disables all gates and waits for a Boost plan to latch before enabling, rather than changing mapping under a stale Buck plan.
- No-load test initially rose to96V. HOLD gatesoff at1.005×finaltarget, resumes below.99×target with |IL|<.25A, refreshes bootstrap and arms the retained operating mode. Holding freezes voltage PI and clears current PI; resume clears PI and starts reference at measured output. This fixes light-load resume windup (initial model17A) while reference/filters continue updating during HOLD.
- Eight average and eleven ideal switching cases pass. Light-load resume can show+1.917A signed instantaneous ripple for5samples; reverse polarity qualifies16samples while absolute7.5A protection remains immediate.
- Ten host test groups pass; Keil full build0errors/0warnings. Stack callgraph foreground528B +IRQ136B +hardwareFP frame104B≈768B/1024B; actual timing and waveforms require board validation. Originals SHA256 unchanged; no flash/hardware operation.
- Independent review found a pre-existing cold bootstrap off-by-one. Added failing30-complete-interval test, then skipped the enabling IRQ decrement after all protection checks. Cold and HOLD now both give30complete low-side periods plus2arming periods. Reviewer confirmed fix, independently reran backend tests and reported no remaining finding. Final full suite10/10 and Keil0errors/0warnings, Code34860B.
