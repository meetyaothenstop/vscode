/* Real production backend with independent early/late ADC register inputs. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main
#include "power_event_diag.h"
#define VALID_LATE (ADC_ISR_EOC | ADC_ISR_EOS)
static void worker(void)
{
#if POWER_MULTIRATE_CONTROL
    before_call(); Power_ControlIRQ(); after_call();
#endif
}
static void sample(float early, float late, float right, uint32_t flags)
{
    early_input_raw=raw_voltage(early,ADC_LEFT_VOLTAGE_GAIN);
    override_early_input=1U;next_input_flags=flags;
    period(late,right,0);
}
static void start(unsigned mode,unsigned phase)
{
    stopped_samples(12,0,0);CHECK(request(mode,mode==1?6:18));
    for(unsigned n=0;n<phase;n++) {period(12,0,0);worker();CHECK(!power_status.fault);}
}
int main(void)
{
    const uint32_t invalid[]={0,ADC_ISR_EOC,ADC_ISR_EOS,VALID_LATE|ADC_ISR_OVR};
    const unsigned phases[]={0,1,30,31,32,33};
    cold_boot();
    for(unsigned mode=1;mode<=2;mode++) {
        /* Both request paths must choose late input even if ADC1 is low. */
        stopped_samples(12,0,0);sample(8.9f,12,0,VALID_LATE);
        CHECK(request(mode,mode==1?6:18));
        stopped_samples(12,0,0);sample(12,8.9f,0,VALID_LATE);
        CHECK(!request(mode,mode==1?6:18) && power_status.fault_detail==4);
        for(unsigned bad=0;bad<4;bad++) {
            stopped_samples(12,0,0);sample(12,12,0,invalid[bad]);
            CHECK(!request(mode,mode==1?6:18) && power_status.fault_detail==2);
        }
        for(unsigned phase=0;phase<6;phase++) {
            start(mode,phases[phase]);sample(8.9f,12,0,VALID_LATE);worker();
            CHECK(!power_status.fault && power_status.raw_vh==raw_voltage(12,ADC_LEFT_VOLTAGE_GAIN));
            CHECK(Power_SamplesAreValid());
            start(mode,phases[phase]);sample(12,8.9f,0,VALID_LATE);
            CHECK(power_status.fault==POWER_FAULT_INPUT && enabled_outputs==0);
            CHECK(fabsf(power_status.fault_input_volts-8.9f)<0.02f);
            for(unsigned bad=0;bad<4;bad++) {
                start(mode,phases[phase]);sample(12,12,0,invalid[bad]);
                CHECK(power_status.fault==POWER_FAULT_ADC && !Power_SamplesAreValid());
                CHECK(enabled_outputs==0 && !Power_IsStartPending());
#if POWER_ADC_FAULT_DETAILS
                CHECK(power_status.fault_detail==POWER_ADC_REASON_LATE_INVALID);
                CHECK(power_event.fault.adc2_entry_flags==invalid[bad]);
#endif
                worker();CHECK(enabled_outputs==0);
            }
        }
        /* The real worker/controller must consume the same selected source. */
        start(mode,33);
        for(unsigned n=0;n<7;n++) {sample(18,11.5f,0,VALID_LATE);worker();CHECK(!power_status.fault);}
        CHECK(fabsf(forward_U.V_left-11.5f)<0.02f);
    }
    for(unsigned bad=0;bad<4;bad++) {
        start(2,33);
#if POWER_MULTIRATE_CONTROL
        for(unsigned n=0;n<600 && power_status.state!=POWER_CHANGEOVER;n++) {
            sample(8.9f,12,11.99f,VALID_LATE);worker();CHECK(!power_status.fault);
        }
        CHECK(power_status.state==POWER_CHANGEOVER && enabled_outputs==0);
#else
        /* The legacy synchronous forward backend has no external
         * POWER_CHANGEOVER state; retain its existing controller delay. */
        for(unsigned n=0;n<600 && !forward_Y.BOOST_RUN;n++) {
            sample(8.9f,12,11.99f,VALID_LATE);CHECK(!power_status.fault);
        }
        CHECK(forward_Y.BOOST_RUN && power_status.state==POWER_RUNNING);
#endif
        sample(12,12,11.99f,invalid[bad]);worker();
        CHECK(power_status.fault==POWER_FAULT_ADC && enabled_outputs==0);
    }
    /* Reverse left bus is output; it must retain ADC1, even if ADC2 is lost
     * or reports a charged left bus that would fail cold-start interlocks. */
    for(unsigned mode=3;mode<=4;mode++) {
        stopped_samples(0,12,0);sample(0,20,12,0);
        CHECK(request(mode,mode==3?6:18));
        for(unsigned n=0;n<40;n++) {sample(0,20,12,0);worker();CHECK(!power_status.fault);}
        CHECK(power_status.state==POWER_RUNNING && power_status.raw_vh==0);
        CHECK(Power_SamplesAreValid() && reverse_U.V_left==0);
    }
    puts("Forward input ADC2: request, validity, every startup/run phase, changeover and worker; reverse ADC1 preserved: PASS");
    return 0;
}
