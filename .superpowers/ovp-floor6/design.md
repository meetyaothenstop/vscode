# MAIN forward Buck6V OVP floor — 2026-11-02

User asks that targets below6V whose original OVP is below6V use6V instead.
Scope follows the MAIN forward Buck12->3V fault discussion; other modes stay.
In request acceptance, compute original ratio, apply mode1 target<6V and
trip<6V floor, retain mode1 cap40V, cache before pending publication.
For valid nonzero mode1 requests this equals min(max(target*1.5,6),40).
0V remains OFF; all startup/running current, PI/ADC/PWM and W128 remain.
Identity MAIN ADC OVP6-40 / M-OV640; existing project and sole bench reference.

RED/GREEN: replay supplied4.729247V sample for3V target. Literal ADC edge
table covers1/2/3/3.999/4V floor346/347,4.01/5/5.9V retain proportional
thresholds,6V unaffected; preserve upper cap, first fault, zero and cache
replacement. Reverse3V remains3.75V. Full47 groups and main/bench ARM rebuild,
actual image/CCM/DMA/export/identity verification, self-review, records,
commit/tag/push and verify remote SHA. User alone operates downloads.
