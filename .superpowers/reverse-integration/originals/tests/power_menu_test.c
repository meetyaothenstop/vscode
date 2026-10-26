#include "power_menu.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static MenuTelemetry data;
static MenuRequest press(PowerMenu *m, MenuKey key)
{
    return Menu_HandleKey(m, key, &data);
}
static void setup(PowerMenu *m)
{
    memset(&data, 0, sizeof(data));
    data.valid = 1;
    Menu_Init(m);
}
static void edit(PowerMenu *m, int count)
{
    press(m, MENU_KEY_OK);
    for (int i = 0; i < (count < 0 ? -count : count); ++i)
        press(m, count < 0 ? MENU_KEY_DOWN : MENU_KEY_UP);
    press(m, MENU_KEY_OK);
}
static void test_navigation_and_transactional_edits(void)
{
    PowerMenu m;
    setup(&m);
    assert(press(&m, MENU_KEY_OK).action == MENU_ACTION_NONE);
    assert(m.page == MENU_PARAMETERS); /* Fails if the second page is missing. */
    assert(m.selection == 0);
    assert(Menu_Mode(&m) == MENU_MODE_BOOST);
    press(&m, MENU_KEY_OK);
    press(&m, MENU_KEY_UP);
    assert(m.edit_volts == 13 && m.settings[0].input_volts == 12);
    press(&m, MENU_KEY_BACK);
    assert(!m.editing && m.page == MENU_PARAMETERS);
    assert(m.settings[0].input_volts == 12);
    edit(&m, 2);
    assert(m.settings[0].input_volts == 14);
    press(&m, MENU_KEY_DOWN);
    edit(&m, -1);
    assert(m.settings[0].output_volts == 35);
    press(&m, MENU_KEY_UP);
    assert(m.selection == 0);
    press(&m, MENU_KEY_UP);
    assert(m.selection == 2);
    press(&m, MENU_KEY_DOWN);
    assert(m.selection == 0);
    press(&m, MENU_KEY_BACK);
    assert(m.page == MENU_HOME && m.direction == MENU_FORWARD);
    press(&m, MENU_KEY_UP);
    assert(m.direction == MENU_REVERSE);
    press(&m, MENU_KEY_OK);
    assert(m.settings[1].input_volts == 36 && Menu_Mode(&m) == MENU_MODE_BUCK);
    edit(&m, -1);
    press(&m, MENU_KEY_BACK);
    press(&m, MENU_KEY_DOWN);
    press(&m, MENU_KEY_OK);
    assert(m.settings[0].input_volts == 14 && m.settings[1].input_volts == 35);
}
static void test_limits_auto_mode_and_actions(void)
{
    PowerMenu m;
    setup(&m);
    press(&m, MENU_KEY_OK);
    edit(&m, 100);
    assert(m.settings[0].input_volts == 40);
    assert(Menu_Mode(&m) == MENU_MODE_BUCK);
    press(&m, MENU_KEY_DOWN);
    edit(&m, -100);
    assert(m.settings[0].output_volts == 1);
    edit(&m, 100);
    assert(m.settings[0].output_volts == 36);
    press(&m, MENU_KEY_UP);
    edit(&m, -100);
    assert(m.settings[0].input_volts == 6);
    press(&m, MENU_KEY_UP);
    MenuRequest r = press(&m, MENU_KEY_OK);
    assert(r.action == MENU_ACTION_START && r.mode == MENU_MODE_BOOST);
    assert(r.target_volts == 36.0f);
    Menu_StartResult(&m, 0);
    MenuFrame f;
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[7], "START REJECTED"));
    Menu_StartResult(&m, 1);
    m.settings[0].input_volts = 40;
    r = press(&m, MENU_KEY_OK);
    assert(r.action == MENU_ACTION_START && r.mode == MENU_MODE_BUCK);
    m.settings[0].input_volts = 36;
    assert(Menu_Mode(&m) == MENU_MODE_EQUAL);
    assert(press(&m, MENU_KEY_OK).action == MENU_ACTION_NONE);
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[7], "SET VIN != VOUT"));
    data.state = MENU_FAULT;
    data.fault = 128;
    assert(press(&m, MENU_KEY_OK).action == MENU_ACTION_NONE);
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[7], "FAULT:128"));
}
static void test_pending_running_back_and_reverse(void)
{
    PowerMenu m;
    setup(&m);
    press(&m, MENU_KEY_OK);
    data.pending = 1;
    press(&m, MENU_KEY_OK);
    assert(!m.editing);
    MenuFrame f;
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[5], "stop"));
    assert(press(&m, MENU_KEY_BACK).action == MENU_ACTION_STOP);
    assert(m.page == MENU_HOME);
    data.pending = 0;
    data.state = MENU_RUNNING;
    press(&m, MENU_KEY_OK);
    press(&m, MENU_KEY_UP);
    assert(press(&m, MENU_KEY_OK).action == MENU_ACTION_STOP);
    data.state = MENU_IDLE;
    press(&m, MENU_KEY_BACK);
    press(&m, MENU_KEY_DOWN);
    press(&m, MENU_KEY_OK);
    press(&m, MENU_KEY_UP);
    assert(press(&m, MENU_KEY_OK).action == MENU_ACTION_NONE);
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[7], "NOT IMPLEMENTED"));
}
static void test_rendered_values_direction_and_no_data(void)
{
    PowerMenu m;
    setup(&m);
    data.v_left = 12.25f; data.v_right = 35.75f; data.il = -1.25f;
    press(&m, MENU_KEY_OK);
    MenuFrame f;
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[0], "FORWARD BOOST"));
    assert(strstr(f.rows[1] + 11, "IL"));
    assert(strstr(f.rows[2] + 11, "-1.25 A"));
    assert(!strncmp(f.rows[3] + 11, "intput_ret", 10));
    assert(!strncmp(f.rows[5] + 11, "output_ret", 10));
    assert(strstr(f.rows[4] + 11, "12.25 V"));
    assert(strstr(f.rows[6] + 11, "35.75 V"));
    for (unsigned i = 0; i < MENU_ROWS; ++i) assert(strlen(f.rows[i]) == MENU_COLUMNS);
    press(&m, MENU_KEY_BACK);
    press(&m, MENU_KEY_DOWN);
    press(&m, MENU_KEY_OK);
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[4] + 11, "35.75 V"));
    assert(strstr(f.rows[6] + 11, "12.25 V"));
    data.valid = 0;
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[2] + 11, "--"));
    assert(strstr(f.rows[4] + 11, "--"));
    data.valid = 1; data.il = NAN;
    Menu_Render(&m, &data, &f);
    assert(strstr(f.rows[2] + 11, "--"));
}
int main(void)
{
    test_navigation_and_transactional_edits();
    test_limits_auto_mode_and_actions();
    test_pending_running_back_and_reverse();
    test_rendered_values_direction_and_no_data();
    puts("Menu navigation, edits, automatic modes, action gates and telemetry: PASS");
    return 0;
}
