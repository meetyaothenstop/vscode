#include "reverse.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern reverse_ExternalInputs reference_reverse_U;
extern reverse_ExternalOutputs reference_reverse_Y;
extern reverse_BlockOutputs reference_reverse_B;
extern reverse_ModelStates reference_reverse_X;
extern const char *reference_reverse_errorStatus;
extern void reference_reverse_configure(float, float, float);
extern void reference_reverse_initialize(float);
extern void reference_reverse_step(void);
extern void reference_reverse_hold_step(void);
extern void reference_reverse_prepare_resume(float);

static void same(float actual, float expected)
{
    assert(isfinite(actual) && isfinite(expected));
    assert(memcmp(&actual, &expected, sizeof(actual)) == 0);
}
#define B(field) same(reverse_B.field, reference_reverse_B.field)
#define X(field) same(reverse_X.field, reference_reverse_X.field)

static void compare(unsigned mode)
{
    assert(!reverse_errorStatus && !reference_reverse_errorStatus);
    assert(memcmp(&reverse_Y, &reference_reverse_Y, sizeof(reverse_Y)) == 0);
    if (mode == 4U) {
        assert(memcmp(&reverse_B, &reference_reverse_B, sizeof(reverse_B)) == 0);
        assert(memcmp(&reverse_X, &reference_reverse_X, sizeof(reverse_X)) == 0);
        return;
    }
    B(Integrator); B(DiscreteIntegrator3); B(Saturation4); B(Saturation1);
    B(Product3); B(Saturation); B(Gain2); B(DiscreteIntegrator1);
    B(Sum3); B(Saturation2);
    X(Integrator_x); X(DiscreteTransferFcn);
    X(DiscreteIntegrator3_i1_x); X(DiscreteIntegrator3_i2_prevU);
    X(DiscreteIntegrator1_i1_x); X(DiscreteIntegrator1_i2_prevU);
    assert(reverse_X.DiscreteIntegrator3_first == reference_reverse_X.DiscreteIntegrator3_first);
    assert(reverse_X.DiscreteIntegrator1_first == reference_reverse_X.DiscreteIntegrator1_first);
    assert(reverse_X.DiscreteIntegrator1_i3_prevReset == reference_reverse_X.DiscreteIntegrator1_i3_prevReset);
}

static void initialize_pair(unsigned mode, float source, float target)
{
    float start = mode == 4U ? source : 0.1f;
    reverse_configure(start, target, 6.0f);
    reference_reverse_configure(start, target, 6.0f);
    reverse_initialize(0.0f);
    reference_reverse_initialize(0.0f);
    reverse_U = (reverse_ExternalInputs){.V_right=source, .mode=(float)mode};
    reference_reverse_U = reverse_U;
}

int main(void)
{
    /* Catches dispatch to the old full step rather than omitting Boost work. */
    initialize_pair(3U, 12.0f, 6.0f);
    reverse_step();
    assert(reverse_B.Gain3 == 0.0f && reverse_X.DiscreteTransferFcn2 == 0.0f);
    assert(reverse_X.DiscreteIntegrator4_first == -1 && reverse_X.DiscreteIntegrator5_first == -1);
    uint32_t random = 367U;
    unsigned samples = 0U;
    const unsigned modes[] = {3U, 4U, 3U, 4U, 3U};
    const float sources[] = {12.0f, 12.0f, 24.0f, 24.0f, 40.0f};
    const float targets[] = {6.0f, 36.0f, 12.0f, 36.0f, 36.0f};
    for (unsigned run = 0U; run < 5U; ++run) {
        for (unsigned randomized = 0U; randomized < 2U; ++randomized) {
            initialize_pair(modes[run], sources[run], targets[run]);
            for (unsigned step = 0U; step < 30000U; ++step) {
                random = random * 1664525U + 1013904223U;
                reverse_U.V_left = randomized ? (random % 45001U) * 0.001f :
                    (step < 10000U ? 0.0f : targets[run]);
                reverse_U.IL = randomized ? ((random >> 16) % 15001U) * 0.001f - 7.5f : -0.06f;
                reference_reverse_U = reverse_U;
                if (step == 20000U) {
                    reverse_prepare_resume(reverse_U.V_left);
                    reference_reverse_prepare_resume(reverse_U.V_left);
                }
                if (step >= 18000U && step < 20000U) {
                    reverse_hold_step(); reference_reverse_hold_step();
                } else { reverse_step(); reference_reverse_step(); }
                compare(modes[run]);
                ++samples;
            }
        }
    }
    puts("MAIN reverse Buck omits Boost-only states; full Boost, reinitialization, HOLD/resume and300000 bitwise samples: PASS");
    assert(samples == 300000U);
    return 0;
}
