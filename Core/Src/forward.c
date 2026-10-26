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

void forward_step(void)
{
   if (forward_errorStatus)
   {
      return;
   }

#if POWER_BENCH_FAST_BUCK
   /* This image never transitions to Boost. Reject rather than accidentally
    * run a Buck command if called outside the restricted backend contract. */
   if (forward_U.mode != 1.f)
   {
      forward_errorStatus = "Restricted controller requires forward Buck.";
      memset(&forward_Y, 0, sizeof(forward_Y));
      return;
   }
#else
   /* Gain : 'forward/Gain2'
    * incorporates
    *  Signal Inport : 'forward/V_left'
    */
   forward_B.Gain2 = 0.90f*forward_U.V_left; /* bootstrap-compatible precharge */

   /* Memory : 'forward/SR Flip-flop/Memory' */
   forward_B.Memory = forward_X.Memory;

   /* Target Code Block : 'forward/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      float *buffer = &forward_D_double[0];
      forward_B.PulseDelay = buffer[1*forward_D_size_t[0]+0];
   }

   /* Discrete Transfer Function : 'forward/Discrete\nTransfer Fcn1' */
   forward_B.DiscreteTransferFcn1 = 0.0203698000000000004f*(forward_U.V_right)+
                                    0.0399097424959199976f*
                                    forward_X.DiscreteTransferFcn1;

   /* Comparator : 'forward/PRECHARGE_ACHED' */
   if (forward_B.DiscreteTransferFcn1 > forward_B.Gain2)
      forward_B.PRECHARGE_ACHED = 1;
   else if (forward_B.DiscreteTransferFcn1 < forward_B.Gain2)
      forward_B.PRECHARGE_ACHED = 0;
#endif
   /* Discrete Integrator : 'forward/Discrete\nIntegrator3' */
   if (forward_X.DiscreteIntegrator3_first < 0)
   {
      forward_B.DiscreteIntegrator3 = forward_P.v_start;
   }
   else if (forward_X.DiscreteIntegrator3_first)
   {
      forward_B.DiscreteIntegrator3 = forward_X.DiscreteIntegrator3_i1_x;
   }
   else
   {
      forward_B.DiscreteIntegrator3 = forward_X.DiscreteIntegrator3_i1_x +
                                      6.66666666666666665e-06f*
                                      forward_X.DiscreteIntegrator3_i2_prevU;
   }

   /* Saturation : 'forward/Saturation4' */
   if (forward_B.DiscreteIntegrator3 > forward_P.v_target)
      forward_B.DiscreteIntegrator3 = forward_P.v_target;
   forward_B.Saturation4 = forward_B.DiscreteIntegrator3;
   if (forward_B.Saturation4 < 0.100000000000000006f)
   {
      forward_B.Saturation4 = 0.100000000000000006f;
   }

#if !POWER_BENCH_FAST_BUCK
   /* Saturation : 'forward/Saturation1'
    * incorporates
    *  Sum : 'forward/Sum6'
    *  Product : 'forward/Divide'
    *  Signal Inport : 'forward/V_left'
    *  Constant : 'forward/Constant1'
    */
   forward_B.Saturation1 = -(forward_U.V_left / forward_B.Saturation4) + 1.f;
   if (forward_B.Saturation1 > 0.800000000000000044f)
   {
      forward_B.Saturation1 = 0.800000000000000044f;
   }
   else if (forward_B.Saturation1 < 0.f)
   {
      forward_B.Saturation1 = 0.f;
   }

   /* Discrete Integrator : 'forward/Discrete\nIntegrator4' */
   if (forward_X.DiscreteIntegrator4_first < 0)
   {
      forward_B.DiscreteIntegrator4 = 0.f;
   }
   else if (forward_X.DiscreteIntegrator4_first)
   {
      forward_B.DiscreteIntegrator4 = forward_X.DiscreteIntegrator4_i1_x;
   }
   else
   {
      forward_B.DiscreteIntegrator4 = forward_X.DiscreteIntegrator4_i1_x +
                                      6.66666666666666665e-06f*
                                      forward_X.DiscreteIntegrator4_i2_prevU;
   }

   /* Saturation : 'forward/Saturation6' */
   forward_B.Saturation6 = forward_B.DiscreteIntegrator4;
   if (forward_B.Saturation6 > 2.5f)
   {
      forward_B.Saturation6 = 2.5f;
   }
   else if (forward_B.Saturation6 < 0.f)
   {
      forward_B.Saturation6 = 0.f;
   }

   /* Relational Operator : 'forward/EN_F_BUCK1'
    * incorporates
    *  Signal Inport : 'forward/mode'
    *  Constant : 'forward/Constant2'
    */
   forward_B.EN_F_BUCK1 = forward_U.mode == 1.f;

   /* Relational Operator : 'forward/EN_F_BOOST1'
    * incorporates
    *  Signal Inport : 'forward/mode'
    *  Constant : 'forward/Constant3'
    */
   forward_B.EN_F_BOOST1 = forward_U.mode == 2.f;
#else
   forward_B.EN_F_BUCK1 = true;
   forward_B.EN_F_BOOST1 = false;
#endif

   /* Saturation : 'forward/Saturation3'
    * incorporates
    *  Sum : 'forward/Sum2'
    *  Product : 'forward/Product'
    *  Product : 'forward/Divide1'
    *  Signal Inport : 'forward/V_left'
    *  Product : 'forward/Product1'
    *  Product : 'forward/Product7'
    *  Constant : 'forward/k_ff'
    */
#if POWER_BENCH_FAST_BUCK
   forward_B.Saturation3 = forward_B.Saturation4 / forward_U.V_left;
#else
   forward_B.Saturation3 =
      (((float)forward_B.EN_F_BUCK1) *
       (forward_B.Saturation4 /
        forward_U.V_left)) +
      (((float)forward_B.EN_F_BOOST1) *
       (forward_B.Saturation1 * 0.200000000000000011f));
#endif
   if (forward_B.Saturation3 > 0.949999999999999956f)
   {
      forward_B.Saturation3 = 0.949999999999999956f;
   }
   else if (forward_B.Saturation3 < 0.f)
   {
      forward_B.Saturation3 = 0.f;
   }

   /* Logical Operator : 'forward/SR Flip-flop/Logical\nOperator'
    * incorporates
    *  Logical Operator : 'forward/SR Flip-flop/Logical\nOperator1'
    *  Logical Operator : 'forward/SR Flip-flop/Logical\nOperator2'
    *  Logical Operator : 'forward/Logical\nOperator1'
    */
#if POWER_BENCH_FAST_BUCK
   forward_B.LogicalOperator = false;
   forward_B.LogicalOperator15 = false;
   forward_B.LogicalOperator17 = false;
   forward_B.LogicalOperator18 = true;
#else
   forward_B.LogicalOperator = (!forward_B.EN_F_BUCK1) &&
                               ((forward_B.EN_F_BOOST1 &&
                                 forward_B.PRECHARGE_ACHED) ||
                                forward_B.Memory);

   /* Logical Operator : 'forward/Logical\nOperator15'
    * incorporates
    *  Logical Operator : 'forward/Logical\nOperator'
    */
   forward_B.LogicalOperator15 = (!forward_B.LogicalOperator) &&
                                 forward_B.EN_F_BOOST1;

   /* Logical Operator : 'forward/Logical\nOperator17' */
   forward_B.LogicalOperator17 = forward_B.PulseDelay &&
                                 forward_B.EN_F_BOOST1;

   /* Logical Operator : 'forward/Logical\nOperator18' */
   forward_B.LogicalOperator18 = forward_B.EN_F_BUCK1 ||
                                 forward_B.LogicalOperator17;
#endif

   /* Product : 'forward/Product2'
    * incorporates
    *  Sum : 'forward/Sum'
    *  Discrete Transfer Function : 'forward/Discrete\nTransfer Fcn'
    */
   forward_B.Product2 =
      (forward_B.Saturation4 -
       (0.0203698000000000004f*(forward_U.V_right)+0.0399097424959199976f*
        forward_X.DiscreteTransferFcn)) *
   ((float)forward_B.LogicalOperator18);

   /* Discrete Integrator : 'forward/Discrete\nIntegrator' */
   if (forward_X.DiscreteIntegrator_first < 0 ||
       (!forward_X.DiscreteIntegrator_i3_prevReset &&
        forward_B.LogicalOperator))
   {
      forward_B.DiscreteIntegrator = 0.f;
   }
   else if (forward_X.DiscreteIntegrator_first)
   {
      forward_B.DiscreteIntegrator = forward_X.DiscreteIntegrator_i1_x;
   }
   else
   {
      forward_B.DiscreteIntegrator = forward_X.DiscreteIntegrator_i1_x +
                                     6.66666666666666665e-06f*
                                     forward_X.DiscreteIntegrator_i2_prevU;
   }

   /* Saturation : 'forward/Saturation'
    * incorporates
    *  Sum : 'forward/Sum1'
    *  Gain : 'forward/Kpv'
    */
   forward_B.Saturation =
      (0.299999999999999989f*
       forward_B.Product2) + forward_B.DiscreteIntegrator;
   if (forward_B.Saturation > forward_P.current_limit)
   {
      forward_B.Saturation = forward_P.current_limit;
   }
   else if (forward_B.Saturation < 0.f)
   {
      forward_B.Saturation = 0.f;
   }

   /* Discrete Integrator : 'forward/Discrete\nIntegrator1' */
   if (forward_X.DiscreteIntegrator1_first < 0 ||
       (!forward_X.DiscreteIntegrator1_i3_prevReset &&
        forward_B.LogicalOperator))
   {
      forward_B.DiscreteIntegrator1 = 0.f;
   }
   else if (forward_X.DiscreteIntegrator1_first)
   {
      forward_B.DiscreteIntegrator1 = forward_X.DiscreteIntegrator1_i1_x;
   }
   else
   {
      forward_B.DiscreteIntegrator1 = forward_X.DiscreteIntegrator1_i1_x +
                                      6.66666666666666665e-06f*
                                      forward_X.DiscreteIntegrator1_i2_prevU;
   }

#if !POWER_BENCH_FAST_BUCK
   /* Discrete Integrator : 'forward/Discrete\nIntegrator2'
    * incorporates
    *  Logical Operator : 'forward/Logical\nOperator2'
    */
   if (forward_X.DiscreteIntegrator2_first < 0 ||
       (!forward_B.LogicalOperator17 != 0))
   {
      forward_B.DiscreteIntegrator2 = 0.f;
   }
   else if (forward_X.DiscreteIntegrator2_first)
   {
      forward_B.DiscreteIntegrator2 = forward_X.DiscreteIntegrator2_i1_x;
   }
   else
   {
      forward_B.DiscreteIntegrator2 = forward_X.DiscreteIntegrator2_i1_x +
                                      6.66666666666666665e-06f*
                                      forward_X.DiscreteIntegrator2_i2_prevU;
   }
   if (forward_B.DiscreteIntegrator2 > 1.f)
   {
      forward_B.DiscreteIntegrator2 = 1.f;
   }
   else if (forward_B.DiscreteIntegrator2 < 0.f)
   {
      forward_B.DiscreteIntegrator2 = 0.f;
   }

   /* Saturation : 'forward/Saturation5' */
   forward_B.Saturation5 = forward_B.DiscreteIntegrator2;
   if (forward_B.Saturation5 > 1.f)
   {
      forward_B.Saturation5 = 1.f;
   }
   else if (forward_B.Saturation5 < 0.f)
   {
      forward_B.Saturation5 = 0.f;
   }
#endif

   /* Sum : 'forward/Sum3'
    * incorporates
    *  Sum : 'forward/Sum7'
    *  Product : 'forward/Product4'
    *  Sum : 'forward/Sum8'
    *  Product : 'forward/Product6'
    *  Product : 'forward/Product5'
    *  Product : 'forward/Product3'
    *  Signal Inport : 'forward/IL'
    */
#if POWER_BENCH_FAST_BUCK
   forward_B.Sum3 = forward_B.Saturation - forward_U.IL;
#else
   forward_B.Sum3 =
      ((forward_B.Saturation *
        ((float)forward_B.EN_F_BUCK1)) +
       ((forward_B.Saturation *
         (((float)forward_B.LogicalOperator17) *
          forward_B.Saturation5)) +
        (((float)forward_B.LogicalOperator15) *
      forward_B.Saturation6))) - forward_U.IL;
#endif

   /* Saturation : 'forward/Saturation2'
    * incorporates
    *  Sum : 'forward/Sum5'
    *  Sum : 'forward/Sum4'
    *  Gain : 'forward/Gain'
    */
   forward_B.Saturation2 =
      ((0.0149999999999999994f*
        forward_B.Sum3) +
       forward_B.DiscreteIntegrator1) + forward_B.Saturation3;
   if (forward_B.Saturation2 > 0.949999999999999956f)
   {
      forward_B.Saturation2 = 0.949999999999999956f;
   }
   else if (forward_B.Saturation2 < 0.f)
   {
      forward_B.Saturation2 = 0.f;
   }

   /* Global output signals */
   forward_Y.PRECHARGE_DONE = forward_B.LogicalOperator;
   forward_Y.D_cmd = forward_B.Saturation2;
   forward_Y.EN_F_BUCK = forward_B.EN_F_BUCK1;
   forward_Y.EN_F_BOOST = forward_B.EN_F_BOOST1;
   forward_Y.PRECCHARGE_RUN = forward_B.LogicalOperator15;
   forward_Y.BOOST_RUN = forward_B.LogicalOperator17;
   forward_Y.CTRL_RUN = forward_B.LogicalOperator18;

   if (forward_errorStatus)
   {
      return;
   }

#if !POWER_BENCH_FAST_BUCK
   /* Update for Memory : 'forward/SR Flip-flop/Memory' */
   forward_X.Memory = forward_B.LogicalOperator;

   /* Update for Target Code Block : 'forward/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      float *buffer = &forward_D_double[0];
      buffer[1*forward_D_size_t[0] + 0] = forward_B.LogicalOperator;
      forward_D_size_t[0] = (forward_D_size_t[0]+1) % 3;
   }
#endif

   /* Update for Discrete Transfer Function : 'forward/Discrete\nTransfer Fcn'
    * incorporates
    *  Signal Inport : 'forward/V_right'
    */
   {
      float newX;
      newX = (forward_U.V_right)+0.959260400000000013f*
             forward_X.DiscreteTransferFcn;
      forward_X.DiscreteTransferFcn = newX;
   }

#if !POWER_BENCH_FAST_BUCK
   /* Update for Discrete Transfer Function : 'forward/Discrete\nTransfer Fcn1'
    * incorporates
    *  Signal Inport : 'forward/V_right'
    */
   {
      float newX;
      newX = (forward_U.V_right)+0.959260400000000013f*
             forward_X.DiscreteTransferFcn1;
      forward_X.DiscreteTransferFcn1 = newX;
   }
#endif

   /* Update for Discrete Integrator : 'forward/Discrete\nIntegrator3'
    * incorporates
    *  Constant : 'forward/R'
    */
   forward_X.DiscreteIntegrator3_first = 0;
   forward_X.DiscreteIntegrator3_i1_x = forward_B.DiscreteIntegrator3;
   forward_X.DiscreteIntegrator3_i2_prevU = 120.f;

#if !POWER_BENCH_FAST_BUCK
   /* Update for Discrete Integrator : 'forward/Discrete\nIntegrator4'
    * incorporates
    *  Constant : 'forward/R1'
    */
   forward_X.DiscreteIntegrator4_first = 0;
   forward_X.DiscreteIntegrator4_i1_x = forward_B.DiscreteIntegrator4;
   forward_X.DiscreteIntegrator4_i2_prevU = 500.f;
#endif

   /* Update for Discrete Integrator : 'forward/Discrete\nIntegrator'
    * incorporates
    *  Gain : 'forward/Kiv'
    */
   forward_X.DiscreteIntegrator_first = 0;
   forward_X.DiscreteIntegrator_i1_x = forward_B.DiscreteIntegrator;
   forward_X.DiscreteIntegrator_i2_prevU =
      ((forward_B.Saturation >= forward_P.current_limit && forward_B.Product2 > 0.0f) ||
       (forward_B.Saturation <= 0.0f && forward_B.Product2 < 0.0f)) ?
      0.0f : 8.0f*forward_B.Product2;
   forward_X.DiscreteIntegrator_i3_prevReset = !!(forward_B.LogicalOperator);

   /* Update for Discrete Integrator : 'forward/Discrete\nIntegrator1'
    * incorporates
    *  Gain : 'forward/Gain1'
    */
   forward_X.DiscreteIntegrator1_first = 0;
   forward_X.DiscreteIntegrator1_i1_x = forward_B.DiscreteIntegrator1;
   forward_X.DiscreteIntegrator1_i2_prevU =
      ((forward_B.Saturation2 >= 0.95f && forward_B.Sum3 > 0.0f) ||
       (forward_B.Saturation2 <= 0.0f && forward_B.Sum3 < 0.0f)) ?
      0.0f : 20.0f*forward_B.Sum3;
   forward_X.DiscreteIntegrator1_i3_prevReset = !!(forward_B.LogicalOperator);

#if !POWER_BENCH_FAST_BUCK
   /* Update for Discrete Integrator : 'forward/Discrete\nIntegrator2'
    * incorporates
    *  Constant : 'forward/Constant'
    */
   forward_X.DiscreteIntegrator2_first = 0;
   forward_X.DiscreteIntegrator2_i1_x = forward_B.DiscreteIntegrator2;
   forward_X.DiscreteIntegrator2_i2_prevU = 100.f;
#endif
}

void forward_terminate(void)
{
}


