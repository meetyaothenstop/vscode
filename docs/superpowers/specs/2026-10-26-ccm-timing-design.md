# CCM execution comparison for measured first-control T3

Board evidence: unified diagnostic first-fault valid_mask=0x3F, stage=7,
entry_state=POWER_ARMING, entry_spacing=1000 CPU cycles, entry_counter=0x1720.
Cumulative checkpoints: 192,302,527,735,1785,2026 cycles. Section differences:
192,110,225,208,1050,241. All include probes. No register-write checkpoint.
The write budget remains840 cycles. This locates the longest region in controller
execution; it does not by itself prove Flash access is its root cause.

Selected first optimization: preserve instructions/control behavior and change
the hot path's execution placement in a separate build. STM32G474xB has32KiB
CCM at0x10000000, aliased at0x20018000 (DS12288 Rev6 section3.5).
Use an Arm scatter execution region copied from Flash by standard __main startup.
Keep vectors, reset/startup and emergency-off/fatal handling executable in Flash.
Move Power_TimerIRQ, its IRQ wrapper, enable_outputs, forward_step, PWM mapping
and the protection polling stub into CCM. Keep data/stack in ordinary96KiB SRAM
to exclude the overlapping CCM alias. No VTOR remap or manual code copy.

Do not change thresholds, sample trigger/phase, controller math, PI parameters,
PWM/CPU clocks, output sequencing or OLED transfer strategy. Require restricted
bench, unified probes and OLED-mute. Give this build its own project/HEX/identity.
Keep the Flash unified image as a baseline. Do not flash/start automatically.

Alternatives: rewriting the generated controller or reducing control rate changes
more behavior before memory-placement impact is measured. DMA-first does not
target the measured controller region. Defer both for this comparison.

Validation: a build-artifact checker must reject the existing Flash baseline and
accept only a CCM-linked image with hot symbols in CCM, emergency/reset in Flash,
RW limited below the alias and a Flash-backed startup copy table. Run host suite,
ARM builds and independent review. Runtime improvement and cold-boot execution
remain board-verification requirements; do not claim T3 fixed from linking alone.
