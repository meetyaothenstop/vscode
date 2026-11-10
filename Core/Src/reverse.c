#include "power_build_config.h"
#if POWER_MAIN_APPLICATION
#include "reverse_forward_impl.inc"
#else
/* Historical non-MAIN reference; active MAIN uses the shared forward core. */
#include "reverse_legacy_impl.inc"
#endif
