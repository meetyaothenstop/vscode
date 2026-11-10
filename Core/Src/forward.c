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
#include "control_rates.h"
#include "boost_control_rates.h"
/* Shared initialization and controller instances; arithmetic is unchanged. */
#include "forward_controller_impl.inc"
