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
extern const char *reference_forward_errorStatus;
extern void reference_forward_configure(float, float, float);
extern void reference_forward_initialize(float);
extern void reference_forward_step(void);

static void same(float actual, float expected)
{
    assert(isfinite(actual) && isfinite(expected));
    assert(memcmp(&actual, &expected, sizeof(actual)) == 0);
}

#define B(field) same(forward_B.field, reference_forward_B.field)
#define X(field) same(forward_X.field, reference_forward_X.field)
#define BX(field) assert(forward_X.field == reference_forward_X.field)
#define BB(field) assert(forward_B.field == reference_forward_B.field)

/* These are the externally used outputs and the PI/ramp states retained by
 * both steps. The independently compiled full controller is the oracle. */
static void compare_common(void)
{
    assert(forward_errorStatus == NULL && reference_forward_errorStatus == NULL);
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
    BB(EN_F_BUCK1); BB(EN_F_BOOST1);
    BB(LogicalOperator); BB(LogicalOperator15);
    BB(LogicalOperator17); BB(LogicalOperator18);
    X(DiscreteTransferFcn);
    X(DiscreteIntegrator3_i1_x); X(DiscreteIntegrator3_i2_prevU);
    X(DiscreteIntegrator_i1_x); X(DiscreteIntegrator_i2_prevU);
    X(DiscreteIntegrator1_i1_x); X(DiscreteIntegrator1_i2_prevU);
    BX(DiscreteIntegrator3_first); BX(DiscreteIntegrator_first);
    BX(DiscreteIntegrator1_first); BX(DiscreteIntegrator_i3_prevReset);
    BX(DiscreteIntegrator1_i3_prevReset);
}

static void compare_full(void)
{
    compare_common();
    BB(PRECHARGE_ACHED); BB(Memory);
    B(Saturation1); B(DiscreteIntegrator4); B(Saturation6);
    B(DiscreteIntegrator2); B(Saturation5); B(Gain2);
    B(PulseDelay); B(DiscreteTransferFcn1);
    BX(Memory); X(DiscreteTransferFcn1);
    BX(DiscreteIntegrator4_first);
    X(DiscreteIntegrator4_i1_x); X(DiscreteIntegrator4_i2_prevU);
    BX(DiscreteIntegrator2_first);
    X(DiscreteIntegrator2_i1_x); X(DiscreteIntegrator2_i2_prevU);
}

static void initialize_pair(unsigned mode, float input, float target)
{
    const float start = mode == 2U ? input : 0.1f;
    forward_configure(start, target, 6.0f);
    reference_forward_configure(start, target, 6.0f);
    forward_initialize(0.0f);
    reference_forward_initialize(0.0f);
    forward_U = (forward_ExternalInputs){0.0f, input, 0.0f, (float)mode};
    reference_forward_U = forward_U;
}

/* A mode-1 dispatch to the old full step would update this precharge gain
 * and its filter/ramp. The optimized step must omit all Boost-only work. */
static void buck_skips_boost_state(void)
{
    initialize_pair(1U, 24.0f, 12.0f);
    forward_step();
    assert(forward_B.Gain2 == 0.0f);
    assert(forward_X.DiscreteTransferFcn1 == 0.0f);
    assert(forward_X.DiscreteIntegrator4_first == -1);
    assert(forward_X.DiscreteIntegrator2_first == -1);
    assert(forward_Y.EN_F_BUCK && !forward_Y.EN_F_BOOST);
}

static unsigned compare_trajectory(unsigned mode, float input, float target,
                                   int randomized, uint32_t *random)
{
    initialize_pair(mode, input, target);
    for (unsigned step = 0; step < 30000U; ++step) {
        *random = *random * 1664525U + 1013904223U;
        forward_U.V_right = randomized ?
            (float)(*random % 45001U) * 0.001f :
            (step < 10000U ? 0.0f : target);
        forward_U.IL = randomized ?
            (float)((*random >> 16) % 15001U) * 0.001f - 7.5f : 0.06f;
        reference_forward_U = forward_U;
        forward_step();
        reference_forward_step();
        if (mode == 1U) compare_common();
        else compare_full();
    }
    return 30000U;
}

/* Mode changes are supported through the backend's STOP/reinitialize path.
 * Exercise both directions after the previous mode has populated its state. */
static void reinitialize_between_modes(void)
{
    const unsigned modes[] = {1U, 2U, 1U, 2U, 1U};
    const float inputs[] = {12.0f, 12.0f, 40.0f, 24.0f, 24.0f};
    const float targets[] = {6.0f, 36.0f, 36.0f, 36.0f, 12.0f};
    for (unsigned run = 0; run < sizeof(modes) / sizeof(modes[0]); ++run) {
        initialize_pair(modes[run], inputs[run], targets[run]);
        for (unsigned step = 0; step < 100U; ++step) {
            forward_U.V_right = step < 50U ? 0.0f : targets[run];
            reference_forward_U = forward_U;
            forward_step();
            reference_forward_step();
            if (modes[run] == 1U) {
                compare_common();
                assert(forward_B.Gain2 == 0.0f);
                assert(forward_X.DiscreteTransferFcn1 == 0.0f);
            } else {
                compare_full();
                assert(forward_B.Gain2 > 0.0f && forward_Y.EN_F_BOOST);
            }
        }
    }
}

int main(void)
{
    uint32_t random = 107U;
    unsigned buck_samples = 0U, boost_samples = 0U;
    const float inputs[] = {12.0f, 24.0f, 40.0f};
    const float targets[] = {1.0f, 6.0f, 12.0f, 36.0f};
    buck_skips_boost_state();
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
        for (unsigned t = 0; t < sizeof(targets) / sizeof(targets[0]); ++t) {
            for (int randomized = 0; randomized <= 1; ++randomized) {
                if (targets[t] <= 0.95f * inputs[i])
                    buck_samples += compare_trajectory(1U, inputs[i], targets[t],
                                                       randomized, &random);
                else if (targets[t] > inputs[i])
                    boost_samples += compare_trajectory(2U, inputs[i], targets[t],
                                                        randomized, &random);
            }
        }
    }
    reinitialize_between_modes();
    printf("Main 6A Buck/full reference bitwise trajectories: %u/%u samples\n",
           buck_samples, boost_samples);
    puts("Main Buck omits Boost state; Boost and reinitialized mode switches: PASS");
    return 0;
}
