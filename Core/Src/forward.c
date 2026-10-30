/* MCU adaptation: single precision for Cortex-M4F; bounded reference,
 * bootstrap precharge threshold and conditional PI anti-windup.
 * Derived from the PLECS forward controller below. */
/*
 * Implementation file for: 正向（未放入子系统）/forward
 * Generated with         : PLECS 5.0.3
 * Generated on           : 18 Oct 2026 19:02:44
 */
#include "forward.h"
#include "power_build_config.h"
#ifndef PLECS_HEADER_forward_h_
#error The wrong header file "forward.h" was included. Please check your
#error include path to see whether this file name conflicts with the name
#error of another header file.
#endif /* PLECS_HEADER_forward_h_ */
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
#define PLECSRunTimeError(msg) forward_errorStatus = msg
#define forward_UNCONNECTED 0
static float forward_D_double[3];
static size_t forward_D_size_t[1];
static uint32_t forward_tickLo;
static int32_t forward_tickHi;
forward_ExternalInputs forward_U;
forward_ExternalOutputs forward_Y;
forward_BlockOutputs forward_B;
forward_ModelStates forward_X _ALIGN;
const char * forward_errorStatus;
static forward_Parameters forward_P = {12.0f, 36.0f, 6.0f};
const float forward_sampleTime = 6.66666666666666665e-06f;
const char * const forward_checksum =
   "2eafec47fb68d388383447afbafc9e3b3aea0681";
void forward_configure(float start_volts, float target_volts, float current_limit)
{
   forward_P.v_start = start_volts;
   forward_P.v_target = target_volts;
   forward_P.current_limit = current_limit;
}
void forward_initialize(float time)
{
   float remainder;
   forward_errorStatus = NULL;
   forward_tickHi = floorf(time/(4294967296.0f*forward_sampleTime));
   remainder = time - forward_tickHi*4294967296.0f*forward_sampleTime;
   forward_tickLo = floorf(remainder/forward_sampleTime + .5f);
   remainder -= forward_tickLo*forward_sampleTime;
   if (fabsf(remainder) > 1e-6f*fabsf(time))
   {
      forward_errorStatus =
         "Start time must be an integer multiple of the base sample time.";
   }
   memset(&forward_X, 0, sizeof(forward_X));
   memset(&forward_B, 0, sizeof(forward_B));
   memset(&forward_Y, 0, sizeof(forward_Y));

   /* Initialization for Memory : 'forward/SR Flip-flop/Memory' */
   forward_X.Memory = false;

   /* Initialization for Target Code Block : 'forward/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      size_t i;
      float *buffer = &forward_D_double[0];
      for (i = 0; i < 1*3; ++i)
      {
         buffer[i] = 0.000000000000000000e+00f;
      }
      forward_D_size_t[0] = 0;
   }

   /* Initialization for Discrete Transfer Function : 'forward/Discrete\nTransfer Fcn' */
   forward_X.DiscreteTransferFcn = 0.f;

   /* Initialization for Discrete Transfer Function : 'forward/Discrete\nTransfer Fcn1' */
   forward_X.DiscreteTransferFcn1 = 0.f;

   /* Initialization for Discrete Integrator : 'forward/Discrete\nIntegrator3' */
   forward_X.DiscreteIntegrator3_first = -1;
   forward_X.DiscreteIntegrator3_i1_x = 0;
   forward_X.DiscreteIntegrator3_i2_prevU = 0.f;

   /* Initialization for Discrete Integrator : 'forward/Discrete\nIntegrator4' */
   forward_X.DiscreteIntegrator4_first = -1;
   forward_X.DiscreteIntegrator4_i1_x = 0;
   forward_X.DiscreteIntegrator4_i2_prevU = 0.f;

   /* Initialization for Discrete Integrator : 'forward/Discrete\nIntegrator' */
   forward_X.DiscreteIntegrator_first = -1;
   forward_X.DiscreteIntegrator_i1_x = 0;
   forward_X.DiscreteIntegrator_i2_prevU = 0.f;
   forward_X.DiscreteIntegrator_i3_prevReset = 0.f;

   /* Initialization for Discrete Integrator : 'forward/Discrete\nIntegrator1' */
   forward_X.DiscreteIntegrator1_first = -1;
   forward_X.DiscreteIntegrator1_i1_x = 0;
   forward_X.DiscreteIntegrator1_i2_prevU = 0.f;
   forward_X.DiscreteIntegrator1_i3_prevReset = 0.f;

   /* Initialization for Discrete Integrator : 'forward/Discrete\nIntegrator2' */
   forward_X.DiscreteIntegrator2_first = -1;
   forward_X.DiscreteIntegrator2_i1_x = 0;
   forward_X.DiscreteIntegrator2_i2_prevU = 0.f;
}

#if POWER_MAIN_APPLICATION
/* Keep both entry points visible for linked-image CCM verification. */
#define FORWARD_STEP_ATTRIBUTES __attribute__((noinline, used))
#define FORWARD_STEP_NAME forward_main_buck_step
#define FORWARD_STEP_FAST_BUCK 1
#include "forward_step_impl.inc"

#define FORWARD_STEP_ATTRIBUTES __attribute__((noinline, used))
#define FORWARD_STEP_NAME forward_main_full_step
#define FORWARD_STEP_FAST_BUCK 0
#include "forward_step_impl.inc"

void forward_step(void)
{
   if (forward_U.mode == 1.0f)
      forward_main_buck_step();
   else
      forward_main_full_step();
}
#else
/* Historical builds retain their full or restricted public step unchanged. */
#define FORWARD_STEP_ATTRIBUTES
#define FORWARD_STEP_NAME forward_step
#define FORWARD_STEP_FAST_BUCK POWER_BENCH_FAST_BUCK
#include "forward_step_impl.inc"
#endif

void forward_terminate(void)
{
}


