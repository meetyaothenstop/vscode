/* Private compile-time reverse step template. MAIN specializes Buck while
 * the full path retains Boost/precharge. Reinitialize before mode changes. */
#if !defined(REVERSE_STEP_NAME) || !defined(REVERSE_STEP_FAST_BUCK) || !defined(REVERSE_STEP_ATTRIBUTES)
#error Reverse template requires a name, specialization and attributes.
#endif
REVERSE_STEP_ATTRIBUTES void REVERSE_STEP_NAME(void)
{
   if (reverse_errorStatus)
   {
      return;
   }
#if REVERSE_STEP_FAST_BUCK
   /* MAIN backend contract: initialized mode3, finite fresh ADC inputs and
    * right-input protection checked before this call. Public reverse_step()
    * retains validation for callers outside that protected IRQ path. */
   if (reverse_U.mode != 3.0f) {
      reverse_errorStatus = "Restricted controller requires reverse Buck.";
      memset(&reverse_Y, 0, sizeof(reverse_Y));
      return;
   }
#else
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

#endif

   /* Integrator : 'reverse/Integrator' */
   reverse_B.Integrator = reverse_X.Integrator_x;

#if !REVERSE_STEP_FAST_BUCK
   /* Gain : 'reverse/Gain3'
    * incorporates
    *  Signal Inport : 'reverse/V_right'
    */
   /* Precharge nearly to the source before boost: the reverse buck mapping
    * permits command 1.0 while both actual high duties remain <=0.85. */
   reverse_B.Gain3 = 0.99f*reverse_U.V_right;

   /* Memory : 'reverse/SR Flip-flop/Memory' */
   reverse_B.Memory = reverse_X.Memory;

#endif
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
                                      CONTROL_CURRENT_DT*
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

#if !REVERSE_STEP_FAST_BUCK
   /* Target Code Block : 'reverse/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      float *buffer = &reverse_D_double[0];
      reverse_B.PulseDelay = buffer[1*reverse_D_size_t[0]+0];
   }

   /* Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn2' */
   reverse_B.DiscreteTransferFcn2 = CONTROL_PRECHARGE_B*(reverse_U.V_left)+
                                    CONTROL_PRECHARGE_STATE*
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
                                      CONTROL_CURRENT_DT*
                                      reverse_X.DiscreteIntegrator4_i2_prevU;
   }

   if (reverse_B.DiscreteIntegrator4 > 2.5f)
      reverse_B.DiscreteIntegrator4 = 2.5f;
   if (reverse_B.DiscreteIntegrator4 > reverse_P.current_limit)
      reverse_B.DiscreteIntegrator4 = reverse_P.current_limit;

#endif
   /* Relational Operator : 'reverse/EN__R_BUCK'
    * incorporates
    *  Signal Inport : 'reverse/mode'
    *  Constant : 'reverse/Constant2'
    */
#if REVERSE_STEP_FAST_BUCK
   reverse_B.EN_R_BUCK = true;
#else
   reverse_B.EN_R_BUCK = reverse_U.mode == 3.f;
#endif

   /* Relational Operator : 'reverse/EN_R_BOOST2'
    * incorporates
    *  Signal Inport : 'reverse/mode'
    *  Constant : 'reverse/Constant3'
    */
#if REVERSE_STEP_FAST_BUCK
   reverse_B.EN_R_BOOST2 = false;
#else
   reverse_B.EN_R_BOOST2 = reverse_U.mode == 4.f;
#endif

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
#if REVERSE_STEP_FAST_BUCK
   reverse_B.Saturation1 = reverse_B.Saturation4 / reverse_U.V_right;
#else
   reverse_B.Saturation1 =
      (((float)reverse_B.EN_R_BUCK) *
       (reverse_B.Saturation4 /
        reverse_U.V_right)) +
      (((float)reverse_B.EN_R_BOOST2) *
       (1.f - (reverse_U.V_right / reverse_B.Saturation4)));
#endif
   if (reverse_B.Saturation1 > 0.949999999999999956f)
   {
      reverse_B.Saturation1 = 0.949999999999999956f;
   }
   else if (reverse_B.Saturation1 < 0.f)
   {
      reverse_B.Saturation1 = 0.f;
   }

#if REVERSE_STEP_FAST_BUCK
   reverse_B.LogicalOperator = false;
   reverse_B.LogicalOperator15 = false;
   reverse_B.LogicalOperator17 = false;
   reverse_B.LogicalOperator18 = true;
#else
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

#endif
   unsigned voltage_step = 1U;
#if POWER_MULTIRATE_CONTROL
   voltage_step = reverse_voltage_phase == 0U;
   reverse_voltage_phase = (reverse_voltage_phase + 1U) % 5U;
   if (reverse_voltage_enabled != (unsigned)reverse_B.LogicalOperator18) {
      voltage_step = 1U;
      reverse_voltage_phase = 1U;
   }
   reverse_voltage_enabled = (unsigned)reverse_B.LogicalOperator18;
   if (voltage_step) ++reverse_voltage_updates;
#endif
   if (voltage_step) {
   /* Product : 'reverse/Product3'
    * incorporates
    *  Sum : 'reverse/Sum'
    *  Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn'
    */
   reverse_B.Product3 = ((float)reverse_B.LogicalOperator18) *
                        (reverse_B.Saturation4 -
                         (CONTROL_FILTER_B*(reverse_U.V_left)+
                          CONTROL_FILTER_STATE*
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

   }

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
                                      CONTROL_CURRENT_DT*
                                      reverse_X.DiscreteIntegrator1_i2_prevU;
   }

#if !REVERSE_STEP_FAST_BUCK
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
                                      CONTROL_CURRENT_DT*
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

#endif
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
#if REVERSE_STEP_FAST_BUCK
   reverse_B.Sum3 = reverse_U.IL - reverse_B.Gain2;
#else
   reverse_B.Sum3 = reverse_U.IL -
                    ((((-1.f*
                        reverse_B.DiscreteIntegrator4) *
                       ((float)reverse_B.LogicalOperator15)) +
                      ((((float)reverse_B.LogicalOperator17) *
                        reverse_B.Saturation6) *
                       reverse_B.Gain2)) +
                     (((float)reverse_B.EN_R_BUCK) * reverse_B.Gain2));

#endif
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
#if !REVERSE_STEP_FAST_BUCK
   reverse_X.Memory = reverse_B.LogicalOperator;
#endif

   /* Update for Discrete Integrator : 'reverse/Discrete\nIntegrator3'
    * incorporates
    *  Constant : 'reverse/R'
    */
   reverse_X.DiscreteIntegrator3_first = 0;
   reverse_X.DiscreteIntegrator3_i1_x = reverse_B.DiscreteIntegrator3;
   reverse_X.DiscreteIntegrator3_i2_prevU = reverse_B.LogicalOperator18 ? 120.f : 0.f;

#if !REVERSE_STEP_FAST_BUCK
   /* Update for Target Code Block : 'reverse/Pulse Delay/Configurable\nSubsystem/Non-zero delay (fixed-step)/Pulse Delay' */
   {
      float *buffer = &reverse_D_double[0];
      buffer[1*reverse_D_size_t[0] + 0] = (reverse_B.LogicalOperator);
      reverse_D_size_t[0] = (reverse_D_size_t[0]+1) % CONTROL_DELAY_STEPS;
   }

#endif
   if (voltage_step) {
   /* Update for Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn'
    * incorporates
    *  Signal Inport : 'reverse/V_left'
    */
   {
      float newX;
      newX = (reverse_U.V_left)+CONTROL_FILTER_A*
             reverse_X.DiscreteTransferFcn;
      reverse_X.DiscreteTransferFcn = newX;
   }

   }

#if !REVERSE_STEP_FAST_BUCK
   /* Update for Discrete Transfer Function : 'reverse/Discrete\nTransfer Fcn2'
    * incorporates
    *  Signal Inport : 'reverse/V_left'
    */
   {
      float newX;
      newX = (reverse_U.V_left)+CONTROL_PRECHARGE_A*
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

#endif
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

#if !REVERSE_STEP_FAST_BUCK
   /* Update for Discrete Integrator : 'reverse/Discrete\nIntegrator5'
    * incorporates
    *  Constant : 'reverse/Constant1'
    */
   reverse_X.DiscreteIntegrator5_first = 0;
   reverse_X.DiscreteIntegrator5_i1_x = reverse_B.DiscreteIntegrator5;
   reverse_X.DiscreteIntegrator5_i2_prevU = 100.f;

#endif
   if (voltage_step) {
   /* Derivatives for Integrator : 'reverse/Integrator'
    * incorporates
    *  Gain : 'reverse/Kiv'
    */
   reverse_deriv[0] =
      ((reverse_B.Saturation >= reverse_P.current_limit && reverse_B.Product3 > 0.0f) ||
       (reverse_B.Saturation <= 0.0f && reverse_B.Product3 < 0.0f)) ?
      0.0f : 10.0f*reverse_B.Product3;

   /* Update continuous states */
   reverse_X.Integrator_x += CONTROL_VOLTAGE_DT*reverse_deriv[0];   }

}

#undef REVERSE_STEP_NAME
#undef REVERSE_STEP_FAST_BUCK
#undef REVERSE_STEP_ATTRIBUTES
