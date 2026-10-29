#include "power_ui.h"
#include "power_menu.h"
#include "power_control.h"
#include "power_build_config.h"
#include "key.h"
#include "OLED_Function.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

volatile PowerStatus power_status;
static uint32_t tick;
static unsigned keys, scans, starts, stops, last_mode, page_calls, pending, clears, last_page;
static float last_target;
static int valid = 1, accept = 1, fail_display;
static MenuFrame drawn;
static AdcMeasurements measured;
static HAL_StatusTypeDef transport = HAL_OK, submission = HAL_OK;
static uint8_t transport_allowed;

uint32_t HAL_GetTick(void) { return tick; }
void Key_Scan(void) { ++scans; }
uint8_t Key_IsPressed(uint8_t id)
{
    unsigned mask = 1U << id;
    uint8_t result = (keys & mask) != 0;
    keys &= ~mask;
    return result;
}
int Power_IsStartPending(void) { return (int)pending; }
int Power_SamplesAreValid(void) { return valid; }
int Power_RequestStart(unsigned mode, float target)
{
    ++starts; last_mode = mode; last_target = target;
    if (accept) pending = 1;
    return accept;
}
void Power_Stop(void) { ++stops; pending = 0; power_status.state = POWER_STOPPED; }
void OLED_ClearRAM(void) { ++clears; memset(&drawn, 0, sizeof(drawn)); }
void OLED_DrawStr(int16_t x, int16_t y, const char *text)
{
    assert(x == 0 && y >= 0 && y <= 56 && y % 8 == 0);
    assert(strlen(text) == MENU_COLUMNS);
    memcpy(drawn.rows[y / 8], text, MENU_COLUMNS + 1);
}
HAL_StatusTypeDef OLED_RefreshPage(uint8_t page)
{
    assert(page < 8); ++page_calls; last_page = page;
    return fail_display ? HAL_ERROR : submission;
}
HAL_StatusTypeDef OLED_TransferPoll(uint8_t allow)
{ transport_allowed = allow; return transport; }
static void service(unsigned mask)
{
    unsigned before = page_calls;
    keys = mask;
    PowerUI_Service(&measured);
    assert(keys == 0); /* Every event is consumed, including lower priority keys. */
    assert(page_calls - before <= 1); /* The foreground is never held for a frame. */
    tick += 25;
}
static void key(unsigned id) { service(1U << id); }
static void settle(void) { for (unsigned i = 0; i < 20; ++i) service(0); }
static void test_external_start_cancels_edit_and_transitions_clear_old_notices(void)
{
    power_status.state = POWER_STOPPED;
    power_status.fault = 0;
    pending = 0; valid = 1; fail_display = 0;
    PowerUI_Init();
    key(3); key(3); key(1); settle();
    assert(strstr(drawn.rows[7], "EDIT K3:OK"));
    pending = 1; /* Existing debugger start API can act while UI is editing. */
    settle();
    assert(strstr(drawn.rows[7], "STARTING"));
    assert(strstr(drawn.rows[2], "12 V")); /* Unconfirmed 13 V was discarded. */
    pending = 0; power_status.state = POWER_RUNNING;
    settle();
    assert(strstr(drawn.rows[7], "RUNNING"));
    key(3); settle();
    assert(strstr(drawn.rows[7], "STOP BEFORE EDIT"));
    power_status.state = POWER_STOPPED;
    settle();
    assert(strstr(drawn.rows[7], "STOPPED"));
}
static void test_reverse_boost_changeover_and_hold(void)
{
    power_status.state = POWER_STOPPED;
    power_status.fault = 0;
    pending = 0; valid = accept = 1; fail_display = 0;
    unsigned before = starts, stopped_before = stops;
    PowerUI_Init();
    key(2); key(3); key(3);
    for (unsigned i = 0; i < 24; ++i) key(2); /* Reverse input 36->12. */
    key(3); key(2); key(3);
    for (unsigned i = 0; i < 24; ++i) key(1); /* Reverse output 12->36. */
    key(3); key(2); key(3);
    assert(starts == before + 1 && last_mode == 4 && last_target == 36.0f);
    pending = 0;
    power_status.state = POWER_CHANGEOVER;
    settle();
    assert(strstr(drawn.rows[7], "STARTING") && strstr(drawn.rows[5], "stop"));
    power_status.state = POWER_HOLD;
    settle();
    assert(strstr(drawn.rows[7], "RUNNING") && strstr(drawn.rows[5], "stop"));
    key(2); key(3); settle();
    assert(strstr(drawn.rows[7], "STOP BEFORE EDIT"));
    key(4);
    assert(stops == stopped_before + 1 && power_status.state == POWER_STOPPED);
}
#if POWER_BENCH_LIVE_OLED
int main(void)
{
    measured.v_high = 12.0; measured.v_low = 5.9; measured.il = 0.1;
    PowerUI_Init(); settle();
    key(3); settle(); key(1); settle(); /* Parameter page, select start. */
    unsigned before = page_calls;
    key(3); settle();
    assert(starts == 1U && pending && page_calls == before && !transport_allowed);
    pending = 0;
    power_status.state = POWER_BOOTSTRAP; settle();
    power_status.state = POWER_ARMING; settle();
    assert(page_calls == before && !transport_allowed);
    power_status.state = POWER_RUNNING; settle();
    assert(page_calls > before && transport_allowed); /* Regression: old mute freezes here. */
    assert(strstr(drawn.rows[6], "5.90 V"));
    measured.v_low = 6.0; settle();
    assert(strstr(drawn.rows[6], "6.00 V") && strstr(drawn.rows[7], "RUNNING"));
    assert(strstr(drawn.rows[0], "SW-DMA8"));

    transport = HAL_BUSY;
    unsigned old_clears = clears; before = page_calls;
    measured.v_low = 6.1; settle();
    assert(clears == old_clears && page_calls == before); /* Do not mutate a frame in flight. */
    key(3); /* Selected STOP is handled before the busy transport. */
    assert(stops == 1U && power_status.state == POWER_STOPPED && page_calls == before);
    transport = HAL_OK; service(0);
    assert(last_page == 0U && clears == old_clears + 1U && strstr(drawn.rows[7], "STOPPED"));
    assert(strstr(drawn.rows[6], "6.10 V"));

    transport = HAL_BUSY; pending = 1; before = page_calls;
    service(0); assert(!transport_allowed && page_calls == before);
    key(4); assert(stops == 2U && !pending && page_calls == before);
    transport = HAL_OK; service(0);
    assert(last_page == 0U && strstr(drawn.rows[0], "12V BUCK DMA OVP8"));
    transport = HAL_BUSY; old_clears = clears;
    power_status.state = POWER_FAULT; power_status.fault = 4U;
    service(0); assert(clears == old_clears);
    transport = HAL_OK; service(0);
    assert(last_page == 0U && strstr(drawn.rows[7], "FAULT:4"));

    /* Submission BUSY also holds the page, without an error retry delay. */
    submission = HAL_BUSY; service(0); unsigned held_page = last_page;
    submission = HAL_OK; service(0); assert(last_page == held_page);

    transport = HAL_ERROR; before = page_calls; unsigned old_scans = scans;
    power_status.state = POWER_RUNNING;
    key(4); settle();
    assert(stops == 3U && scans == old_scans + 21U && page_calls == before);
    puts("Live OLED: RUNNING telemetry, startup mute, busy/error STOP, immutable frames and full transition redraw: PASS");
    return 0;
}
#elif defined(POWER_TEST_SKIP_OLED_ACTIVE) && POWER_TEST_SKIP_OLED_ACTIVE
int main(void)
{
    measured.v_high = 12.0; measured.v_low = 0.0; measured.il = 0.0;
    PowerUI_Init();
    settle();
    key(3); settle(); /* Parameters, bench default12->6. */
    assert(strstr(drawn.rows[0], "FORWARD BUCK"));
    key(1); settle(); /* Wrap input selection to start. */
    unsigned before = page_calls;
    key(3);
    assert(starts == 1U && last_mode == 1U && last_target == 6.0f && pending);
    assert(page_calls == before); /* No transfer after accepting a start. */
    settle();
    assert(page_calls == before);
    pending = 0U;
    power_status.state = POWER_RUNNING;
    settle();
    assert(page_calls == before);
    key(3); settle(); /* Selected stop must still execute with display muted. */
    assert(stops == 1U && power_status.state == POWER_STOPPED);
    assert(page_calls > before && strstr(drawn.rows[7], "STOPPED"));
    pending = 1U;
    before = page_calls;
    service(0);
    assert(page_calls == before);
    key(4); settle(); /* Back is another working stop path. */
    assert(stops == 2U && !pending && power_status.state == POWER_STOPPED);
    assert(page_calls > before);
#if POWER_BENCH_FAST_BUCK
    assert(strstr(drawn.rows[0], "12V BUCK ADC2 OVP8"));
#else
    assert(strstr(drawn.rows[0], "OLED-MUTE"));
#endif
    power_status.state = POWER_FAULT;
    power_status.fault = 64U;
    power_status.fault_detail = 3U;
    settle();
    assert(strstr(drawn.rows[7], "FAULT:64 T3"));
    puts("OLED timing isolation: active rendering/transfers muted, keys/stop/fault display preserved: PASS");
    return 0;
}
#else
int main(void)
{
    measured.v_high = 12.25; measured.v_low = 0.0; measured.il = -0.1;
    PowerUI_Init();
    service(0);
    assert(strstr(drawn.rows[2], ">forward"));
    assert(starts == 0 && stops == 0);
    key(3); settle();
    assert(strstr(drawn.rows[0], "FORWARD BOOST"));
    key(1); key(3); /* Up wraps to start, confirm submits Boost once. */
    assert(starts == 1 && last_mode == 2 && last_target == 36.0f);
    settle();
    assert(strstr(drawn.rows[5], "stop") && strstr(drawn.rows[7], "STARTING"));
    key(2); key(3); settle(); /* Pending startup locks voltage editing. */
    assert(strstr(drawn.rows[7], "STOP BEFORE EDIT"));
    key(4);
    assert(stops == 1 && pending == 0);
    key(3); key(3); /* Edit input from 12 to 40. */
    for (unsigned i = 0; i < 28; ++i) key(1);
    key(3); key(1); key(3);
    assert(starts == 2 && last_mode == 1 && last_target == 36.0f);
    key(4);
    assert(stops == 2);
    key(2); key(3); key(1); key(3); settle();
    assert(starts == 3 && last_mode == 3 && last_target == 12.0f);
    assert(strstr(drawn.rows[7], "STARTING"));
    /* Simultaneous OK and BACK: return wins and cannot queue a start. */
    service((1U << 3) | (1U << 4)); settle();
    assert(starts == 3 && strstr(drawn.rows[0], "BIDIRECTIONAL"));
    key(1); key(3); key(1);
    accept = 0;
    key(3); settle();
    assert(starts == 4 && strstr(drawn.rows[7], "START REJECTED"));
    power_status.state = POWER_FAULT;
    power_status.fault = 8;
    key(3); settle();
    assert(starts == 4 && strstr(drawn.rows[7], "FAULT:8"));
    power_status.fault = 64U;
    power_status.fault_detail = 3U;
    settle();
    assert(strstr(drawn.rows[7], "FAULT:64 T3"));
    power_status.fault = 128U;
    power_status.fault_detail = 5U;
    settle();
    assert(strstr(drawn.rows[7], "FAULT:128 S5"));
    power_status.fault_detail = 0U;
    power_status.fault = 2U;
    power_status.fault_input_volts = 9.75f;
    settle();
    assert(strstr(drawn.rows[7], "FAULT:2 IN 9.75 V"));
    valid = 0;
    settle();
    assert(strstr(drawn.rows[2] + 11, "--"));
    /* Missing OLED must still scan keys and allow navigation/stop. */
    fail_display = 1;
    unsigned old_scans = scans;
    key(4); settle();
    assert(scans == old_scans + 21);
    test_external_start_cancels_edit_and_transitions_clear_old_notices();
    test_reverse_boost_changeover_and_hold();
    puts("UI hardware actions, key priority, pending lock and page scheduling: PASS");
    return 0;
}
#endif
