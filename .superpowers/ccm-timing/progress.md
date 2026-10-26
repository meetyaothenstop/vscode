# SDD ledger — plan: docs/superpowers/plans/2026-10-26-ccm-timing.md

Continuing the user-authorized T3 optimization after new real section-time evidence.
No Git repository: preserve original files and the Flash unified HEX in originals/.
Pre-flight: Task2's CCM identity is valid only when Task1's linked-image verifier passes.
Task1: complete. Flash baseline rejected by placement checker (red-placement.log). CCM build 0 errors/0 warnings; linked MAP/HEX checker passes, including copy table and vector, 3684-byte CCM region loaded from Flash.
Task2: complete. CCM menu identity RED (red-identity.log), then GREEN. Full host suite19groups passed; independent review found no Critical/Important issue. Missing negative-test log corrected by explicit try/catch; independent reviewer also confirmed baseline rejection.
Final verification: linked MAP/HEX checker rerun PASS; source power_control.c/.h match the Flash-baseline manifest; original Flash HEX preserved. Runtime speedup and cold boot await board evidence; no flash/start performed.
