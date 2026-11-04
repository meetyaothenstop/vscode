#define main legacy_test_main
#include "power_control_test.c"
#undef main
#include "power_event_diag.h"

static void start_phase(unsigned mode,unsigned phase)
{
    float left=mode==1U?12.0f:0.0f,right=mode==1U?0.0f:12.0f;
    stopped_samples(left,right,0);
    CHECK(!power_event.fault.valid);
    CHECK(request(mode,6));
    if(phase==1U)period(left,right,0);
    if(phase==2U) {
        for(unsigned n=0;n<35;n++) {
            period(left,right,0);
            before_call();Power_ControlIRQ();after_call();
        }
        CHECK(power_status.state==POWER_RUNNING);
    }
}
static void check_adc(unsigned reason,unsigned flags1,unsigned flags2)
{
    expect_fault(POWER_FAULT_ADC);
    CHECK(!enabled_outputs && !Power_SamplesAreValid());
    CHECK(power_event.fault.valid && power_event.fault.code==POWER_FAULT_ADC);
#if POWER_ADC_FAULT_DETAILS
    CHECK(power_status.fault_detail==reason);
    CHECK(power_event.fault.detail==reason && power_event.fault.adc_reason==reason);
    CHECK(power_event.fault.adc1_entry_flags==flags1);
    CHECK(power_event.fault.adc2_entry_flags==flags2);
#else
    (void)reason;(void)flags1;(void)flags2;
    CHECK(!power_status.fault_detail && !power_event.fault.detail);
    CHECK(!power_event.fault.adc_reason && !power_event.fault.adc1_entry_flags && !power_event.fault.adc2_entry_flags);
#endif
}
int main(void)
{
    cold_boot();
    CHECK(power_event.version==(POWER_ADC_FAULT_DETAILS?3U:2U));
    const unsigned flags[]={ADC_ISR_JEOC,ADC_ISR_JEOS|ADC_ISR_JEOC|ADC_ISR_JQOVF,ADC_ISR_JQOVF};
    const unsigned reasons[]={1U,2U,3U};
    for(unsigned phase=0;phase<3;phase++)for(unsigned i=0;i<3;i++) {
        start_phase(3,phase);
        next_input_flags=ADC_ISR_EOC|ADC_ISR_OVR;
        irq(0,12,0,flags[i],1000,640);
        check_adc(reasons[i],flags[i],ADC_ISR_EOC|ADC_ISR_OVR);
        CHECK(power_event.fault.state==(phase==0?POWER_STOPPED:phase==1?POWER_BOOTSTRAP:POWER_RUNNING));
        CHECK(power_event.fault.start_pending==(phase==0));
        PowerEventFault saved=power_event.fault;
        /* IRQ has written W1C masks; subsequent healthy/failing IRQs, service,
         * STOP and a rejected clear must not replace first-entry evidence. */
        period(0,12,0);irq(0,12,0,ADC_ISR_JQOVF,1000,640);service();stop();
        protection_active=1;clear_fault();protection_active=0;
        CHECK(!memcmp(&saved,(const void*)&power_event.fault,sizeof(saved)));
        clear_fault();
        PowerEventFault zero={0};
        CHECK(!memcmp(&zero,(const void*)&power_event.fault,sizeof(zero)));
    }
    const unsigned late_flags[]={0,ADC_ISR_EOC,ADC_ISR_EOS,ADC_ISR_EOC|ADC_ISR_EOS|ADC_ISR_OVR};
    for(unsigned phase=0;phase<3;phase++)for(unsigned i=0;i<4;i++) {
        start_phase(1,phase);next_input_flags=late_flags[i];
        irq(12,0,0,ADC_ISR_JEOS|ADC_ISR_JEOC,1000,640);
        check_adc(4U,ADC_ISR_JEOS|ADC_ISR_JEOC,late_flags[i]);
    }
    start_phase(3,1);next_input_flags=0;period(0,12,0);
    CHECK(!power_status.fault && !power_event.fault.valid);
    /* Context from a preceding ADC fault must not contaminate another code. */
    period(0,12,7.6f);expect_fault(POWER_FAULT_CURRENT);
    CHECK(power_event.fault.valid && !power_event.fault.adc_reason);
    CHECK(!power_event.fault.adc1_entry_flags && !power_event.fault.adc2_entry_flags);
    puts("ADC first-entry flags/reasons, START/bootstrap/run, immutable capture and clear: PASS");
    return 0;
}
