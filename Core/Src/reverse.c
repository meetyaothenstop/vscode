/* MCU adaptation of the supplied PLECS reverse export. Physical IL remains
 * positive left-to-right; reverse current references are negative.
 * Single precision, bounded configurable reference/current, corrected boost
 * feedforward and conditional PI integration are project adaptations. */
/*
 * Implementation file for: reverse（离散）/reverse
 * Generated with         : PLECS 5.0.3
 * Generated on           : 22 Oct 2026 20:02:47
 */
#include "reverse.h"
#include "power_build_config.h"
#include "control_rates.h"
#if POWER_MAIN_APPLICATION
#include "reverse_main_buck.h"
#include "reverse_main_boost.h"
#endif
#ifndef PLECS_HEADER_reverse_h_
#error The wrong header file "reverse.h" was included. Please check your
#error include path to see whether this file name conflicts with the name
#error of another header file.
#endif /* PLECS_HEADER_reverse_h_ */
#if defined(__GNUC__) && (__GNUC__ > 4)
#   define _ALIGNMENT 16
#   define _RESTRICT __restrict
#   define _ALIGN __attribute__((aligned(_ALIGNMENT)))
#   if defined(__clang__)
#      if __has_builtin(__builtin_assume_aligned)
#         define _ASSUME_ALIGNED(a) __builtin_assume_aligned(a, _ALIGNMENT)
#      else
#         define _ASSUME_ALIGNED(a) a
#      endif
#   else
#      define _ASSUME_ALIGNED(a) __builtin_assume_aligned(a, _ALIGNMENT)
#   endif
#else
#   ifndef _RESTRICT
#      define _RESTRICT
#   endif
#   ifndef _ALIGN
#      define _ALIGN
#   endif
#   ifndef _ASSUME_ALIGNED
#      define _ASSUME_ALIGNED(a) a
#   endif
#endif
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#define PLECSRunTimeError(msg) reverse_errorStatus = msg
#define reverse_UNCONNECTED 0
static float reverse_D_double[3];
static size_t reverse_D_size_t[1];
static float reverse_deriv[1] _ALIGN;
static uint32_t reverse_tickLo;
static int32_t reverse_tickHi;
reverse_ExternalInputs reverse_U;
reverse_ExternalOutputs reverse_Y;
reverse_BlockOutputs reverse_B;
reverse_ModelStates reverse_X _ALIGN;
const char * reverse_errorStatus;
const float reverse_sampleTime = CONTROL_CURRENT_DT;
#if POWER_MULTIRATE_CONTROL
static unsigned reverse_voltage_phase, reverse_voltage_enabled;
unsigned reverse_voltage_updates;
#endif
const char * const reverse_checksum =
   "95d54ecefa15e840ed3fa24e4d40032a8a3afc65";
static reverse_Parameters reverse_P = {12.0f, 36.0f, 6.0f};
void reverse_configure(float start_volts, float target_volts, float current_limit)
{
   reverse_P.v_start = start_volts;
   reverse_P.v_target = target_volts;
   reverse_P.current_limit = current_limit;
}
void reverse_initialize(float time)
{
   float remainder;
   reverse_errorStatus = NULL;
#if POWER_MULTIRATE_CONTROL
   reverse_voltage_phase = reverse_voltage_enabled = reverse_voltage_updates = 0U;
#endif
   reverse_tickHi = floorf(time/(4294967296.0f*reverse_sampleTime));
   remainder = time - reverse_tickHi*4294967296.0f*reverse_sampleTime;
   reverse_tickLo = floorf(remainder/reverse_sampleTime + .5f);
   remainder -= reverse_tickLo*reverse_sampleTime;
   if (fabsf(remainder) > 1e-6f*fabsf(time))
   {
      reverse_errorStatus =
         "Start time must be an integer multiple of the base sample time.";
   }
   memset(&reverse_X, 0, sizeof(reverse_X));
   memset(&reverse_B, 0, sizeof(reverse_B));
   memset(&reverse_Y, 0, sizeof(reverse_Y));
   if (!(reverse_P.v_start >= 0.1f && reverse_P.v_start <= 40.0f) ||
       !(reverse_P.v_target >= 1.0f && reverse_P.v_target <= 36.0f) ||
       !(reverse_P.current_limit > 0.0f && reverse_P.current_limit <= 6.0f))
      reverse_errorStatus = "Invalid reverse controller configuration.";

   /* Initialization for Integrator : 'reverse/Integrator' */
   reverse_X.Integrator_x = 0.f;

   /* Initialization for Memory : 'reverse/SR Flip-flop/Memory' */
   reverse_X.Memory = false;

   /* Initialization for Discrete Integrator : 'reverse/Discrete\nIntegrator3' */
   reverse_X.DiscreteIntegrator3_first = -1;
   reverse_X.DiscreteIntegrator3_i1_x = 0;
   reverse_X.DiscreteIntegrator3_i2_prevU = 0.f;

   /* Initialization for Target Code Block : 'reverse/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      size_t i;
      float *buffer = &reverse_D_double[0];
      for (i = 0; i < 1*3; ++i)
      {
         buffer[i] = 0.000000000000000000e+00f;
      }
      reverse_D_size_t[0] = 0;
   }

   /* Initialization for Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn' */
   reverse_X.DiscreteTransferFcn = 0.f;

   /* Initialization for Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn2' */
   reverse_X.DiscreteTransferFcn2 = 0.f;

   /* Initialization for Discrete Integrator : 'reverse/Discrete\nIntegrator4' */
   reverse_X.DiscreteIntegrator4_first = -1;
   reverse_X.DiscreteIntegrator4_i1_x = 0;
   reverse_X.DiscreteIntegrator4_i2_prevU = 0.f;

   /* Initialization for Discrete Integrator : 'reverse/Discrete\nIntegrator1' */
   reverse_X.DiscreteIntegrator1_first = -1;
   reverse_X.DiscreteIntegrator1_i1_x = 0;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.f;
   reverse_X.DiscreteIntegrator1_i3_prevReset = 0.f;

   /* Initialization for Discrete Integrator : 'reverse/Discrete\nIntegrator5' */
   reverse_X.DiscreteIntegrator5_first = -1;
   reverse_X.DiscreteIntegrator5_i1_x = 0;
   reverse_X.DiscreteIntegrator5_i2_prevU = 0.f;
}

#if POWER_MAIN_APPLICATION
#define REVERSE_STEP_ATTRIBUTES __attribute__((noinline, used))
#define REVERSE_STEP_NAME reverse_main_buck_step
#define REVERSE_STEP_FAST_BUCK 1
#include "reverse_step_impl.inc"
#define REVERSE_STEP_ATTRIBUTES __attribute__((noinline, used))
#define REVERSE_STEP_NAME reverse_main_boost_step
#define REVERSE_STEP_FAST_BUCK 0
#define REVERSE_STEP_BOOST_RATES 1
#include "reverse_step_impl.inc"
#define REVERSE_STEP_ATTRIBUTES __attribute__((noinline, used))
#define REVERSE_STEP_NAME reverse_main_full_step
#define REVERSE_STEP_FAST_BUCK 0
#include "reverse_step_impl.inc"
void reverse_step(void)
{
   if (reverse_U.mode == 3.0f) {
      if (reverse_errorStatus) return;
      if (!(reverse_U.V_right > 0.0f) || !isfinite(reverse_U.V_right) ||
          !isfinite(reverse_U.V_left) || !isfinite(reverse_U.IL)) {
         reverse_errorStatus = "Invalid reverse controller input.";
         memset(&reverse_Y, 0, sizeof(reverse_Y));
         return;
      }
      reverse_main_buck_step();
   }
   else reverse_main_full_step();
}

/* Only the already-qualified MAIN mode3 IRQ bypasses the public validation. */
void reverse_main_buck_hold_step(void)
{
   float voltage_integral = reverse_X.Integrator_x;
   reverse_X.DiscreteIntegrator1_i1_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.0f;
   reverse_main_buck_step();
   reverse_X.Integrator_x = voltage_integral;
   reverse_X.DiscreteIntegrator1_i1_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.0f;
}

void reverse_main_boost_hold_step(void)
{
   float voltage_integral = reverse_X.Integrator_x;
   reverse_X.DiscreteIntegrator1_i1_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.0f;
   reverse_main_boost_step();
   reverse_X.Integrator_x = voltage_integral;
   reverse_X.DiscreteIntegrator1_i1_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.0f;
}
#else
#define REVERSE_STEP_ATTRIBUTES
#define REVERSE_STEP_NAME reverse_step
#define REVERSE_STEP_FAST_BUCK 0
#include "reverse_step_impl.inc"
#endif

void reverse_hold_step(void)
{
   float voltage_integral = reverse_X.Integrator_x;
   reverse_X.DiscreteIntegrator1_i1_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.0f;
   reverse_step();
   reverse_X.Integrator_x = voltage_integral;
   reverse_X.DiscreteIntegrator1_i1_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.0f;
}

void reverse_prepare_resume(float output_volts)
{
#if POWER_MULTIRATE_CONTROL
   reverse_voltage_phase = reverse_voltage_enabled = 0U;
#endif
   float reference = fminf(reverse_P.v_target, fmaxf(0.1f, output_volts));
   reverse_X.DiscreteIntegrator3_first = 0;
   reverse_X.DiscreteIntegrator3_i1_x = reference;
   reverse_X.DiscreteIntegrator3_i2_prevU = 0.0f;
   reverse_X.Integrator_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i1_x = 0.0f;
   reverse_X.DiscreteIntegrator1_i2_prevU = 0.0f;
}

void reverse_terminate(void)
{
}

