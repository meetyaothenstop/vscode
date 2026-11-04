/* Included only by power_control.c. No calls from healthy fast IRQ/worker. */
volatile PowerEventDiagnostics power_event;

static void event_publish(volatile PowerEventMailbox *mailbox, uint32_t stage,
                          uint32_t page, uint32_t status, unsigned entry)
{
    unsigned old=mailbox->active, next=old^1U;
    PowerEventProgress p=mailbox->slots[old];
    uint32_t cycles=DWT->CYCCNT, tick=HAL_GetTick();
    uint32_t samples=power_status.samples;
    if(entry) {
        uint32_t gap=cycles-p.entry_cycles;
        if(p.calls && gap>p.gap_cycles_max)p.gap_cycles_max=gap;
        uint32_t sample_gap=samples-p.entry_samples;
        if(p.calls && sample_gap>p.gap_samples_max)p.gap_samples_max=sample_gap;
        ++p.calls; p.entry_cycles=cycles; p.entry_tick=tick;
        p.entry_samples=samples;
    }
    p.stage=stage;p.stage_cycles=cycles;p.stage_tick=tick;p.page=page;p.io_status=status;
    p.stage_samples=samples;
    if(stage==POWER_EVENT_UI_PAGE_QUEUED) {
        ++p.page_submissions;p.page_cycles=cycles;p.page_tick=tick;p.last_page=page;
        p.page_samples=samples;
    }
    if(stage==POWER_EVENT_UI_FRAME_DONE) {
        ++p.frames_completed;p.frame_cycles=cycles;p.frame_tick=tick;p.frame_samples=samples;
    }
    mailbox->slots[next]=p;
    __DMB();
#if defined(POWER_TEST_EVENT_PUBLISH)
    extern void test_event_before_publish(void);
    test_event_before_publish();
#endif
    mailbox->active=next;
}
void PowerEvent_UIMark(uint32_t stage,uint32_t page,uint32_t status)
{ event_publish(&power_event.ui,stage,page,status,stage==POWER_EVENT_UI_ENTRY); }
static void event_foreground(void)
{ event_publish(&power_event.foreground,0U,0U,0U,1U); }
static void event_init(void)
{
    power_event=(PowerEventDiagnostics){0};
    power_event.version=2U;
    power_event.cpu_hz=HAL_RCC_GetHCLKFreq();
    power_event.sample_hz=PWM_FREQUENCY_HZ;
}
/* Existing stopped-state start lock excludes fault capture during reset. */
static void event_run_start(void)
{
    ++power_event.run_id;
    power_event.run_start_cycles=DWT->CYCCNT;
    power_event.run_start_tick=HAL_GetTick();
    power_event.run_start_samples=power_status.samples;
    power_event.foreground=(PowerEventMailbox){0};
    power_event.ui=(PowerEventMailbox){0};
}
/* Cold path: caller has already disabled outputs and holds the first-fault
 * lock. Read each published slot once before cancellation changes context. */
static __attribute__((noinline)) void event_capture(uint32_t code,uint32_t detail)
{
    volatile PowerEventFault *f=&power_event.fault;
    uint32_t cycles=DWT->CYCCNT,tick=HAL_GetTick();
    f->code=code;f->detail=detail;f->state=(uint32_t)power_status.state;
    f->mode=active_mode;f->samples=power_status.samples;f->run_id=power_event.run_id;
    f->capture_cycles=cycles;f->capture_tick=tick;
    f->clock_ready=power_event.clock_ready;
    f->run_age_cycles=power_event.run_id?cycles-power_event.run_start_cycles:0U;
    f->run_age_ticks=power_event.run_id?tick-power_event.run_start_tick:0U;
    f->run_age_samples=power_event.run_id?f->samples-power_event.run_start_samples:0U;
    f->start_pending=start_pending;f->bootstrap_resume=bootstrap_resume;
    f->bootstrap_remaining=power_status.state==POWER_BOOTSTRAP?bootstrap_cycles:0U;
    f->arming_remaining=power_status.state==POWER_ARMING?arming_cycles:0U;
    f->resume_pending=resume_bootstrap_pending;f->polarity_samples=reverse_polarity_cycles;
    f->target_volts=active_target;f->duty=power_status.d_cmd;f->reference_volts=power_status.v_reference;
    f->left_compare=power_status.left_compare;f->right_compare=power_status.right_compare;
    unsigned fg=power_event.foreground.active,ui=power_event.ui.active;
    __DMB();
    f->foreground=power_event.foreground.slots[fg];
    f->ui=power_event.ui.slots[ui];
    f->foreground_age_cycles=f->foreground.calls?cycles-f->foreground.entry_cycles:0U;
    f->foreground_age_ticks=f->foreground.calls?tick-f->foreground.entry_tick:0U;
    f->ui_entry_age_cycles=f->ui.calls?cycles-f->ui.entry_cycles:0U;
    f->ui_entry_age_ticks=f->ui.calls?tick-f->ui.entry_tick:0U;
    f->ui_progress_age_cycles=f->ui.stage?cycles-f->ui.stage_cycles:0U;
    f->ui_progress_age_ticks=f->ui.stage?tick-f->ui.stage_tick:0U;
    f->ui_page_age_cycles=f->ui.page_submissions?cycles-f->ui.page_cycles:0U;
    f->ui_page_age_ticks=f->ui.page_submissions?tick-f->ui.page_tick:0U;
    f->foreground_age_samples=f->foreground.calls?f->samples-f->foreground.entry_samples:0U;
    f->ui_entry_age_samples=f->ui.calls?f->samples-f->ui.entry_samples:0U;
    f->ui_progress_age_samples=f->ui.stage?f->samples-f->ui.stage_samples:0U;
    f->ui_page_age_samples=f->ui.page_submissions?f->samples-f->ui.page_samples:0U;
    f->ui_frame_age_cycles=f->ui.frames_completed?cycles-f->ui.frame_cycles:0U;
    f->ui_frame_age_ticks=f->ui.frames_completed?tick-f->ui.frame_tick:0U;
    f->ui_frame_age_samples=f->ui.frames_completed?f->samples-f->ui.frame_samples:0U;
    __DMB();f->valid=1U;
}
