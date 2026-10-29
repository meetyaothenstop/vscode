#include "power_menu.h"
#include "power_build_config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    PowerMenu menu;
    MenuFrame frame;
    MenuTelemetry data = {0};
    data.valid = 1U;
#if POWER_BENCH_24V_BUCK_TEST
    const unsigned expected_input = 24U, expected_output = 12U;
#else
    const unsigned expected_input = 12U, expected_output = 6U;
#endif
    Menu_Init(&menu);
    assert(menu.settings[MENU_FORWARD].input_volts == expected_input);
    assert(menu.settings[MENU_FORWARD].output_volts == expected_output);
    Menu_Render(&menu, &data, &frame);
#if POWER_BENCH_24V_BUCK_TEST
    assert(strstr(frame.rows[0], "24V BUCK DMA OVP14 C6"));
#elif POWER_BENCH_LIVE_OLED
    assert(strstr(frame.rows[0], "12V BUCK DMA OVP8"));
#elif POWER_BENCH_LATE_INPUT_ADC
    assert(strstr(frame.rows[0], "12V BUCK ADC2 OVP8"));
#elif defined(POWER_BENCH_FAST_BUCK) && POWER_BENCH_FAST_BUCK
    assert(strstr(frame.rows[0], "12V BUCK FAST OVP8"));
#elif defined(POWER_CCM_CONTROL) && POWER_CCM_CONTROL && !POWER_UNIFIED_DIAGNOSTICS
    assert(strstr(frame.rows[0], "12V CCM BASE"));
#elif defined(POWER_CCM_CONTROL) && POWER_CCM_CONTROL
    assert(strstr(frame.rows[0], "12V CCM DIAG"));
#elif defined(POWER_UNIFIED_DIAGNOSTICS) && POWER_UNIFIED_DIAGNOSTICS
    assert(strstr(frame.rows[0], "12V UNIFIED DIAG"));
#else
    assert(strstr(frame.rows[0], "SW"));
#endif
    (void)Menu_HandleKey(&menu, MENU_KEY_OK, &data);
    assert(Menu_Mode(&menu) == MENU_MODE_BUCK);
    Menu_Render(&menu, &data, &frame);
#if POWER_BENCH_24V_BUCK_TEST
    assert(strstr(frame.rows[0], "SW24-C6"));
    assert(strstr(frame.rows[2], "24 V"));
    assert(strstr(frame.rows[4], "12 V"));
#elif POWER_BENCH_LIVE_OLED
    assert(strstr(frame.rows[0], "SW-DMA8"));
#elif POWER_BENCH_LATE_INPUT_ADC
    assert(strstr(frame.rows[0], "SW-A2-8"));
#elif defined(POWER_BENCH_FAST_BUCK) && POWER_BENCH_FAST_BUCK
    assert(strstr(frame.rows[0], "SW-FAST"));
#elif defined(POWER_CCM_CONTROL) && POWER_CCM_CONTROL && !POWER_UNIFIED_DIAGNOSTICS
    assert(strstr(frame.rows[0], "SW-BASE"));
#elif defined(POWER_CCM_CONTROL) && POWER_CCM_CONTROL
    assert(strstr(frame.rows[0], "SW-CCM"));
#elif defined(POWER_UNIFIED_DIAGNOSTICS) && POWER_UNIFIED_DIAGNOSTICS
    assert(strstr(frame.rows[0], "SW-DIAG"));
#else
    assert(strstr(frame.rows[0], "SW"));
#endif
    (void)Menu_HandleKey(&menu, MENU_KEY_DOWN, &data);
    (void)Menu_HandleKey(&menu, MENU_KEY_DOWN, &data);
    MenuRequest request = Menu_HandleKey(&menu, MENU_KEY_OK, &data);
    assert(request.action == MENU_ACTION_START);
    assert(request.mode == MENU_MODE_BUCK && request.target_volts == (float)expected_output);
    data.state = MENU_RUNNING;
    request = Menu_HandleKey(&menu, MENU_KEY_BACK, &data);
    assert(request.action == MENU_ACTION_STOP);
    printf("Bench OLED identity, %u->%u default and menu start/stop: PASS\n",
           expected_input, expected_output);
    return 0;
}
