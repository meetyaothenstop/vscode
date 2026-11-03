/* Included by power_control.c: fast IRQ owns all hardware/state changes.
 * PendSV exclusively owns PI/filter state while running. Foreground init
 * cannot run concurrently with PendSV on this single-core target. */
typedef struct {
    uint32_t generation, begin;
    unsigned mode, hold, resume;
    float left, right, current;
} ControlRequest;
typedef struct {
    uint32_t generation;
    unsigned hold, resume, valid, boost, changeover, run;
    float duty, reference;
    PwmPlan plan;
} ControlResult;
static ControlRequest control_request;
static ControlResult control_result;
static volatile uint32_t control_generation;
static volatile unsigned control_pending, control_ready;
static unsigned control_countdown, control_run;
static unsigned control_changeover_cycles;
static PwmPlan control_plan;
static float control_duty, control_pulse_fraction;
static unsigned control_plan_valid, control_boost;

static void control_cancel(void)
{
    ++control_generation;
    control_pending = control_ready = 0U;
    control_countdown = control_run = 0U;
    control_changeover_cycles = 0U;
    control_plan_valid = 0U;
    control_pulse_fraction = 0.0f;
    SCB->ICSR = SCB_ICSR_PENDSVCLR_Msk;
    __DMB();
}

void Power_ControlIRQ(void)
{
    if (!control_pending || control_ready) return;
    __DMB();
    const ControlRequest request = control_request;
    ControlResult result = {0};
    result.generation = request.generation;
    result.hold = request.hold;
    result.resume = request.resume;
    if (request.mode >= 3U) {
        reverse_U = (reverse_ExternalInputs){.V_left=request.left,
            .V_right=request.right,.IL=request.current,.mode=(float)request.mode};
        if (request.resume) reverse_prepare_resume(request.left);
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
        forward_U = (forward_ExternalInputs){.V_left=request.left,
            .V_right=request.right,.IL=request.current,.mode=(float)request.mode};
        forward_step();
        result.duty = forward_Y.D_cmd;
        result.reference = forward_B.Saturation4;
        result.boost = forward_Y.BOOST_RUN;
        result.changeover = forward_Y.PRECHARGE_DONE && !result.boost;
        result.run = forward_Y.CTRL_RUN;
        result.valid = !forward_errorStatus &&
            PWM_MakePlan(result.duty, result.boost, &result.plan);
    }
    /* A fast fault/stop can preempt any calculation above. Its generation
     * invalidates this result even if it preempts between this check and ready. */
    if (request.generation != control_generation || !control_pending) return;
    control_result = result;
    uint32_t elapsed = DWT->CYCCNT - request.begin;
    if (elapsed > power_status.control_cycles_max) power_status.control_cycles_max = elapsed;
    ++power_status.control_steps;
    extern unsigned forward_voltage_updates, reverse_voltage_updates;
    power_status.voltage_steps = request.mode >= 3U ? reverse_voltage_updates : forward_voltage_updates;
    __DMB();
    control_ready = 1U;
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
    (void)control_write(&plan, begin, spacing);
}

static void control_slice(uint32_t begin, uint32_t spacing)
{
    const unsigned reverse = active_mode >= 3U;
    const float vout = reverse ? power_status.v_left : power_status.v_right;
    if (reverse && power_status.state == POWER_RUNNING && control_run &&
        vout >= active_target * 1.005f) {
        Power_EmergencyOff();
        power_status.state = POWER_HOLD;
        control_plan_valid = 0U;
        control_pulse_fraction = 0.0f;
    }
    unsigned due = !control_countdown || --control_countdown == 0U;
    /* Commit completed work at the earliest safe PWM boundary. Holding a
     * completed result until the next50kHz request added needless loop delay. */
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
            power_status.d_cmd = result.duty;
            power_status.v_reference = result.reference;
            control_run = result.run;
            if (holding) {
                if (result.resume && vout < active_target * 0.99f && fabsf(power_status.il) < 0.25f) {
                    resume_plan = result.plan;
                    bootstrap_resume = 1U;
                    bootstrap_cycles = 31U;
                    resume_bootstrap_pending = 1U;
                    power_status.state = POWER_BOOTSTRAP;
                    control_countdown = 0U;
                    control_plan_valid = 0U;
                    control_pulse_fraction = 0.0f;
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
                control_duty = result.duty;
                control_boost = result.boost;
                control_plan_valid = 1U;
                if (result.boost || result.duty <= 0.0f || result.duty >= 0.04f / PWM_REFRESH_HIGH ||
                    power_status.state != POWER_RUNNING) {
                    control_pulse_fraction = 0.0f;
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
        .resume=power_status.state == POWER_HOLD && vout < active_target * 0.99f && fabsf(power_status.il) < 0.25f,
        .left=power_status.v_left,.right=power_status.v_right,.current=power_status.il};
    control_countdown = 3U;
    __DMB();
    control_pending = 1U;
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}
