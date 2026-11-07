/* Exercise the real backend; register events are supplied by the shared fixture. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main

static void run_forward_boost(float target)
{
    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(2U, target));
    for (unsigned n = 0U; n < 33U; ++n) period(12.0f, 0.0f, 0.0f);
    CHECK(power_status.state == POWER_RUNNING && enabled_outputs == ALL_OUTPUTS);
}

int main(void)
{
    cold_boot();
    /* Catch applying 3 V only to the request, not to fresh cold-start samples. */
    for (unsigned sign = 0U; sign < 2U; ++sign) {
        float current = sign ? -0.98f : 0.98f;
        stopped_samples(12.0f, 2.99f, current);
        CHECK(request(2U, 18.0f));
        for (unsigned n = 0U; n < 33U; ++n) {
            period(12.0f, 2.99f, current);
            CHECK(power_status.fault == 0U);
        }
        CHECK(power_status.state == POWER_RUNNING);
    }
    stopped_samples(12.0f, 3.05f, 0.0f);
    CHECK(!request(2U, 18.0f) && power_status.fault_detail == 6U);
#if !POWER_RAW_FAST_IRQ
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_right = 3.0f;
    CHECK(request(2U, 18.0f));
    stopped_samples(12.0f, 0.0f, 0.0f);
    power_status.v_right = 3.001f;
    CHECK(!request(2U, 18.0f) && power_status.fault_detail == 6U);
#endif
    const unsigned stages[] = {0U, 1U, 30U, 31U, 32U};
    for (unsigned n = 0U; n < sizeof(stages) / sizeof(stages[0]); ++n) {
        stopped_samples(12.0f, 0.0f, 0.0f);
        CHECK(request(2U, 18.0f));
        for (unsigned k = 0U; k < stages[n]; ++k) period(12.0f, 0.0f, 0.0f);
        period(12.0f, 3.05f, 0.0f);
        CHECK(power_status.fault == POWER_FAULT_START && power_status.fault_detail == 9U);
        CHECK(enabled_outputs == 0U && HRTIM1->sCommonRegs.OENR == 0U);
        CHECK(fabsf(power_status.fault_v_right - 3.05f) < 0.02f);
    }
    /* Accept the new input window, but never turn Boost into Buck at startup. */
    const struct { float input, target; unsigned detail; } inputs[] = {
        {9.9f, 18.0f, 4U}, {10.1f, 18.0f, 0U}, {35.0f, 36.0f, 0U},
        {41.0f, 36.0f, 7U}, {42.1f, 36.0f, 4U},
        {12.0f, 11.9f, 7U}, {12.0f, 6.0f, 7U}, {12.0f, 0.0f, 3U}
    };
    for (unsigned n = 0U; n < sizeof(inputs) / sizeof(inputs[0]); ++n) {
        stopped_samples(inputs[n].input, 0.0f, 0.0f);
        int accepted = request(2U, inputs[n].target);
        if (accepted != (inputs[n].detail == 0U))
            fprintf(stderr, "Boost input case%u: Vin=%g target=%g accepted=%d detail=%lu\n",
                    n, inputs[n].input, inputs[n].target, accepted,
                    (unsigned long)power_status.fault_detail);
        CHECK(accepted == (inputs[n].detail == 0U));
        if (inputs[n].detail) CHECK(power_status.fault_detail == inputs[n].detail);
    }
    stopped_samples(12.0f, 0.0f, 0.0f);
    /* Equality uses the represented ADC input, not an unquantized nominal12 V. */
    float equal_input = (float)ADC1->JDR2 * ADC_LEFT_VOLTS_PER_COUNT;
    CHECK(!request(2U, equal_input) && power_status.fault_detail == 7U);
    /* Synthetic ADC events check the cached runtime window, not a physical
     * claim that a Boost can regulate below its instantaneous input. */
    run_forward_boost(36.0f);
    period(41.9f, 0.0f, 0.0f);
    CHECK(power_status.fault == 0U);
    period(42.1f, 0.0f, 0.0f);
    CHECK((power_status.fault & POWER_FAULT_INPUT) != 0U);
    run_forward_boost(18.0f);
    period(9.9f, 0.0f, 0.0f);
    CHECK((power_status.fault & POWER_FAULT_INPUT) != 0U);
    const struct { float target, trip; } ovp[] = {
        {18.0f, 27.0f}, {26.0f, 39.0f}, {27.0f, 40.0f}, {36.0f, 40.0f}
    };
    for (unsigned n = 0U; n < sizeof(ovp) / sizeof(ovp[0]); ++n) {
        run_forward_boost(ovp[n].target);
        const unsigned below = (unsigned)floorf(ovp[n].trip / ADC_RIGHT_VOLTS_PER_COUNT);
        period(12.0f, below * ADC_RIGHT_VOLTS_PER_COUNT, 0.0f);
        CHECK(power_status.fault == 0U);
        period(12.0f, (below + 1U) * ADC_RIGHT_VOLTS_PER_COUNT, 0.0f);
        CHECK(power_status.fault == POWER_FAULT_OUTPUT && power_status.fault_raw_vl == below + 1U);
    }
    /* Reverse Boost retains its independent original input/residual/OVP policy. */
    stopped_samples(0.0f, 6.1f, 0.0f);
    CHECK(request(4U, 18.0f));
    stopped_samples(0.0f, 41.0f, 0.0f);
    CHECK(!request(4U, 36.0f) && power_status.fault_detail == 4U);
    stopped_samples(0.3f, 12.0f, 0.0f);
    CHECK(!request(4U, 18.0f) && power_status.fault_detail == 6U);
    stopped_samples(0.0f, 12.0f, 0.0f);
    CHECK(request(4U, 18.0f));
    for (unsigned n = 0U; n < 33U; ++n) period(0.0f, 12.0f, 0.0f);
    period(19.7f, 12.0f, 0.0f);
    CHECK(power_status.fault == 0U);
    period(19.9f, 12.0f, 0.0f);
    CHECK(power_status.fault == POWER_FAULT_OUTPUT);
    puts("Forward Boost shares Buck electrical limits; cold3V, input10..42V, OVP1.5x/40V and reverse isolation: PASS");
    return 0;
}
