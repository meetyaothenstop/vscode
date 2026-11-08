/* End-to-end nominal switched plant: real fast/slow backend and ADC scaling.
 * Does not model sensor transients, diode recovery, ESR or board WCET. */
#define main legacy_test_main
#include "power_control_test.c"
#undef main
static void plant(unsigned mode, double source, double target)
{
    const unsigned steps=200U;
    const double dt=1.0/(150000.0*steps), load=100.0;
    double v=0.0,i=0.0,sv=0.0,si=0.0,peak=0.0;
    unsigned start_steps=0,voltage_steps=0;
    stopped_samples(mode>=3U?0.0f:(float)source,mode>=3U?(float)source:0.0f,0);
    CHECK(request(mode,(float)target));
    for(unsigned n=0;n<90000U;n++) {
        period(mode>=3U?(float)sv:(float)source,mode>=3U?(float)source:(float)sv,(float)si);
#ifdef TRACE_PLANT
        if(mode==2U && n>3490U) fprintf(stderr,"n%u v%.5f si%.5f d%.5f ref%.5f iref%.5f int%.5f cmp%u/%u boost%u pre%u\n",n,v,si,power_status.d_cmd,power_status.v_reference,forward_B.Saturation,forward_B.DiscreteIntegrator1,(unsigned)active_left_compare,(unsigned)active_right_compare,forward_Y.BOOST_RUN,forward_Y.PRECHARGE_DONE);
#endif
        if(power_status.fault) {
            fprintf(stderr,"mode%u cycle%u v%.5f i%.5f fault%u/%u\n",mode,n,v,i,
                    (unsigned)power_status.fault,(unsigned)power_status.fault_detail);
            CHECK(!power_status.fault);
        }
        /* Worker publishes after fast IRQ; nearest safe fast IRQ writes
         * the result, then both compares preload at the following PWM. */
        before_call(); Power_ControlIRQ(); after_call();
        double dl=active_left_compare?((double)active_left_compare-960.0)/32000.0:0;
        double dr=active_right_compare?((double)active_right_compare-960.0)/32000.0:0;
        for(unsigned k=0;k<steps;k++) {
            double phase=(k+0.5)/steps;
            int l=enabled_outputs==ALL_OUTPUTS && phase>=0.03 && phase<0.03+dl;
            int r=enabled_outputs==ALL_OUTPUTS && phase>=0.03 && phase<0.03+dr;
            if(enabled_outputs!=ALL_OUTPUTS) {
                double left_bus=mode>=3U?v:source,right_bus=mode>=3U?source:v;
                /* Complementary low sides, when enabled, conduct after
                 * CMP1 plus deadtime until the next period reset. CMP1=0
                 * null duty keeps its low side on. Ideal diodes otherwise
                 * clamp positive IL to right bus and negative IL to left. */
                int ll=enabled_outputs==LOW_OUTPUTS && (!active_left_compare || phase>=active_left_compare/32000.0+0.03);
                int rl=enabled_outputs==LOW_OUTPUTS && (!active_right_compare || phase>=active_right_compare/32000.0+0.03);
                double next=i<0?fmin(0.0,i+dt*((ll?0:left_bus)-0.05*i)/22e-6):
                                fmax(0.0,i+dt*(-(rl?0:right_bus)-0.05*i)/22e-6);
                double out=mode>=3U?(i<0&&!ll?-(i+next)*0.5:0):(i>0&&!rl?(i+next)*0.5:0);
                v+=dt*(out-v/load)/460e-6; i=next;
            } else {
                i+=dt*((mode>=3U?l*v-r*source:l*source-r*v)-0.05*i)/22e-6;
                v+=dt*((mode>=3U?-l*i:r*i)-v/load)/460e-6;
            }
            if(v>peak)peak=v;
            if(k==69U)si=i;
            if(k==(mode>=3U?99U:128U))sv=v;
        }
        start_steps=power_status.control_steps; voltage_steps=power_status.voltage_steps;
    }
    printf("Multirate real backend mode%u %.0f->%.0fV: output%.3fV peak%.3fV steps%u voltage%u\n",
           mode,source,target,v,peak,start_steps,voltage_steps);
    CHECK(fabs(v-target)<0.65);
    CHECK(start_steps>25000U && voltage_steps>4500U);
    stop();
}
int main(void)
{
    cold_boot();
    plant(1U,24.0,12.0); plant(2U,12.0,24.0);
    plant(2U,12.0,18.0); /* Current board-test target, through the real worker. */
    plant(3U,12.0,6.0); plant(4U,12.0,24.0);
#ifdef EXTENDED_PLANT
    plant(1U,12.0,1.0); plant(1U,12.0,6.0); plant(1U,12.0,11.0);
    plant(1U,40.0,1.0); plant(1U,40.0,12.0); plant(1U,40.0,36.0);
    plant(3U,12.0,1.0); plant(3U,40.0,1.0); plant(3U,40.0,36.0);
#endif
    puts("Four-mode nominal switched plant with real multirate backend: PASS");
    return 0;
}
