# Preliminary ADC calibration — 2026-10-24

User measurements, with MOSFETs not installed and UI STOPPED:

- INA240 supply 4.95 V; INA240 OUT and MCU PA2 each 2.49 V.
- MCU 3.3 V / VDDA / VREF+ reported 3.31 V. PA0 reported approximately 0.54 V.
- Initial actual input 11.93 V, old OLED feedback 11.63–11.68 V.
- Latest actual input 11.83 V, old OLED feedback approximately 11.57 +/-0.02 V.

Use the latest pair for a provisional left-port scale correction, preserving
the actual ADC counts: 0.047 * (11.57/11.83) * (3.31/3.30) = 0.046107...,
rounded to 0.0461. The 0.54 V report lacks the precision/consistency needed to
independently infer an exact analog gain. No full-range or loaded calibration
has been performed; verify at ~6 V and another higher voltage next.

Common parameters live in Core/Inc/adc_measure.h. They are used by foreground
conversion and float-only control IRQ; the OCP DAC reference and current zero
also use these shared constants. Right voltage gain stays nominal 0.047 since
there is no right-port calibration data. Current sensitivity remains nominal
0.1 V/A; zero 2.49 V assumes no shunt current during the reported idle test.
Known-current gain and polarity verification is still required.

Updated DAC window for +/-7 A: 3.19 V / 1.79 V at 3.31 V reference, codes 3948 /
2215. The wiring confirmation guard stays 0, so default power start is blocked.
No automatic zeroing, display filtering, PI tuning or output enable was added.

Regression tests failed on the previous code for measured left-voltage counts
and for backend feedback with the new reference/zero. They now check both
measured input points, independent right-channel gain, current polarity/zero,
UI/controller conversion consistency and DAC threshold codes.

Verification: all 12 test groups pass (host_tests.log). Both Keil full rebuilds
exit 0 with no errors/warnings: MDK-ARM/adc_calibration_build.log (default,
Code35000/RO1272/RW40/ZI4080), adc_calibration_ocp_build.log (compile-only enabled
configuration, Code38168/RO1292/RW40/ZI4176). Default HEX SHA256:
285EE7CE7291CF1F0545F8D8C9427AC2DF3B61A49135CEAD544D7199333174F1.
No board download was performed; user must load the default project image and
compare simultaneous meter/OLED readings, then check another voltage point.
