#include "reverse_main_boost.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern reverse_ExternalInputs boost_reference_reverse_U;
extern reverse_ExternalOutputs boost_reference_reverse_Y;
extern reverse_BlockOutputs boost_reference_reverse_B;
extern reverse_ModelStates boost_reference_reverse_X;
extern const char *boost_reference_reverse_errorStatus;
extern void boost_reference_reverse_configure(float,float,float);
extern void boost_reference_reverse_initialize(float);
extern void boost_reference_reverse_step(void);
extern void boost_reference_reverse_hold_step(void);
extern void boost_reference_reverse_prepare_resume(float);
#if POWER_MULTIRATE_CONTROL
extern unsigned reverse_voltage_updates,boost_reference_reverse_voltage_updates;
#endif
static unsigned samples;
static void compare(void)
{
    assert(!reverse_errorStatus && !boost_reference_reverse_errorStatus);
    assert(memcmp(&reverse_Y,&boost_reference_reverse_Y,sizeof(reverse_Y))==0);
    assert(memcmp(&reverse_B,&boost_reference_reverse_B,sizeof(reverse_B))==0);
    assert(memcmp(&reverse_X,&boost_reference_reverse_X,sizeof(reverse_X))==0);
#if POWER_MULTIRATE_CONTROL
    assert(reverse_voltage_updates==boost_reference_reverse_voltage_updates);
#endif
    ++samples;
}
static void initialize(float input,float target)
{
    reverse_configure(input,target,6);boost_reference_reverse_configure(input,target,6);
    reverse_initialize(0);boost_reference_reverse_initialize(0);
}
int main(void)
{
    uint32_t random=0x6B91U;
    const float inputs[]={9.1f,12,12,24,35,12},targets[]={18,18,36,36,36,27};
    for(unsigned run=0;run<12;run++) {
        float input=inputs[run%6],target=targets[run%6];
        reverse_configure(0.1f,6,6);reverse_initialize(0);
        reverse_U=(reverse_ExternalInputs){.V_right=12,.mode=3};reverse_step();
        initialize(input,target);
        unsigned precharge=0,boost=0;
        for(unsigned step=0;step<30000;step++) {
            random=random*1664525U+1013904223U;
            float output=step<100?0:(step<600?input*(float)(step-100)/500:target);
            /* Let even the150kHz compatibility filter latch near35V before
             * randomized output replaces the physically plausible ramp. */
            if(run>=6 && step>=1000)output=(float)(random%45001U)*0.001f;
            float current=run>=6?0.98f-(float)((random>>16)%8401U)*0.001f:-0.06f;
            reverse_U=(reverse_ExternalInputs){.V_right=input,.V_left=output,.IL=current,.mode=4};
            boost_reference_reverse_U=reverse_U;
            if(step==20000) {
                reverse_prepare_resume(output);boost_reference_reverse_prepare_resume(output);
            }
            if(step>=18000 && step<20000) {
                reverse_main_boost_hold_step();boost_reference_reverse_hold_step();
            } else {reverse_main_boost_step();boost_reference_reverse_step();}
            compare();precharge|=reverse_Y.PRECHARGE_RUN_R;boost|=reverse_Y.BOOST_RUN_R;
        }
        if(!precharge || !boost)fprintf(stderr,"run%u input%g target%g precharge%u boost%u\n",run,input,target,precharge,boost);
        assert(precharge && boost);
    }
    const float rejected[]={0,1,2,3,NAN};
    for(unsigned n=0;n<5;n++) {
        initialize(12,18);reverse_U=(reverse_ExternalInputs){.V_right=12,.mode=4};
        reverse_main_boost_step();reverse_U.mode=rejected[n];reverse_main_boost_step();
        assert(reverse_errorStatus && reverse_Y.D_cm_R==0);
        assert(!reverse_Y.EN_R_BUCK && !reverse_Y.EN_R_BOOST && !reverse_Y.CTRL_RUN_R);
        assert(!reverse_Y.BOOST_RUN_R && !reverse_Y.D_cm_R4 && !reverse_Y.PRECHARGE_RUN_R);
        reverse_U.mode=4;reverse_main_boost_step();assert(reverse_errorStatus && reverse_Y.D_cm_R==0);
    }
    printf("Reverse Boost vs independent70e5a84 same-rate algorithm: %u B/X/Y bitwise steps, HOLD/resume/reinit/mode guard PASS\n",samples);
    return 0;
}
