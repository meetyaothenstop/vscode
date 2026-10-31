#include "power_menu.h"
#include "power_build_config.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int active(const MenuTelemetry *data)
{
    return data->pending || data->state == MENU_STARTING || data->state == MENU_RUNNING;
}

void Menu_Init(PowerMenu *menu)
{
    memset(menu, 0, sizeof(*menu));
    menu->settings[MENU_FORWARD] = (MenuSettings){POWER_FORWARD_DEFAULT_INPUT_VOLTS,
                                                POWER_FORWARD_DEFAULT_OUTPUT_VOLTS};
    menu->settings[MENU_REVERSE] = (MenuSettings){36, 12};
}

MenuMode Menu_Mode(const PowerMenu *menu)
{
    const MenuSettings *s = &menu->settings[menu->direction];
    if (s->output_volts < s->input_volts) return MENU_MODE_BUCK;
    if (s->output_volts > s->input_volts) return MENU_MODE_BOOST;
    return MENU_MODE_EQUAL;
}

MenuRequest Menu_HandleKey(PowerMenu *menu, MenuKey key, const MenuTelemetry *data)
{
    MenuRequest request = {MENU_ACTION_NONE, MENU_MODE_EQUAL, 0};
    if (key == MENU_KEY_NONE) return request;
    menu->notice = MENU_NOTICE_NONE;
    if (key == MENU_KEY_BACK) {
        if (menu->editing) {
            menu->editing = 0;
        } else {
            menu->page = MENU_HOME;
            if (active(data)) request.action = MENU_ACTION_STOP;
        }
        return request;
    }
    if (menu->page == MENU_HOME) {
        if (key == MENU_KEY_UP || key == MENU_KEY_DOWN)
            menu->direction = menu->direction == MENU_FORWARD ? MENU_REVERSE : MENU_FORWARD;
        if (key == MENU_KEY_OK) {
            menu->page = MENU_PARAMETERS;
            menu->selection = 0;
        }
        return request;
    }
    MenuSettings *s = &menu->settings[menu->direction];
    if (menu->editing) {
        if (active(data)) {
            menu->editing = 0;
            menu->notice = MENU_NOTICE_LOCKED;
            return request;
        }
        unsigned minimum = menu->selection == 0 ?
            (POWER_MAIN_APPLICATION ? POWER_MAIN_INPUT_SETTING_MIN : 6U) :
            (POWER_MAIN_APPLICATION ? POWER_MAIN_OUTPUT_SETTING_MIN : 1U);
        unsigned maximum = menu->selection == 0 ?
            (POWER_MAIN_APPLICATION ? POWER_MAIN_INPUT_SETTING_MAX : 40U) :
            (POWER_MAIN_APPLICATION ? POWER_MAIN_OUTPUT_SETTING_MAX : 36U);
        if (key == MENU_KEY_UP && menu->edit_volts < maximum) ++menu->edit_volts;
        if (key == MENU_KEY_DOWN && menu->edit_volts > minimum) --menu->edit_volts;
        if (key == MENU_KEY_OK) {
            if (menu->selection == 0) s->input_volts = menu->edit_volts;
            else s->output_volts = menu->edit_volts;
            menu->editing = 0;
        }
        return request;
    }
    if (key == MENU_KEY_UP) menu->selection = (menu->selection + 2U) % 3U;
    if (key == MENU_KEY_DOWN) menu->selection = (menu->selection + 1U) % 3U;
    if (key != MENU_KEY_OK) return request;
    if (menu->selection < 2) {
        if (active(data)) menu->notice = MENU_NOTICE_LOCKED;
        else {
            menu->editing = 1;
            menu->edit_volts = menu->selection == 0 ? s->input_volts : s->output_volts;
        }
    } else if (active(data)) {
        request.action = MENU_ACTION_STOP;
    } else if ((POWER_MAIN_APPLICATION && s->output_volts == 0U) ||
               (data->state != MENU_FAULT && data->fault == 0U)) {
        /* Main Buck zero is a backend OFF request, including while faulted.
         * The backend disables outputs and preserves the first fault. */
        if (Menu_Mode(menu) == MENU_MODE_EQUAL) menu->notice = MENU_NOTICE_EQUAL;
        else {
            request.action = MENU_ACTION_START;
            request.mode = Menu_Mode(menu);
            request.target_volts = (float)s->output_volts;
        }
    }
    return request;
}

void Menu_StartResult(PowerMenu *menu, int accepted)
{
    menu->notice = accepted ? MENU_NOTICE_NONE : MENU_NOTICE_REJECTED;
}

static void put(MenuFrame *frame, unsigned row, unsigned column, const char *text)
{
    while (*text && column < MENU_COLUMNS) frame->rows[row][column++] = *text++;
}

/* Integer formatting avoids linking a double-precision printf in the UI. */
static void reading(char *buffer, size_t size, float value, int valid, int current)
{
    if (!valid || !isfinite(value) || fabsf(value) > 999.99f) {
        (void)snprintf(buffer, size, "--");
        return;
    }
    unsigned hundredths = (unsigned)(fabsf(value) * 100.0f + 0.5f);
    const char *sign = value < -0.005f ? "-" : (current ? "+" : "");
    (void)snprintf(buffer, size, "%s%u.%02u %c", sign,
                   hundredths / 100U, hundredths % 100U, current ? 'A' : 'V');
}

static void status(const PowerMenu *menu, const MenuTelemetry *data, char *text, size_t size)
{
    if (data->state == MENU_FAULT || data->fault != 0U) {
        char kind = (data->fault & 64U) ? 'T' : ((data->fault & 128U) ? 'S' : '\0');
        if (data->fault == 2U) {
            char voltage[12];
            reading(voltage, sizeof(voltage), data->fault_input_volts, 1, 0);
            (void)snprintf(text, size, "FAULT:2 IN %.9s", voltage);
        } else if (data->fault_detail >= 1U && data->fault_detail <= 9U && kind)
            (void)snprintf(text, size, "FAULT:%lu %c%c", (unsigned long)data->fault,
                           kind, (int)('0' + data->fault_detail));
        else (void)snprintf(text, size, "FAULT:%lu", (unsigned long)data->fault);
    } else {
        const char *message = "STOPPED";
        if (data->pending || data->state == MENU_STARTING) message = "STARTING";
        else if (data->state == MENU_RUNNING) message = "RUNNING";
        if (menu->notice == MENU_NOTICE_EQUAL) message = "SET VIN != VOUT";
        else if (menu->notice == MENU_NOTICE_REJECTED) message = "START REJECTED";
        else if (menu->notice == MENU_NOTICE_LOCKED) message = "STOP BEFORE EDIT";
        else if (menu->editing) message = "EDIT K3:OK K4:CANCEL";
        (void)snprintf(text, size, "%s", message);
    }
}

void Menu_Render(const PowerMenu *menu, const MenuTelemetry *data, MenuFrame *frame)
{
    char text[MENU_COLUMNS + 1];
    for (unsigned row = 0; row < MENU_ROWS; ++row) {
        memset(frame->rows[row], ' ', MENU_COLUMNS);
        frame->rows[row][MENU_COLUMNS] = '\0';
    }
    if (menu->page == MENU_HOME) {
        put(frame, 0, 0, POWER_MAIN_APPLICATION ?
            (POWER_UNIFIED_DIAGNOSTICS ?
                (POWER_MAIN_ADC_SOFTWARE_ONLY ? "MAIN ADC DIAG 6A" : "MAIN HW DIAG 6A") :
                (POWER_MAIN_ADC_SOFTWARE_ONLY ? "MAIN ADC OVP1.5 C40" : "MAIN HW GUARD 6A")) :
            POWER_BENCH_24V_BUCK_TEST ?
            (POWER_BENCH_LIVE_OLED ? "24V BUCK DMA OVP15 C6" : "24V FAST OVP15 C6") :
            POWER_BENCH_LIVE_OLED ? "12V BUCK DMA OVP8" :
            POWER_BENCH_LATE_INPUT_ADC ? "12V BUCK ADC2 OVP8" :
            POWER_BENCH_FAST_BUCK ? "12V BUCK FAST OVP8" : POWER_CCM_CONTROL ?
            (POWER_UNIFIED_DIAGNOSTICS ? "12V CCM DIAG" : "12V CCM BASE") :
            (POWER_UNIFIED_DIAGNOSTICS ? "12V UNIFIED DIAG" :
            (POWER_TEST_SKIP_OLED_ACTIVE ? "12V SW OLED-MUTE" :
            (POWER_BENCH_12V_BUCK_TEST ? "12V BUCK SW DIAG" : "BIDIRECTIONAL DC/DC"))));
        put(frame, 2, 1, "forward");
        put(frame, 4, 1, "reverse");
        put(frame, menu->direction == MENU_FORWARD ? 2U : 4U, 0, ">");
        put(frame, 6, 0, "K1/2:UP/DN K3:OK");
    } else {
        MenuMode mode = Menu_Mode(menu);
        (void)snprintf(text, sizeof(text), "%s %s%s",
                       menu->direction == MENU_FORWARD ? "FORWARD" : "REVERSE",
                       mode == MENU_MODE_BUCK ? "BUCK" :
                       (mode == MENU_MODE_BOOST ? "BOOST" : "EQUAL"),
                       POWER_MAIN_APPLICATION ? (POWER_UNIFIED_DIAGNOSTICS ? " M-DIAG" : " M-15C40") :
                       POWER_BENCH_24V_BUCK_TEST ? " SW15-C6" :
                       POWER_BENCH_LIVE_OLED ? " SW-DMA8" :
                       POWER_BENCH_LATE_INPUT_ADC ? " SW-A2-8" : POWER_BENCH_FAST_BUCK ? " SW-FAST" :
                       POWER_CCM_CONTROL ? (POWER_UNIFIED_DIAGNOSTICS ? " SW-CCM" : " SW-BASE") :
                       (POWER_UNIFIED_DIAGNOSTICS ? " SW-DIAG" :
                       (POWER_TEST_SKIP_OLED_ACTIVE ? " SW-MUTE" :
                       (POWER_BENCH_12V_BUCK_TEST ? " SW" : ""))));
        put(frame, 0, 0, text);
        put(frame, 1, 1, "input_v");
        put(frame, 3, 1, "output_v");
        put(frame, 5, 1, active(data) ? "stop" : "start");
        put(frame, 1U + 2U * menu->selection, 0, menu->editing ? "*" : ">");
        const MenuSettings *s = &menu->settings[menu->direction];
        unsigned vin = menu->editing && menu->selection == 0 ? menu->edit_volts : s->input_volts;
        unsigned vout = menu->editing && menu->selection == 1 ? menu->edit_volts : s->output_volts;
        (void)snprintf(text, sizeof(text), "%u V", vin);
        put(frame, 2, 1, text);
        (void)snprintf(text, sizeof(text), "%u V", vout);
        put(frame, 4, 1, text);
        put(frame, 1, 11, "IL");
        put(frame, 3, 11, "intput_ret");
        put(frame, 5, 11, "output_ret");
        reading(text, sizeof(text), data->il, data->valid, 1);
        put(frame, 2, 11, text);
        reading(text, sizeof(text), menu->direction == MENU_FORWARD ? data->v_left : data->v_right,
                data->valid, 0);
        put(frame, 4, 11, text);
        reading(text, sizeof(text), menu->direction == MENU_FORWARD ? data->v_right : data->v_left,
                data->valid, 0);
        put(frame, 6, 11, text);
    }
    status(menu, data, text, sizeof(text));
    put(frame, 7, 0, text);
}
