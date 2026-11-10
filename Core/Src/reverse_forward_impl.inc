/* MAIN reverse is an independent instance of the exact forward controller.
 * Only this adapter knows physical bus direction and the physical IL sign. */
#include "reverse.h"
#include "forward.h"
#include "reverse_main_buck.h"
#include "reverse_main_boost.h"
#include "control_rates.h"
#include "boost_control_rates.h"
#include <math.h>
#include <string.h>

reverse_ExternalInputs reverse_U;
reverse_ExternalOutputs reverse_Y;

/* Internal entry points and I/O never share storage with the forward side.
 * Public reverse_B/X use forward types so all controller state is inspectable. */
#define FORWARD_API static
#define FORWARD_IO_LINKAGE static
#define forward_U reverse_core_U
#define forward_Y reverse_core_Y
#define forward_B reverse_B
#define forward_X reverse_X
#define forward_P reverse_parameters
#define forward_D_double reverse_delay_values
#define forward_D_size_t reverse_delay_index
#define forward_tickLo reverse_tickLo
#define forward_tickHi reverse_tickHi
#define forward_voltage_phase reverse_voltage_phase
#define forward_voltage_enabled reverse_voltage_enabled
#define forward_voltage_updates reverse_voltage_updates
#define forward_errorStatus reverse_errorStatus
#define forward_sampleTime reverse_sampleTime
#define forward_checksum reverse_checksum
#define forward_configure reverse_core_configure
#define forward_initialize reverse_core_initialize
#define forward_step reverse_core_step
#define forward_terminate reverse_core_terminate
#define forward_main_buck_step reverse_core_buck_step
#define forward_main_boost_step reverse_core_boost_step
#define forward_main_full_step reverse_core_full_step
#include "forward_controller_impl.inc"
#undef forward_U
#undef forward_Y
#undef forward_B
#undef forward_X
#undef forward_P
#undef forward_D_double
#undef forward_D_size_t
#undef forward_tickLo
#undef forward_tickHi
#undef forward_voltage_phase
#undef forward_voltage_enabled
#undef forward_voltage_updates
#undef forward_errorStatus
#undef forward_sampleTime
#undef forward_checksum
#undef forward_configure
#undef forward_initialize
#undef forward_step
#undef forward_terminate
#undef forward_main_buck_step
#undef forward_main_boost_step
#undef forward_main_full_step

static void reverse_reject(const char *reason)
{
    reverse_errorStatus=reason;
    memset(&reverse_core_Y,0,sizeof(reverse_core_Y));
    memset(&reverse_Y,0,sizeof(reverse_Y));
}

/* required=0 is the public compatibility entry; 3/4 are protected entries. */
static int reverse_load(unsigned required)
{
    if(reverse_errorStatus) {memset(&reverse_Y,0,sizeof(reverse_Y));return 0;}
    if(!required && reverse_U.mode==0.0f) {
        memset(&reverse_Y,0,sizeof(reverse_Y));return 0;
    }
    if((reverse_U.mode!=3.0f && reverse_U.mode!=4.0f) ||
       (required && reverse_U.mode!=(float)required) ||
       !(reverse_U.V_right>0.0f) || !isfinite(reverse_U.V_right) ||
       !isfinite(reverse_U.V_left) || !isfinite(reverse_U.IL)) {
        reverse_reject("Invalid reverse controller input or mode.");return 0;
    }
    reverse_core_U=(forward_ExternalInputs){.V_left=reverse_U.V_right,
        .V_right=reverse_U.V_left,.IL=-reverse_U.IL,
        .mode=reverse_U.mode==3.0f?1.0f:2.0f};
    return 1;
}

static void reverse_publish(void)
{
    if(reverse_errorStatus) {memset(&reverse_Y,0,sizeof(reverse_Y));return;}
    reverse_Y=(reverse_ExternalOutputs){.D_cm_R=reverse_core_Y.D_cmd,
        .EN_R_BUCK=reverse_core_Y.EN_F_BUCK,.EN_R_BOOST=reverse_core_Y.EN_F_BOOST,
        .PRECHARGE_RUN_R=reverse_core_Y.PRECCHARGE_RUN,
        .D_cm_R4=reverse_core_Y.PRECHARGE_DONE,.BOOST_RUN_R=reverse_core_Y.BOOST_RUN,
        .CTRL_RUN_R=reverse_core_Y.CTRL_RUN};
}

void reverse_configure(float start_volts,float target_volts,float current_limit)
{ reverse_core_configure(start_volts,target_volts,current_limit); }

void reverse_initialize(float time)
{
    reverse_core_initialize(time);
    memset(&reverse_Y,0,sizeof(reverse_Y));
    if(!(reverse_parameters.v_start>=0.1f && reverse_parameters.v_start<=40.0f) ||
       !(reverse_parameters.v_target>=1.0f && reverse_parameters.v_target<=36.0f) ||
       !(reverse_parameters.current_limit>0.0f && reverse_parameters.current_limit<=6.0f))
        reverse_reject("Invalid reverse controller configuration.");
}

void reverse_step(void)
{ if(reverse_load(0)) {reverse_core_step();reverse_publish();} }
void reverse_main_buck_step(void)
{ if(reverse_load(3)) {reverse_core_buck_step();reverse_publish();} }
void reverse_main_boost_step(void)
{ if(reverse_load(4)) {reverse_core_boost_step();reverse_publish();} }

/* Compatibility only. MAIN has continuous forward-style regulation and no
 * output-threshold HOLD/resume caller. These aliases do not freeze/reset PI. */
void reverse_hold_step(void) {reverse_step();}
void reverse_main_buck_hold_step(void) {reverse_main_buck_step();}
void reverse_main_boost_hold_step(void) {reverse_main_boost_step();}
void reverse_prepare_resume(float output_volts) {(void)output_volts;}
void reverse_terminate(void) {reverse_core_terminate();}
