# Main T3 measurement — 2026-10-30

User image codex-clipboard-1dfb267e-ab37-47e3-8405-70d6093ad450.png:
fault0x40/detail3, elapsed0x35A=858, spacing0x3E8=1000,
counter0x72A0=29344, reference0.10000001V. User says immediate after START.
The native write limit840 is exceeded18cycles; phase29344 also exceeds26880.
This is consistent with the first control step, not proof of its internal hotspot.

Reuse the existing seven-checkpoint diagnostic mechanism in the existing main
project; explicitly allow MAIN with diagnostic probes and give it its own OLED
identity. Keep all modes, PI, enable sequencing, DMA, software protection, ADC
phase and840/936 limits unchanged. Probes add time: profile data locates regions,
and cannot be treated as the uninstrumented858-cycle timing or a speedup.
No board connection/download/start by the assistant. This is a diagnostic
candidate, not a T3 fix or a continuous-output qualification.

Plan: RED main diagnostic compile/identity; enable existing capability with
identity; GREEN first-control partial-profile/latch/clear regression and old
suite; ARM build/actual-image check; independent review; document, commit/push
and verify remote SHA. No project copies or rebuildable products committed.
