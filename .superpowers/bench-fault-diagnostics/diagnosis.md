# Bench startup / timing fault investigation — 2026-10-25

User first downloaded the default image and saw512; after correcting the
download, some attempts yield128, others64. Reported idle feedback is
11.95 V input, 0.03 V output, 0.1 A. These three values pass static startup
checks, but do not capture transient values or confirm page/settings. OLED
keeps refreshing: firmware intentionally updates samples/UI after fault
while disabling PWM, so this does not establish timely 150 kHz control.
Keil pause/single-step/breakpoint involvement is not yet confirmed.

Later user reports2 and confirms the source stayed in CV, with no CC
indication. Supply current limiting is therefore NOT established as the
cause; brief board-side supply transients or ADC disturbances remain
unconfirmed possibilities. User clarifies different faults occur across
separate resets/clears/downloads, not spontaneous replacement of latched
faults in one run. Earlier512 was explicitly a wrong-image download.

No cause is established yet. Do not change timing/current/cold-start limits.
Add first-fault details and latched CPU timing context to separate causes:

| Display | Meaning |
| --- | --- |
| FAULT:128 S1 | Invalid mode, including reverse/Boost in bench build |
| S2 | Fewer than two samples or invalid sample flag |
| S3 | Target outside1-6 V in bench build, or nonfinite target |
| S4 | Measured source outside10.5-13.5 V in bench build |
| S5 | Static absolute IL not below0.25 A at request |
| S6 | Output above0.25 V at request |
| S7 | Measured source incompatible with requested Buck/Boost relation |
| S8 | Fresh cold-start/arming current not below0.25 A |
| S9 | Fresh cold-start/arming output above0.25 V |
| FAULT:64 T1 | Adjacent ISR entry spacing above1256 CPU cycles |
| T2 | ISR entry timer count above8192 ticks |
| T3 | Before compare write, elapsed exceeds840 CPU cycles |
| T4 | Before compare write, timer count above26880 ticks |
| T5 | ISR end elapsed above936 CPU cycles |
| T6 | New HRTIM repetition flag already pending at ISR end |

At150 MHz, cycle budget1000 corresponds to6.667 us. Timing thresholds are
unchanged. Latched Watch fields: power_status.fault_detail,
fault_elapsed_cycles, fault_spacing_cycles, fault_counter_ticks. They are
recorded only on first timing fault and remain until explicit clear.
isr_cycles_max alone cannot locate faults that return before its update.
Fault output disable precedes diagnostic stores. First reason remains
latched; normal subsequent fault-state sampling cannot overwrite it.
Also latch fault_v_left/fault_v_right/fault_il, fault_raw_vh/fault_raw_vl/
fault_raw_il, and fault_input_volts for input faults. For exact fault2, OLED
shows `FAULT:2 IN x.xx V` using the latched input, while normal telemetry
continues updating. These are the last completed samples at shutdown, not
an independent analog measurement. Snapshot and first-fault state updates
use a short PRIMASK critical section; output disable occurs before stores.
New bench homepage: `12V BUCK SW DIAG`.

Tests first fail on missing cold-start reason and missing OLED suffix.
Existing backend tests now cover S1/S3/S4/S5/S6/S8/S9 and T1/T2/T3 including
latched values, later sample retention and clear-without-restart. UI tests
check actual backend-to-menu detail propagation and visible T3/S5 suffixes.
T4/T5/T6 retain the old guard thresholds and need actual board timing data;
the synthetic fixture does not advance CYCCNT during real controller math.

Additional RED->GREEN: missing snapshot contents and missing latched input
on the UI failed first. Full15-group host suite now passes. Final Keil full
rebuilds both exit0 with0 errors/0 warnings: bench Code34152/RO1272/RW28/
ZI3932; default Code37068/RO1272/RW40/ZI4120. Build logs are
MDK-ARM/fault_diagnostics_bench_build.log and fault_diagnostics_default_build.log.
Bench HEX SHA256:
B8B9CA4013CF3B18C9D7B4397602AFD7E620FE21A50FC7EDCBB4765EC4B4703C
Default HEX SHA256:
3F1D84E2C5BFFCDB0A183A7295CFB46E430CB700B58C603576626A4BF196628B
No board download or automatic restart was performed. Root cause remains
unresolved until the user reports the first latched subcode/measurements.

Next board reports:128 S6, then128 S5 across attempts. Latest displayed
feedback reported as input11.98 V, output3.8 V, IL about0.1 A. The present
3.8 V output exceeds the cold-start .25 V threshold and can account forS6;
S5 means the sampled current failed the .25 A idle test at the request, not
necessarily at the later OLED refresh. Do not widen these limits. Awaiting
independent meter confirmation of output and actual load type/resistance.
It is not yet known whether3.8 V is residual capacitor voltage, a persistent
power path, or an ADC/reporting discrepancy; no hardware cause established.

User confirms both meter and OLED show about3.8 V, the available load is
100 ohm, and all four MOSFETs are now installed. The relationship between
that voltage reading and the attached-load condition was initially unclear.

LATEST CORRECTION: user explicitly reports the abnormal slow rise to about
4 V occurs with NO load. With100-ohm load attached, user says output is
normal both before and after start. Exact loaded pre-start/post-start meter
voltages and RUNNING/FAULT state are requested; do not infer proven6 V
regulation merely from the word normal. Prior assumption of persistent
3.8 V ACROSS100 ohm and its38 mA calculation is superseded and must not be
used as evidence of a substantial unintended power path.

No-load slow charging could be due to a small leakage/bias path; this is a
hypothesis, not confirmation that the MOSFETs/drivers are fault-free or that
the symptom is normal. Off-state semiconductor leakage exists, but its
actual board source has not been measured. Keep100-ohm load for the initial
bench tests and retain all protection thresholds. A no-load4 V output still
violates the .25 V cold-start condition and can explain S6. S5 remains open
pending transient/latched current evidence. Gate-source DC/pulse checks
remain useful if further hardware diagnosis is needed.

Firmware inspection shows explicit four-output ODISR at initialization,
active-high polarity and inactive idle level; IDLEMODE_NONE only excludes
burst-mode idle control per HAL definitions. No PWM/idle configuration was
changed based solely on the no-load voltage observation.

Latest follow-up: to the question asking loaded output before/after start,
user answers both11.94 V and FAULT128 S9. Then reports all four VGS read0 V
with or without load. Awaiting explicit clarification whether11.94 V is the
left input or right output (it matches earlier input readings). Do not infer
loaded regulation, a shorted MOSFET, or input-to-output pass-through until
the measurement port/reference is confirmed. VGS DC readings after a fault
do not establish the absence of brief pulses during startup preparation.
S9 means a fresh sample after request acceptance exceeded .25 V, either
before initial bootstrap enable or during cold bootstrap/arming checks.

Clarification received:11.94 V is the LEFT INPUT, not right output. It is
not evidence of an input-to-output short or successful output regulation.
Actual loaded right-output voltage remains unreported. Next needed evidence
is power_status.fault_v_right (and raw code if available) while S9 remains
latched, compared with an independent measurement at right output+ relative
to board GND. Do not confuse current live telemetry/VGS after shutdown with
the sample/brief gate behavior that caused the startup rejection.

Watch setup screenshot: Keil title is MDK-ARM/bidreaction.uvprojx, Command
shows Load "bidreaction\\bidreaction.axf" (DEFAULT build, not bench12).
Watch contains old samples/raw entries and the old unevaluable static
sample_valid expression; fault_v_right has not been added. CPU is paused
in I2C code, samples/raw values zero. This screenshot is not the requested
bench S9 fault snapshot; paused I2C line alone is not proof of an I2C fault.
Need exit this debug session, open bench12_buck.uvprojx/bench12_buck_sw,
enter debug/run that image, reproduce S9 then halt only after fault and
read public power_status.fault_v_right. Entering the default debug session
may have replaced/reset the previously downloaded bench image. Previous
instruction should have explicitly restated the required project selection.

Next screenshot confirms bench12_buck.uvprojx and correct bench AXF loaded.
Bottom-right pane is Call Stack + Locals, not Watch1; requested snapshot
expression still not visible. Computer-use skill was loaded and Keil target
window discovered, but state capture was denied with "Computer Use was not
approved to use Keil uVision5". No app input or target execution changes were
performed; do not bypass that access restriction. Manual fallback is to add
fault, fault_detail and fault_v_right using WS1 debugger commands, then open
View/Watch Windows/Watch1 and report the Value column from a faulted state.

User subsequently enters WS commands manually. Screenshot reports E122
AGDI memory read failed at0x20000918 (fault) and0x2000094C (fault_detail).
Symbols resolve correctly: current bench map has power_status at0x20000914,
size96, and these offsets match the diagnostic struct. These are valid
SRAM addresses; no expression-name/struct-offset fix is indicated. Target
RAM read transport/access is failing, and its cause is not established.
Saved ST-LINK options request SF10000 (10 MHz); propose changing ONLY SWD
clock to1 MHz, reconnecting, and testing Watch reads while halted at main
before running/start. This is a diagnostic reduction, not proof that clock
rate caused the error. If initial RAM reads work, then reproduce S9 and
read the latched output. Do not alter converter protection to address E122.

Latest Watch screenshot successfully reads fault0x40/detail3 and locked
fault_v_right0.0346465 V; samples1366902, live raw_vh682/raw_vl3/raw_il3079.
Then fault_elapsed_cycles=0x76A=1898 cycles (~12.65 us at150 MHz). This
establishes this attempt's pre-write CPU deadline violation; it is notS9.
The earlier exact SRAM access root cause is not proven by eventual success.

User hypothesizes OLED polling should useDMA. Inspected main-loop UI/I2C:
not called inside control IRQ, no global IRQ disable in I2C driver, timer
IRQpriority0. Foreground time isn't directly inside the1898-cycle measure;
indirect cache/bus/FPU effects still possible. Prepared opt-in OLED-MUTE
comparison, documented .superpowers/oled-timing-ab and docs/OLED耗时对照测试.md.
No DMA/control-clock/PI/timing-threshold change; wait for board comparison
before declaring OLED causality or selecting a performance fix. CCM code
execution is a researched later option (ST AN4296), not implemented here.
