#include "pwm_density.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    const struct {uint32_t inc,n,pulses,remainder;} cases[]={
        {0,10,0,0},{1,10,0,10},{8388608,10,5,0},
        {8388608,11,5,8388608},{16777215,10,9,16777206},{16777216,10,10,0}};
    for(unsigned c=0;c<sizeof(cases)/sizeof(cases[0]);c++) {
        uint32_t fraction=0,pulses=0;
        for(unsigned n=0;n<cases[c].n;n++)pulses+=PWM_DensityStep(&fraction,cases[c].inc);
        assert(pulses==cases[c].pulses && fraction==cases[c].remainder);
    }
    uint32_t fraction=16777215;assert(PWM_DensityStep(&fraction,1)==1 && fraction==0);
    puts("Q24 zero/tiny/half/near-full/full density and exact carry: PASS");
}
