# MAIN forward Buck OVP1.5 capped40V — 2026-10-31

User explicitly requested a40V maximum for the forward Buck protection line
after the54V-at36V limitation was explained. Base e4a9fc5, current main project
and branch; user alone downloads. Interpret as trip-voltage cap, not a new
output target range or physical clamp guarantee.

Compute/cache min(target*1.5,40.0) at MAIN mode1 request acceptance. Keep
reverse Buck1.25, Boost1.1, bench policy, single-sample IRQ comparison,
first-fault latch, W128 timing, PI and all ADC/PWM/current settings unchanged.
Identity: MAIN ADC OVP1.5 C40 / M-15C40 (longest parameter header21 columns).

RED/GREEN: preserve6/9/12V proportional boundaries;26V->39V remains uncapped,
27/30/36V->40V must accept raw2309 (~39.99938V) and trip at2310 (~40.01671V).
Switching back from capped targets must restore9V target's13.5V trip.
Reverse36V target must still accept44.5V and trip above45V, proving scope.
Run47 host groups, ARM main/bench and actual image/CCM/DMA/export checks;
review; update records, commit/tag/push and verify branch/tag SHA. No board actions.
