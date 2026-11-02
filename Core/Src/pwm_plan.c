#include "pwm_plan.h"
#include "power_build_config.h"

int PWM_EntryIsTimely(uint32_t elapsed_cycles, uint32_t counter_ticks)
{
    return elapsed_cycles <= 1256U && counter_ticks <= 8192U;
}

int PWM_MakePlan(float command, int boost_run, PwmPlan *out)
{
    return PWM_ForwardPlanImpl(command, boost_run, out);
}

int PWM_MakeReversePlan(float command, int boost_run, PwmPlan *out)
{
    return PWM_ReversePlanImpl(command, boost_run, out);
}
