# Checkpoint

## 2026-06-12 Mechanics And Storage Precedence Slice

Completed:
- Added CASP3 vertical upward SUVAT working for maximum height and return time prompts.
- Added CASP3 `from u to v over s` constant-acceleration working for acceleration and time.
- Added CASP3 Poisson distribution notation routing for hypothesis tests before exact-probability fallback.
- Added CASP3 normal mean-test free-text routing before generic summary-statistics fallback.
- Added CASP3 rough-horizontal force-at-angle working, including vertical reaction adjustment and friction.
- Added a tolerance for polar-force equilibrium checks where common-angle trig approximation leaves a tiny residual.
- Added CSCALC image-size precedence for `bit colour depth` phrasing.
- Added CSCALC colour-count wording for `how many colours with colour depth ...`.
- Added CSCALC MiB/KiB/GiB bitrate scaling without changing existing MB/KB wording.
- Added CSCALC signed fixed-point encode routing before integer two's-complement fallback.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text routing and working-quality support.
- Did not touch CAS Pure behavior, NOTES, menus, or shared UI/status code.

## 2026-06-12 Generic Routing Precedence Slice

Completed:
- Added CASP3 `X~B(...)` precedence for normal-approximation prompts before exact binomial fallback.
- Added CASP3 `X~B(...)` precedence for hypothesis-test prompts before exact binomial fallback.
- Added CASP3 lift-tension free-text working for upward/downward acceleration.
- Added CSCALC base-16/base-2 phrase routing, including alpha-only hex values such as `FF`.
- Moved two's-complement decode/value prompts ahead of generic binary-to-denary fallback.
- Added CSCALC normalised-floating-point single-mantissa wording.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text routing and working-quality support.
- Did not touch CAS Pure behavior, NOTES, menus, or shared UI/status code.

## 2026-06-12 Distribution Notation And CS Wording Slice

Completed:
- Added CASP3 notation parsing for `X~N(...)`, `X~B(...)`, and `X~Po(...)` independent of number order.
- Added symbolic probability tail parsing such as `P(X<=r)`, `P(X>=r)`, and interval `P(a<X<b)`.
- Added CASP3 handling for `standard deviation squared` / `sigma squared` as variance.
- Added CSCALC base-N wording like `base 10 to base 16` and `base 16 to base 10`.
- Added CSCALC symbol-bit wording for `how many bits for ... symbols`.
- Added CSCALC floating-point decode wording when mantissa/exponent bit strings are given without labels.
- Preserved truth-table variable names from prose such as `variables X Y`.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text routing and working-quality support.
- Did not touch CAS Pure behavior, NOTES, menus, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Output And CASP3 Stats Intent Slice

Completed:
- Fixed CSCALC Boolean prompts written as `simplify output F = ...`.
- Fixed CSCALC output-column prompts such as `derive boolean expression for output column 0110 variables A B`.
- Fixed CASP3 discrete random variable prompts with `values ... probabilities ... find mean and variance` so they route to E(X)/Var(X) working instead of summary-statistics working.
- Fixed CASP3 histogram prompts asking to find frequency from frequency density and class width.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text routing and working-quality support.
- Did not touch CAS Pure behavior, NOTES, menus, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Assignment Notation Slice

Completed:
- Fixed assignment-style Boolean prompts like `simplify F = A and B or A and not B`.
- Fixed worded assignment prompts like `simplify Q equals A B + A C`.
- Fixed truth-table prompts written as `truth table for F = ...`.
- Kept genuine identity/proof prompts on `boolprove(...)`, so `prove A+B = B+A` still compares both sides.
- Avoided target-only `_snprintf` link failure by using the existing calculator-safe `sprintf` style.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean/free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, menus, or shared UI/status code.

## 2026-06-12 CASP3 Distribution And Horizontal Mechanics Slice

Completed:
- Added `B(n,p)` distribution notation parsing for binomial probability prompts.
- Added word-number handling for `at least one` in `B(n,p)` binomial tails.
- Fixed inverse-normal quantile wording like `find k such that P(X<k)=0.95`.
- Fixed upper-tail inverse-normal wording like `P(X>k)=0.05` by converting to left-tail area.
- Added horizontal force-component wording.
- Added smooth/rough horizontal-plane acceleration working, including friction on rough horizontal planes.
- Guarded the new horizontal-plane route so it does not steal inclined-plane questions.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 free-text support.
- Did not touch CAS Pure behavior, CSCALC logic, NOTES, menus, or shared UI/status code.

## 2026-06-12 CSCALC Storage And Float Range Wording Slice

Completed:
- Added generic colour-depth inverse working: colours from bits per pixel, and bits per pixel from colour count.
- Added generic symbol-bit working for prompts like `257 different symbols`.
- Fixed ASCII/Unicode storage prose so it routes to text-size working instead of code-point conversion.
- Broadened floating-point range wording so `largest positive`/`smallest positive` mantissa-exponent prompts show range working.
- Tightened the float-range route so it does not steal `extra mantissa bits needed` prompts.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 free-text calculation support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, menus, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Old-Syntax Slice

Completed:
- Located the earlier Python Boolean algebra implementation in git history (`src/ComputerScience/booleanProgram.py`).
- Restored old comma-as-NOT input syntax for Boolean expressions.
- Fixed `bool(...)`/`truth(...)` parsing so commas inside one Boolean expression are not treated as separate command parameters.
- Fixed free-text NAND/NOR conversion phrases like `convert A and B to NAND only` and `NAND form for A and B`.
- Fixed free-text `not not` and comma-NOT proof phrases.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra/free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, menus, or shared UI/status code.

## 2026-06-12 CASP3 Mechanics And Binomial Prose Slice

Completed:
- Fixed SUVAT prose like `accelerates from rest to 30 m/s in 12 seconds` by inferring final speed from `from rest to`.
- Added symbolic variable-acceleration working for prompts like `a=6t-4 ... find v and s` with initial velocity and position.
- Fixed binomial `less than or equal to` tail parsing so it routes to `P(X<=r)`, not `P(X<r)`.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 free-text support.
- Did not touch CAS Pure behavior, CSCALC logic beyond previous committed slice, NOTES, menus, or shared UI/status code.

## 2026-06-12 CSCALC Storage Routing Hardening Slice

Completed:
- Fixed sound-file prose so sample rate/duration/resolution/channel prompts route to sound-size working, not bit-rate working.
- Fixed dotted fixed-point binary free text so `1011.011` is decoded as fixed point, not plain binary.
- Fixed memory address prose with word size, e.g. `2 GB ... 32 bits`, by using the second numeric value as word bits when labelled parsing is absent.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 storage/number-system free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, menus, or shared UI/status code.

## 2026-06-12 CSCALC Transfer Unit Free-Text Slice

Completed:
- Fixed free-text transfer/download/sent prompts with byte units and bit-rate units.
- Added generic conversion to bits and bit/s for KB/KiB/MB/MiB/GB/GiB and kbit/Mbit/Gbit rates.
- Token-matched `sent` so words like `representable` no longer trigger transfer-time handling.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 storage/data-rate free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, menus, or shared UI/status code.

## 2026-06-12 CASP3 Binomial Stats Prose Slice

Completed:
- Fixed binomial prose without the word `binomial`, e.g. faulty-component/sample questions.
- Fixed `n 500 p 0.02` word-number parsing for binomial/normal-approx prompts.
- Routed normal-approximation binomial tail prompts before generic normal routes.
- Fixed coin hypothesis test parsing so `5 percent` is alpha and not the trial count.
- Added simple two-tailed handling for `hypbinom(..., tail=0)`.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 free-text hardening for Paper 3 stats.
- Did not touch CAS Pure behavior, CSCALC logic, NOTES, menus, or shared UI/status code.

## 2026-06-12 CSCALC Spaced Binary And CASP3 Projectile Wording Slice

Completed:
- Fixed CSCALC base conversion for spaced binary groups, e.g. `1010 1100` now converts as one byte.
- Fixed CSCALC logical/arithmetic shift prompts where the bit pattern was being mistaken for the shift count.
- Added explicit shift-count parsing from `by N`.
- Added CASP3 projectile synonyms for `thrown`, `fired`, and `launched`.
- Added CASP3 word-number parsing for projectile speed, angle, and launch height/from-above wording.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text hardening.
- Did not touch CAS Pure behavior, NOTES, menus, or shared UI/status code.

## 2026-06-12 CASP3 Rough Plane Free-Text Slice

Completed:
- Fixed rough inclined-plane prompts being stolen by the generic friction route.
- Added label/word parsing for `mass`, `angle`, `mu`, and coefficient/friction wording.
- Added a fallback for common exam wording where coefficient appears before angle.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics free-text support.
- Did not touch CAS Pure behavior, CSCALC logic, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC BCD And Spaced Binary Arithmetic Slice

Completed:
- Added BCD encode/decode working lines via `bcd(...)` and `bcddec(...)`.
- Routed free-text prompts like `convert 39 to BCD` and `decode BCD 0011 1001`.
- Fixed spaced binary addition prompts like `add 1 0 1 1 and 0 1 1 0 in binary`.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 number-system/free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Spaced Truth-Column Slice

Completed:
- Fixed Boolean truth-table free text where output columns are written as separated bits, e.g. `0 1 1 0`.
- Added generic spaced-bit-column scanning and routed truth/output column wording through `truthbits(...)`.
- Preserved existing contiguous output-bit handling such as `0110`.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean/free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Hex Token Base-Conversion Slice

Completed:
- Fixed free-text base-conversion prompts containing hexadecimal tokens with letters, e.g. `3F`.
- Added real hex-token detection so `convert hexadecimal 3F to binary` no longer degrades to `3`.
- Routed hex-to-binary, hex-to-denary, and binary-to-hex wording through the existing base conversion working engine.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 number-system free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 SUVAT Natural Wording Slice

Completed:
- Fixed CASP3 SUVAT free-text prompts using exam wording such as `starts from rest`, `accelerates at`, `reaches speed`, `for ... seconds`, and `in ... seconds`.
- Added requested-output handling so distance prompts with `u,a,t` show `s = ut + 1/2at^2` instead of defaulting to final velocity.
- Added combined acceleration-and-distance working for `u,v,t` prompts.
- Guarded the broader SUVAT route so it does not steal variable-acceleration prompts.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics free-text support.
- Did not touch CAS Pure behavior, CSCALC logic, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Image Byte-Depth Slice

Completed:
- Fixed free-text bitmap/image size prompts where colour depth is given as bytes per pixel.
- Added a visible method line converting bytes per pixel to bits per pixel before applying the image-size formula.
- Kept normal bit-depth prompts unchanged, including `24 bit colour`.
- Added regression tests for plain and labelled byte-depth image prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 image/storage free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Storage Unit Scaling Slice

Completed:
- Fixed free-text sound/storage routing so sample rates written as `kHz` / `MHz` are converted to Hz before file-size working.
- Fixed duration phrases using minutes, hours, or milliseconds so they are converted to seconds before sound and bit-rate working.
- Added regression tests for AQA-style sound file size with `44.1 kHz` and `2 minutes`.
- Added regression coverage for bit-rate prompts using minutes.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 storage/data-rate free-text support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Proof Phrase Slice

Completed:
- Located the old Boolean Python work in git history/current legacy paths (`ComputerScience/booleanProgram.py`, `src/calc_files/boolean.py`, legacy copies).
- Fixed Boolean proof free text so `equals`, `is equal to`, and `equivalent to` split LHS/RHS instead of being parsed as variables.
- Added regression tests for distributive, commutative, and XOR identity proof phrases.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC Boolean algebra free-text working support.
- Did not touch CAS Pure behavior, CASP3 logic, NOTES, or shared UI/status code.

## 2026-06-12 CASP3/CSCALC Exam Phrase Routing Slice

Completed:
- Fixed CASP3 normal probability phrasing like `70 less than X less than 95` so it routes to two-bound `normalprob` working.
- Added CASP3 biased-coin probability inference without requiring the word `binomial`.
- Fixed CSCALC two's-complement encode prompts containing `binary` so they are not stolen by plain binary conversion.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text working support.
- Did not touch CAS Pure behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Free-Text Routing Slice

Completed:
- Added `projected` / `projection` as projectile wording, so common exam phrasing routes to projectile working.
- Added spaced word-number parsing for stats phrases such as `mean 50 standard deviation 8`.
- Fixed normal-tail inference to choose the unused threshold number instead of the last number.
- Guarded conditional normal probability so it still reaches `normalcond`.
- Added coin-test inference for hypothesis-test wording with default `p=0.5` and percent significance conversion.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 free-text working support.
- Did not touch CAS Pure behavior, CSCALC logic, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Storage Representation Free-Text Slice

Completed:
- Fixed sound-file free text so `stereo` implies 2 channels and `mono` implies 1 channel.
- Added `fixedfrac(...)` for fixed-point encoding when a prompt gives only the decimal value and fractional-bit count.
- Prevented generic binary conversion from stealing fixed-point conversion prompts.
- Added `addressbits(...)` / `minaddressbits(...)` / `addresslines(...)` for inverse memory-address sizing.
- Added free-text routing for prompts such as “how many address bits are needed to address 4 GB of memory”.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 storage/representation calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Bitrate File-Size Slice

Completed:
- Added `bitratemb(...)` and `bitratekb(...)` so file-size/time questions convert bytes to bits before computing data rate.
- Added free-text routing for prompts such as “bit rate for file size 12 megabytes transmitted in 6 seconds”.
- Preserved explicit download/transfer-time routing when a prompt asks for time with a given bitrate.
- Added regression tests for MB and KB bitrate working.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 storage/data-rate calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Minimum Bit-Width Slice

Completed:
- Added `bitsneeded(...)` / `minbits(...)` / `bitwidth(...)` for unsigned, two's-complement, and sign-and-magnitude representation widths.
- Added free-text routing for “minimum/fewest/smallest bits needed” prompts without intercepting floating-point mantissa precision questions.
- Added regression tests for unsigned `150`, two's-complement `-45`, and sign-and-magnitude `-45`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 representation calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Planned Command Alias Slice

Completed:
- Added first-class aliases from the multi-app plan: `normal_work`, `binom_work`, `prob_work`, `regress_work`, and `hyp_test`.
- Routed aliases through the existing generic stats engines, preserving the same working lines and avoiding duplicated logic.
- Added help-surface entries for the aliases.
- Added regression tests for direct alias commands.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 command coverage.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Proof Working Slice

Completed:
- Added generic Boolean covering-law simplification for `A + A'B -> A+B` and the dual `A(A'+B) -> AB`.
- Added side-by-side algebra trace lines for `boolprove(...)` / free-text `prove ... = ...`.
- Added simplified LHS/RHS lines before truth-row comparison, so proofs no longer jump straight to row matching.
- Added regression tests for covering-law simplification and proof outputs.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Algebra Trace Hardening Slice

Completed:
- Added recursive inner-expression Boolean law simplification, with whole-expression equivalence checking still guarding displayed steps.
- Fixed precedence handling so product rewrites are not attempted across outer sums such as `A+A&B`.
- Added set-based absorption for multi-term sum factors such as `(A+B)(A+B+C)`.
- Added regression tests for nested complement/identity/dominance and multi-term absorption traces.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Floating-Point Exact Mantissa Bits Slice

Completed:
- Added `floatbitsadd(...)` / `mantissabitsadd(...)` / `exactfloatbits(...)`.
- Added free-text routing for AQA-style questions asking how many extra mantissa bits are needed to represent a decimal exactly.
- Added regression tests for the 2024-style `12.765625` with 8-bit mantissa and 4-bit exponent case.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 floating-point calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Stats Labelled Free-Text Slice

Completed:
- Added labelled free-text routing for binomial probability/tails, binomial summaries, normal approximation, Poisson approximation, binomial critical regions, and binomial hypothesis tests.
- Added labelled free-text routing for normal distribution intervals, normal tails, z-scores, inverse normal, and normal mean hypothesis tests.
- Added regression tests for reordered `n=`, `p=`, `x=`, `alpha=`, `lower=`, `upper=`, `mean=`, `sd=`, `variance=`, `xbar=`, and `sigma=` prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Labelled Storage Free-Text Slice

Completed:
- Hardened CSCALC labelled-number parsing so labels may contain spaces, hyphens, or underscores.
- Added labelled free-text routing for sound/audio storage, transfer/download time, compression ratio, dictionary compression, RLE size, database record size, character storage, and character-set storage.
- Added regression tests for reordered labelled Paper 2 storage/data-rate prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation/free-text support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Projectile Labelled Free-Text Slice

Completed:
- Improved projectile free-text routing so labelled values are preferred over raw number order.
- Added support for `speed=`, `u=`, `angle=`, `theta=`, `distance=`, `x=`, `y=`, `height=`, `initialheight=`, and `g=` across projectile range, launch-height, point-height, and angle-to-target routes.
- Added regression tests for reordered labelled projectile prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 SUVAT Free-Text Label Slice

Completed:
- Improved labelled-number parsing so labels may contain spaces, hyphens, or underscores.
- Added SUVAT free-text synonyms for `initial velocity`, `initial speed`, `final velocity`, `final speed`, `acceleration`, `displacement`, `distance`, and `time`.
- Added a known-value count guard so unrelated prompts containing `distance=` do not get swallowed by SUVAT routing.
- Added regression tests for common exam-style labelled SUVAT prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 SUVAT Rearrangement Slice

Completed:
- Hardened `suvat(...)` to cover the standard constant-acceleration rearrangements rather than only a few target variables.
- Fixed the previous `a=0` truthiness weakness in the time branch.
- Added branches for missing `u`, `v`, `a`, `s`, and `t`, including quadratic time solving and cases where `s,a,t` or `u,a,s` imply more than one useful unknown.
- Added regression tests for the new rearrangements and zero-acceleration time case.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Truth Output Column Slice

Completed:
- Added `truthbits(...)`, `truthout(...)`, and `outputbits(...)`.
- Output now turns a truth-table output column into 1-cells, 0-cells, canonical SOP, simplified SOP, canonical POS, and simplified POS.
- Added free-text routing for prompts such as `truth table output bits 0110`.
- Added regression tests for direct and natural-language output-column inputs.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC POS From Expression Slice

Completed:
- Added `posform(...)`, `cnf(...)`, and `productofsums(...)`.
- Output now builds a truth table from an arbitrary Boolean expression, lists zero rows, writes maxterms, gives canonical POS, and gives simplified POS.
- Added free-text routing for prompts such as `product of sums form for A or B` and `CNF A and not B`.
- Added regression tests for direct and natural-language POS expression inputs.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Normal Hypothesis P-Value Slice

Completed:
- Improved `hypnormal(...)`, `normaltest(...)`, and `hypmean(...)`.
- Output now computes the z statistic, p-value, rejection rule, and reject/not-reject conclusion.
- Added two-tailed/two-sided free-text routing for normal hypothesis tests.
- Added regression tests for upper-tail and two-tailed prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Correlation Test Slice

Completed:
- Added `corrtest(...)`, `correlationtest(...)`, and `pmcctest(...)`.
- Added free-text routing for PMCC/Spearman correlation hypothesis-test prompts with critical values.
- Output shows hypotheses, correct one-/two-tailed comparison, reject/not reject decision, and context conclusion.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Dictionary Compression Slice

Completed:
- Added `dictcompress(...)`, `dictionary(...)`, and `lzdict(...)`.
- Added free-text routing for dictionary/LZ-style compression prompts.
- Output shows dictionary bits, reference-token bits, compressed size, compression ratio, and percentage saving.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Projectile Target Angle Slice

Completed:
- Added a compact arctan helper for angle output without adding a library dependency.
- Added `projectileangle(...)`, `projangle(...)`, and `targetangle(...)`.
- Added free-text routing for prompts asking for projectile launch angle through a target point.
- Output uses the trajectory equation, substitutes `T=tan(theta)`, forms the quadratic, then gives both possible launch angles.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Projectile At Distance Slice

Completed:
- Added `projectileat(...)`, `projectilepoint(...)`, and `projat(...)`.
- Added free-text routing for projectile prompts asking for height at a horizontal distance.
- Output shows horizontal and vertical resolving, time from horizontal motion, vertical substitution, and final height.
- Preserved existing `projectileh(...)` launch-height-to-range behaviour.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Normal Conditional Slice

Completed:
- Added `normalcond(...)`, `normalgiven(...)`, and `normalconditional(...)`.
- Added free-text routing for normal conditional prompts such as `more than ... given more than ... mean ... sd ...`.
- Output shows `P(A|B)=P(A and B)/P(B)`, identifies the combined event, standardises denominator and numerator bounds, gives fx-CG50 NormalCD guidance, and computes the conditional probability.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Normal Parameter Slice

Completed:
- Added a lightweight normal CDF and inverse-normal helper inside `p3_engine.cpp`.
- Added `normalparams(...)`, `normalparameters(...)`, and `normalmeansd(...)`.
- Added free-text routing for normal prompts that ask to find mean/standard deviation from two percentile equations.
- Output shows the two z-equations, substitution into `(x-mu)/sigma`, solving for sigma, then solving for mu.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Truth Table Rows Slice

Completed:
- Added row-by-row truth-table output for `truth(...)`, `truthtable(...)`, and `truthrows(...)`.
- Added free-text routing for prompts like `truth table for A and not B`.
- Kept ordinary `bool(...)` compact, while truth mode shows variable headers, all rows up to 4 variables, minterms, and simplified expression.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra/free-text support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC K-map Don't-Care Slice

Completed:
- Added `kmapdc(...)`, `mintermsdc(...)`, `dcminterms(...)`, and `dontcare(...)` style parsing.
- Added free-text parsing for K-map/minterm prompts with `dc`, `x`, `dont care`, and `don't care` rows.
- Added shared minimisation using real 1-cells plus don't-care rows for grouping, while only requiring real 1-cells to be covered.
- Added POS/maxterm don't-care support for zero-row grouping.
- Output now shows 1-cell terms, don't-care rows, method guidance, and the simplified expression/POS result.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra/free-text support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Maxterm POS Slice

Completed:
- Added `maxterms(...)`, `pos(...)`, and `zeros(...)`.
- Added free-text routing for maxterm / zero-row prompts.
- Reused the existing minterm parser and Boolean simplifier so POS inputs get the same truth-table-backed simplification route.
- Output shows variables, maxterm factors, POS expression, minterm rows for the equivalent function, and simplified expression.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra/free-text support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Sign-Magnitude Slice

Completed:
- Added `signmag(...)`, `signmagnitude(...)`, and `sm(...)`.
- Added `signmagdec(...)`, `signmagnitudedec(...)`, and `smdec(...)`.
- Added `signmagrange(...)`, `signmagnituderange(...)`, and `smrange(...)`.
- Added free-text routing for sign-and-magnitude range, decode, and encode prompts before generic binary decode.
- Output shows sign bit, magnitude bits, range, and the duplicate zero representation.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 binary representation working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Outlier Fence Slice

Completed:
- Added `outliers(...)`, `iqrfences(...)`, and `outlierfences(...)`.
- Added free-text routing for outlier/IQR/fence prompts with labelled `q1` and `q3`.
- Output shows IQR, lower fence, upper fence, and classifies supplied values.
- Added direct and labelled free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Work-Energy Force Slice

Completed:
- Added `workenergyforce(...)`, `energyforce(...)`, and `driveforce(...)`.
- Added free-text routing for work-energy / driving-force prompts before SUVAT fallback.
- Output shows the work-energy equation, gain in KE, gain in GPE, work against resistance, substitution into `F*d`, and final driving force.
- Added direct and labelled free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Arithmetic Shift Slice

Completed:
- Added `arithshift(...)`, `arithmeticshift(...)`, and `signedshift(...)`.
- Added free-text routing for arithmetic/signed shift prompts before the existing logical shift fallback.
- Output decodes the input and shifted result as two's complement, preserves the sign bit on right shifts, and flags left-shift sign-change overflow risk.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 binary arithmetic/representation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Repetition Code Majority Vote Slice

Completed:
- Added `repeatenc(...)`, `repetitionenc(...)`, and `repeatencode(...)`.
- Added `repeatdec(...)`, `repetitiondec(...)`, and `majority(...)`.
- Added free-text routing for repetition-code encode/decode and majority-vote prompts.
- Output shows grouping, per-group one/zero counts, corrected transmitted bits, decoded data bits, and correction count.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 error-detection/correction calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Floating-Point Multiply Divide Slice

Completed:
- Added `floatmul(...)`, `fpmul(...)`, and `floatingmul(...)`.
- Added `floatdiv(...)`, `fpdiv(...)`, and `floatingdiv(...)`.
- Added free-text routing for floating-point multiply/divide prompts with two mantissa/exponent pairs.
- Output decodes both operands, multiplies or divides mantissas, adds or subtracts exponents, normalises, and re-encodes with the original bit widths.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 floating-point working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Floating-Point Arithmetic Slice

Completed:
- Added `floatadd(...)`, `fpadd(...)`, and `floatingadd(...)`.
- Added `floatsub(...)`, `fpsub(...)`, and `floatingsub(...)`.
- Added free-text routing for floating-point add/subtract prompts with two mantissa/exponent pairs.
- Output decodes both numbers, aligns to a common exponent, combines mantissas, normalises, and re-encodes with the original bit widths.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 floating-point working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Ladder Equilibrium Slice

Completed:
- Added `ladder(...)`, `ladderwall(...)`, and `ladderrough(...)`.
- Added free-text routing for ladder/wall/floor/rough/smooth prompts.
- Output shows a force model, vertical equilibrium, horizontal equilibrium, moments about the foot, wall reaction, friction, and limiting friction coefficient.
- Supports optional point load/person/load at a distance along the ladder.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics/free-text support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC K-map Minterm Slice

Completed:
- Added `minterms(...)`, `kmap(...)`, and `karnaugh(...)`.
- Added free-text routing for K-map/minterm prompts such as `simplify minterms 1 2 for A B`.
- The route converts minterms into canonical SOP terms, then reuses the existing Boolean simplifier instead of adding a second simplification engine.
- Output shows variables, minterm terms, SOP, minterm rows, and simplified expression.
- Added direct, inferred-variable, and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra/free-text support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Rough Incline Acceleration Slice

Completed:
- Added `inclineacc(...)`, `roughplaneacc(...)`, and `planeacc(...)`.
- Added free-text routing for rough plane / incline acceleration prompts.
- Output shows resolving parallel/perpendicular components, reaction, friction, net force, and acceleration using `F=ma`.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Two's Complement Subtract Slice

Completed:
- Improved `twossub(...)`, `tcsub(...)`, and `twossubtract(...)` working.
- Output now shows the decoded values, the two's-complement form of the second value, fixed-width addition, and final denary result.
- Added free-text routing for two's-complement add/subtract prompts.
- Handled "subtract A from B" by reversing operands.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 binary arithmetic working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Beam Reactions Slice

Completed:
- Added `beam(...)`, `beamreactions(...)`, and `supportreactions(...)`.
- Added free-text routing for beam/support/reaction prompts with load, distance, and optional beam weight.
- Output shows moments about the left support, right reaction, vertical equilibrium, and left reaction.
- Added direct and labelled free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Distribution Mean Variance Slice

Completed:
- Added `binomstats(...)`, `binommeanvar(...)`, and `binomialstats(...)`.
- Added `poissonstats(...)`, `poissonmeanvar(...)`, and `postats(...)`.
- Added free-text routing for binomial/Poisson mean, variance, and standard-deviation prompts.
- Output shows the exam formula, substitution, variance, and standard deviation.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Vector Kinematics Slice

Completed:
- Added `vectorkin(...)`, `vectormotion(...)`, and `vectorsuvat(...)`.
- Added free-text routing for vector position/velocity/acceleration motion prompts.
- Output shows vector SUVAT formula, substitution, final position, final velocity, and speed.
- Added direct, labelled free-text, and plain-number free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC SQL Query Trace Slice

Completed:
- Added `sqlselect(...)`, `selectwhere(...)`, and `sqlquery(...)` for SELECT/FROM/WHERE working.
- Added `sqlcount(...)`, `countwhere(...)`, and `countrecords(...)` for COUNT(*) query working.
- Added free-text routing for SQL select/count/where prompts, including "greater than" and "at least" wording.
- Output shows clause order and the final query.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 database/query working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Equilibrium Components Slice

Completed:
- Added `equilibrium(...)`, `forcebalance(...)`, and `balanceforces(...)` for forces supplied as component pairs.
- Added `equilpolar(...)`, `forcepolar(...)`, and `balancepolar(...)` for forces supplied as magnitude/angle pairs.
- Added free-text routing for equilibrium/balance prompts with component or angle wording.
- Added degree-angle snapping for common force resolution angles so 0, 90, 180, and 270 degrees do not suffer from polynomial trig approximation drift.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Variable Acceleration Slice

Completed:
- Added `varacct(...)` / `variableacct(...)` for acceleration as a quadratic function of time.
- Added `varaccx(...)` / `variableaccx(...)` for acceleration as a quadratic function of displacement using `a = v dv/dx`.
- Added label-based free-text routing for variable acceleration coefficient prompts.
- Output shows the selected method, integration setup, substitution, velocity, and displacement where relevant.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Explicit Tree Traversal Slice

Completed:
- Added explicit binary-tree traversal commands using `root,node,left,right` triples.
- Supports `preordertree(...)`, `inordertree(...)`, and `postordertree(...)`.
- Free-text tree prompts with root/left/right/triple wording now route to explicit child-link traversal instead of assuming complete level order.
- Kept existing complete-tree level-order traversal unchanged.
- Added regression tests for direct and free-text diagram-style inputs.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm/trace working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC FSM Trace Slice

Completed:
- Added direct finite state machine tracing with `fsm(start,input,state,symbol,next,...)`.
- Added Mealy-style output tracing with `fsmout(start,input,state,symbol,next,output,...)`.
- Added free-text routing for finite-state-machine prompts.
- Output shows start state, each consumed input symbol, each transition, final state, and output stream where supplied.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 trace-working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Consensus Slice

Completed:
- Located the archived Python Boolean algebra implementation in `python/src/ComputerScience/booleanProgram.py` from commit `d9a86d0f173a39411a40dd4f150ee874aa251cd1`.
- Compared its documented law coverage with current CSCALC Boolean support.
- Added the missing consensus theorem route for expressions such as `AB + A'C + BC`.
- Kept the existing truth-table equivalence guard before displaying the law step.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Grouped Data Slice

Completed:
- Added `groupmean(...)`, `groupedmean(...)`, and `groupstats(...)`.
- Supports grouped mean and standard deviation from midpoint/frequency pairs.
- Added free-text dispatch for grouped-data prompts with midpoints and frequencies.
- Output shows `sum f`, `sum fx`, mean, `sum fx^2`, and standard deviation formula.
- Added a Grouped data help page in CASP3.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Sort Trace Slice

Completed:
- Added `selectionsort(...)` / `selection(...)` working traces.
- Added `mergesort(...)` / `merge(...)` working traces.
- Added free-text dispatch for selection-sort and merge-sort prompts.
- Output shows the sorting rule and intermediate list states.
- Added sort commands to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Dijkstra Slice

Completed:
- Added `dijkstra(...)`, `shortestpath(...)`, and `shortpath(...)` working traces.
- Supports `dijkstra(start,end,from,to,weight,...)` with compact undirected edge triples.
- Added free-text dispatch for shortest-path route prompts.
- Output shows the Dijkstra rule, fixed nodes, distance updates, final path, and final distance.
- Added Dijkstra line to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Tree Traversal Slice

Completed:
- Added `preorder(...)`, `inorder(...)`, and `postorder(...)` working traces for complete binary trees supplied in level order.
- Added aliases `treepre(...)`, `treein(...)`, `treepost(...)`, `pretraverse(...)`, `intraverse(...)`, and `posttraverse(...)`.
- Added free-text dispatch for tree traversal prompts.
- Output states the level-order assumption, traversal rule, and final visit order.
- Added tree traversal lines to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 data structure traversal support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Spearman Slice

Completed:
- Added `spearman(...)` / `spearmanrank(...)` / `rankcorr(...)` working.
- Added free-text/labeled dispatch for Spearman rank correlation using `n` and `sumd2`.
- Output shows the formula, substitution, and final `r_s`.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 statistics support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Stack Queue Trace Slice

Completed:
- Added `stack(...)` / `stacktrace(...)` / `pushpop(...)` working traces.
- Added `queue(...)` / `queuetrace(...)` / `enqueue(...)` working traces.
- Added free-text dispatch for stack push/pop and queue enqueue/dequeue prompts.
- Output shows LIFO/FIFO rule, each operation, current state, and underflow/overflow guards.
- Added stack/queue commands to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed after replacing calculator-incompatible `snprintf` with `sprintf`.

Drift check:
- Still inside CSCALC AQA Paper 2 data structure trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Search Sort Trace Slice

Completed:
- Added `binarysearch(...)`, `linearsearch(...)`, `bubblesort(...)`, and `insertionsort(...)` working traces.
- Added free-text dispatch for binary/linear search and bubble/insertion sort prompts.
- Output shows comparisons, found position, sort passes, or insertion steps.
- Added an Algorithms help page in the CSCALC menu.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC ASCII Unicode Slice

Completed:
- Added ASCII/Unicode code conversion working for `ascii(...)`, `charcode(...)`, `unicode(...)`, and free-text prompts.
- Numeric code inputs show character/code point, denary, binary, and hex.
- Free-text prompts such as `ASCII code for A` preserve uppercase case.
- Added character-storage menu help for ASCII code conversion.
- Added regression tests for ASCII numeric, ASCII free-text, and Unicode code point conversion.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 character/data representation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Hash Linear Probe Slice

Completed:
- Added `hashlinear(...)` / `linearprobe(...)` / `hashprobe(...)` for hash table insertion with linear probing.
- Output shows modulo address, occupied slots/probe count, and final placement.
- Added free-text dispatch for prompts mentioning hash table linear probing.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 hash table calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC RPN Slice

Completed:
- Added `rpn(...)` / `postfix(...)` / `reversepolish(...)` working for Reverse Polish notation.
- Output shows stack pushes, each operation, and final answer.
- Added free-text parsing for prompts such as `evaluate RPN 3 4 + 5 *` and word operators such as `divide` / `plus`.
- Added regression tests for direct and free-text RPN inputs.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Huffman Slice

Completed:
- Added `huffman(...)` / `huff(...)` / `huffmancode(...)` working for frequency tables.
- Output shows lowest-weight merges, code lengths, encoded bit total, and fixed-length comparison.
- Added free-text dispatch for prompts such as `huffman frequencies 45 13 12 16 9 5`.
- Added regression tests for direct command and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 compression calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Boolean Parser Slice

Completed:
- Fixed Boolean expression evaluation so `AND` parsing always consumes the RHS factor.
- This fixes direct SOP forms such as `A&B'+A'&B` and implicit forms such as `A'B+AB'`.
- Added regression coverage for both explicit and implicit XOR/SOP expressions.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Boolean Law Guard Slice

Completed:
- Compared the old Boolean helper with current `CSCALC` support.
- Added a truth-table equivalence guard before Boolean algebra law steps are displayed.
- Added bounded repeated Boolean-law trace output instead of a single unchecked line.
- Fixed a class of bad output where De Morgan's law could be shown for only part of a larger expression.
- Added regression coverage for `bool(not(A and B)+A)` so non-equivalent law fragments are rejected.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Impact Solve Slice

Completed:
- Added `impactsolve(...)` / `collisionsolve(...)` / `restitutionsolve(...)` for final velocities from masses, initial velocities, and coefficient of restitution.
- Output shows conservation of momentum, restitution equation, substitution, and final velocities.
- Added labelled free-text dispatch for `m1=... u1=... m2=... u2=... e=...`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Address Space Slice

Completed:
- Added `addressspace(...)` / `addresses(...)` / `addressbus(...)` working for addressable-location counts.
- Added `memorycapacity(...)` / `addresscapacity(...)` / `memorybus(...)` working for address bus plus word-size capacity.
- Added free-text dispatch for address bus/location/memory capacity prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Normal Variance Slice

Completed:
- Added `normalvar(...)` / `normalzvar(...)` / `zscorevar(...)`.
- Added interval, tail, and inverse-normal variance variants.
- Added free-text dispatch so prompts with `variance` convert to `sigma=sqrt(variance)` before using NormalCD/InvNorm working.
- Added regression tests covering standardising, interval, tail, and inverse-normal prompts with variance.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Projectile Height Slice

Completed:
- Added `projectileh(...)` / `projectileheight(...)` / `projheight(...)` for projectiles launched from a height.
- Added free-text dispatch before the same-height projectile fallback when prompts mention height/above.
- Output now shows resolving, vertical equation, time from the quadratic, and horizontal range.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Conservation Of Momentum Slice

Completed:
- Added `momentum(...)` / `momcons(...)` / `consmomentum(...)` working for two-particle conservation of linear momentum.
- Added `commonvelocity(...)` / `coalesce(...)` / `stick(...)` working for particles moving together after impact.
- Added labelled free-text dispatch for `m1=... u1=... m2=... u2=... v1=...`.
- Fixed `momentum` being misrouted as `moment`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Hash Table Slice

Completed:
- Added `hashmod(...)` / `hashtable(...)` / `modhash(...)` working for hash address calculation.
- Added collision notes when two keys map to the same table address.
- Added free-text dispatch for hash-table prompts such as `hash table size 10 keys 27 18 29 37`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 SUVAT Free-Text Bridge Slice

Completed:
- Broadened the labelled SUVAT parser so constant-acceleration/velocity/distance/time wording works without the literal word `suvat`.
- Added tests for natural labelled constant-acceleration prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics free-text support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Bitwise Operation Slice

Completed:
- Added `xorbits(...)`, `andbits(...)`, `orbits(...)`, `notbits(...)` / `invertbits(...)`.
- Added free-text dispatch for bitwise XOR/AND/OR/NOT while avoiding broad `and` matches in ordinary wording.
- Added tests for direct commands and natural XOR/NOT prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Setup Formula Slice

Completed:
- Added `resolve(...)` / `componentsfromforce(...)` / `forcecomponents(...)` for force components.
- Added `stratified(...)` / `stratifiedsample(...)` / `stratum(...)` for stratified sample-size working.
- Added free-text dispatch for force-component and stratified-sampling prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 setup/method support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Error Detection Slice

Completed:
- Added `hamming(...)` / `hammingdistance(...)` / `bitdiff(...)` working.
- Added `checksum(...)` / `checksummod(...)` / `binarychecksum(...)` working.
- Added free-text dispatch for Hamming distance and checksum prompts before generic binary addition.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Discrete Random Variable Slice

Completed:
- Added `discrete(...)` / `expectation(...)` / `randomvar(...)` working from `(x,p)` pairs.
- Added free-text parsing for value/probability rows, e.g. `values 0 1 2 probabilities 0.2 0.5 0.3`.
- Output now shows probability sum, `E(X)`, `E(X^2)`, and `Var(X)` lines.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 stats support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Bayes / Independence Slice

Completed:
- Added `bayes(...)` / `bayestheorem(...)` / `reverseconditional(...)` working.
- Added `independent(...)` / `independence(...)` / `testindependent(...)` working.
- Ordered free-text dispatch so reverse-conditional/Bayes prompts are not stolen by generic conditional probability.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 probability free-text/method support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 Labelled Stats / Boolean Proof Slice

Completed:
- Added free-text Boolean identity proof parsing, e.g. `prove A+B = B+A`.
- Added labelled PMCC parsing for raw sums: `n=`, `sx=`, `sy=`, `sxy=`, `sx2=`, `sy2=`.
- Added PMCC summary-value route using `Sxy/sqrt(Sxx*Syy)`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `./compile`: passed.

Drift check:
- Still serves CASP3/CSCALC arbitrary free-text goal.
- Stayed inside shared engine/test/build files.
- No UI/menu/status changes.

Next:
- Add labelled parsing for more Paper 3 and CS past-paper phrasings.
- Add more Boolean law working, not only truth-table proof.

## 2026-06-11 Distribution Tail Slice

Completed:
- Added generic binomial tail working for `at least`, `more than`, `less than`, and `at most`.
- Added Poisson cumulative/tail working for matching free-text phrasings.
- Updated CASP3 supported-command text and tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 stats free-text hardening.
- No CAS pure/menu/UI change.

## 2026-06-11 Inverse Normal Slice

Completed:
- Added `invnormal(area,mu,sigma)` / `normalinv` / `inversenormal` setup route.
- Added free-text parsing for normal critical-value / percentile phrasing.
- Output gives exam setup and fx-CG50 `InvNorm(area, sigma, mu)` input.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 stats free-text/method support.
- No UI/menu/status changes.

## 2026-06-11 Boolean Law Step Slice

Completed:
- Read the old Python Boolean program from git history.
- Added compact Boolean algebra step output before truth-table/minterm simplification for common laws:
  idempotent, complement, identity, dominance, absorption, double complement, and De Morgan.
- Broadened free-text Boolean dispatch so symbolic `+` / `*` expressions route correctly.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC Boolean algebra/free-text goal.
- Did not touch UI/menu/status code.

## 2026-06-11 Floating Point Normalisation Slice

Completed:
- Added CSCALC `floatnorm` / `fpnorm` / `normalise` / `normalize` route for AQA mantissa/exponent normalisation.
- Added `floatprecision` / `fpprecision` / `floatstep` route for representable step-size working.
- Added natural-language triggers for normalisation, decimal-to-floating encode, and precision questions.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation scope.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Normal Tail/Test Slice

Completed:
- Added CASP3 `normaltail` / `normalupper` / `normaltp` route with z-standardisation and fx-CG50 NormalCD setup.
- Added `hypnormal` / `normaltest` / `hypmean` route with H0/H1, standard error, z statistic, alpha comparison, and context conclusion prompt.
- Added natural-language parsing for normal upper/lower tail and normal hypothesis-test wording.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 method support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Regression Summary Slice

Completed:
- Added CASP3 least-squares regression route from summary values:
  `regresscalc(n,Sx,Sy,Sxx,Sxy[,x])`.
- Added route from means and corrected sums:
  `regresss(xbar,ybar,Sxx,Sxy[,x])`.
- Added free-text labelled parsing for regression summaries and predictions.
- Hardened label parsing in CASP3 and CSCALC so short labels do not match inside longer labels, e.g. `x` inside `sx`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text arbitrary-input hardening.
- Did not touch CAS Pure, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC RLE Sequence Slice

Completed:
- Added `rletext(sequence,symbolBits,countBits)` / `rlestring` / `runencode`.
- Added run summary output from raw sequence, original bits, and encoded bits.
- Added free-text extraction for run-length encoding questions containing a repeated-symbol sequence.
- Preserved existing numeric `rle(runs,symbolBits,countBits)` route.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC Character Set Slice

Completed:
- Added `charset(characters,characterSetSize)` / `charsetsize` / `textsymbols`.
- Added working for bits per character from `ceil(log2(character set size))`.
- Added free-text parsing for character-set/alphabet/symbol storage questions.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 storage-calculation support.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Mechanics Free-Text Slice

Completed:
- Added free-text dispatch for impulse/momentum-change questions.
- Added free-text dispatch for variable-acceleration integration questions.
- Hardened connected-particles parsing when the prompt contains spaced words.
- Prevented generic force parsing from stealing connected-particle prompts.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC Closest Float Slice

Completed:
- Added `floatnearest(value,mantissaBits,exponentBits)` / `fpnearest` / `closestfloat`.
- Added working for normalising the value, finding the step at that exponent, rounding to nearest representable multiple, and emitting mantissa/exponent bits.
- Added free-text parsing for closest/nearest representable floating-point questions.
- Ordered free-text dispatch so `representable` does not get stolen by floating-point encode.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 floating-point calculation support.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Binomial Normal Approx Slice

Completed:
- Added `binomnorm(n,p,lower,upper)` / `normalapproxbinom` / `binomnormal`.
- Added mean, standard deviation, continuity-correction, z-value, and NormalCD setup working.
- Added free-text parsing for normal approximation to binomial prompts.
- Ordered dispatch so normal-between prompts do not steal binomial-normal approximation prompts.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC Boolean Distributive Slice

Completed:
- Added named Boolean algebra working for common-factor/distributive simplification, e.g. `AB+AC -> A(B+C)`.
- Preserved the existing truth-table/minterm simplification fallback.
- Added a golden test for spaced free-text style Boolean input.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 Distribution Approx / Fixed Point Encode Slice

Completed:
- Added CASP3 Poisson approximation to binomial route:
  `poissonapprox(n,p,r,tail)` / free text such as `poisson approximation to binomial n 200 p 0.01 at most 3`.
- Added CSCALC fixed-point encoding routes:
  `fixedenc(value,wholeBits,fractionBits)` and signed `fixedtcenc(value,wholeBits,fractionBits)`.
- Added free-text fixed-point encode parsing before fixed-point decode so decimal prompts do not get stolen by binary decode.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 78048 bytes`
  - `CSCALC.g3a: 80348 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text arbitrary-input hardening.
- Did not touch CAS Pure behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Transfer Unit Slice

Completed:
- Added explicit `transfermb(sizeMB,rateMbitPerSecond)` and `transferkb(sizeKB,rateKbitPerSecond)` routes.
- Added free-text handling for megabyte/megabit and kilobyte/kilobit transfer-time prompts.
- Added tests that catch the common wrong `10/2=5s` route for `10 megabytes at 2 megabits per second`; expected method converts to `80 Mbit`, then `40s`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 78048 bytes`
  - `CSCALC.g3a: 81264 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Boolean Product-of-Sums Slice

Completed:
- Added Boolean algebra law working for product-of-sums common-term simplification:
  `(A+B)(A+C) -> A+BC`.
- Kept truth-table/minterm simplification as the fallback after the algebra line.
- Added a golden test for `bool((A+B)(A+C))`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 78048 bytes`
  - `CSCALC.g3a: 81556 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-12 P3/CS Free-Text Hardening Slice

Completed:
- Added generic number-before-keyword parsing for CASP3 free text.
- Fixed beam/rod reaction prose such as `20N load is 2m from...`.
- Added safer velocity-polynomial parsing for displacement from `v = at^2+bt+c`.
- Fixed ladder limiting-angle prose with smooth wall/rough ground.
- Added coin/toss two-tailed critical-region working before generic hypothesis routing.
- Fixed `B(n,p)` normal/Poisson approximation prose with one bound or exact point.
- Improved coded-data wording to avoid abbreviated/confusing `sd` output.
- Added CSCALC RLE text output when bit widths are not supplied.
- Rebuilt all calculator files.

Evidence:
- Broad free-text probe batch: 23/23 passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 169388 bytes`
  - `CSCALC.g3a: 158720 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future probe-driven hardening; this slice is verified.

## 2026-06-12 P3/CS Prose Coverage Slice

Completed:
- CASP3: strengthened vertical/upward projectile parsing so `upwards`, ground hits, and launch heights from `cliff/high/height` prose are handled before generic projectile routes.
- CASP3: added two-tailed binomial critical-region working for generic `binomial distribution` prose and moved critical-region handling before accidental point-probability inference.
- CASP3: fixed normal percentile parsing so `90th percentile` becomes area `0.9`, not a misplaced mean/probability label.
- CASP3: allowed grouped upper/lower quartile prompts without an explicit numeric `q`, deriving `0.75`/`0.25`.
- CSCALC: routed floating-point mantissa/exponent decode/normalise prompts before generic two's-complement bit decoding.
- CSCALC: added denary/decimal to octal free-text conversion.
- CSCALC: cleaned Boolean free-text proofs/simplifications with leading De Morgan wording and terminal punctuation.
- Rebuilt all calculator files.

Evidence:
- Fresh unseen probe set: 31/31 passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 165100 bytes`
  - `CSCALC.g3a: 156380 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text exam-calculation support.
- Did not change CAS Pure source, NOTES source, menus, or shared UI/status behavior.

## 2026-06-12 P3/CS Free Text Hardening Slice

Completed:
- CASP3: made prose number extraction accept "is/of" label phrasing.
- CASP3: routed vertical projected-motion and final-rest braking prompts before projectile/generic SUVAT fallbacks.
- CASP3: accepted `distributed N(...)` notation, natural histogram density/frequency prompts, product-moment correlation wording, and sample mean hypothesis wording with spaced `standard deviation`.
- CSCALC: accepted base names and `in base` phrasing for base 2/base 8 conversions.
- CSCALC: prioritised explicit binary-to-hex wording over accidental hex word tokens.
- CSCALC: handled worded negative two's-complement encoding.
- CSCALC: fixed large address-space prompts by avoiding scientific notation inside the generated command.
- CSCALC: normalised Boolean word operators in free text and ignored terminal punctuation in Boolean proofs.
- Rebuilt all calculator files.

Evidence:
- Targeted fresh probe set: 25/25 passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 161412 bytes`
  - `CSCALC.g3a: 155680 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text exam-calculation support.
- Did not change CAS Pure source, NOTES source, menus, or shared UI/status behavior.

## 2026-06-12 Free-Text Hardening Slice

Completed:
- Hardened CASP3 notation routing:
  - `X~N(mu,sigma^2)` inverse-normal prompts such as `find k such that P(X<k)=...`.
  - `X~Po(lambda)` critical-region prompts before plain `P(X=r)` routing.
- Hardened CASP3 mechanics free text:
  - vertical projection to a stated height/above point solves the quadratic instead of using maximum-height route.
  - braking/deceleration prompts infer final velocity 0 and show SUVAT working.
  - lift tension now parses mass/acceleration in either order and handles descending/downward acceleration.
- Hardened CSCALC free text:
  - generic `convert(...,from,to)` now prints final output for bases beyond 2/16, including octal.
  - binary-to-octal and octal-to-binary worded prompts route by word order.
  - sound storage/sample prompts parse values attached to `kHz` and `minute/hour/second`.
  - output-column truth-table prompts accept `variables`.
  - Boolean simplification strips trailing method words such as `using De Morgan`.
- Rebuilt calculator files.

Evidence:
- Targeted P3/CS free-text probe suite: passed, 22/22.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 158964 bytes`
  - `CSCALC.g3a: 154748 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC arbitrary free-text calculation support.
- Did not touch CAS Pure source, NOTES source, menus, or shared status/UI code.

## 2026-06-12 CSCALC Base Wording Follow-Up

Completed:
- Added free-text routing for `base 2 to base 8 ...` and `base 8 to base 2 ...` wording.
- Changed `convert(...,*,2)` output to preserve source grouping width for hex/octal without adding unrelated 16-bit padding.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- Base adjacency probes for base-2/base-8/octal/hex-to-binary: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 158964 bytes`
  - `CSCALC.g3a: 155020 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 free-text conversion support.
- Did not touch CAS Pure, CASP3 source, NOTES source, menus, or shared status/UI code.

## 2026-06-11 CASP3 Coded Statistics Slice

Completed:
- Added coded-statistics transforms:
  `uncode(meanY,sdY,a,b)` for `Y=(X-a)/b`, and `code(meanX,sdX,a,b)`.
- Added free-text route for prompts like `coded data y=(x-20)/5 mean 3 sd 2`.
- Fixed free-text sign handling so `x-20` gives `a=20`, not `a=-20`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 79328 bytes`
  - `CSCALC.g3a: 81556 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Parity Bit Slice

Completed:
- Added `parity(bits,even|odd)` / `paritybit(...)` route.
- Added free-text handling for even/odd parity-bit prompts.
- Output counts one-bits, states parity rule, and gives transmitted bits.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 79328 bytes`
  - `CSCALC.g3a: 81984 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Poisson Normal Approximation Slice

Completed:
- Added `poissonnorm(lambda,lo,hi)` / `normalapproxpoisson(...)` / `poissonnormal(...)`.
- Added free-text route for prompts like `normal approximation to poisson mean 64 between 55 and 70`.
- Output shows model choice, `mu`, `sigma`, continuity correction, both z-values, and the fx-CG50 `NormalCD` input.
- Added `poissonnorm` to the supported-command line.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 80080 bytes`
  - `CSCALC.g3a: 81984 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Poisson Hypothesis Slice

Completed:
- Added shared stable Poisson probability helper using scaled `e^-lambda` and recurrence, replacing the weak short alternating exponential series.
- Added `critpoisson(lambda,alpha,tail)` / `criticalpoisson(...)` / `poissoncrit(...)`.
- Added `hyppoisson(lambda,x,alpha,tail)` / `poissontest(...)` / `poissonhyp(...)`.
- Added free-text routes for Poisson critical-region and hypothesis-test prompts.
- Added regression coverage for large-lambda Poisson probability output.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 81736 bytes`
  - `CSCALC.g3a: 81984 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Check Digit Slice

Completed:
- Added generic weighted modulo check-digit working:
  `checkdigit(digits,modulus,w1,w2,...)` / `modcheck(...)` / `weightedcheck(...)`.
- Added free-text handling for prompts like `check digit for 12345 weights 6 5 4 3 2 mod 11`.
- Output shows weighted-sum method, remainder, and final check digit.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 81736 bytes`
  - `CSCALC.g3a: 82736 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Grouped Interpolation Slice

Completed:
- Added grouped-data interpolation working for median:
  `groupmedian(L,cfBefore,f,width,n)`.
- Added grouped quantile interpolation:
  `groupquantile(L,cfBefore,f,width,n,q)`.
- Added free-text handling for grouped median/quartile prompts.
- Output shows position, interpolation formula, substitution, and final value.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 83688 bytes`
  - `CSCALC.g3a: 82736 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Histogram Density Slice

Completed:
- Added histogram frequency-density working:
  `histdensity(frequency,width)` / `frequencydensity(...)`.
- Added reverse route:
  `histfreq(density,width)` / `frequencyfromdensity(...)`.
- Added free-text handling for simple histogram density prompts.
- Output shows formula, substitution, and value.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 84512 bytes`
  - `CSCALC.g3a: 82736 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC XOR Algebra Slice

Completed:
- Added Boolean algebra working line for XOR:
  `A xor B -> A'B + AB'`.
- Kept existing truth-table simplification as the final confirmation.
- Added tests for text `xor` and symbolic `^`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 84512 bytes`
  - `CSCALC.g3a: 82852 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-12 Distributional Approximation and Boolean Proof Slice

Completed:
- Added CASP3 exact interval working for `distribution B(n,p)` prompts such as `P(3<X<=8)`.
- Added generic CASP3 distributional-approximation prose handling for percent/probability wording, including automatic Poisson vs normal approximation choice and contextual-number filtering.
- Added CASP3 sample-mean normal probability working with standard error lines.
- Fixed CSCALC explicit bit-width ASCII storage prompts.
- Fixed CSCALC Boolean proof extraction after leading prose and normalised `.`/`*` as AND for Boolean law traces.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch: 9/9 passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 173340 bytes`
  - `CSCALC.g3a: 158912 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Coded Data and Probability Parser Slice

Completed:
- Fixed CASP3 coded-data prose for transforms like `y=(x-12)/3` so constants, mean, and standard deviation are read from the transform and final stated stats rather than incidental sample-size numbers.
- Fixed CASP3 numeric parser so generated commands containing scientific notation are not misread as large integers.
- Fixed CASP3 distributional approximation parsing so explicit `p 0.003` and `p=0.003` stay as probabilities, not percentages.
- Fixed CASP3 automatic Poisson approximation so explicit `normal approximation` continues to use the normal route.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch found coded-data and small-probability approximation misses; fixed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 193352 bytes`
  - `CSCALC.g3a: 166896 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Grouped Classes, Normal Intervals, Units, Records Slice

Completed:
- Added CASP3 grouped class-range parsing so `0-10 10-20 ... frequencies ...` becomes midpoint/frequency working instead of negative-number noise.
- Narrowed CASP3 generic summary-statistics fallback so normal distribution prompts with mean/variance stay in the normal engine.
- Added CASP3 limiting-equilibrium inclined-plane coefficient route using `mu R = mg sin(theta)` and `mu = tan(theta)`.
- Fixed CSCALC K-map/minterm prose parsing so words like `draw`, `and`, and `or` are not mistaken for variables.
- Fixed CSCALC megapixel image prompts with `bytes per pixel`.
- Fixed CSCALC sound prompts with explicit `2 channels`.
- Fixed CSCALC `transmission time` wording and MiB/Mbit unit route.
- Fixed CSCALC two's-complement addition from signed denary operands with a stated bit width.
- Added CSCALC record-field file-size working where per-record size is the sum of field byte sizes.
- Added regression tests and rebuilt calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 192900 bytes`
  - `CSCALC.g3a: 166052 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Coded Data, Variable Acceleration, Boolean Prose, Memory Slice

Completed:
- Moved CASP3 coded-data free-text handling before generic conditional probability so `Given coded data...` routes to coding working.
- Broadened CASP3 variable-acceleration parsing to infer initial velocity `0` from `starts from rest`.
- Fixed CSCALC maxterm/K-map prose parsing by ignoring `use`, `using`, `to`, and `the`.
- Fixed CSCALC NOR/NAND-form extraction by ignoring leading `find` / `the` wording.
- Added CSCALC ASCII string storage counting for prompts such as `string HELLO`.
- Added CSCALC character-set storage precedence so `300 characters ... set of 1000 symbols` computes total storage, not only bits per symbol.
- Added CSCALC memory-capacity working for address-width plus bytes-per-address wording.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probe batch passed for coded data, rest-start variable acceleration, maxterm K-map, NOR-only form, ASCII string storage, address capacity, and character-set storage.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 193380 bytes`
  - `CSCALC.g3a: 166896 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Stats, Vectors, Colour Depth, Truth Extraction Slice

Completed:
- Added grouped-data variance line before standard deviation.
- Added combined conditional-probability and independence working for prompts asking both.
- Fixed binomial percent probability parsing for `2 percent` style prompts and inferred `at least one`.
- Fixed binomial critical-region parsing when significance is written as `1 percent`.
- Added natural vector-component force magnitude routing for `7i-24j` style wording.
- Added CSCALC colour-depth-only routing when only colours and bits-per-pixel are requested.
- Fixed Boolean truth-table free-text extraction for `Draw the truth table for ...` prompts.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch exposed 6 parser/output gaps; all were fixed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 190576 bytes`
  - `CSCALC.g3a: 164848 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256 0e07665584385a98273ed84a0f92a1d26cdde2537f6b857a638ad07ac287f014`
  - `CSCALC.g3a sha256 6d4f0f9ac765a88d00a4d508a72226661f9c89a0296f5f13626d73e2ced36c84`

Drift check:
- Still inside CASP3/CSCALC parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Conditional/Poisson/Beam and CS Unit Parser Slice

Completed:
- Added CASP3 normal conditional interval working for prompts like `45<X<55 given X<60`.
- Added CASP3 one-period Poisson rate wording for prompts like calls arriving at a mean rate per hour.
- Added CASP3 multi-load beam reaction working using repeated load-distance pairs.
- Fixed CSCALC image colour-count extraction when the colour count appears before the resolution.
- Fixed CSCALC compression ratio for mixed `MB before` / `KB after` wording.
- Fixed CSCALC signed fixed-point decode so fixed-point tokens are not intercepted by integer two's-complement decode.
- Added CSCALC natural decimal two's-complement subtraction routing with bit width.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch exposed 7 parser/output gaps; all were fixed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 188676 bytes`
  - `CSCALC.g3a: 164252 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256 b028fd8db2f7d38d814efbd5e9202165753eb52e0d37137759717c4da2571013`
  - `CSCALC.g3a sha256 821686a85757ec5a408805c77f557bcfc9afe2bb3b7fdb9e4c386c096b43000b`

Drift check:
- Still inside CASP3/CSCALC parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Conditional Normal, Histogram, Stratified, Normalise, Megapixel Slice

Completed:
- Fixed CASP3 labelled conditional normal probability so mean/sigma are not mistaken for thresholds.
- Fixed CASP3 `X>... given X>...` symbol wording to choose the upper-tail conditional route.
- Fixed CASP3 stratified sampling prose where sample size, population, and stratum group are given in sentence form.
- Fixed CASP3 histogram prose where class width appears before frequency density.
- Fixed CSCALC normalise-vs-normalised routing so `normalise ...` transforms mantissa/exponent while `normalised ... decode` still decodes.
- Fixed CSCALC megapixel image storage when colour count is supplied, converting colours to colour depth first.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probes:
  - `X is normally distributed with mean 60 and standard deviation 8. Find P(X>70 given X>55).` now uses `X~N(60, 8^2)` and upper-tail conditional probability.
  - `A histogram has class width 5 and frequency density 3.2. Find the frequency.` now gives `3.2*5 = 16`.
  - `A sample of 80 ... population of 500 ... stratum contains 125` now gives `125/500 * 80 = 20`.
  - `Normalise ... mantissa 00101100 exponent 0101` now gives `mantissa = 01011000`, `exponent = 0100`.
  - `2 megapixels and 4096 colours` now gives `ceil(log2(4096)) = 12` and `2000000*12`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 186768 bytes`
  - `CSCALC.g3a: 162384 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 SUVAT, Fixed-Point Width, and Sound Sample Wording Slice

Completed:
- Fixed CASP3 SUVAT prose parsing so `accelerates uniformly at N for T seconds` is read as acceleration/time, not final velocity.
- Added combined final-speed and distance working when both are requested.
- Preserved `from rest to V in T seconds` acceleration/distance route by requiring an `at` marker before treating post-accelerates numbers as acceleration.
- Fixed CSCALC fixed-point wording where `8 bits with 4 bits after the point` means 8 total bits, 4 fractional bits.
- Fixed CSCALC negative fixed-point encoding to use signed two's-complement output by default.
- Fixed CSCALC sound storage wording so `16 bit samples` does not route to sample-count output unless the prompt asks for number/how many samples.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probes:
  - `A car starts from rest and accelerates uniformly at 2.4 m s^-2 for 8 seconds. Find the final speed and distance travelled.` now gives `v = 19.2`, `s = 76.8`.
  - `Represent -5.75 in fixed point binary using 8 bits with 4 bits after the binary point.` now gives `write -92 in 8-bit two's complement`, `fixed point = 1010.0100`.
  - `A sound file is sampled at 44.1 kHz with 16 bit samples in stereo for 3 minutes...` now gives `44100*180*16*2 = 254016000 bits`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 184596 bytes`
  - `CSCALC.g3a: 161752 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Variable Acceleration, Normal IQR, Coin Tail, Megapixel Slice

Completed:
- Added CASP3 variable-acceleration prose parsing for `acceleration 4-2t` style inputs.
- Guarded the variable-acceleration route so constant acceleration prompts still use SUVAT.
- Added CASP3 normal-distribution interquartile range working from mean and standard deviation/variance.
- Fixed CASP3 biased coin `fewer than` wording to route to the strict lower binomial tail.
- Added CSCALC megapixel image-storage route so megapixels are treated as pixel count, not colour count.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probes:
  - `A particle moves with acceleration 4-2t... after 3 seconds` now gives `v = 8`, `s = 24`.
  - `normal ... mean 100 ... standard deviation 15 ... interquartile range` now gives quartile lines and `IQR = 20.2346925`.
  - `biased coin ... fewer than 10 heads` now gives `P(X<10)` and `sum from 0 to 9`.
  - `5 megapixel image with 12 bit colour depth` now gives `5000000*12 = 60000000 bits`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 185768 bytes`
  - `CSCALC.g3a: 162188 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Vector Impulse, Horizontal Work-Energy, Poisson Wording Slice

Completed:
- Added CASP3 vector impulse free-text working for `i/j` velocity and impulse prompts.
- Broadened driving-force work-energy routing to horizontal/level cases with `height=0`.
- Generalised Poisson rate-over-exposure routing beyond calls/minutes to defect/metre style wording.
- Fixed plain-English Poisson `fewer than` routing to use cumulative working.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes passed for vector impulse, horizontal driving force, Poisson fewer-than, and defects-per-metre prompts.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 183404 bytes`
  - `CSCALC.g3a: 159520 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, CSCALC source, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Beam, Normal Parameter, Float Width Slice

Completed:
- Fixed CASP3 preceding-number parser so non-numeric prose before keywords cannot become `0`.
- Fixed CASP3 beam reaction wording for `A load of ... placed ... from A`.
- Broadened CASP3 normal-parameter free-text routing for separated `standard deviation` wording.
- Fixed CSCALC `N-bit mantissa` / `N-bit exponent` parsing for nearest-representable floating-point prompts.
- Fixed CSCALC two's-complement encode routing for `convert denary -13 to 8 bit ... binary` and similar bit-width wording without stealing fixed-point prompts.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes passed for uniform beam reactions, normal mean/sd from two probabilities, nearest representable float with bit-width prose, and two's-complement encode with bit-width prose.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 183464 bytes`
  - `CSCALC.g3a: 160960 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Correlation, Hash, Infix Extraction Slice

Completed:
- Fixed CASP3 correlation-test free-text extraction so sample size/significance are ignored and `r`/critical value are selected correctly.
- Fixed CSCALC hash free-text routing so `table size` is used as modulus and excluded from keys.
- Fixed CSCALC infix-to-postfix extraction so leading prose like `the` is not included in the expression.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes passed for PMCC positive-tail test, modulo hash with linear probing, and infix-to-RPN conversion.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 183848 bytes`
  - `CSCALC.g3a: 161116 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Impulse, Work-Energy, Poisson Rate, Storage Wording Slice

Completed:
- Added CASP3 free-text impulse-opposite-direction working.
- Added CASP3 driving-force work-energy routing for resistance/rise/speed-change wording.
- Added CASP3 Poisson call-arrival rate-over-time routing.
- Expanded CASP3 binomial probability wording for seeds/germination and fixed `no more than` tail precedence.
- Fixed CASP3 normal mean hypothesis parsing when population mean appears after `less than`/`greater than`.
- Fixed CSCALC image colour-count routing before colour-depth fallback.
- Fixed CSCALC MB/KB bitrate wording and compression percentage-reduction wording.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch: actual failures fixed; Boolean consensus expectation rejected as invalid for `AB+AC+BC`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 182236 bytes`
  - `CSCALC.g3a: 159520 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Mechanics, Normal Totals, Units, and Boolean Parser Slice

Completed:
- Added CASP3 free-text variable-acceleration integration for symbolic `a = kt + c` style prompts with initial velocity and time.
- Fixed vertical-upwards-from-height wording so speed and launch height are not swapped.
- Added rough inclined-plane equilibrium working for friction and normal reaction when no coefficient is supplied.
- Added proportion/percentage binomial hypothesis-test routing with explicit `H1` wording.
- Added normal total/sum distribution working for totals of independent normal variables.
- Fixed CSCALC arithmetic-shift prose so it routes before two's-complement decode.
- Added MiB output lines for image and sound storage calculations.
- Fixed fixed-point denary-to-binary free-text routing before binary fixed-point decode.
- Improved Boolean free-text parsing for `using Boolean algebra simplify ...` prompts.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch: 10/10 passed after expectation correction.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 179456 bytes`
  - `CSCALC.g3a: 159276 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.
