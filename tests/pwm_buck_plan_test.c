#include "pwm_plan.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern int PWM_MakeBuckCompares(float command, uint32_t *left, uint32_t *right);

static void compare(float command)
{
    PwmPlan generic = {0.123f, 0.456f, 123U, 456U};
    PwmPlan specialized = generic;
    int expected = PWM_MakePlan(command, 0, &generic);
    assert(PWM_MakeBuckPlan(command, &specialized) == expected);
    assert(memcmp(&specialized.left_high, &generic.left_high, sizeof(float)) == 0);
    assert(memcmp(&specialized.right_high, &generic.right_high, sizeof(float)) == 0);
    assert(specialized.left_compare == generic.left_compare);
    assert(specialized.right_compare == generic.right_compare);
    uint32_t left = 123U, right = 456U;
    assert(PWM_MakeBuckCompares(command, &left, &right) == expected);
    assert(left == generic.left_compare && right == generic.right_compare);
}

int main(void)
{
    const float edge[] = {-INFINITY, -FLT_MAX, -1.0f, -0.0f, 0.0f,
        nextafterf(0.04f / PWM_REFRESH_HIGH, 0.0f), 0.04f / PWM_REFRESH_HIGH,
        nextafterf(0.04f / PWM_REFRESH_HIGH, 1.0f),
        nextafterf(0.95f, 0.0f), 0.95f, nextafterf(0.95f, 1.0f),
        1.0f, FLT_MAX, INFINITY, NAN};
    for (unsigned n = 0; n < sizeof(edge) / sizeof(edge[0]); ++n) compare(edge[n]);
    for (unsigned n = 0; n <= 100000U; ++n) compare(n / 100000.0f);
    uint32_t bits = 107U;
    for (unsigned n = 0; n < 100000U; ++n) {
        bits = bits * 1664525U + 1013904223U;
        float value;
        memcpy(&value, &bits, sizeof(value));
        compare(value);
    }
    assert(!PWM_MakeBuckPlan(0.5f, NULL));
    uint32_t unchanged = 777U;
    assert(!PWM_MakeBuckCompares(0.5f, NULL, &unchanged) && unchanged == 777U);
    assert(!PWM_MakeBuckCompares(0.5f, &unchanged, NULL) && unchanged == 777U);
    puts("Buck specialized/generic mapper: 200k+ commands, null/min-pulse/upper boundary, NaN/inf, unchanged invalid outputs: PASS");
    return 0;
}
