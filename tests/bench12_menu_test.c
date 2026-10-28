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
    Menu_Init(&menu);
    assert(menu.settings[MENU_FORWARD].input_volts == 12U);
    assert(menu.settings[MENU_FORWARD].output_volts == 6U);
    Menu_Render(&menu, &data, &frame);
#if POWER_BENCH_LATE_INPUT_ADC
    assert(strstr(frame.rows[0], "12V BUCK ADC2 OVP7"));
#elif defined(POWER_BENCH_FAST_BUCK) && POWER_BENCH_FAST_BUCK
    assert(strstr(frame.rows[0], "12V BUCK FAST OVP7"));
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
#if POWER_BENCH_LATE_INPUT_ADC
    assert(strstr(frame.rows[0], "SW-A2-7"));
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
    assert(request.mode == MENU_MODE_BUCK && request.target_volts == 6.0f);
    data.state = MENU_RUNNING;
    request = Menu_HandleKey(&menu, MENU_KEY_BACK, &data);
    assert(request.action == MENU_ACTION_STOP);
    puts("Bench OLED identity, 12->6 default and menu start/stop: PASS");
    return 0;
}
