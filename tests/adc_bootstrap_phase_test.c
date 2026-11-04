#define main legacy_test_main
#include "power_control_test.c"
#undef main
#include "power_event_diag.h"
int main(void)
{
    cold_boot();
    stopped_samples(0,12,0);CHECK(request(3,6));
    next_input_flags=0xFU;irq(0,12,0,ADC_ISR_ADRDY|ADC_ISR_JEOS|ADC_ISR_JEOC,1000,640);
    next_input_flags=0xFU;irq(0,12,0,ADC_ISR_ADRDY|ADC_ISR_JEOS|ADC_ISR_JEOC,1000,640);
    CHECK(!power_status.fault && power_status.state==POWER_BOOTSTRAP);
    CHECK(!power_event.fault.valid);
    puts("Two normally phased bootstrap samples: no ADC fault");

    stopped_samples(0,12,0);CHECK(request(3,6));
    /* Hypothetical late entry at CPU phase750 / HRTIM count24000. At this
     * phase, nominal ADC1 conversion of THIS period has already completed.
     * Previous entry was at phase20. STOP has no entry-timing rejection. */
    next_input_flags=0xFU;irq(0,12,0,ADC_ISR_ADRDY|ADC_ISR_JEOS|ADC_ISR_JEOC,1730,24000);
    CHECK(power_status.state==POWER_BOOTSTRAP && !power_status.fault);
    /* Next normal REP entry at phase20,270cycles later; next ADC1 trigger
     * at phase250 has not happened. Late previous IRQ cleared its JEOS.
     * ADC2 late conversion is complete by now. This is a constructed input,
     * not a measurement that such lateness occurred on the physical board. */
    next_input_flags=0xFU;irq(0,12,0,ADC_ISR_ADRDY,270,640);
    CHECK(power_status.fault==POWER_FAULT_ADC && power_status.fault_detail==1);
    CHECK(power_event.fault.valid && power_event.fault.state==POWER_BOOTSTRAP);
    CHECK(power_event.fault.bootstrap_remaining==30 && power_event.fault.mode==3);
    CHECK(power_event.fault.adc1_entry_flags==1 && power_event.fault.adc2_entry_flags==0xF);
    CHECK(power_event.fault.adc_reason==1);
    CHECK(power_status.fault_spacing_cycles==270 && power_status.fault_counter_ticks==640);
    CHECK(!power_status.control_steps && !power_status.voltage_steps);
    printf("Constructed late-entry case matches code=%u detail=%u state=%u remaining=%u flags=%X/%X reason=%u; PI steps=%u/%u\n",
       power_event.fault.code,power_event.fault.detail,power_event.fault.state,
       power_event.fault.bootstrap_remaining,power_event.fault.adc1_entry_flags,
       power_event.fault.adc2_entry_flags,power_event.fault.adc_reason,
       power_status.control_steps,power_status.voltage_steps);
    puts("Mechanism reproduced in real backend; physical cause remains unverified: PASS");
    return 0;
}
