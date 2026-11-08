#include "power_ui.h"
#include "power_menu.h"
#include "power_control.h"
#include "power_build_config.h"
#include "key.h"
#include "OLED_Function.h"
#include "power_event_diag.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#if !POWER_MAIN_APPLICATION
#error Main UI tests require the main application profile.
#endif

volatile PowerStatus power_status;
static AdcMeasurements measured;
static MenuFrame drawn;
static uint32_t tick;
static unsigned keys, scans, starts, stops, last_mode, pending;
static unsigned clears, page_calls, last_page, polls, stopped_at_poll;
static float last_target;
static int valid, accept;
static uint8_t transport_allowed;
static HAL_StatusTypeDef transport, submission;
#if POWER_OLED_FRAME_RESUME
static uint8_t queued_page,completed_page;
static unsigned page_histogram[8];
uint8_t OLED_TakePageComplete(void)
{uint8_t done=completed_page;completed_page=0;return done;}
#endif
#if POWER_EVENT_DIAGNOSTICS
static uint32_t event_last_stage,event_seen,event_entries;
void PowerEvent_UIMark(uint32_t stage,uint32_t page,uint32_t status)
{
    (void)page;(void)status;
    event_last_stage=stage;event_seen|=1U<<stage;
    if(stage==POWER_EVENT_UI_ENTRY)++event_entries;
}
#endif

uint32_t HAL_GetTick(void) { return tick; }
void Key_Scan(void) { ++scans; }
uint8_t Key_IsPressed(uint8_t id)
{
    unsigned mask = 1U << id;
    uint8_t result = (keys & mask) != 0U;
    keys &= ~mask;
    return result;
}
int Power_IsStartPending(void) { return (int)pending; }
int Power_SamplesAreValid(void) { return valid; }
int Power_RequestStart(unsigned mode, float target)
{
    ++starts;
    last_mode = mode;
    last_target = target;
    if (accept) {
        pending = target != 0.0f;
        if (target == 0.0f && power_status.state != POWER_FAULT)
            power_status.state = POWER_STOPPED;
    }
    return accept;
}
void Power_Stop(void)
{
    ++stops;
    pending = 0U;
    if (power_status.state != POWER_FAULT) power_status.state = POWER_STOPPED;
}
void OLED_ClearRAM(void) { ++clears; memset(&drawn, 0, sizeof(drawn)); }
void OLED_DrawStr(int16_t x, int16_t y, const char *text)
{
    assert(x == 0 && y >= 0 && y <= 56 && y % 8 == 0);
    assert(strlen(text) == MENU_COLUMNS);
    memcpy(drawn.rows[y / 8], text, MENU_COLUMNS + 1);
}
HAL_StatusTypeDef OLED_RefreshPage(uint8_t page)
{
    assert(page < MENU_ROWS);
    ++page_calls;
    last_page = page;
#if POWER_OLED_FRAME_RESUME
    if(submission==HAL_OK){queued_page=1;++page_histogram[page];}
#endif
    return submission;
}
HAL_StatusTypeDef OLED_TransferPoll(uint8_t allow)
{
    ++polls;
    transport_allowed = allow;
    stopped_at_poll = stops;
#if POWER_OLED_FRAME_RESUME
    if(transport==HAL_OK && queued_page){completed_page=1;queued_page=0;}
#endif
    return transport;
}

static void setup(void)
{
    memset((void *)&power_status, 0, sizeof(power_status));
    memset(&drawn, 0, sizeof(drawn));
    measured.v_high = 24.0;
    measured.v_low = 11.9;
    measured.il = 0.1;
    tick = 1000U;
    keys = scans = starts = stops = last_mode = pending = 0U;
    clears = page_calls = last_page = polls = stopped_at_poll = 0U;
    valid = accept = 1;
    transport = submission = HAL_OK;
    transport_allowed = 1U;
    PowerUI_Init();
#if POWER_OLED_FRAME_RESUME
    queued_page=completed_page=0;memset(page_histogram,0,sizeof(page_histogram));
#endif
}

static void service(unsigned mask)
{
    unsigned before = page_calls;
    keys = mask;
    PowerUI_Service(&measured);
    assert(!keys);
    assert(page_calls - before <= 1U);
    tick += 25U;
}
static void key(unsigned id) { service(1U << id); }
static void settle(void) { for (unsigned i = 0; i < 20U; ++i) service(0U); }
static void edit(int steps)
{
    key(3);
    for (int i = 0; i < (steps < 0 ? -steps : steps); ++i)
        key(steps < 0 ? 2U : 1U);
    key(3);
}

/* Catches guarding DMA and startup quieting with the legacy bench flags. */
static void test_startup_always_mutes_before_any_page_submission(void)
{
    setup();
    pending = 1U;
    service(0U);
    assert(page_calls == 0U && clears == 0U);
    assert(polls == 1U && !transport_allowed);
    pending = 0U;
    const PowerState preparing[] = {POWER_BOOTSTRAP, POWER_ARMING, POWER_CHANGEOVER};
    for (unsigned i = 0; i < sizeof(preparing) / sizeof(preparing[0]); ++i) {
        power_status.state = preparing[i];
        service(0U);
        assert(page_calls == 0U && clears == 0U && !transport_allowed);
    }
    power_status.state = POWER_RUNNING;
    service(0U);
    assert(page_calls == 1U && last_page == 0U && transport_allowed);
    assert(strstr(drawn.rows[7], "RUNNING"));
    settle();
    unsigned before = page_calls;
    pending = 1U;
    service(0U);
    assert(page_calls == before && !transport_allowed);
    key(4);
    assert(stops == 1U && stopped_at_poll == 1U && !pending);
    assert(last_page == 0U && strstr(drawn.rows[7], "STOPPED"));
}

/* Catches removing a mode, changing direction mapping, or selecting a bench
 * default when the user invokes the main image through the actual keys. */
static void test_all_four_ui_actions(void)
{
    const struct { unsigned reverse, boost, mode; float target; } cases[] = {
        {0U, 0U, 1U, 12.0f}, {0U, 1U, 2U, 36.0f},
        {1U, 0U, 3U, 6.0f}, {1U, 1U, 4U, 36.0f}
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        setup();
        if (cases[i].reverse) key(2);
        key(3);
        /* Select each test operating point independently of editable defaults. */
        edit(-100); /* Input to 12 V. */
        if (!cases[i].reverse && !cases[i].boost) edit(12); /* Forward Buck: 24 V. */
        key(2);
        edit(-100); /* Output to zero, then the explicit case target. */
        edit((int)cases[i].target);
        key(2);
        unsigned before = page_calls;
        key(3);
        assert(starts == 1U && last_mode == cases[i].mode && last_target == cases[i].target);
        assert(pending && page_calls == before && !transport_allowed);
        service((1U << 3) | (1U << 4)); /* BACK wins over OK. */
        assert(starts == 1U && stops == 1U && !pending);
    }
}

/* Catches a 1 V floor, swallowing zero locally, a Boost zero request, or
 * clearing a latched fault just to perform the main menu's OFF action. */
static void test_zero_off_is_submitted_in_both_directions_and_fault(void)
{
    for (unsigned reverse = 0U; reverse <= 1U; ++reverse) {
        setup();
        if (reverse) key(2);
        key(3); key(2); edit(-100); key(2);
        key(3);
        assert(starts == 1U && last_mode == (reverse ? 3U : 1U) && last_target == 0.0f);
        assert(!pending && power_status.state == POWER_STOPPED);
        settle();
        assert(strstr(drawn.rows[4], "0 V") && strstr(drawn.rows[0], (POWER_MULTIRATE_CONTROL ? (POWER_RAW_FAST_IRQ ? (POWER_EVENT_DIAGNOSTICS ? (POWER_OLED_FRAME_RESUME ? (POWER_ADC_FAULT_DETAILS ? "BUCK M-ADG2" : "BUCK M-FRM1") : "BUCK M-EVT1") : "BUCK M-RAW1") : "BUCK M-50/10") : "BUCK M-RRES1")));
        power_status.state = POWER_FAULT;
        power_status.fault = 8U;
        key(3);
        assert(starts == 2U && last_target == 0.0f && !pending);
        assert(power_status.state == POWER_FAULT && power_status.fault == 8U);
        assert(strstr(drawn.rows[7], "FAULT:8"));
    }
}

/* Catches active DMA frame mutation, rendering faster than 200 ms, stale
 * transition pages, blocking STOP on BUSY/error, or error retry spinning. */
static void test_running_frame_cadence_busy_stop_and_fault_restart(void)
{
    setup();
    service(0U);
#if POWER_MAIN_ADC_SOFTWARE_ONLY
    assert(strstr(drawn.rows[0], POWER_MULTIRATE_CONTROL ? "MAIN ADC B25/5" : "MAIN ADC RBUCK RES1"));
#else
    assert(strstr(drawn.rows[0], POWER_MULTIRATE_CONTROL ? "MAIN HW B25/5" : "MAIN HW GUARD 6A"));
#endif
    key(3); key(2); edit(-100); edit(6); key(2); key(3); /* Explicit Buck output. */
    pending = 0U;
    power_status.state = POWER_RUNNING;
    service(0U);
    assert(strstr(drawn.rows[0], (POWER_MULTIRATE_CONTROL ? (POWER_RAW_FAST_IRQ ? (POWER_EVENT_DIAGNOSTICS ? (POWER_OLED_FRAME_RESUME ? (POWER_ADC_FAULT_DETAILS ? "FORWARD BUCK M-ADG2" : "FORWARD BUCK M-FRM1") : "FORWARD BUCK M-EVT1") : "FORWARD BUCK M-RAW1") : "FORWARD BUCK M-50/10") : "FORWARD BUCK M-RRES1")));
    assert(strstr(drawn.rows[6], "11.90 V"));
    assert(strstr(drawn.rows[7], "RUNNING") && last_page == 0U);
    unsigned before = clears;
    for (unsigned i = 0; i < 7U; ++i) service(0U); /* Drain pages 1..7, still <200 ms. */
    assert(clears == before && last_page == 7U);
    measured.v_low = 12.0;
    service(0U); /* Exactly 200 ms starts a fresh RUNNING frame. */
    assert(clears == before + 1U && last_page == 0U);
    assert(strstr(drawn.rows[6], "12.00 V"));

    transport = HAL_BUSY;
    before = clears;
    unsigned before_pages = page_calls;
    measured.v_low = 12.1;
    settle();
    assert(clears == before && page_calls == before_pages);
    key(3);
    assert(stops == 1U && stopped_at_poll == 1U && page_calls == before_pages);
    transport = HAL_OK;
    service(0U);
    assert(last_page == 0U && clears == before + 1U);
    assert(strstr(drawn.rows[6], "12.10 V") && strstr(drawn.rows[7], "STOPPED"));
    transport = HAL_BUSY;
    before = clears;
    power_status.state = POWER_FAULT;
    power_status.fault = 64U;
    power_status.fault_detail = 3U;
    service(0U);
    assert(clears == before);
    transport = HAL_OK;
    service(0U);
    assert(last_page == 0U && strstr(drawn.rows[7], "FAULT:64 T3"));

    submission = HAL_BUSY;
    service(0U);
    unsigned held_page = last_page;
    submission = HAL_OK;
    service(0U);
    assert(last_page == held_page);
    transport = HAL_ERROR;
    before_pages = page_calls;
    before = scans;
    pending = 1U;
    key(4);
    assert(stops == 2U && stopped_at_poll == 2U && !pending);
    settle();
    assert(scans == before + 21U && page_calls == before_pages);

    transport = HAL_OK;
    submission = HAL_ERROR;
    service(0U);
    before_pages = page_calls;
    for (unsigned i = 0; i < 7U; ++i) service(0U);
    assert(page_calls == before_pages);
    service(0U);
    assert(page_calls == before_pages + 1U);
    unsigned old_starts = starts;
    key(3); settle(); /* Fault still forbids a nonzero start. */
    assert(starts == old_starts && power_status.fault == 64U);
}

#if POWER_EVENT_DIAGNOSTICS
static void test_event_progress_paths(void)
{
    setup();event_last_stage=event_seen=event_entries=0;
    pending=1;service(0);
    assert(event_last_stage==POWER_EVENT_UI_STARTUP_MUTED);
    pending=0;power_status.state=POWER_RUNNING;transport=HAL_BUSY;service(0);
    assert(event_last_stage==POWER_EVENT_UI_WAIT_DMA);
    transport=HAL_OK;submission=HAL_ERROR;service(0);
    assert(event_last_stage==POWER_EVENT_UI_IO_ERROR && event_entries==3);
    assert(event_seen&(1U<<POWER_EVENT_UI_RENDER));
    assert(event_seen&(1U<<POWER_EVENT_UI_SUBMIT_PAGE));
}
#endif
#if POWER_OLED_FRAME_RESUME
static void test_transient_starting_preserves_full_frame(void)
{
    setup();service(0);key(3);key(1);key(3);pending=0;
    power_status.state=POWER_BOOTSTRAP;service(0);
    power_status.state=POWER_RUNNING;service(0);
    unsigned frozen_clears=clears;
    assert(strstr(drawn.rows[6],"11.90 V"));
    measured.v_low=20.0;
    for(unsigned n=0;n<6;n++) {
        power_status.state=POWER_BOOTSTRAP;service(0);
        power_status.state=POWER_RUNNING;service(0);
        assert(clears==frozen_clears && strstr(drawn.rows[6],"11.90 V"));
    }
    power_status.state=POWER_BOOTSTRAP;service(0);
    power_status.state=POWER_RUNNING;service(0);
    assert(clears==frozen_clears && last_page==7);
    power_status.state=POWER_BOOTSTRAP;service(0);
    power_status.state=POWER_RUNNING;service(0);
    assert(clears==frozen_clears+1 && strstr(drawn.rows[6],"20.00 V"));
    memset(page_histogram,0,sizeof(page_histogram));
    for(unsigned n=0;n<400;n++) {
        power_status.state=POWER_BOOTSTRAP;service(0);
        power_status.state=POWER_RUNNING;service(0);
    }
    assert(page_histogram[7]>0);
    for(unsigned p=0;p<8;p++)assert(page_histogram[p]>0);
    power_status.state=POWER_FAULT;power_status.fault=16;service(0);
    assert(last_page==0 && strstr(drawn.rows[7],"FAULT:16"));
    power_status.state=POWER_STOPPED;power_status.fault=0;service(0);
    assert(last_page==0 && strstr(drawn.rows[7],"STOPPED"));
}
#endif
int main(int argc, char **argv)
{
#if POWER_OLED_FRAME_RESUME
    if(argc==1||!strcmp(argv[1],"frame"))test_transient_starting_preserves_full_frame();
#endif
#if POWER_EVENT_DIAGNOSTICS
    test_event_progress_paths();
#endif
    if (argc == 1 || !strcmp(argv[1], "startup")) test_startup_always_mutes_before_any_page_submission();
    if (argc == 1 || !strcmp(argv[1], "actions")) test_all_four_ui_actions();
    if (argc == 1 || !strcmp(argv[1], "zero")) test_zero_off_is_submitted_in_both_directions_and_fault();
    if (argc == 1 || !strcmp(argv[1], "transport")) test_running_frame_cadence_busy_stop_and_fault_restart();
    puts("Main UI: four modes, zero OFF, startup quiet, RUNNING 200 ms, busy/error STOP and fault page zero: PASS");
    return 0;
}
