# Reverse Boost7V Undervoltage Trial

**Goal:** User-requested7V undervoltage trial for MAIN mode4 after right12V/0.9A limit/100ohm/18V target trips code2 at8.99V.
**Architecture:** Select a mode4-only input minimum at request and active-policy caching; all fresh raw/float IRQ stages use the existing cached limits. Other modes retain9V; no ADC/PI/rate changes.
**Tech Stack:** STM32G474 C, GCC backend fixtures, existing Keil ARM6.18 main/sole bench.
**Spec:** User explicitly requested7V; scope is the presently tested reverse Boost. Preserve user12→18V reverse defaults, forward18→3V defaults, IDE state and all other protections. Hardware cause remains unresolved; no board operations.

- [x] Preserve user defaults in e197a11; push and verify pre-reverse-boost-uv7-20261109 before edits.
- [x] RED: existing9V policy rejects new7.1V reverse Boost request.
- [x] Add POWER_MAIN_REVERSE_BOOST_INPUT_MIN_VOLTS=7.0f and shared request/cache selection helper; mark UI R7/R25V7.
- [x] Verify mode4 accepts7.1V/8.99V at request and every fresh phase, rejects6.9V and the adjacent ADC code below7V, retains42V upper bound. Other MAIN modes must still reject8.9V.
- [x] Run full86-group suite, main/bench rebuild0/0, actual image checks and unchanged bench HEX; obtain read-only review.
- [x] Update README/fault checklist and concise test record.

Finalization: commit/tag main-reverse-boost-uv7-20261109, push and verify remote SHA before claiming upload complete.

Review focus: request-only update; accidental widening of other modes; wrong reverse input bus; stale UI identity; missing inter-request fast protection. Tests cover these through the existing backend and image fixtures.
