# OVP7 candidate — 2026-10-28

User explicitly requests 7 V after reporting fault4; this supersedes the previous
wait-for-phase-results sequence. Reported firmware identity and waveform are unverified.

RED: real backend rejects the measured 6.6875 V; OLED lacks OVP7 identity.
GREEN: full 25-group suite passes, including adjacent ADC codes around 7 V,
first-fault raw/calibrated output and immediate shutdown, and 3 V proportional limit.
Non-FAST builds retain 1.1; nominal switching-model peak assertion remains 6.6 V.
ARM rebuild and CCM checks pass; see ovp7-manifest.json for hash and addresses.

Independent read-only reviewer /root/ovp7_check: no blocking findings. Only one
runtime OVP path, correct FAST macro scope, target/PI/ADC/current/input/timing retained.
Review nit addressed: model comment describes 6.6 V as a regression guard, not FAST OVP.
That last change was comment-only; functional code unchanged after the passing suite.

Same checkout/project/branch; existing IDE dirty files excluded from Git.
Computer-use Download rejected by auto-review before execution. No CLI or indirect
fallback attempted. Software candidate is ready; board programming awaits explicit
stopped-output confirmation and authorization. No new electrical outcome claimed.
