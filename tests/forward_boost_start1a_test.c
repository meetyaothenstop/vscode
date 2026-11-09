/* Real backend and ADC conversion; only register events use the existing fixture. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main

int main(void)
{
    cold_boot();
    /* Catch changing only request acceptance while cold IRQ checks stay at 0.25 A. */
    const float accepted[] = {-0.98f, -0.3f, 0.3f, 0.98f};
    for (unsigned n = 0U; n < sizeof(accepted) / sizeof(accepted[0]); ++n) {
        stopped_samples(12.0f, 0.0f, accepted[n]);
        CHECK(request(2U, 18.0f));
        for (unsigned cycle = 0U; cycle < 33U; ++cycle) {
            period(12.0f, 0.0f, accepted[n]);
            CHECK(power_status.fault == 0U);
        }
        CHECK(power_status.state == POWER_RUNNING && enabled_outputs == ALL_OUTPUTS);
    }
    const float rejected[] = {-1.02f, 1.02f};
    for (unsigned n = 0U; n < 2U; ++n) {
        stopped_samples(12.0f, 0.0f, rejected[n]);
        CHECK(!request(2U, 18.0f));
        CHECK(power_status.fault == POWER_FAULT_START && power_status.fault_detail == 5U);
    }
#if !POWER_RAW_FAST_IRQ
    /* Exact comparison independently of ADC quantization: equality is rejected. */
    for (unsigned sign = 0U; sign < 2U; ++sign) {
        stopped_samples(12.0f, 0.0f, 0.0f);
        power_status.il = sign ? -1.0f : 1.0f;
        CHECK(!request(2U, 18.0f) && power_status.fault_detail == 5U);
    }
#endif
    /* Recheck before low-side enable, in bootstrap, and at both arming boundaries. */
    const unsigned stages[] = {0U, 1U, 31U, 32U};
    for (unsigned n = 0U; n < sizeof(stages) / sizeof(stages[0]); ++n) {
        stopped_samples(12.0f, 0.0f, 0.0f);
        CHECK(request(2U, 18.0f));
        for (unsigned cycle = 0U; cycle < stages[n]; ++cycle)
            period(12.0f, 0.0f, 0.0f);
        period(12.0f, 0.0f, 1.02f);
        CHECK(power_status.fault == POWER_FAULT_START && power_status.fault_detail == 8U);
        CHECK(enabled_outputs == 0U && HRTIM1->sCommonRegs.OENR == 0U);
        CHECK(fabsf(power_status.fault_il - 1.02f) < 0.02f);
    }
    /* Reverse Boost now shares1A while retaining strict rejection at the limit. */
    for (unsigned sign = 0U; sign < 2U; ++sign) {
        stopped_samples(0.0f, 12.0f, sign ? -0.3f : 0.3f);
        CHECK(request(4U, 18.0f));
    }
    stopped_samples(0.0f, 12.0f, -0.2f);
    CHECK(request(4U, 18.0f));
    period(0.0f, 12.0f, -1.02f);
    CHECK(power_status.fault == POWER_FAULT_START && power_status.fault_detail == 8U);
    /* Output residual, sampled overcurrent, and wrong-direction protection remain. */
    stopped_samples(12.0f, 3.05f, 0.0f);
    CHECK(!request(2U, 18.0f) && power_status.fault_detail == 6U);
    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(2U, 18.0f));
    period(12.0f, 3.05f, 0.0f);
    CHECK(power_status.fault == POWER_FAULT_START && power_status.fault_detail == 9U);
    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(2U, 18.0f));
    for (unsigned n = 0U; n < 33U; ++n) period(12.0f, 0.0f, 0.0f);
    period(12.0f, 0.0f, 7.6f);
    CHECK((power_status.fault & POWER_FAULT_CURRENT) != 0U && enabled_outputs == 0U);
    stopped_samples(12.0f, 0.0f, 0.0f);
    CHECK(request(2U, 18.0f));
    for (unsigned n = 0U; n < 33U; ++n) period(12.0f, 0.0f, 0.0f);
    period(12.0f, 0.0f, -1.02f);
    CHECK((power_status.fault & POWER_FAULT_REVERSE) != 0U && enabled_outputs == 0U);
    puts("Both Boost directions use startup below1A; sampled current and direction trips retained: PASS");
    return 0;
}
