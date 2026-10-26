#include "forward.h"
#include "pwm_plan.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void mapping(void)
{
    assert(PWM_EntryIsTimely(1000U, 640U));
    assert(!PWM_EntryIsTimely(2000U, 640U)); /* whole missed period, same phase */
    assert(!PWM_EntryIsTimely(1000U, 9000U)); /* delayed current entry */
    PwmPlan p;
    assert(PWM_MakePlan(0.5f, 0, &p));
    assert(fabsf(p.left_high - 0.425f) < 0.0001f);
    assert(fabsf(p.right_high - 0.85f) < 0.0001f);
    assert(PWM_MakePlan(2.0f / 3.0f, 1, &p));
    assert(fabsf(p.left_high - 0.85f) < 0.0001f);
    assert(fabsf(p.right_high - 0.85f / 3.0f) < 0.0001f);
    assert(p.left_compare == 28160U); /* .85 effective + .03 dead time */
    for (unsigned i = 0; i <= 950; ++i) {
        assert(PWM_MakePlan(i / 1000.0f, 1, &p));
        assert(p.left_compare < PWM_PERIOD_TICKS);
        assert(p.right_compare >= PWM_MIN_COMPARE_TICKS);
        assert(PWM_PERIOD_TICKS - p.left_compare - PWM_DEADTIME_TICKS >= 2880U);
    }
    assert(!PWM_MakePlan(NAN, 0, &p));
    assert(!PWM_MakePlan(-0.1f, 0, &p));
    assert(!PWM_MakePlan(1.0f, 0, &p));
    assert(!PWM_MakePlan(INFINITY, 1, &p));
    assert(!PWM_MakePlan(0.5f, 1, NULL));
}

static void controller(void)
{
    forward_configure(12.0f, 36.0f, 6.0f);
    forward_initialize(0.0f);
    forward_U.mode = 2.0f;
    forward_U.V_left = 12.0f;
    forward_U.V_right = 11.0f; /* reachable with bootstrap refresh */
    forward_U.IL = 0.0f;
    for (unsigned n = 0; n < 150000; ++n) {
        forward_step();
        assert(isfinite(forward_Y.D_cmd));
        assert(forward_Y.D_cmd >= 0.0f && forward_Y.D_cmd <= 0.95f);
        assert(forward_B.Saturation4 <= 36.0f);
        assert(forward_B.Saturation <= 6.0f);
    }
    assert(forward_Y.BOOST_RUN);
    assert(fabsf(forward_B.Saturation4 - 36.0f) < 0.001f);
    forward_initialize(0.0f);
    forward_step();
    assert(!forward_Y.BOOST_RUN);
    assert(fabsf(forward_B.Saturation4 - 12.0f) < 0.001f);
}

static void averaged_plant(void)
{
    const float dt = 1.0f / 150000.0f;
    float il = 0.0f, voltage = 0.0f, peak = 0.0f;
    PwmPlan p = {0};
    forward_configure(12.0f, 36.0f, 6.0f);
    forward_initialize(0.0f);
    forward_U.mode = 2.0f;
    for (unsigned n = 0; n < 300000; ++n) {
        /* Averaged 22 uH / 690 uF stage; deliberately not a switching model. */
        float di = (p.left_high * 12.0f - p.right_high * voltage - 0.05f * il) / 22e-6f;
        float dv = (p.right_high * il - voltage / 36.0f) / 690e-6f;
        il += dt * di;
        voltage += dt * dv;
        if (il > peak) peak = il;
        forward_U.V_left = 12.0f;
        forward_U.V_right = voltage;
        forward_U.IL = il;
        forward_step();
        assert(PWM_MakePlan(forward_Y.D_cmd, forward_Y.BOOST_RUN, &p));
        assert(isfinite(il) && isfinite(voltage));
    }
    printf("Averaged 12->36 V, 36 ohm: Vout=%.3f V, peak IL=%.3f A\n", voltage, peak);
    assert(fabsf(voltage - 36.0f) < 0.5f);
    fflush(stdout);
    assert(peak < 7.5f);
}

int main(void)
{
    mapping();
    controller();
    averaged_plant();
    puts("PWM mapping and forward startup tests passed");
    return 0;
}
