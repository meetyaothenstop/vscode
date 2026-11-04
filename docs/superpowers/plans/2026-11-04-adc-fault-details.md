# MAIN first-fault ADC entry diagnostics

User reports FRM1 OLED now refreshes normally, but fault1 can occur immediately
after START, right12V to left6V with100ohm. Fault1 is an ADC-validity guard;
the physical cause is not established.

Use inline implementation and one fresh read-only final reviewer. Preserve
all four modes, FRM1 OLED behavior, frequencies, PI and existing protections.
User downloads; no board actions. Baseline d54d61a, backup
pre-adcdiag-20261104 pushed and remotely dereferenced before firmware edits.

- [x] RED: real backend test for missing ADC1 JEOS, JQOVF, both, and invalid
  forward-Buck ADC2; cover pending START/bootstrap/running and reverse's
  unused late ADC2. Exact cached flags survive flag clearing and later events.
- [x] GREEN: MAIN event-enabled cold-only ADC context after emergency off,
  under the first-fault lock, valid published last. No normal-path probes.
  Reason bits1/2/4; new ADG1 identity and event version3. Disabled diagnostics
  and sole restricted bench retain their prior behavior.
- [x] Full host regression, MAIN/bench ARM rebuild, actual image checks and
  unchanged bench hash. Fresh independent review and fix important findings.
- [x] README, test handoff and verification manifest. Commit/tag/push and
  verify remote branch/tag. Do not claim physical fault1 solved.

Final review: no Critical/Important. Minor optional frame-resume-off identity
combination documented in handoff; code label cleanup deferred. All board
behavior/WCET/OCP and physical causes remain unverified; unrelated files
remain outside this change. Publication is checked by the executor's release
step; any failure must be reported as pending.
