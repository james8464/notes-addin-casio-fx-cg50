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
