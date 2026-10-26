/*
 * Header file for: 正向（未放入子系统）/forward
 * Generated with : PLECS 5.0.3
 * Generated on   : 18 Oct 2026 19:02:44
 */
#ifndef PLECS_HEADER_forward_h_
#define PLECS_HEADER_forward_h_

#include <stdbool.h>
#include <stdint.h>

/* Model floating point type */
typedef float forward_FloatType;

/* Original export checksum; MCU adaptations change the implementation. */
extern const char * const forward_checksum;

/* Model error status */
extern const char * forward_errorStatus;


/* Model sample time */
extern const float forward_sampleTime;


/*
 * Model states */
typedef struct
{
   bool Memory;                     /* forward/SR Flip-flop/Memory */
   float DiscreteTransferFcn;      /* forward/Discrete Transfer Fcn */
   float DiscreteTransferFcn1;     /* forward/Discrete Transfer Fcn1 */
   int8_t DiscreteIntegrator3_first; /* forward/Discrete Integrator3 */
   float DiscreteIntegrator3_i1_x; /* forward/Discrete Integrator3 */
   float DiscreteIntegrator3_i2_prevU; /* forward/Discrete Integrator3 */
   int8_t DiscreteIntegrator4_first; /* forward/Discrete Integrator4 */
   float DiscreteIntegrator4_i1_x; /* forward/Discrete Integrator4 */
   float DiscreteIntegrator4_i2_prevU; /* forward/Discrete Integrator4 */
   int8_t DiscreteIntegrator_first; /* forward/Discrete Integrator */
   float DiscreteIntegrator_i1_x;  /* forward/Discrete Integrator */
   float DiscreteIntegrator_i2_prevU; /* forward/Discrete Integrator */
   bool DiscreteIntegrator_i3_prevReset; /* forward/Discrete Integrator */
   int8_t DiscreteIntegrator1_first; /* forward/Discrete Integrator1 */
   float DiscreteIntegrator1_i1_x; /* forward/Discrete Integrator1 */
   float DiscreteIntegrator1_i2_prevU; /* forward/Discrete Integrator1 */
   bool DiscreteIntegrator1_i3_prevReset; /* forward/Discrete Integrator1 */
   int8_t DiscreteIntegrator2_first; /* forward/Discrete Integrator2 */
   float DiscreteIntegrator2_i1_x; /* forward/Discrete Integrator2 */
   float DiscreteIntegrator2_i2_prevU; /* forward/Discrete Integrator2 */
} forward_ModelStates;
extern forward_ModelStates forward_X;


/* External inputs */
typedef struct
{
   float IL;                       /* forward/IL */
   float V_left;                   /* forward/V_left */
   float V_right;                  /* forward/V_right */
   float mode;                     /* forward/mode */
} forward_ExternalInputs;
extern forward_ExternalInputs forward_U;


/* External outputs */
typedef struct
{
   bool PRECHARGE_DONE;             /* forward/PRECHARGE_DONE */
   float D_cmd;                    /* forward/D_cmd */
   bool EN_F_BUCK;                  /* forward/EN_F_BUCK */
   bool EN_F_BOOST;                 /* forward/EN_F_BOOST */
   bool PRECCHARGE_RUN;             /* forward/PRECCHARGE_RUN */
   bool BOOST_RUN;                  /* forward/BOOST_RUN */
   bool CTRL_RUN;                   /* forward/CTRL_RUN */
} forward_ExternalOutputs;
extern forward_ExternalOutputs forward_Y;


/* Block outputs */
typedef struct
{
   bool PRECHARGE_ACHED;            /* forward/PRECHARGE_ACHED */
   float DiscreteIntegrator3;      /* forward/Discrete Integrator3 */
   float Saturation4;              /* forward/Saturation4 */
   float Saturation1;              /* forward/Saturation1 */
   float DiscreteIntegrator4;      /* forward/Discrete Integrator4 */
   float Saturation6;              /* forward/Saturation6 */
   float Saturation3;              /* forward/Saturation3 */
   float DiscreteIntegrator;       /* forward/Discrete Integrator */
   float Saturation;               /* forward/Saturation */
   float DiscreteIntegrator1;      /* forward/Discrete Integrator1 */
   float DiscreteIntegrator2;      /* forward/Discrete Integrator2 */
   float Saturation5;              /* forward/Saturation5 */
   float Saturation2;              /* forward/Saturation2 */
   float Gain2;                    /* forward/Gain2 */
   bool Memory;                     /* forward/SR Flip-flop/Memory */
   float PulseDelay;               /* forward/Pulse Delay/Configurable Subsystem/Non-zero delay (fixed-step)/Pulse Delay */
   float DiscreteTransferFcn1;     /* forward/Discrete Transfer Fcn1 */
   bool EN_F_BUCK1;                 /* forward/EN_F_BUCK1 */
   bool EN_F_BOOST1;                /* forward/EN_F_BOOST1 */
   bool LogicalOperator;            /* forward/SR Flip-flop/Logical Operator */
   bool LogicalOperator15;          /* forward/Logical Operator15 */
   bool LogicalOperator17;          /* forward/Logical Operator17 */
   bool LogicalOperator18;          /* forward/Logical Operator18 */
   float Product2;                 /* forward/Product2 */
   float Sum3;                     /* forward/Sum3 */
} forward_BlockOutputs;
extern forward_BlockOutputs forward_B;

typedef struct {
   float v_start;
   float v_target;
   float current_limit;
} forward_Parameters;
/* Call only while stopped, before forward_initialize(). */
void forward_configure(float start_volts, float target_volts, float current_limit);
/* Entry point functions */
void forward_initialize(float time);
void forward_step(void);
void forward_terminate(void);

#endif /* PLECS_HEADER_forward_h_ */


