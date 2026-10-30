# SDD ledger — plan: docs/superpowers/plans/2026-10-29-main-buck.md

Base: `c59df1c`; branch: `codex/main-buck`.
The user requested integration into the existing main project, Buck nominal
12–40V input, 0–36V output menu and 6A current control. They explicitly confirmed
retaining Boost/reverse and ADC software-protection testing. Hardware readiness
is not forged. The user operates downloads; this task performs no board actions.
The same checkout is retained, with pre-existing bench IDE changes untouched.

Ruling: 6A is the reference clamp; original main sampled trip stays 7.5A — allow
tracking transients — actual instantaneous current is not guaranteed below 6A.
This distinction was communicated to the user. Buck sample protection is
10–42V for nominal endpoint margin; OVP is target×1.25. Zero means all-gates-off,
passive discharge, fault preserved. Other modes retain their original limits.

Owners: architecture — forward template/equivalence; backend — zero/protection/
ADC selection; UI — menu/DMA front end; root — configuration/project/scatter,
driver/PWM capabilities, runner, builds and documentation. Interfaces are in spec.

Task 1: complete. Single template emits fast/full helpers. RED/GREEN tests cover
skipped Boost work, 540,000 Buck / 120,000 Boost oracle samples and mode resets.
Task 2: complete. Software-only and guard variants pass, including 32 zero-state
combinations and a RED/GREEN STOP/clear sample-freshness regression.
Task 3: complete. Software-only and guard menu/UI variants pass.

Nominal main model 12→1/6/11, 24→12 and 40→1/12/36V settles. At 40→36V the sampled
peak is about 1.507A. Ruling: use a 2A main-model regression bound for the larger
range — retain the original bench 1.5A bound — physical load stress remains
unverified. Neither model bound changes the actual 6A reference or 7.5A trip.

Concurrent owner full-suite attempts hit shared executable locks. The root's
final serial run after all owner edits supersedes those attempts: **43/43 PASS**,
including the original 32 groups.

Initial sandboxed main build reported missing RTE packages. Audit found installed
PDSCs but sandbox reads denied. Original RTE metadata was restored; escalated
build passed without package/version changes. Main ARM6.18: zero errors/warnings,
Code45500/RO1288/RW44/ZI4476; CCM7712 bytes, load0x080098A4/run0x10000000.
Actual HEX verifies vectors/copy, fast/full dispatcher, reverse step/hold/resume
and ordinary DMA SRAM. Main HEX SHA256:
`C21781791837435A881C1C96CB96A9982E10094D842736E7459EFF39D50A7749`.
The AXF export matches exactly; the separate AXF hash is in the manifest.
Bench rebuild: zero errors/warnings, CCM3380 bytes, load0x080095BC. HEX SHA256:
`7D9ADE7EC0931D15CEA20725440093E05D75BE36B98B11FE4B0A2B6FB23C0056`.

Task 4: code/build/documentation complete; fresh whole-change review in progress.
README/AGENTS/main manual/test records/manifest are updated. Electrical behavior,
full-power operation and worst-case board timing remain unverified.

2026-10-30 resume: 43/43 serial host groups pass again. Main ARM rebuild has
zero errors/warnings and the identical HEX fingerprint. CCM/image checks pass.
The previous reviewer failed on usage limits without producing findings; a fresh
replacement final reviewer completed with no Critical/Important code findings.
GitHub read access works over HTTPS via
the existing local proxy; SSH origin is unchanged. Only the explicit 27 source,
test, project and documentation paths were staged; review disposition follows.

Final: Ruling: the minor README primary-HEX mismatch is a delivery correctness
issue because it could cause selecting the restricted bench image. Correct the
existing main/bench wording before handoff to satisfy the user's current README
requirement. The contradictory text was reproduced by reading README lines186–189;
after correction verify the primary output matches the project manifest. No
firmware source changes or additional electrical claims result from this fix.
Final: Ruling: hardware OCP readiness, board timing, electrical stability,
full-load capability and flashing outcome remain unverified; host tests and
linked-image checks do not establish those properties. User-operated board
validation is still required. No deferred code findings.
Task 4: review and local delivery artifacts complete. Delivery procedure: commit,
push branch and candidate tag, then verify both remote SHAs before claiming upload.
