/* Actual backend: continuous forward-style reverse regulation, no old HOLD. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main
static void run_worker(void) {before_call();Power_ControlIRQ();after_call();}
static void begin_mode(unsigned mode)
{
    stopped_samples(0,12,0);CHECK(request(mode,mode==3?6:18));
    for(unsigned n=0;n<33;n++)period(0,12,0);
    run_worker();CHECK(power_status.state==POWER_RUNNING);
}
int main(void)
{
    cold_boot();
    for(unsigned mode=3;mode<=4;mode++) {
        begin_mode(mode);
        const float target=mode==3?6:18;
        for(unsigned n=0;n<9000;n++) {
            /* Above old HOLD threshold, then the reported synthetic spike path.
             * All values remain below the independent electrical OVP. */
            float output=mode==3?6.1f:(n%60==0?18.2f:16.0f);
            period(output,12,0);run_worker();
            CHECK(!power_status.fault && power_status.state!=POWER_HOLD);
            CHECK(power_status.state!=POWER_BOOTSTRAP && power_status.state!=POWER_ARMING);
        }
        CHECK(power_status.state==POWER_RUNNING && enabled_outputs==ALL_OUTPUTS);
        CHECK(fabsf(power_status.v_reference-target)<0.0001f);
        CHECK(reverse_Y.CTRL_RUN_R && (mode==3 || reverse_Y.BOOST_RUN_R));
        period(mode==3?9.1f:27.1f,12,0);CHECK(power_status.fault==POWER_FAULT_OUTPUT);
        CHECK(enabled_outputs==0);run_worker();CHECK(enabled_outputs==0);
        begin_mode(mode);stop();run_worker();
        for(unsigned n=0;n<8;n++){period(0,12,0);run_worker();}
        CHECK(power_status.state==POWER_STOPPED && enabled_outputs==0);
    }
    puts("MAIN reverse continuous regulation: no HOLD/reference reset on target crossing or spikes; OVP and STOP retained: PASS");
    return 0;
}
