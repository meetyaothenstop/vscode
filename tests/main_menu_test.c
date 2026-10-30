#include "power_menu.h"
#include "power_build_config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#if !POWER_MAIN_APPLICATION
#error Main menu tests require the main application profile.
#endif

static MenuTelemetry data;

static MenuRequest press(PowerMenu *menu, MenuKey key)
{
    return Menu_HandleKey(menu, key, &data);
}

static void setup(PowerMenu *menu, MenuDirection direction)
{
    memset(&data, 0, sizeof(data));
    data.valid = 1U;
    Menu_Init(menu);
    if (direction == MENU_REVERSE) press(menu, MENU_KEY_DOWN);
    press(menu, MENU_KEY_OK);
}

static void edit(PowerMenu *menu, int steps)
{
    press(menu, MENU_KEY_OK);
    assert(menu->editing);
    for (int i = 0; i < (steps < 0 ? -steps : steps); ++i)
        press(menu, steps < 0 ? MENU_KEY_DOWN : MENU_KEY_UP);
    press(menu, MENU_KEY_OK);
    assert(!menu->editing);
}

/* Catches retaining legacy 6 V/1 V edit floors in either direction. */
static void test_bounds_and_whole_volt_steps(void)
{
    for (MenuDirection direction = MENU_FORWARD; direction <= MENU_REVERSE; ++direction) {
        PowerMenu menu;
        setup(&menu, direction);
        assert(menu.settings[MENU_FORWARD].input_volts == 24U);
        assert(menu.settings[MENU_FORWARD].output_volts == 12U);
        assert(menu.settings[MENU_REVERSE].input_volts == 36U);
        assert(menu.settings[MENU_REVERSE].output_volts == 12U);
        edit(&menu, -100);
        assert(menu.settings[direction].input_volts == 12U);
        edit(&menu, 1);
        assert(menu.settings[direction].input_volts == 13U);
        edit(&menu, 100);
        assert(menu.settings[direction].input_volts == 40U);
        edit(&menu, -1);
        assert(menu.settings[direction].input_volts == 39U);
        press(&menu, MENU_KEY_DOWN);
        edit(&menu, -100);
        assert(menu.settings[direction].output_volts == 0U);
        edit(&menu, 1);
        assert(menu.settings[direction].output_volts == 1U);
        edit(&menu, 100);
        assert(menu.settings[direction].output_volts == 36U);
        edit(&menu, -1);
        assert(menu.settings[direction].output_volts == 35U);
    }
}

/* Catches a bench identity or truncated SW-MAIN marker in the main image. */
static void test_identity_and_full_width_headers(void)
{
    PowerMenu menu;
    MenuFrame frame;
    setup(&menu, MENU_FORWARD);
    press(&menu, MENU_KEY_BACK);
    Menu_Render(&menu, &data, &frame);
#if POWER_UNIFIED_DIAGNOSTICS
    assert(strstr(frame.rows[0], POWER_MAIN_ADC_SOFTWARE_ONLY ?
                  "MAIN ADC DIAG 6A" : "MAIN HW DIAG 6A"));
#elif POWER_MAIN_ADC_SOFTWARE_ONLY
    assert(strstr(frame.rows[0], "MAIN ADC OPT2 6A"));
#else
    assert(strstr(frame.rows[0], "MAIN HW GUARD 6A"));
#endif
    press(&menu, MENU_KEY_OK);
    Menu_Render(&menu, &data, &frame);
    assert(strstr(frame.rows[0], POWER_UNIFIED_DIAGNOSTICS ? "FORWARD BUCK M-DIAG" : "FORWARD BUCK M-OPT2"));
    menu.settings[MENU_FORWARD] = (MenuSettings){12U, 36U};
    Menu_Render(&menu, &data, &frame);
    assert(strstr(frame.rows[0], POWER_UNIFIED_DIAGNOSTICS ? "FORWARD BOOST M-DIAG" : "FORWARD BOOST M-OPT2"));
    menu.direction = MENU_REVERSE;
    menu.settings[MENU_REVERSE] = (MenuSettings){36U, 36U};
    Menu_Render(&menu, &data, &frame);
    assert(strstr(frame.rows[0], POWER_UNIFIED_DIAGNOSTICS ? "REVERSE EQUAL M-DIAG" : "REVERSE EQUAL M-OPT2"));
    for (unsigned row = 0; row < MENU_ROWS; ++row)
        assert(strlen(frame.rows[row]) == MENU_COLUMNS);
}

/* Catches collapsing any direction to Buck, treating equal as bypass, or
 * consuming zero locally instead of sending the backend its OFF request. */
static void test_four_modes_equal_and_zero_off(void)
{
    for (MenuDirection direction = MENU_FORWARD; direction <= MENU_REVERSE; ++direction) {
        PowerMenu menu;
        setup(&menu, direction);
        press(&menu, MENU_KEY_UP); /* Start/stop selection. */
        MenuRequest request = press(&menu, MENU_KEY_OK);
        assert(request.action == MENU_ACTION_START && request.mode == MENU_MODE_BUCK);
        assert(request.target_volts == 12.0f);
        menu.settings[direction] = (MenuSettings){12U, 36U};
        request = press(&menu, MENU_KEY_OK);
        assert(request.action == MENU_ACTION_START && request.mode == MENU_MODE_BOOST);
        assert(request.target_volts == 36.0f);
        menu.settings[direction] = (MenuSettings){36U, 36U};
        assert(press(&menu, MENU_KEY_OK).action == MENU_ACTION_NONE);
        assert(menu.notice == MENU_NOTICE_EQUAL);
        menu.settings[direction].output_volts = 0U;
        request = press(&menu, MENU_KEY_OK);
        assert(request.action == MENU_ACTION_START && request.mode == MENU_MODE_BUCK);
        assert(request.target_volts == 0.0f);
        data.state = MENU_FAULT;
        data.fault = 8U;
        request = press(&menu, MENU_KEY_OK);
        assert(request.action == MENU_ACTION_START && request.mode == MENU_MODE_BUCK);
        assert(request.target_volts == 0.0f);
        menu.settings[direction].output_volts = 12U;
        assert(press(&menu, MENU_KEY_OK).action == MENU_ACTION_NONE);
        data.state = MENU_RUNNING;
        data.fault = 0U;
        assert(press(&menu, MENU_KEY_OK).action == MENU_ACTION_STOP);
        data.state = MENU_IDLE;
        data.pending = 1U;
        assert(press(&menu, MENU_KEY_BACK).action == MENU_ACTION_STOP);
        assert(menu.page == MENU_HOME);
    }
}

int main(int argc, char **argv)
{
    if (argc == 1 || !strcmp(argv[1], "bounds")) test_bounds_and_whole_volt_steps();
    if (argc == 1 || !strcmp(argv[1], "identity")) test_identity_and_full_width_headers();
    if (argc == 1 || !strcmp(argv[1], "actions")) test_four_modes_equal_and_zero_off();
    puts("Main menu: 12..40/0..36, whole-volt edits, four modes, equal rejection, zero OFF and identity: PASS");
    return 0;
}
