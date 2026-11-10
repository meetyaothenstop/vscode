/* Catches stale150kHz integration coefficients and outer-loop execution
 * on every inner step. Exercise the real adapted controller, no PI mocks. */
#include "forward.h"
#include "reverse.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void near(float a, float b) { assert(fabsf(a-b) < 0.00001f); }
static void init(unsigned mode, float start)
{
    forward_configure(start,6.0f,6.0f); forward_initialize(0.0f);
    reverse_configure(start,6.0f,6.0f); reverse_initialize(0.0f);
    forward_U=(forward_ExternalInputs){.mode=(float)mode,.V_left=12.0f};
    reverse_U=(reverse_ExternalInputs){.mode=(float)mode,.V_right=12.0f};
}
int main(void)
{
    init(1U,0.1f);
    for(unsigned n=0;n<3;n++) forward_step();
    float previous=forward_B.Saturation4;
    forward_step();
    near(forward_B.Saturation4-previous,0.0024f); /*120V/s x20us*/
    init(3U,0.1f);
    for(unsigned n=0;n<3;n++) reverse_step();
    previous=reverse_B.Saturation4; reverse_step();
    near(reverse_B.Saturation4-previous,0.0024f);
    init(1U,6.0f); forward_U.V_right=1.0f; forward_step();
    near(forward_B.Product2,5.7622532f); /*6238/(s+6238), Tustin100us*/
    previous=forward_B.Saturation;
    for(unsigned n=0;n<4;n++) {
        forward_U.V_right=2.0f; forward_step();
        near(forward_B.Saturation,previous);
    }
    forward_step(); assert(forward_B.Saturation!=previous);
    init(1U,1.0f); forward_step();
    for(unsigned n=0;n<4;n++)forward_step();
    near(forward_B.DiscreteIntegrator,0.0f);
    forward_step();near(forward_B.DiscreteIntegrator,0.0008f); /*8/s x1V x100us*/
    init(3U,6.0f); reverse_U.V_left=1.0f; reverse_step();
    near(reverse_B.Product2,5.7622532f);
    previous=reverse_B.Saturation;
    for(unsigned n=0;n<4;n++) {
        reverse_U.V_left=2.0f; reverse_step();
        near(reverse_B.Saturation,previous);
    }
    reverse_step(); assert(reverse_B.Saturation!=previous);
    puts("50kHz ramp, 10kHz voltage sample/hold and Tustin response: PASS");
    return 0;
}
