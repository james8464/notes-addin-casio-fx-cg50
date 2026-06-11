# Checkpoint

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
