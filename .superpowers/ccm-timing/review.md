# CCM execution comparison — independent review

Reviewer: /root/review_ccm_timing, GPT-6-astra high; 2026-10-26. Read-only inspection
of source/config diffs, map, HEX, disassembly, startup and saved verification logs.

Verdict: deliverable as restricted CCM comparison; no Critical/Important findings.
Verified seven hot functions in CCM, three correct Thumb veneers, standard
Reset_Handler -> SystemInit -> __main -> __scatterload, Flash-backed CCM copy table,
Flash vector resolving to CCM wrapper, and Flash-resident reset/emergency/fatal code.
All ordinary RW/heap/stack end at0x20001058, below the CCM alias.
The reviewer independently reran the read-only artifact verifier: CCM passes and
Flash baseline is rejected. No control/ADC/clock/protection/sequence change found.

Minor evidence issue: red-placement.log was empty because the initial terminating
PowerShell exception escaped redirection. Parent had already identified this and
reran the expected-failure check with explicit try/catch; the log now records
`Expected baseline rejection: Missing ER_CCMRAM execution region`.
No firmware change or extra host regression run was needed for this log correction.

Declined to judge: actual hardware cold boot; speedup/T3 elimination/Flash as root
cause; gate/deadtime/analog/closed-loop behavior; full audit of unchanged legacy
controller/protection design. These remain real board-validation requirements.

Evidence: MDK-ARM/ccm_diag_build.log0errors0warnings; host-tests.log19groups PASS;
placement.log actual MAP/HEX validation. No download/start performed.
