# User-supplied first-control T3 evidence — 2026-10-26

Firmware title/Load: bench12_unified_diag / bidreaction_bench12_unified_diag.axf.
Fault64/detail3. write_budget840, exit_budget936, isr_cycles_max0 (early return).

Images:
- C:/Users/k/AppData/Local/Temp/codex-clipboard-aae49d46-e095-4565-a949-6f602fb881a1.png
- C:/Users/k/AppData/Local/Temp/codex-clipboard-2e1ecfa2-1be0-4d38-b696-0bfb14df0508.png

Profile stage7, entry_state2=ARMING, entry_samples0x839A1,
entry_spacing0x3E8=1000, entry_counter0x1720=5920, valid_mask0x3F.
Checkpoints: [0xC0,0x12E,0x20F,0x2DF,0x6F9,0x7EA,0].
Decimal cumulative: [192,302,527,735,1785,2026,invalid].
Section deltas: [192,110,225,208,1050,241,invalid].

Largest measured region: controller,1050cycles/7us including boundary probes.
After map:2026cycles/13.5067us; precise before_write is slightly later and was
not shown in these images. Point6 is invalid, not zero write cost. Current code
reaches stage7 from ARMING only after enabling outputs and entering RUNNING.
This evidence does not establish continuous RUNNING or exclude short gate pulses.

The earlier S9 sample0.58899051V/raw34/current-0.23059845A belongs to another
attempt. The no-trigger output observation accompanied a T3, not proof against S9.
