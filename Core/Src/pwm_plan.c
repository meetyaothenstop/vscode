#include "pwm_plan.h"
#include "power_build_config.h"

int PWM_EntryIsTimely(uint32_t elapsed_cycles, uint32_t counter_ticks)
{
    return elapsed_cycles <= 1256U && counter_ticks <= 8192U;
}

static float pulse_limit(float duty)
{
    return duty < 0.04f ? 0.04f : duty;
}

int PWM_MakePlan(float command, int boost_run, PwmPlan *out)
{
    /* This comparison also rejects NaN. */
    if (!(command >= 0.0f && command <= 0.95f) || out == 0) return 0;
    out->left_high = pulse_limit(boost_run ? PWM_REFRESH_HIGH :
                                PWM_REFRESH_HIGH * command);
#if POWER_FORWARD_NULL_DUTY
    /* Match the existing reverse Buck null-duty policy: a cold output
     * must not receive forced minimum energy when the PI requests less.
     * CMP1=0 is the HRTIMv2 continuous PER-set/CMP-reset zero-duty case. */
    if (!boost_run && PWM_REFRESH_HIGH * command < 0.04f)
        out->left_high = 0.0f;
#endif
    out->right_high = pulse_limit(boost_run ?
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

int PWM_MakeReversePlan(float command, int boost_run, PwmPlan *out)
{
    if (!(command >= 0.0f && command <= (boost_run ? 0.95f : 1.0f)) || out == 0) return 0;
    out->left_high = pulse_limit(boost_run ? PWM_REFRESH_HIGH * (1.0f - command) : PWM_REFRESH_HIGH);
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
