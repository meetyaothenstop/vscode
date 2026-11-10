/* Independent pinned FORWARD algorithm, physical reverse adapter under test. */
#include "reverse.h"
#include "forward.h"
#include "reverse_main_buck.h"
#include "reverse_main_boost.h"
#include "forward_main_boost.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern forward_ExternalInputs mirror_reference_forward_U;
extern forward_ExternalOutputs mirror_reference_forward_Y;
extern forward_BlockOutputs mirror_reference_forward_B;
extern forward_ModelStates mirror_reference_forward_X;
extern const char *mirror_reference_forward_errorStatus;
extern void mirror_reference_forward_configure(float,float,float);
extern void mirror_reference_forward_initialize(float);
extern void mirror_reference_forward_step(void);
extern void mirror_reference_forward_main_buck_step(void);
extern void mirror_reference_forward_main_boost_step(void);
extern void forward_main_buck_step(void);
#if POWER_MULTIRATE_CONTROL
extern unsigned reverse_voltage_updates,mirror_reference_forward_voltage_updates;
#endif
static unsigned count;
static void pair_init(float input,float target,unsigned mode)
{
    float start=mode==3?0.1f:input;
    reverse_configure(start,target,6);reverse_initialize(0);
    forward_configure(start,target,6);forward_initialize(0);
    mirror_reference_forward_configure(start,target,6);mirror_reference_forward_initialize(0);
}
static void compare(void)
{
    const forward_ExternalOutputs *y=&mirror_reference_forward_Y;
    assert(!reverse_errorStatus && !mirror_reference_forward_errorStatus);
    assert(!forward_errorStatus);
    assert(memcmp(&forward_B,&mirror_reference_forward_B,sizeof(forward_B))==0);
    assert(memcmp(&forward_X,&mirror_reference_forward_X,sizeof(forward_X))==0);
    assert(memcmp(&forward_Y,&mirror_reference_forward_Y,sizeof(forward_Y))==0);
    /* First behavioral check catches the old reverse reference waiting for CTRL. */
    assert(reverse_B.Saturation4==mirror_reference_forward_B.Saturation4);
    assert(reverse_Y.D_cm_R==y->D_cmd && reverse_Y.EN_R_BUCK==y->EN_F_BUCK);
    assert(reverse_Y.EN_R_BOOST==y->EN_F_BOOST && reverse_Y.PRECHARGE_RUN_R==y->PRECCHARGE_RUN);
    assert(reverse_Y.D_cm_R4==y->PRECHARGE_DONE && reverse_Y.BOOST_RUN_R==y->BOOST_RUN);
    assert(reverse_Y.CTRL_RUN_R==y->CTRL_RUN);
    assert(sizeof(reverse_B)==sizeof(mirror_reference_forward_B));
    assert(sizeof(reverse_X)==sizeof(mirror_reference_forward_X));
    assert(memcmp(&reverse_B,&mirror_reference_forward_B,sizeof(reverse_B))==0);
    assert(memcmp(&reverse_X,&mirror_reference_forward_X,sizeof(reverse_X))==0);
#if POWER_MULTIRATE_CONTROL
    assert(reverse_voltage_updates==mirror_reference_forward_voltage_updates);
#endif
    ++count;
}
int main(void)
{
    const unsigned modes[]={4,3,4,3,4,3};
    const float inputs[]={12,12,24,24,9.1f,40},targets[]={18,6,36,12,18,36};
    uint32_t random=0x98B21U;
    for(unsigned route=0;route<2;route++)for(unsigned run=0;run<12;run++) {
        unsigned mode=modes[run%6];float input=inputs[run%6],target=targets[run%6];
        pair_init(input,target,mode);
        unsigned precharge=0,boost=0;
        for(unsigned n=0;n<30000;n++) {
            random=random*1664525U+1013904223U;
            float output=n<100?0:(n<1000?target*(float)(n-100)/900:target);
            float physical_current=-0.06f;
            if(run>=6 && n>=1000) {
                output=(random%45001U)*0.001f;
                physical_current=0.98f-((random>>16)%8401U)*0.001f;
            }
            reverse_U=(reverse_ExternalInputs){.V_right=input,.V_left=output,.IL=physical_current,.mode=(float)mode};
            mirror_reference_forward_U=(forward_ExternalInputs){.V_left=input,.V_right=output,.IL=-physical_current,.mode=(float)(mode-2)};
            forward_U=mirror_reference_forward_U;
            if(n==20000)reverse_prepare_resume(output); /* MAIN compatibility no-op. */
            if(route) {
                if(mode==3){if(n%7==0)reverse_main_buck_hold_step();else reverse_main_buck_step();mirror_reference_forward_main_buck_step();}
                else {if(n%7==0)reverse_main_boost_hold_step();else reverse_main_boost_step();mirror_reference_forward_main_boost_step();}
            } else {if(n%7==0)reverse_hold_step();else reverse_step();mirror_reference_forward_step();}
            if(route){if(mode==3)forward_main_buck_step();else forward_main_boost_step();}
            else forward_step();
            compare();precharge|=reverse_Y.PRECHARGE_RUN_R;boost|=reverse_Y.BOOST_RUN_R;
        }
        if(mode==4)assert(precharge && boost);
    }
    printf("MAIN reverse equals independent pinned forward: %u complete B/X/Y steps, Buck/Boost public/protected/random/reinit PASS\n",count);
    return 0;
}
