#ifndef PWM_PLAN_INLINE_H
#define PWM_PLAN_INLINE_H
#ifndef PWM_PLAN_H
#error Include pwm_plan.h instead of this private implementation.
#endif
#include "power_build_config.h"

static inline float PWM_PulseLimitImpl(float duty)
{
    return duty < 0.04f ? 0.04f : duty;
}

/* Shared expressions for the generic mapper and compile-time Buck path.
 * Inlining lets the IRQ discard unused plan fields without changing duties. */
static inline __attribute__((always_inline)) int PWM_ForwardPlanImpl(float command, int boost_run, PwmPlan *out)
{
    /* This comparison also rejects NaN. */
    if (!(command >= 0.0f && command <= 0.95f) || out == 0) return 0;
    out->left_high = PWM_PulseLimitImpl(boost_run ? PWM_REFRESH_HIGH :
                                      PWM_REFRESH_HIGH * command);
#if POWER_FORWARD_NULL_DUTY
    /* Preserve the HRTIMv2 null-duty policy for subminimum Buck pulses. */
    if (!boost_run && PWM_REFRESH_HIGH * command < 0.04f)
        out->left_high = 0.0f;
#endif
    out->right_high = PWM_PulseLimitImpl(boost_run ?
                                       PWM_REFRESH_HIGH * (1.0f - command) :
                                       PWM_REFRESH_HIGH);
    out->left_compare = (uint32_t)(out->left_high * PWM_PERIOD_TICKS + 0.5f) +
                        PWM_DEADTIME_TICKS;
#if POWER_FORWARD_NULL_DUTY
    if (out->left_high == 0.0f) out->left_compare = 0U;
#endif
    out->right_compare = (uint32_t)(out->right_high * PWM_PERIOD_TICKS + 0.5f) +
                         PWM_DEADTIME_TICKS;
    return 1;
}

/* Same forward mapper with boost_run fixed to zero; no extra IRQ call. */
static inline __attribute__((always_inline)) int PWM_MakeBuckPlan(float command, PwmPlan *out)
{
    return PWM_ForwardPlanImpl(command, 0, out);
}

/* The backend consumes only compares. Keep the full temporary local so its
 * unused float fields never escape to generic/external mapper calls. */
static inline __attribute__((always_inline)) int PWM_MakeBuckCompares(float command, uint32_t *left, uint32_t *right)
{
    PwmPlan plan;
    if (left == 0 || right == 0 || !PWM_ForwardPlanImpl(command, 0, &plan)) return 0;
    *left = plan.left_compare;
    *right = plan.right_compare;
    return 1;
}

/* Shared reverse mapper: MAIN Buck fixes boost_run=0 and consumes compares only. */
static inline __attribute__((always_inline)) int PWM_ReversePlanImpl(float command, int boost_run, PwmPlan *out)
{
    if (!(command >= 0.0f && command <= (boost_run ? 0.95f : 1.0f)) || out == 0) return 0;
    out->left_high = PWM_PulseLimitImpl(boost_run ? PWM_REFRESH_HIGH * (1.0f - command) : PWM_REFRESH_HIGH);
    out->right_high = boost_run ? PWM_REFRESH_HIGH : PWM_REFRESH_HIGH * command;
    /* Do not inject the minimum pulse into a cold output when the current
     * controller asks for zero. HRTIMv2 supports CMP1=0 null duty in our
     * continuous PER-set/CMP1-reset, equality-compare configuration (AN4539 A.1.9). */
    if (out->right_high < 0.04f) out->right_high = 0.0f;
    out->left_compare = (uint32_t)(out->left_high * PWM_PERIOD_TICKS + 0.5f) + PWM_DEADTIME_TICKS;
    out->right_compare = (uint32_t)(out->right_high * PWM_PERIOD_TICKS + 0.5f) + PWM_DEADTIME_TICKS;
    if (out->right_high == 0.0f) out->right_compare = 0U;
    return 1;
}

static inline __attribute__((always_inline)) int PWM_MakeReverseBuckCompares(float command, uint32_t *left, uint32_t *right)
{
    PwmPlan plan;
    if (left == 0 || right == 0 || !PWM_ReversePlanImpl(command, 0, &plan)) return 0;
    *left = plan.left_compare;
    *right = plan.right_compare;
    return 1;
}
#endif
