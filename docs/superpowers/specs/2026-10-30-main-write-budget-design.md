# Main forward Buck write-window candidate

User authorization: after discussing the software phase guard versus the hardware
update boundary, the user said to continue auditing and preparing the change.
Keep the existing project/branch and user-operated download workflow.

## Evidence and approaches

The post-OPT2 screenshot has prewrite elapsed792, spacing1000 and HRTIM counter27232.
The old guard26880 reserves160 CPU clocks before the32000-tick update boundary;
the screenshot still has149 clocks left. It does not establish worst-case tail time.
The actual ARM image has an inline paired write and no normal-path tail calls.
The IRQ saves/restores r4-r11 and d8-d9; exception entry/return and peripheral
waits are not included in a host test or instruction count. The old exit check
uses function-relative DWT only, so it is not an absolute phase guarantee.

Options considered: keep160 and restructure scheduling (larger algorithm/sequence
change); simply widen T4 (does not check downstream absolute headroom); a bounded
forward-Buck-only window plus an absolute tail checkpoint (selected candidate).
No RTOS, frequency change, control-step decimation, PI change or enable reorder.

## Candidate contract

- MAIN mode1 only: prewrite absolute phase limit27904, reserving128 CPU clocks
  (4096 ticks,0.853us). Cache the limit when accepting the request.
- All modes retain prewrite DWT840, relative exit936, entry spacing1256 and
  entry counter8192. Main Boost/reverse and the restricted bench retain T4=26880.
- MAIN mode1 adds a T5 check at the existing postwrite checkpoint: absolute
  counter must be<=29952 (936 CPU clocks), reserving64 clocks from that
  checkpoint for remaining checks/epilogue/exception return. This is a design
  reserve, not measured return margin. T6 pending-period detection remains.
- Keep paired preload-disable/write/re-enable logic unchanged. Failures disable
  all gates and latch first-fault elapsed/spacing/counter. No new ISR telemetry
  or profiling storage; keep the current fault fields and diagnostic budget840.
- Identity: MAIN ADC W128 6A / M-W128, diagnostics=0. Preserve four modes,
 6A reference,7.5A sampled trip, ADC phases, OCP truth and DMA behavior.

## Validation and limits

Use the real backend with a register-only DWT/HRTIM fixture to inject the captured
792/27232 point, exact prewrite27904/27905 and postwrite29952/29953 boundaries,
T3 precedence, original other-mode limits, T5 elapsed, T6 pending and first-fault
retention. Host timing injection tests policy only. Run all host groups and ARM
main/bench rebuilds; inspect actual HEX/CCM/no-probes/DMA and paired-write code.
Review generated tail instructions separately. Do not claim a proven worst-case
cycle bound: bus stalls/FPU/exception return must be checked on the board.
Hand off as a candidate, preserve the previous firmware by Git tag, commit/push
and verify remote SHA. The user alone downloads and repeats low-power24->12V.
