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
const float reverse_sampleTime = 6.66666666666666665e-06f;
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

void reverse_step(void)
{
   if (reverse_errorStatus)
   {
      return;
   }
   if (reverse_U.mode == 0.0f) {
      memset(&reverse_Y, 0, sizeof(reverse_Y));
      return;
   }
   if ((reverse_U.mode != 3.0f && reverse_U.mode != 4.0f) ||
       !(reverse_U.V_right > 0.0f) || !isfinite(reverse_U.V_right) ||
       !isfinite(reverse_U.V_left) || !isfinite(reverse_U.IL)) {
      reverse_errorStatus = "Invalid reverse controller input.";
      memset(&reverse_Y, 0, sizeof(reverse_Y));
      return;
   }

   /* Integrator : 'reverse/Integrator' */
   reverse_B.Integrator = reverse_X.Integrator_x;

   /* Gain : 'reverse/Gain3'
    * incorporates
    *  Signal Inport : 'reverse/V_right'
    */
   /* Precharge nearly to the source before boost: the reverse buck mapping
    * permits command 1.0 while both actual high duties remain <=0.85. */
   reverse_B.Gain3 = 0.99f*reverse_U.V_right;

   /* Memory : 'reverse/SR Flip-flop/Memory' */
   reverse_B.Memory = reverse_X.Memory;

   /* Discrete Integrator : 'reverse/Discrete\nIntegrator3' */
   if (reverse_X.DiscreteIntegrator3_first < 0)
   {
      reverse_B.DiscreteIntegrator3 = reverse_P.v_start;
   }
   else if (reverse_X.DiscreteIntegrator3_first)
   {
      reverse_B.DiscreteIntegrator3 = reverse_X.DiscreteIntegrator3_i1_x;
   }
   else
   {
      reverse_B.DiscreteIntegrator3 = reverse_X.DiscreteIntegrator3_i1_x +
                                      6.66666666666666665e-06f*
                                      reverse_X.DiscreteIntegrator3_i2_prevU;
   }

   /* Saturation : 'reverse/Saturation4' */
   if (reverse_B.DiscreteIntegrator3 > reverse_P.v_target)
      reverse_B.DiscreteIntegrator3 = reverse_P.v_target;
   reverse_B.Saturation4 = reverse_B.DiscreteIntegrator3;
   if (reverse_B.Saturation4 < 0.100000000000000006f)
   {
      reverse_B.Saturation4 = 0.100000000000000006f;
   }

   /* Target Code Block : 'reverse/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      float *buffer = &reverse_D_double[0];
      reverse_B.PulseDelay = buffer[1*reverse_D_size_t[0]+0];
   }

   /* Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn2' */
   reverse_B.DiscreteTransferFcn2 = 0.0203698000000000004f*(reverse_U.V_left)+
                                    0.0399097424959199976f*
                                    reverse_X.DiscreteTransferFcn2;

   /* Comparator : 'reverse/PRECHARGE_ACHED' */
   if (reverse_B.DiscreteTransferFcn2 > reverse_B.Gain3)
      reverse_B.PRECHARGE_ACHED = 1;
   else if (reverse_B.DiscreteTransferFcn2 < reverse_B.Gain3)
      reverse_B.PRECHARGE_ACHED = 0;
   /* Discrete Integrator : 'reverse/Discrete\nIntegrator4' */
   if (reverse_X.DiscreteIntegrator4_first < 0)
   {
      reverse_B.DiscreteIntegrator4 = 0.f;
   }
   else if (reverse_X.DiscreteIntegrator4_first)
   {
      reverse_B.DiscreteIntegrator4 = reverse_X.DiscreteIntegrator4_i1_x;
   }
   else
   {
      reverse_B.DiscreteIntegrator4 = reverse_X.DiscreteIntegrator4_i1_x +
                                      6.66666666666666665e-06f*
                                      reverse_X.DiscreteIntegrator4_i2_prevU;
   }

   if (reverse_B.DiscreteIntegrator4 > 2.5f)
      reverse_B.DiscreteIntegrator4 = 2.5f;
   if (reverse_B.DiscreteIntegrator4 > reverse_P.current_limit)
      reverse_B.DiscreteIntegrator4 = reverse_P.current_limit;

   /* Relational Operator : 'reverse/EN__R_BUCK'
    * incorporates
    *  Signal Inport : 'reverse/mode'
    *  Constant : 'reverse/Constant2'
    */
   reverse_B.EN_R_BUCK = reverse_U.mode == 3.f;

   /* Relational Operator : 'reverse/EN_R_BOOST2'
    * incorporates
    *  Signal Inport : 'reverse/mode'
    *  Constant : 'reverse/Constant3'
    */
   reverse_B.EN_R_BOOST2 = reverse_U.mode == 4.f;

   /* Saturation : 'reverse/Saturation1'
    * incorporates
    *  Sum : 'reverse/Sum2'
    *  Product : 'reverse/Product'
    *  Product : 'reverse/Divide'
    *  Signal Inport : 'reverse/V_right'
    *  Product : 'reverse/Product1'
    *  Sum : 'reverse/Sum6'
    *  Product : 'reverse/Divide1'
    *  Constant : 'reverse/Constant'
    */
   reverse_B.Saturation1 =
      (((float)reverse_B.EN_R_BUCK) *
       (reverse_B.Saturation4 /
        reverse_U.V_right)) +
      (((float)reverse_B.EN_R_BOOST2) *
       (1.f - (reverse_U.V_right / reverse_B.Saturation4)));
   if (reverse_B.Saturation1 > 0.949999999999999956f)
   {
      reverse_B.Saturation1 = 0.949999999999999956f;
   }
   else if (reverse_B.Saturation1 < 0.f)
   {
      reverse_B.Saturation1 = 0.f;
   }

   /* Logical Operator : 'reverse/SR Flip-flop/Logical\nOperator'
    * incorporates
    *  Logical Operator : 'reverse/SR Flip-flop/Logical\nOperator1'
    *  Logical Operator : 'reverse/SR Flip-flop/Logical\nOperator2'
    *  Logical Operator : 'reverse/Logical\nOperator1'
    */
   reverse_B.LogicalOperator = (!reverse_B.EN_R_BUCK) &&
                               ((reverse_B.EN_R_BOOST2 &&
                                 reverse_B.PRECHARGE_ACHED) ||
                                reverse_B.Memory);

   /* Logical Operator : 'reverse/Logical\nOperator15'
    * incorporates
    *  Logical Operator : 'reverse/Logical\nOperator'
    */
   reverse_B.LogicalOperator15 = (!reverse_B.LogicalOperator) &&
                                 reverse_B.EN_R_BOOST2;

   /* Logical Operator : 'reverse/Logical\nOperator17' */
   reverse_B.LogicalOperator17 = reverse_B.PulseDelay &&
                                 reverse_B.EN_R_BOOST2;

   /* Logical Operator : 'reverse/Logical\nOperator18' */
   reverse_B.LogicalOperator18 = reverse_B.EN_R_BUCK ||
                                 reverse_B.LogicalOperator17;
   if (reverse_B.EN_R_BOOST2 && !reverse_B.LogicalOperator17)
      reverse_B.Saturation1 = 0.0f;

   /* Product : 'reverse/Product3'
    * incorporates
    *  Sum : 'reverse/Sum'
    *  Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn'
    */
   reverse_B.Product3 = ((float)reverse_B.LogicalOperator18) *
                        (reverse_B.Saturation4 -
                         (0.0203698000000000004f*(reverse_U.V_left)+
                          0.0399097424959199976f*
                          reverse_X.DiscreteTransferFcn));

   /* Saturation : 'reverse/Saturation'
    * incorporates
    *  Sum : 'reverse/Sum1'
    *  Gain : 'reverse/Kpv'
    */
   reverse_B.Saturation =
      (0.800000000000000044f*reverse_B.Product3) + reverse_B.Integrator;
   if (reverse_B.Saturation > reverse_P.current_limit)
   {
      reverse_B.Saturation = reverse_P.current_limit;
   }
   else if (reverse_B.Saturation < 0.f)
   {
      reverse_B.Saturation = 0.f;
   }

   /* Gain : 'reverse/Gain2' */
   reverse_B.Gain2 = -1.f*reverse_B.Saturation;

   /* Discrete Integrator : 'reverse/Discrete\nIntegrator1' */
   if (reverse_X.DiscreteIntegrator1_first < 0 ||
       (!reverse_X.DiscreteIntegrator1_i3_prevReset &&
        reverse_B.LogicalOperator))
   {
      reverse_B.DiscreteIntegrator1 = 0.f;
   }
   else if (reverse_X.DiscreteIntegrator1_first)
   {
      reverse_B.DiscreteIntegrator1 = reverse_X.DiscreteIntegrator1_i1_x;
   }
   else
   {
      reverse_B.DiscreteIntegrator1 = reverse_X.DiscreteIntegrator1_i1_x +
                                      6.66666666666666665e-06f*
                                      reverse_X.DiscreteIntegrator1_i2_prevU;
   }

   /* Discrete Integrator : 'reverse/Discrete\nIntegrator5'
    * incorporates
    *  Logical Operator : 'reverse/Logical\nOperator2'
    */
   if (reverse_X.DiscreteIntegrator5_first < 0 ||
       (!reverse_B.LogicalOperator17 != 0))
   {
      reverse_B.DiscreteIntegrator5 = 0.f;
   }
   else if (reverse_X.DiscreteIntegrator5_first)
   {
      reverse_B.DiscreteIntegrator5 = reverse_X.DiscreteIntegrator5_i1_x;
   }
   else
   {
      reverse_B.DiscreteIntegrator5 = reverse_X.DiscreteIntegrator5_i1_x +
                                      6.66666666666666665e-06f*
                                      reverse_X.DiscreteIntegrator5_i2_prevU;
   }
   if (reverse_B.DiscreteIntegrator5 > 1.f)
   {
      reverse_B.DiscreteIntegrator5 = 1.f;
   }
   else if (reverse_B.DiscreteIntegrator5 < 0.f)
   {
      reverse_B.DiscreteIntegrator5 = 0.f;
   }

   /* Saturation : 'reverse/Saturation6' */
   reverse_B.Saturation6 = reverse_B.DiscreteIntegrator5;
   if (reverse_B.Saturation6 > 1.f)
   {
      reverse_B.Saturation6 = 1.f;
   }
   else if (reverse_B.Saturation6 < 0.f)
   {
      reverse_B.Saturation6 = 0.f;
   }

   /* Sum : 'reverse/Sum3'
    * incorporates
    *  Signal Inport : 'reverse/IL'
    *  Sum : 'reverse/Sum7'
    *  Sum : 'reverse/Sum8'
    *  Product : 'reverse/Product2'
    *  Gain : 'reverse/Gain4'
    *  Product : 'reverse/Product4'
    *  Product : 'reverse/Product5'
    *  Product : 'reverse/Product6'
    */
   reverse_B.Sum3 = reverse_U.IL -
                    ((((-1.f*
                        reverse_B.DiscreteIntegrator4) *
                       ((float)reverse_B.LogicalOperator15)) +
                      ((((float)reverse_B.LogicalOperator17) *
                        reverse_B.Saturation6) *
                       reverse_B.Gain2)) +
                     (((float)reverse_B.EN_R_BUCK) * reverse_B.Gain2));

   /* Saturation : 'reverse/Saturation2'
    * incorporates
    *  Sum : 'reverse/Sum5'
    *  Sum : 'reverse/Sum4'
    *  Gain : 'reverse/Gain'
    */
   reverse_B.Saturation2 =
      ((0.0149999999999999994f*
        reverse_B.Sum3) +
       reverse_B.DiscreteIntegrator1) + reverse_B.Saturation1;
   float duty_limit = reverse_B.LogicalOperator17 ? 0.95f : 1.0f;
   if (reverse_B.Saturation2 > duty_limit)
   {
      reverse_B.Saturation2 = duty_limit;
   }
   else if (reverse_B.Saturation2 < 0.f)
   {
      reverse_B.Saturation2 = 0.f;
   }

   /* Global output signals */
   reverse_Y.D_cm_R = reverse_B.Saturation2;
   reverse_Y.EN_R_BUCK = reverse_B.EN_R_BUCK;
   reverse_Y.EN_R_BOOST = reverse_B.EN_R_BOOST2;
   reverse_Y.PRECHARGE_RUN_R = reverse_B.LogicalOperator15;
   reverse_Y.D_cm_R4 = reverse_B.LogicalOperator;
   reverse_Y.BOOST_RUN_R = reverse_B.LogicalOperator17;
   reverse_Y.CTRL_RUN_R = reverse_B.LogicalOperator18;

   if (reverse_errorStatus)
   {
      return;
   }

   /* Update for Memory : 'reverse/SR Flip-flop/Memory' */
   reverse_X.Memory = reverse_B.LogicalOperator;

   /* Update for Discrete Integrator : 'reverse/Discrete\nIntegrator3'
    * incorporates
    *  Constant : 'reverse/R'
    */
   reverse_X.DiscreteIntegrator3_first = 0;
   reverse_X.DiscreteIntegrator3_i1_x = reverse_B.DiscreteIntegrator3;
   reverse_X.DiscreteIntegrator3_i2_prevU = reverse_B.LogicalOperator18 ? 120.f : 0.f;

   /* Update for Target Code Block : 'reverse/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      float *buffer = &reverse_D_double[0];
      buffer[1*reverse_D_size_t[0] + 0] = (reverse_B.LogicalOperator);
      reverse_D_size_t[0] = (reverse_D_size_t[0]+1) % 3;
   }

   /* Update for Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn'
    * incorporates
    *  Signal Inport : 'reverse/V_left'
    */
   {
      float newX;
      newX = (reverse_U.V_left)+0.959260400000000013f*
             reverse_X.DiscreteTransferFcn;
      reverse_X.DiscreteTransferFcn = newX;
   }

   /* Update for Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn2'
    * incorporates
    *  Signal Inport : 'reverse/V_left'
    */
   {
      float newX;
      newX = (reverse_U.V_left)+0.959260400000000013f*
             reverse_X.DiscreteTransferFcn2;
      reverse_X.DiscreteTransferFcn2 = newX;
   }

   /* Update for Discrete Integrator : 'reverse/Discrete\nIntegrator4'
    * incorporates
    *  Constant : 'reverse/R1'
    */
   reverse_X.DiscreteIntegrator4_first = 0;
   reverse_X.DiscreteIntegrator4_i1_x = reverse_B.DiscreteIntegrator4;
   reverse_X.DiscreteIntegrator4_i2_prevU = 500.f;

   /* Update for Discrete Integrator : 'reverse/Discrete\nIntegrator1'
    * incorporates
    *  Gain : 'reverse/Gain1'
    */
   reverse_X.DiscreteIntegrator1_first = 0;
   reverse_X.DiscreteIntegrator1_i1_x = reverse_B.DiscreteIntegrator1;
   reverse_X.DiscreteIntegrator1_i2_prevU =
      ((reverse_B.Saturation2 >= duty_limit && reverse_B.Sum3 > 0.0f) ||
       (reverse_B.Saturation2 <= 0.0f && reverse_B.Sum3 < 0.0f)) ?
      0.0f : 20.0f*reverse_B.Sum3;
   reverse_X.DiscreteIntegrator1_i3_prevReset = !!(reverse_B.LogicalOperator);

   /* Update for Discrete Integrator : 'reverse/Discrete\nIntegrator5'
    * incorporates
    *  Constant : 'reverse/Constant1'
    */
   reverse_X.DiscreteIntegrator5_first = 0;
   reverse_X.DiscreteIntegrator5_i1_x = reverse_B.DiscreteIntegrator5;
   reverse_X.DiscreteIntegrator5_i2_prevU = 100.f;

   /* Derivatives for Integrator : 'reverse/Integrator'
    * incorporates
    *  Gain : 'reverse/Kiv'
    */
   reverse_deriv[0] =
      ((reverse_B.Saturation >= reverse_P.current_limit && reverse_B.Product3 > 0.0f) ||
       (reverse_B.Saturation <= 0.0f && reverse_B.Product3 < 0.0f)) ?
      0.0f : 10.0f*reverse_B.Product3;

   /* Update continuous states */
   reverse_X.Integrator_x += 6.66666666666666665e-06f*reverse_deriv[0];
}

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

