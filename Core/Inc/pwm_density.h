#ifndef PWM_DENSITY_H
#define PWM_DENSITY_H
#include <stdint.h>
#define PWM_DENSITY_SCALE 16777216U
/* Producer supplies0..SCALE; accumulator remains0..SCALE-1. */
static inline unsigned PWM_DensityStep(uint32_t *fraction,uint32_t increment)
{
    *fraction+=increment;
    if(*fraction>=PWM_DENSITY_SCALE){*fraction-=PWM_DENSITY_SCALE;return 1U;}
    return 0U;
}
#endif
