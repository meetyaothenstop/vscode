# CCM profiled T3 — 2026-10-26

Image: C:/Users/k/AppData/Local/Temp/codex-clipboard-3c6d902b-1a0e-4876-a477-06e306936af8.png
Context: user reports CCM target still64/T3 in preceding images; first other attempt
was128/S8 and must not be combined with this timing record.

Visible: entry_counter0xE60=3680; mask0x3F; checkpoint array
[0x65,0xB8,0x121,0x18C,0x37A,0x3F4,0].
Decimal cumulative[101,184,289,396,890,1012,invalid]; deltas[101,83,105,107,494,122,invalid].
The current crop omits entry_state. Do not infer same entry phase as the previous
Flash ARMING screenshot. Total after map is6.7467us, controller3.2933us including
probes. The exact before-write canonical measurement is later and not visible.

Next comparison removes unified probes but keeps all canonical timing protection.
No assumption that removing probes alone will satisfy the deadline.
