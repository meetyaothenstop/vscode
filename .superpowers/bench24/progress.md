# SDD ledger — plan: docs/superpowers/plans/2026-10-29-bench24.md

Base9799a57. Existing dirty uvprojx/uvoptx and untracked directories are preserved.
Ruling: continue same feature branch/checkout and single active Keil project, per user.
Ruling: user delegated appropriate voltage margins; choose20–28V input and14V OVP at12V
target, proportional for lower targets, not a full16V OVP scale-up. Current/PI/timing unchanged.
Cost if wrong: board may still trip on genuine transients; physical ratings remain to confirm.
Ruling: keep a24 flag default FAST, with oldFAST explicit0 regressions; no duplicated projects.
Task1 produces config macros; Task2 consumes them for menu/default/model, no interface ambiguity.
Hardware state: user reports12→6 and OLED works. No board actions in this task so far.
Physical resistor power/capacitor voltage question pending; this does not block offline preparation.
RED: new backend compiled then failed request(1,12) on old6Vcap; new menu failed expected24input.
GREEN: new24 backend (initial0.5Aref) input20..28,12target,14Vfirsttrip and cold/ADC/timing/STOP PASS.
Discovery: actual C nominal24/12 switch test at0.5Aref settles6.367V even4seconds; not a tuning claim.
Ruling: revise24-profile reference to1A (old12stays0.5), sampletrip remains1.5A, PI/phase unchanged.
Probe0.75A fails28Vinput (~8.826V);1A passes20/24/28, output11.997..11.998V,
worstvoltage12.589V and sampledcurrent1.079A. Cost: board-current calibration/model omissions
remain; user informed and actual hardware run still needs STOP/rating confirmation.
Task1 complete:24V backend GREEN at1A ref; exact request bounds, first ADC count above14V,
all prep/run stages input/ADC errors, cold limits/current/reverse/timing/STOP covered.
Task2 code complete:24menu/UI and actual20/24/28model GREEN; old12 numerical guards unchanged.
Verification: full32/32 host PASS, ARM6.18 zeroerrors/warnings; Code40416 RO1288 RW32 ZI4264;
CCM3384bytes load0x080095B0. ActualHEX CCM/copy/vector/uninstrumented andOLEDvector/SRAM PASS.
SHA256 F07116B02C42882430DAC75602AEECB8F07C36F70CA7D4FD1BD4841EB189E631.
Independent final review pending. Hardware ratings still unanswered; no board actions.
Final review complete: noCritical/Important; oneMinor comparison-test compatibility issue.
Final: minor (deferred): optional24V early-input=0 backend-test guards andLIVE=0 menu-test
identity expectations need follow-up; default active firmware/32groups unaffected.
Final: Ruling: hardware ratings/calibration/instantaneousstress/ISRtiming and other targets/loads
are not established by nominal12V/100ohmmodel; do not energize24V or claim boardverification.
Cost if wrong: physically unsuitable components or unmodeled dynamics require revised setup.
Final: Ruling: preserve unrelatedIDE/scratch and samebranch; user standing agreement authorizes
selected-file commit/push, no merge or forcepush. RemoteSHA will be checked at delivery.
Task2 complete: docs and softwarecandidate ready, hardwareconfirmationpending.
2026-10-29 follow-up: user reports load rated150W andoutputcapacitor50V; record onlythese,
not input/wholeboardratings. Firmwarehash unchangedF07116B02C42882430DAC75602AEECB8F07C36F70CA7D4FD1BD4841EB189E631.
FreshSTOP/output-off,currentinputandinputcapacitorrating requested; no hardwareactions so far.
Follow-up user:input/outputsymmetrical,currentinput24V;freshnormalSTOP/outputoff confirmed.
CUwindow minimized twice despite launchrecovery;purposeKeilCLI-f used on sameproject/target.
Flashlog18:46:59 EraseDone/ProgrammingDone/VerifyOK,exit0. AXFexportHEXexactlymatchescandidateSHA.
AXFSHA7DCC7EAE528F9825CB8F2F1F893B55269DDC84A38BD858F61DCF5968CAD3E790.
Power-startnotsent;OLEDidentityand12Vpoweroutputfeedbackpending. This isflashverificationonly.
