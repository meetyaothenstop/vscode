/* Integer-only healthy fast path. The foreground policy enumerates the
 * original calibrated predicates, preserving strict/inclusive boundaries. */
static inline unsigned raw_output(void)
{ return active_mode>=3U?power_status.raw_vh:power_status.raw_vl; }
static __attribute__((noinline)) void raw_checks(unsigned starting,unsigned reverse)
{
    unsigned input=reverse?power_status.raw_vl:power_status.raw_vh;
    unsigned output=raw_output(),current=power_status.raw_il;
    if(!Raw_Contains(raw_policy.input,input))trip(POWER_FAULT_INPUT);
    if(!Raw_Contains(raw_policy.output,output))trip(POWER_FAULT_OUTPUT);
    if(!Raw_Contains(raw_policy.current,current))trip(POWER_FAULT_CURRENT);
    if(reverse) {
        reverse_polarity_cycles=!Raw_Contains(raw_policy.direction,current)?reverse_polarity_cycles+1U:0U;
        if(reverse_polarity_cycles>=16U)trip(POWER_FAULT_REVERSE);
    } else if(!Raw_Contains(raw_policy.direction,current))trip(POWER_FAULT_REVERSE);
    if((starting||(!bootstrap_resume && (power_status.state==POWER_BOOTSTRAP||power_status.state==POWER_ARMING))) &&
       (!Raw_Contains(raw_policy.start_current,current)||!Raw_Contains(raw_policy.start_output,output)))
        trip_detail(POWER_FAULT_START,!Raw_Contains(raw_policy.start_current,current)?8U:9U);
}
