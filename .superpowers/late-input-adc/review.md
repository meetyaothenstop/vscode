# Independent review — 2026-10-28

Fresh read-only reviewer `/root/late_adc_review`, selected functional diff versus `116139d`.
No blocking findings. No hardware, IDE or file mutations by reviewer.

Checked:
- Local HAL ADC1/2 regular EXTSEL21 = HRTIM TRG1; TRG1 TA CMP4 mapping.
- Repeated external regular triggers after one HAL_ADC_Start, continuous mode disabled.
- PA0 ADC12_IN1 support in ST DS12288 pin table.
- Both EOC/EOS required, OVR rejected; ADC2 DR source and W1C with no early fallback.
- Six setup/arm failures return before counters, outputs disabled at init entry.
- Original injected ranks/timing, protections and 840/936-cycle gates retained.
- Meaningful early/late disagreement, snapshots and startup/run invalid sample tests.

Limits: host flags and synthetic registers do not emulate silicon trigger timing or W1C.
ADC matching, settling/noise and worst-case IRQ cycles require the documented board checks.

Primary pin reference: https://www.st.com/resource/en/datasheet/stm32g474cb.pdf#page=59
