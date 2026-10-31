# MAIN forward Buck startup1A/3V — 2026-10-31

User explicitly requests1A startup current and3V startup output acceptance
after36V->18V reports128 S8. Scope follows this forward Buck discussion:
MAIN mode1 only, same bidreaction project, user operates all downloads.
Keep current strictly below1A in absolute value and output<=3V, preserving
the existing comparison operators. Bootstrap30 periods and arming2 periods
are unchanged. The voltage is allowed residual voltage, not soft-start reference.

Select new forward-only constants in the foreground request, use the same
selected limits for S5/S6 and cache for IRQ S8/S9 before publication. Keep
reverse Buck0.5A/1V, Boost0.25A/0.25V and bench profiles. Retain single-sample
shutdown, first fault, all running protections, PI, ADC/PWM, W128 and OVP cap.
Identity MAIN ADC START1A 3V / M-S1A3V.

Test first:36->18 accepts+/-0.98A and2.99V through all preparation stages;
request exact+/-1A rejects,3V accepts and3.001V rejects. IRQ raw edges
3204/3205 (+1A),2957 (inside-1A),173/174 (3V); S8/S9 at pending/bootstrap/
arming; immutable fault; reverse/Boost limits and mode changes preserved.
Full host suite, ARM main/bench rebuild, actual image verification, fresh
review, docs/manifest, commit/tag/push and remote SHA verification. No board work.
