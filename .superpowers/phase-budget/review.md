# Independent review — W128 candidate

Fresh reviewer review_write_budget read the spec/plan, seven source/test diffs
against4688502, actual candidate ARM tail and root verification results.
Verdict: approve engineering-candidate handoff; no Critical or Important findings.

Checked cached publication/DMB, T3 precedence, paired preloads, retained T5/T6,
first-fault locking, other-mode cache values and real-backend fixture behavior.
ARM confirms no unexpected normal-path tail call, retained d8-d9/r4-r11 restores.

Minor (deferred): add a pending-REP case with wrapped-low exit counter640;
the present case uses29088. Production already checks sticky REP after phase T5.

Reviewer declined to judge measured WCET,64-clock return margin,T4 elimination,
electrical/OCP readiness, and final documentation/upload (not yet complete then).
Root ruling: these remain explicitly unverified board properties; validate
documentation and remote SHA separately before final handoff, no safety claim.

Static inventory, not cycle proof: paired normal preload transaction at
0x10000c64..0x10000c8a is13 instructions; prewrite T3/cache check at0x10000b8e..ba6;
postwrite absolute T5 and retained REP read at0x10000ca0..cb8. Bus wait, DMA
competition, FPU stack and architectural exception return are not certified.
