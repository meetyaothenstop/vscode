# SDD ledger — plan: docs/superpowers/plans/2026-10-25-bench12-buck.md

Pre-flight: one integrated task; no cross-task interfaces. User selected
12 V ADC-only testing and proposed a 0.2 A adjustable supply limit.
Ruling: work in place with .before backups because this workspace has no
Git repository; no branch, worktree, commit, or Git helper scripts apply.
Implementation choices and limits are in the linked spec, not user-measured
capabilities. Firmware preparation does not authorize an automatic download.

Task 1 RED: backend test failed on a valid ADC-only 12->6 request with
fault 512; menu test failed because the old default was 36 V. Logs saved.
Task 1 implementation: separate build profile, truthful readiness, source/
mode/target restrictions, reduced sampled current limits and SW UI label.
Host suite: all 14 groups passed; host_tests.log contains the full output.

Final review: independent read-only reviewer identified two Important issues
and no other Important/Critical findings; both are addressed below.
Final: fixed forced minimum Buck energy — bench12_switching_test RED->GREEN.
Before: 12->6/100-ohm ideal model peak2.571 A, sampled+2.526/-1.444 A;
after bench null duty: Vout5.999 V, peak0.561 A, sampled max0.520/min0 A.
Final: fixed enables preceding fresh checks — cold_start_samples_rechecked
RED->GREEN in both default and bench tests. OENR remains zero for invalid
pending-start samples. Cold arming rechecks residual voltage/current.
The first expanded host run caught a test-fixture expectation changed in
BOOTSTRAP rather than ARMING; corrected the fixture to reflect actual stages.
Suite after both fixes: all15 groups PASS in host_tests.log.
Final: Ruling: physical gate/analog/ISR/stability verification cannot be
established by host tests; retained as explicit board checks in usage docs,
not a software readiness claim. No deferred minor findings.

Task 1: complete — all15 host groups pass after final fixes; both final
Keil full rebuilds exit0, 0 errors/0 warnings. Default Code35108/RO1272/
RW40/ZI4080; bench Code32468/RO1272/RW28/ZI3884. Build logs are
MDK-ARM/bench12_default_build.log and MDK-ARM/bench12_buck_build.log.
Default uvprojx and power_protection.h match their .before backups.
Final default HEX SHA256:
AFC72F4AE1FA08971BD157938DA8090DA23AF2FCFBC224C7BD44A7A29DAAA0F9
Final bench HEX SHA256:
D4C9617447EA143C7AB4A3A70EEB8AAB645F8911FB3AA1FD0F34107F90326574
No board download/start performed. Git-specific commit/merge/cleanup steps
do not apply; retained the review, tests, logs and .before recovery artifacts.

Follow-up diagnostic preparation complete: after user's corrected download,
separate attempts report128/64/2. Added first-fault S/T details, latched timing
and ADC evidence, visible input-fault sample and DIAG homepage. Current guard
values preserved. Independent review: no Important/Critical; one duplicated
diagnostic threshold minor deferred. Full15 host groups PASS, default/bench
Keil0 errors/0 warnings. New hashes and investigation record live in
.superpowers/bench-fault-diagnostics/diagnosis.md. Board root cause remains
open; user needs to download this image and report first fault detail.
