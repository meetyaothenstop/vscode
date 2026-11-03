#ifndef POWER_RAW_POLICY_H
#define POWER_RAW_POLICY_H
#include "adc_measure.h"
#include "power_build_config.h"
#include <math.h>
typedef struct { unsigned lo,hi; } RawWindow;
typedef struct {
    RawWindow input,output,current,start_current,start_output,direction;
    RawWindow hold_output,resume_output,resume_current;
} RawPolicy;
static inline int Raw_Contains(RawWindow w,unsigned raw)
{ return raw>=w.lo && raw<=w.hi; }
static inline void Raw_Add(RawWindow *w,unsigned raw,int accepted)
{ if(accepted) {if(raw<w->lo)w->lo=raw;w->hi=raw;} }
/* Foreground only. Enumerate existing float decisions rather than rounding
 * inverse calibration and accidentally moving a strict protection boundary. */
static inline void RawPolicy_Build(RawPolicy *p,unsigned mode,float target,
    float imin,float imax,float ov,float start_i,float start_v)
{
    RawWindow empty={4096U,0U};
    *p=(RawPolicy){empty,empty,empty,empty,empty,empty,empty,empty,empty};
    int reverse=mode>=3U,buck=mode==1U||mode==3U;
    float vg=reverse?ADC_LEFT_VOLTS_PER_COUNT:ADC_RIGHT_VOLTS_PER_COUNT;
    float ig=reverse?ADC_RIGHT_VOLTS_PER_COUNT:ADC_LEFT_VOLTS_PER_COUNT;
    for(unsigned raw=0;raw<4096U;raw++) {
        float vin=(float)raw*ig,v=(float)raw*vg;
        float i=(float)raw*ADC_CURRENT_AMPS_PER_COUNT-ADC_CURRENT_ZERO_AMPS;
        Raw_Add(&p->input,raw,vin>=imin&&vin<=imax&&(!buck||!(target>vin*POWER_MAIN_BUCK_MAX_DUTY)));
        Raw_Add(&p->output,raw,!(v>ov));
        Raw_Add(&p->current,raw,fabsf(i)<POWER_CURRENT_TRIP_AMPS&&raw>5U&&raw<4090U);
        Raw_Add(&p->start_current,raw,fabsf(i)<start_i);
        Raw_Add(&p->start_output,raw,v<=start_v);
        Raw_Add(&p->direction,raw,reverse?i<=1.0f:i>=-1.0f);
        Raw_Add(&p->hold_output,raw,v>=target*1.005f);
        Raw_Add(&p->resume_output,raw,v<target*0.99f);
        Raw_Add(&p->resume_current,raw,fabsf(i)<0.25f);
    }
}
#endif
