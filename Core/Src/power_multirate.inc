/* Included by power_control.c: fast IRQ owns all hardware/state changes.
 * PendSV exclusively owns PI/filter state while running. Foreground init
 * cannot run concurrently with PendSV on this single-core target. */
#include "pwm_density.h"
typedef struct {
    uint32_t generation, begin;
    unsigned mode, hold, resume;
#if POWER_RAW_FAST_IRQ
    unsigned left,right,current;
#else
    float left,right,current;
#endif
} ControlRequest;
typedef struct {
    uint32_t generation;
    unsigned hold, resume, valid, boost, changeover, run;
    union { float duty; uint32_t duty_bits; };
    union { float reference; uint32_t reference_bits; };
    PwmPlan plan;
#if POWER_RAW_FAST_IRQ
    uint32_t pulse_increment;
#endif
} ControlResult;
static ControlRequest control_request;
static ControlResult control_result;
static volatile uint32_t control_generation;
static volatile unsigned control_pending, control_ready;
static unsigned control_countdown, control_run;
static unsigned control_changeover_cycles;
static PwmPlan control_plan;
#if POWER_RAW_FAST_IRQ
static uint32_t control_pulse_increment,control_pulse_fraction;
#else
static float control_duty,control_pulse_fraction;
#endif
static unsigned control_plan_valid, control_boost;

static void control_cancel(void)
{
    ++control_generation;
    control_pending = control_ready = 0U;
    control_countdown = control_run = 0U;
    control_changeover_cycles = 0U;
    control_plan_valid = 0U;
    control_pulse_fraction = 0;
    SCB->ICSR = SCB_ICSR_PENDSVCLR_Msk;
    __DMB();
}

void Power_ControlIRQ(void)
{
    if (!control_pending || control_ready) return;
    __DMB();
#if POWER_RAW_FAST_IRQ
    uint32_t entry_mask=__get_PRIMASK(); __disable_irq();
    uint32_t worker_begin=DWT->CYCCNT,fast_begin=fast_c_cycles_total;
    __set_PRIMASK(entry_mask);
#endif
    const ControlRequest request = control_request;
#if POWER_RAW_FAST_IRQ
    float left=(float)request.left*ADC_LEFT_VOLTS_PER_COUNT;
    float right=(float)request.right*ADC_RIGHT_VOLTS_PER_COUNT;
    float current=(float)request.current*ADC_CURRENT_AMPS_PER_COUNT-ADC_CURRENT_ZERO_AMPS;
#else
    float left=request.left,right=request.right,current=request.current;
#endif
    ControlResult result = {0};
    result.generation = request.generation;
    result.hold = request.hold;
    result.resume = request.resume;
    if (request.mode >= 3U) {
        reverse_U = (reverse_ExternalInputs){.V_left=left,
            .V_right=right,.IL=current,.mode=(float)request.mode};
        if (request.resume) reverse_prepare_resume(left);
        if (request.hold && !request.resume) {
            if (request.mode == 3U) reverse_main_buck_hold_step();
            else reverse_hold_step();
        }
        else reverse_step();
        result.duty = reverse_Y.D_cm_R;
        result.reference = reverse_B.Saturation4;
        result.boost = reverse_Y.BOOST_RUN_R;
        result.changeover = reverse_Y.D_cm_R4 && !result.boost;
        result.run = reverse_Y.CTRL_RUN_R;
        result.valid = !reverse_errorStatus &&
            PWM_MakeReversePlan(result.duty, result.boost, &result.plan);
    } else {
        forward_U = (forward_ExternalInputs){.V_left=left,
            .V_right=right,.IL=current,.mode=(float)request.mode};
        if (request.mode == 2U) forward_main_boost_step();
        else forward_step();
        result.duty = forward_Y.D_cmd;
        result.reference = forward_B.Saturation4;
        result.boost = forward_Y.BOOST_RUN;
        result.changeover = forward_Y.PRECHARGE_DONE && !result.boost;
        result.run = forward_Y.CTRL_RUN;
        result.valid = !forward_errorStatus &&
            PWM_MakePlan(result.duty, result.boost, &result.plan);
    }
#if POWER_RAW_FAST_IRQ
    if(!result.boost && result.duty>0.0f && result.duty<0.04f/PWM_REFRESH_HIGH)
        result.pulse_increment=(uint32_t)(result.duty*(PWM_REFRESH_HIGH/0.04f)*(float)PWM_DENSITY_SCALE+0.5f);
#endif
    uint32_t publish_mask=__get_PRIMASK(); __disable_irq();
    if(request.generation!=control_generation || !control_pending) {
        __set_PRIMASK(publish_mask); return;
    }
#if POWER_RAW_FAST_IRQ
    power_status.v_left=left;power_status.v_right=right;power_status.il=current;
#endif
    control_result = result;
    uint32_t worker_end=DWT->CYCCNT;
    uint32_t elapsed=worker_end-request.begin;
#if POWER_RAW_FAST_IRQ
    uint32_t delay=worker_begin-request.begin,body=worker_end-worker_begin,preempted=fast_c_cycles_total-fast_begin;
    if(delay>power_status.control_start_delay_max)power_status.control_start_delay_max=delay;
    if(body>power_status.control_run_wall_cycles_max)power_status.control_run_wall_cycles_max=body;
    if(preempted>power_status.control_fast_c_cycles_max)power_status.control_fast_c_cycles_max=preempted;
#endif
    if (elapsed > power_status.control_cycles_max) power_status.control_cycles_max = elapsed;
    ++power_status.control_steps;
    extern unsigned forward_voltage_updates, reverse_voltage_updates;
    power_status.voltage_steps = request.mode >= 3U ? reverse_voltage_updates : forward_voltage_updates;
    __DMB();
    control_ready = 1U;
    __set_PRIMASK(publish_mask);
}

static int control_write(const PwmPlan *plan, uint32_t begin, uint32_t spacing)
{
    uint32_t elapsed = DWT->CYCCNT - begin;
    uint32_t phase = HRTIM1->sTimerxRegs[TIMER_B].CNTxR;
    if (elapsed > cycle_budget - 160U || phase > active_limits.write_phase_limit_ticks) {
        timing_trip(elapsed > cycle_budget - 160U ? 3U : 4U, elapsed, spacing, phase);
        return 0;
    }
    write_plan(plan);
    return 1;
}

static void control_modulate(uint32_t begin, uint32_t spacing)
{
#if POWER_RAW_FAST_IRQ
    if(!control_plan_valid||control_boost||!control_pulse_increment||power_status.state!=POWER_RUNNING)return;
    uint32_t source_compare=PWM_DensityStep(&control_pulse_fraction,control_pulse_increment)?PWM_MIN_COMPARE_TICKS:0U;
    PwmPlan plan;
    plan.left_compare=active_mode>=3U?control_plan.left_compare:source_compare;
    plan.right_compare=active_mode>=3U?source_compare:control_plan.right_compare;
#else
    if (!control_plan_valid || control_boost || control_duty <= 0.0f ||
        control_duty >= 0.04f / PWM_REFRESH_HIGH || power_status.state != POWER_RUNNING) return;
    /* The existing mapper has a267ns minimum source pulse. At50kHz,
     * grouping three null/minimum pulses causes low-voltage limit cycles.
     * First-order pulse density distributes the same requested mean duty
     * over individual PWM periods; this is modulation, not another PI. */
    control_pulse_fraction += control_duty * (PWM_REFRESH_HIGH / 0.04f);
    uint32_t source_compare = 0U;
    if (control_pulse_fraction >= 1.0f) {
        control_pulse_fraction -= 1.0f;
        source_compare = PWM_MIN_COMPARE_TICKS;
    }
    PwmPlan plan = control_plan;
    if (active_mode >= 3U) plan.right_compare = source_compare;
    else plan.left_compare = source_compare;
#endif
    (void)control_write(&plan, begin, spacing);
}

static void control_slice(uint32_t begin, uint32_t spacing)
{
    const unsigned reverse = active_mode >= 3U;
#if POWER_RAW_FAST_IRQ
    unsigned vout=raw_output();
#else
    const float vout=reverse?power_status.v_left:power_status.v_right;
#endif
    if (reverse && power_status.state == POWER_RUNNING && control_run &&
#if POWER_RAW_FAST_IRQ
        Raw_Contains(raw_policy.hold_output,vout))
#else
        vout>=active_target*1.005f)
#endif
    {
        Power_EmergencyOff();
        power_status.state = POWER_HOLD;
        control_plan_valid = 0U;
        control_pulse_fraction = 0;
    }
    unsigned due = !control_countdown || --control_countdown == 0U;
    /* Commit completed work at the earliest safe PWM boundary. Holding a
     * completed result until the next control request adds needless loop delay. */
    if (!due && !control_ready) return;
    if (control_pending) {
        if (!control_ready) {
            timing_trip(7U, DWT->CYCCNT - control_request.begin, spacing,
                        HRTIM1->sTimerxRegs[TIMER_B].CNTxR);
            return;
        }
        __DMB();
        const ControlResult result = control_result;
        control_pending = control_ready = 0U;
        const unsigned holding = power_status.state == POWER_HOLD;
        if (result.generation == control_generation && result.hold == holding) {
            if (!result.valid) { trip(POWER_FAULT_CONTROLLER); return; }
            uint32_t elapsed = DWT->CYCCNT - begin;
            uint32_t phase = HRTIM1->sTimerxRegs[TIMER_B].CNTxR;
            if (elapsed > cycle_budget - 160U || phase > active_limits.write_phase_limit_ticks) {
                timing_trip(elapsed > cycle_budget - 160U ? 3U : 4U, elapsed, spacing, phase);
                return;
            }
#if POWER_RAW_FAST_IRQ
            /* Bit-preserving telemetry copy, with no FP register access
             * (even a VLDR/VSTR would trigger lazy FP stacking in this IRQ). */
            power_status.d_cmd_bits=result.duty_bits;
            power_status.v_reference_bits=result.reference_bits;
#else
            power_status.d_cmd = result.duty;
            power_status.v_reference = result.reference;
#endif
            control_run = result.run;
            if (holding) {
                if (result.resume &&
#if POWER_RAW_FAST_IRQ
                    raw_can_resume()
#else
                    vout < active_target * 0.99f && fabsf(power_status.il) < 0.25f
#endif
                    ) {
                    resume_plan = result.plan;
                    bootstrap_resume = 1U;
                    bootstrap_cycles = 31U;
                    resume_bootstrap_pending = 1U;
                    power_status.state = POWER_BOOTSTRAP;
                    control_countdown = 0U;
                    control_plan_valid = 0U;
                    control_pulse_fraction = 0;
                    return;
                }
            } else {
                if ((active_mode == 2U || active_mode == 4U) && result.changeover) {
                    Power_EmergencyOff();
                    power_status.state = POWER_CHANGEOVER;
                    transition_ready = 0U;
                    control_changeover_cycles = 3U; /*three complete off PWM periods*/
                }
                control_plan = result.plan;
#if POWER_RAW_FAST_IRQ
                control_pulse_increment=result.pulse_increment;
#else
                control_duty = result.duty;
#endif
                control_boost = result.boost;
                control_plan_valid = 1U;
                if (
#if POWER_RAW_FAST_IRQ
                    !result.pulse_increment ||
#else
                    result.boost || result.duty <= 0.0f || result.duty >= 0.04f / PWM_REFRESH_HIGH ||
#endif
                    power_status.state != POWER_RUNNING) {
                    control_pulse_fraction = 0;
                    if (!control_write(&result.plan, begin, spacing)) return;
                }
                if (power_status.state == POWER_CHANGEOVER && result.boost)
                    transition_ready = 1U;
            }
        }
    }
    if (!due) return;
    control_request = (ControlRequest){.generation=control_generation,
        .begin=begin,.mode=active_mode,.hold=power_status.state == POWER_HOLD,
        .resume=power_status.state==POWER_HOLD &&
#if POWER_RAW_FAST_IRQ
        raw_can_resume(),
        .left=power_status.raw_vh,.right=power_status.raw_vl,.current=power_status.raw_il
#else
        vout<active_target*0.99f && fabsf(power_status.il)<0.25f,
        .left=power_status.v_left,.right=power_status.v_right,.current=power_status.il
#endif
    };
    control_countdown = active_mode == 2U ? BOOST_CONTROL_PWM_PERIODS : 3U;
    __DMB();
    control_pending = 1U;
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}
