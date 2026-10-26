#include "forward.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern forward_ExternalInputs reference_forward_U;
extern forward_ExternalOutputs reference_forward_Y;
extern forward_BlockOutputs reference_forward_B;
extern forward_ModelStates reference_forward_X;
extern void reference_forward_configure(float, float, float);
extern void reference_forward_initialize(float);
extern void reference_forward_step(void);

/* Catch changed PI coefficients, update order, ramp, clamps or active states.
 * The independently compiled general controller is the regression oracle. */
static void same(float actual, float expected)
{
    assert(isfinite(actual) && isfinite(expected));
    assert(memcmp(&actual, &expected, sizeof(actual)) == 0);
}
#define B(field) same(forward_B.field, reference_forward_B.field)
#define X(field) same(forward_X.field, reference_forward_X.field)
static void compare(void)
{
    same(forward_Y.D_cmd, reference_forward_Y.D_cmd);
    assert(forward_Y.PRECHARGE_DONE == reference_forward_Y.PRECHARGE_DONE);
    assert(forward_Y.EN_F_BUCK == reference_forward_Y.EN_F_BUCK);
    assert(forward_Y.EN_F_BOOST == reference_forward_Y.EN_F_BOOST);
    assert(forward_Y.PRECCHARGE_RUN == reference_forward_Y.PRECCHARGE_RUN);
    assert(forward_Y.BOOST_RUN == reference_forward_Y.BOOST_RUN);
    assert(forward_Y.CTRL_RUN == reference_forward_Y.CTRL_RUN);
    B(DiscreteIntegrator3); B(Saturation4); B(Saturation3);
    B(DiscreteIntegrator); B(Saturation); B(DiscreteIntegrator1);
    B(Saturation2); B(Product2); B(Sum3);
    X(DiscreteTransferFcn);
    X(DiscreteIntegrator3_i1_x); X(DiscreteIntegrator3_i2_prevU);
    X(DiscreteIntegrator_i1_x); X(DiscreteIntegrator_i2_prevU);
    X(DiscreteIntegrator1_i1_x); X(DiscreteIntegrator1_i2_prevU);
    assert(forward_X.DiscreteIntegrator3_first == reference_forward_X.DiscreteIntegrator3_first);
    assert(forward_X.DiscreteIntegrator_first == reference_forward_X.DiscreteIntegrator_first);
    assert(forward_X.DiscreteIntegrator1_first == reference_forward_X.DiscreteIntegrator1_first);
    assert(forward_X.DiscreteIntegrator_i3_prevReset == reference_forward_X.DiscreteIntegrator_i3_prevReset);
    assert(forward_X.DiscreteIntegrator1_i3_prevReset == reference_forward_X.DiscreteIntegrator1_i3_prevReset);
}
int main(void)
{
    uint32_t random = 107U;
    const float targets[] = {1.0f, 3.0f, 6.0f};
    const float inputs[] = {10.5f, 12.0f, 13.5f};
    unsigned samples = 0;
    for (unsigned run = 0; run < 18; ++run) {
        float target = targets[run % 3];
        forward_configure(0.1f, target, 0.5f);
        reference_forward_configure(0.1f, target, 0.5f);
        forward_initialize(0.0f);
        reference_forward_initialize(0.0f);
        for (unsigned step = 0; step < 30000; ++step) {
            random = random * 1664525U + 1013904223U;
            forward_U.mode = 1.0f;
            forward_U.V_left = inputs[(run / 3) % 3];
            forward_U.V_right = run < 9 ? (step < 10000 ? 0.0f : target) :
                (float)(random % 14001U) * 0.001f - 0.1f;
            forward_U.IL = run < 9 ? 0.06f :
                (float)((random >> 16) % 3001U) * 0.001f - 1.5f;
            reference_forward_U = forward_U;
            forward_step(); reference_forward_step(); compare(); ++samples;
        }
    }
    printf("Buck reference bitwise trajectory match: %u samples\n", samples);
    /* Catch accidental acceptance of Boost, reverse, invalid and NaN modes.
     * Rejection must clear a previously nonzero command, and remain latched. */
    const float invalid_modes[] = {0.0f, 2.0f, 3.0f, 4.0f, NAN};
    for (unsigned i = 0; i < sizeof(invalid_modes)/sizeof(invalid_modes[0]); ++i) {
        forward_initialize(0.0f);
        forward_U = (forward_ExternalInputs){0.0f, 12.0f, 0.0f, 1.0f};
        forward_step(); assert(forward_Y.D_cmd > 0.0f);
        forward_U.mode = invalid_modes[i]; forward_step();
        assert(forward_errorStatus != NULL);
        assert(forward_Y.D_cmd == 0.0f && !forward_Y.CTRL_RUN);
        assert(!forward_Y.EN_F_BUCK && !forward_Y.EN_F_BOOST);
        assert(!forward_Y.PRECCHARGE_RUN && !forward_Y.BOOST_RUN);
        forward_U.mode = 1.0f; forward_step();
        assert(forward_errorStatus != NULL && forward_Y.D_cmd == 0.0f);
    }
    puts("Restricted Buck mode rejection and restart: PASS");
    return 0;
}
