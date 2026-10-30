# Ledger — plan: docs/superpowers/plans/2026-10-30-main-write-budget.md

Base:4688502; inline execution in the existing codex/main-buck checkout.
Baseline46 host variants passed (phase_budget_baseline.log).
Ruling: keep the current checkout and branch — the user requires the existing
main project and current-branch history; moving would separate the live Keil
project from its build. Preserve all pre-existing IDE/untracked changes.
Ruling:128/64 are candidate engineering reserves, not proven WCET — static
instructions cannot bound board bus/FPU/exception timing; board validation is
required and the cost of an insufficient reserve is a timing stop or missed
update if detection comes too late. Do not report electrical safety as verified.
Task1: baseline/audit complete; RED timing tests next.

Task1: implementation and software verification complete. RED: old backend
rejects screenshot point at power_control_test.c1453. GREEN:47/47 groups including
first ARMING control step and inclusive936 original-mode DWT exit.
Main/bench ARM6.18 rebuilds0 errors0 warnings; actual HEX/CCM/no-probes/DMA and
main AXF export match. Main status address moved to0x20000B10; bench hash unchanged.
Independent review:0 Critical/Important. Final: minor (deferred): additional
wrapped-low-counter REP fixture; sticky REP is retained in source/ARM and the
current T6 fixture verifies pending-period shutdown.
Final: Ruling: unmeasured WCET/return margin/T4 elimination/OCP are not accepted
as proven — preserve board-unverified labeling; cost of insufficient headroom
is timing stop or missed-update behavior requiring board investigation.
Task1: complete for implementation/software verification; final documentation,
manifest and handoff checks passed. Release commit/tag/push verification is the
remaining delivery action and will be reported with the actual Git SHA.
