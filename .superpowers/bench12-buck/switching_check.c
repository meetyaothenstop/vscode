#include "forward.h"
#include "pwm_plan.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    /* Ideal switched stage with the real sample phase and one-period
     * compare preload delay. Excludes diode and analog sensor behavior.
     * Numerical substeps are not a replacement for PLECS or board tests. */
    const unsigned substeps = 400U;
    const double dt = 1.0 / (150000.0 * 400.0);
    double il = 0.0, voltage = 0.0, sample_il = 0.0, sample_v = 0.0;
    double peak = 0.0, sampled_peak = 0.0, sampled_min = 0.0;
    unsigned minimum_cycle = 0U;
    double minimum_voltage = 0.0;
    PwmPlan active, pending;
    forward_configure(0.1f, 6.0f, 0.5f);
    forward_initialize(0.0f);
    forward_U.mode = 1.0f;
    assert(PWM_MakePlan(0.0f, 0, &pending));
    for (unsigned n = 0; n < 60000U; ++n) {
        active = pending;
        forward_U.V_left = 12.0f;
        forward_U.V_right = (float)sample_v;
        forward_U.IL = (float)sample_il;
        forward_step();
        assert(PWM_MakePlan(forward_Y.D_cmd, forward_Y.BOOST_RUN, &pending));
        for (unsigned k = 0; k < substeps; ++k) {
            double phase = (k + 0.5) / substeps;
            int left = phase >= 0.03 && phase < 0.03 + active.left_high;
            int right = phase >= 0.03 && phase < 0.03 + active.right_high;
            il += dt * (12.0 * left - voltage * right - 0.05 * il) / 22e-6;
            voltage += dt * (right * il - voltage / 100.0) / 690e-6;
            if (il > peak) peak = il;
            /* Track/hold ends at .25 + 24.5/37.5MHz / period = .348. */
            if (k == 138U) {
                sample_il = il;
                if (sample_il > sampled_peak) sampled_peak = sample_il;
                if (sample_il < sampled_min) {
                    sampled_min = sample_il;
                    minimum_cycle = n;
                    minimum_voltage = voltage;
                }
            }
            /* VL rank 3 hold: .25 + (2*37+24.5)/250 = .644. */
            if (k == 257U) sample_v = voltage;
        }
    }
    printf("Bench ideal switching 12->6 V, 100 ohm, 0.5 A reference: Vout=%.3f V, peak IL=%.3f A, sampled peak=%.3f A\n",
           voltage, peak, sampled_peak);
    assert(forward_Y.EN_F_BUCK && !forward_Y.EN_F_BOOST);
    assert(fabs(voltage - 6.0) < 0.5);
    printf("Minimum sampled IL: %.3f A at cycle %u, voltage %.3f V\n",
           sampled_min, minimum_cycle, minimum_voltage);
    assert(sampled_min > -1.0);
    assert(sampled_peak < 1.5);
    return 0;
}
