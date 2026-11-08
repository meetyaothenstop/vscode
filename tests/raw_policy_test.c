#include "power_raw_policy.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void)
{
    const float targets[]={1,3,6,12,36};
    for(unsigned mode=1;mode<=4;mode++)for(unsigned t=0;t<5;t++) {
        float gain=mode>=3?ADC_LEFT_VOLTS_PER_COUNT:ADC_RIGHT_VOLTS_PER_COUNT;
        float igain=mode>=3?ADC_RIGHT_VOLTS_PER_COUNT:ADC_LEFT_VOLTS_PER_COUNT;
        int buck=mode==1||mode==3;
        int shared=mode!=4;
        float target=targets[t],ov=shared?fminf(40,fmaxf(6,target*1.5f)):target*1.1f;
        RawPolicy p; RawPolicy_Build(&p,mode,target,shared?9:6,shared?42:40,ov,shared?1.0f:0.25f,shared?3:0.25f);
        for(unsigned r=0;r<4096;r++) {
            float i=(float)r*ADC_CURRENT_AMPS_PER_COUNT-ADC_CURRENT_ZERO_AMPS;
            float v=(float)r*gain,vin=(float)r*igain;
            assert(Raw_Contains(p.input,r)==(vin>=(shared?9:6)&&vin<=(shared?42:40)&&(!buck||!(target>vin*POWER_MAIN_BUCK_MAX_DUTY))));
            assert(Raw_Contains(p.output,r)==!(v>ov));
            assert(Raw_Contains(p.current,r)==(fabsf(i)<7.5f&&r>5&&r<4090));
            assert(Raw_Contains(p.start_current,r)==(fabsf(i)<(shared?1.0f:0.25f)));
            assert(Raw_Contains(p.start_output,r)==(v<=(shared?3:0.25f)));
            assert(Raw_Contains(p.direction,r)==(mode>=3?i<=1.0f:i>=-1.0f));
            assert(Raw_Contains(p.hold_output,r)==(v>=target*1.005f));
            assert(Raw_Contains(p.resume_output,r)==(v<target*0.99f));
            assert(Raw_Contains(p.resume_current,r)==(fabsf(i)<0.25f));
        }
    }
    puts("Raw protection/hold/resume matches float predicates for all12-bit codes: PASS");
}
