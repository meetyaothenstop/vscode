# OPT2: inline Buck PWM mapping — 2026-10-30

Base dd3e6ff. Existing bidreaction project, no copies, preserve user IDE files.
User reports64T4 after OPT. Read-only existing Watch shows fault64/detail4,
elapsed0x326=806, spacing1000, counter0x6C20=27680, reference0.10000001.
Input fault24.0211468V/output0.0692929998V/current0.100805283A.
Native840 deadline now passes; actual phase exceeds26880 by800ticks=25CPUcycles.
Previous native fault858/29344 was a different attempt; observed difference52
cycles is not a controlled benchmark or proof of worst-case margin.

Second optimization attempt: expose the existing shared forward mapper through
an always-inline Buck API, letting the IRQ compiler remove call overhead and
unused intermediate plan fields. Same formulas/helper for generic/Buck paths;
reverse expressions unchanged. No limit, phase, PI, enable-order or clock edit.
Actual ARM/map must show no out-of-line Buck mapper reference from control IRQ.
API differential200016 cases and old protection/fault/mode tests remain gates.
First ARM inlining inspection still showed stores of unused plan duty fields:
the private HOLD resume whole-struct copy retained them. Preserve only its two
compare fields (the only fields write_plan consumes); recheck HOLD/resume tests
and real ARM output. No change to gate enable or compare value sequencing.
Second ARM inspection still retained duty stores because generic mapper calls
expose the shared plan address. Use a compare-only always-inline adapter with
its own non-escaping full temporary; keep shared formulas, write only the two
backend compare fields. Differential tests include invalid/null output retention.
An early zero-conversion branch spike was not retained: ARM still speculated
the conversion. Keep the original arithmetic/order and null override unchanged.
Final delivered scope is inlining, non-escaping compare-only adapter and saving
only the two private resume comparisons. No cycle saving is inferred from size.
New identity MAIN ADC OPT2 6A / M-OPT2; diagnostics stays explicitly0.

Plan: RED out-of-line mapper image check and identity; move unchanged helper
to private header, always-inline Buck API; GREEN all46 host groups; main/bench
ARM and actual HEX/CCM/DMA/no-probes/export checks; fresh read-only review;
docs/manifest, commit/tag/push, verify remote SHAs. User alone downloads/tests.
Board timing improvement and T4 elimination are unverified until remeasurement.
