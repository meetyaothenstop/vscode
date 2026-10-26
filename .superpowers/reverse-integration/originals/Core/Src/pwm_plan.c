#include "pwm_plan.h"

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
    out->right_high = pulse_limit(boost_run ?
                                 PWM_REFRESH_HIGH * (1.0f - command) :
                                 PWM_REFRESH_HIGH);
    out->left_compare = (uint32_t)(out->left_high * PWM_PERIOD_TICKS + 0.5f) +
                        PWM_DEADTIME_TICKS;
    out->right_compare = (uint32_t)(out->right_high * PWM_PERIOD_TICKS + 0.5f) +
                         PWM_DEADTIME_TICKS;
    return 1;
}
