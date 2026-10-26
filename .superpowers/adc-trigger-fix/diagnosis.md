# ADC1 injected trigger diagnosis — 2026-10-24

Board: STM32G474, no MOSFETs installed per user. User reports OLED UI/keys work,
12 V input, voltage sense 0.55 V, INA240 output 2.36 V, telemetry shown as --.

Read-only ST-Link debugger observations after user ran firmware and paused:

- power_status.samples = 0, all three cached ADC raw values = 0.
- ADC1 ISR=0x00000001, CR=0x10000009, CFGR=0x80000000,
  JSQR=0x004086EE, JDR1=JDR2=JDR3=0.
- HRTIM MCR=0x00060000, Timer A CR=0x08020008,
  PER=0x7D00, CMP3=0x1F40, ADC1R=0x800, CR1=0x10000.
- Timer B DIER=0x10, ISR=0x203E41, NVIC ISER2=0x20.

Root cause: JEXTSEL=27, called HRTIM_TRG1 by HAL, is restricted to ADC3/4/5
injected groups. On ADC1/2 this encoding selects TIM16_CC1. The HRTIM trigger
was configured and ADC armed, but the requested connection does not exist.
References: vendor stm32g4xx_hal_adc_ex.h IS_ADC_EXTTRIGINJEC;
stm32g4xx_ll_adc.h LL_ADC_INJ_TRIG_EXT_HRTIM_TRG1; RM0440 Tables 67/167.

Fix: HRTIM Timer A CMP2 -> ADC trigger 2 -> ADC1 injected sequence, preserving
CMP=8000/period=32000 and conversion order IL, left voltage, right voltage.
No power output was enabled and no register was written during diagnosis.

Regression: host backend init now checks the ADC1-supported trigger, matching
HRTIM trigger route and unchanged quarter-period sampling. It failed on the
old code at the new ADC trigger assertion before production changes.

The previous host fixture synthesized ADC completion and did not validate the
ADC instance/trigger mapping. Host checks alone do not prove board operation.
Hardware protection wiring confirmation remains disabled by default.

Verification after the fix: all 12 host test groups PASS (host_tests.log).
Keil full rebuild exit 0, zero errors/warnings (MDK-ARM/adc_trigger_build.log):
Code=35760, RO=1272, RW=40, ZI=4080. Updated output:
MDK-ARM/bidreaction/bidreaction.hex. Not downloaded to the board by the agent;
the paused debug session still contains the pre-fix firmware. Board confirmation
requires loading the new image and checking the sample counter/telemetry.
