#include "reverse.h"
#include "pwm_plan.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void run(double source, double target, double load, unsigned mode)
{
    const unsigned substeps = 400;
    const double dt = 1.0 / (150000.0 * 400.0);
    double il = 0, voltage = 0, sampled_il = 0, sampled_v = 0;
    double peak = 0, sampled_peak = 0, opposite = 0;
    PwmPlan pending;
    int pending_off = 0;
    unsigned holding = 0, resume_cycles = 0, hold_count = 0;
    unsigned opposite_periods = 0, max_opposite_periods = 0;
    reverse_configure(mode == 4 ? (float)source : 0.1f, (float)target, 6.0f);
    reverse_initialize(0.0f);
    reverse_U.mode = (float)mode;
    assert(PWM_MakeReversePlan(0, 0, &pending));
    for (unsigned n = 0; n < 90000; ++n) {
        PwmPlan active = pending;
        int outputs_off = pending_off;
        int resuming = 0;
        if (!holding && !resume_cycles && reverse_Y.CTRL_RUN_R &&
            sampled_v >= target * 1.005) {
            holding = 1;
            ++hold_count;
        }
        if (holding && sampled_v < target * 0.99 && fabs(sampled_il) < 0.25) {
            holding = 0;
            resume_cycles = 32;
            reverse_prepare_resume((float)sampled_v);
            resuming = 1;
        }
        int paused = holding || resume_cycles;
        int stepping = !resume_cycles || resuming;
        if (resume_cycles) --resume_cycles;
        if (stepping) {
            reverse_U.V_right = (float)source;
            reverse_U.V_left = (float)sampled_v;
            reverse_U.IL = (float)sampled_il;
            if (holding) reverse_hold_step();
            else reverse_step();
            assert(reverse_errorStatus == 0);
            assert(PWM_MakeReversePlan(reverse_Y.D_cm_R, reverse_Y.BOOST_RUN_R, &pending));
            /* Stop immediately on handover; restart only after a boost compare
             * plan has crossed the same one-period preload boundary as firmware. */
            pending_off = mode == 4 && reverse_Y.D_cm_R4 && !reverse_Y.BOOST_RUN_R;
            if (pending_off) outputs_off = 1;
        }
        if (paused) outputs_off = 1;
        for (unsigned k = 0; k < substeps; ++k) {
            if (outputs_off) {
                double next = il < 0 ? fmin(0.0, il + dt * (voltage - 0.05 * il) / 22e-6) :
                                      fmax(0.0, il + dt * (-source - 0.05 * il) / 22e-6);
                double output_current = il < 0 ? -(il + next) * 0.5 : 0;
                voltage += dt * (output_current - voltage / load) / 690e-6;
                il = next;
            } else {
                double phase = (k + 0.5) / substeps;
                int left = phase >= 0.03 && phase < 0.03 + active.left_high;
                int right = phase >= 0.03 && phase < 0.03 + active.right_high;
                il += dt * (left * voltage - right * source - 0.05 * il) / 22e-6;
                voltage += dt * (-left * il - voltage / load) / 690e-6;
            }
            if (-il > peak) peak = -il;
            /* PA2/IL hold ends at .348 PWM periods. */
            if (k == 138) {
                sampled_il = il;
                if (-il > sampled_peak) sampled_peak = -il;
                if (il > opposite) opposite = il;
                opposite_periods = il > 1.0 ? opposite_periods + 1 : 0;
                if (opposite_periods > max_opposite_periods) max_opposite_periods = opposite_periods;
            }
            /* Left output voltage is injected rank2, hold ends at .496. */
            if (k == 198) sampled_v = voltage;
        }
    }
    printf("Reverse ideal switching %.0f->%.0f V R=%.0f: Vout=%.3f, peak=%.3f, sampled=%.3f, opposite=%.3f A (%u periods)\n",
           source, target, load, voltage, peak, sampled_peak, opposite, max_opposite_periods);
    fflush(stdout);
    assert(fabs(voltage - target) < 0.5);
    assert(peak < 8.0 && sampled_peak < 7.5 && max_opposite_periods < 16);
    if (load > 100000 && mode == 4) assert(hold_count > 0);
}

int main(void)
{
    run(12, 36, 36, 4);
    run(36, 12, 12, 3);
    run(24, 12, 12, 3);
    run(6, 12, 12, 4);
    run(24, 36, 36, 4);
    run(35, 36, 36, 4);
    run(6, 36, 72, 4);
    run(40, 1, 2, 3);
    run(12, 36, 1000000, 4);
    run(36, 12, 1000000, 3);
    run(12, 36, 1000, 4);
    return 0;
}
