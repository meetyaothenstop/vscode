/* Real IRQ fixture: timebase, mode-specific deadline and inter-step safety. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main
#include "forward_main_boost.h"
#include "reverse_main_boost.h"
extern unsigned forward_voltage_updates;
extern unsigned reverse_voltage_updates;

static void worker(void) { before_call(); Power_ControlIRQ(); after_call(); }
static void start_mode(unsigned mode)
{
    stopped_samples(mode >= 3U ? 0 : 12, mode >= 3U ? 12 : 0, 0);
    CHECK(request(mode, mode == 2U || mode == 4U ? 18 : 6));
    for (unsigned n=0; n<33; ++n) period(mode >= 3U ? 0 : 12, mode >= 3U ? 12 : 0, 0);
    CHECK(power_status.state == POWER_RUNNING);
}
static void model(void)
{
    forward_configure(0.1f,18,6); forward_initialize(0);
    forward_U=(forward_ExternalInputs){.mode=2,.V_left=12,.V_right=1};
    forward_main_boost_step();
    CHECK(fabsf(forward_B.DiscreteTransferFcn1-0.11092144101852841f)<1e-6f);
    forward_main_boost_step();
    CHECK(fabsf(forward_B.DiscreteTransferFcn1-0.30815719090033147f)<1e-6f);
    forward_configure(0.1f,18,6); forward_initialize(0);
    forward_U=(forward_ExternalInputs){.mode=2,.V_left=12,.V_right=0};
    for(unsigned n=0;n<4;n++) forward_main_boost_step();
    float ramp=forward_B.Saturation4, precharge=forward_B.Saturation6;
    forward_main_boost_step();
    CHECK(fabsf(forward_B.Saturation4-ramp-0.0048f)<1e-6f); /*120V/s*40us*/
    CHECK(fabsf(forward_B.Saturation6-precharge-0.02f)<1e-6f); /*500A/s*/
    forward_configure(18,18,6);forward_initialize(0);
    forward_U=(forward_ExternalInputs){.mode=2,.V_left=12,.V_right=12};
    for(unsigned n=0;n<100 && !forward_Y.CTRL_RUN;n++) forward_main_boost_step();
    CHECK(forward_Y.CTRL_RUN);
    /* On enable voltage phase resets to1; the next update is five calls away. */
    float integral=forward_B.DiscreteIntegrator, previous_error=forward_B.Product2;
    float previous_state=forward_X.DiscreteTransferFcn;
    float alpha=forward_B.Saturation5;
    unsigned updates=forward_voltage_updates;
    for(unsigned n=0;n<4;n++)forward_main_boost_step();
    CHECK(forward_voltage_updates==updates);
    forward_main_boost_step();
    CHECK(forward_voltage_updates==updates+1U);
    CHECK(fabsf(forward_B.DiscreteIntegrator-integral-0.0002f*8*previous_error)<1e-5f);
    /*6238/(s+6238) Tustin at200us, evaluated from prior state.*/
    float filtered=0.38416061091267395f*12+0.47316247187175015f*previous_state;
    CHECK(fabsf(forward_B.Product2-(18-filtered))<1e-5f);
    CHECK(fabsf(forward_B.Saturation5-alpha-5*0.004f)<1e-6f); /*100/s*40us*/
    float current_integral=forward_B.DiscreteIntegrator1;
    float current_derivative=forward_X.DiscreteIntegrator1_i2_prevU;
    forward_main_boost_step();
    CHECK(fabsf(forward_B.DiscreteIntegrator1-current_integral-0.00004f*current_derivative)<1e-6f);
}
static void reverse_model(void)
{
    reverse_configure(0.1f,18,6);reverse_initialize(0);
    reverse_U=(reverse_ExternalInputs){.mode=4,.V_right=12,.V_left=1};
    reverse_main_boost_step();
    CHECK(fabsf(reverse_B.DiscreteTransferFcn2-0.11092144101852841f)<1e-6f);
    reverse_main_boost_step();
    CHECK(fabsf(reverse_B.DiscreteTransferFcn2-0.30815719090033147f)<1e-6f);
    reverse_configure(0.1f,18,6);reverse_initialize(0);
    reverse_U=(reverse_ExternalInputs){.mode=4,.V_right=12,.V_left=0};
    for(unsigned n=0;n<4;n++)reverse_main_boost_step();
    float ramp=reverse_B.Saturation4,precharge=reverse_B.DiscreteIntegrator4;
    reverse_main_boost_step();
    CHECK(reverse_B.Saturation4==ramp); /*Reverse reference waits for CTRL_RUN.*/
    CHECK(fabsf(reverse_B.DiscreteIntegrator4-precharge-0.02f)<1e-6f);
    reverse_U.V_left=12;
    for(unsigned n=0;n<100 && !reverse_Y.CTRL_RUN_R;n++)reverse_main_boost_step();
    CHECK(reverse_Y.CTRL_RUN_R);
    ramp=reverse_B.Saturation4;reverse_main_boost_step();
    CHECK(fabsf(reverse_B.Saturation4-ramp-0.0048f)<1e-6f);
    reverse_configure(18,18,6);reverse_initialize(0);
    reverse_U=(reverse_ExternalInputs){.mode=4,.V_right=12,.V_left=12};
    for(unsigned n=0;n<100 && !reverse_Y.CTRL_RUN_R;n++)reverse_main_boost_step();
    CHECK(reverse_Y.CTRL_RUN_R);
    float integral=reverse_X.Integrator_x;
    float state=reverse_X.DiscreteTransferFcn,alpha=reverse_B.DiscreteIntegrator5;
    unsigned updates=reverse_voltage_updates;
    for(unsigned n=0;n<4;n++)reverse_main_boost_step();
    CHECK(reverse_voltage_updates==updates);
    reverse_main_boost_step();CHECK(reverse_voltage_updates==updates+1);
    CHECK(fabsf(reverse_X.Integrator_x-integral-0.0002f*10*reverse_B.Product3)<1e-5f);
    float filtered=0.38416061091267395f*12+0.47316247187175015f*state;
    CHECK(fabsf(reverse_B.Product3-(18-filtered))<1e-5f);
    CHECK(fabsf(reverse_B.DiscreteIntegrator5-alpha-5*0.004f)<1e-6f);
    float current_integral=reverse_B.DiscreteIntegrator1;
    float derivative=reverse_X.DiscreteIntegrator1_i2_prevU;
    reverse_main_boost_step();
    CHECK(fabsf(reverse_B.DiscreteIntegrator1-current_integral-0.00004f*derivative)<1e-6f);
}
int main(void)
{
    cold_boot();
    for(unsigned mode=1;mode<=4;mode++) {
        unsigned interval=(mode==2U || mode==4U)?6U:3U;
        start_mode(mode); worker();
        for(unsigned n=1;n<interval;n++) {
            period(mode>=3?0:12,mode>=3?12:0,0);worker();
            CHECK(power_status.fault==0 && power_status.control_steps==1U);
        }
        period(mode>=3?0:12,mode>=3?12:0,0);worker();
        CHECK(power_status.control_steps==2U);
        start_mode(mode); /*Withhold worker: deadline must use this mode's cadence.*/
        for(unsigned n=1;n<interval;n++) {
            period(mode>=3?0:12,mode>=3?12:0,0); CHECK(!power_status.fault);
        }
        period(mode>=3?0:12,mode>=3?12:0,0);
        CHECK(power_status.fault==POWER_FAULT_TIMING && power_status.fault_detail==7);
        worker(); CHECK(enabled_outputs==0);
    }
    /*Every inter-request PWM, not only each sixth, must enforce fast safety.*/
    for(unsigned mode=2;mode<=4;mode+=2)for(unsigned phase=1;phase<6;phase++) {
        start_mode(mode);worker();
        for(unsigned n=1;n<phase;n++)period(mode==2?12:0,mode==2?0:12,0);
        period(mode==2?12:0,mode==2?0:12,mode==2?7.6f:-7.6f);
        CHECK(power_status.fault==POWER_FAULT_CURRENT && enabled_outputs==0);
        worker(); CHECK(enabled_outputs==0);
        start_mode(mode);worker();
        for(unsigned n=1;n<phase;n++)period(mode==2?12:0,mode==2?0:12,0);
        period(mode==2?8.9f:0,mode==2?0:8.9f,0);
        CHECK(power_status.fault==POWER_FAULT_INPUT && enabled_outputs==0);
        worker(); CHECK(enabled_outputs==0);
    }
    model();
    reverse_model();
    puts("Both Boost directions25/5kHz timebase; Buck50/10kHz; PWM150kHz safety: PASS");
    return 0;
}
