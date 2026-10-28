# SDD ledger — plan: docs/superpowers/plans/2026-10-28-late-input-adc.md

Baseline: 116139d; 25 host test groups PASS before changes.
Execution: inline, with independent technical research and final diff review.
Ruling: Work in the existing checkout on codex/late-input-adc — user requires one active project and no copied project directories — IDE changes remain unstaged.
Ruling: Use ADC2 regular instead of shifting the shared ADC1 trigger — preserve current/output timing and avoid multi-trigger rank drift — ADC2 static offset and hardware timing must be checked on board.
Pre-flight: Task 2 consumes Task 1 late-input build flag for OLED identity; build flag is scoped to FAST, defaults enabled there and is overrideable for baseline builds.
Stage 2: 7V change waits for stage 1 board results, as accepted in the conversation.

Task 1: host RED confirmed on original backend: input_started && input_init_step == 6U failed.
Task 1: GREEN focused FAST integration, then full suite 25/25 PASS (host-tests-task1.log).
Task 1: ADC2 init/cal/config/CMP4/TRG1/arm failures each checked; early/late disagreement,
missing EOC/EOS/OVR tested pending/bootstrap/arming/running and stopped states.
Task 2: OLED RED confirmed after fixture included build config: missing 12V BUCK ADC2.
Task 1: complete — ADC2 behavior and all 25 host groups PASS.
Task 2: OLED GREEN and final 25/25 suite PASS (host-tests-final.log).
Task 2: Keil -r/-j0/-sg rebuild only, exit 0, 0 errors/warnings; no flash/reset/start.
Task 2: CCM copy/vector/uninstrumented PASS, 3384 bytes at 0x10000000, load 0x08007E34.
Task 2: IRQ assembly shows ADC2 ISR mask 0x1C == 0x0C, DR read and W1C,
no new HAL wait call; forward_step remains 696 bytes. Board cycle budget remains unverified.
Task 2: README, bench procedure and dated record updated; HEX hash in manifest.json.
Ruling: Preserve this local branch and commit the candidate — AGENTS.md requests Git history
and a single active project; no merge, push or project copy is necessary for the bench build.
Stage 1 board work pending: stopped DC ADC1/ADC2 comparison, input/PA0 scope comparison,
startup waveform and current firmware isr_cycles_max. Fixed phase may cross switching edges
at other duty ratios; no claim that 4.8 us is universally quiet.
Final review: fresh read-only reviewer /root/late_adc_review found no blocking issue;
checked real HAL trigger/re-arm semantics, PA0 dual ADC support, flags, setup failures,
retained protections and tests. Limits recorded in review.md. Functional code unchanged
after final GREEN suite and independent review; subsequent edits were documentation only.
Task 2: software complete — candidate committed locally; no board result claimed.
