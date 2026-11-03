/* Use the existing register fixture, but drive the real fast IRQ and worker
 * separately. A synchronous controller call in the fast IRQ must fail. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main
#if defined(BASELINE_TEST)
void Power_ControlIRQ(void) { }
#else
void Power_ControlIRQ(void);
#endif
static void worker(void) { before_call(); Power_ControlIRQ(); after_call(); }
#if defined(POWER_TEST_WORKER_PREEMPT)
static unsigned preempt_worker;
static unsigned deferred_fault;
void test_irq_unmask(void)
{if(deferred_fault){deferred_fault=0U;period(12,0,7.6f);}}
DWT_Type *test_dwt_access(void)
{
    if(preempt_worker && --preempt_worker==0U) {
        /* Preempt just before ready publication, after the epoch check. */
        if(test_primask)deferred_fault=1U;
        else period(12,0,7.6f);
    }
    return &test_dwt;
}
#endif
static void run_start(unsigned mode)
{
    float l=mode>=3U?0.0f:12.0f,r=mode>=3U?12.0f:0.0f;
    stopped_samples(l,r,0.0f); CHECK(request(mode,6.0f));
    for(unsigned n=0;n<33;n++) period(l,r,0.0f);
    CHECK(power_status.state==POWER_RUNNING);
}
int main(void)
{
    cold_boot();
    stopped_samples(12,0,0); protection_ready=0U;
    CHECK(request(1U,6)); /*explicitly authorized ADC-only policy*/
    stop(); protection_ready=1U;
    run_start(1U);
    CHECK(forward_B.Saturation4==0.0f); /*fast IRQ only queued work*/
    uint32_t cmp=HRTIM1->sTimerxRegs[TIMER_B].CMP1xR;
    worker(); CHECK(forward_B.Saturation4>0.0f);
    CHECK(HRTIM1->sTimerxRegs[TIMER_B].CMP1xR==cmp);
    run_start(1U); worker();
    for(unsigned n=0;n<300;n++) {period(12,0,0);worker();}
    CHECK(power_status.control_steps==101U && power_status.voltage_steps==21U);
    run_start(1U); worker(); irq(12,0,0,ADC_ISR_JEOS,1000,8000);
    CHECK(power_status.fault==0U); /*normal permissible entry phase*/
    const float bad[][3]={{9.9f,0,0},{42.1f,0,0},{12,9.1f,0},{12,0,-1.1f}};
    const uint32_t faults[]={POWER_FAULT_INPUT,POWER_FAULT_INPUT,POWER_FAULT_OUTPUT,POWER_FAULT_REVERSE};
    for(unsigned n=0;n<4;n++) {
        run_start(1U); worker(); period(bad[n][0],bad[n][1],bad[n][2]);
        expect_fault(faults[n]); worker(); CHECK(enabled_outputs==0U);
    }
    run_start(1U); worker(); irq(12,0,0,0,1000,640);
    expect_fault(POWER_FAULT_ADC); worker(); CHECK(enabled_outputs==0U);
#if defined(POWER_TEST_WORKER_PREEMPT)
    run_start(1U); preempt_worker=2U; worker();
    expect_fault(POWER_FAULT_CURRENT);
    CHECK(fabsf(power_status.fault_il-7.6f)<0.01f);
    cmp=HRTIM1->sTimerxRegs[TIMER_B].CMP1xR;
    for(unsigned n=0;n<4;n++) {period(12,0,0);worker();}
    CHECK(enabled_outputs==0U && HRTIM1->sTimerxRegs[TIMER_B].CMP1xR==cmp);
    run_start(1U); worker(); period(12,0,0);
    CHECK(power_status.fault==0U && power_status.control_steps==1U);
#endif
    run_start(1U); worker();
    period(12,0,0);
    CHECK(power_status.v_reference>0.0f); /*publish at next fast boundary*/
    CHECK(power_status.control_steps==1U);
    period(12,0,0); worker(); CHECK(power_status.control_steps==1U);
    period(12,0,0); CHECK(power_status.fault==0U); worker();
    CHECK(power_status.control_steps==2U);
    run_start(1U); /*worker deliberately withheld*/
    for(unsigned n=0;n<3;n++) period(12,0,0);
    expect_fault(POWER_FAULT_TIMING); CHECK(power_status.fault_detail==7U);
    worker(); CHECK(enabled_outputs==0U);
    run_start(1U); worker();
    period(12,0,7.6f); expect_fault(POWER_FAULT_CURRENT);
    worker(); CHECK(enabled_outputs==0U);
    run_start(1U); worker(); cmp=HRTIM1->sTimerxRegs[TIMER_B].CMP1xR;
    stop(); worker();
    for(unsigned n=0;n<5;n++) {period(12,0,0); worker();}
    CHECK(enabled_outputs==0U && power_status.state==POWER_STOPPED);
    CHECK(HRTIM1->sTimerxRegs[TIMER_B].CMP1xR==cmp);
    for(unsigned mode=1;mode<=4;mode++) {
        /*Boost needs a target above source. Buck cadence is tested here;
         * changeover uses a dedicated charged-output sequence below.*/
        if(mode==2U||mode==4U) continue;
        run_start(mode);
        for(unsigned n=0;n<60;n++) {
            worker(); period(mode==3U?0.0f:12.0f,mode==3U?12.0f:0.0f,0.0f);
            CHECK(power_status.fault==0U);
        }
    }
    run_start(3U); worker();
    for(unsigned n=0;n<8 && power_status.state!=POWER_HOLD;n++) {period(6.04f,12,0);worker();}
    CHECK(power_status.state==POWER_HOLD && enabled_outputs==0U);
    for(unsigned n=0;n<8 && power_status.state!=POWER_BOOTSTRAP;n++) {period(5.8f,12,0);worker();}
    CHECK(power_status.state==POWER_BOOTSTRAP && enabled_outputs==0U);
    period(5.8f,12,0); worker(); CHECK(enabled_outputs==LOW_OUTPUTS);
    for(unsigned n=0;n<29;n++) {period(5.8f,12,0);worker();CHECK(power_status.state==POWER_BOOTSTRAP);}
    period(5.8f,12,0);worker(); CHECK(power_status.state==POWER_ARMING);
    period(5.8f,12,0);worker(); CHECK(power_status.state==POWER_ARMING);
    period(5.8f,12,0);worker(); CHECK(power_status.state==POWER_RUNNING && enabled_outputs==ALL_OUTPUTS);
    /* A fresh sample can cancel staged recovery before low-side enable. */
    run_start(3U); worker();
    for(unsigned n=0;n<8 && power_status.state!=POWER_HOLD;n++) {period(6.04f,12,0);worker();}
    CHECK(power_status.state==POWER_HOLD);
    for(unsigned n=0;n<8 && power_status.state!=POWER_BOOTSTRAP;n++) {period(5.8f,12,0);worker();}
    CHECK(power_status.state==POWER_BOOTSTRAP && enabled_outputs==0U);
    period(6.0f,12,0);worker();
    CHECK(power_status.state==POWER_HOLD && enabled_outputs==0U);
    /* Both Boost directions must go all-off during the20us handover delay. */
    for(unsigned mode=2;mode<=4;mode+=2) {
        stopped_samples(mode==4?0:12,mode==4?12:0,0); CHECK(request(mode,24));
        for(unsigned n=0;n<33;n++) period(mode==4?0:12,mode==4?12:0,0);
        for(unsigned n=0;n<240 && power_status.state!=POWER_CHANGEOVER;n++) {
            period(mode==4?11.99f:12,mode==4?12:11.99f,0);
            /*Complete transition request late, then following request fast.*/
            if(n%3U==1U)worker();
        }
        CHECK(power_status.state==POWER_CHANGEOVER && enabled_outputs==0U);
        worker();
        for(unsigned n=0;n<2;n++) {
            period(mode==4?11.99f:12,mode==4?12:11.99f,0);worker();
            CHECK(power_status.state==POWER_CHANGEOVER && enabled_outputs==0U);
        }
        period(mode==4?11.99f:12,mode==4?12:11.99f,0);worker();
        CHECK(power_status.state==POWER_RUNNING && enabled_outputs==ALL_OUTPUTS);
        stop();worker();period(12,12,0);worker();
        CHECK(power_status.state==POWER_STOPPED && enabled_outputs==0U);
    }
    puts("Separated fast/worker IRQs, three-period deadline, protection and stop: PASS");
    return 0;
}
