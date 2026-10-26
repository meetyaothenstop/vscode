# Preliminary right-port voltage calibration — 2026-10-25

Continuation of the unpopulated-power-stage, STOPPED right-input/reverse-page
measurement workflow. User reports actual right port 11.9 V, OLED about
11.8 V; actual 23.9 V, OLED 23.70-23.77 V. IL is about 0-0.1 A at both
points. These are rounded display/meter reports, not captured raw ADC codes;
the 12 V display has no precise reported range. Auxiliary rails were not
newly measured in this exchange.

The current right gain is 0.047 with 3.31 V ADC reference. For an unchanged
ADC count, new gain = old gain * old display / actual port voltage.
Individual point estimates are 0.046605 at 11.9 V and 0.0466755 at 23.9 V
using display midpoint 23.735 V. A least-squares multiplicative display
correction (no additive offset) gives gain 0.0466615; provisionally use
0.04666. Expected same-count readings: about 11.886 V and 23.873-23.943 V.
Do not infer greater calibration accuracy from the number of stored digits.

Only ADC_RIGHT_VOLTAGE_GAIN changes in production code. Foreground and
float-only controller conversion already share this parameter. Left gain,
VDDA, current zero/sensitivity, PI, input protection thresholds and the
POWER_OCP_WIRING_CONFIRMED=0 startup guard remain unchanged.

Existing ADC calibration fixtures now use inferred counts 686 / 1380 for
the two right-port points (not direct board ADC measurements). Before the
production parameter change, the conversion test fails as expected;
before_test.log records the assertion failure. The backend fixture models
the independently calibrated right channel and checks the shared feedback
at 23.9 V. These are host conversion checks, not board calibration proof.

No firmware download or power-stage start is part of this change. After
loading the default HEX, repeat simultaneous meter/OLED readings near 12 V
and 24 V with MOSFETs still absent and the UI STOPPED. Current gain/polarity,
actual shunt position and hardware fault shutdown still need verification.

Verification: tests/run_tests.ps1 exits 0; all 12 host test groups pass
(host_tests.log). Default Keil full rebuild exits 0, 0 errors / 0 warnings,
Code35000 / RO1272 / RW40 / ZI4080
(MDK-ARM/right_adc_calibration_build.log). The first sandboxed build could
not read installed DFP/CMSIS packs; rerunning with access to the installed
packs succeeded. The default project's protection guard is still 0.
Default HEX SHA256:
A186C79A9CFA37046EC049EED13CF9C037F05DF1E059CA9679287C5E8BD38059.
The previous default HEX is retained as bidreaction.hex.before here.
