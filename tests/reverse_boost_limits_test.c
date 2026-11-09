/* Catch a mode4 request-only policy change or use of the wrong bus at runtime. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main

static void run_boost(float target)
{
    stopped_samples(0,12,0); CHECK(request(4,target));
    for(unsigned n=0;n<33;n++)period(0,12,0);
    CHECK(power_status.state==POWER_RUNNING && enabled_outputs==ALL_OUTPUTS);
}
int main(void)
{
    cold_boot();
    /* Reported8.99V must qualify mode4 at request and every fresh phase. */
    const unsigned accept_phases[]={0,1,30,31,32,33};
    const float accepted_input[]={7.1f,8.99f};
    for(unsigned v=0;v<2;v++) {
        stopped_samples(0,accepted_input[v],0);CHECK(request(4,18));
        for(unsigned p=0;p<6;p++) {
            stopped_samples(0,12,0);CHECK(request(4,18));
            for(unsigned n=0;n<accept_phases[p];n++)period(0,12,0);
            period(0,accepted_input[v],0);CHECK(!power_status.fault);
        }
    }
    for(unsigned sign=0;sign<2;sign++) {
        float current=sign?-0.98f:0.98f;
        stopped_samples(2.99f,12,current); CHECK(request(4,18));
        for(unsigned n=0;n<33;n++) {period(2.99f,12,current);CHECK(!power_status.fault);}
        CHECK(power_status.state==POWER_RUNNING);
        stopped_samples(0,12,sign?-1.02f:1.02f);
        CHECK(!request(4,18) && power_status.fault_detail==5);
    }
    stopped_samples(3.05f,12,0);CHECK(!request(4,18) && power_status.fault_detail==6);
    const unsigned phases[]={0,1,30,31,32};
    for(unsigned p=0;p<5;p++)for(unsigned kind=0;kind<5;kind++) {
        stopped_samples(0,12,0);CHECK(request(4,18));
        for(unsigned n=0;n<phases[p];n++)period(0,12,0);
        period(kind==4?3.05f:0,kind==0?6.9f:(kind==1?42.1f:12),
               kind==2?-1.02f:(kind==3?1.02f:0));
        CHECK(power_status.fault==(kind<2?POWER_FAULT_INPUT:POWER_FAULT_START));
        if(kind>=2)CHECK(power_status.fault_detail==(kind==4?9U:8U));
        CHECK(enabled_outputs==0 && HRTIM1->sCommonRegs.OENR==0);
    }
    /* OVP is left output, independently quantized using its calibration. */
    const float targets[]={18,26,27,36},trips[]={27,39,40,40};
    for(unsigned n=0;n<4;n++) {
        run_boost(targets[n]);
        unsigned below=(unsigned)floorf(trips[n]/ADC_LEFT_VOLTS_PER_COUNT);
        period(below*ADC_LEFT_VOLTS_PER_COUNT,12,0);CHECK(!power_status.fault);
        period((below+1)*ADC_LEFT_VOLTS_PER_COUNT,12,0);
        CHECK(power_status.fault==POWER_FAULT_OUTPUT && power_status.fault_raw_vh==below+1);
    }
    run_boost(36);period(0,41.9f,0);CHECK(!power_status.fault);
    period(0,42.1f,0);CHECK(power_status.fault==POWER_FAULT_INPUT);
    stopped_samples(0,12,0);
    float equal=(float)ADC1->JDR3*ADC_RIGHT_VOLTS_PER_COUNT;
    CHECK(!request(4,equal) && power_status.fault_detail==7);
    stopped_samples(0,12,0);CHECK(!request(4,0) && power_status.fault_detail==3);
    stopped_samples(0,12,0);CHECK(!request(4,36.1f) && power_status.fault_detail==3);
    run_boost(18);period(0,12,-7.6f);CHECK(power_status.fault==POWER_FAULT_CURRENT);
    run_boost(18);
    for(unsigned n=0;n<15;n++) {
        period(0,12,1.02f);CHECK(!power_status.fault);
        before_call();Power_ControlIRQ();after_call();
    }
    period(0,12,1.02f);CHECK(power_status.fault==POWER_FAULT_REVERSE && enabled_outputs==0);
    puts("Reverse Boost7..42V/1A/3V/cappedOVP; reported8.99V accepted, fresh checks and direction/current: PASS");
    return 0;
}
