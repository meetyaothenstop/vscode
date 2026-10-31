# Forward Buck OVP1.5 candidate — 2026-10-31

User explicitly requested forward Buck overvoltage multiplier1.5. Base a32304c.
Use the current branch and existing main project; user downloads firmware.

Implementation: add a forward-only MAIN multiplier, selected when publishing
the cached output trip voltage. Keep the existing MAIN Buck1.25 constant for
reverse Buck; Boost stays1.1 and restricted bench keeps its previous policy.
The IRQ comparison, single-sample shutdown, first-fault latch, W128 timing,
PI/soft-start, ADC/PWM and current limits are unchanged.

Identity: MAIN ADC OVP1.5 6A / M-OV1.5 (fits21-column display). Hardware-guard
and diagnostic home identities retain their existing labels.

RED/GREEN: captured9V fault sample11.5719309V must pass under13.5V; adjacent
ADC cases519/520 (6V->9V),779/780 (9V->13.5V),1039/1040 (12V->18V),3117/3118
(36V->54V) must pass/trip on the first sample and retain first-fault evidence.
Four-mode tests must still trip reverse Buck at1.25 and Boost at1.1. Keep the
nominal switching-model1.25 peak guards unchanged. Run47 host groups, ARM main/
bench rebuilds, actual HEX/CCM/no-probes/DMA checks and independent scope review.

Material limit: exact1.5 policy gives54V for36V target, exceeding the user's
reported50V capacitors. Do not silently cap the requested ratio or alter the
existing target range; state this limitation in README/handoff. This is an
authorized threshold experiment for the current24V low-power setup, not
validation of high-voltage operation or a root-cause fix. No board actions.
