#define main legacy_test_main
#include "power_control_test.c"
#undef main
static void step(void) {before_call();Power_ControlIRQ();after_call();}
static unsigned unmask_action,disable_count;
void test_before_irq_disable(void)
{if(disable_count && --disable_count==0U)period(12,0,7.6f);}
void test_irq_unmask(void)
{
    unsigned action=unmask_action;unmask_action=0U;
    if(action==1U)period(12,0,0);
    if(action==2U)period(12,1,0.5f);
}
int main(int argc,char **argv)
{
    cold_boot();
    if(argc>1 && argv[1][0]=='4') {
        for(unsigned mode=1;mode<=3;mode+=2) {
            stopped_samples(mode==3?0:12,mode==3?12:0,0);CHECK(request(mode,6));
            for(unsigned n=0;n<33;n++)period(mode==3?0:12,mode==3?12:0,0);
            step();unsigned empty=0,pulses=0;
            for(unsigned n=0;n<150;n++) {
                period(mode==3?0:12,mode==3?12:0,0);step();
                unsigned source=mode==3?power_status.right_compare:power_status.left_compare;
                unsigned peer=mode==3?power_status.left_compare:power_status.right_compare;
                CHECK(peer==28160U && (source==0U||source==PWM_MIN_COMPARE_TICKS));
                if(source)pulses++;else empty++;
            }
            CHECK(pulses && empty);stop();unsigned saved=power_status.left_compare;
            period(mode==3?0:12,mode==3?12:0,0);step();
            CHECK(enabled_outputs==0U && power_status.left_compare==saved);
        }
        puts("Both PDM directions preserve peer compare and stop cancellation: PASS");return 0;
    }
    if(argc>1 && argv[1][0]=='1') {
        stopped_samples(12,0,0);next_input_flags=0U;period(9,0,0);
        unmask_action=1U;CHECK(!request(1,6));
        CHECK(power_status.fault_detail==2U);
        puts("Raw sample and validity form one request epoch: PASS");return 0;
    }
    if(argc>1 && argv[1][0]=='2') {
        stopped_samples(12,0,0);override_early_input=1U;early_input_raw=0U;period(12,0,0);
        unmask_action=2U;CHECK(!request(2,24));CHECK(power_status.fault_detail==4U);
        CHECK(power_status.fault_raw_vh==0U && power_status.fault_raw_vl==raw_voltage(0,0.04666f));
        CHECK(power_status.fault_raw_il==raw_current(0));
        CHECK(power_status.fault_v_left==0.0f);
        puts("Rejected start retains selected immutable ADC phase snapshot: PASS");return 0;
    }
    if(argc>1 && argv[1][0]=='3') {
        stopped_samples(12,0,0);CHECK(request(1,6));
        for(unsigned n=0;n<33;n++)period(12,0,0);
        disable_count=2U;step();expect_fault(POWER_FAULT_CURRENT);
        CHECK(power_status.control_steps==0U && power_status.control_cycles_max==0U);
        CHECK(power_status.voltage_steps==0U && fabsf(power_status.fault_il-7.6f)<0.01f);
        puts("Fault before final worker publication cancels telemetry and counters: PASS");return 0;
    }
    /* New timing fields distinguish healthy fast path from fault path. */
    CHECK(power_status.fast_run_cycles_max==0U);
    stopped_samples(12,0,0);CHECK(request(1,6));
    for(unsigned n=0;n<33;n++)period(12,0,0);
    test_dwt.CYCCNT+=100U;step();
    CHECK(power_status.control_start_delay_max==100U);
    CHECK(power_status.control_cycles_max==100U);
    CHECK(power_status.control_run_wall_cycles_max==0U);
    period(12,0,0);step();
    float previous=power_status.v_right;
    period(12,9.1f,0);
    expect_fault(POWER_FAULT_OUTPUT);
    CHECK(fabsf(power_status.fault_v_right-9.1f)<0.02f);
    CHECK(power_status.fault_v_right!=previous);
    step();
    stopped_samples(12,0,0);
    /* Request validation must use the latest raw sample, even without a
     * foreground telemetry refresh or a completed worker calculation. */
    period(9.0f,0,0);CHECK(!request(1,6));expect_fault(POWER_FAULT_START);
    stopped_samples(0,12,0); CHECK(request(3,6));
    for(unsigned n=0;n<33;n++)period(0,12,0);
    step();period(0,12,0);step();
    period(9.1f,12,-0.1f);expect_fault(POWER_FAULT_OUTPUT);
    CHECK(fabsf(power_status.fault_v_left-9.1f)<0.02f);
    CHECK(fabsf(power_status.fault_il+0.1f)<0.01f);
    puts("Raw request freshness, first-fault conversions and timing split: PASS");
}
