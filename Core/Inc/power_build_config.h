#ifndef POWER_BUILD_CONFIG_H
#define POWER_BUILD_CONFIG_H

/* Main application policy is independent of all restricted bench identities. */
#ifndef POWER_MAIN_APPLICATION
#define POWER_MAIN_APPLICATION 0U
#endif
#ifndef POWER_MULTIRATE_CONTROL
#define POWER_MULTIRATE_CONTROL POWER_MAIN_APPLICATION
#endif
#ifndef POWER_RAW_FAST_IRQ
#define POWER_RAW_FAST_IRQ POWER_MULTIRATE_CONTROL
#endif
#ifndef POWER_EVENT_DIAGNOSTICS
#define POWER_EVENT_DIAGNOSTICS POWER_RAW_FAST_IRQ
#endif
#if POWER_EVENT_DIAGNOSTICS && (!POWER_MAIN_APPLICATION || !POWER_RAW_FAST_IRQ)
#error Event diagnostics requires the MAIN raw fast IRQ.
#endif
#ifndef POWER_ADC_FAULT_DETAILS
#define POWER_ADC_FAULT_DETAILS POWER_EVENT_DIAGNOSTICS
#endif
#if POWER_ADC_FAULT_DETAILS && !POWER_EVENT_DIAGNOSTICS
#error ADC fault details requires MAIN event diagnostics.
#endif
#ifndef POWER_MAIN_ADC_SOFTWARE_ONLY
#define POWER_MAIN_ADC_SOFTWARE_ONLY 0U
#endif
#if POWER_MAIN_ADC_SOFTWARE_ONLY && !POWER_MAIN_APPLICATION
#error Main ADC-only policy requires the main application profile.
#endif

/* Explicit opt-in for the separate 12 V ADC-only bench image.
 * This does not assert hardware OCP readiness or validate board wiring. */
#ifndef POWER_BENCH_12V_BUCK_TEST
#define POWER_BENCH_12V_BUCK_TEST 0U
#endif

/* Temporary timing experiment: keep keys/control alive, suppress display
 * rendering and I2C pages only while a bench start/run is active. */
#ifndef POWER_TEST_SKIP_OLED_ACTIVE
#define POWER_TEST_SKIP_OLED_ACTIVE 0U
#endif
#if POWER_TEST_SKIP_OLED_ACTIVE && !POWER_BENCH_12V_BUCK_TEST
#error OLED timing isolation requires the restricted 12 V bench profile.
#endif

/* Extra CYCCNT probes and coherent first-fault record. Deliberately opt-in:
 * probes consume part of the existing budget; they do not relax it. */
#ifndef POWER_UNIFIED_DIAGNOSTICS
#define POWER_UNIFIED_DIAGNOSTICS 0U
#endif
#if POWER_UNIFIED_DIAGNOSTICS && !POWER_MAIN_APPLICATION && (!POWER_BENCH_12V_BUCK_TEST || !POWER_TEST_SKIP_OLED_ACTIVE)
#error Unified diagnostics requires main application or restricted bench with OLED mute.
#endif

/* Placement experiment only. The linked CCM region is verified separately;
 * enabling this identity must not enable unrestricted power operation. */
#ifndef POWER_CCM_CONTROL
#define POWER_CCM_CONTROL POWER_MAIN_APPLICATION
#endif
#if POWER_CCM_CONTROL && !POWER_MAIN_APPLICATION && (!POWER_BENCH_12V_BUCK_TEST || !POWER_TEST_SKIP_OLED_ACTIVE)
#error CCM timing comparison requires restricted 12 V bench and OLED mute.
#endif

/* Buck-only specialization; no Boost state is maintained in this image. */
#ifndef POWER_BENCH_FAST_BUCK
#define POWER_BENCH_FAST_BUCK 0U
#endif
#if POWER_BENCH_FAST_BUCK && (!POWER_BENCH_12V_BUCK_TEST || !POWER_TEST_SKIP_OLED_ACTIVE || !POWER_CCM_CONTROL)
#error Fast Buck requires restricted 12 V bench, OLED mute and CCM placement.
#endif

/* FAST can refresh in RUNNING using asynchronous, low-priority I2C DMA.
 * Override to zero for the original all-active-state OLED mute comparison. */
#ifndef POWER_BENCH_LIVE_OLED
#define POWER_BENCH_LIVE_OLED POWER_BENCH_FAST_BUCK
#endif
#if POWER_BENCH_LIVE_OLED && !POWER_BENCH_FAST_BUCK
#error Live OLED is restricted to the FAST Buck bench.
#endif

/* Next restricted bench step, within the same active FAST project. Set zero
 * to rebuild the earlier 12->6 V configuration without copying the project. */
#ifndef POWER_BENCH_24V_BUCK_TEST
#define POWER_BENCH_24V_BUCK_TEST POWER_BENCH_FAST_BUCK
#endif
#if POWER_BENCH_24V_BUCK_TEST && !POWER_BENCH_FAST_BUCK
#error The 24 V bench requires the restricted FAST Buck profile.
#endif
#if POWER_MAIN_APPLICATION && (POWER_BENCH_12V_BUCK_TEST || POWER_BENCH_FAST_BUCK || POWER_BENCH_24V_BUCK_TEST)
#error Main and restricted bench profiles are mutually exclusive.
#endif

/* Phase-only bench experiment: ADC2 reads the same PA0 input later, while
 * ADC1 current/output sampling stays at the original injected trigger. */
#ifndef POWER_BENCH_LATE_INPUT_ADC
#define POWER_BENCH_LATE_INPUT_ADC POWER_BENCH_FAST_BUCK
#endif
#if POWER_BENCH_LATE_INPUT_ADC && !POWER_BENCH_FAST_BUCK
#error Late input sampling is restricted to the FAST Buck bench.
#endif
#define POWER_INPUT_ADC_PHASE_TICKS 23040U /* 4.8 us of the 6.667 us PWM period */

/* Implementation capabilities, separate from restricted bench range/policy. */
#define POWER_OLED_DMA (POWER_MAIN_APPLICATION || POWER_BENCH_LIVE_OLED)
#ifndef POWER_OLED_FRAME_RESUME
#define POWER_OLED_FRAME_RESUME POWER_EVENT_DIAGNOSTICS
#endif
#if POWER_OLED_FRAME_RESUME && (!POWER_MAIN_APPLICATION || !POWER_OLED_DMA)
#error Resumable OLED frames require MAIN DMA.
#endif
#define POWER_LATE_INPUT_ADC (POWER_MAIN_APPLICATION || POWER_BENCH_LATE_INPUT_ADC)
#define POWER_FORWARD_NULL_DUTY (POWER_MAIN_APPLICATION || POWER_BENCH_12V_BUCK_TEST)

/* Power-on menu defaults. Edit the four MAIN values together here.
 * Input settings select Buck/Boost in the menu; feedback still comes from ADC.
 * The debug mailbox also defaults to the forward output setting. */
#if POWER_MAIN_APPLICATION
#define POWER_FORWARD_DEFAULT_INPUT_VOLTS  12U
#define POWER_FORWARD_DEFAULT_OUTPUT_VOLTS 18U
#define POWER_REVERSE_DEFAULT_INPUT_VOLTS  12U
#define POWER_REVERSE_DEFAULT_OUTPUT_VOLTS  6U
#elif POWER_BENCH_12V_BUCK_TEST
#if POWER_BENCH_24V_BUCK_TEST
#define POWER_FORWARD_DEFAULT_INPUT_VOLTS  24U
#define POWER_FORWARD_DEFAULT_OUTPUT_VOLTS 12U
#else
#define POWER_FORWARD_DEFAULT_INPUT_VOLTS  12U
#define POWER_FORWARD_DEFAULT_OUTPUT_VOLTS  6U
#endif
#define POWER_REVERSE_DEFAULT_INPUT_VOLTS  36U
#define POWER_REVERSE_DEFAULT_OUTPUT_VOLTS 12U
#else
#define POWER_FORWARD_DEFAULT_INPUT_VOLTS  12U
#define POWER_FORWARD_DEFAULT_OUTPUT_VOLTS 36U
#define POWER_REVERSE_DEFAULT_INPUT_VOLTS  36U
#define POWER_REVERSE_DEFAULT_OUTPUT_VOLTS 12U
#endif

/* Main Buck nominal menu range12..40V; fresh input protection has endpoint
 * margin, without extending the claimed nominal operating range. */
#define POWER_MAIN_INPUT_SETTING_MIN           12U
#define POWER_MAIN_INPUT_SETTING_MAX           40U
#define POWER_MAIN_OUTPUT_SETTING_MIN           0U
#define POWER_MAIN_OUTPUT_SETTING_MAX          36U
#define POWER_MAIN_BUCK_INPUT_MIN_VOLTS        10.0f
#define POWER_MAIN_BUCK_INPUT_MAX_VOLTS        42.0f
#define POWER_MAIN_BUCK_MAX_TARGET_VOLTS       36.0f
/* MAIN Buck directions share the forward policy selected by the user. */
#define POWER_MAIN_BUCK_OUTPUT_OVERVOLTAGE_RATIO 1.5f
#define POWER_MAIN_BUCK_OUTPUT_OVERVOLTAGE_MIN_VOLTS 6.0f
#define POWER_MAIN_BUCK_OUTPUT_OVERVOLTAGE_MAX_VOLTS 40.0f
#define POWER_MAIN_BUCK_START_CURRENT_LIMIT_AMPS 1.0f
#define POWER_MAIN_BUCK_START_OUTPUT_MAX_VOLTS   3.0f
#define POWER_MAIN_BUCK_MAX_DUTY                 0.95f

/* MAIN forward Boost cold-start only; reverse Boost keeps its original limit. */
#define POWER_MAIN_FORWARD_BOOST_START_CURRENT_LIMIT_AMPS 1.0f

/* User-selected cold-start acceptance for the new bench image only.
 * Current is strictly below the limit; output voltage may equal its limit.
 * These are not the running overcurrent or HOLD-resume thresholds. */
#if POWER_BENCH_FAST_BUCK
#define POWER_START_CURRENT_LIMIT_AMPS 0.5f
#define POWER_START_OUTPUT_MAX_VOLTS   1.0f
/* Keep proportional protection when selecting a lower target. */
#if POWER_BENCH_24V_BUCK_TEST
#define POWER_OUTPUT_OVERVOLTAGE_RATIO (15.0f / 12.0f)
#else
#define POWER_OUTPUT_OVERVOLTAGE_RATIO (8.0f / 6.0f)
#endif
#else
#define POWER_START_CURRENT_LIMIT_AMPS 0.25f
#define POWER_START_OUTPUT_MAX_VOLTS   0.25f
#define POWER_OUTPUT_OVERVOLTAGE_RATIO 1.1f
#endif

#if POWER_BENCH_12V_BUCK_TEST
#if POWER_BENCH_24V_BUCK_TEST
/* Fixed-phase sampled current exceeds output/load average. Nominal 24->12 V
 * switching regression needs this reference headroom. */
#define POWER_CURRENT_REFERENCE_LIMIT       1.0f
/* User-requested threshold experiment after fault8. This changes the sampled
 * overcurrent trip only; reference/startup/ADC-rail/reverse checks remain. */
#define POWER_CURRENT_TRIP_AMPS             6.0f
#else
#define POWER_CURRENT_REFERENCE_LIMIT       0.5f
#define POWER_CURRENT_TRIP_AMPS             1.5f
#endif
#if POWER_BENCH_24V_BUCK_TEST
#define POWER_INPUT_MIN_VOLTS              20.0f
#define POWER_INPUT_MAX_VOLTS              28.0f
#elif POWER_BENCH_FAST_BUCK
/* User-requested margin after IN13.53/10.31V with measured input11.9..12V.
 * This is an acceptance limit, not a calibration or noise filter. */
#define POWER_INPUT_MIN_VOLTS              10.0f
#define POWER_INPUT_MAX_VOLTS              14.0f
#else
#define POWER_INPUT_MIN_VOLTS              10.5f
#define POWER_INPUT_MAX_VOLTS              13.5f
#endif
#if POWER_BENCH_24V_BUCK_TEST
#define POWER_MAX_TARGET_VOLTS             12.0f
#else
#define POWER_MAX_TARGET_VOLTS              6.0f
#endif
#else
#define POWER_CURRENT_REFERENCE_LIMIT       6.0f
#define POWER_CURRENT_TRIP_AMPS             7.5f
#define POWER_INPUT_MIN_VOLTS               6.0f
#define POWER_INPUT_MAX_VOLTS              40.0f
#define POWER_MAX_TARGET_VOLTS             36.0f
#endif

#endif
