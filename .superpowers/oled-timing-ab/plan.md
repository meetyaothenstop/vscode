# OLED/control timing isolation — 2026-10-25

Board evidence: fault0x40/detail3, fault_v_right0.0346465 V, elapsed0x76A
=1898 CPU cycles (~12.65 us at150 MHz). ISR pre-write limit840 cycles;
PWM period1000 cycles/6.667 us. This proves this measured deadline violation,
not its precise subroutine/cache/interrupt cause. User proposes OLED/DMA.

Source checks: OLED polling I2C and rendering run in PowerUI_Service in
main. HRTIM IRQ priority0; OLED/I2C code has no PRIMASK/global IRQ disable.
Foreground OLED time is not directly inside the ISR measurement, but cache/
bus/interrupt side effects remain possible. DMA can free CPU transfer work,
but is not yet established as the fix. Do not loosen timing protection.

Small experimental change: opt-in POWER_TEST_SKIP_OLED_ACTIVE, only with
POWER_BENCH_12V_BUCK_TEST. Skip rendering/page transfers while pending,
starting or running; preserve key scanning/start/stop processing. Resume
display on fault or stop. Separate project/HEX and OLED-MUTE home identity;
all existing controller/ADC/PWM/protection code and thresholds are retained.

- [x] Update UI fixture: expected zero page writes after accepted start,
  during pending/running; K3/K4 stop still works; stopped/fault rendering resumes.
- [x] Observe RED on current code; implement conditional refresh gate/identity.
- [x] Run full host suite and compile normal bench/experimental images.
- [x] Independent focused review, record artifacts and board comparison steps.

Board comparison: same12 V/.2 A,100-ohm load, same SWD settings. Load one
image at a time. Experimental display freezing during active control is
intentional; if it remains frozen, press K4 to STOP before halting CPU.
Compare first fault and fault_elapsed_cycles; if no fault, read preserved
isr_cycles_max after Stop. No DMA, clock, PI or memory-placement change in
this experiment. No automatic download/start. This workspace has no Git;
keep .before backups and test/build logs instead of commits/worktrees.
