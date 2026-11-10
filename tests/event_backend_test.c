#define main legacy_test_main
#include "power_control_test.c"
#undef main
#include "power_event_diag.h"

static unsigned interrupt_publish;
void test_event_before_publish(void)
{
    if(interrupt_publish) {interrupt_publish=0;period(0,12,7.6f);}
}
static void worker(void) {before_call();Power_ControlIRQ();after_call();}
int main(int argc,char **argv)
{
    cold_boot();
    if(argc>1 && argv[1][0]=='4') {
        stopped_samples(0,12,0);CHECK(request(3,6));period(0,12,0);
        AdcMeasurements m;Power_Service(&m);
        PowerEvent_UIMark(POWER_EVENT_UI_ENTRY,0,HAL_OK);
        PowerEvent_UIMark(POWER_EVENT_UI_PAGE_QUEUED,7,HAL_OK);
        PowerEvent_UIMark(POWER_EVENT_UI_FRAME_DONE,7,HAL_OK);
        power_status.samples+=18000000U; /*120s of accepted150kHz ADC periods*/
        test_dwt.CYCCNT+=(uint32_t)(120ULL*150000000ULL);test_tick+=11484U;
        protection_latched=1;before_call();Power_FaultIRQ();after_call();
        CHECK(power_event.fault.foreground_age_samples==18000000U);
        CHECK(power_event.fault.ui_entry_age_samples==18000000U);
        CHECK(power_event.fault.ui_page_age_samples==18000000U);
        CHECK(power_event.fault.ui_frame_age_samples==18000000U);
        CHECK(power_event.fault.ui.frames_completed==1U);
        CHECK(power_event.fault.run_age_samples>=18000000U);
        CHECK(power_event.fault.foreground_age_ticks==11484U);
        puts("120s sample-period ages survive DWT wraps and slow HAL tick: PASS");return 0;
    }
    if(argc>1 && argv[1][0]=='1') {
        stopped_samples(0,12,0);CHECK(request(3,6));period(0,12,0);
        PowerEvent_UIMark(POWER_EVENT_UI_WAIT_DMA,4,HAL_BUSY);
        test_dwt.CYCCNT+=20;period(0,12,7.6f);
        CHECK(power_event.fault.ui.calls==0 && power_event.fault.ui.stage==POWER_EVENT_UI_WAIT_DMA);
        CHECK(power_event.fault.ui_progress_age_cycles==1020);
        CHECK(power_event.fault.ui_entry_age_cycles==0);
        puts("Post-start UI stage without a new entry retains its true age: PASS");return 0;
    }
    if(argc>1 && argv[1][0]=='2') {
        stopped_samples(0,12,0);CHECK(request(3,6));
        for(unsigned n=0;n<31;n++)period(0,12,0);
        period(0,12,7.6f);CHECK(power_event.fault.arming_remaining==2);
        stopped_samples(0,12,0);CHECK(request(3,6));period(0,12,0);
        period(0,12,7.6f);
        CHECK(power_event.fault.state==POWER_BOOTSTRAP && power_event.fault.bootstrap_remaining==30);
        CHECK(power_event.fault.arming_remaining==0);
        puts("Inactive phase countdowns never leak from a preceding failed run: PASS");return 0;
    }
    if(argc>1 && argv[1][0]=='3') {
        stopped_samples(0,12,0);CHECK(request(3,6));period(0,12,0);
        PowerEvent_UIMark(POWER_EVENT_UI_ENTRY,0,HAL_OK);
        PowerEvent_UIMark(POWER_EVENT_UI_PAGE_QUEUED,3,HAL_OK);
        for(unsigned n=0;n<10;n++) {
            test_dwt.CYCCNT+=10;++test_tick;
            PowerEvent_UIMark(POWER_EVENT_UI_ENTRY,4,HAL_OK);
            PowerEvent_UIMark(POWER_EVENT_UI_WAIT_DMA,4,HAL_BUSY);
        }
        period(0,12,7.6f);
        CHECK(power_event.fault.ui.calls==11 && power_event.fault.ui.stage==POWER_EVENT_UI_WAIT_DMA);
        CHECK(power_event.fault.ui.page_submissions==1 && power_event.fault.ui.last_page==3);
        CHECK(power_event.fault.ui_page_age_cycles==1100 && power_event.fault.ui_page_age_ticks==10);
        CHECK(power_event.fault.ui_progress_age_cycles==1000);
        puts("Repeated busy UI entries preserve last accepted-page time independently: PASS");return 0;
    }
    CHECK(!power_event.fault.valid);
    stopped_samples(0,12,0);CHECK(request(3,6));
    period(0,12,0); /* BOOTSTRAP, thirty complete periods still required. */
    CHECK(power_status.state==POWER_BOOTSTRAP);
    test_tick=100;AdcMeasurements m;Power_Service(&m);
    test_dwt.CYCCNT+=50;test_tick=101;
    PowerEvent_UIMark(POWER_EVENT_UI_ENTRY,0,HAL_OK);
    test_dwt.CYCCNT+=20;
    PowerEvent_UIMark(POWER_EVENT_UI_WAIT_DMA,4,HAL_BUSY);
    period(0,12,7.6f);
    CHECK(power_event.fault.valid && enabled_outputs==0);
    CHECK(power_event.fault.clock_ready==1);
    CHECK(power_event.fault.state==POWER_BOOTSTRAP && power_event.fault.mode==3);
    CHECK(power_event.fault.bootstrap_remaining==30 && !power_event.fault.arming_remaining);
    CHECK(power_event.fault.target_volts==6 && power_event.fault.code==POWER_FAULT_CURRENT);
    CHECK(power_event.fault.foreground.calls==1 && power_event.fault.ui.calls==1);
    CHECK(power_event.fault.foreground_age_cycles==1070 && power_event.fault.foreground_age_ticks==1);
    CHECK(power_event.fault.ui_entry_age_cycles==1020 && power_event.fault.ui_progress_age_cycles==1000);
    CHECK(power_event.fault.ui.stage==POWER_EVENT_UI_WAIT_DMA && power_event.fault.ui.page==4);
    PowerEventFault saved=power_event.fault;
    Power_Service(&m);PowerEvent_UIMark(POWER_EVENT_UI_IO_ERROR,7,HAL_ERROR);
    period(0,12,0);Power_Stop();
    CHECK(!memcmp(&saved,(const void*)&power_event.fault,sizeof(saved)));
    protection_active=1;clear_fault();
    CHECK(!memcmp(&saved,(const void*)&power_event.fault,sizeof(saved)));
    protection_active=protection_latched=0;clear_fault();CHECK(!power_event.fault.valid);

    stopped_samples(0,12,0);CHECK(request(3,6));
    for(unsigned n=0;n<33;n++)period(0,12,0);
    worker();period(0,12,0);worker();
    CHECK(power_event.foreground.slots[power_event.foreground.active].calls==0);
    for(unsigned n=0;n<16;n++){period(5.2f,12,1.23f);worker();}
    CHECK(power_event.fault.code==POWER_FAULT_REVERSE && power_event.fault.state==POWER_RUNNING);
    CHECK(power_event.fault.polarity_samples==16 && !power_event.fault.foreground_age_cycles);
    CHECK(power_event.fault.reference_volts==power_status.v_reference);
    CHECK(power_event.fault.duty==power_status.d_cmd);
    CHECK(power_event.fault.left_compare==power_status.left_compare);

    stopped_samples(0,12,0);CHECK(request(3,6));period(0,12,0);
    PowerEvent_UIMark(POWER_EVENT_UI_ENTRY,0,HAL_OK);
    interrupt_publish=1;
    PowerEvent_UIMark(POWER_EVENT_UI_RENDER,7,HAL_ERROR);
    CHECK(power_event.fault.ui.stage==POWER_EVENT_UI_ENTRY && power_event.fault.ui.page==0);
    CHECK(power_event.fault.ui.io_status==HAL_OK);

    stopped_samples(0,12,0);CHECK(request(3,6));
    test_dwt.CYCCNT=UINT32_MAX-99U;test_tick=UINT32_MAX;
    Power_Service(&m);
    test_dwt.CYCCNT=50;test_tick=1;
    Power_Service(&m);
    CHECK(power_event.foreground.slots[power_event.foreground.active].gap_cycles_max==150);
    CHECK(power_event.foreground.slots[power_event.foreground.active].calls==2);
    stopped_samples(0,12,0);CHECK(request(3,6));
    for(unsigned n=0;n<31;n++)period(0,12,0);
    CHECK(power_status.state==POWER_ARMING);
    period(0,12,7.6f);
    CHECK(power_event.fault.state==POWER_ARMING && power_event.fault.arming_remaining==2);
    stopped_samples(0,12,0);CHECK(request(3,6));
    for(unsigned n=0;n<33;n++)period(0,12,0);
    worker();period(0,12,0);worker();period(6.1f,12,0);worker();
    CHECK(power_status.state==POWER_RUNNING);
    period(6.1f,12,7.6f);CHECK(power_event.fault.state==POWER_RUNNING);
    stopped_samples(0,12,0);CHECK(request(3,6));
    for(unsigned n=0;n<33;n++)period(0,12,0);
    worker();period(0,12,0);worker();period(6.1f,12,0);worker();
    unsigned recovered=0;
    for(unsigned n=0;n<45;n++) {
        period(5.8f,12,0);worker();
        if(power_status.state==POWER_RUNNING){recovered=1;break;}
    }
    CHECK(recovered);
    period(5.8f,12,7.6f);
    CHECK(power_event.fault.state==POWER_RUNNING && power_event.fault.bootstrap_resume==0);
    CHECK(!power_event.fault.bootstrap_remaining && !power_event.fault.arming_remaining);
    stopped_samples(0,12,0);CHECK(request(3,6));period(0,12,0);
    power_event.clock_ready=0;period(0,12,7.6f);
    power_event.clock_ready=1;
    CHECK(power_event.fault.clock_ready==0);
    puts("Cold first-fault phase/ages, immutable clear, unpublished-slot IRQ and wrap: PASS");
    return 0;
}
