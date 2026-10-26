/*
 * Header file for: reverse（离散）/reverse
 * Generated with : PLECS 5.0.3
 * Generated on   : 22 Oct 2026 20:02:47
 */
#ifndef PLECS_HEADER_reverse_h_
#define PLECS_HEADER_reverse_h_

#include <stdbool.h>
#include <stdint.h>

/* Model floating point type */
typedef float reverse_FloatType;

/* Original export checksum; the project copy contains MCU adaptations. */
extern const char * const reverse_checksum;

/* Model error status */
extern const char * reverse_errorStatus;


/* Model sample time */
extern const float reverse_sampleTime;


/*
 * Model states */
typedef struct
{
   float Integrator_x;             /* reverse/Integrator */
   bool Memory;                     /* reverse/SR Flip-flop/Memory */
   int8_t DiscreteIntegrator3_first; /* reverse/Discrete Integrator3 */
   float DiscreteIntegrator3_i1_x; /* reverse/Discrete Integrator3 */
   float DiscreteIntegrator3_i2_prevU; /* reverse/Discrete Integrator3 */
   float DiscreteTransferFcn;      /* reverse/Discrete Transfer Fcn */
   float DiscreteTransferFcn2;     /* reverse/Discrete Transfer Fcn2 */
   int8_t DiscreteIntegrator4_first; /* reverse/Discrete Integrator4 */
   float DiscreteIntegrator4_i1_x; /* reverse/Discrete Integrator4 */
   float DiscreteIntegrator4_i2_prevU; /* reverse/Discrete Integrator4 */
   int8_t DiscreteIntegrator1_first; /* reverse/Discrete Integrator1 */
   float DiscreteIntegrator1_i1_x; /* reverse/Discrete Integrator1 */
   float DiscreteIntegrator1_i2_prevU; /* reverse/Discrete Integrator1 */
   bool DiscreteIntegrator1_i3_prevReset; /* reverse/Discrete Integrator1 */
   int8_t DiscreteIntegrator5_first; /* reverse/Discrete Integrator5 */
   float DiscreteIntegrator5_i1_x; /* reverse/Discrete Integrator5 */
   float DiscreteIntegrator5_i2_prevU; /* reverse/Discrete Integrator5 */
} reverse_ModelStates;
extern reverse_ModelStates reverse_X;


/* External inputs */
typedef struct
{
   float V_right;                  /* reverse/V_right */
   float IL;                       /* reverse/IL */
   float V_left;                   /* reverse/V_left */
   float mode;                     /* reverse/mode */
} reverse_ExternalInputs;
extern reverse_ExternalInputs reverse_U;


/* External outputs */
typedef struct
{
   float D_cm_R;                   /* reverse/D.cm_R */
   bool EN_R_BUCK;                  /* reverse/EN_R_BUCK */
   bool EN_R_BOOST;                 /* reverse/EN_R_BOOST */
   bool PRECHARGE_RUN_R;            /* reverse/PRECHARGE_RUN_R */
   bool D_cm_R4;                    /* reverse/D.cm_R4 */
   bool BOOST_RUN_R;                /* reverse/BOOST_RUN_R */
   bool CTRL_RUN_R;                 /* reverse/CTRL_RUN_R */
} reverse_ExternalOutputs;
extern reverse_ExternalOutputs reverse_Y;


/* Block outputs */
typedef struct
{
   float DiscreteIntegrator3;      /* reverse/Discrete Integrator3 */
   float Saturation4;              /* reverse/Saturation4 */
   bool PRECHARGE_ACHED;            /* reverse/PRECHARGE_ACHED */
   float DiscreteIntegrator4;      /* reverse/Discrete Integrator4 */
   float Saturation1;              /* reverse/Saturation1 */
   float Saturation;               /* reverse/Saturation */
   float DiscreteIntegrator1;      /* reverse/Discrete Integrator1 */
   float DiscreteIntegrator5;      /* reverse/Discrete Integrator5 */
   float Saturation6;              /* reverse/Saturation6 */
   float Saturation2;              /* reverse/Saturation2 */
   float Integrator;               /* reverse/Integrator */
   float Gain3;                    /* reverse/Gain3 */
   bool Memory;                     /* reverse/SR Flip-flop/Memory */
   float PulseDelay;               /* reverse/Pulse Delay/Configurable Subsystem/Non-zero delay (fixed-step)/Pulse Delay */
   float DiscreteTransferFcn2;     /* reverse/Discrete Transfer Fcn2 */
   bool EN_R_BUCK;                  /* reverse/EN__R_BUCK */
   bool EN_R_BOOST2;                /* reverse/EN_R_BOOST2 */
   bool LogicalOperator;            /* reverse/SR Flip-flop/Logical Operator */
   bool LogicalOperator15;          /* reverse/Logical Operator15 */
   bool LogicalOperator17;          /* reverse/Logical Operator17 */
   bool LogicalOperator18;          /* reverse/Logical Operator18 */
   float Product3;                 /* reverse/Product3 */
   float Gain2;                    /* reverse/Gain2 */
   float Sum3;                     /* reverse/Sum3 */
} reverse_BlockOutputs;
extern reverse_BlockOutputs reverse_B;

typedef struct {
   float v_start, v_target, current_limit;
} reverse_Parameters;
/* Configure only while stopped, then initialize before every accepted start. */
void reverse_configure(float start_volts, float target_volts, float current_limit);
void reverse_initialize(float time);
void reverse_step(void);
/* All gates off: advance filters/reference without winding up control PIs. */
void reverse_hold_step(void);
/* Keep the Boost/precharge latch, restart the reference from the measured
 * charged output and clear PI history before the bootstrap refresh. */
void reverse_prepare_resume(float output_volts);
void reverse_terminate(void);

#endif /* PLECS_HEADER_reverse_h_ */

