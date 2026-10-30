#ifndef PWM_PLAN_H
#define PWM_PLAN_H
#include <stdint.h>

#define PWM_FREQUENCY_HZ       150000U
#define PWM_PERIOD_TICKS       32000U /* 150 MHz x 32 / 150 kHz */
#define PWM_DEADTIME_TICKS     960U   /* 200 ns at 4.8 GHz */
#define PWM_DEADTIME_REG       240U   /* 200 ns at fHRTIM x 8 */
#define PWM_MIN_COMPARE_TICKS  2240U  /* .04 effective high + .03 dead time */
#define PWM_REFRESH_HIGH       0.85f

typedef struct {
    float left_high;
    float right_high;
    uint32_t left_compare;  /* Timer B: PA10 / PA11 */
    uint32_t right_compare; /* Timer A: PA8 / PA9 */
} PwmPlan;

/* D_cmd is left high duty in buck/precharge; right low duty in boost.
 * Both bridge duties are scaled to retain bootstrap refresh on both sides.
 * High pulses below 267 ns are clamped in the default build; the opt-in
 * 12 V bench Buck build uses null duty for subminimum source pulses.
 * Stopping always uses output disable. */
int PWM_MakePlan(float command, int boost_run, PwmPlan *out);
/* Reverse source is Timer A/right. Buck/precharge accepts 0..1 (physical
 * high <=.85); boost accepts 0...95. Subminimum source pulses use HRTIMv2
 * CMP1=0 null duty rather than forcing energy into an uncharged output. */
int PWM_MakeReversePlan(float command, int boost_run, PwmPlan *out);
/* 1000 CPU cycles per period at 150 MHz; tolerate normal entry jitter. */
int PWM_EntryIsTimely(uint32_t elapsed_cycles, uint32_t counter_ticks);
#include "pwm_plan_inline.h"
#endif
