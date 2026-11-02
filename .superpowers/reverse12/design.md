# MAIN reverse Buck12->6V — 2026-11-02

User requests right-input Buck, initially12V->6V, input/output protection
similar to forward and careful timing. Base b99b2b7; same main application,
all four modes retained, sole bench reference unchanged; user alone downloads.

Share current Buck protection in both directions:10..42V input, target<=.95Vin,
min(max(target*1.5,6),40) OVP and startup |IL|<1A/output<=3V. Boost policies and
running6A/7.5A/direction checks stay. Reverse UI defaults12/6; mailbox5 target6.

Generate MAIN reverse Buck/full steps from one private template. Buck omits
Boost-only precharge/filter/ramp/delay work; retain original active PI math,
HOLD/resume, signs, zero-duty and initialization contract. Inline reverse Buck
compare mapping from the same generic expressions. Place both steps in CCM.
Keep reverse ADC1 phases; ADC2 remains a late sample of PA0, not right input.

Retain earlier reverse T4=26880 (forward27904), DWT840; add absolute T5=29952
to reverse Buck, retain DWT936 and T1/T2/T6.160/64 CPU-cycle reserves are
engineering settings, not measured WCET. Other modes retain their guard values.

RED controller-dispatch and preparation-policy tests, then GREEN;300000 bitwise
steps vs full controller and separately vs original base source,48 host groups,
both mappers, reverse12/6 nominal switched models, main/bench ARM rebuild,
actual HEX/CCM/DMA/identity/export checks, self-review, docs and fingerprints,
commit/tag/push with remote SHA verification. No board operations/claims.
