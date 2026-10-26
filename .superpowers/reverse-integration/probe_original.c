#include "reverse.h"
#include <stdio.h>
int main(void)
{
    reverse_initialize(0.0);
    reverse_U.mode = 4.0;
    reverse_U.V_right = 12.0;
    reverse_U.V_left = 0.0;
    reverse_U.IL = 0.0;
    reverse_step();
    printf("original first reference %.3f V, boost feedforward %.3f\n", reverse_B.Saturation4, reverse_B.Saturation1);
    for (unsigned n = 0; n < 150000; ++n) reverse_step();
    printf("original after 1 s: reference %.3f V, precharge magnitude %.3f A, duty %.3f\n", reverse_B.Saturation4, reverse_B.DiscreteIntegrator4, reverse_Y.D_cm_R);
    return 0;
}
