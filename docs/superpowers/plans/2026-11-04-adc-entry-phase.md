# ADC fault entry spacing/phase observation

Continue the authorized ADC-validity investigation. User supplied ADC1 flags1,
ADC2 flags0xF, reason1, cold BOOTSTRAP remaining30, and confirms continuous
execution with only the board START button. Missing JEOS is established;
physical trigger loss, late completion and prior IRQ clearing remain distinct.

Constructed real-backend late-entry schedule reproduces those flags/stage:
first IRQ phase750 CPU cycles consumes this period's completed ADC1, next
normal IRQ phase20 arrives270 cycles later with no new ADC1 sequence yet.
This is a hypothetical schedule, not proof of physical lateness. Linked MAIN
also clears/copies two172-byte event mailboxes inside the start PRIMASK lock;
its board duration has not been measured.

Use inline execution and one fresh read-only final review. Existing branch
and unrelated edits remain; baseline7564102 backed up and remotely verified
as pre-adcphase-20261104 before editing. Keep all four modes, sole bench,
FRM1 OLED, rates/PI/protection; user flashes, no board actions.

- [x] RED: existing real-backend ADC test expects cached entry spacing and
  counter in existing PowerStatus fault fields; preserve freeze/clear semantics.
- [x] GREEN: pass already-read begin/spacing/counter only on ADC-fault branches,
  off before recording, retain same first-fault lock. ADG2/event version4.
  Add the constructed schedule with real ISR masks, plus normal-phase control.
- [x] Full71groups, details/event-disabled compatibility, ARM MAIN/bench0/0,
  actual image/CCM/stack/DMA/AXF/integer checks, same bench hash, fresh review.
- [x] Document evidence versus hypothesis and next measurement; README,
  manifest, commit/tag/push with remote verification. No physical fix claim.

Fresh read-only review:0 Critical/Important/Minor. Board timing, startup lock
duration, physical root cause and OCP remain unverified; final release upload
is checked by the executor, and any failure must be reported as pending.
