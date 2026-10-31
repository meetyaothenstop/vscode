/* Diagnostic reuse of the existing switched-stage model; no board access. */
#define main existing_regression_main
#include "../../tests/bench12_switching_test.c"
#undef main
int main(void)
{
    const double targets[] = {11.0, 10.0, 8.0, 6.0, 1.0};
    setvbuf(stdout, NULL, _IONBF, 0);
    for (unsigned n = 0U; n < sizeof(targets) / sizeof(targets[0]); ++n)
        run(24.0, targets[n], targets[n] * 1.25, 0.1);
    return 0;
}
