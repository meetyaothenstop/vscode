# RES1 review and verification — 2026-11-03

RED: real backend preparation test failed on W128 because LOW_OUTPUTS were
enabled in the calculation IRQ. GREEN:48 full groups, including staged
30+2 sequencing, saved pair, fresh revalidation/cancel, STOP/zero/clear,
ADC/input/output/current faults and hardware short-fault interleavings.
Preparation, early enable and cancellation all retain T5/T6; the old
907/31072 event is still rejected. Register time is synthetic, not ARM WCET.

Independent read-only reviewer review_reverse_resume found no remaining
Critical/Important/Minor. Verified same-priority Timer/FLT behavior, goto
does not skip a VLA or any variable used at the exit, fresh protections,
preload boundary and full bootstrap/arming periods, pending cancellation.

An initial apparent instruction-count difference was a counting-scope
mistake: old13 omitted two status writes/branch. Reviewer rechecked and
withdrew that finding. New/old same-scope hardware pair both13 with matching
opcode order. A noinline bootstrap experiment was reverted before final
build; no scatter/verifier changes remain. No intentional extra mode changes.

Final ARM6.18 MAIN and bench0errors/0warnings. Actual HEX/CCM/copy/vector,
no probes, ordinary-SRAM DMA, AXF export equality and identities verified.
MAIN Code47048/RO1320/RW44/ZI4508, CCM9060/load0x0800998C. Bench HEX unchanged.
Final hashes and fresh MAP addresses are recorded in manifest.json.

Board timing, output/HOLD cause, regulation and hardware OCP remain unverified.
Only the firmware user downloads it. No board operation was performed.
