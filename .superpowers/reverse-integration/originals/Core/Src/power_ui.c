#include "power_ui.h"
#include "power_menu.h"
#include "power_control.h"
#include "key.h"
#include "OLED_Function.h"

#define DISPLAY_PERIOD_MS 200U

static PowerMenu menu;
/* Foreground-only scratch frame: keep 176 bytes off the shared IRQ stack. */
static MenuFrame frame;
static uint32_t frame_tick, retry_tick, last_fault;
static MenuState last_state;
static uint8_t next_page, dirty, display_failed, last_pending, last_valid;

void PowerUI_Init(void)
{
    Menu_Init(&menu);
    frame_tick = retry_tick = last_fault = 0;
    next_page = MENU_ROWS;
    dirty = 1;
    display_failed = last_pending = last_valid = 0;
    last_state = MENU_IDLE;
}

static MenuTelemetry telemetry(const AdcMeasurements *measurements)
{
    MenuTelemetry data = {0};
    /* Read pending before state: if the IRQ consumes pending between these
     * reads, its new starting state still prevents edits and permits stop. */
    data.pending = (uint8_t)Power_IsStartPending();
    switch (power_status.state) {
    case POWER_BOOTSTRAP: case POWER_ARMING: data.state = MENU_STARTING; break;
    case POWER_RUNNING: data.state = MENU_RUNNING; break;
    case POWER_FAULT: data.state = MENU_FAULT; break;
    default: data.state = MENU_IDLE; break;
    }
    data.fault = power_status.fault;
    data.valid = (uint8_t)Power_SamplesAreValid();
    /* Power_Service already copied/converts the three ADC channels in the
     * foreground. No format or OLED operation masks the control interrupt. */
    data.v_left = (float)measurements->v_high;
    data.v_right = (float)measurements->v_low;
    data.il = (float)measurements->il;
    return data;
}

static void sync_control_state(const MenuTelemetry *data)
{
    if (data->state != last_state || data->fault != last_fault || data->pending != last_pending) {
        menu.notice = MENU_NOTICE_NONE;
        if (data->pending || data->state == MENU_STARTING || data->state == MENU_RUNNING)
            menu.editing = 0;
        dirty = 1;
    }
    if (data->valid != last_valid) dirty = 1;
    last_state = data->state;
    last_fault = data->fault;
    last_pending = data->pending;
    last_valid = data->valid;
}

void PowerUI_Service(const AdcMeasurements *measurements)
{
    Key_Scan();
    uint8_t up = Key_IsPressed(KEY_ID_1);
    uint8_t down = Key_IsPressed(KEY_ID_2);
    uint8_t ok = Key_IsPressed(KEY_ID_3);
    uint8_t back = Key_IsPressed(KEY_ID_4);
    MenuKey key = back ? MENU_KEY_BACK : (ok ? MENU_KEY_OK :
                  (up ? MENU_KEY_UP : (down ? MENU_KEY_DOWN : MENU_KEY_NONE)));
    MenuTelemetry data = telemetry(measurements);
    sync_control_state(&data);
    if (key != MENU_KEY_NONE) {
        MenuRequest request = Menu_HandleKey(&menu, key, &data);
        if (request.action == MENU_ACTION_STOP) Power_Stop();
        else if (request.action == MENU_ACTION_START)
            Menu_StartResult(&menu, Power_RequestStart((unsigned)request.mode, request.target_volts));
        data = telemetry(measurements);
        dirty = 1;
    }
    sync_control_state(&data);

    uint32_t now = HAL_GetTick();
    if (display_failed && now - retry_tick < DISPLAY_PERIOD_MS) return;
    if (next_page == MENU_ROWS && (dirty || now - frame_tick >= DISPLAY_PERIOD_MS)) {
        Menu_Render(&menu, &data, &frame);
        OLED_ClearRAM();
        for (unsigned row = 0; row < MENU_ROWS; ++row)
            OLED_DrawStr(0, (int16_t)(row * 8U), frame.rows[row]);
        next_page = 0;
        frame_tick = now;
        dirty = 0;
    }
    if (next_page < MENU_ROWS) {
        if (OLED_RefreshPage(next_page) == HAL_OK) {
            ++next_page;
            display_failed = 0;
        } else {
            /* An absent display never blocks key handling or power control. */
            display_failed = 1;
            retry_tick = now;
            next_page = MENU_ROWS;
            dirty = 1;
        }
    }
}
