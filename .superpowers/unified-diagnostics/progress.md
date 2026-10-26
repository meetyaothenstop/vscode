# SDD ledger — plan: docs/superpowers/plans/2026-10-25-unified-diagnostics.md

User authorized execution of the proposed unified diagnostic build in this chat.
Ruling: execute directly with backups because the supplied outer project is not a Git repository; no worktree/commit or Git-only ledger scripts can run here. Originals retained under `originals/`.
Pre-flight: Task 2 consumes the opt-in macro and debugger types from Task 1; the menu, host test build and Keil target must use identical flags.
Task 1: complete. `red-api.log` missing diagnostic API; `red-stages.log` runtime wrong IRQ context; implemented probes/capture and `green-backend.log` passes. Full `tests/run_tests.ps1` passed 17 cases (`host-tests-task1.log`).
Task 2: complete. Identity RED (`red-identity.log`) then GREEN; complete suite passed 18 cases (`host-tests-final.log`). Diagnostic and private uninstrumented comparison ARM builds passed 0 errors/0 warnings.
Ruling: use native PowerShell for Keil project preparation because Python is not installed in this shell; no dependency installation is needed.
Final review: fresh reviewer /root/review_unified_diagnostics; no Critical/Important findings. Two Minor test gaps deferred in review.md (remaining-count assertions, capture-time assertion).
Final scope: diagnostic implementation complete; board measurements, root-cause fixes and OLED DMA are not claimed complete. No download/start performed.
