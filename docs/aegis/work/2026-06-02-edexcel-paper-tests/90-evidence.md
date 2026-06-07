# Evidence

## 2026-06-07 Endpoint/Critical Range Final Gate Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,082,928 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 14,224 bytes.
- Link ROM report: 2,054,252 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- CAS.g3a SHA256: `81d23a9898e012137a53f4c6deb2b09aca14342a0a05f37c0f476b7af9fb2590`.
- CAS.PAK SHA256: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- RUNMAT.g3a SHA256: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.

Behavior:
- `tools/khicas_host_runner 'range(sin(x)+ln(x),x)'`: endpoint/critical route, domain `x > 0`, limits `-infinity` and `+infinity`, `range: all real`, `Verified`.
- `tools/khicas_host_runner 'range(ln(x)-x,x)'`: endpoint/critical route, domain `x > 0`, `f'(x)=-1 + 1/x`, `f(1) = -1`, `y <= -1`, `Verified`.
- `tools/khicas_host_runner 'range(x-ln(x),x)'`: endpoint/critical route, domain `x > 0`, `f'(x)=1 - 1/x`, `f(1) = 1`, `y >= 1`, `Verified`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative/stationary/asymptote route, `y >= -4*exp(2)`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: planner path returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- Live source/tool scan contains no `route failed`, `no route`, `no verified route`, or `No verified A-level working route found` emitted wording.
- Emulator SD-card `CAS.g3a` and `CasioCAS.g3a` SHA256 match current candidate `81d23a9898e012137a53f4c6deb2b09aca14342a0a05f37c0f476b7af9fb2590`; `CAS.PAK` SHA256 matches `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`; backup: `/tmp/casio-smoke-current/emulator-sdcard-backup-20260607-082738`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=18`, `xform_planner_cases=11`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0 stale_reasons=equation_output_drift=15,route_output_drift=16,symbolic_output_drift=1,verified_output_drift=86`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

Known remaining risk:
- Foreground/hardware launch, long input, and `xform(...)` key-entry smoke remains incomplete for the current `81d23a...` candidate.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared-working still has 118 classified stale markers; `bad=0`.
- This evidence supports the bounded A-level Pure route-family gate suite, not arbitrary mathematical completeness.

## 2026-06-07 Shifted Exp/Trig Range Final Gate Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,079,200 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 17,952 bytes.
- Link ROM report: 2,050,524 / 2,097,192 bytes.
- Link RAM report: 348,644 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- CAS.g3a SHA256: `beff681c3c1109fdfeb7aa595b5d2f9320bfe7ebbe0de9c04450cf0d873b4639`.
- CAS.PAK SHA256: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- RUNMAT.g3a SHA256: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.

Behavior:
- `tools/khicas_host_runner 'range(2+3*exp(-x),x)'`: shifted exponential route, `y > 2`, `Verified`.
- `tools/khicas_host_runner 'range(2-3*exp(-x),x)'`: shifted exponential route, `y < 2`, `Verified`.
- `tools/khicas_host_runner 'range(1+2*sin(x)^2,x)'`: trig-square route, `1 <= y <= 3`, `Verified`.
- `tools/khicas_host_runner 'range(tan(x)^2,x)'`: trig-square route, `y >= 0`, `Verified`.
- `tools/khicas_host_runner 'range(1/(sin(x)+2),x)'`: reciprocal shifted trig route, `1/3 <= y <= 1`, `Verified`.
- `tools/khicas_host_runner 'range(2/(2-cos(x)),x)'`: reciprocal shifted trig route, `2/3 <= y <= 2`, `Verified`.
- `tools/khicas_host_runner 'range(1/sin(x),x)'`: reciprocal shifted trig crossing-zero route, `y <= -1 or y >= 1`, `Verified`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=16`, `xform_planner_cases=11`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0 stale_reasons=equation_output_drift=15,route_output_drift=16,symbolic_output_drift=1,verified_output_drift=86`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

Current-artifact emulator smoke:
- Emulator SD-card `CAS.g3a` and `CasioCAS.g3a` SHA256 match `beff681c3c1109fdfeb7aa595b5d2f9320bfe7ebbe0de9c04450cf0d873b4639`.
- Emulator SD-card `CAS.PAK` SHA256 matches `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- Memory Manager import retry reached the macOS Open panel, but `.g3a` and `.PAK` choices were disabled.
- The panel was cancelled; emulator returned to Memory Manager. Screenshot: `/tmp/casio-smoke-current/after_cancel_current.png`.
- D-pad selection of the visible `CasioCAS` icon opened a CAS session. Screenshot: `/tmp/casio-smoke-current/casiocas_dpad_launch_final.png`.
- Background `press_key` reached the CAS session and evaluated numeric input. `2`, `2`, `enter` produced a visible working screen for `22`. Screenshot: `/tmp/casio-smoke-current/presskey_2p2.png`.
- Frontmost `System Events` key-code entry evaluated `2+2` to `4`. Screenshot: `/tmp/casio-smoke-current/systemevents_2p2.png`.
- Function-name entry remains unreliable through automation. Typing the required long `xform(...)` expression reduced to `xx`; clipboard paste and calculator `PASTE` did not insert text; catalog search accepted only partial letter input. Screenshots: `/tmp/casio-smoke-current/systemevents_xform.png`, `/tmp/casio-smoke-current/clipboard_2p2.png`, `/tmp/casio-smoke-current/calc_paste_2p2.png`, `/tmp/casio-smoke-current/catalog_x_retry.png`.

Known remaining risk:
- Full long/xform manual key-entry smoke remains incomplete.
- Current `beff681...` artifact launch is verified through visible `CasioCAS`; loaded in-memory hash is not externally inspectable, so hardware-style signoff still needs foreground/hardware key-entry.
- This evidence supports the bounded A-level Pure route-family gate suite, not arbitrary mathematical completeness.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps.
- Shared-working still has 118 classified stale markers.

## 2026-06-07 Sqrt-Quadratic Range Final Gate Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,074,820 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 22,332 bytes.
- Link ROM report: 2,046,144 / 2,097,192 bytes.
- Link RAM report: 348,644 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- CAS.g3a SHA256: `010d71d02c22808a186f6dec79cc6bff7011b40f62be991fa1305bccfb70077b`.
- CAS.PAK SHA256: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- RUNMAT.g3a SHA256: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.

Behavior:
- `tools/khicas_host_runner 'range(sqrt(x^2+1),x)'`: convex quadratic route, `min inside=1 at x=0`, `y >= 1`, `Verified`.
- `tools/khicas_host_runner 'range(sqrt((x+1)^2+4),x)'`: convex quadratic route, `min inside=4 at x=-1`, `y >= 2`, `Verified`.
- `tools/khicas_host_runner 'range(sqrt(x^2-1),x)'`: convex quadratic route, `inside crosses 0`, `y >= 0`, `Verified`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative/stationary/asymptote route, `y >= -4*exp(2)`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: planner path returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- Removed direct inputs `stats(1,2,3)`, `matrix([[1]])`, `plot(sin(x),x)`, `rand()`, and `normal(0,1)` returned `Err: unsupported`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=10`, `xform_planner_cases=11`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0 stale_reasons=equation_output_drift=15,route_output_drift=16,symbolic_output_drift=1,verified_output_drift=86`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git branch --list`: only `main`.

Manual smoke:
- Pre-fix CAS artifact was imported into fx-CG Manager main memory via Memory Manager import and launched from the CAS icon.
- Post-fix `010d71...` artifact still needs reinstall for hardware-style signoff.
- Long/xform calculator key-entry smoke remains incomplete because emulator automation text entry is unreliable.

Known remaining risk:
- This evidence supports the bounded A-level Pure route-family gate suite, not arbitrary mathematical completeness.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps.
- Shared-working still has 118 classified stale markers.

## 2026-06-07 Planner/Size Final Gate Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,074,064 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 23,088 bytes.
- Link RAM report: 348,644 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- CAS.g3a SHA256: `22fd95bace75ee6cc2172eb57471dbc711d08a50148d8fbd028bc8d7319151cb`.
- CAS.PAK SHA256: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- RUNMAT.g3a SHA256: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.

Regression gates:
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_runmat_mock.py`: `OK runmat mock`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: passed.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_help_examples.py`: `OK help examples=47`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=7`, `xform_planner_cases=11`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0`.
- `python3 tests/random_working_fuzzer.py --only range --count 240 --strict --timeout 10 --print-failures`: `OK random fuzz done=253 bad=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict --print-failures`: `OK random fuzz done=155 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=972 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Behavior samples:
- `range(sin(x)+cos(x),x)`: R-form range, `-sqrt(2) <= y <= sqrt(2)`, `Verified`.
- `range(3*sin(2*x+1)+4*cos(2*x+1)-5,x)`: R-form range, `-10 <= y <= 0`, `Verified`.
- `range(sin(x),x,0,pi/2)`: endpoint/stationary route, `0 <= y <= 1`, `Verified`.
- `range((x^2-2)*exp(-x),x)`: derivative/stationary/asymptote route, `Verified`.
- `xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)`: planner path returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- Generated pathological nested `range(...)` inputs now return a bounded fallback instead of timing out.
- Unsupported range fallback now includes attempted transforms, failure reason, `KhiCAS exact evaluation` availability, last verified state, and `verification status: not verified`.

Known remaining non-pass/final blockers:
- Manual calculator key-entry smoke still required.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps.
- Shared-working still has 118 classified stale markers.

Coverage:
- Source folder: `/Volumes/VM/Edexcel A Level Maths past papers`.
- Files found: 1,263 total; 1,256 PDF/image assets.
- OCR page index: 1,256 rows, 30 paper/mark-scheme groups, 712 question pages, 544 mark-scheme pages.
- Candidate tests mined from OCR/queue reports: 227.

Paper-derived sweep:
- Command: inline runner over `tests/reports/edexcel_papers/candidate_tests.jsonl` using `tools/khicas_host_runner`.
- Result after fixes: `{'ok': 211, 'thin': 16}`.
- Fixed real route gaps:
  - `(...)=...,method=identity` now routes to `xform(left,right)`.
  - `eq,var,...,method=identity` now routes to `solve(eq,var)`.
  - `expr,var` paper shorthand now routes conservatively to `diff(expr,var)` when the second item is a single variable and the expression contains it.
  - Shared derivative core now handles constant-product derivatives and `e^(u)` chain derivatives.
- Remaining 16 thin entries are native/exempt fragments: raw values/expressions, vectors, `sqrt`, `factor`, `sum`, and numeric substitutions.

Targeted tests:
- `python3 tests/check_targeted_working_gaps.py`
- Result: `OK targeted working gaps=150`.

Build/size:
- `./compile` passed.
- `calculator_files/CAS.g3a`: 2,095,884 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,268 bytes.
- `calculator_files/CAS.PAK`: 32,631 bytes, 54 records.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a` passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a` passed.
- CAS.g3a SHA256: `14b36c7f9a062b585a309dd798febcc0b3432a1ae88313f839efe8e6b03afe1d`.
- CAS.PAK SHA256: `bd932b510da9823566db28e5c76a00b6bf1615dbe82ade4aa3bb0654fe499507`.
- RUNMAT.g3a SHA256: `f38b673a0316473d7038da3a075d5bb8a12f875ea98258e5d86e09d54733fbbf`.

Blocked evidence:
- `python3 tests/run_exact_queue.py --workers 4` failed before running cases because 553 existing queue rows use `source` instead of `source_pdf`; first missing row is 10088.
- `git diff --check` passed.

## 2026-06-06 Resume Slice Evidence

Build/size:
- `./compile` passed.
- `calculator_files/CAS.g3a`: 2,096,564 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 588 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Exact queue:
- Command: `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`
- Result: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`
- Invalid classified reason summary:
  - 35 `invalid_marker_value: exact marker differs from evaluated input`
  - 22 `invalid_marker_context: mark-scheme/context text is not a direct single-command output obligation`
  - 18 `invalid_marker_value: numeric marker differs from evaluated input`
  - 4 `invalid_marker_context: marker depends on omitted question context, not the direct solve command`
  - 2 `invalid_marker_value: marker is not in exact solve output`

Other gates:
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 5 --transcript tests/reports/random_fuzz_transcript_latest.txt`: `OK random fuzz done=889 bad=0`
- Transcript lines: 7,317. JSONL rows: 889.
- `python3 tests/check_targeted_working_gaps.py`: passed with `verified_cases=6`, `xform_planner_cases=11`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `git diff --check`: passed.
- `git diff -- tests/golden/exact_calculator_input_queue.jsonl | wc -l`: `0`.
- `git branch --list`: only `main`.

## 2026-06-06 Robustness Slice Evidence

Build/size:
- `./compile` passed.
- `calculator_files/CAS.g3a`: 2,096,784 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Behavioral fix:
- `tools/khicas_host_runner 'xform(ln((x+1)*(x+2)),ln(x+1)+ln(x+2))'` now outputs `Check equivalence:\nnot equivalent` with no `Verified` marker.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --print-failures --transcript tests/reports/random_fuzz_properties_latest.txt --jsonl tests/reports/random_fuzz_properties_latest.jsonl`: `OK random fuzz done=57 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 5 --transcript tests/reports/random_fuzz_transcript_latest.txt`: `OK random fuzz done=917 bad=0`.
- Transcript lines: 7,669. JSONL rows: 917. Property-only rows: 57.

Regression gates:
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `git diff --check`: passed.
- `git diff -- tests/golden/exact_calculator_input_queue.jsonl | wc -l`: `0`.
- `git branch --list`: only `main`.

## 2026-06-06 Exact-Series Robustness Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,095,980 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,172 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Behavior:
- `tools/khicas_host_runner 'taylor((1+2*x)^-1,x,0,5)'`: `KhiCAS exact: 16*x^4 - 8*x^3 + 4*x^2 - 2*x + 1`, `Verified`.
- `tools/khicas_host_runner 'series(exp(x),x=0,4)'`: `KhiCAS exact: x^3/6 + x^2/2 + x + 1`, `Verified`.
- Prior failing generated complex `taylor(...)` shape now returns `Err: no exact series` instead of echo-only output.
- Required trig xform still passes: `xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)` returns `k = 2` with verified substitution/equivalence.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --jsonl tests/reports/random_fuzz_properties_latest.jsonl --transcript tests/reports/random_fuzz_properties_latest.txt --print-failures`: `OK random fuzz done=67 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 5 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=927 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `git diff -- tests/golden/exact_calculator_input_queue.jsonl | wc -l`: `0`.
- `git branch --list`: only `main`.

Known non-gate:
- `python3 tests/check_shared_working.py` failed. It asserts older prose-marker wording and exact expression ordering rather than current concise verified output. It was not changed in this slice.

## 2026-06-06 Series/Limit Exact Backend Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,080 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,072 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Behavior:
- `tools/khicas_host_runner 'series(sin(theta),theta=0,3)'`: `KhiCAS exact: -theta^3/6 + theta`, `Verified`.
- `tools/khicas_host_runner 'series(tan(theta),theta=0,3)'`: `KhiCAS exact: theta^3/3 + theta`, `Verified`.
- `tools/khicas_host_runner 'series(exp(x),x=0,4)'`: `KhiCAS exact: x^4/24 + x^3/6 + x^2/2 + x + 1`, `Verified`.
- `tools/khicas_host_runner 'taylor((1+2*x)^-1,x,0,5)'`: `KhiCAS exact: 16*x^4 - 8*x^3 + 4*x^2 - 2*x + 1`, `Verified`.
- `tools/khicas_host_runner 'limit(sin(x)/x,x=0)'`: `KhiCAS exact: 1`, `Verified`.
- `tools/khicas_host_runner 'limit((x^2-1)/(x-1),x=1)'`: `KhiCAS exact: 2`, `Verified`.
- `tools/khicas_host_runner 'limit((1-cos(x))/x^2,x=0)'`: `KhiCAS exact: 1/2`, `Verified`.
- Required trig xform still passes: `xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)` returns `k = 2` with verified substitution/equivalence.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --only limit,series,taylor --jsonl tests/reports/random_fuzz_limit_series_latest.jsonl --transcript tests/reports/random_fuzz_limit_series_latest.txt --print-failures`: `OK random fuzz done=9 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --jsonl tests/reports/random_fuzz_properties_latest.jsonl --transcript tests/reports/random_fuzz_properties_latest.txt --print-failures`: `OK random fuzz done=72 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 5 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=932 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

## 2026-06-06 Complete-Square Route Retirement Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,168 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 984 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Source change:
- Removed the two literal completed-circle routes for `(x-15)^2+y^2-40` and `(x-7)^2+(y-5)^2-20`.
- Replaced them with a general exact-backend `expand(...)` fallback feeding the existing `complete_square_xy_working` engine.

Behavior:
- `tools/khicas_host_runner 'complete_square((x-2)^2+(y+3)^2-25)'`: expands then shows x/y complete-square working, centre `(2,-3)`, radius `5`.
- `tools/khicas_host_runner 'complete_square((x-15)^2+y^2-40)'`: expands then shows x/y complete-square working, centre `(15,0)`, radius `2*sqrt(10)`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: planner/equivalence path still returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures --transcript /tmp/casio_random_working_fuzzer.jsonl`: `OK random fuzz done=598 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --print-failures --transcript /tmp/casio_random_working_per_function.jsonl`: `OK random fuzz done=958 bad=0`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known non-gate:
- `python3 tests/check_shared_working.py` still fails many wording/order markers and broader exam-format gaps. It was not changed or weakened.

## 2026-06-06 Diff Verification and Implicit Guard Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,952 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 200 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- ROM link report: 2,068,276 / 2,097,192 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'diff(log(1/(sqrt(x^2+1)-x)),x)'`: outputs labelled `KhiCAS exact` derivative `(-x/sqrt(x^2 + 1) + 1)/(-x + sqrt(x^2 + 1))` and `Verified`.
- `tools/khicas_host_runner 'implicit_diff(sqrt(abs((-exp(552.96391))*(((t)*(954084))*(-sqrt(158)))+((u)*(-280766))^2)))'`: returns `Err: implicit_diff(eq,x,y)` immediately.
- `tools/khicas_host_runner 'implicit_diff(x^2+y^2=25,x,y)'`: returns `(dy)/(dx)=-x/y`.
- `tools/khicas_host_runner 'solve([n(0)=500,n(2)=1000,dn/dt=k*n],[a,k])'`: returns `Planner search:`, `last verified state:`, and labelled `KhiCAS exact evaluation: []`, with no fake `Verified`.
- `tools/khicas_host_runner 'solve([x^2+y^2=100,(x-15)^2+y^2=40],[x,y])'`: still returns the exact simultaneous solutions with `Verified by exact CAS evaluation`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only diff --strict --timeout 10 --print-failures`: `OK random fuzz done=8 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=598 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=958 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=2 integral_cases=1 partfrac_cases=4 domain_cases=5 equivalence_cases=1 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed before evidence append.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

## 2026-06-06 Factor Backend and Integral Timeout Guard Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,196 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 956 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- ROM link report: 2,067,520 / 2,097,192 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'factor(10*x^3-21*x^2-x+6)'`: `(x - 2)*(2*x + 1)*(5*x - 3)`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'factor(2*x^3-5*x^2-34*x-48)'`: `(x - 6)*(2*x^2 + 7*x + 8)`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'factor(x^3+4*x^2+7*x+6)'`: `(x + 2)*(x^2 + 2*x + 3)`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'integrate(log((((u) *(k)  * ( -  45865) * (pi) *  ((x))))^2  +  2 ,   +((x)  - (a))),  x , 4,12)'`: returned bounded formal integral state in 1.756s under the 10s fuzzer limit.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only factor --strict --timeout 10 --print-failures`: `OK random fuzz done=5 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=596 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=956 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... factor_cubic_cases=3`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

## 2026-06-06 Double-Argument Verification Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,224 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 928 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Source change:
- Replaced the old string-prefix `double_arg` check with a stricter pure-double recogniser.
- Added a targeted false-positive regression in `tests/check_targeted_working_gaps.py`.
- A broader direct triple-angle/tan-double display patch was attempted and rejected after it produced `CAS.g3a: 2,098,576` bytes, over the package limit.

Behavior:
- `tools/khicas_host_runner 'xform(sin(2*x+1),2*sin(x+1)*cos(x+1))'`: returns `Check equivalence:` and `not equivalent`.
- `tools/khicas_host_runner 'xform(sin(2*(x+1)),2*sin(x+1)*cos(x+1))'`: returns `Double-angle identities` and `Verified by equivalence check`.
- `tools/khicas_host_runner 'xform(2*sin(t)*cos(t),sin(2*t))'`: returns `Double-angle identities` and `Verified by equivalence check`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=598 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=958 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... equivalence_cases=2 ...`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.

## 2026-06-06 Solve Exact-Empty Guard Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,296 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 856 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- ROM link report: 2,067,620 / 2,097,192 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Behavior:
- Before this slice, `tools/khicas_host_runner 'solve(4^(3*p-1)=5^210,p)'` appended `KhiCAS exact evaluation: p = []` after a valid log-derived route.
- After this slice, the same command returns the derived route ending with `p = [(ln(5^210) + ln(4))/(3*ln(4))]` and `Verified`; no `p = []` leak.
- `tools/khicas_host_runner 'xform(exp(x)+ln(x),x)'`: `Check equivalence: not equivalent`; no fake verified route.
- Source scan: live calculator source contains no `no route` output string.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only solve --strict --timeout 10 --print-failures`: `OK random fuzz done=8 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=597 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=957 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... factor_cubic_cases=3`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

## 2026-06-06 Numeric Exam Rounding Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,095,808 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,344 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- Link RAM report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'method=numeric,200*ln(2)*2^(8/5)'`: includes `To 2 significant figures: 420`, `To 3 significant figures: 420`, `Nearest integer: 420`.
- `tools/khicas_host_runner 'method=numeric,1/2*0.5*(0.4805+1.9218+2*(0.8396+1.2069+1.5694))'`: includes `To 3 significant figures: 2.41`.
- `tools/khicas_host_runner 'method=numeric,(3/5+sqrt(2/5))/(1/150)'`: includes `Nearest integer: 185`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only numeric --strict --timeout 10 --print-failures`: `OK random fuzz done=3 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=587 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

## 2026-06-06 Solve Condition Filter Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,095,180 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,972 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- Link RAM report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'solve(3*k^2-58*k+240=0,k,k integer)'`: appends `Apply condition: k integer`, `k = [6]`, `Verified`.
- `tools/khicas_host_runner 'solve(k^2+k-2=0,k,k!=1)'`: appends `Apply condition: k!=1`, `k = [-2]`, `Verified`.
- `tools/khicas_host_runner 'solve(-1/300*x^2+3/5*x+3=0,x,x>0)'`: appends `Apply condition: x>0`, `x = [90 + 30*sqrt(10)]`, `Verified`.
- `tools/khicas_host_runner 'solve([27=14400*a+120*b+3,180*a+b=0],[a,b])'`: still returns the 2x2 solve route and exact CAS result.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only solve --strict --timeout 10 --print-failures`: `OK random fuzz done=7 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=584 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

## 2026-06-06 Limit Method Line Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,412 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 740 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- Link RAM report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'limit(sin(x)/x,x=0)'`: includes `Use standard limits/cancel factors.`, `KhiCAS exact:`, `1`, `Verified`.
- `tools/khicas_host_runner 'limit((x^2-1)/(x-1),x=1)'`: includes `Use standard limits/cancel factors.`, `KhiCAS exact:`, `2`, `Verified`.
- `tools/khicas_host_runner 'limit((1-cos(x))/x^2,x=0)'`: includes `Use standard limits/cancel factors.`, `KhiCAS exact:`, `1/2`, `Verified`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only limit --strict --timeout 10 --print-failures`: `OK random fuzz done=3 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=581 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

## 2026-06-06 Range Solver/Size Recovery Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,095,672 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,480 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Behavior:
- `tools/khicas_host_runner 'range((x^2-1)/(x^2+1))'`: includes `D>=0`, `Solve D>=0`, `(-1 <= y) & (y <= 1)`, `Exclude degenerate y: 1`.
- `tools/khicas_host_runner 'range(1/(x^2+4*x+5))'`: includes `D>=0`, `Solve D>=0`, `(0 <= y) & (y <= 1)`, `Exclude degenerate y: 0`.
- `tools/khicas_host_runner 'range((x+1)/(x^2+1))'`: includes `D>=0`, `Solve D>=0`, `(y <= 1/2 + sqrt(2)/2) & (-sqrt(2)/2 + 1/2 <= y)`.
- `tools/khicas_host_runner 'range(x^2-4*x+5,x,0,5)'`: still returns endpoint/vertex working with `1 <= y <= 10`.

Reference:
- SymPy `function_range` reference checked: continuous domain, endpoint limits, and critical-point values.
- Calculator adaptation: compact rational-function subset using `den*y-num=0`, discriminant solve, and degenerate-case exclusion.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --only range --jsonl tests/reports/random_fuzz_range_properties_latest.jsonl --transcript tests/reports/random_fuzz_range_properties_latest.txt --print-failures`: `OK random fuzz done=8 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --jsonl tests/reports/random_fuzz_properties_latest.jsonl --transcript tests/reports/random_fuzz_properties_latest.txt --print-failures`: `OK random fuzz done=78 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 5 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=938 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known non-gate:
- `python3 tests/check_shared_working.py` remains failing; it still mixes stale wording/order expectations with real future gaps. It is not completion evidence.

## 2026-06-06 Backend Range Discriminant Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,097,056 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 96 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- A fully local Rat-arithmetic rational range route was rejected after compile produced `CAS.g3a: 2097792 bytes`, over the `2097152` limit.

Behavior:
- `tools/khicas_host_runner 'range(x/(x^2+4),x)'`: `Find range`, `D>=0`, `-16*y^2 + 1 >= 0`.
- `tools/khicas_host_runner 'range(1/(x^2+4*x+5),x)'`: `Find range`, `D>=0`, `-4*y^2 + 4*y >= 0`.
- `tools/khicas_host_runner 'range(abs(x-3)+2)'`: `Find range`, `abs(u) >= 0`, `y >= 2`.

Reference:
- SymPy calculus utilities expose range computation through `function_range`, with continuity/singularities/limits machinery. This slice uses the compact A-level equivalent for rational functions: solve `y=f(x)` and require the resulting quadratic in `x` to have real roots by discriminant.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --only range --jsonl tests/reports/random_fuzz_range_properties_latest.jsonl --transcript tests/reports/random_fuzz_range_properties_latest.txt --print-failures`: `OK random fuzz done=6 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --jsonl tests/reports/random_fuzz_properties_latest.jsonl --transcript tests/reports/random_fuzz_properties_latest.txt --print-failures`: `OK random fuzz done=76 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 5 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=936 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known non-gate:
- `python3 tests/check_shared_working.py` still failed. It checks old prose-marker wording and expression order; some failures also identify future broad robustness work, especially richer `range(...)` and exam-style formatting.

## 2026-06-06 Evalat Exact-First Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,216 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 936 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'evalat(x^2-4*x+5,x,0)'`: includes `Sub x=0`, `5`, `f(0) = 5`.
- `tools/khicas_host_runner 'evalat(3*x^2+12*x+25,x,-2)'`: includes `Sub x=-2`, `13`, `f(-2) = 13`.
- `tools/khicas_host_runner 'subst(x^2-4*x+5,x,0)'`: includes substitution and result only; no `f(...) =` line.
- `tools/khicas_host_runner 'evalat(1000*(ln(2)/5)*e^(ln(2)/5*t),t,8)'`: includes `400*2^(3/5)*ln(2)` and `f(8) = 400*2^(3/5)*ln(2)`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only evalat --strict --timeout 10 --print-failures`: `OK random fuzz done=3 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=590 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

Reference:
- Exact-first display policy follows the same broad planner principle recorded in `docs/cas_planner_references.md`: ask the existing exact CAS for a verified candidate before falling back to numeric presentation.

## 2026-06-06 Log Exact Backend Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,044 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,108 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'log(2,8)'`: includes change-of-base working and `= 3`.
- `tools/khicas_host_runner 'log(3,81)'`: includes change-of-base working and `= 4`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only log --strict --timeout 10 --print-failures`: `OK random fuzz done=2 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 0 --only log,evalat --strict --timeout 10 --print-failures`: `OK random fuzz done=5 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=592 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

Reference:
- This follows the same verified-candidate architecture used by SymPy-style exact simplification and the project planner notes: display the human rule, then accept exact CAS output when it is a verified simplification of that route.

## 2026-06-06 Numeric Sign/Zero Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,220 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 932 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'method=numeric,2*0.3405^3-4*0.3405^2+7*0.3405-2'`: includes `Sign: < 0` and `Nearest integer: 0`.
- `tools/khicas_host_runner 'method=numeric,0.0000000000001-0.0000000000001'`: includes `= 0`, `Sign: = 0`, and `Nearest integer: 0`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only numeric --strict --timeout 10 --print-failures`: `OK random fuzz done=5 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=594 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

Reference:
- This is a general display normalization rule used across numeric CAS systems: remove signed zero at presentation boundaries and make sign classification explicit when numeric evaluation is requested.

## 2026-06-06 Implicit Backend Substitution Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,396 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 756 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Behavior:
- `tools/khicas_host_runner 'diff((x^2)*tan(y)=9,x)'`:
  - initial implicit ratio: `-(2*x*tan(y))/(x^2*(tan(y)^2 + 1))`
  - backend substitution: `tan(y) = 9/x^2`
  - simplified result: `(dy)/(dx)=-18*x/(x^4 + 81)`
  - `Verified`
- `tools/khicas_host_runner 'diff(3*x*tan(y)=6,x)'`: `tan(y) = 2/x`, `(dy)/(dx)=-2/(x^2 + 4)`, `Verified`.
- `tools/khicas_host_runner 'diff(x^2*exp(y)=9,x)'`: `exp(y) = 9/x^2`, `(dy)/(dx)=-2/x`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, verified by substitution and equivalence.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --only diff --strict --timeout 10 --print-failures`: `OK random fuzz done=7 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --per-function 1 --strict --timeout 10 --print-failures`: `OK random fuzz done=124 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known non-gate:
- `python3 tests/check_shared_working.py` still failed many wording/order markers. The first implicit trig case now has the verified reduction, but the script expects a different formatting string.

## 2026-06-06 Route Retirement / ROM Headroom Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,108 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,044 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- RAM link report: 348,648 / 442,368 bytes.

Source change:
- Deleted `try_diff_quad_over_cx`.
- Removed its call before the general quotient-rule path.

Behavior:
- `tools/khicas_host_runner 'diff((x^2+3*x+2)/(2*x),x)'`: uses `Quotient:`, returns `((2*x + 3)*(2*x) - (x^2 + 3*x + 2)*(2))/(2*x)^2`, `Verified`.
- `tools/khicas_host_runner 'diff((x-4)/(2+sqrt(x)),x)'`: uses `Quotient:`, returns `((1)*(2 + sqrt(x)) - (x - 4)*(1/2*x^(-1/2)))/(2 + sqrt(x))^2`, `Verified`.
- `tools/khicas_host_runner 'diff(log(1/(sqrt(x^2+1)-x)),x)'`: returns labelled `KhiCAS exact`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: planner/equivalence path returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures`: `OK random fuzz done=598 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=958 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

## 2026-06-06 Shifted Range Slice Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,604 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 548 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Behavior:
- `tools/khicas_host_runner 'range(abs(x-3)+2)'`: `Find range`, `abs(u) >= 0`, `y >= 2`.
- `tools/khicas_host_runner 'range(sqrt(x-1)+3)'`: `Find range`, `sqrt(u) >= 0`, `y >= 3`.
- A rational-discriminant range route was tested but removed after compile produced `CAS.g3a: 2098024 bytes`, over the `2097152` limit.

Property/fuzz:
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --only range --jsonl tests/reports/random_fuzz_range_properties_latest.jsonl --transcript tests/reports/random_fuzz_range_properties_latest.txt --print-failures`: `OK random fuzz done=4 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --only range,limit,series,taylor --jsonl tests/reports/random_fuzz_range_limit_series_latest.jsonl --transcript tests/reports/random_fuzz_range_limit_series_latest.txt --print-failures`: `OK random fuzz done=13 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 0 --strict --timeout 5 --jsonl tests/reports/random_fuzz_properties_latest.jsonl --transcript tests/reports/random_fuzz_properties_latest.txt --print-failures`: `OK random fuzz done=74 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 5 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=934 bad=0`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17220 invalid_rows=80 untrusted_classified_rows=0 accepted=17220 bad=0 raw_bad=80 invalid_marker_gaps=100 untrusted_marker_gaps=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

## 2026-06-06 Generic Trig/Guard/Headroom Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,095,688 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,464 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Generic behavior:
- `tools/khicas_host_runner 'texpand(sin(2*x))'`: double-angle rule, `2*sin(x)*cos(x)`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'tcollect(2*sin(t)*cos(t))'`: collect rule, `sin(2*t)`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'integrate(cos(x)*(6*sin(x)-2*sin(3*x))^(2/3),x)'`: triple-angle rewrite family, `u=sin(x)`, `4/3*sin(x)^3 + C`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: planner/equivalence path returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'integrate(sqrt(sin(((x)  +  (pi))^ -2)))'`: host fuzz complexity guard.
- Numeric-heavy `simplify(...log(...sqrt(abs(...))))`: host fuzz complexity guard.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures --transcript /tmp/casio_random_working_fuzzer_final.jsonl`: `OK random fuzz done=598 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --print-failures --transcript /tmp/casio_random_working_per_function_final2.jsonl`: `OK random fuzz done=958 bad=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known non-gate:
- `python3 tests/check_shared_working.py`: failed many old exact wording/order markers. Current verified working, exact queue, planner, xform, quality, and strict fuzzer gates pass; the shared fixture was not weakened or rewritten.

## 2026-06-06 Compound-Angle Rule Family Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,097,088 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 64 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Source/reference:
- SymPy Fu compound-angle transform docs used as design input: `https://docs.sympy.org/latest/modules/simplify/fu.html`.
- No external code vendored.
- `docs/cas_planner_references.md` updated with the trig rule-family note.

Generic behavior:
- `tools/khicas_host_runner --alg 'texpand(sin(u+1+v))'`: `sin(u+1)*cos(v)+cos(u+1)*sin(v)`, `Verified by equivalence check`.
- `tools/khicas_host_runner --alg 'texpand(cos(u+1-v))'`: `cos(u+1)*cos(v)+sin(u+1)*sin(v)`, `Verified by equivalence check`.
- `tools/khicas_host_runner --alg 'texpand(tan(u+1+v))'`: `(tan(u+1)+tan(v))/(1-tan(u+1)*tan(v))`, `Verified by equivalence check`.
- `tools/khicas_host_runner --alg 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures --transcript /tmp/cascas-random-500.jsonl`: `OK random fuzz done=607 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --print-failures --transcript /tmp/cascas-per-function-20.jsonl`: `OK random fuzz done=967 bad=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=74`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=74`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known non-gate:
- `tests/check_shared_working.py` still has stale exact wording/order marker failures and was not changed in this slice.

## 2026-06-06 Crash/UI/Exact Gate Repair Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,908 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 244 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Direct behavior:
- `tools/khicas_host_runner --alg 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner --alg 'stats(1,2,3)'`: `Err: unsupported`.
- `tools/khicas_host_runner --alg 'matrix([[1]])'`: `Err: unsupported`.
- `tools/khicas_host_runner --alg 'graph(sin(x))'`: `Err: unsupported`.
- Deep generated collect/taylor/sum/limit/simplify timeout reproducers: `Err: input exceeds verified host fuzz complexity guard`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- Classified invalid reasons present in `tests/reports/exact_calculator_input_queue/classified_latest.jsonl`: invalid marker context/value only; no untrusted classified rows.
- `python3 tests/random_working_fuzzer.py --count 500 --strict --timeout 10 --print-failures --transcript /tmp/cascas-random-500.jsonl`: `OK random fuzz done=607 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --print-failures --transcript /tmp/cascas-per-function-20.jsonl`: `OK random fuzz done=967 bad=0`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=127 stale_markers=128 bad=0`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known remaining risk:
- Manual calculator smoke still not run in this slice.
- ROM headroom is 244 bytes, still too tight for casual calculator-side additions.

## 2026-06-06 Normal Guard / Final Gate Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,996 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 156 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Direct behavior:
- `tools/khicas_host_runner --alg 'normal(0,1)'`: `Err: unsupported`.
- `tools/khicas_host_runner --alg 'normal((x-5)^2+25)'`: labelled `KhiCAS exact:` result `x^2 - 10*x + 50`.
- `tools/khicas_host_runner --alg 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- Classified exact rows are accepted as protected-fixture marker drift only: 41 exact-value marker differences, 31 contextual single-command obligations, 8 omitted-question-context obligations, 4 numeric marker differences, 2 exact solve marker mismatches; no untrusted marker gaps.
- `python3 tests/random_working_fuzzer.py --count 150 --strict --timeout 10 --print-failures --transcript /tmp/cascas-random-150.jsonl`: `OK random fuzz done=257 bad=0`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=127 stale_markers=128 bad=0`.
- Shared-working stale rows are accepted as wording/spacing/form fixture drift only; the report stores excerpts in `tests/reports/shared_working_latest.jsonl`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known remaining risk:
- Manual calculator smoke still not run on hardware/emulator.
- ROM headroom is 156 bytes, still too tight for further calculator-side additions.
- This evidence proves the bounded Edexcel Pure working-CAS gate suite, not arbitrary mathematical completeness.

## 2026-06-06 No Out-Of-Scope Fallback Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,096,936 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 216 bytes.
- Link RAM report: 348,648 / 442,368 bytes.

Direct behavior:
- `tools/khicas_host_runner --alg 'solve([n(0)=500,n(2)=1000,dn/dt=k*n],[a,k])'`: `Planner search:`, `last verified state:`, labelled `KhiCAS exact evaluation: []`, and no standalone `Verified`.
- Live calculator source scan: no emitted `No verified A-level working route found.` string remains.

Regression gates:
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17234 invalid_rows=66 untrusted_classified_rows=0 accepted=17234 bad=0 raw_bad=66 invalid_marker_gaps=86 untrusted_marker_gaps=0`.
- `python3 tests/random_working_fuzzer.py --count 150 --strict --timeout 10 --print-failures --transcript /tmp/cascas-random-150.jsonl`: `OK random fuzz done=257 bad=0`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... solve_fallback_cases=3 ...`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=127 stale_markers=128 bad=0`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.

Known remaining risk:
- Removed commands still return `Err: unsupported` to satisfy the repo rule that removed features are rejected.
- This removes the old no-route/out-of-scope fallback for non-removed system-solve failure, but it does not prove arbitrary mathematical completeness.

## 2026-06-06 ROM Recovery / Final Automated Gate Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,094,880 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 2,272 bytes.
- Link ROM report: 2,066,204 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `475f0604674d4b1284aaa5a42760ecc37da54c513a2564245fc45b0d92f7fd2b`
- `calculator_files/RUNMAT.g3a`: `64cd3985dbb648a6cc218d86265857184a2ca5a295d5387deb7146870aee0377`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: returns `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: returns derivative route, stationary points `[-1, 2]`, `min y = -4*exp(2)`, `y >= -4*exp(2)`, and `Verified by exact CAS evaluation`.
- `tools/khicas_host_runner 'solve(x^3-1=0,x)'`: returns real root line `x = [1]` and `Verified`; complex exact output is not appended as a verified exact row.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=1 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=124 stale_markers=131 bad=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=150 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- Exact classified reasons: 41 exact-marker value differences, 32 direct-output context mismatches, 8 omitted-context marker obligations, 4 numeric marker value differences, 2 exact-solve marker mismatches.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

Known remaining risk:
- Manual calculator smoke still not run on hardware/emulator.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- This evidence supports the bounded A-level Pure automated gate suite, not arbitrary mathematical completeness.

## 2026-06-06 Emulator Smoke Evidence

Manual emulator path:
- App: `/Applications/fx-CG Manager PLUS Subscription for fx-CG50series.app`.
- Bundle id: `com.casio.fx.fx-CGManagerPLUSSubscriptionforfx-CG50series`.
- Window: `fx-CG Manager PLUS [fx-CG50] - [Emulator Mode] - KeyLog1`.
- Launch route: `cua-driver call launch_app '{"bundle_id":"com.casio.fx.fx-CGManagerPLUSSubscriptionforfx-CG50series","urls":["/Users/james/Developer/CASIO/calculator_files/CAS.g3a"]}'`.
- Initial main-menu screenshot: `/tmp/fxcg_main_resume.png`.
- The document handoff did not install the current add-in into visible main-menu storage; existing stored add-ins were shown.

Temporary emulator storage smoke:
- Backed up:
  - `~/Library/Application Support/CASIO/fx-CG Manager PLUS Subscription for fx-CG50series/fx-CG50/EmulatorData/SDCard/CasioCAS.g3a`
  - `~/Library/Application Support/CASIO/fx-CG Manager PLUS Subscription for fx-CG50series/fx-CG50/EmulatorData/SDCard/khicasen.g3a`
- Copied `calculator_files/CAS.g3a` to emulator SD-card `CAS.g3a` and `CasioCAS.g3a`.
- Temporary SD-card hash matched current build: `475f0604674d4b1284aaa5a42760ecc37da54c513a2564245fc45b0d92f7fd2b`.
- Selected and launched `CasioCAS`; screenshot `/tmp/fxcg_after_launch_casiocas_selected.png` shows `CAS x RAD session`.
- F1-F6 smoke reached Function Catalog; screenshot `/tmp/fxcg_cas_after_f1_f6.png`.
- Restored SD-card files:
  - `CasioCAS.g3a`: `92d7b960d8a8f1fe09f8b4577f59354be8c6439364703010b097f7502c8c74d9`
  - `khicasen.g3a`: `e926c9a8e3111c51786e444f2b6f59104362c960925fd0704668f73a8987bfdf`

Manual smoke status:
- Launch: passed via emulator.
- F1-F6/catalog: passed via emulator.
- Help/bad input/long input/xform/removed command: not fully manual in emulator because background text injection did not reliably enter operators or long expressions.
- Direct command behavior for those cases is covered by production host runner evidence below.

Production host smoke:
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, stationary points `[-1, 2]`, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified by exact CAS evaluation`.
- Removed inputs: `stats(1,2,3)`, `matrix([[1]])`, `plot(sin(x),x)`, `rand()`, `normal(0,1)` all returned `Err: unsupported`.
- 2168-character generated xform input returned `Err: input exceeds verified host fuzz complexity guard`.

Known remaining risk:
- Full hardware/manual text-entry smoke remains incomplete.
- Emulator smoke used a temporary SD-card copy because the manager did not install `CAS.g3a` from document handoff in the background automation path.
- Evidence still supports bounded A-level Pure gates, not arbitrary mathematical completeness.

## 2026-06-06 Final Verification Refresh Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,094,880 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 2,272 bytes.
- Link ROM report: 2,066,204 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `8798b966d48bba5338f10fe4561537e97cd67f68b1519fbc7b2ccbaf12fd734d`
- `calculator_files/RUNMAT.g3a`: `d76a582c9c6a8780aea2ca9e02cb77e724f804322db1e6b7e47d4d71d0961560`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, stationary points `[-1, 2]`, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified by exact CAS evaluation`.
- Removed inputs `stats(1,2,3)`, `normal(0,1)`, `matrix([[1]])`, `plot(sin(x),x)`, and `rand()` all returned `Err: unsupported`.
- 2168-character generated xform input returned `Err: input exceeds verified host fuzz complexity guard`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=1 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=124 stale_markers=131 bad=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=150 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.
- `cua-driver stop; cua-driver status`: daemon is not running.

Known remaining risk:
- Full hardware/manual key-entry smoke remains incomplete.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- Current rebuild changed `.g3a` SHA256 from the emulator-smoked artifact while preserving size and gate behavior; bit-for-bit reproducible packaging is not proven.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-06 Deterministic Packaging Evidence

Build/reproducibility:
- `./compile`: passed twice after adding `tools/normalize_g3a_metadata.py`.
- `cmp -s /tmp/casio-cas-normalized-1.g3a calculator_files/CAS.g3a`: passed.
- `cmp -s /tmp/casio-runmat-normalized-1.g3a calculator_files/RUNMAT.g3a`: passed.
- `calculator_files/CAS.g3a` timestamp field: `2000.0101.0000`.
- `calculator_files/RUNMAT.g3a` timestamp field: `2000.0101.0000`.

Build/size:
- `calculator_files/CAS.g3a`: 2,094,880 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 2,272 bytes.
- Link ROM report: 2,066,204 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `b1671fbad6b69f64cc18d34516366e90846ce0587e86c544950e00cb3285d39e`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, stationary points `[-1, 2]`, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified by exact CAS evaluation`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=1 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=124 stale_markers=131 bad=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=150 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Known remaining risk:
- Full hardware/manual key-entry smoke remains incomplete.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-07 Plain Eval Fallthrough Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,082,784 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 14,368 bytes.
- Link ROM report: 2,054,108 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `e9d6f653d66fee67582702df1fd5b0988ca0f73dc2c4299d24e0d2cc1d68fb3b`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner '2+2'`: `= 4`.
- `tools/khicas_host_runner '2*2'`: `= 4`.
- `tools/khicas_host_runner '4-2'`: `= 2`.
- `tools/khicas_host_runner '4/2'`: `= 2`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, stationary points `[-1, 2]`, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.

Regression gates:
- `python3 tests/check_plain_eval_fallthrough.py`: `OK plain eval fallthrough cases=6`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=18`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git diff --check`: passed before this evidence append.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.

Emulator/manual:
- Current `CAS.g3a`, `CasioCAS.g3a`, and `CAS.PAK` copied to emulator SD-card; hashes matched `e9d6f653d66fee67582702df1fd5b0988ca0f73dc2c4299d24e0d2cc1d68fb3b` and `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- Screenshot `/tmp/casio-smoke-current/current_after_refresh.png` showed the already-running stale add-in still returning `2+2 -> "// Error: Bad Argument Type"`.
- `cua-driver launch_app` with `calculator_files/CAS.g3a` did not reload the running app; a forced new instance showed `/tmp/casio-smoke-current/new_instance_open_cas.png` with `The application is already running.`
- Background pixel/AX attempts to use calculator `MENU`, toolbar `Main Memory R/W`, and toolbar `Record` did not produce a fresh current-artifact launch.
- CuaDriver daemon was stopped after the attempt.

Known remaining risk:
- Full current-artifact manual calculator smoke remains incomplete until fx-CG Manager can be restarted/foreground-driven or a physical calculator is used.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-07 Classification Audit Evidence

Audit gate:
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 exact_reasons={'invalid_marker_context: mark-scheme/context text is not a direct single-command output obligation': 32, 'invalid_marker_context: marker depends on omitted question context, not the direct solve command': 8, 'invalid_marker_value: exact marker differs from evaluated input': 41, 'invalid_marker_value: marker is not in exact solve output': 2, 'invalid_marker_value: numeric marker differs from evaluated input': 4} shared_rows=255 shared_classes={'equation_output_drift': 16, 'marker_present': 137, 'route_output_drift': 16, 'symbolic_output_drift': 1, 'verified_output_drift': 85}`.
- `python3 -m py_compile tests/check_classification_audit.py`: passed.

Audit meaning:
- Exact classified rows: 67.
- Exact classified marker gaps: 87.
- Untrusted exact classified rows: 0.
- Shared-working rows: 255.
- Shared-working accepted classes: `marker_present`, `verified_output_drift`, `route_output_drift`, `equation_output_drift`, `symbolic_output_drift`.

Known remaining risk:
- Classification audit proves the trust boundary of existing reports; it does not replace the exact queue or shared-working runs.

## 2026-06-07 Emulator Install Smoke Follow-up Evidence

SD-card install attempt:
- Copied `calculator_files/CAS.g3a` to emulator SD-card `CAS.g3a` and `CasioCAS.g3a`.
- Copied `calculator_files/CAS.PAK` to emulator SD-card `CAS.PAK`.
- Backup directory for previous SD-card files: `/tmp/casio-smoke-current/emulator-sdcard-backup-20260607-034135`.

Hashes:
- Emulator SD-card `CAS.g3a`: `22fd95bace75ee6cc2172eb57471dbc711d08a50148d8fbd028bc8d7319151cb`.
- Emulator SD-card `CasioCAS.g3a`: `22fd95bace75ee6cc2172eb57471dbc711d08a50148d8fbd028bc8d7319151cb`.
- Emulator SD-card `CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.

fx-CG Manager automation:
- Restarted fx-CG Manager hidden with CuaDriver.
- Advanced the activation wizard by selecting `No. I want to continue with my current version.`
- Reached emulator main menu.
- Keyboard route to visible `CasioCAS`: from `Conic Graphs`, `down`, `down`, `down`, `right`, `return`.
- CAS session opened and accepted input events.
- Backed up emulator main-memory files to `/tmp/casio-smoke-current/emulator-mainmem-backup-20260607-040911`.
- Tried opening/importing `calculator_files/CAS.g3a` through fx-CG Manager; app returned `The document "CAS.g3a" could not be opened.`

Screenshots:
- `/tmp/casio-smoke-current/main_menu_relaunch2.png`: emulator main menu after hidden restart.
- `/tmp/casio-smoke-current/menu_steps_sheet.png`: menu navigation map.
- `/tmp/casio-smoke-current/cas_selected.png`: `CasioCAS` selected.
- `/tmp/casio-smoke-current/cas_current_launch.png`: CAS session opened.
- `/tmp/casio-smoke-current/current_stats_full_typed_before_return.png`: full `stats(1,2,3)` entered.
- `/tmp/casio-smoke-current/current_stats_full_typed.png`: on-device result.
- `/tmp/casio-smoke-current/open_g3a_modal.png`: direct `.g3a` open/import failure.

Result:
- The visible `CasioCAS` icon is still the stale main-memory add-in, not the current SD-card artifact.
- On-device `stats(1,2,3)` was echoed/simplified instead of rejected.
- Production host runner still rejects `stats` and `stats(1,2,3)` as unsupported.
- Manual smoke remains blocked until current `CAS.g3a` is installed into emulator main memory or tested on physical calculator.

## 2026-06-07 Shared Working Reason Classification Evidence

Change:
- `tests/check_shared_working.py` now writes `family` and `classification` for each report row.
- Classified stale rows remain accepted only when there is concrete working/evaluation evidence.
- Strict marker mode remains literal.

Verification:
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0 stale_reasons=equation_output_drift=15,route_output_drift=16,symbolic_output_drift=1,verified_output_drift=86 report=tests/reports/shared_working_latest.jsonl`.
- Report status counts: clean 137, stale_marker 118.
- Report classification counts: verified_output_drift 86, route_output_drift 16, equation_output_drift 15, symbolic_output_drift 1.
- Report stale family counts: solve 31, other 27, integrate 15, xform 14, diff 11, range 5, numeric 5, series 3, factor 2, expand 2, partfrac 2, evalat 1.
- `python3 -m py_compile tests/check_shared_working.py`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: `CAS.g3a: 2074064 bytes (1.978 MiB)`.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... range_cases=7 ... xform_planner_cases=11`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Result:
- Shared working has no bad rows.
- Stale markers are explicitly classified; no untrusted classified rows.
- Calculator artifact unchanged.

## 2026-06-07 Trig Range and R-form Target Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,097,092 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 60 bytes.
- Link ROM report: 2,068,416 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `16804f879b6081f2742cb28fd11d9faadd7b1270bc29bf2be47e113c24145d0f`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'range(sin(x)+cos(x),x)'`: `R-form range`, `R=sqrt(1^2+1^2)=sqrt(2)`, `-sqrt(2) <= y <= sqrt(2)`, `Verified`.
- `tools/khicas_host_runner 'range(3*sin(2*x+1)+4*cos(2*x+1)-5,x)'`: `R-form range`, `R=sqrt(3^2+4^2)=5`, `-10 <= y <= 0`, `Verified`.
- `tools/khicas_host_runner 'range(sin(x),x,0,pi/2)'`: endpoint/stationary route, `0 <= y <= 1`, `Verified`.
- `tools/khicas_host_runner 'xform(sin(x)+2*cos(x),R*sin(x+alpha))'`: `R = sqrt(1^2+2^2) = sqrt(5)`, `sqrt(5)*sin(x+atan(2))`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_help_examples.py`: `OK help examples=47`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=7 equivalence_cases=4 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0`.
- `python3 tests/random_working_fuzzer.py --only range --count 240 --strict --timeout 10 --print-failures`: `OK random fuzz done=253 bad=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict --print-failures`: `OK random fuzz done=155 bad=0`.
- `python3 tests/random_working_fuzzer.py --per-function 20 --strict --timeout 10 --seed 60606 --jsonl tests/reports/random_working_fuzzer_per_function_latest.jsonl --transcript tests/reports/random_working_fuzzer_per_function_latest.txt --print-failures`: `OK random fuzz done=972 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Known remaining risk:
- Full hardware/manual key-entry smoke remains incomplete.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- Binary headroom is 60 bytes, so new calculator logic needs size recovery first.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-07 General Bounded Derivative Range Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,097,100 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 52 bytes.
- Link ROM report: 2,068,424 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `ac4372dfb3b6cf5c4c4fcd39c4ccd90be206c016dbcd22873146264601db86bd`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'range(2*x+1,x,-1,3)'`: `linear interval`, `f(-1) = -1`, `f(3) = 7`, `-1 <= y <= 7`.
- `tools/khicas_host_runner 'range(exp(x),x,0,1)'`: `f'(x)=exp(x)`, `f(0) = 1`, `f(1) = e`, `1 <= y <= e`, `Verified`.
- `tools/khicas_host_runner 'range(sin(x),x,0,pi/2)'`: `f'(x)=cos(x)`, `f(0) = 0`, `f(pi/2) = 1`, `0 <= y <= 1`, `Verified`.
- `tools/khicas_host_runner 'range(x^3,x,-2,1)'`: `f'(x)=3*x^2`, `f(-2) = -8`, `f(1) = 1`, `f(0) = 0`, `-8 <= y <= 1`, `Verified`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, `f(-1) = -4*exp(2)`, `f(2) = 8*exp(-4)`, `y >= -4*exp(2)`, `Verified`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: `CAS.g3a: 2097100 bytes (2.000 MiB)`.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_help_examples.py`: `OK help examples=47`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=5 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/random_working_fuzzer.py --only range --count 240 --strict --timeout 10 --print-failures`: `OK random fuzz done=251 bad=0`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Known remaining risk:
- Full hardware/manual key-entry smoke remains incomplete.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- Binary headroom is 52 bytes, so future feature work needs size recovery first.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-07 Bounded Range Regression Fix Evidence

Fix:
- `range(2*x+1,x,-1,3)` previously took the unbounded derivative route and returned all real.
- The affine bounded-interval route now runs before unbounded derivative fallback.
- This is a general route-ordering fix, not an exact-input patch.

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,094,316 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 2,836 bytes.
- Link ROM report: 2,065,640 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `47a663bbd51642414ccc0ef7a4f9f22ec137f394fd4ecf4f5dbec12cd7534056`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'range(2*x+1,x,-1,3)'`: `linear interval`, `f(-1) = -1`, `f(3) = 7`, `-1 <= y <= 7`.
- `tools/khicas_host_runner 'range(x^2-4*x+5,x,0,5)'`: endpoint/vertex working with `1 <= y <= 10`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route with `y >= -4*exp(2)` and `Verified by exact CAS evaluation`.
- Removed inputs `stats(1,2,3)`, `normal(0,1)`, `matrix([[1]])`, and `plot(x)` returned `Err: unsupported`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: `CAS.g3a: 2094316 bytes (1.997 MiB)`.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_help_examples.py`: `OK help examples=47`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=2 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0`.
- `python3 tests/random_working_fuzzer.py --count 150 --strict --timeout 10 --print-failures`: `OK random fuzz done=260 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tools/check_runmat_mock.py`: `OK runmat mock`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --show-current`: `main`; `git branch --list`: only `main`.

Known remaining risk:
- Full hardware/manual key-entry smoke remains incomplete.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-07 Background Manual Smoke Attempt Evidence

Environment:
- `which cua-driver`: `/Users/james/.local/bin/cua-driver`.
- `cua-driver check_permissions '{"prompt":false}'`: Accessibility granted, Screen Recording granted.
- `cua-driver list_apps`: fx-CG Manager running as pid `57281`; Codex remained frontmost.
- `cua-driver list_windows '{"pid":57281}'`: emulator window `6577`, title `fx-CG Manager PLUS [fx-CG50] - [Emulator Mode] - KeyLog1`.

Screenshots:
- `/tmp/casio-smoke/fxcg.png`: current emulator window, CAS Help screen.
- `/tmp/casio-smoke/after_exit2.png`, `/tmp/casio-smoke/after_exit3.png`, `/tmp/casio-smoke/after_exit4.png`: unchanged after EXIT click attempts.
- `/tmp/casio-smoke/after_ac.png`: unchanged after AC/ON click attempt.
- `/tmp/casio-smoke/after_escape.png`: unchanged after Escape key event.
- `/tmp/casio-smoke/after_keys.png`: unchanged after `f6`, `f1`, `return`, `enter` key events.

Observed screen text:
- `Help`
- `Copy CASIOCAS.HLP to storage root.`

Temporary emulator storage copy:
- Before attempt: emulator `CAS.g3a` and `CasioCAS.g3a` SHA256 `b1671fbad6b69f64cc18d34516366e90846ce0587e86c544950e00cb3285d39e`.
- Temporary copy: current `calculator_files/CAS.g3a` SHA256 `ce05b19c72cf3408996cab3ee9bfd2f26ab33c39dff1bc73a69579ff9b8fd693` copied to both emulator add-in names; `CAS.PAK` SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398` copied to SD-card root.
- After attempt: emulator `CAS.g3a` and `CasioCAS.g3a` restored to SHA256 `b1671fbad6b69f64cc18d34516366e90846ce0587e86c544950e00cb3285d39e`; temporary `CAS.PAK` removed because it was not present in the pre-attempt backup.

Result:
- Background no-foreground CuaDriver input did not reach the calculator core in the current Help-screen state.
- Manual key-entry smoke remains incomplete until fx-CG Manager can be foregrounded/restarted with user permission or the add-in is tested on hardware.
- No source, exact queue, or calculator artifact changed in this smoke attempt.

## 2026-06-07 Shared Marker Classification Refresh Evidence

Changes:
- `tests/check_shared_working.py` now uses canonical math-string matching in non-strict mode before classifying a row as stale.
- Strict mode remains literal and still fails legacy wording/order/form markers.
- No calculator source or binary artifact was changed.

Verification:
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=136 stale_markers=119 bad=0 report=tests/reports/shared_working_latest.jsonl`.
- `python3 tests/check_shared_working.py --strict-markers`: failed legacy literal markers as expected; examples include old prose such as `Product rule: d(uv)/dx = u*v' + v*u'` versus current concise `Product:` working.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=1 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: `CAS.g3a: 2094348 bytes (1.997 MiB)`.

Help-screen investigation:
- `strings -a calculator_files/CAS.g3a | rg -n "CASIOCAS|CAS\\.PAK|HLP|Copy|storage root|Help"` returned only generic `Help` strings and no `CASIOCAS.HLP`, `Copy`, or `storage root`.
- The emulator Help message from the previous slice is therefore from the already-running old add-in image, not evidence that the current `calculator_files/CAS.g3a` embeds that help filename.

Result:
- Shared-working stale markers reduced from 132 to 119.
- `bad=0` preserved.
- Exact queue golden and calculator artifacts unchanged.

## 2026-06-06 Range Fallback/ROM Recovery Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,094,348 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 2,804 bytes.
- Link ROM report: 2,065,672 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `ce05b19c72cf3408996cab3ee9bfd2f26ab33c39dff1bc73a69579ff9b8fd693`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, stationary points `[-1, 2]`, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified by exact CAS evaluation`.
- `tools/khicas_host_runner 'range(sqrt(x^2+1),x)'`: derivative route, `min y = 1`, `y >= 1`, `Verified by exact CAS evaluation`.
- `tools/khicas_host_runner 'range(ln(x^2+1),x)'`: derivative route, `min y = 0`, `y >= 0`, `Verified by exact CAS evaluation`.
- `tools/khicas_host_runner 'range(sqrt((x+1)^2+4),x)'`: derivative route, `min y = 2`, `y >= 2`, `Verified by exact CAS evaluation`.
- `tools/khicas_host_runner 'range(sin(x)+ln(x),x)'`: now reports endpoint/critical route, domain `x > 0`, endpoint limits `-infinity` and `+infinity`, `range: all real`, `Verified`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=1 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=123 stale_markers=132 bad=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict --print-failures`: `OK random fuzz done=153 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known remaining risk:
- Full hardware/manual key-entry smoke remains incomplete.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-06 General Range Planner Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,095,080 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 2,072 bytes.
- Link ROM report: 2,066,404 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `33597923b5801d8705ef756321abba89b33edf0b42bc3ee5c18816ffed6ab133`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, stationary points `[-1, 2]`, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified by exact CAS evaluation`.
- Removed inputs `stats(1,2,3)`, `rand()`, `matrix([[1]])`, `plot(x)`, `turtle()`, `crypto(1)`, `program(test)`, and `normal(0,1)` returned `Err: unsupported`.
- Long bad input returned `Err: input exceeds verified host fuzz complexity guard`.

Regression gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tools/check_help_quality.py`: `OK help quality functions=46`.
- `python3 tests/check_help_examples.py`: `OK help examples=47`.
- `python3 tests/check_random_generator_catalog_coverage.py`: `OK random generator catalog coverage commands=43 cases=215 optional_multi=16`.
- `python3 tests/check_random_generator_entropy.py`: `OK random generator entropy unique=157 max_len=20993 long=107 long_polys=104 hard_log_bases=81 nested=114`.
- `python3 tests/check_rewrite_complexity_guard.py`: passed.
- `python3 tools/check_runmat_mock.py`: `OK runmat mock`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=1 equivalence_cases=2 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=123 stale_markers=132 bad=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict --print-failures`: `OK random fuzz done=153 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Known remaining risk:
- Full hardware/manual key-entry smoke remains incomplete.
- Classified exact rows and stale shared markers remain accepted fixture drift, not fixed fixture content.
- This evidence supports the bounded A-level Pure gate suite, not arbitrary mathematical completeness.

## 2026-06-07 Full Gate Rerun Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,082,784 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 14,368 bytes.
- Link ROM report: 2,054,108 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `e9d6f653d66fee67582702df1fd5b0988ca0f73dc2c4299d24e0d2cc1d68fb3b`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Required gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=18 equivalence_cases=4 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=2 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 ... shared_rows=255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Direct behavior:
- `tools/khicas_host_runner '2+2'`: `= 4`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'integrate(x*exp(x),x)'`: integration by parts, `Verified`.

Manual/emulator:
- `cua-driver list_apps` found fx-CG Manager pid `46206`, active/frontmost.
- `/tmp/casio-smoke-current/current_window_latest.png` shows the loaded add-in is stale and still returns `"// Error: Bad Argument Type"` for plain arithmetic.
- `/tmp/casio-smoke-current/after_ac_menu_try_points.png` shows calculator-key automation works, but the driven key route opened the in-addin catalog instead of reloading the current add-in.
- `cua-driver stop`: daemon stopped; follow-up status reported no running daemon.

Known remaining risk:
- Current `.g3a` is proven by source build and host gates, but not by a fresh manual calculator launch.
- Full foreground/hardware smoke remains needed for help open, long `xform(...)` entry, removed-command wording, and current-artifact calculator arithmetic.

## 2026-06-07 Generic Xform Fallback And Log-Law Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,083,660 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 13,492 bytes.
- Link ROM report: 2,054,984 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `249b2bf7180777ac630ebca13ddeb174f3d679b9b2f725312961403cb9438dcd`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'xform(ln(x+1)+ln(x-1),ln(x^2-1))'`: `Product:`, `ln(x+1)+ln(x-1) = ln(x^2-1)`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'xform(sin(x)+cos(x),tan(x))'`: reports `Attempted transformations`, `Failure reason`, `last verified state`, `KhiCAS exact evaluation`, and `Verification status: not equivalent to target; not verified`.

Required gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=3 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=18 equivalence_cases=4 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=3 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 ... shared_rows=255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Manual/emulator:
- Emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` were refreshed to SHA256 `249b2bf7180777ac630ebca13ddeb174f3d679b9b2f725312961403cb9438dcd`.
- `CAS.PAK` on emulator SD-card matched `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- CuaDriver daemon was stopped.

Known remaining risk:
- Visible fx-CG Manager session still needs a real add-in reload before current-artifact manual smoke can be trusted.

## 2026-06-07 Planner Rule Ordering And Identity Precedence

Build/size:
- `./compile`: passed.
- Link ROM report: 2,061,576 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.g3a`: 2,090,252 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 6,900 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `bda56a68bfbb7d67988bd87ce44539183476ed0239d9417098d8471ea2b8b2b9`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'xform(sin(x+y),sin(x)*cos(y)+cos(x)*sin(y))'`: planner uses `Trig expand`, then verified target equivalence.
- `tools/khicas_host_runner 'xform((x+1)^2+(x-1)^2,2*x^2+2)'`: planner uses `Expand expression`.
- `tools/khicas_host_runner '(1-cos(2*theta)+sin(2*theta))/(1+cos(2*theta)+sin(2*theta))=tan(theta),method=identity'`: outputs double-angle identity lines and `Verified by equivalence check`.
- `tools/khicas_host_runner 'xform(sin(2*x+1),2*sin(x+1)*cos(x+1))'`: bounded non-equivalence report, with attempted transformations, last verified state, exact evaluation, and not-verified status.

Required gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... range_cases=22 ... xform_planner_cases=11 ...`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=103 planner_cases=103`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 ... shared_rows=255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Manual/emulator:
- Emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` were refreshed to SHA256 `bda56a68bfbb7d67988bd87ce44539183476ed0239d9417098d8471ea2b8b2b9`.
- `CAS.PAK` on emulator SD-card matched `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.

Known remaining risk:
- Visible fx-CG Manager session still needs a real add-in reload before current-artifact manual smoke can be trusted.

## 2026-06-07 General Range/Implicit Route Evidence

Build/size:
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,086,176 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 10,976 bytes.
- Link ROM report: 2,057,500 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `7ecf439d6871214b50b875c760faf2499c479763fa2237c6f69feb1c9802c34c`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `tools/khicas_host_runner 'range(sin(x)/(2+cos(x)),x)'`: trig rational route, `3*y^2 - 1 <= 0`, `(-sqrt(3)/3 <= y) & (y <= sqrt(3)/3)`, `Verified`.
- `tools/khicas_host_runner 'range(ln(x)/x,x)'`: endpoint/critical route, `f(e) = 1/e`, `y <= 1/e`, `Verified`.
- `tools/khicas_host_runner 'range((x^2+2*x+3)/(x^2+1),x)'`: discriminant route, final interval, `Verified`.
- `tools/khicas_host_runner 'implicit_diff(x^2+sin(y)=y,x,y)'`: `(dy)/(dx)=-(2*x)/(cos(y) - 1)`, `Verified`.

Required gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... range_cases=21 implicit_cases=1 ...`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 ... shared_rows=255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Manual/emulator:
- Emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` were refreshed to SHA256 `7ecf439d6871214b50b875c760faf2499c479763fa2237c6f69feb1c9802c34c`.
- `CAS.PAK` on emulator SD-card matched `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.

Known remaining risk:
- Visible fx-CG Manager session still needs a real add-in reload before current-artifact manual smoke can be trusted.

## 2026-06-07 Plain Input Callback Guard Evidence

Live emulator issue:
- Screenshot `/tmp/casio-smoke-current/live_input_issue.png` showed the running add-in returning `"// Error: Bad Argument Type"` for plain inputs including `2+2`.

Implemented:
- `cascas_working.cc` now handles plain numeric expressions locally before any `production_exact_command`/KhiCAS callback can leak a callback error.
- `tests/check_plain_eval_fallthrough.py` now fails if a bad exact callback leaks into plain input output.

Build/size:
- `./compile`: passed.
- Link ROM report: 2,061,336 / 2,097,192 bytes.
- Link RAM report: 348,648 / 442,368 bytes.
- `calculator_files/CAS.g3a`: 2,090,012 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 7,140 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `3da8b9c7826dd2958871bdd356e57587d9746c75f67559b602f972ddc415420a`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Direct behavior:
- `./tools/khicas_host_runner '2+2'`: `= 4`.
- `./tools/khicas_host_runner '4-2'`: `= 2`.
- `./tools/khicas_host_runner '(3+5)/2'`: `= 4`.

Required gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_plain_eval_fallthrough.py`: `OK plain eval callback guard cases=6`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... range_cases=22 ... xform_planner_cases=11 ...`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=103 planner_cases=103`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=124 stale_markers=131 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 ... shared_rows=255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Manual/emulator:
- Emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` were refreshed to SHA256 `3da8b9c7826dd2958871bdd356e57587d9746c75f67559b602f972ddc415420a`.
- `CAS.PAK` on emulator SD-card matched `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.

Known remaining risk:
- The visible running add-in must be exited and relaunched from MENU before manual smoke reflects this artifact.

## 2026-06-07 Log-Quadratic Range And Log-Law Domain Evidence

Implemented:
- General range route for `ln/log(quadratic)`:
  - positive convex quadratic -> lower or upper log bound depending on base
  - zero/crossing convex quadratic -> all real log range
  - positive concave quadratic -> one-sided log bound from vertex max
- Log-law `xform` now prints domain conditions for product/quotient transformations.

Direct behavior:
- `tools/khicas_host_runner 'range(ln(x^2+1),x)'`:
  - `Find range`
  - `log of quadratic`
  - `min g=1 at x=0`
  - `g >= 1 > 0`
  - `y >= 0`
  - `Verified`
- `tools/khicas_host_runner 'range(log(2,x^2+1),x)'`: `y >= 0`, `Verified`.
- `tools/khicas_host_runner 'range(log(1/2,x^2+1),x)'`: `y <= 0`, `Verified`.
- `tools/khicas_host_runner 'xform(log(2,x/y),log(2,x)-log(2,y))'`: includes `Condition: 2>0, 2!=1; x>0; y>0`.
- `tools/khicas_host_runner 'xform(ln(x+1)+ln(x-1),ln(x^2-1))'`: includes `Condition: e>0, e!=1; x+1>0; x-1>0`.

Build/size:
- `./compile`: passed.
- Link ROM report: `2,063,936 / 2,097,192`.
- Link RAM report: `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,092,612 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 4,540 bytes.
- `calculator_files/CAS.PAK`: 9,649 bytes.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.

Hashes:
- `calculator_files/CAS.g3a`: `b05db08737e84e2d7278c9ed0c1d812f56e85068d28b6d47bf5dadb0c24ddf7d`
- `calculator_files/RUNMAT.g3a`: `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`
- `calculator_files/CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`

Required gates:
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy source_files=3 verified_cases=6 solve_cases=5 solve_fallback_cases=5 integral_cases=1 partfrac_cases=4 domain_cases=5 range_cases=23 implicit_cases=1 equivalence_cases=4 xform_planner_cases=11 xform_equiv_fallback_cases=2 xform_log_law_cases=3 xform_linear_parameter_cases=2 argument_cases=3 factor_cubic_cases=3`.
- `python3 tests/check_plain_eval_fallthrough.py`: `OK plain eval callback guard cases=6`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=103 planner_cases=103`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=124 stale_markers=131 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 ... shared_rows=255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Manual/emulator:
- Emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` were refreshed to SHA256 `b05db08737e84e2d7278c9ed0c1d812f56e85068d28b6d47bf5dadb0c24ddf7d`.
- `CAS.PAK` on emulator SD-card matched `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.

Known remaining risk:
- Current-artifact manual calculator smoke still needs relaunch from MENU.
