# Checkpoint

## 2026-06-13 P3 Unseen Prompt Sweep Slice

Completed:
- Probed unseen CASP3 and CSCALC free-text prompts across normal, binomial, Poisson, SUVAT, pulleys, histograms, binary, floating point, storage, sound, and Boolean algebra.
- Fixed CASP3 absolute normal wording `within k of the mean` without breaking `within k standard deviations`.
- Fixed CASP3 normal unknown-mean prompts with compact wording like `P(X less than 40)=0.2`.
- Fixed CASP3 SUVAT requested final speed from `u`, `a`, and `s` so requested unknowns are not treated as given values.
- Fixed CASP3 rough pulley/free-text connected-particle prompts before the generic friction fallback.
- Fixed CASP3 histogram hyphen class intervals like `class 15-25 frequency 40 find density`.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 526464 bytes`
  - `CSCALC.g3a: 290456 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`
- Artifact hashes:
  - `CASP3.g3a: 3f7a3961c87740200a5a6e6ba8a78fc1f681e96e9b5322c9e84c41c0c84bda83`
  - `CSCALC.g3a: a23bb3352e80937397958d2258c206956ebe268dad8e4e4657873d1512ea999e`

Drift check:
- Stayed inside CASP3 parser hardening, tests, checkpoint, and rebuilt app artifacts.
- CSCALC probes passed and no CSCALC source changes were needed in this slice.
- No Pure CAS source, NOTES source, menus, session sharing, or shared UI/status code changed.
- Active goal remains open; production readiness is still not globally proven for arbitrary paper prompts.

## 2026-06-13 Final Parser Gap Sweep Slice

Completed:
- Fixed CASP3 histogram interval wording like `from 12 to 20 ... frequency 36` so width is parsed as `20-12`.
- Fixed CASP3 grouped quartile interpolation when `cumulative frequency before class` is supplied in natural wording.
- Fixed CASP3 loaded uniform-rod centre of mass when particles are at A, midpoint, and B.
- Added CASP3 absolute normal probability handling for wording like `absolute X minus 30 less than 6`.
- Fixed CSCALC decimal whole+fraction conversion such as `13.625 to binary`, without stealing binary-to-denary fixed-point prompts.
- Fixed CSCALC binary addition when the second addend is a single bit.
- Fixed CSCALC Boolean set-notation dont-care parsing for `Σm(...) with dont cares 1 5`.
- Rebuilt calculator-ready CASP3 and CSCALC artifacts.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 522196 bytes`
  - `CSCALC.g3a: 290456 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`
- Artifact hashes:
  - `CASP3.g3a: effd86ef6bf150b4b85d89dd731c892d97f280817ca802509e8dd3d03ed1727a`
  - `CSCALC.g3a: a23bb3352e80937397958d2258c206956ebe268dad8e4e4657873d1512ea999e`

Drift check:
- Stayed inside CASP3/CSCALC parser hardening, tests, checkpoint, and rebuilt app artifacts.
- No Pure CAS source, NOTES source, menus, session sharing, or shared UI/status code changed.
- Active goal remains open for further unseen-case hardening if needed.

## 2026-06-13 P3/CS General Parser Finalisation Slice

Completed:
- Hardened CASP3 chi-squared goodness-of-fit free text so expected ratios/probabilities are scaled to observed totals.
- Added CASP3 generic histogram class-interval frequency handling, grouped quartile labelled interpolation, and loaded uniform-rod centre-of-mass route.
- Added CSCALC generic free-text CRC, growing-width binary addition, binary multiplication, decimal-fraction-to-binary fallback, and robust floating mantissa/exponent width parsing.
- Fixed fixed-binary scanning so decimal values like `0.1875` are not partially read as binary `0.1`.
- Added regressions and rebuilt calculator-ready app files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 519636 bytes`
  - `CSCALC.g3a: 289144 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`
- Artifact hashes:
  - `CASP3.g3a: 6079e07bcee428ffebea3bf4a9724fddd8b97e3befd522ec04af4a8a7d5bd312`
  - `CSCALC.g3a: 8f68423d99a239f62e60da32d452f04e1cf4024c8c9c2f2bbbf788a70ba2be86`

Drift check:
- Stayed inside CASP3/CSCALC parser hardening, tests, checkpoint, and rebuilt artifacts.
- No Pure CAS source, NOTES source, menus, or shared UI/status code changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Boolean Gate And Binomial Parameter Slice

Completed:
- Found old Boolean Python script in git history at `ComputerScience/booleanProgram.py`; checked its law coverage against current CSCALC.
- Fixed NOR/NAND free-text command cleanup so wording like `in NOR only form` does not leak `I`/`N` into the Boolean expression.
- Added generic binomial parameter solving for `P(X=r)=q` with symbolic/unknown `p`, including compact `X~B(n,p)` wording.
- Added regressions for NOR wording and binomial `p` solving at `r=0`, `r=n`, and `find the value of p`.

Evidence:
- Direct probe `Write (A+B) in NOR only form.` gives `NOR form: (A NOR B) NOR (A NOR B)`.
- Direct probe `A binomial variable has P(X=0)=0.1073741824 and n=10. Find p.` gives `p = 0.2`.
- Direct probe `A binomial variable has P(X=10)=0.0009765625 and n=10. Find p.` gives `p = 0.5`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 499512 bytes`
  - `CSCALC.g3a: 275260 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC parser hardening, tests, checkpoint, and rebuilt app artifacts.
- No Pure CAS source, NOTES source, menus, or shared UI/status code changed.
- Active goal remains open.

## 2026-06-13 CASP3 Normal Parameter Routing Slice

Completed:
- Fixed compact normal-parameter prompts like `Given P(X<20)=0.1 and P(X<50)=0.9 for normal X, find mean and sd`.
- Added a stricter parameter-request guard before the generic conditional-normal fallback.
- Confirmed genuine conditional-normal prompts still route to `normalcond`.
- Added regression and rebuilt calculator-ready outputs.

Evidence:
- Direct probe now shows `P(X<=20)=0.1`, `P(X<=50)=0.9`, `mean = 35`, `sd = 11.70455878`.
- Conditional probe `P(X>70 given X>55)` still shows `Use conditional probability P(A|B)=P(A and B)/P(B)`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 496096 bytes`
  - `CSCALC.g3a: 275252 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3 normal free-text routing/tests/checkpoint and rebuilt CASP3 artifact.
- No Pure CAS source, CSCALC logic, NOTES source, menus, or shared UI/status code changed.
- Active goal remains open.

## 2026-06-13 CSCALC Boolean Free-Text Cleanup Slice

Completed:
- Fixed Boolean free-text command cleanup so `Show NAND only form of A+B` parses only `A+B`.
- Fixed `F(A,B,C)=... to minterms/simplify` routing so the function header and trailing instruction words are stripped before Boolean parsing.
- Added regressions for both forms.

Evidence:
- Direct probe gives `minterms: 3,5,7` and `simplified = BC+AC`.
- Direct probe gives `NAND form: ((A NAND A) NAND (B NAND B))`.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 495972 bytes`
  - `CSCALC.g3a: 275252 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC Boolean free-text parsing/tests/checkpoint and rebuilt CSCALC artifact.
- No Pure CAS source, CASP3 logic, NOTES source, menus, or shared UI/status code changed.
- Active goal remains open.

## 2026-06-13 CASP3 Free-Text Stats Hardening Slice

Completed:
- Fixed correlation-test alpha parsing so `1% level` is treated as `alpha = 0.01`.
- Added Spearman rank-correlation test routing for `rs=... with n=...` wording.
- Added regression-line routing for mean/gradient wording, e.g. `mean x`, `mean y`, `gradient`.
- Added normal conditional probability routing for compact `P(X>... | X>...)` syntax.
- Added sample-mean normal probability routing before inverse-normal fallback.
- Broadened interpolation wording to accept `at x=...` and `points`.

Evidence:
- New regressions cover PMCC, Spearman, regression line from means, interpolation, conditional normal, and sample-mean probability.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 495972 bytes`
  - `CSCALC.g3a: 275084 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3 free-text stats parsing, P3 tests, checkpoint, and rebuilt CASP3 artifact.
- No Pure CAS source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 CSCALC Unicode Maxterm Notation Slice

Completed:
- Fixed Boolean set-notation parsing so Unicode product/maxterm forms `ΠM(...)` and `∏M(...)` route like existing `pi M(...)`.
- Added regression for `F(A,B,C)=ΠM(0,2,4,6)`.

Evidence:
- Direct probe now gives `Maxterm/POS method`, maxterm factors, and `simplified = C`.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 489784 bytes`
  - `CSCALC.g3a: 275084 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC Boolean free-text parsing and tests.
- No Pure CAS, CASP3, NOTES, menu/UI/session, or shared status-bar source changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 P3/CS Unseen Routing Cleanup Slice

Completed:
- Fixed CASP3 projectile prompts that ask for both maximum height and range from known speed/angle, so they no longer route to the inverse "find projection speed from max height" method.
- Narrowed CASP3 loaded rod/beam reaction parsing so weighted uniform rods with multiple loads include the rod weight, while no-weight multi-load beams still use the existing load-only route.
- Fixed CSCALC product-of-sums/maxterm free text with headers like `F(A,B,C)=A and not C` so declared variables are preserved even if one variable is absent from the expression.
- Added regressions and rebuilt calculator-ready CASP3/CSCALC artifacts.

Evidence:
- Direct probe `projectile speed 25 angle 35 find maximum height and range` now shows resolved components, maximum height, time of flight, and range.
- Direct probe `rod length 6 weight 20 load 30 at 2m from A load 40 at 5m from A find reactions` now gives `R_B = 53.33333333 N` and `R_A = 36.66666667 N`.
- Direct probe `Find maxterms for F(A,B,C)=A and not C` now uses `variables: A,B,C`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 528312 bytes`
  - `CSCALC.g3a: 290788 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`
- Artifact hashes:
  - `CASP3.g3a: 2977019ba5fb34043c2e2d4e9096bbba62f38d3b160f4bf9e5af6ac0813a62ea`
  - `CSCALC.g3a: edf91ae2802286da28ca3f8a5d232601e3d5c33bb5c00513501f8747f272ab8c`

Drift check:
- Stayed inside CASP3/CSCALC parser hardening, tests, checkpoint, and rebuilt app artifacts.
- No Pure CAS source, NOTES source, menu/UI/session, or shared status-bar code changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Final Free-Text Parser Sweep Slice

Completed:
- Fixed CASP3 absolute-normal bar notation such as `P(|X-30|<6)` using the existing interval/z-score method.
- Fixed CASP3 total-distance prompts with compact bounds like `find total distance 0 to 4` while avoiding accidental capture of `initial velocity` in acceleration questions.
- Fixed CASP3 repeated histogram class wording so `class ... frequency ... class ... frequency ...` gives every density.
- Fixed CASP3 grouped median interpolation with labelled `lower`, `upper`, `CF before`, `frequency`, and `total` values.
- Cleaned PMCC test display for negative `r` so the working line prints `1-0.72^2`, not `1--0.72^2`.
- Fixed CSCALC Boolean `F(A,B,C)=...` minterm/POS free text so declared variables are preserved.
- Fixed CSCALC plain text `sum m 1 3 5 7` routing to the existing minterm engine.
- Preserved existing `bool(A,)` comma-not behavior and mixed-expression comma parsing.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 530260 bytes`
  - `CSCALC.g3a: 291280 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`
- Artifact hashes:
  - `CASP3.g3a: 9e69a919b2a3ee783cd73483efda2ab45c6f775325ce5abcd814f1ae94d5fd8c`
  - `CSCALC.g3a: 94c773ff8496a92b5cf62010eb164bbc328edbead5e0aa145b6f646f731fd49a`

Drift check:
- Stayed inside CASP3/CSCALC parser hardening, tests, checkpoint, and rebuilt app artifacts.
- No Pure CAS source, NOTES source, menu/UI/session, or shared status-bar code changed.
- Active goal remains open for further unseen-case hardening if more time is available.

## 2026-06-13 CSCALC Explicit Check Digit Weights Slice

Completed:
- Fixed check-digit free-text prompts with explicit digit and weight lists so stated weights are used instead of default descending weights.
- Added regression and rebuilt calculator-ready outputs.

Evidence:
- `digits 3 7 1 4 weights 5 4 3 2 mod 11` now gives `weighted sum = 54` and `check digit = (11-10) mod 11 = 1`.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 489784 bytes`
  - `CSCALC.g3a: 275036 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC check-digit free-text parsing, CS tests, checkpoint, and rebuilt CSCALC artifact.
- No Pure CAS source, CASP3 source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Parser Guard Sweep Slice

Completed:
- Fixed rough-plane prompts containing `to the horizontal` so they do not route as horizontal-plane force questions.
- Extended CSCALC image storage parsing for `photo` and `shades` wording.
- Prevented standalone colour-depth parsing from stealing photo/file-size prompts.
- Added fixed-point free-text route for two-number `bits after point` prompts.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Rough inclined plane prompt now uses `5*9.8 sin(30)` and friction along the plane.
- Greyscale `256 shades` prompt now uses `ceil(log2(256)) = 8 bits per pixel`.
- `3 megapixel photo ... colour depth 24 ... bytes` now uses `3000000*24`.
- Widthless fixed-point prompt now gives scaled-integer working instead of help fallback.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 489784 bytes`
  - `CSCALC.g3a: 274540 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text parser guards, tests, checkpoint, and rebuilt CASP3/CSCALC artifacts.
- No Pure CAS source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 P3 Incline Label Parser Slice

Completed:
- Fixed rough/smooth incline fallback parsing so `kg`, `degrees`, and `inclined at` wording are used before raw number order.
- Prevented angle-before-mass prompts from swapping mass and angle.
- Added regression and rebuilt calculator-ready outputs.

Evidence:
- `slides down a rough plane inclined at 25 degrees ... mass 3kg` now uses `3*9.8 sin(25)`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 489752 bytes`
  - `CSCALC.g3a: 273564 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3 incline free-text parsing, P3 tests, checkpoint, and rebuilt CASP3 artifact.
- No Pure CAS source, CSCALC source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 CSCALC Image Depth Parser Slice

Completed:
- Fixed generic image/bitmap storage parsing so answer-unit wording like `in bytes` does not make palette size or bit depth get treated as bytes per pixel.
- Kept explicit `bytes per pixel` and small labelled byte-depth prompts working.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- `256 colours` image prompts now use `ceil(log2(256)) = 8 bits per pixel`.
- `depth 24 calculate bytes` now uses 24 bits per pixel, not 24 bytes per pixel.
- `depth=3 bytes` and `3 bytes per pixel` prompts still use 24 bits per pixel.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 489492 bytes`
  - `CSCALC.g3a: 273564 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC image storage parsing, CS tests, checkpoint, and rebuilt CSCALC artifact.
- No Pure CAS source, CASP3 source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 P3 Braking Route Precedence Slice

Completed:
- Fixed down-slope constant-speed braking-force prompts so they route before generic SUVAT deceleration.
- Added explicit force-balance working: down-plane weight component, resistance/braking up the plane, and `mg sin(theta) = resistance + braking force`.
- Added regression and rebuilt calculator-ready outputs.

Evidence:
- Targeted braking prompt now gives `constant speed down the slope`, `mg sin(theta) = resistance + braking force`, and `braking force =`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 489492 bytes`
  - `CSCALC.g3a: 273336 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3 route precedence, P3 tests, checkpoint, and rebuilt CASP3 artifact.
- No Pure CAS source, CSCALC source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 P3 CS Wording Precedence Slice

Completed:
- Ran another source-like P3/CS probe batch.
- Added normal-approximation precedence for coin/toss wording with stated probability.
- Added Poisson `not equal` complement working for `X~Po(...)` notation.
- Extended regression-line parsing from `mx+c` to `c+mx` forms.
- Added CSCALC instruction-field remaining-bit working.
- Added explicit RLE encoded-string output.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Coin normal-approx prompt now gives `29.5 < Y < 60.5` and `NormalCD`.
- Poisson not-equal prompt now gives `P(X!=0)=1-P(X=0)`.
- Regression prompt `y=2.5+0.8x` now estimates from the given line.
- Instruction prompt now gives `remaining bits = 32 - 6 - 20 = 6 bits`.
- RLE prompt now gives `encoded string = 4A2B2C1D2A`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 488568 bytes`
  - `CSCALC.g3a: 273336 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text routing, tests, checkpoint, and rebuilt generated artifacts.
- No Pure CAS source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 P3 Route Precedence And Raw Data Slice

Completed:
- Ran source-like P3/CS probe batch after the previous commit.
- Fixed connected smooth-table particle wording so it gives system acceleration and tension instead of falling into elastic-string working.
- Narrowed elastic-string detection so Poisson `lambda` hypothesis tests are not stolen by Hooke's-law routes.
- Added greatest-height output when projectile ground-hit prompts also ask for greatest/maximum height.
- Extended raw-data median/IQR routing to questionnaire/coding/observation wording.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted connected prompt now gives `a = 25/(2+3) = 5` and `T = m1*a`.
- Targeted projectile prompt now includes `At greatest height` and the height line before range.
- Targeted questionnaire coding prompt now gives sorted raw-data median/IQR working.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 487604 bytes`
  - `CSCALC.g3a: 273872 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3 route precedence/free-text parsing, P3 tests, checkpoint, and rebuilt CASP3 artifact.
- No Pure CAS source, CSCALC source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 P3 Normal Vector Power Slice

Completed:
- Ran another 25-case CS/P3 unseen wording probe batch.
- Added P3 normal `within k standard deviations` handling using `mu +/- k*sigma` and `NormalCD`.
- Added P3 vector momentum route before scalar kinetic-energy fallback.
- Added explicit `F=ma` method line to power/resistance acceleration outputs.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted normal-within prompt now gives `NormalCD(lower=9, upper=15, sigma=2, mu=12)`.
- Targeted vector momentum prompt now gives `momentum = 12 i +6 j` and vector KE.
- Targeted power prompt now includes `Use F=ma with the resultant force.`
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 486152 bytes`
  - `CSCALC.g3a: 273872 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3 free-text/stat/mechanics routes, tests, checkpoint, and rebuilt CASP3 file.
- No Pure CAS, CSCALC source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 CSCALC Cache Scanner Slice

Completed:
- Ran another 30-case unseen CS/P3 wording probe batch.
- Fixed CSCALC cache address parsing when `address fields` appears before `32 bit addresses`.
- Fixed CSCALC `N cache lines` parsing for direct-mapped cache prompts.
- Tightened the shared number-before-word scanner so a bare word ending in `e` is not accepted as scientific-notation input.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted `256 sets ... 32 bit addresses` prompt now gives `tag bits = 32 - 8 - 5 = 19`.
- Targeted `512 cache lines of 8 bytes ... 20 bits` prompt now gives `cache size = lines*line size = 512*8 = 4096 bytes` and `tag bits = 20 - 9 - 3 = 8`.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 484016 bytes`
  - `CSCALC.g3a: 273872 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC shared scanners/cache parsing, tests, checkpoint, and rebuilt CSCALC calculator file.
- No Pure CAS, CASP3 source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Route Guard And Cache Lines Slice

Completed:
- Ran another unseen CS/P3 batch with cache line-count wording, rough horizontal force wording, projectile angle wording, Boolean, floating-point, storage, mechanics, and stats prompts.
- Fixed CSCALC cache prompts where `<n> lines, line size <b>` means `<n>` cache lines/blocks, not total cache bytes.
- Fixed P3 circular-motion route stealing caused by matching `round` inside `ground`.
- Fixed P3 projectile route reading `degrees above horizontal` as initial height.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted cache prompt now gives `cache size = lines*line size = 128*16 = 2048 bytes` and `tag bits = 24 - 7 - 4 = 13`.
- Targeted rough-horizontal prompt now gives rough-plane resolving/friction/F=ma lines.
- Targeted projectile prompt now gives time of flight and range, without false height.
- Unseen probe batch after fixes: `cases 19 failures 0`.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 484016 bytes`
  - `CSCALC.g3a: 273728 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC generic prose parsing, tests, checkpoint, and rebuilt generated files.
- No Pure CAS, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Cache Ways Prose Slice

Completed:
- Ran another unseen CS/P3 probe batch with source-like cache, floating-point, Boolean, mechanics, and statistics wording.
- Fixed CSCALC cache parsing for plural `ways`, e.g. `64 sets, 4 ways and line size 32 bytes`.
- Added a derivation line when cache size is inferred from `sets*ways*line size`.
- Added regression and rebuilt calculator-ready outputs.

Evidence:
- Probe case `A cache has 64 sets, 4 ways and line size 32 bytes. Find total cache capacity.`: passed with `8192 bytes`.
- Unseen probe batch after the fix: `failures 0`.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 483844 bytes`
  - `CSCALC.g3a: 273428 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC cache prose parsing, tests, checkpoint, and rebuilt CSCALC calculator file.
- No Pure CAS, CASP3 source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Boolean Prefix Cleanup Slice

Completed:
- Ran a compact unseen CS/P3 probe batch after the prior commit.
- Fixed CSCALC Boolean simplification prompts with prose prefixes and punctuation, e.g. `Simplify using Boolean algebra: (P+Q)(P+Q')`.
- Added regression and rebuilt calculator-ready outputs.

Evidence:
- Probe case `Simplify using Boolean algebra: (P+Q)(P+Q')`: passed with `simplified = P`.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 483844 bytes`
  - `CSCALC.g3a: 273260 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC Boolean prefix parsing, tests, checkpoint, and rebuilt calculator files.
- No Pure CAS, CASP3 source, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Boolean And Paper 3 Prose Hardening Slice

Completed:
- Fixed CSCALC Boolean expression cleanup so prose tails like `with variables A B C` are removed before parsing the expression.
- Fixed CSCALC truth-table output-column variable parsing so punctuation in `variables X Y.` does not drop the final variable.
- Fixed CASP3 rough-horizontal pulls written as `pulled ... by 40N at 25 degrees`.
- Fixed CASP3 slope-power acceleration prompts using attached units like `36kW` and `18m/s`.
- Added generic CASP3 beam/rod parsing for repeated `<load>N at <distance>m` loads, including rod mass converted to weight.
- Fixed CASP3 `X~N(mu,var)` conditional normal prompts before the plain upper-tail route can consume them.
- Fixed CASP3 sample-mean prompts from `N(100,15^2)` with symbolic bounds such as `96<Xbar<104`.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for Boolean truth tables, output columns, rough horizontal pulls, slope power acceleration, beam load pairs, conditional normal, and sample means: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 483844 bytes`
  - `CSCALC.g3a: 273156 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC generic free-text parsing, tests, checkpoint, and generated calculator files.
- No Pure CAS, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Bits Float Power And Pull Wording Slice

Completed:
- Ran another compact unseen wording probe batch after the prior commit.
- Fixed CSCALC `number of bits needed` wording for unsigned widths.
- Fixed CSCALC signed fixed-point decode when the prompt gives an integer bitstring plus fractional-bit count.
- Fixed CSCALC concatenated floating-point decode when mantissa/exponent widths are stated in prose.
- Kept floating-point absolute-error prompts on the error route instead of the plain decode route.
- Fixed CASP3 rough-horizontal prompts like `pulled by 30N at 20 degrees`.
- Fixed CASP3 `find engine power` on slopes at constant speed without hijacking `find angle given power`.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for bits-needed, signed fixed decode, concatenated float decode, rough horizontal pull, and slope engine power: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 479036 bytes`
  - `CSCALC.g3a: 273080 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text parser hardening, tests, checkpoint, and generated calculator files.
- No Pure CAS, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Sets Fixed-Point And Stratified Wording Slice

Completed:
- Ran another free-text probe batch across CS transfer/cache/fixed-point/storage and P3 mechanics/stats phrasing.
- Fixed CSCALC cache prompts that give `sets`, `ways`, and `line size` but no explicit capacity; capacity is derived as sets*ways*line size.
- Tightened cache set parsing so `set associative` and `blocks and sets` are not mistaken for a set count.
- Fixed CSCALC signed fixed-point decode when the bitstring has no binary point but the prompt says how many bits are after the point.
- Fixed CASP3 stratified sampling prompts where sample size and population are labelled, with group sizes listed between them.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted host probes for sets/ways/line cache, integer signed fixed-point decode, and labelled stratified sampling: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 477256 bytes`
  - `CSCALC.g3a: 270340 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text parser hardening, tests, checkpoint, and generated calculator files.
- No Pure CAS, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Send Line-Size And Plane Unit Wording Slice

Completed:
- Ran fresh CS/P3 free-text probes around transfer verbs, cache line-size wording, attached units, and no-mass rough-plane acceleration.
- Fixed CSCALC transfer routes so `send` is accepted as a transfer-time verb.
- Fixed CSCALC cache parsing so `line size` is treated as block size and MiB/MB cache capacities are scaled correctly.
- Fixed CASP3 smooth-plane force/mass parsing for attached units like `20N` and `5kg`.
- Fixed CASP3 rough-plane no-mass acceleration prompts using the mass-cancels formula.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted host probes for send/GiB transfer, line-size cache, MiB byte-addressable cache, attached-unit smooth plane, and no-mass rough plane: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 476696 bytes`
  - `CSCALC.g3a: 269692 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text parser hardening, tests, checkpoint, and generated calculator files.
- No Pure CAS, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Adjacent Units And Slope Resistance Slice

Completed:
- Ran another compact CS/P3 free-text probe pass.
- Fixed CSCALC storage-size units so `KiB` file sizes are not confused by nearby `Mbit/s` rate units.
- Fixed CSCALC direct-mapped cache wording and `byte block size` ordering.
- Fixed CASP3 slope resistance prompts so a stated resistance force is not treated as a coefficient of friction.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted host probes for direct-mapped cache, KiB/Mbit transfer, and slope resistance acceleration: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 475764 bytes`
  - `CSCALC.g3a: 269500 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text parser hardening, tests, checkpoint, and generated calculator files.
- No Pure CAS, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Cache Byte-Block Wording Slice

Completed:
- Probed another free-text CS/P3 batch after the previous commit.
- Found CSCALC cache prompts like `64 byte blocks` could fall back to the associativity value as block size.
- Fixed cache block-size extraction to accept singular `byte` and nearby `block(s)` wording before positional fallback.
- Added a regression and rebuilt calculator-ready outputs.

Evidence:
- Targeted host probe for `256 KiB 4 way cache ... 64 byte blocks ... 48 bit addresses`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 473668 bytes`
  - `CSCALC.g3a: 268500 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC cache free-text parsing, tests, checkpoint, and generated calculator files.
- No Pure CAS, menu/UI/session, CASP3 source, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Cache Transfer Normal Conditional Stratified Slice

Completed:
- Probed fresh CS/P3 free-text variants for cache address fields, packet-overhead transfer time, conditional normal probability, and labelled stratified sampling.
- Fixed CSCALC cache parsing so `32 bit address` is used for tag bits instead of the first earlier numeric value.
- Fixed CSCALC transfer-time overhead parsing so percentage and rate values are taken from labelled units, not raw numeric order.
- Fixed CASP3 normal conditional probability wording for `Given X is less than ..., find P(X>...)`.
- Fixed CASP3 stratified sampling when prompts give total population, total sample, then group sizes.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted host probes for cache, overhead transfer, normal conditional, and stratified wording: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 473668 bytes`
  - `CSCALC.g3a: 268368 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text parser hardening, tests, checkpoint, and generated calculator files.
- No Pure CAS, menu/UI/session, NOTES source, or shared status-bar behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 SOP/POS Assignment And Vector Mass Wording Slice

Completed:
- Probed more same-family unseen wording after the previous commit.
- Fixed CSCALC so `SOP form for ...`, `Find sum of products for F=...`, and `Find product of sums for F=...` ignore command words and assignment labels instead of treating them as Boolean variables.
- Fixed CASP3 vector impulse so `initial velocity ... final velocity ...` and `0.5kg particle velocity changes from ... to ...` route to component impulse working.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for SOP/POS assignment forms and kg vector impulse wording: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 472752 bytes`
  - `CSCALC.g3a: 268152 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside generic command-word/assignment cleanup and vector mechanics parser hardening.
- No menu/UI/session behavior changed.
- Active goal remains open for further unseen-case hardening.

## 2026-06-13 Boolean SOP And Vector Impulse Parser Slice

Completed:
- Probed fresh unseen-style CASP3/CSCALC prompts across Boolean SOP/POS conversion, address lines, Dijkstra, hashing, braking, grouped stats, trapezium rule, Spearman, and variable acceleration.
- Fixed CSCALC Boolean expression cleaning so `into sum of products` / `into product of sums` does not leave `in`/`into` as fake Boolean variables.
- Fixed CASP3 vector impulse routing so `changes velocity from ai+bj to ci+dj` uses full component-by-component impulse instead of falling through to a scalar route.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for `Convert (A+B)(A+C) into sum of products.` and vector impulse `from 3i-2j to -i+4j`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 472940 bytes`
  - `CSCALC.g3a: 267868 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside broad parser hardening and tests.
- No menu/UI/session behavior changed.
- Active goal remains open for further overnight unseen-case hardening.

## 2026-06-13 Geometric, Braking, And K-Map Wording Slice

Completed:
- Compared current CSCALC Boolean engine with the legacy Python Boolean parser idea for implicit products and K-map parsing.
- Fixed CSCALC K-map/minterm free-text parsing so hyphenated `K-map` is treated as a command word, not variables `K,M,A,P`.
- Added CASP3 direct geometric-distribution working for arbitrary `X~Geometric` tail/exact prompts using stated `p`.
- Broadened CASP3 braking/stopping-distance work-energy routing so it works without explicitly saying `work-energy`.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for `K-map variables A B C...`, `X has geometric distribution... P(X>5)`, and mass/speed/resistance stopping-distance wording: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 472572 bytes`
  - `CSCALC.g3a: 267824 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text route hardening, tests, checkpoint, and generated calculator files.
- Did not alter Pure CAS, NOTES source, menus, or shared UI/status code.
- Active goal remains open for more unseen-case hardening.

## 2026-06-13 CS Coding, Subnet, Hashing, And Graph Wording Slice

Completed:
- Added CSCALC Gray-code encode/decode commands and free-text routing.
- Added Hamming(7,4) even-parity encode working for 4-bit data.
- Added quadratic probing hash-table working, distinct from linear probing.
- Added dotted subnet-mask network/broadcast working, not only CIDR `/n`.
- Added bit-mask prose routing for AND/OR bit operations.
- Added deterministic exact-binary-fraction reasoning for decimal fractions such as `0.1`.
- Extended Dijkstra free-text parsing so compact edge notation like `AB 4` is accepted.
- Added regressions and rebuilt calculator-ready output.

Evidence:
- Hard host probes for Gray, Hamming encode, quadratic probing, dotted subnet mask, bit mask, exact `0.1`, and compact Dijkstra: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 438640 bytes`
  - `CSCALC.g3a: 249776 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC route/engine hardening, tests, checkpoint, and generated CSCALC artifact.
- Did not alter Pure CAS, CASP3 source, NOTES source, menus, or shared UI/status code.
- Active goal remains open for more unseen-case hardening.

## 2026-06-12 Boolean Gate Output Wrapping Slice

Completed:
- Compared current CSCALC Boolean engine against the old Python `booleanProgram.py` history.
- Probed harder Boolean, NAND/NOR, minterm, truth-column, two's-complement hex, and P3 mechanics/stats free-text cases.
- Found a quality issue where long NAND/NOR-only formulas could be truncated by the 80-character calculator line buffer.
- Added shared wrapped output for long CSCALC lines and applied it to NAND/NOR universal-gate formulas.
- Added a regression for long NOR-only output.
- Rebuilt calculator-ready outputs.

Evidence:
- Hard host probe batch: passed, except the long gate output quality issue that this slice fixed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 438640 bytes`
  - `CSCALC.g3a: 245716 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CSCALC output quality, tests, checkpoint, and generated calculator files.
- Did not alter Pure CAS, CASP3 source, NOTES source, menus, or shared UI/status code.
- Active goal remains open for more unseen-case hardening.

## 2026-06-12 Inverse Normal And Hex Two's-Complement Slice

Completed:
- Fixed CASP3 free-text normal routing so percentile/unknown-k prompts use inverse normal before tail CDF.
- Kept central normal interval prompts on the central-interval route.
- Added CSCALC fixed-point relative-error working for stored rounded fractional values.
- Added CSCALC two's-complement hexadecimal encode/decode/addition routes so signed intent wins over unsigned hex conversion.
- Added regressions and rebuilt calculator-ready outputs.

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
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 438640 bytes`
  - `CSCALC.g3a: 245456 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text engines, tests, checkpoint, and generated calculator files.
- Did not alter Pure CAS menus, NOTES source, or shared UI/status code.
- Active goal remains open for more unseen-case hardening.

## 2026-06-12 Normal Notation, Inverse Binomial, Varacc, And CS Timing Slice

Completed:
- Added CASP3 direct `X~N(mu,sigma^2)` probability parsing, including intervals and central intervals where a bound can equal the variance numerically.
- Added CASP3 inverse cumulative binomial working for prompts such as `smallest k such that P(X<=k)>0.95`.
- Fixed CASP3 variable-acceleration wording with `at time n`, while keeping plain constant-acceleration prompts on SUVAT.
- Added `was` as a general P3 numeric connector, fixing prompts such as `variance was 9`.
- Added CASP3 variance-after-removal working when the removed value and old variance are described in prose.
- Added CSCALC CPU timing support for `billion` and `thousand` instruction magnitude words.
- Added CSCALC free-text modulo check-digit working.
- Fixed CSCALC normalised floating-point questions so `Is ... normalised?` gives a yes/no normalisation check instead of a renormalisation trace.
- Rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for the above cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 438328 bytes`
  - `CSCALC.g3a: 242296 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text engines, tests, checkpoint, and generated calculator files.
- Did not alter Pure CAS, menus, NOTES source, or shared UI/status code.
- Active goal remains open for more unseen-case hardening.

## 2026-06-12 CS Bit Width And P3 Inverse Stats Slice

Completed:
- Added CSCALC free-text handling for `how many bits are needed...` two's-complement prompts, including values below the current range such as `-129`.
- Fixed CSCALC sign-and-magnitude free text where wording gives the bit width before the negative value.
- Added CSCALC character-set bit-width working for prompts like `300 different characters`.
- Added CASP3 mean update working for `n values have mean m; another value is added and the mean becomes M`.
- Added CASP3 Poisson mean inference from `P(X=0)=q`.
- Rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for the above cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 433464 bytes`
  - `CSCALC.g3a: 241336 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working engines, tests, checkpoint, and generated calculator files.
- Did not alter Pure CAS, menus, NOTES source, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Distance, Probability Target, Projectile Angle, And De Morgan Slice

Completed:
- Added CASP3 total-distance working for velocity prompts phrased as `in the first n seconds`.
- Added CASP3 two-without-replacement different-colour probability via complement of same colour.
- Fixed CASP3 first-target-without-replacement target inference for prompts like `blue is first obtained on the fourth draw`.
- Added CASP3 same-level projectile range-to-angle handling for `lands x m away, find possible angles`.
- Added CSCALC explicit 3-variable De Morgan rewrite for `not(A or B or C)`.
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- New hard probe batch: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 406544 bytes`
  - `CSCALC.g3a: 231728 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working engines, tests, checkpoint, and generated calculator files.
- Did not alter Pure CAS, menus, NOTES, or shared UI.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Final Route-Priority Hardening Slice

Completed:
- Kept the final pass small and general: no menu churn, no Pure CAS edits, no broad refactor.
- Hardened CASP3 route priority for:
  - all-different without-replacement colour counters
  - first target on ordinal draws such as `third draw`
  - `kx(a-x)` pdf probability prompts, not only mean prompts
  - quadratic CDF median prompts
  - projectile `hit ground` from a height
  - acceleration on an incline when driving force and resistance are already given
- Hardened CSCALC route priority for:
  - binary two's-complement addition prompts that also say `two's complement`
  - combined floating-point bit strings split by stated mantissa/exponent widths
  - Mbps bit-rate output wording
- Added regressions and rebuilt calculator-ready outputs.

Evidence:
- Targeted hard probes for the above cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 404176 bytes`
  - `CSCALC.g3a: 231456 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: 18178 bytes`

Drift check:
- Stayed inside final CASP3/CSCALC free-text parsing and generated calculator outputs.
- Removed no tested capability; broad code deletion was avoided because current route breadth is what protects unseen inputs.
- Active goal remains open for future source-driven hardening, but this checkpoint is built and usable.

## 2026-06-12 CS Boolean Named-Function And Implication Slice

Completed:
- Recovered the old Python Boolean implementation from git history and compared it with the current CSCALC C++ Boolean engine.
- Confirmed the current C++ engine already carries the old core idea plus truth tables, K-map/minterms/maxterms, POS, NAND/NOR forms, proof checks, and Boolean-law traces.
- Fixed free-text extraction gaps found by probes:
  - `F(A,B,C)=...` simplification now simplifies the RHS instead of proving against the function label.
  - `F(A,B)=...` POS/CNF prompts now use the RHS expression.
  - `A implies B` is accepted in simplify/truth-table free text and normalized to implication.
  - `to truth table` sentence tails no longer become fake variables.
  - `variables X Y Z` in K-map prompts no longer treats `X` as a don't-care marker.
- Added regressions and rebuilt calculator-ready files.

Evidence:
- Targeted probes for named Boolean functions, implication truth tables, POS named functions, and K-map variables: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 394132 bytes`
  - `CSCALC.g3a: 230584 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CSCALC free-text Boolean parsing and generated calculator output.
- Did not touch Pure CAS, CASP3 source, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further unseen-case hardening.

## 2026-06-12 Incline Resistance, Beam Mass, Colour Count, Float Range, And De Morgan Slice

Completed:
- Added CASP3 free-text working for vehicles on an inclined plane at constant speed/acceleration with resistance, avoiding false `mu R` friction parsing.
- Added CASP3 uniform beam/rod mass handling by converting mass to weight at the midpoint.
- Added CSCALC direct colour-count handling from bits per pixel.
- Fixed CSCALC floating-point range prose so largest/smallest normalised value is handled before bit-pattern normalisation checks.
- Added direct CSCALC De Morgan rewrite working when the prompt asks to rewrite without giving a target RHS.
- Rebuilt calculator-ready outputs.

Evidence:
- Targeted probes for incline resistance, beam mass, colour count, floating range, and De Morgan rewrite: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 364076 bytes`
  - `CSCALC.g3a: 220044 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC generic free-text working support and generated calculator outputs.
- Did not touch Pure CAS, menus, NOTES source, or shared UI/status code.
- Active goal remains open for further unseen-case hardening.

## 2026-06-12 DAC Resolution And Coded Variance Slice

Completed:
- Added CSCALC DAC voltage-resolution free-text support by sharing the generic ADC quantisation route.
- Added CASP3 coded-data variance working for both X-from-Y and Y-from-X prompts.
- Kept the changes minimal: no Pure CAS, menu, UI, or NOTES changes.
- Rebuilt calculator-ready outputs.

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
  - `CASP3.g3a: 362140 bytes`
  - `CSCALC.g3a: 219064 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Avoided broad deletion/refactor at finalisation stage because the tested breadth depends on the current generic route set.
- Active goal remains open for future unseen-case hardening.

## 2026-06-12 Final PMCC, Mean Update, Boolean, And CPU Timing Slice

Completed:
- Added CASP3 generic PMCC significance-test fallback when prose gives `n`, `r`, alpha, and tail but no critical value.
- Added CASP3 mean-after-removal/addition working so these prompts no longer fall into generic `meanvar`.
- Hardened CSCALC Boolean free-text RHS trimming so prose such as `to a truth table` is not parsed as variables.
- Fixed CSCALC Boolean XOR law trace to use bounded string construction on host and calculator target.
- Added CSCALC CPU execution-time working from clock rate, instruction count, and cycles per instruction.
- Added CSCALC address/data bus memory output in KiB/MiB where relevant.
- Rebuilt all calculator-ready files.

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
  - `CASP3.g3a: 352920 bytes`
  - `CSCALC.g3a: 214456 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Kept Pure CAS stable; this slice only touched CASP3/CSCALC engines, their tests, checkpoint, and generated calculator outputs.
- Avoided broad code deletion/refactor at finalisation stage because the tested free-text breadth depends on the existing generic route set.
- Active goal remains open for future unseen-case hardening; current calculator outputs are freshly built and usable.

## 2026-06-12 Fair-Die Normal Approximation Slice

Completed:
- Added CASP3 free-text support for fair six-sided die normal-approximation prompts.
- The parser now infers `X~B(n,1/6)` from die/dice wording, applies the requested tail, and emits continuity-correction working.
- Added regression and rebuilt calculator files.

Evidence:
- Targeted probe `A fair six sided die is rolled 60 times... P(X >= 18)`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 350168 bytes`
  - `CSCALC.g3a: 213020 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3 free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, CSCALC source, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Beam Lists, Histogram Triples, Image Depth, And Latency Slice

Completed:
- Fixed CASP3 beam prose where load magnitudes are listed before positions, e.g. `Loads of 20N and 30N act 2m and 7m from A`.
- Fixed CASP3 histogram prose with repeated `class a-b density d` triples.
- Fixed CSCALC reverse bitmap colour-depth parsing so the number beside `MiB`/`MB`/etc is used as file size, not the first number in the prompt.
- Added CSCALC latency working for distance/speed prompts, including scientific notation such as `2.0e8 m/s`.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for beam load-list reactions, histogram class-density frequencies, reverse bitmap colour depth, and propagation latency: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 349264 bytes`
  - `CSCALC.g3a: 213020 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Function Trapezium, Normal Absolute, And Image Depth Slice

Completed:
- Added CASP3 trapezium-rule working from a simple function, bounds, and strip count, not only supplied y-values.
- Added CASP3 normal absolute-deviation probability working for prompts such as `P(|X-mu|<a)`.
- Added CSCALC reverse bitmap colour-depth working from file size and resolution.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for function trapezium, normal absolute-deviation probability, and bitmap colour-depth reverse calculation: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 347648 bytes`
  - `CSCALC.g3a: 212200 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Projectile Ground, Timed Varacc Constants, And Binary-Unit Transfer Slice

Completed:
- Fixed CASP3 variable-acceleration working when the given velocity/displacement are at `t=t0`, not only at `t=0`.
- Added CASP3 labelled projectile-from-height ground-impact working for speed, launch angle, height, time, and range.
- Added CSCALC separated binary/decimal transfer-unit scanning so `KiB` file size with `Mib/s` rate does not fall back to unitless division.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for nonzero-initial-time variable acceleration, projectile impact from height, and `800 KiB` over `2 Mib/s`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 345776 bytes`
  - `CSCALC.g3a: 210960 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Boolean Implication, Fixed-Point Widths, Compression Percent, And Variable Acceleration Slice

Completed:
- Added CSCALC Boolean implication parsing so free-text truth tables for `A implies B` use `A>B`, not the letters in `implies`.
- Fixed fixed-point free-text encoding where width wording appears before the value, e.g. `5 bits before ... 3 bits after ... -6.375`.
- Fixed compression percent-reduction prompts so `compressed by 35 percent` gives the compressed size, not a bogus ratio.
- Added CASP3 variable-acceleration displacement working from `a(t)`, initial velocity, initial displacement, and target time.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for Boolean implication, fixed-point two's-complement encoding, compression by percent, and variable acceleration: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 343324 bytes`
  - `CSCALC.g3a: 209940 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Exponential Tail, Poisson Test, Motion Split, Histogram, Sound, And CIDR Slice

Completed:
- Added CASP3 exponential pdf tail working for `f(x)=lambda exp(-lambda x)` prompts.
- Fixed free-text Poisson hypothesis parsing so labelled `lambda`, observed value, percent alpha, and increased-tail wording are respected.
- Added CASP3 two-stage acceleration/deceleration total-distance working while ignoring unit exponents such as `m/s^2`.
- Added multi-class histogram frequency-from-density working.
- Fixed CSCALC labelled sound storage parsing so duration/resolution/channels are not swapped.
- Added CSCALC usable host range output for CIDR prompts requesting it.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for exponential tail, Poisson hypothesis test, two-stage motion, histogram frequencies, sound storage, and CIDR host range: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 342100 bytes`
  - `CSCALC.g3a: 208092 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Rough Table Pulley And Exponential CDF Slice

Completed:
- Added CASP3 rough-horizontal-table pulley working before the plain friction fallback.
- Added acceleration and tension lines for table particle plus hanging particle systems.
- Added CASP3 exponential CDF median route for `F(x)=1-exp(-lambda*x)` before the loose power-CDF fallback.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for rough table pulley and exponential CDF median: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 337996 bytes`
  - `CSCALC.g3a: 207564 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3 free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, CSCALC source, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Geometric Cumulative And Rough Incline Pull Slice

Completed:
- Added CASP3 cumulative geometric working for prompts such as first defective item on or before a trial.
- Added CASP3 rough inclined-plane acceleration for an applied force parallel to the plane.
- Kept existing angled rough-plane pull handling intact.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for first defective on/before 5 and rough incline parallel pull: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 336400 bytes`
  - `CSCALC.g3a: 207564 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3 free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, CSCALC source, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Mixed Binomial Intervals, Normal Absolute Deviation, And Attribute Storage Slice

Completed:
- Added CASP3 symbolic mixed inclusive/exclusive binomial interval handling, e.g. `5 <= X < 12` and `3 < X <= 8`.
- Fixed CASP3 interval matcher order so `X <= upper` is not treated as strict `< upper`.
- Added CASP3 normal absolute-deviation probability handling, e.g. `P(abs(X-mu)<a)`.
- Added CSCALC table/attribute/key storage parsing for database table wording.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for `P(5 <= X < 12)`, `P(abs(X-40)<7)`, and table attribute/key storage: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 333764 bytes`
  - `CSCALC.g3a: 207564 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Cumulative Frequency, Stratified Lists, And Multi-Field Records Slice

Completed:
- Added CASP3 cumulative-frequency table interpolation from boundary and cumulative-frequency lists.
- Added CASP3 multi-stratum sampling from population-size lists and total sample size.
- Prevented cumulative-frequency prompts from being treated as algebraic CDF prompts.
- Added CSCALC repeated field-group parsing such as `3 fields of 12 bytes and 2 fields of 4 bytes`.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for cumulative-frequency median/IQR, multi-stratum sampling, and multi-field record storage: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 332292 bytes`
  - `CSCALC.g3a: 206692 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Raw Paired Data And SQL Aggregate Slice

Completed:
- Added CASP3 raw paired-list handling for PMCC from `x values ... y values ...`.
- Added CASP3 raw paired-list handling for Spearman rank correlation, including average ranks for ties.
- Added CASP3 raw paired-list regression working and summary-statistic regression for `x on y`.
- Tightened route order so regression prompts are not intercepted by interpolation and Spearman prompts are not swallowed by PMCC.
- Fixed CSCALC SQL free-text aggregates: AVG, SUM, MIN, MAX.
- Fixed CSCALC record-size wording for `n fields each m bytes` plus extra per-record bytes.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for raw PMCC, raw Spearman, raw regression, `x on y` regression, SQL AVG/SUM, and record-field storage: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 330396 bytes`
  - `CSCALC.g3a: 206452 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Storage, Histogram, Trapezium, And Complement Route Slice

Completed:
- Fixed CSCALC image storage wording with explicit metadata/header bytes.
- Fixed CSCALC record-size wording with repeated fields plus key bytes.
- Fixed CASP3 trapezium wording with separate `x values` and `y values`.
- Added CASP3 multi-class histogram density working.
- Added CASP3 independence testing when a conditional probability is given.
- Fixed CASP3 binomial `not equal` wording to use the complement route.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for image metadata, record fields, trapezium x/y values, histogram densities, conditional independence, and binomial complement: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 327520 bytes`
  - `CSCALC.g3a: 205892 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Free-Text Gate, Poisson Interval, And Conditional Probability Slice

Completed:
- Fixed CSCALC free-text NAND/NOR gate conversion so instruction words are not treated as Boolean variables.
- Added XOR recognition in free-text gate conversion.
- Shortened generic NAND/NOR conversion by simplifying negated AND/OR and double negation, avoiding truncated calculator lines.
- Fixed CASP3 Poisson interval parsing so `a <= X <= b` routes to range probability before single-tail fallback.
- Fixed CASP3 natural Poisson conditional wording such as `given X is at least 2`.
- Fixed CASP3 probability wording where `P(A|B)` is given and `P(A and B)`/`P(A or B)` are requested.
- Added regressions and rebuilt calculator files.

Evidence:
- Targeted probes for free-text gate forms, Poisson intervals, Poisson conditional wording, and conditional probability wording: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 325628 bytes`
  - `CSCALC.g3a: 204772 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Stayed inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Boolean Minimiser And Gate-Form Hardening Slice

Completed:
- Compared current CSCALC Boolean behavior against the legacy Python Boolean program in git history.
- Fixed CSCALC Boolean minimisation buffer caps so 6-variable simplifications do not silently drop a variable.
- Fixed XOR conversion to NAND-only and NOR-only forms so it uses the actual operands, not hard-coded `A` and `B`.
- Added regression coverage and rebuilt calculator files.

Evidence:
- Legacy reference found at `d9a86d0f173a39411a40dd4f150ee874aa251cd1:python/src/ComputerScience/booleanProgram.py`.
- Direct probes for XOR NAND/NOR conversion and 6-variable Boolean simplification: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 324432 bytes`
  - `CSCALC.g3a: 204504 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Only CSCALC Boolean source/tests and generated `CSCALC.g3a` changed.
- Did not touch CAS Pure behavior, P3 logic, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Route-Order Hardening For Shifted P3 And CS Parser Cases

Completed:
- Fixed CASP3 linear denominator parsing so missing explicit `^1` defaults correctly for `(ax+b)` routes.
- Added CASP3 reciprocal-linear pdf mean working via `E(X)=integral x*f(x) dx`.
- Moved rational velocity displacement above polynomial fallbacks so `v=k/(at+b)+c` gives log working, not a false polynomial route.
- Added shifted linear-power force work handling in both free-text and mechanics paths, e.g. `F=(2x+1)^3`.
- Fixed CASP3 sample-mean interval wording such as `sample of 36 ... between a and b`.
- Fixed shorthand binomial interval wording such as `X~B(n,p) between a and b inclusive`.
- Hardened CSCALC minterm/K-map parsing for `for variables A B C`, punctuation, `don't cares`, and article words.
- Fixed CSCALC image storage wording for byte-per-pixel colour depth and added KiB output.
- Rebuilt calculator files.

Evidence:
- Fresh adversarial probes for the patched P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 324432 bytes`
  - `CSCALC.g3a: 204320 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Linear-Power Pdf, Log Antiderivative, Mixed Conditional, And Transfer Overhead Slice

Completed:
- Added CASP3 generic CDF handling for endpoint-normalised fixed-denominator powers such as `(x^3-1)/26`.
- Added CASP3 generic pdf handling for `k(ax+b)^n`, including normalisation, tail probabilities, and `E(X)`.
- Extended CASP3 reciprocal-linear antiderivatives so `k/(ax+b)` works for pdfs, acceleration integration, and variable-force work.
- Fixed CASP3 parser scanning so `k(5-x)` is not misread as `k(5x)`.
- Added CASP3 mixed normal conditional handling for cases such as `P(X<a given X>b)` by using the intersection interval.
- Added CSCALC transfer-time handling where packet overhead is a percentage added to payload bits.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh hard probes for the patched P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 315464 bytes`
  - `CSCALC.g3a: 202576 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 CDF, Rational Acceleration, Conditional Probability, And Float Width Slice

Completed:
- Added CASP3 additive-constant CDF handling for forms like `(x^2+ax+k)/d`, including endpoint consistency checks.
- Added CASP3 rational acceleration integration for `a=k/(bt+d)^n` with an initial velocity condition.
- Extended CASP3 rational velocity displacement to include an added constant term.
- Fixed exact binomial interval wording such as `at least a and less than b` for shorthand `X~B(n,p)`.
- Fixed Poisson conditional free text so the main event is read from `P(...)` and the condition from `given ...`.
- Fixed CSCALC floating-point free text so `convert ... with m bit mantissa and e bit exponent` uses the explicit mantissa/exponent widths.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh hard probes for the patched P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 312184 bytes`
  - `CSCALC.g3a: 201624 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Rational Denominator And Subtract-From Slice

Completed:
- Added a shared CASP3 parser and antiderivative helper for linear-power denominators `(ax+b)^n`.
- Generalised CASP3 pdf, displacement, and work routes for `k/(ax+b)^n`, including normalisation and probability working.
- Fixed CASP3 sample-mean free-text parsing so sample size is not mistaken for population mean.
- Fixed CSCALC two's-complement wording for `subtract X from Y` so operands are handled in exam order.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh unseen probes for the patched P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 306556 bytes`
  - `CSCALC.g3a: 201608 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 CDF, Normal Inverse, Projectile, And Fixed Decode Slice

Completed:
- Added CASP3 shifted power-CDF probability working for forms like `F(x)=k(x^2-a)` with `P(X<...)`/`P(X>...)`.
- Extended CASP3 inverse-normal output to include the numeric `z` value and substituted critical value, while keeping fx-CG50 menu guidance.
- Fixed CASP3 displacement-polynomial derivative working so `s(t)` questions return `v=ds/dt` and `a=dv/dt` without stealing velocity/displacement integration questions.
- Added CASP3 combined projectile time-of-flight plus maximum-height working from speed and angle.
- Hardened CSCALC fixed-point routing so denary decimals are not misread as binary fixed-point tokens.
- Added CSCALC fixed binary-to-denary/hex working, signed two's-complement shift from denary input, combined subnet mask plus usable-host output, KiB image-size output, and truth-table prompt stripping.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh unseen probes for the patched P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 303644 bytes`
  - `CSCALC.g3a: 201568 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Shifted Pdf, Power Work, Fixed Subtract, And Address Bits Slice

Completed:
- Added CASP3 shifted linear pdf support for `k(a-x)` and `k(x-a)` over nonzero intervals, including probability and mean working.
- Added CASP3 rational velocity displacement working for forms like `v=1/(t+1)^2`.
- Generalised CASP3 variable-force work for `k/x^n` instead of only `x^-2`.
- Added CSCALC binary fixed-point subtraction working.
- Fixed CSCALC decimal-to-binary fixed-point routing so denary decimals such as `0.1` are not misread as binary fixed-point inputs.
- Fixed CSCALC address-bit free-text route for memory-location counts such as `4096 memory locations`.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh unseen probes for the patched P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 301052 bytes`
  - `CSCALC.g3a: 198208 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Rational Motion, Simpson, Fixed-Point, And Complexity Slice

Completed:
- Added CASP3 rational variable-acceleration working for forms like `a=12/(2t+1)^3` with an initial velocity condition.
- Hardened CASP3 polynomial-only parsers so rational force/acceleration expressions are not silently treated as constants.
- Added CASP3 inverse-square variable-force work integration.
- Added CASP3 Simpson's-rule working from ordinate lists and `h`.
- Added CASP3 projectile direction-after-time working from resolved velocity components.
- Fixed CASP3 shifted linear pdf normalisation for nonzero lower bounds and shifted log CDF median working.
- Added CSCALC decimal-to-binary fixed-point fraction working, binary fixed-point addition, exponent-only range working, and Big-O route ordering before Boolean fallback.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh unseen probes for the patched P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 298540 bytes`
  - `CSCALC.g3a: 197628 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Projectile, Geometric Tail, RSA Decrypt, And Packet Slice

Completed:
- Added CASP3 projectile speed-after-time working using resolved velocity components.
- Fixed CASP3 rough-horizontal pull-at-angle working to distinguish forces above vs below the horizontal when finding the normal reaction and friction.
- Added CASP3 geometric tail working for first success after `r` attempts.
- Added CASP3 shifted reciprocal pdf normalisation for forms such as `k/(x+1)^2`.
- Added CASP3 trapezium-rule working from ordinate lists and width.
- Added CSCALC RSA decryption working from ciphertext, `n`, and private exponent `d`.
- Extended CSCALC packet-overhead working to include efficiency when requested.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes for the new P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 295036 bytes`
  - `CSCALC.g3a: 194292 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for further source/probe-driven hardening.

## 2026-06-12 Vector, Distribution, Floating Mantissa, And RSA Slice

Completed:
- Added CASP3 vector constant-acceleration working for unknown initial velocity from two position vectors.
- Added CASP3 vector `r=r0+ut+1/2at^2` position working from acceleration, initial velocity, and initial position.
- Added CASP3 vector-velocity differentiation/integration working for acceleration at a time and displacement over an interval.
- Fixed CASP3 `i`/`j` vector scanning so letters inside ordinary words are not treated as vector components.
- Added CASP3 reciprocal pdf `k/x^2`, logarithmic CDF median, geometric first-success, and without-replacement same-colour working.
- Added CSCALC mixed binary/hex/decimal comparison working before generic conversion routes.
- Added CSCALC RSA modulus/phi/private-key/ciphertext working.
- Fixed CSCALC dotted floating mantissas like `0.101000` so the leading sign bit is kept before float add/sub/mul/div routing.
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
  - `CASP3.g3a: 291300 bytes`
  - `CSCALC.g3a: 193304 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 CDF, Slope Power, and Record Units Slice

Completed:
- Added CASP3 generic free-text CDF route for `F(x)=kx^n` on `0<x<a`, including `P(X>...)` and `P(X<...)` tails.
- Preserved existing interval CDF handling by keeping `P(a<X<b)` ahead of tail fallback.
- Added CASP3 power output for constant-speed slope/resistance prompts after finding driving force.
- Added CSCALC KiB/MiB unit lines for record and database storage calculations.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed:
  - `F(x)=kx^2 for 0<x<2 ... P(X>1)` now gives `k=0.25`, `P(X>1)=0.75`.
  - constant-speed slope power now gives `Power = Fv`.
  - record/database storage now includes KiB output.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 374556 bytes`
  - `CSCALC.g3a: 224492 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Discrete Tables, Geometric Dice, Nyquist, and File-Size Slice

Completed:
- Added CASP3 labelled discrete probability-table parsing for `x: ... p: ...` prompts.
- Added CASP3 proportional discrete distribution working for `P(X=x)` proportional to `x`.
- Added CASP3 geometric die working for first-six-after prompts.
- Added CASP3 multi-force polar resultant working by resolving all force components.
- Added CASP3 hanging elastic-string extension working using `T=mg` and Hooke's law.
- Added CSCALC Nyquist minimum sampling-frequency working.
- Added CSCALC file-size-from-bitrate-duration working.
- Added CSCALC image colours-from-file-size working.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch: all newly targeted P3/CS cases produced working lines.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 372648 bytes`
  - `CSCALC.g3a: 224276 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Final Generic Cleanup Slice

Completed:
- Hardened CASP3 variable-force impulse parsing so `F=(at+b)` gives impulse and speed working.
- Added smooth banked-curve working before broader circular/friction routes.
- Added generic continuous pdf working for `k*x*(a-x)` and numeric `c*x^n` moment/variance cases.
- Added combined-mean free-text handling before the generic `meanvar` fallback.
- Hardened CSCALC CPU execution-time parsing for `CPI` wording.
- Fixed image metadata overhead so percent overhead is not treated as bytes.
- Added sensor sample-storage working without stealing sound/audio sample routes.
- Fixed free-text XOR truth-table parsing so prompt words are not treated as variables.
- Added regression tests and rebuilt calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 369840 bytes`
  - `CSCALC.g3a: 221392 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Polynomial Motion, Poisson Interval, And CS Unit Slice

Completed:
- Fixed CASP3 variable-acceleration prompts with labelled `v=`, `u=`, `s=`, and `t=` values so initial conditions are not guessed from number order.
- Added CASP3 quartic-displacement stationary-time working instead of misreading quartics as cubics.
- Fixed CASP3 strict discrete probability intervals so `P(2<X<7)` becomes `P(3<=X<=6)`.
- Hardened CSCALC image colour-count parsing for wording such as `16 million colours`.
- Hardened CSCALC compression working for mixed units such as `3 MiB` to `750 KiB`.
- Added CSCALC Dijkstra prose parsing for `from A to E with edges A B 2 ...`.
- Added regression tests and rebuilt calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- No menu/catalog/UI/status changes.
- Active goal remains open for continued hardening.

## 2026-06-12 Parser Precedence And Network Slice

Completed:
- Fixed CASP3 two-stage acceleration/deceleration total-time working so total distance/final braking distance are not treated as a single SUVAT route.
- Added CASP3 cubic-velocity total-distance working and regression coverage.
- Fixed CASP3 Poisson rate scaling for `per day/hour/minute` plus separate durations such as `in 6 hours`.
- Added CASP3 unknown normal standard-deviation working from probability statements.
- Added CASP3 `kx^n` pdf probability working for `P(X<...)` after normalising.
- Fixed CASP3 histogram class-interval prompts such as `class 10 to 20` with frequency density.
- Added CASP3 rough-incline pull-at-angle acceleration working.
- Fixed CASP3 engine-power/resistance acceleration prompts via `P=Fv`.
- Fixed CSCALC slash-CIDR IPv4 network/broadcast parsing and host-bit sizing.
- Added CSCALC denary-value normalised floating-point encoding route.
- Hardened CSCALC minterm prose and Boolean equality tail cleanup.
- Added CSCALC Hamming parity-bit count working.
- Added regression tests and rebuilt calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- No menu/catalog/UI/status changes.
- Active goal remains open for continued hardening.

## 2026-06-12 Variable Force, Pdf Power, Fixed-Point, And Storage Slice

Completed:
- Fixed CASP3 variable-force work prompts so `F=(3x^2+2x)` integrates force over displacement instead of using the constant-force route.
- Added CASP3 work-energy with resistance prompts for final speed.
- Fixed CASP3 cubic velocity prompts so they integrate cubic velocity instead of being misread as quadratic SUVAT-style input.
- Added CASP3 generic `kx^n` pdf normalisation/mean working.
- Added CASP3 linear-pdf interval probability working.
- Fixed CASP3 `X~B(n,p)` hypothesis tests where the null probability is given in prose.
- Added CASP3 regression-summary working from `sum x`, `sum y`, `sum x^2`, `sum xy` prompts.
- Added CASP3 labelled box-plot outlier working from Q1/Q3/min/max prompts.
- Fixed CSCALC fixed-point prompts with separate integer/fractional bit counts.
- Added CSCALC record-overhead storage working.
- Added CSCALC byte-addressable RAM address-bit working.
- Added compact CSCALC Booth multiplication working.
- Added regression tests and rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 P3 Method Coverage and CS Parser Guard Slice

Completed:
- Added CASP3 confidence-interval working for sample mean prompts.
- Added CASP3 linear interpolation working from two points.
- Added CASP3 angular-speed circular-motion working and hill normal-reaction working.
- Narrowed CASP3 circular route guards so `radius` no longer triggers angular-speed handling.
- Added CASP3 rough-wall/rough-floor ladder equilibrium equations with an underdetermined-contact warning instead of wrong smooth-wall working.
- Added CASP3 `kx(a-x)` continuous pdf normalisation/mean route.
- Added CSCALC SQL parser support for schema-style prompts without explicit `FROM`.
- Added CSCALC multi-field `SELECT` handling without splitting selected fields into condition arguments.
- Added CSCALC `Mbps/Kbps/Gbps` transfer-rate parsing and `MiB` transfer-time conversion.
- Added CSCALC decimal fixed-point conversion with only fractional-bit count.
- Added CSCALC Boolean simplification for prose prompts containing constants `0` and `1`.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh probe batch exposed the gaps above; patched cases now pass.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 274908 bytes`
  - `CSCALC.g3a: 189820 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Probability, Circular Motion, SQL, And Float Guard Slice

Completed:
- Added CASP3 union/intersection/conditional probability route from `P(A or B)`.
- Added CASP3 mutually-exclusive union route.
- Added CASP3 complement conditional `P(A|B')` route.
- Added CASP3 binomial-test prose extraction for sample/success/proportion prompts and guarded it away from normal mean tests.
- Added CASP3 linear velocity total-distance split for `v=a+bt`.
- Added CASP3 circular/centripetal force route.
- Added CASP3 elastic string tension route.
- Added CASP3 projectile speed from maximum height route before generic projectile routes.
- Added CASP3 histogram class interval density route.
- Added CSCALC SQL SELECT prose route.
- Added CSCALC floating encode value/bit-width route and guarded representability questions.
- Added CSCALC unsigned binary plus/overflow route.
- Added CSCALC De Morgan Boolean proof shortcut.
- Added regressions and rebuilt calculator files.

Evidence:
- Second fresh probe batch passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 269756 bytes`
  - `CSCALC.g3a: 189236 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 P3/CScalc Free-Text Route-Order Hardening Slice

Completed:
- Added CASP3 smooth inclined-plane pull working so pull force is not mistaken for friction coefficient.
- Added CASP3 ladder equilibrium check with coefficient of friction before generic polar-equilibrium routing.
- Added CASP3 CDF `F(x)=kx^n` interval working.
- Added CASP3 descending linear pdf `k(a-x)` normalisation/mean/interval working.
- Added CASP3 combined `P(A or B)` plus independence working for probability prompts asking both.
- Added CASP3 two-number Poisson normal-approximation tail working.
- Added CSCALC memory address/data bus free-text working before binary-add fallback.
- Fixed CSCALC set-associative cache working to use sets, index bits, and tag bits.
- Added CSCALC Big-O working for common nested-loop prose.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probes for all above cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 263388 bytes`
  - `CSCALC.g3a: 187440 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Quartic Motion, K-Distributions, And CS Bit-String Slice

Completed:
- Added CASP3 quartic velocity displacement working.
- Added CASP3 quartic displacement total-distance working with stationary split points.
- Added CASP3 same-level projectile angle working from speed and range.
- Broadened CASP3 rough horizontal pull-at-angle routing without weakening existing projectile routes.
- Added CASP3 continuous `kx^n` pdf interval working for `P(a<X<b)`.
- Added CASP3 discrete `P(X=x)=kx` normalisation and expectation working.
- Fixed CSCALC bit-string two's-complement add/sub precedence over decimal fallback.
- Added CSCALC Caesar shift encrypt/decrypt working.
- Added regression tests and rebuilt calculator files.

Evidence:
- Focused fresh probes for the new P3/CS cases: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 257304 bytes`
  - `CSCALC.g3a: 184248 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Percentile, Poisson, CIDR, and Endian Slice

Completed:
- Added CASP3 inverse-normal free text for “value exceeded by x percent”.
- Hardened CASP3 unknown-normal-mean prompts from `P(X<k)=p` style wording.
- Added CASP3 Poisson interval routing and fixed “per minute/hour” default-duration parsing.
- Added CASP3 binomial `mean + variance -> n,p` working.
- Fixed CASP3 variable-force work for implicit multiplication like `F=5x-2` and `from x=... to x=...`.
- Added CASP3 connected rough-horizontal system working before single-particle rough-plane fallback.
- Added CSCALC CIDR subnet host-count and subnet-mask working.
- Added CSCALC endian byte-order working for hex words with all-digit hex values.
- Added CSCALC binary fractional decode from plain `binary ... to denary` wording.
- Added CSCALC floating-point representability/range check for normalised formats.
- Added CSCALC truth-table SOP routing for “sum of products for output ...” phrasing.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh focused probe batch: all new failing cases now produce working lines.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 238084 bytes`
  - `CSCALC.g3a: 178388 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Free-Text Stats Pdf, Mechanics, and CS Subtraction Slice

Completed:
- Added CASP3 variable-acceleration shorthand from `a=...`, `v=...`, and interval wording.
- Added CASP3 driving-force/resistance acceleration working.
- Added CASP3 uniform rod/beam reactions with self-weight plus multiple extra forces.
- Fixed CASP3 normal-approximation-to-binomial interval precedence before exact binomial summation.
- Added CASP3 continuous pdf normalisation/probability routes for `c*x*(a-x)` and `k(x+b)`.
- Added CASP3 raw Spearman rank working with average ranks for ties.
- Added CASP3 histogram class-width inverse working from frequency and density.
- Added CASP3 raw data quartile/IQR/fence/outlier working before older quartile fallbacks.
- Added CSCALC binary subtraction, two's-complement subtraction routing, and defensive bit-width clamping.
- Added CSCALC ISBN-13/EAN check digit working.
- Added CSCALC truth-table routing for `F = ...` assignment phrasing.
- Added regression tests and rebuilt calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 232792 bytes`
  - `CSCALC.g3a: 174916 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

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

## 2026-06-12 Polynomial Motion, CDF, Regression, and CS Rate Slice

Completed:
- Ran a fresh unseen probe batch across CASP3 mechanics/stats and CSCALC data/Boolean/storage prompts.
- Added CASP3 rough inclined-plane pulley working with coefficient-of-friction extraction.
- Added CASP3 projectile requested-height time solving with two roots where relevant.
- Added CASP3 polynomial-motion total-distance splitting at velocity sign changes.
- Added CASP3 cubic displacement stationary-point working with acceleration values.
- Added CASP3 inverse normal mean-from-probability working for unknown mean.
- Added CASP3 simple CDF median inversion for `F(x)=x^2/a` style prompts.
- Added CASP3 direct regression-line prediction from prose `y = mx + c`.
- Improved CSCALC signed fixed-point decode working detail.
- Added CSCALC decimal two's-complement subtraction from hyphenated prose.
- Added CSCALC network/file bit-rate parsing for GiB over minutes with Mbit/s display.
- Added CSCALC packet overhead percentage working.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probe failures from this slice now produce detailed working lines.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 208724 bytes`
  - `CSCALC.g3a: 169436 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3 sha256: 0b0c105a38dd3f6af13c31c2b8658b4728f8a6789dd5991ee895a3b64149b696`
  - `CSCALC sha256: fa18c545087fce6f0f8a282ddc39bf0f4ff5fc50bd0479ddbd2cddcf58b52401`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Discrete, Normal Test, Pulley, Boolean, Transfer Slice

Completed:
- Added CASP3 smooth inclined-plane pulley working from free-text connected-particle prompts.
- Added acceleration lines to velocity-polynomial displacement prompts when acceleration is requested.
- Fixed CASP3 normal sample-mean hypothesis-test prose so it does not route as a normal tail probability.
- Hardened CASP3 discrete-distribution parsing for split value/probability lists and `P(X=...)` wording.
- Fixed CSCALC Boolean proof parsing for `Use Boolean algebra to prove ...` wording.
- Fixed CSCALC NAND-form parsing for `Write A OR B using NAND gates only`.
- Added CSCALC modulo checksum prose route.
- Added minutes/hours conversion lines to CSCALC transfer-time output when requested.
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
  - `CASP3.g3a: 199056 bytes`
  - `CSCALC.g3a: 167556 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3 sha256: 0bb30acb1d0c02f7992e01d0c4eab29c9fa1f52a4dff40987c2c9eb3e37e8c7b`
  - `CSCALC sha256: 308c00ccf1da342880894b84a2ccb714f08aaa497b8af9ea8ad4700b388dad42`

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

## 2026-06-12 Tail Wording and Normal Bound Slice

Completed:
- Added shared `P(X<...)` / `P(X>...)` bound extraction for normal tail prompts.
- Fixed `no more than` tail detection so it no longer matches the embedded `more than`.
- Broadened coded-transform routing so prompts using `y=(x-a)/b` work even without the word `coded`.
- Added coded-transform direction detection for `find mean and sd of y`.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh final probe batch found three misses; all fixed:
  - coded transform without `coded`
  - `no more than` Poisson-approx tail
  - normal tail written as `P(X<93)`
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 194904 bytes`
  - `CSCALC.g3a: 166896 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 free-text parser hardening and generated calculator outputs.
- Did not touch CAS Pure behavior, CSCALC source, NOTES source, menus, or shared UI/status code.
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

## 2026-06-12 P3 Continuous Stats and CS Memory Parser Slice

Completed:
- Added CASP3 total-distance working from cubic displacement, including stationary split points.
- Added CASP3 maximum-displacement working from quadratic velocity by solving `v=0` and integrating.
- Added CASP3 variable-acceleration-to-rest working with distance to rest.
- Added CASP3 falling-against-resistance work-energy working.
- Added CASP3 continuous pdf handling for `kx` normalisation/mean and power-pdf tail probabilities.
- Fixed CASP3 grouped class median parsing for hyphenated intervals and restored grouped class mean routing after widening numeric scans.
- Added CASP3 PMCC summary-significance route using `Sxy/sqrt(Sxx*Syy)` before conditional-probability fallback.
- Added CSCALC cache block/index/offset bit working, AMAT working, ADC resolution working, and RLE decode working.
- Added KiB lines to CSCALC text/character storage output.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh focused probe batch: all previously failing P3/CS cases now produced working lines.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 218352 bytes`
  - `CSCALC.g3a: 172292 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text working support and generated calculator outputs.
- Did not touch CAS Pure behavior, NOTES source, menus, or shared UI/status code.
- Active goal remains open for future source/probe-driven hardening.

## 2026-06-12 Final Probability, Mechanics, and Float-Range Slice

Completed:
- Fixed CASP3 exact conditional-probability parsing when `P(A and B)` is given, including union and independence comparison.
- Added CASP3 independent-events free-text working from `P(A)` and `P(B)`.
- Added CASP3 without-replacement "both target" working for bag/ball prompts.
- Added CASP3 vector impulse from before/after velocity vectors.
- Added CASP3 work-energy braking stopping-distance working.
- Added CASP3 sample total from sample size and mean.
- Added CSCALC floating-point range pre-dispatch before broad two's-complement parsing.
- Checked app engines for obvious TODO/stub/placeholder clutter; none found outside legitimate runtime messages.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: conditional/union, independent events, without replacement, vector impulse, braking work-energy, sample total, max floating-point range.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 378100 bytes`
  - `CSCALC.g3a: 224832 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Kept changes to CASP3/CSCALC general free-text handling, regressions, generated calculator outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Momentum and Multi-Colour Probability Slice

Completed:
- Added CASP3 natural-language coalescence/stick-together collision routing to common-velocity conservation of momentum.
- Added CASP3 direct-collision routing where one particle is initially at rest and one final speed is given.
- Generalised CASP3 same-colour without-replacement probability from two colours to all detected colour groups.
- Preserved explicit combination working line shape for old two-colour outputs.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: coalescing particles, 3-colour same-colour probability, and collision with one particle initially at rest.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed after replacing calculator-unsupported `snprintf` with `sprintf`.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 379080 bytes`
  - `CSCALC.g3a: 224832 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Kept changes to CASP3 general free-text handling, regressions, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, shared UI/status code, or CSCALC source.

## 2026-06-12 Final Free-Text Cleanup Slice

Completed:
- Added CASP3 fair/unbiased coin free-text binomial probability working.
- Added CASP3 three-target without-replacement probability working with detected colour wording.
- Broadened CASP3 variable-force-over-time routing to natural `force (at+b)` wording.
- Added CASP3 slowing/deceleration over distance SUVAT working.
- Added CSCALC Boolean substitution route for prompts like `when A=1`.
- Kept the implementation narrow: no CAS Pure/menu/UI changes, no new app framework, no broad clutter.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: fair coin exact binomial, all-three red without replacement, variable force speed from rest, train slowing over distance, Boolean substitution.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 381204 bytes`
  - `CSCALC.g3a: 225528 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: 1324456e5fca6e925d5919c11c372dffde6ac061515b74c2a9c28adb11014ebe`
  - `CSCALC.g3a sha256: d32222e75ec16cab11840e48b94b9be77c0daf4cabcb8e687118f590098f5868`

Drift check:
- Kept changes to CASP3/CSCALC general free-text handling, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Summary Stats and Instruction Address Slice

Completed:
- Stopped the CASP3 generic sample-total route from stealing summary-statistics prompts with `sum x` and `sum x^2`.
- Added CASP3 recognition for pasted `sum x^2` summary notation.
- Added CSCALC instruction/opcode address-count working when the address-field width is implied.
- Added regression tests and rebuilt calculator-ready outputs.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 504868 bytes`
  - `CSCALC.g3a: 280756 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: fa5c4ad10e1b98ea6a646b171c28fc43af781740f2c4a83be382a56aa0a82feb`
  - `CSCALC.g3a sha256: 6e913cd6a7cde40bf93c0bf5cfbb8aff0250da1913e2aa7e28ad377d6e491657`

Drift check:
- Kept changes to CASP3/CSCALC general free-text handling, regressions, generated CASP3/CSCALC outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Mechanics Parser Finalisation Slice

Completed:
- Hardened generic ladder parsing so `5m long ... 60 degrees ... weight 200N` uses length, angle, and weight correctly instead of raw number order.
- Added downhill constant-speed power-angle working with downhill equilibrium wording.
- Added regression tests and rebuilt calculator-ready outputs.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 505924 bytes`
  - `CSCALC.g3a: 280756 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 90ae77f02d6b2e83a8a7f50c174405076da9e1aff4e95db3432a64895850cbe4`

Drift check:
- Kept changes to CASP3 general free-text mechanics parsing, regressions, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, shared UI/status code, or CSCALC source in this slice.

## 2026-06-13 Superscript, Not-Equal, Minute, and Gate Assignment Slice

Completed:
- Added CASP3 pasted notation normalization for `≠`, `²`, and `³`.
- Reused the existing CASP3 not-equal complement route for `P(X≠r)`.
- Fixed normal variance forms like `10²` so `sigma = sqrt(100)`, not `sqrt(10)`.
- Treated standalone `min` as minutes in CSCALC time scaling and sound free-text duration parsing.
- Routed named Boolean outputs like `F=A∨B` through the RHS before NAND/NOR conversion.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: `P(X≠6)`, `X~N(50,10²), P(X≤65)`, `44.1 kHz, 16 bit, stereo, 3 min`, `F=A∨B to NAND only`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `CASP3.g3a: 504036 bytes`
  - `CSCALC.g3a: 278036 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: 2b5eb5703a18972c2b18538be513cbed90578f1475f4055633e828b7e832f279`
  - `CSCALC.g3a sha256: 2f32ee8dbe9ace1f4ce3704c4e311a33423d55730f4420294be8137b17035b23`

Drift check:
- Kept changes to CASP3/CSCALC parser generalisation, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Subscript Base and Variable-Acceleration Slice

Completed:
- Found old Boolean Python scripts in git history: `boolean.py`, `ComputerScience/boolean.py`, and `ComputerScience/booleanProgram.py`; current CSCALC already has the Boolean core in C++.
- Added generic CSCALC subscript-base token parsing for pasted `...₂`, `...₈`, `...₁₀`, and `...₁₆` values.
- Routed subscript-base values through existing base-conversion and two's-complement decode engines.
- Fixed CASP3 variable-acceleration fallback so `initial velocity ...` is not confused with `at t=...`.
- Prevented that older velocity-only fallback from stealing displacement prompts that need both `v` and `s`.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: `10101111₂ to hexadecimal`, `255₁₀ to hex`, `377₈ to binary`, `F3₁₆ as 8-bit two's complement`, and variable acceleration with `initial velocity 5 ... at t=3`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `CASP3.g3a: 504800 bytes`
  - `CSCALC.g3a: 279956 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: 11e6c018cf02cfaa8db6f8ce3409c16a4eec58a5b1b1f61cc9cba5969067f3e9`
  - `CSCALC.g3a sha256: d9a02351c5bfb2411d1567e3797dfaee24232b75e11671e34db7e5ba3d98161e`

Drift check:
- Kept changes to CASP3/CSCALC parser generalisation, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Resistance, Power, and Subscript Base Slice

Completed:
- Fixed CASP3 rough-horizontal force parsing so explicit resistance is used when no coefficient of friction is supplied.
- Fixed CASP3 constant-speed power prompts phrased as `moves at ... m/s against resistance ...`.
- Added CSCALC Unicode subscript digit normalization and base-16 suffix handling for prompts like `AF₁₆`.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: rough horizontal `force 20N, resistance 5N`, constant-speed power `12 m/s against resistance 300N`, and `AF₁₆ to binary`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `CASP3.g3a: 504280 bytes`
  - `CSCALC.g3a: 278536 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: 4a6acccf8363a4ca28fa79b6e585dbcf8b889ff6b52fc5614fc200c077c01bc4`
  - `CSCALC.g3a sha256: 3ab8e240c8d0de706675ca615a6687461222d8c65089ec14337cb7958b614c8c`

Drift check:
- Kept changes to CASP3/CSCALC parser generalisation, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Pasted Symbol and Boolean Truth Slice

Completed:
- Added CASP3 parser-boundary handling for pasted projectile notation:
  - Unicode minus/dash in unit exponents such as `m s−1`.
  - Degree symbol `°`, including direct `30°` parsing through existing `degrees` routes.
- Added CSCALC parser-boundary handling for Unicode minus/dash in negative values.
- Cleaned CSCALC Boolean truth-table mode so it no longer prints algebra-law simplification noise before truth tables.
- Removed the narrow XOR truth-table shortcut; all truth-table Boolean expressions now use the general Boolean parser.
- Added regressions for pasted projectile notation, Unicode negative two's complement, and 3-variable Unicode XOR truth table.

Evidence:
- Fresh probes fixed:
  - `A particle is projected with speed 20 m s−1 at 30° above the horizontal. Find range.` now uses `u_x = 20 cos 30` and `range = 35.3478`.
  - `Convert −13 to 8-bit two’s complement.` now encodes `-13 -> 11110011`.
  - `Truth table for A ⊕ B ⊕ C.` now starts with the truth table, not noisy simplification expansion.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 503972 bytes`
  - `CSCALC.g3a: 277756 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 281f41a228a9c97216706933ef6cc40c707b2acb275320b0ab3aa2c3b1754cd7`
  - `CSCALC.g3a sha256: c9deb8e96e965cca322fe11ceb1d9679e6ff94fe8134c528cd698febefecde4e`

Drift check:
- Kept changes to CASP3/CSCALC parser generalisation, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure source, menus, NOTES source, or shared UI/status code.

## 2026-06-13 CASP3 Free-Text Normalisation Slice

Completed:
- Added a normalised human-text buffer for CASP3 free-text parsing.
- Free-text routes now receive pasted math symbols normalised before helper parsing, instead of only direct command parsing.
- This fixes prose values such as Unicode negative initial velocity without patching each helper separately.
- Added regression coverage for Unicode-negative SUVAT prose.

Evidence:
- Fresh probe fixed:
  - `constant acceleration initial velocity −5 acceleration 2 time 3 find final velocity` now gives `v = -5 + 2*3` and `v = 1`.
- Existing pasted projectile probe still passes:
  - `A particle is projected with speed 20 m s−1 at 30° above the horizontal. Find range.`
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 503968 bytes`
  - `CSCALC.g3a: 277756 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 2f2b0a0bdca8eef9cf47e1cb37ccb58a29d8deb7ec4477b75fcadd58e187320a`

Drift check:
- Kept changes to CASP3 parser generalisation, regression, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure source, menus, CSCALC source, NOTES source, or shared UI/status code.

## 2026-06-13 CASP3 Pasted Maths Notation Slice

Completed:
- Added CASP3 parser-boundary UTF-8 normalisation for pasted exam notation:
  - `<=`/`>=` equivalents from `≤`/`≥`.
  - Greek parameter labels: `μ`/`µ`, `σ`, `λ`.
  - `sqrt` equivalent from `√`.
- Fixed cumulative/tail routing for binomial, Poisson, and normal prompts using pasted symbols.
- Added regression coverage for exact binomial tails, Poisson tails, normal tails, Greek labels, and mixed interval notation.

Evidence:
- Fresh probes fixed:
  - `For X~B(20,0.35), find P(X≤6).` gives cumulative `P(X<=6)` and `sum from 0 to 6`.
  - `For X~B(20,0.35), find P(X≥6).` gives tail `P(X>=6)` and `sum from 6 to 20`.
  - `For X~Po(3.2), find P(X≤5).` gives cumulative Poisson working.
  - `Normal μ=50 σ=10 find P(X≤65).` gives normal standardisation and `NormalCD` setup.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 503672 bytes`
  - `CSCALC.g3a: 277832 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 4740d35a578bdb31d748f3843b38ef3efdb6aec68bf1d634f958f1fad9677e79`

Drift check:
- Kept changes to CASP3 input normalisation, regressions, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure source, menus, CSCALC source, NOTES source, or shared UI/status code.

## 2026-06-13 CSCALC Boolean Paste-Notation Slice

Completed:
- Compared current CSCALC Boolean engine against the archived Python Boolean program.
- Kept current engine: it already supports more operators, truth tables, K-map/minterms, POS, NAND/NOR, proofs, and law traces than the old Python script.
- Added generic Boolean UTF-8 operator normalisation for pasted exam/notes notation:
  - `¬` -> NOT
  - `∧`, `·`, `⋅`, `×` -> AND
  - `∨` -> OR
  - `⊕` -> XOR
  - `→`, `⇒` -> implication
- Applied the same normalisation to command forms, free-text forms, and NAND/NOR conversion parsing.
- Added regression coverage for bare pasted Boolean expressions, truth tables, proofs, implication, and NAND conversion.

Evidence:
- Fresh probes fixed:
  - `¬(A∨B)`
  - `Simplify Boolean expression ¬(A∨B).`
  - `Construct truth table for A⊕B.`
  - `Prove A∧(B∨C)=A∧B∨A∧C`
  - `A∧B ⇒ A`
  - `convert A∨B to NAND only`
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 503264 bytes`
  - `CSCALC.g3a: 277832 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CAS.g3a sha256: 15f3aa342df2f8e9f3608032a17e116254df92d19141c271e45553de0b01a39e`
  - `CASP3.g3a sha256: ed2e4fa9a06c17dc36d00c52ef2426ccddb0266bbad1e4b2e9b9aa4fdf68896e`
  - `CSCALC.g3a sha256: a3682994f094c39985c520dcd18fc9725235847bd993497f3c727f877413229b`

Drift check:
- Kept changes to CSCALC Boolean input normalisation, regressions, generated CSCALC output, and this checkpoint.
- Did not touch CAS Pure source, menus, CASP3 source, NOTES source, or shared UI/status code.

## 2026-06-13 Rough Plane, Linear PDF, and Sound Unit Slice

Completed:
- Stopped generic SUVAT deceleration routing from stealing rough/inclined/friction plane questions.
- Shortened rough-plane first line to avoid calculator-width truncation.
- Extended bare linear PDF parsing (`f(x)=kx`) to find `k` and requested probabilities such as `P(X<2)`.
- Added CSCALC raw unit scanning so attached units like `44.1kHz`, `16 bit`, and `2 minutes` parse correctly.
- Added regressions for rough-plane stopping distance, linear PDF probability, and stereo sound file size in MiB.

Evidence:
- Fresh probes fixed: rough-plane projected stopping distance, bare `kx` PDF probability, and attached-unit sound storage.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `git diff --check`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 500796 bytes`
  - `CSCALC.g3a: 276356 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CAS.g3a sha256: 15f3aa342df2f8e9f3608032a17e116254df92d19141c271e45553de0b01a39e`
  - `RUNMAT.g3a sha256: 084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
  - `CASP3.g3a sha256: da7322fa15fadcf61c5646ab8cfe2652a464a441228e741a1775ce41472fe26f`
  - `CSCALC.g3a sha256: d101773461be7a96d38dbafa48d4c96540759168b325227501bba4224e0b46d8`
  - `NOTES.g3a sha256: 203cf9cf42777fdab91639e9791b5ce202478fc54471e0addf68add34b5745e6`
  - `CAS.PAK sha256: 4cf970de9480f9f3b06c80e60afe4b69f4d864067a068071cd974474a57f24d2`

Drift check:
- Kept changes to CASP3/CSCALC free-text handling, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure source, menus, NOTES source, or shared UI/status code.

## 2026-06-13 CSCALC Attached Size/Rate Unit Slice

Completed:
- Added bounded raw unit scanning for attached file-size units such as `1.5GB`, `40MiB`, and `750KB`.
- Reused that scanner in generic storage/bitrate/compression helpers instead of one-off command routes.
- Added attached-rate scanning for `Mbps/Gbps/Kbps` style prose in file-size-from-rate and transfer paths.
- Preserved exam-style conversion lines such as `12 MB = 96 Mbit` and compact `Mbit/s` substitution.
- Added regressions for attached `GB` bitrate and attached `Mbps` stream-size prompts.

Evidence:
- Fresh probes fixed:
  - `A network sends 1.5GB in 2 minutes. Find bitrate in Mbps.`
  - `A video stream has bitrate 5Mbps for 45 seconds. Find file size in MB.`
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `git diff --check`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 500796 bytes`
  - `CSCALC.g3a: 277456 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CSCALC.g3a sha256: 80ed2f1cf52713948dabd0284534f82dd1006fd3552dc02692cf15054f16b210`

Drift check:
- Kept changes to CSCALC free-text unit handling, regressions, generated CSCALC output, and this checkpoint.
- Did not touch CAS Pure source, CASP3 source, menus, NOTES source, or shared UI/status code.

## 2026-06-13 P3 Vector, Collision, and Cumulative Table Slice

Completed:
- Stopped `rest` inside `restitution` from triggering the wrong collision-at-rest route.
- Added prose mapping for collision questions where masses are listed first and speeds second.
- Added linear position-vector working for `r=r0+t v`, including position at a time and speed.
- Added cumulative-probability table handling by differencing successive cumulative probabilities.
- Added regressions for restitution collisions, position vectors, and cumulative probability tables.

Evidence:
- Fresh probes fixed:
  - direct collision with coefficient of restitution and masses/speeds prose
  - `r=(2i+3j)+t(4i-j)` position/speed
  - cumulative probability table with `P(X=2)` and `E(X)`
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `git diff --check`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 503264 bytes`
  - `CSCALC.g3a: 277456 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: ed2e4fa9a06c17dc36d00c52ef2426ccddb0266bbad1e4b2e9b9aa4fdf68896e`

Drift check:
- Kept changes to CASP3 free-text handling, regressions, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure source, CSCALC source, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Transfer-Time and Rough-Pulley Slice

Completed:
- Fixed CSCALC prose routing so `sent over 100 Mbps ... find time` is treated as transfer time, not bit-rate.
- Preserved explicit bit-rate and overhead transmission routes by excluding those from the new early transfer guard.
- Fixed CASP3 pulley rough-table prose so `table rough coefficient ...` includes friction even without explicit `hangs freely` wording.
- Added regressions for both failures and rebuilt changed calculator files.

Evidence:
- Fresh probes fixed:
  - `A file of 25 MiB is sent over 100 Mbps network. Find time in seconds.`
  - `Two particles 2kg and 7kg connected over a pulley, table rough coefficient 0.3. Find acceleration and tension.`
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 499468 bytes`
  - `CSCALC.g3a: 275836 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 55b0babb0f7e21407672b13feccaf2ef2bd482b3ea0f0cf019655b79d57c2b74`
  - `CSCALC.g3a sha256: 60ac097070ff264d9ac1c123232f5f955a6859e09d2290e82beecc8adf697a04`

Drift check:
- Kept changes to CASP3/CSCALC generic free-text routing, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Uphill Power Routing Slice

Completed:
- Fixed CASP3 incline/slope power routing so `Find power` with mass, slope angle, resistance, and speed computes `Power = Fv`.
- Preserved the separate route for finding slope angle from known engine power.
- Added regression coverage and rebuilt changed calculator files.

Evidence:
- Fresh probes fixed:
  - `A car of mass 1200kg moves up a 5 degree slope with resistance 400N at speed 12m/s. Find power.`
  - Existing angle-from-power wording still routes correctly.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 499512 bytes`
  - `CSCALC.g3a: 275836 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: c4c7cfc9c8ab003eecd58ebf46551ec24a9b12c54fff7980bf0a3868478b3c0c`
  - `CSCALC.g3a sha256: 60ac097070ff264d9ac1c123232f5f955a6859e09d2290e82beecc8adf697a04`

Drift check:
- Kept changes to CASP3 free-text routing, regression, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, CSCALC source, or shared UI/status code.

## 2026-06-12 Two's Complement, Word Addressing, and Inverse Binomial Slice

Completed:
- Added CSCALC free-text two's-complement negative encoding for wording like `Represent -37 in 8 bit two's complement`.
- Fixed CSCALC word-addressed capacity parsing so `4 byte words` means 4 bytes per address, not 4 bits.
- Added CASP3 inverse-binomial coin route for unknown `p` from `P(exactly r heads)=...`, including multiple valid roots.
- Added regressions and rebuilt calculator files.

Evidence:
- Fresh probes fixed: negative two's-complement representation, byte-word address capacity, inverse binomial unknown `p`.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 432416 bytes`
  - `CSCALC.g3a: 240204 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: ef382909711e49315e6048534474fe0f7197ddea202bca119fe71166194294d7`
  - `CSCALC.g3a sha256: be003739e3c7744b3ba7907df0bfe9bd69976daa3526f6f61cac47d8a93a9feb`

Drift check:
- Kept changes to CASP3/CSCALC free-text handling, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Powered Slope, Vertical Height, and Vector Slice

Completed:
- Fixed CASP3 powered slope acceleration so `5 degrees at 15 m/s` uses angle and speed correctly.
- Added CASP3 vertical greatest/maximum-height working from launch height above ground.
- Added CASP3 polar resultant force resolution by resolving each force into components.
- Rechecked current CSCALC Boolean simplification/proof/NAND/NOR behavior against the old Python reference logic; no source change needed.

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
  - `CASP3.g3a: 430220 bytes`
  - `CSCALC.g3a: 239612 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: b25c8ce537119b2327f4a86a1ffd128202f5c665ebd83495d72bc1840aa415dc`

Drift check:
- Kept changes to CASP3 general free-text handling, regressions, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, shared UI/status code, or CSCALC source in this slice.

## 2026-06-12 Final Parser Simplification Slice

Completed:
- Fixed CASP3 normal mean-test prose so `sample mean ... exceeds 40` uses the sample mean as `xbar`, the comparison value as `mu`, and an upper-tail test.
- Fixed CASP3 projectile prose with launch height before speed/angle, including range and greatest-height working from the same route.
- Added CASP3 momentum-only collision routing when one final velocity is given and no restitution coefficient is supplied.
- Fixed CSCALC image metadata/header units so `2 MiB metadata` is converted to bytes and reported in MiB when requested.
- Added regressions for each route and rebuilt calculator outputs.

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
  - `CASP3.g3a: 421800 bytes`
  - `CSCALC.g3a: 236900 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: 96138b96aa524014c3c71862d94a54c25dde331baa4473fd877528c16b7449b1`
  - `CSCALC.g3a sha256: 74840567dafe0f36c7dc12bb449c3360b01346819b6a64c26672e160db62547b`

Drift check:
- Kept changes to parser hardening, regressions, generated CASP3/CSCALC outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Final Precedence Hardening Slice

Completed:
- Fixed CASP3 rough inclined-plane projection-to-rest prompts so they route to inclined-plane SUVAT, not projectile motion.
- Fixed CASP3 maximum-speed power/resistance prompts so `v=P/R` is used instead of treating power as a speed.
- Fixed CASP3 uniform rod/beam load-at-end support reactions with midpoint rod weight.
- Stopped CASP3 conditional-probability fallback from stealing regression estimate wording with `given`.
- Fixed CSCALC denary binary-fraction conversion with requested bit width.
- Fixed CSCALC combined floating-point bit-string decode before normalisation checks.
- Moved CSCALC RLE size routing before generic compression-ratio fallback.
- Added De Morgan law wording before generic Boolean truth-table proof.

Evidence:
- Fresh probes fixed: rough plane projected-up stopping distance, max speed from power/resistance, supported rod load at end, regression estimate with `given`, 8-bit binary fraction, combined float decode, RLE compressed size, De Morgan proof wording.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 420704 bytes`
  - `CSCALC.g3a: 236196 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: d14adcc6a4ff18d0c8dcc1413b83578d7aa13cb53ee8e648b9d09038ad7f52bf`
  - `CSCALC.g3a sha256: f87f35d86d084043edfdd2a3b840df76f4a1b552c6870510007866dcbc085a14`

Drift check:
- Kept changes to CASP3/CSCALC general precedence parsing, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Final Probe Hardening Slice 2

Completed:
- Added CASP3 limiting-force route for rough inclined planes, so least-force prompts produce equilibrium working instead of acceleration.
- Added CASP3 flat constant-speed power route using `P=Fv`, avoiding the generic work/time fallback.
- Hardened CASP3 binomial hypothesis routing so one-tailed tests with no stated direction use the observed side of `np`.
- Improved `hypbinom` working lines to explicitly state `P(X>=x)` or `P(X<=x)`.
- Added CSCALC shared fixed-point width parsing for `N-bit fixed point with F bits after the point`.
- Added CSCALC regression coverage for POS output-column prompts.

Evidence:
- Fresh probes fixed: rough plane least force, flat resistance power, `X~B(20,0.5)` observed-side one-tailed test, signed fixed point total/fractional bit wording, POS truthbits output.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 416780 bytes`
  - `CSCALC.g3a: 233840 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 46b560b6ae80d0f235e5bb800f2aa9309d7c3ade858861b4eaa79c7b32c56d60`
  - `CSCALC.g3a sha256: 68cb3c3a0840b770fa48ac7147be6375c308861e2e9932f285383dffb4923402`

Drift check:
- Kept changes to CASP3/CSCALC general free-text handling, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Final Simplification and Generic Route Slice

Completed:
- Removed a duplicate CASP3 summary-statistics branch so summary routes have one owner.
- Made CASP3 variable-acceleration prose assume `s(0)=0` when displacement is requested but no initial displacement is given.
- Preserved interval displacement wording for the existing dedicated interval route.
- Added CASP3 constant-speed down-slope braking equilibrium working.
- Added CSCALC half-adder, full-adder, signed-overflow truth-table, and sigma-minterm free-text handling before generic Boolean parsing.
- Rebuilt calculator-ready CASP3 and CSCALC add-ins.

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
  - `CASP3.g3a: 423852 bytes`
  - `CSCALC.g3a: 238516 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a sha256: 1e0da61652d3664ad335dd66669658df5ef8dd7072623cb144d99b72b5c39f76`
  - `CSCALC.g3a sha256: f1a6027bafd3d2bf456770acaf77538af8f561826460b9994c1b0c67b382a5a2`

Drift check:
- Kept changes limited to CASP3/CSCALC engines, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Urgent Generic Gap Slice

Completed:
- Fixed CASP3 offset-support rod reactions using signed moments about support A.
- Fixed `X~B(...)` prose so explicit Poisson approximation requests do not fall through to exact binomial output.
- Added CSCALC unused address-line working from address bus width and memory size.
- Added regressions and rebuilt calculator-ready outputs.

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
  - `CASP3.g3a: 425348 bytes`
  - `CSCALC.g3a: 239612 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a sha256: 066caa1c32a6e48d81f9187bde1041b67ee89db24bde746d1ed4d032f55324a6`
  - `CSCALC.g3a sha256: eb6468dac2ed2892f70d6640725fd8697ad30ceb15d3e51876888aac303feb9b`

Drift check:
- Kept changes limited to CASP3/CSCALC generic route hardening, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Final Generalisation Cleanup Slice

Completed:
- Added CASP3 towing/trailer system handling with one-system acceleration and trailer-alone tension lines.
- Added CASP3 exact-two-without-replacement combination handling before broader all-three routing.
- Hardened exact-two selection so the requested colour is used, including `exactly 2` digit wording.
- Added CASP3 summary-statistics `n`, `Sx`, `Sx2` mean/variance routing before loose total-from-mean routing.
- Fixed CSCALC reverse image colour-depth/bits-per-pixel routing for file-size plus resolution prompts.
- Fixed CSCALC compound XOR truth-table routing so trailing `AND`/`OR` expressions stay intact.
- Checked maintained app sources for obvious TODO/FIXME/placeholder/stub clutter.

Evidence:
- Fresh probes fixed: trailer acceleration/tension, exactly-two red/blue without replacement, summary-statistics variance, image bits-per-pixel, compound XOR truth table.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 411928 bytes`
  - `CSCALC.g3a: 232124 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 6050a3ee1f43214ccd8ab8c1b772f44f61871ea60995f356c473c9ddc13d37d5`
  - `CSCALC.g3a sha256: e592b2cc7f1d460707c190469b15b0192c114350d1c729180edfc2d6aee1a49f`

Drift check:
- Kept changes to CASP3/CSCALC general free-text handling, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 Poisson Union, Inverse Coin, Reciprocal Median, and POS Slice

Completed:
- Added CASP3 disjoint Poisson tail-union working for prompts like fewer than one bound or greater than another.
- Added CASP3 inverse no-heads coin/binomial parameter working from `P(no heads)`.
- Added CASP3 reciprocal-pdf median working for `k/(x+a)` style densities after finding `k`.
- Fixed CSCALC Boolean product-of-sums tail cleanup so `product of sums` wording is not parsed as variables.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: Poisson split tails, inverse biased coin probability, reciprocal pdf median, Boolean POS conversion.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 407888 bytes`
  - `CSCALC.g3a: 231752 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CASP3.g3a sha256: 1295ec8896cc35b1fd9ca197f483573d9abac78f93a52165f904a0859d3b628a`
  - `CSCALC.g3a sha256: ae202d90fe199b056f83deed9d5fdee50bbed3bea050f4cfb946d2925e4caf4f`

Drift check:
- Kept changes to CASP3/CSCALC general free-text handling, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-12 P3 Route Finalisation Slice

Completed:
- Hardened CASP3 prose parsing for variable acceleration with labelled initial/target times.
- Fixed polynomial extraction so words like `particle` do not start a fake `t` expression.
- Added compact-unit projectile speed parsing for forms like `20m/s`.
- Added loaded-ladder equilibrium working with person/load distance parsing.
- Stopped horizontal driving-force routing from stealing slope resistance questions.
- Added inclined-plane resistance/braking acceleration handling.
- Added reciprocal-quadratic trapezium rule support for `1/(1+x^2)`.
- Checked maintained app tooling for obvious TODO/FIXME/placeholder clutter; no app-engine clutter found.

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
  - `CASP3.g3a: 399460 bytes`
  - `CSCALC.g3a: 230584 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: 453555d832e1c6fc8d9314d03331c1ad387f1c41faf2435eaa344a7251506e7a`

Drift check:
- Kept changes to CASP3 general free-text handling, regressions, generated CASP3 output, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, shared UI/status code, or CSCALC source in this slice.

## 2026-06-12 Bayes, Interval, Variance, Address, and Image Slice

Completed:
- Added CASP3 total-probability/Bayes routing for prompts with `P(B|A)` and `P(B|not A)`.
- Added shared CASP3 prose interval bounds for binomial count wording such as `at least ... but fewer than ...`.
- Extended CASP3 add/remove-one-value mean updates to also update variance from known variance.
- Fixed CASP3 rough-plane force direction wording/sign for forces explicitly down the plane.
- Added CASP3 `P(neither)` output for independent/mutually-exclusive union prompts.
- Added CSCALC address-bus capacity routing before broad address-bits fallback.
- Added CSCALC image colour-count routing from file size in bits and pixel dimensions.
- Added regression tests and rebuilt calculator files.

Evidence:
- Fresh probes fixed: Bayes total probability, bounded binomial coin interval, mean+variance after adding a value, down-plane rough force, independent `neither`, address-bus capacity, image colours from file bits.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 385028 bytes`
  - `CSCALC.g3a: 227416 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CASP3.g3a sha256: d6ce4211203d4a6a2e186fb6c2f9eca3b140a2f60e6316bc6380d0189f43b175`
  - `CSCALC.g3a sha256: ee94c519d742a5d6690d0a2faad839114f83107c81aa61bc1c1c9c8579e8d338`

Drift check:
- Kept changes to CASP3/CSCALC general free-text handling, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Free Text Wrong Answer Hardening Slice

Completed:
- Fixed CASP3 rough-horizontal force parsing so `50N force at 30 degrees` is not confused with mass, angle, or words ending in `n`.
- Added CASP3 rough-plane return-to-start working before broad projectile routing.
- Added CASP3 right-tail normal parameter parsing for prompts such as `P(X>75)=0.2`.
- Added CASP3 unbiased summary-variance working from `sum x` and `sum x^2` prompts.
- Added CASP3 inverse-proportional discrete distribution working for `P(X=x)=k/x`.
- Added CSCALC one's-complement encode/decode working.
- Fixed CSCALC check-digit parsing when weights are written before digits.
- Added CSCALC Huffman code-table encode working for prompts with explicit symbol codes.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 509272 bytes`
  - `CSCALC.g3a: 282904 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CAS.g3a sha256: 15f3aa342df2f8e9f3608032a17e116254df92d19141c271e45553de0b01a39e`
  - `RUNMAT.g3a sha256: 084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
  - `CASP3.g3a sha256: 3c178a80fbe711fd3bcfd8b9cbb2514a60c35f69278ee0f2fd6a303be68235e8`
  - `CSCALC.g3a sha256: 5c9870c0b13395b6c4bd4233f81fdcc2f90ca640da21f2b28246b8ca852486be`
  - `NOTES.g3a sha256: 203cf9cf42777fdab91639e9791b5ce202478fc54471e0addf68add34b5745e6`
  - `CAS.PAK sha256: 4cf970de9480f9f3b06c80e60afe4b69f4d864067a068071cd974474a57f24d2`

Drift check:
- Kept changes to CASP3/CSCALC parser/working logic, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Chi-Square, Parity, and Vigenere Hardening Slice

Completed:
- Added CASP3 chi-squared goodness-of-fit working from observed/expected lists.
- Added CASP3 2x2 contingency chi-squared independence working before probability-independence fallback.
- Fixed CSCALC Vigenere free-text parsing so `with key ...` treats the following word as the keyword.
- Added CSCALC parity validation when a received parity bit is supplied.
- Fixed chi-square significance parsing so table cells like `10` are not mistaken for `10 percent`.
- Added regressions and rebuilt calculator outputs.

Evidence:
- Probe-fixed cases: CASP3 chi-squared goodness-of-fit, CASP3 contingency table independence at 5 percent, CSCALC odd parity validation, CSCALC Vigenere decrypt/encrypt.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 517948 bytes`
  - `CSCALC.g3a: 285276 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CAS.g3a sha256: 15f3aa342df2f8e9f3608032a17e116254df92d19141c271e45553de0b01a39e`
  - `RUNMAT.g3a sha256: 084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
  - `CASP3.g3a sha256: d3f49078700ebafe983776251fab8dabc09e849a843dca6aec9dab789f3917f6`
  - `CSCALC.g3a sha256: 26c5dec8e97ef5de746d32e4a451f806aa208e404e8a062a7f5544fb6291402a`
  - `NOTES.g3a sha256: 203cf9cf42777fdab91639e9791b5ce202478fc54471e0addf68add34b5745e6`
  - `CAS.PAK sha256: 4cf970de9480f9f3b06c80e60afe4b69f4d864067a068071cd974474a57f24d2`

Drift check:
- Kept changes to CASP3/CSCALC parser/working logic, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.

## 2026-06-13 Vector, Normal, Boolean, and Encoding Free Text Slice

Completed:
- Added generic CASP3 polynomial vector parsing for `r=(... )i+(... )j` and `v=(... )i+(... )j` free text.
- Stopped scalar displacement calculus from stealing vector displacement questions.
- Added CASP3 constant-speed then acceleration/deceleration staged journey working.
- Stopped die/normal-approximation wording from being routed as projectile motion.
- Added known-parameter normal probability parsing so stated `mean/sd` cases do not become unknown-parameter solves.
- Added CSCALC one’s-complement range working.
- Added CSCALC Vigenere cipher encrypt/decrypt working.
- Hardened CSCALC truth-table output-column/maxterm routing so `variables A B` is used instead of words from the prompt.
- Added regressions for the fixed free-text variants and rebuilt calculator outputs.

Evidence:
- Probe-fixed cases: CASP3 vector derivative/speed, velocity-vector integration to position, staged journeys, die normal approximation, known-parameter normal tails.
- Probe-fixed cases: CSCALC explicit weighted check digit, plain-text RLE, Vigenere, one’s-complement range, maxterms from output bits.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2087936 bytes`
  - `RUNMAT.g3a: 30216 bytes`
  - `CASP3.g3a: 513380 bytes`
  - `CSCALC.g3a: 284620 bytes`
  - `NOTES.g3a: 46952 bytes`
  - `CAS.PAK: records=48 bytes=18178`
  - `CAS.g3a sha256: 15f3aa342df2f8e9f3608032a17e116254df92d19141c271e45553de0b01a39e`
  - `RUNMAT.g3a sha256: 084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
  - `CASP3.g3a sha256: 2ab85f2112c0052f0d8c300272151b8c50873e93a4845c535eb9e095f76e0037`
  - `CSCALC.g3a sha256: 353fa0eb76b830ce8095df01b96c20e8b6fe153caff965e930d9e178f577980f`
  - `NOTES.g3a sha256: 203cf9cf42777fdab91639e9791b5ce202478fc54471e0addf68add34b5745e6`
  - `CAS.PAK sha256: 4cf970de9480f9f3b06c80e60afe4b69f4d864067a068071cd974474a57f24d2`

Drift check:
- Kept changes to CASP3/CSCALC parser/working logic, regressions, generated app outputs, and this checkpoint.
- Did not touch CAS Pure behavior, menus, NOTES source, or shared UI/status code.
