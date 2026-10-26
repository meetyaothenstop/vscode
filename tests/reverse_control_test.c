#include "reverse.h"
#include "pwm_plan.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void reference_limits_and_restart(void)
{
    reverse_configure(12.0f, 36.0f, 6.0f);
    reverse_initialize(0.0f);
    reverse_U.mode = 4;
    reverse_U.V_right = 12;
    reverse_U.V_left = 0;
    reverse_U.IL = 0;
    for (unsigned i = 0; i < 150000; ++i) {
        reverse_step();
        assert(reverse_B.Saturation4 <= 36.0f);
        assert(reverse_B.Saturation <= 6.0f);
        assert(reverse_B.DiscreteIntegrator4 <= 2.5f);
        assert(isfinite(reverse_Y.D_cm_R));
    }
    reverse_U.V_left = 12.1f;
    for (unsigned i = 0; i < 150000; ++i) reverse_step();
    assert(reverse_Y.BOOST_RUN_R);
    assert(fabs(reverse_B.Saturation4 - 36.0f) < 0.001f);
    reverse_configure(0.1f, 12.0f, 4.0f);
    reverse_initialize(0.0f);
    assert(!reverse_Y.BOOST_RUN_R && !reverse_Y.D_cm_R4);
    reverse_U.mode = 3;
    reverse_U.V_right = 24;
    reverse_step();
    assert(fabs(reverse_B.Saturation4 - 0.1f) < 0.0001f);
    reverse_U.mode = 0;
    reverse_step();
    assert(!reverse_Y.EN_R_BUCK && !reverse_Y.EN_R_BOOST);
}

static void mapping(void)
{
    PwmPlan p;
    assert(PWM_MakeReversePlan(0.5f, 0, &p));
    assert(fabsf(p.left_high - 0.85f) < 0.0001f);
    assert(fabsf(p.right_high - 0.425f) < 0.0001f);
    assert(PWM_MakeReversePlan(2.0f / 3.0f, 1, &p));
    assert(fabsf(p.left_high - 0.85f / 3.0f) < 0.0001f);
    assert(fabsf(p.right_high - 0.85f) < 0.0001f);
    assert(p.right_compare == 28160);
    assert(PWM_MakeReversePlan(1.0f, 0, &p));
    assert(fabsf(p.left_high - 0.85f) < 0.0001f && fabsf(p.right_high - 0.85f) < 0.0001f);
    assert(!PWM_MakeReversePlan(1.0f, 1, &p));
    assert(PWM_MakeReversePlan(0.0f, 0, &p));
    assert(p.right_high == 0.0f && p.right_compare == 0U);
    assert(!PWM_MakeReversePlan(NAN, 0, &p));
    assert(!PWM_MakeReversePlan(-0.1f, 0, &p));
    assert(!PWM_MakeReversePlan(0.5f, 0, 0));
}

static void averaged_case(float source, float target, float load, unsigned mode)
{
    const float dt = 1.0f / 150000.0f;
    float il = 0, voltage = 0, peak = 0, opposite_peak = 0;
    unsigned opposite_cycle = 0;
    int opposite_boost = 0;
    int blank = 0;
    PwmPlan p = {0};
    reverse_configure(mode == 4 ? source : 0.1f, target, 6.0f);
    reverse_initialize(0.0f);
    reverse_U.mode = (float)mode;
    for (unsigned i = 0; i < 225000; ++i) {
        if (blank) {
            /* Four disabled MOSFETs: negative IL freewheels through the left
             * high/right low body diodes until zero, then cannot reverse. */
            float next_il = fminf(0.0f, il + dt * (voltage - 0.05f * il) / 22e-6f);
            voltage += dt * (-(il + next_il) * 0.5f - voltage / load) / 690e-6f;
            il = next_il;
        } else {
            float di = (p.left_high * voltage - p.right_high * source - 0.05f * il) / 22e-6f;
            float dv = (-p.left_high * il - voltage / load) / 690e-6f;
            il += dt * di;
            voltage += dt * dv;
        }
        if (-il > peak) peak = -il;
        if (il > opposite_peak) { opposite_peak = il; opposite_cycle = i; opposite_boost = reverse_Y.BOOST_RUN_R; }
        reverse_U.V_right = source;
        reverse_U.V_left = voltage;
        reverse_U.IL = il;
        reverse_step();
        assert(PWM_MakeReversePlan((float)reverse_Y.D_cm_R, reverse_Y.BOOST_RUN_R, &p));
        blank = mode == 4 && reverse_Y.D_cm_R4 && !reverse_Y.BOOST_RUN_R;
        assert(isfinite(voltage) && isfinite(il));
    }
    printf("Reverse average %.0f->%.0f V, R=%.0f: Vout=%.3f, |IL|peak=%.3f, opposite=%.3f @%u boost=%d\n",
           source, target, load, voltage, peak, opposite_peak, opposite_cycle, opposite_boost);
    fflush(stdout);
    assert(fabsf(voltage - target) < 0.5f);
    assert(peak < 7.5f && opposite_peak < 1.0f);
}

int main(void)
{
    reference_limits_and_restart();
    mapping();
    averaged_case(12, 36, 36, 4);
    averaged_case(24, 12, 12, 3);
    averaged_case(36, 12, 12, 3);
    averaged_case(6, 12, 12, 4);
    averaged_case(24, 36, 36, 4);
    averaged_case(35, 36, 36, 4);
    averaged_case(6, 36, 72, 4);
    averaged_case(40, 1, 2, 3);
    puts("Reverse controller references, mirrored PWM, Buck/Boost models: PASS");
    return 0;
}
