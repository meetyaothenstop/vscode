#include "forward.h"
#include "power_build_config.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern void forward_main_boost_step(void);
extern forward_ExternalInputs boost_reference_forward_U;
extern forward_ExternalOutputs boost_reference_forward_Y;
extern forward_BlockOutputs boost_reference_forward_B;
extern forward_ModelStates boost_reference_forward_X;
extern const char *boost_reference_forward_errorStatus;
extern void boost_reference_forward_configure(float, float, float);
extern void boost_reference_forward_initialize(float);
extern void boost_reference_forward_step(void);
#if POWER_MULTIRATE_CONTROL
extern unsigned forward_voltage_updates, boost_reference_forward_voltage_updates;
#endif

static unsigned samples;
static void compare(void)
{
    assert(!forward_errorStatus && !boost_reference_forward_errorStatus);
    /* Both initialize every byte, including padding, before the trajectory. */
    assert(memcmp(&forward_Y, &boost_reference_forward_Y, sizeof(forward_Y)) == 0);
    assert(memcmp(&forward_B, &boost_reference_forward_B, sizeof(forward_B)) == 0);
    assert(memcmp(&forward_X, &boost_reference_forward_X, sizeof(forward_X)) == 0);
#if POWER_MULTIRATE_CONTROL
    assert(forward_voltage_updates == boost_reference_forward_voltage_updates);
#endif
    ++samples;
}
static void initialize(float input, float target)
{
    forward_configure(input, target, 6.0f);
    boost_reference_forward_configure(input, target, 6.0f);
    forward_initialize(0.0f);
    boost_reference_forward_initialize(0.0f);
}
int main(void)
{
    uint32_t random = 0x2E51U;
    const float inputs[] = {10.1f, 12.0f, 12.0f, 24.0f, 35.0f, 12.0f};
    const float targets[] = {18.0f, 18.0f, 36.0f, 36.0f, 36.0f, 27.0f};
    for (unsigned run = 0U; run < 12U; ++run) {
        float input = inputs[run % 6U], target = targets[run % 6U];
        /* Dirty prior Buck state must not leak through STOP/reinitialization. */
        forward_configure(0.1f, 6.0f, 6.0f);
        forward_initialize(0.0f);
        forward_U = (forward_ExternalInputs){0.0f, 12.0f, 0.0f, 1.0f};
        forward_step();
        initialize(input, target);
        unsigned saw_precharge = 0U, saw_boost = 0U;
        for (unsigned step = 0U; step < 30000U; ++step) {
            random = random * 1664525U + 1013904223U;
            float output = step < 100U ? 0.0f :
                (step < 600U ? input * (float)(step - 100U) / 500.0f : target);
            if (run >= 6U && step >= 600U)
                output = (float)(random % 45001U) * 0.001f;
            float current = run >= 6U ? (float)((random >> 16) % 8401U) * 0.001f - 0.98f : 0.06f;
            forward_U = (forward_ExternalInputs){current, input, output, 2.0f};
            boost_reference_forward_U = forward_U;
            forward_main_boost_step();
            boost_reference_forward_step();
            compare();
            saw_precharge |= forward_Y.PRECCHARGE_RUN;
            saw_boost |= forward_Y.BOOST_RUN;
        }
        assert(saw_precharge && saw_boost);
    }
    /* The protected entry cannot silently operate on another mode. */
    const float rejected[] = {0.0f, 1.0f, 3.0f, 4.0f, NAN};
    for (unsigned n = 0U; n < sizeof(rejected) / sizeof(rejected[0]); ++n) {
        initialize(12.0f, 18.0f);
        forward_U = (forward_ExternalInputs){0.0f, 12.0f, 18.0f, 2.0f};
        forward_main_boost_step();
        forward_U.mode = rejected[n];
        forward_main_boost_step();
        assert(forward_errorStatus && forward_Y.D_cmd == 0.0f);
        assert(!forward_Y.EN_F_BOOST && !forward_Y.EN_F_BUCK && !forward_Y.CTRL_RUN);
        assert(!forward_Y.PRECHARGE_DONE && !forward_Y.PRECCHARGE_RUN && !forward_Y.BOOST_RUN);
        forward_U.mode = 2.0f;
        forward_main_boost_step();
        assert(forward_errorStatus && forward_Y.D_cmd == 0.0f);
    }
    printf("Protected Boost vs independent307bcdd source: %u complete B/X/Y bitwise steps; precharge/handover/PI/random/reinit and mode rejection PASS\n", samples);
    return 0;
}
