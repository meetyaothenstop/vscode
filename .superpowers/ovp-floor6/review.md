# Self-review of forward Buck6V OVP floor

Reviewed the working diff against base4f3dfe8. Only MAIN mode1 request-time
OVP calculation and firmware identity change. Floor requires target<6V and
original_trip<6V; cap40V and cached IRQ comparison remain. Valid nonzero
requests equal min(max(target*1.5,6),40);0V exits before this calculation.
Other mode ratios, startup1A/3V, running protection, first fault and W128 stay.

Tests use the real backend/ADC conversion; independent literal edges cover
the6V floor,4V crossover, proportional thresholds just above it,40V cap,
cache replacement and reverse3V3.75V isolation. Old policy failed the supplied
3V/4.729247V replay; candidate passed all47 host groups. Both ARM rebuilds
report0 errors/0 warnings; actual HEX verifier and AXF export agree. Bench
hash is identical to base. Labels fit21 columns in menu/UI regressions.

Actual ARM selects the mode1 floor at0x080079de..7a0c, retains the cap and
caches at0x08007a3a before pending publication. Home stores decode to
MAIN ADC OVP6-40; parameter suffix is present in actual HEX.
No blocking issue found in this self-review. No independent reviewer was used.

Board electrical behavior/root cause, measured hardware WCET and OCP readiness
remain unverified. Preexisting MDK project/UI edits are outside the change.
