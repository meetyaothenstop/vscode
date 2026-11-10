#include "forward.h"
#include "reverse_main_buck.h"
#include "reverse_main_boost.h"
#include "pwm_plan.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {reverse_ModelStates x;reverse_BlockOutputs b;reverse_ExternalOutputs y;} Snapshot;
static Snapshot golden[1200];
static void configure(unsigned n)
{
    if(n==0 || n==900){reverse_configure(12,18,6);reverse_initialize(0);}
    if(n==600){reverse_configure(0.1f,6,6);reverse_initialize(0);}
}
static void sample(unsigned n)
{
    unsigned buck=n>=600 && n<900;
    reverse_U=(reverse_ExternalInputs){.V_right=buck?24:12,
        .V_left=buck?3:(n<100?0:(n<400?12:18)),.IL=-0.2f,.mode=buck?3:4};
    if(buck)reverse_main_buck_step();else reverse_main_boost_step();
    assert(!reverse_errorStatus);
}
static void isolated_state(void)
{
    for(unsigned n=0;n<1200;n++) {
        configure(n);sample(n);
        golden[n]=(Snapshot){.x=reverse_X,.b=reverse_B,.y=reverse_Y};
    }
    for(unsigned n=0;n<1200;n++) {
        if(n%23==0){forward_configure(0.1f,5+(n%2),3);forward_initialize(0);}
        forward_U=(forward_ExternalInputs){.V_left=24,.V_right=2+(n%3),.IL=0.3f,.mode=1};
        forward_step();
        forward_ModelStates x=forward_X;forward_BlockOutputs b=forward_B;
        forward_ExternalOutputs y=forward_Y;const char *error=forward_errorStatus;
        configure(n);sample(n);
        assert(memcmp(&reverse_X,&golden[n].x,sizeof(reverse_X))==0);
        assert(memcmp(&reverse_B,&golden[n].b,sizeof(reverse_B))==0);
        assert(memcmp(&reverse_Y,&golden[n].y,sizeof(reverse_Y))==0);
        assert(memcmp(&forward_X,&x,sizeof(x))==0 && memcmp(&forward_B,&b,sizeof(b))==0);
        assert(memcmp(&forward_Y,&y,sizeof(y))==0 && forward_errorStatus==error);
    }
}
static void init(void)
{
    reverse_configure(12,18,6);reverse_initialize(0);
    reverse_U=(reverse_ExternalInputs){.V_right=12,.V_left=12,.IL=-0.0f,.mode=4};
}
static void rejected(void)
{
    reverse_ExternalOutputs zero={0};
    assert(reverse_errorStatus && memcmp(&reverse_Y,&zero,sizeof(zero))==0);
    reverse_U=(reverse_ExternalInputs){.V_right=12,.V_left=12,.IL=-0.2f,.mode=4};
    reverse_main_boost_step();assert(reverse_errorStatus && reverse_Y.D_cm_R==0);
    init();reverse_main_boost_step();assert(!reverse_errorStatus && reverse_Y.EN_R_BOOST);
}
static void validation(void)
{
    const float bad_modes[]={1,2,5,NAN,INFINITY};
    for(unsigned n=0;n<5;n++){init();reverse_U.mode=bad_modes[n];reverse_step();rejected();}
    init();reverse_U.mode=3;reverse_main_boost_step();rejected();
    init();reverse_U.mode=4;reverse_main_buck_step();rejected();
    const float bad[]={NAN,INFINITY,-INFINITY};
    for(unsigned field=0;field<3;field++)for(unsigned n=0;n<3;n++) {
        init();
        if(field==0)reverse_U.V_right=bad[n];
        if(field==1)reverse_U.V_left=bad[n];
        if(field==2)reverse_U.IL=bad[n];
        reverse_main_boost_step();rejected();
    }
    const float config[][3]={{0,18,6},{41,18,6},{12,0,6},{12,37,6},{12,18,0},{12,18,7},{NAN,18,6},{12,NAN,6},{12,18,NAN}};
    for(unsigned n=0;n<9;n++){reverse_configure(config[n][0],config[n][1],config[n][2]);reverse_initialize(0);rejected();}
    init();reverse_U.V_right=0;reverse_step();rejected();
    init();reverse_step();Snapshot previous={.x=reverse_X,.b=reverse_B,.y=reverse_Y};
    reverse_prepare_resume(16);
    assert(memcmp(&reverse_X,&previous.x,sizeof(reverse_X))==0 && memcmp(&reverse_B,&previous.b,sizeof(reverse_B))==0);
    reverse_U.mode=0;reverse_step();assert(!reverse_errorStatus && reverse_Y.D_cm_R==0 && !reverse_Y.CTRL_RUN_R);
}
static void mirror_pwm(void)
{
    for(unsigned boost=0;boost<2;boost++)for(unsigned n=0;n<=950;n++) {
        PwmPlan f,r;float duty=n==950?0.95f:n*0.001f;
        assert(PWM_MakePlan(duty,boost,&f) && PWM_MakeReversePlan(duty,boost,&r));
        assert(f.left_compare==r.right_compare && f.right_compare==r.left_compare);
        assert(f.left_high==r.right_high && f.right_high==r.left_high);
    }
}
int main(void)
{
    isolated_state();validation();mirror_pwm();
    puts("MAIN shared controller isolation, reverse mode/input/config rejection/latch/off and exact PWM mirror: PASS");
    return 0;
}
