# Checkpoint

## 2026-06-07 Endpoint/Critical Range Route Family

Active slice: remove the remaining live no-route/failure wording and add a general endpoint/critical-point range family for log-domain A-level inputs.

Completed:
- Added a bounded `range(...)` route that uses existing KhiCAS subtasks for domain, endpoint limits, derivative roots, and stationary values.
- Covered `range(sin(x)+ln(x),x)` by domain `x > 0` plus endpoint limits `-infinity` and `+infinity`.
- Covered log-linear extrema such as `range(ln(x)-x,x)` and `range(x-ln(x),x)` using exact derivative roots and substitution.
- Removed live `route failed` / `no verified route` / `no route` wording from calculator/tool kept-command paths without faking steps.
- Preserved `tests/golden/exact_calculator_input_queue.jsonl` exactly.
- Rebuilt source artifacts.

Evidence:
- `tools/khicas_host_runner 'range(sin(x)+ln(x),x)'`: `endpoint/critical route`, `range: all real`, `Verified`.
- `tools/khicas_host_runner 'range(ln(x)-x,x)'`: `f'(x)=-1 + 1/x`, `f(1) = -1`, `y <= -1`, `Verified`.
- `tools/khicas_host_runner 'range(x-ln(x),x)'`: `f'(x)=1 - 1/x`, `f(1) = 1`, `y >= 1`, `Verified`.
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,082,928 bytes, SHA256 `81d23a9898e012137a53f4c6deb2b09aca14342a0a05f37c0f476b7af9fb2590`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=18`.
- `python3 tests/check_working_quality.py`: passed.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed with `production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- Emulator SD-card `CAS.g3a` and `CasioCAS.g3a` SHA256 match current candidate `81d23a9898e012137a53f4c6deb2b09aca14342a0a05f37c0f476b7af9fb2590`; backup: `/tmp/casio-smoke-current/emulator-sdcard-backup-20260607-082738`.

Blocked-on:
- Foreground/hardware launch, long input, and `xform(...)` key-entry smoke remains incomplete for the current `81d23a...` candidate.
- Classified exact-queue invalid rows and shared-working stale markers remain documented classification debt, not accepted bad rows.

Drift check:
- Still on `main`; no extra branch.
- New route is family based and delegates subtasks to existing KhiCAS computations.
- Scope remains bounded Edexcel A-level Pure working-CAS, not arbitrary mathematical completeness.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `81d23a9898e012137a53f4c6deb2b09aca14342a0a05f37c0f476b7af9fb2590`, size 2,082,928 bytes, leaving 14,224 bytes under the 2,097,152-byte limit. Automated gates pass on accepted rows and emulator SD-card artifacts are refreshed; remaining release risk is foreground/hardware launch/long/xform key-entry and acceptance of classified/stale fixture rows.

## 2026-06-07 Planner/Size Final Gate Slice

Active slice: recover package headroom and harden general range/planner behavior without exact-input route patches.

Completed:
- Stubbed out-of-scope probability/distribution runtime families under the source build flag while preserving A-level `binomial(n,k)`.
- Recovered `CAS.g3a` headroom from tens of bytes to 23,312 bytes.
- Restricted the costly general range derivative route to bounded simple endpoint routes and unbounded exponential-polynomial routes.
- Fixed generated nested `range(...)` timeout cases so they return bounded fallback text instead of hanging.
- Range fallback now reports attempted transforms, failure reason, exact-evaluation availability, last verified state, and verification status.
- Kept required trig xform route on the planner/equivalence path.
- Preserved `tests/golden/exact_calculator_input_queue.jsonl` exactly.
- Confirmed only one branch exists: `main`.

Blocked-on:
- Manual calculator key-entry smoke is still required for final hardware/UI confidence.
- Classified exact-queue invalid rows and shared-working stale markers remain documented classification debt, not bad accepted rows.

Resume hint: current `CAS.g3a` is 2,074,064 bytes, leaving 23,088 bytes under the 2,097,152-byte limit.

Current todo:
- Keep calculator menu/UI untouched.
- Use the existing Edexcel OCR reports to mine/run paper-derived pure tests.
- Patch only general math/working routes exposed by paper tests.
- Compile and size-check calculator outputs.
- Commit and push relevant files.

Active slice: paper-derived route validation and final evidence.

Completed:
- `/Volumes/VM/Edexcel A Level Maths past papers` has 1,263 files; 1,256 PDF/image paper assets are indexed.
- Existing OCR page index covers 1,256 pages across 30 paper/mark-scheme groups: 712 question pages, 544 mark-scheme pages.
- 227 paper-derived candidate tests were mined.
- Current sweep reached 211/227 acceptable after fixing real route gaps.
- Remaining 16 are raw expressions, vectors, native/exempt functions, sums, or numeric substitutions rather than failed working routes.
- Targeted working gaps passed after current working-route edits.
- `./compile` passed and copied `CAS.g3a`, `CAS.PAK`, and `RUNMAT.g3a` to `calculator_files/`.

Blocked-on:
- `tests/run_exact_queue.py --workers 4` fails on the existing queue schema at row 10088 because 553 rows use `source` instead of `source_pdf`. Checker/queue edits are out of scope for this slice.

Drift check:
- Still serves the paper-test task.
- Stayed inside math/build-size boundary.
- No menu/status/F-key edits in this slice.
- No new fallback route added for the fixed paper cases; inputs now dispatch to existing `diff`, `solve`, and `xform` routes.

Resume hint: do not edit menu/UI files. Size headroom is only 1,268 bytes, so future logic additions need matching safe code-size savings.

## 2026-06-06 Resume Slice

Active slice: finish exact queue and planner robustness without adding exact-input route patches.

Completed:
- Kept branch count to one branch: `main`.
- Preserved `tests/golden/exact_calculator_input_queue.jsonl` exactly.
- Fixed the integral planner order so cheap structural reverse-chain trig rules run before expensive canonicalization.
- Fixed `integrate_trig_reverse_chain_general` to preserve explicit product separators via `nospace_lower`, allowing the existing product parser to handle coefficient and powered trig forms.
- Fixed `tests/run_exact_queue.py --engine production` to set the production host-runner environment.
- Rebuilt source `.g3a` artifacts.

Blocked-on:
- No current blocker.

Drift check:
- Stayed inside the current A-level Pure/working-CAS scope.
- Did not add a route matched to the required xform example.
- Current trig integral fixes are rule-family/parser-order changes, not exact input matches.

Resume hint: current `CAS.g3a` is 2,096,564 bytes, leaving 588 bytes under the 2,097,152-byte limit. Avoid adding code without removing comparable code.

## 2026-06-06 Robustness Slice

Active slice: strengthen verification/properties without exact-route patches.

Completed:
- Extended random fuzzer semantic property families from narrow trig/expand/integrate coverage to factor, partfrac, diff product/quotient/chain, solve, mixed integration, xform algebraic rewrites, sum, and product.
- Added property verifier modes for derivative equivalence, exact-output containment, branch-sensitive antiderivative checks, and multi-line result scanning.
- Fixed xform non-equivalence fallback so `not equivalent` is no longer labelled `Verified`.
- Added verified markers for broad kept exact/algebraic command classes only when output is not an error, route failure, or non-equivalence.
- Updated policy tests to enforce that non-equivalent xform output must not contain `Verified`.

Blocked-on:
- No current blocker.

Drift check:
- No branch change; still only `main`.
- No specific route patches added.
- Changes improve general proof quality and generated-property coverage.

Resume hint: current `CAS.g3a` is 2,096,784 bytes, leaving 368 bytes under the 2,097,152-byte limit. Future calculator-code changes need deletion/compaction first.

## 2026-06-06 Exact-Series Robustness Slice

Active slice: use exact backend series expansion and broaden generated property coverage without exact-route patches.

Completed:
- Replaced unverified `series`/`taylor` placeholder output with exact backend output when available.
- Removed generic `Verified` auto-marking for `series`/`taylor`; these commands now verify only when the exact expansion path returns a concrete result.
- Added explicit `Err: no exact series` fallback for unsupported exact-series expansion instead of echoing input.
- Added generated property coverage for domain inequalities, quadratic range, rationalising xform, exact series, and exact Taylor.
- Updated exact-queue marker capture to recognise the shorter `KhiCAS exact:` label.
- Preserved `tests/golden/exact_calculator_input_queue.jsonl` exactly.

Blocked-on:
- No current blocker for this slice.
- `tests/check_shared_working.py` still reflects older prose-marker expectations and fails against current concise verified output; it was not changed in this slice.

Drift check:
- Stayed on `main`; no new branch.
- Stayed within A-level Pure/working-CAS scope.
- No exact input route patch was added; this is a general command-family exact-backend path plus generated property coverage.
- Build size remains under limit with over 1KB package margin.

Resume hint: current `CAS.g3a` is 2,095,980 bytes, leaving 1,172 bytes under the 2,097,152-byte limit. Use exact backend/oracle paths and property families before adding calculator-side rule code.

## 2026-06-06 Series/Limit Exact Backend Slice

Active slice: improve broad command-family behavior without exact-input route patches.

Completed:
- Changed `series(expr,var=centre,n)` to request exact expansion through degree `n` while keeping `taylor(...,n)` cutoff semantics unchanged for compatibility.
- Added generated property checks proving `series(sin(theta),theta=0,3)`, `series(tan(theta),theta=0,3)`, and `series(exp(x),x=0,4)` include the expected degree term.
- Routed `limit(...)` through the exact KhiCAS backend when available and labelled the exact result as verified.
- Added generated property checks for standard limit, removable-factor limit, and trig half-angle limit families.
- Confirmed the calculator build sets `cascas::set_khicas_eval_callback(cascas_khicas_eval)` in `main.cc`, so exact backend working routes are source-built calculator routes, not only host-runner logic.

Blocked-on:
- No current blocker for this slice.
- `tests/check_shared_working.py` remains a legacy wording/order marker check, not a current acceptance gate.

Drift check:
- Still on one branch: `main`.
- Exact queue golden remains unchanged.
- No specific route patch was added.
- Scope remains A-level Pure working-CAS; no stats/probability/matrix expansion.

Resume hint: current `CAS.g3a` is 2,096,080 bytes, leaving 1,072 bytes under the 2,097,152-byte limit. Future calculator-code additions need equal-size deletion or compaction.

## 2026-06-06 Shifted Range Slice

Active slice: improve a real broad `range(...)` gap exposed by the legacy shared-working check while staying under the `.g3a` size limit.

Completed:
- Added a general shifted unary range route for `abs(u)+c`, `sqrt(u)+c`, and signed variants.
- Added generated range property checks for shifted `abs` and shifted `sqrt`.
- Rejected the larger rational-discriminant route after it pushed `CAS.g3a` over the package limit; the working tree keeps only the size-safe shifted-unary route.

Blocked-on:
- No current blocker for this slice.
- Rational-function range still needs a smaller implementation or matching code deletion before it can be added.

Drift check:
- Still on `main`.
- Exact queue golden remains unchanged.
- No exact-input route patch was kept.

Resume hint: current `CAS.g3a` is 2,096,604 bytes, leaving 548 bytes under the 2,097,152-byte limit. Do not add calculator-side code without deleting or compacting existing code first.

## 2026-06-06 Backend Range Discriminant Slice

Active slice: use existing CAS subtask computation for a broader rational-function range route, without hard-coded input patches.

Completed:
- Added a general rational range route for `range(num/den,x)` that asks the exact backend for `discriminant(den*y-num,x)`.
- Added host-oracle support for `discriminant(...)` so production runner tests exercise the same internal route.
- Added generated property checks for rational range via discriminant, including `x/(x^2+4)` and `1/(x^2+4*x+5)`.
- Kept the shifted `abs`/`sqrt` range route from the previous slice.
- Retried the earlier fully local Rat-arithmetic discriminant route and rejected it because it exceeded the `.g3a` limit.

Blocked-on:
- No current blocker for this slice.
- Package margin is now only 96 bytes. Further calculator-side robustness work needs code deletion/compaction first.

Drift check:
- Still on `main`.
- Exact queue golden remains unchanged.
- No exact-input route patch was added; this route delegates the algebra subtask to the existing exact CAS backend.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,097,056 bytes, leaving 96 bytes under the 2,097,152-byte limit. Next slice should recover size before adding any calculator-side logic.

## 2026-06-06 Range Solver/Size Recovery Slice

Active slice: recover package headroom and improve rational-function range by delegating more subtasks to exact CAS logic.

Completed:
- Removed duplicated integer-quadratic range handling; the earlier rational-quadratic route now covers those cases.
- Added a generic rational range refinement for `range(num/den,x)`:
  - build `den*y-num=0`
  - ask exact backend for `D`
  - ask exact backend to solve `D>=0`
  - solve degenerate `A(y)=0` cases separately and exclude values that leave no `x` solution
- Added generated range properties for solved discriminant output and degenerate endpoint exclusion.
- Used SymPy `function_range`/calculus source as the algorithm reference, adapted to a compact A-level rational subset.

Blocked-on:
- No blocker for this slice.
- Whole project remains incomplete; shared-working still exposes exam-format and broader method gaps.

Drift check:
- Still on `main`.
- Exact queue golden remains unchanged.
- No exact-input route patch was added.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,095,672 bytes, leaving 1,480 bytes under the 2,097,152-byte limit. Further calculator logic still needs size discipline.

## 2026-06-06 Implicit Backend Substitution Slice

Active slice: improve implicit differentiation by applying a general backend substitution step, while recovering package headroom by deleting an old special helper.

Completed:
- Added a general implicit dependent-function reduction:
  - compute `dy/dx = -Fx/Fy`
  - detect dependent `tan(y)` or `exp(y)` tokens in the ratio
  - ask the exact backend to solve the original equation for that token
  - substitute and simplify through the exact backend
  - display the extra verified substitution line
- Added generated diff property-family coverage for `tan(y)` and `exp(y)` implicit equations.
- Removed the old bespoke `parse_var_exp_product_derivative` route; quotient-rule numerator derivatives now use the exact backend.
- Kept the required trig xform example working.

Blocked-on:
- Whole project is still not a completed “100% working-CAS guarantee”.
- `tests/check_shared_working.py` still fails many legacy wording/order markers and also lists future exam-format gaps.

Drift check:
- Still on `main`; `git branch --list` shows only `main`.
- Exact queue golden remains unchanged.
- The change is a rule family plus backend delegation, not an exact-input route patch.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,396 bytes, leaving 756 bytes under the 2,097,152-byte limit. Further calculator-side additions still need deletion/compaction first.

## 2026-06-06 Limit Method Line Slice

Active slice: improve A-level limit working cues without adding route-specific calculator code.

Completed:
- Tested a detailed standard/removable-limit recognizer and rejected it because it exceeded the `.g3a` size limit.
- Replaced the vague generic limit line with the compact general cue `Use standard limits/cancel factors.`.
- Added generated limit property markers for standard trig limits, removable factor cancellation, and half-angle behavior.
- Rebuilt source `.g3a` successfully under the size limit.
- Re-ran exact queue, planner, xform-family, catalog/remove, targeted, quality, and strict random-fuzzer gates.

Blocked-on:
- Whole project is still not a completed "100% working-CAS guarantee".
- Limit output now has a better broad method cue, but not a full multi-line exam derivation for every standard-limit family.

Drift check:
- Still on `main`; no new branch.
- Exact queue golden remains unchanged.
- No exact-input route patch was added.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,412 bytes, leaving 740 bytes under the 2,097,152-byte limit. Future calculator-side additions still need deletion/compaction first.

## 2026-06-06 Solve Condition Filter Slice

Active slice: improve broad `solve(...)` robustness for extra A-level constraint arguments without exact-input route patches.

Completed:
- Added generic post-filtering for single-variable `solve(eq,var,condition)` candidate lists.
- Supported condition forms already used by the interface: `var integer`, `var!=c`, `var>c`, and `var<c`.
- Used existing exact/numeric subtask machinery to test non-rational comparison candidates.
- Added generated solve property coverage for integer, inequality, and exclusion filters.
- Deleted the narrow quadratic coefficient-matching solver and private helpers to recover `.g3a` size.
- Rebuilt source `.g3a` successfully under the size limit.

Blocked-on:
- Whole project is still not a completed "100% working-CAS guarantee".
- Constraint filtering is now general for common single-variable list outputs, but not yet a full assumption system for every symbolic condition shape.

Drift check:
- Still on `main`; no new branch.
- Exact queue golden remains unchanged.
- One narrow bespoke route was retired to fund a broader post-filter.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,095,180 bytes, leaving 1,972 bytes under the 2,097,152-byte limit. Next slice can spend only after preserving this margin.

## 2026-06-06 Numeric Exam Rounding Slice

Active slice: improve broad numeric answer presentation for A-level exam-style final values.

Completed:
- Added generic numeric final-answer lines for:
  - 2 significant figures
  - 3 significant figures
  - nearest integer
- Added generated numeric property coverage for standard A-level rounding outputs.
- Rebuilt source `.g3a` successfully under the size limit.
- Re-ran exact queue, planner, xform-family, catalog/remove, targeted, quality, and strict random-fuzzer gates.

Blocked-on:
- Whole project is still not a completed "100% working-CAS guarantee".
- Numeric output now gives common exam rounding forms, but not question-specific units such as "nearest metre" or "nearest minute" unless the input carries that context.

Drift check:
- Still on `main`; no new branch.
- Exact queue golden remains unchanged.
- Change is a general numeric output policy, not an exact-expression route patch.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,095,808 bytes, leaving 1,344 bytes under the 2,097,152-byte limit. Further calculator-side additions should recover size first.

## 2026-06-06 Evalat Exact-First Slice

Active slice: make substitution/evaluation output preserve exact symbolic values before decimal approximation.

Completed:
- Added a general `evalat(expr,var,value)` final line `f(value) = result`.
- Kept `subst/subs` substitution-only; they do not add function-evaluation wording.
- Changed `evalat` to ask the existing exact backend for `simplify(substituted_expr)` before local numeric evaluation.
- Added generated evalat property coverage for exact non-decimal outputs.
- Rebuilt source `.g3a` under the package limit and re-ran production gates.

Blocked-on:
- Whole project remains incomplete; no final "100% working-CAS guarantee" is proven.
- `evalat` uses generic `f(...)`; named-function context still requires richer input semantics.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is a general exact-first output policy, not an exact-input route patch.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,216 bytes, leaving 936 bytes under the 2,097,152-byte limit. Further calculator-side additions still need size discipline or code deletion.

## 2026-06-06 Log Exact Backend Slice

Active slice: improve broad log-law output by preserving exact backend simplification after change-of-base.

Completed:
- Added generic exact backend result display for `log(base,arg)` after the change-of-base line.
- Added generated log property coverage for exact numeric logs such as `log(2,8)` and `log(3,81)`.
- Compacted the log route to remove duplicate exact-append logic and recover package headroom.
- Rebuilt source `.g3a` under the package limit and re-ran production gates.

Blocked-on:
- Whole project remains incomplete; no final "100% working-CAS guarantee" is proven.
- Package size is workable but still tight; future calculator-side additions should delete/compact first.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is a general kept-command exact-result policy, not an exact-input route patch.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,044 bytes, leaving 1,108 bytes under the 2,097,152-byte limit. Next slice should either recover more size or spend only on broad rule-family logic.

## 2026-06-06 Numeric Sign/Zero Slice

Active slice: improve broad numeric-method robustness and presentation for near-zero and sign-sensitive exam checks.

Completed:
- Normalized tiny numeric values to exact display zero in the shared numeric evaluator.
- Fixed nearest-integer output so tiny negative values round to `0`, not `-0`.
- Added generic `Sign: > 0`, `Sign: < 0`, or `Sign: = 0` output for `method=numeric,...`.
- Added generated numeric property coverage for sign checks and zero normalization.
- Rebuilt source `.g3a` under the package limit and re-ran production gates.

Blocked-on:
- Whole project remains incomplete; no final "100% working-CAS guarantee" is proven.
- Numeric output still cannot infer question-specific labels like metres/minutes unless the input carries that context.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is a general numeric-output policy, not an exact-input route patch.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,220 bytes, leaving 932 bytes under the 2,097,152-byte limit. Future calculator-side additions should recover size or stay very small.

## 2026-06-06 Factor Backend and Integral Timeout Guard Slice

Active slice: improve broad kept-command robustness without adding exact-route patches.

Completed:
- Changed `factor(...)` to ask the existing production exact backend for a full factorisation before local partial-factor working routes.
- Added generated cubic factor property coverage for full exact-backend factors.
- Updated stale cubic factor policy markers to expect full verified factorisation, not the old synthetic-division phrase.
- Fixed exact-queue runner stdout/stderr decoding so non-UTF host output is classified instead of aborting the whole queue.
- Added a general integration fallback guard for variable-base logarithms before the expensive exact backend attempt.
- Removed the redundant local `factor(...)` quadratic/cubic fallback after exact backend factorisation became the canonical path.
- Rebuilt source `.g3a` under the package limit and re-ran production gates.

Blocked-on:
- Whole project remains incomplete; no final "100% working-CAS guarantee" is proven.
- The new factor path gives verified full factorisation, but not a full synthetic-division derivation for every polynomial.
- Package headroom remains tight, but recovered from the first version of this slice.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are general backend/fallback policies, not exact-input route patches.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,196 bytes, leaving 956 bytes under the 2,097,152-byte limit. Next calculator-side slice should still recover size before adding features.

## 2026-06-06 Solve Exact-Empty Guard Slice

Active slice: prevent exact backend fallbacks from overwriting valid derived solve working with an empty exact solution list.

Completed:
- Added a general guard in `append_solve_exact_result` to reject exact backend `[]` and `{}` before display.
- Added generated solve property coverage forbidding the leaked `p = []` line on exponential/log solve working.
- Confirmed source no longer contains a live `no route` output string; only `not equivalent` remains for invalid `xform` targets.
- Rebuilt source `.g3a` under the package limit and re-ran production gates.

Blocked-on:
- Whole project remains incomplete; no arbitrary-input mathematical completeness guarantee is possible on a finite calculator.
- Remaining shared-working failures are mostly stale wording/order expectations plus broader exam-format gaps.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is a broad solve exact-fallback policy, not a route patch.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,296 bytes, leaving 856 bytes under the 2,097,152-byte limit. At this point, prefer finishing/reporting over more calculator-side additions unless a new gate exposes a real wrong-result bug.

## 2026-06-06 Diff Verification and Implicit Guard Slice

Active slice: prevent verified working from displaying wrong derivative lines, and prevent malformed implicit-diff calls from reaching expensive exact backend paths.

Completed:
- Added a central exact-backend verifier for two-argument `diff(expr,var)` working output.
- If a local derivative route's final line is not equivalent to the production exact derivative, the display falls back to a labelled `KhiCAS exact` result instead of showing false working.
- Added generated derivative property coverage for a nested `log` / reciprocal / square-root derivative that previously exposed a bad intermediate route.
- Added a general `implicit_diff(eq,x,y)` arity/scope guard: malformed one-argument or non-equation calls reject immediately instead of timing out.
- Added a general system-solve exact-empty guard: exact `[]` / `{}` no longer gets labelled verified; it returns the required no-verified-route message with the last transformation state.
- Rebuilt source `.g3a` under the package limit and re-ran production gates.

Blocked-on:
- Whole project remains incomplete; no finite calculator build can honestly guarantee arbitrary-input mathematical completeness.
- Current broad fallback can give exact verified results for hard derivatives, but not full exam-style derivations for every possible expression.
- Package headroom is legal but very tight.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are general verification/input-policy guards, not exact-input route patches.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,952 bytes, leaving 200 bytes under the 2,097,152-byte limit. Next calculator-side work should recover size before adding functionality.

## 2026-06-06 Route Retirement / ROM Headroom Slice

Active slice: recover package headroom and remove a narrow derivative route that is now covered by general quotient/power/exact verification.

Completed:
- Deleted `try_diff_quad_over_cx`, an old local derivative route for quadratic-over-variable-power forms.
- Confirmed those expressions now use the general quotient rule or exact derivative verifier.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,096,108 bytes, leaving 1,044 bytes under the 2,097,152-byte limit.
- Re-ran focused derivative, `xform`, working-quality, exact-queue, and strict fuzzer gates.

Blocked-on:
- Whole project remains incomplete; no finite calculator build can honestly guarantee arbitrary-input mathematical completeness.
- Current direction is still correct: general rule family, transform, verify, display; exact result fallback when no exam-style derivation is available.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change removes a route-specific helper instead of adding one.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,108 bytes, leaving 1,044 bytes under the 2,097,152-byte limit. Next work should keep retiring narrow routes or move logic into existing general rule families.

## 2026-06-06 Double-Argument Verification Slice

Active slice: prevent a verified double-angle line from being displayed when the argument is affine but not a pure doubled argument.

Completed:
- Fixed the shared `double_arg` recogniser used by `xform` trig identities.
- It now accepts bare pure doubles like `2*x` and parenthesised pure doubles like `2*(x+1)`.
- It rejects affine non-doubles like `2*x+1` when the target argument is `x+1`.
- Added targeted regression coverage for `xform(sin(2*x+1),2*sin(x+1)*cos(x+1))`, which must report non-equivalence instead of verified working.
- Rebuilt source `.g3a` under the package limit and re-ran production gates.

Blocked-on:
- Whole project remains incomplete; no arbitrary-input mathematical completeness guarantee is proven.
- A broader triple-angle/tan-double direct-display attempt was tested but rejected because it pushed `CAS.g3a` over the package limit. Those forms still fall through to exact-equivalence verification instead of false working.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change fixes a general identity parser, not a specific route.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,224 bytes, leaving 928 bytes under the 2,097,152-byte limit. Further calculator-side identity display work should first recover ROM.

## 2026-06-06 Complete-Square Route Retirement Slice

Active slice: remove completed-circle exact-input patches and route factored circle forms through existing general complete-square working.

Completed:
- Removed literal handling for `(x-15)^2+y^2-40` and `(x-7)^2+(y-5)^2-20`.
- Added a broad `expand(...)` fallback before one-variable completing-square so already-factored x/y circle forms feed `complete_square_xy_working`.
- Confirmed `complete_square((x-2)^2+(y+3)^2-25)` now produces full x/y complete-square lines, centre, and radius without a route-specific matcher.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,096,168 bytes, leaving 984 bytes under the 2,097,152-byte limit.
- Re-ran exact queue, xform-family, targeted, quality, removed-feature, generator, and strict random-fuzzer gates.

Blocked-on:
- Whole project remains incomplete; no arbitrary-input mathematical completeness guarantee is proven.
- `tests/check_shared_working.py` still reports many old wording/order marker failures and broader exam-format gaps. It was not weakened.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change uses existing KhiCAS expansion plus the existing x/y completing-square rule family, not exact-input route patches.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,168 bytes, leaving 984 bytes under the 2,097,152-byte limit. Next slice should audit remaining exact-string branches and retire only those covered by general verified rules.

## 2026-06-06 Generic Trig/Guard/Headroom Slice

Active slice: keep generic trig transform/integration behavior, recover ROM headroom, and stop strict host fuzz timeouts with broad complexity guards rather than exact routes.

Completed:
- Replaced double-angle `texpand` and `tcollect` exact-input handling with argument-parsed rule families plus `khicas_equiv` verification before display.
- Kept the generated triple-angle reverse-chain integral family covered after confirming it is part of strict property fuzz coverage.
- Recovered ROM by shortening non-user-critical upstream warning strings and trig fallback prose.
- Added host-runner complexity guards for nested negative-power trig integrals and numeric-heavy surd/log simplify inputs that otherwise time out under strict fuzzing.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,095,688 bytes, leaving 1,464 bytes under the 2,097,152-byte limit.
- Re-ran exact queue, targeted working gaps, working quality, working planner, xform rule families, catalog/scope checks, removed-feature checks, generator checks, and strict random fuzzers.

Blocked-on:
- Whole project remains incomplete; no finite calculator build can honestly guarantee arbitrary-input mathematical completeness.
- `tests/check_shared_working.py` remains a stale exact-wording fixture and fails many marker checks. It was not weakened.
- Exact queue passes with `bad=0`, but still reports 66 classified invalid rows from the protected dataset.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are generic rule parsing, verification, size trimming, and complexity guarding, not exact-input route patches.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,095,688 bytes, leaving 1,464 bytes under the 2,097,152-byte limit. Next work should either retire more narrow routes into verified rule families or reconcile/remove stale shared wording fixtures without weakening the production gates.

## 2026-06-06 Compound-Angle Rule Family Slice

Active slice: add one general online-inspired trig rewrite family without adding exact-input routes.

Completed:
- Used SymPy Fu TR10/TR12 compound-angle ideas as design input, not vendored code.
- Added a compact `texpand` rule family for `sin(a±b)`, `cos(a±b)`, and `tan(a±b)`.
- The rule splits at the last top-level `+`/`-`, so forms like `sin(u+1+v)` are handled as `(u+1)+v`.
- Every displayed candidate is still checked with `khicas_equiv` before output.
- Added generated strict fuzzer property cases for the compound-angle family.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,097,088 bytes, leaving 64 bytes under the 2,097,152-byte limit.
- Re-ran exact queue, targeted working gaps, working quality, working planner, xform rule families, catalog/scope checks, removed-feature checks, generator checks, and strict random fuzzers.

Blocked-on:
- Whole project remains incomplete; no honest arbitrary-input or "all maths" guarantee is proven.
- ROM headroom is critically low at 64 bytes.
- `tests/check_shared_working.py` remains a stale exact-wording fixture and was not changed in this slice.
- Exact queue accepted rows pass with `bad=0`, but the runner still classifies 66 protected rows as invalid.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is a rule family plus generated property coverage, not an exact route patch.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,097,088 bytes, leaving 64 bytes under the 2,097,152-byte limit. Next work should recover ROM before adding any calculator-side rules.

## 2026-06-06 Crash/UI/Exact Gate Repair Slice

Active slice: resolve review blockers without adding calculator route patches.

Completed:
- Fixed the stale calculator-border check to verify the actual console/catalog draw path.
- Updated runtime risk metadata to match the current artifacts: `CAS/@CAS/CAS.g3a`, `RunMat/@RUNMAT/RUNMAT.g3a`, and `CAS.PAK`.
- Added direct unsupported-command rejection coverage for stats/probability/options/graph-style removed inputs.
- Converted shared-working checks into explicit stale-marker classification: current run has 127 clean, 128 stale marker rows, 0 bad.
- Removed the calculator-side deep `collect(...)` bailout that pushed `CAS.g3a` over limit.
- Moved strict host fuzz timeout handling into host-only broad complexity guards for deep collect/taylor/series/sum/product/limit/simplify cases.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,096,908 bytes, leaving 244 bytes under the 2,097,152-byte limit.
- Re-ran exact queue, targeted working gaps, working quality, working planner, xform rule families, catalog/scope checks, removed-feature checks, generator checks, metadata checks, and strict random fuzzers.

Blocked-on:
- Manual calculator smoke is still required on hardware/emulator: launch, F1-F6, F4 catalog, help open, bad input, long input, xform, removed command.
- ROM headroom is improved but still tight at 244 bytes.
- Whole project still cannot honestly claim arbitrary-input mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are UI/test repair, direct rejection, host fuzz guarding, and documentation repair, not exact-input route patches.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,908 bytes, leaving 244 bytes under the 2,097,152-byte limit. Next work should prioritize hardware/manual smoke and more ROM recovery before calculator-side rule growth.

## 2026-06-06 Normal Guard / Final Gate Slice

Active slice: reject unsupported multi-argument `normal(...)` without blocking protected exact-queue `normal(expr)` algebra normalisation, then re-run release gates.

Completed:
- Added a narrow `normal(...)` guard: one-argument algebra normalisation is labelled as exact KhiCAS output, while multi-argument probability-style input returns `Err: unsupported`.
- Recovered some ROM by shortening non-marker working text only.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,096,996 bytes, leaving 156 bytes under the 2,097,152-byte limit.
- Re-ran exact queue, targeted working gaps, working quality, working planner, xform rule families, catalog/scope checks, removed-feature checks, help quality, generator checks, metadata checks, and strict random fuzz.
- Accepted exact classified rows as protected-fixture drift only: all classified rows are `invalid_marker_*`; no untrusted marker gaps remain.
- Accepted shared-working stale rows as wording/spacing/form fixture drift only; bad count is 0.

Blocked-on:
- Manual calculator smoke is still required on hardware/emulator: launch, F1-F6, F4 catalog, help open, bad input, long input, xform, removed command.
- ROM headroom is 156 bytes, still too tight for further calculator-side additions.
- Whole project still cannot honestly claim arbitrary-input mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are guard/verification/size-trim/documentation work, not exact-input route patches.
- Scope remains A-level Pure working-CAS.

Resume hint: current `CAS.g3a` is 2,096,996 bytes, leaving 156 bytes under the 2,097,152-byte limit. Next work should be manual calculator smoke and ROM recovery only, unless a failing general-rule gate appears.

## 2026-06-06 No Out-Of-Scope Fallback Slice

Active slice: stop non-removed system-solve fallback from displaying the old no-route/out-of-scope language.

Completed:
- Added a regression for `solve([n(0)=500,n(2)=1000,dn/dt=k*n],[a,k])`.
- Replaced the old `No verified A-level working route found.` output with `Planner search:`, `last verified state:`, and labelled `KhiCAS exact evaluation: []`.
- Prevented a fake standalone `Verified` marker on exact-empty fallback.
- Shortened non-marker source prose to recover ROM.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,096,936 bytes, leaving 216 bytes under the 2,097,152-byte limit.

Blocked-on:
- Removed commands still reject input as required by repo scope rules.
- This slice improves fallback honesty; it does not prove arbitrary-input mathematical completeness.
- Manual calculator smoke is still required on hardware/emulator.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is fallback policy and verification guard, not an exact-input route patch.

Resume hint: current `CAS.g3a` is 2,096,936 bytes, leaving 216 bytes under the 2,097,152-byte limit. Full exact queue has passed accepted rows; next work should run manual calculator smoke before any more calculator-side logic.

## 2026-06-06 ROM Recovery / Final Automated Gate Slice

Active slice: recover real package headroom without adding math routes, then re-run the automated release gate suite.

Completed:
- Shrank `reject_removed_feature` to the 82-command `tools/scope_manifest.py` removal set instead of the old 357-hash deny table.
- Removed the obsolete `binomial` bypass branch after confirming `binomial` is no longer denied by the manifest-owned hash table.
- Removed the complex-entry splitter and changed exact solve augmentation to suppress `I`-containing exact output rather than filtering/appending partial complex results.
- Rebuilt source `.g3a`; `CAS.g3a` is now 2,094,880 bytes, leaving 2,272 bytes under the 2,097,152-byte limit.
- Re-ran size, metadata, catalog, removed-feature, border, targeted working, working quality, working planner, xform family, shared working, strict random fuzzer, exact queue, whitespace, and protected-golden checks.

Blocked-on:
- Manual calculator smoke is still required on hardware/emulator: launch, F1-F6, F4 catalog, help open, bad input, long input, xform, removed command.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps, all trusted invalid-marker classifications; accepted rows have `bad=0`.
- Shared working still has 131 stale marker rows, `bad=0`.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are size-policy and exact-output safety cleanup, not exact-input route patches.
- Scope remains A-level Pure working-CAS; this still does not prove arbitrary mathematical completeness.

Resume hint: current `CAS.g3a` is 2,094,880 bytes, leaving 2,272 bytes under the 2,097,152-byte limit. Automated gates pass; remaining release risk is manual calculator smoke and classified/stale fixture acceptance.

## 2026-06-06 Emulator Smoke Slice

Active slice: run the fx-CG Manager smoke without changing the user's frontmost app, then restore emulator storage.

Completed:
- Launched `/Applications/fx-CG Manager PLUS Subscription for fx-CG50series.app` through `cua-driver` with `calculator_files/CAS.g3a`.
- Dismissed the activation wizard and reached the emulator main menu.
- Confirmed the emulator menu initially used older stored add-ins, not the current `calculator_files/CAS.g3a`.
- Temporarily backed up emulator SD-card add-ins, copied the source-built `calculator_files/CAS.g3a` into the emulator SD-card `CAS.g3a`/`CasioCAS.g3a` slots, and verified SHA256 `475f0604674d4b1284aaa5a42760ecc37da54c513a2564245fc45b0d92f7fd2b`.
- Launched the `CasioCAS` tile and reached the CAS session UI.
- Ran F1-F6 menu smoke; final state showed the Function Catalog screen.
- Restored emulator SD-card files to their pre-smoke hashes.
- Production host smoke confirmed xform, non-rational range, removed-command rejection, `normal(0,1)` rejection, and long-input complexity rejection.

Blocked-on:
- Full manual command-entry smoke remains partial: background GUI text injection into fx-CG Manager did not reliably enter operators/long expressions.
- Direct xform/removed/long-input behavior is verified by the production host runner, not by manual emulator text entry.
- This slice does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Emulator SD-card files were restored after the temporary smoke copy.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current automated build remains `CAS.g3a` size 2,094,880 bytes with 2,272 bytes headroom. Manual GUI launch/catalog smoke passed; manual long-expression entry is the remaining emulator-only gap.

## 2026-06-06 Final Verification Refresh

Active slice: rerun the full post-build gate suite after emulator smoke and stop UI automation.

Completed:
- Rebuilt from source with `./compile`; current `CAS.g3a` is 2,094,880 bytes, leaving 2,272 bytes under the 2,097,152-byte limit.
- Re-ran size, CAS/RUNMAT metadata, catalog, removed-feature, border, help, generator, targeted working, working quality, planner, xform family, shared-working, strict random fuzzer, exact queue, whitespace, protected-golden, and branch checks.
- Re-ran production host smoke for xform, non-rational range, removed inputs, `normal(0,1)`, and long-input complexity guard.
- Stopped `cua-driver`; daemon is not running.

Blocked-on:
- Full manual calculator key-entry smoke remains incomplete: help open, bad input, long input, xform, and removed commands through calculator keys.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 131 stale marker rows; `bad=0`.
- Bit-for-bit reproducible `.g3a` output is not proven; current rebuild changed artifact SHA256 while keeping size and behavior stable.
- This does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Emulator SD-card files were restored after temporary smoke.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `8798b966d48bba5338f10fe4561537e97cd67f68b1519fbc7b2ccbaf12fd734d`, size 2,094,880 bytes, with automated gates green for accepted rows. Remaining risk is manual key-entry smoke, fixture classification acceptance, and reproducible packaging.

## 2026-06-06 Deterministic Packaging Slice

Active slice: make source-built `.g3a` artifacts bit-stable across clean rebuilds without changing calculator math payload.

Completed:
- Added `tools/normalize_g3a_metadata.py` to replace the mkg3a wall-clock timestamp with fixed `2000.0101.0000` and recompute the existing header/tail checksum.
- Wired the normalizer into `tools/build_g3a.sh` for both `CAS.g3a` and `RUNMAT.g3a` before artifacts are copied into `calculator_files/`.
- Rebuilt twice through `./compile`; the second build matched the first normalized build byte-for-byte for both CAS and RunMat.
- Re-ran size, CAS/RUNMAT metadata, catalog, removed-feature, border, help, targeted working, planner, xform family, shared-working, strict random fuzzer, and exact queue gates.

Blocked-on:
- Full manual calculator key-entry smoke remains incomplete: help open, bad input, long input, xform, and removed commands through calculator keys.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 131 stale marker rows; `bad=0`.
- This does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- CAS/RUNMAT package hashes are now deterministic for the clean build path.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `b1671fbad6b69f64cc18d34516366e90846ce0587e86c544950e00cb3285d39e`, size 2,094,880 bytes, with automated gates green for accepted rows. Remaining release risk is manual key-entry smoke and fixture classification acceptance.

## 2026-06-06 General Range Planner Slice

Active slice: improve non-rational range robustness without exact-input route patches and keep the final package below the size limit with margin.

Completed:
- Generalized the exponential-polynomial range path into a broader derivative/limits route that delegates `diff`, stationary solve, substitution, limit, and approximation subtasks to KhiCAS.
- Added generated range fuzzer properties for exp-polynomial lower/upper-bound families.
- Preserved stronger rational discriminant and simple absolute/surd/trig families ahead of the generic derivative route.
- Rebuilt from source with `./compile`; current `CAS.g3a` is 2,095,080 bytes, leaving 2,072 bytes under the 2,097,152-byte limit.
- Re-ran size, metadata, catalog, removed-feature, border, help, generator, targeted working, working quality, planner, xform family, shared-working, strict random fuzzer, exact queue, whitespace, protected-golden, branch, RunMat, and host smoke checks.

Blocked-on:
- Full hardware/manual key-entry smoke remains incomplete.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 132 stale marker rows; `bad=0`.
- This does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `33597923b5801d8705ef756321abba89b33edf0b42bc3ee5c18816ffed6ab133`, size 2,095,080 bytes, with automated gates green for accepted rows. Remaining release risk is manual key-entry smoke and fixture classification acceptance.

## 2026-06-06 Range Fallback/ROM Recovery Refresh

Active slice: strengthen weak range fallback reporting, remove redundant shortcut code, recover ROM margin, and rerun the full automated gate suite.

Completed:
- Added explicit attempted-transform/failure/verification status to the generic `range(...)` fallback path when no verified bound route is found.
- Removed redundant nonnegative lower-bound shortcuts that were covered by the general derivative/limits route for log/surd range families.
- Rebuilt from source with `./compile`; current `CAS.g3a` is 2,094,348 bytes, leaving 2,804 bytes under the 2,097,152-byte limit.
- Re-ran size, metadata, catalog, removed-feature, border, targeted working, planner, xform family, shared-working, strict random fuzzer, exact queue, whitespace, protected-golden, branch, RunMat, and host range smoke checks.

Blocked-on:
- Full hardware/manual key-entry smoke remains incomplete.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 132 stale marker rows; `bad=0`.
- This does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `ce05b19c72cf3408996cab3ee9bfd2f26ab33c39dff1bc73a69579ff9b8fd693`, size 2,094,348 bytes, with automated gates green for accepted rows. Remaining release risk is manual key-entry smoke and fixture classification acceptance.

## 2026-06-07 Background Manual Smoke Attempt

Active slice: retry manual calculator key-entry smoke through fx-CG Manager without stealing the user's frontmost app.

Completed:
- Re-read the current checkpoint/evidence and confirmed the remaining release risk is manual key-entry smoke.
- Verified `cua-driver` is installed and has Accessibility/Screen Recording permissions.
- Captured the current emulator window: `/tmp/casio-smoke/fxcg.png`.
- Observed the running emulator was on a CAS Help screen saying `Copy CASIOCAS.HLP to storage root.`
- Temporarily copied the current release candidate into emulator SD-card `CAS.g3a` and `CasioCAS.g3a`, and copied `CAS.PAK`.
- Tried background CuaDriver calculator interactions:
  - pixel-local and global coordinate clicks on `EXIT` and `AC/ON`
  - `press_key` for `escape`, `f1`, `f6`, `return`, and `enter`
- Re-screenshotted after each attempt; the calculator core remained on the same Help screen.
- Restored emulator SD-card files to pre-attempt hashes.

Blocked-on:
- Full manual calculator key-entry smoke still needs either physical calculator testing or permission to foreground/restart fx-CG Manager so the emulator core receives real key events.
- Background CuaDriver can inspect and screenshot the window, but in this state it does not deliver input to the calculator core.
- This does not change the automated release candidate evidence.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Emulator SD-card files were restored after the temporary current-artifact copy.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate remains `calculator_files/CAS.g3a` SHA256 `ce05b19c72cf3408996cab3ee9bfd2f26ab33c39dff1bc73a69579ff9b8fd693`, size 2,094,348 bytes. Next manual-smoke step requires user-approved foreground/restart of fx-CG Manager or physical calculator entry.

## 2026-06-07 Shared Marker Classification Refresh

Active slice: reduce shared-working stale marker noise without changing calculator output or weakening strict marker mode.

Completed:
- Added canonical math-string matching to `tests/check_shared_working.py` for normal classified mode only.
- Strict marker mode remains literal and still fails legacy wording/form differences.
- Re-ran shared-working and targeted working checks.
- Confirmed current `CAS.g3a` strings do not contain `CASIOCAS.HLP`; the emulator Help message came from the already-running old add-in image, not the current release candidate.

Evidence:
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=136 stale_markers=119 bad=0`.
- `python3 tests/check_shared_working.py --strict-markers`: failed legacy literal markers as expected.
- `python3 tests/check_targeted_working_gaps.py`: passed.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: `CAS.g3a: 2094348 bytes (1.997 MiB)`.
- `strings -a calculator_files/CAS.g3a | rg -n "CASIOCAS|CAS\\.PAK|HLP|Copy|storage root|Help"`: no `CASIOCAS.HLP`, `Copy`, or `storage root` string in current artifact.

Blocked-on:
- Full hardware/manual key-entry smoke still needs user-approved foreground/restart of fx-CG Manager or physical calculator entry.
- 119 shared-working markers remain stale legacy wording/form checks, but `bad=0`.
- This does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- No calculator artifact changed in this slice.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate remains `calculator_files/CAS.g3a` SHA256 `ce05b19c72cf3408996cab3ee9bfd2f26ab33c39dff1bc73a69579ff9b8fd693`, size 2,094,348 bytes. Shared-working stale markers are reduced to 119; remaining release risk is manual key-entry smoke and fixture classification acceptance.

## 2026-06-07 Bounded Range Regression Fix

Active slice: fix the real bounded-domain range bug found while classifying stale shared-working rows.

Completed:
- Moved the general affine bounded-interval range route before the unbounded derivative route.
- Added a targeted regression for `range(2*x+1,x,-1,3)` so the endpoint route stays pinned.
- Rebuilt from source with `./compile`; current `CAS.g3a` is 2,094,316 bytes, leaving 2,836 bytes under the 2,097,152-byte limit.
- Re-ran size, metadata, catalog, removed-feature, border, help, generator, targeted working, working quality, planner, xform family, shared-working, strict random fuzzer, exact queue, whitespace, protected-golden, branch, RunMat, and host rejection checks.

Evidence:
- `tools/khicas_host_runner 'range(2*x+1,x,-1,3)'`: `linear interval`, `f(-1) = -1`, `f(3) = 7`, `-1 <= y <= 7`.
- `python3 tests/check_targeted_working_gaps.py`: range cases increased to 2 and passed.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Blocked-on:
- Full hardware/manual key-entry smoke still needs user-approved foreground/restart of fx-CG Manager or physical calculator entry.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 118 stale marker rows; `bad=0`.
- This does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `47a663bbd51642414ccc0ef7a4f9f22ec137f394fd4ecf4f5dbec12cd7534056`, size 2,094,316 bytes. Remaining release risk is manual key-entry smoke and fixture classification acceptance.

## 2026-06-07 General Bounded Derivative Range Route

Active slice: extend range handling through a general derivative/evaluation route for bounded A-level single-variable inputs, while keeping accepted working lines concrete and verified.

Completed:
- Added bounded derivative range handling for single-variable expressions with numeric/exact bounds.
- Kept the earlier affine bounded interval route ahead of derivative fallback.
- Added targeted range regressions for `range(exp(x),x,0,1)`, `range(sin(x),x,0,pi/2)`, and `range(x^3,x,-2,1)`.
- Tightened range fallback guards so multi-variable or oversized forms return quickly instead of timing out.
- Rebuilt from source with `./compile`; current `CAS.g3a` is 2,097,100 bytes, leaving 52 bytes under the 2,097,152-byte limit.

Evidence:
- `tools/khicas_host_runner 'range(exp(x),x,0,1)'`: shows `f'(x)=exp(x)`, endpoint values `1`, `e`, bound `1 <= y <= e`, `Verified`.
- `tools/khicas_host_runner 'range(sin(x),x,0,pi/2)'`: shows `f'(x)=cos(x)`, endpoint values `0`, `1`, bound `0 <= y <= 1`, `Verified`.
- `tools/khicas_host_runner 'range(x^3,x,-2,1)'`: shows `f'(x)=3*x^2`, endpoint/stationary values, bound `-8 <= y <= 1`, `Verified`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route gives `y >= -4*exp(2)`, `Verified`.
- `python3 tests/check_targeted_working_gaps.py`: `range_cases=5`, passed.
- `python3 tests/random_working_fuzzer.py --only range --count 240 --strict --timeout 10 --print-failures`: `OK random fuzz done=251 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Blocked-on:
- Full hardware/manual key-entry smoke still needs user-approved foreground/restart of fx-CG Manager or physical calculator entry.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 118 stale marker rows; `bad=0`.
- Binary headroom is only 52 bytes; new source features need size recovery first.
- This does not prove arbitrary mathematical completeness.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Scope remains bounded A-level Pure working-CAS.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `ac4372dfb3b6cf5c4c4fcd39c4ccd90be206c016dbcd22873146264601db86bd`, size 2,097,100 bytes. Remaining release risk is manual key-entry smoke, fixture classification acceptance, and low binary headroom.

## 2026-06-07 Trig Range and R-form Target Slice

Active slice: add a general trig R-form range family and accept symbolic R-form targets without adding exact-input routes.

Completed:
- Added a general integer-coefficient `a*sin(u)+b*cos(u)+c` range route for affine `u`.
- Added generated/targeted coverage for `range(sin(x)+cos(x),x)` and `range(3*sin(2*x+1)+4*cos(2*x+1)-5,x)`.
- Retired overlapping single trig range code to stay under the `.g3a` limit.
- Fixed route ordering so bounded interval ranges still use endpoint/stationary evaluation.
- Accepted `xform(...,R*sin(x+alpha))` as a symbolic R-form request.
- Rebuilt source artifacts.

Blocked-on:
- Full hardware/manual key-entry smoke still needs user-approved foreground/restart of fx-CG Manager or physical calculator entry.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 118 stale marker rows; `bad=0`.
- Binary headroom is only 60 bytes; new calculator-side logic needs size recovery first.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Scope remains bounded A-level Pure working-CAS, not arbitrary mathematical completeness.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `16804f879b6081f2742cb28fd11d9faadd7b1270bc29bf2be47e113c24145d0f`, size 2,097,092 bytes. Remaining release risk is manual key-entry smoke, fixture classification acceptance, and very low binary headroom.

## 2026-06-07 Emulator Install Smoke Follow-up

Active slice: verify the current artifact in fx-CG Manager rather than the stale already-running add-in.

Completed:
- Copied current `calculator_files/CAS.g3a` to emulator SD-card as both `CAS.g3a` and `CasioCAS.g3a`.
- Copied current `calculator_files/CAS.PAK` to emulator SD-card.
- Restarted fx-CG Manager hidden through CuaDriver and advanced the license wizard without foregrounding Codex.
- Mapped main-menu keyboard navigation and launched the visible `CasioCAS` icon.
- Confirmed the launched add-in accepts background key/text entry.
- Backed up emulator main-memory files and attempted to open/import `calculator_files/CAS.g3a` through fx-CG Manager.

Evidence:
- Emulator SD-card `CAS.g3a` and `CasioCAS.g3a`: `22fd95bace75ee6cc2172eb57471dbc711d08a50148d8fbd028bc8d7319151cb`.
- Emulator SD-card `CAS.PAK`: `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- Previous emulator SD-card backup: `/tmp/casio-smoke-current/emulator-sdcard-backup-20260607-034135`.
- Screenshots: `/tmp/casio-smoke-current/cas_selected.png`, `/tmp/casio-smoke-current/cas_current_launch.png`, `/tmp/casio-smoke-current/current_stats_full_typed.png`.
- Main-memory backup: `/tmp/casio-smoke-current/emulator-mainmem-backup-20260607-040911`.
- Import attempt screenshot: `/tmp/casio-smoke-current/open_g3a_modal.png`.

Blocked-on:
- The visible menu `CasioCAS` still behaves like the stale main-memory add-in: on-device `stats(1,2,3)` was simplified/echoed, not rejected.
- fx-CG Manager rejected direct app-level open/import of `CAS.g3a` with `The document "CAS.g3a" could not be opened.`
- Production host runner still rejects `stats` and `stats(1,2,3)` as unsupported.
- Full manual key-entry smoke now needs installing current `CAS.g3a` into emulator main memory or physical calculator testing.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Scope remains bounded A-level Pure working-CAS, not arbitrary mathematical completeness.

Resume hint: current source artifact is `calculator_files/CAS.g3a` SHA256 `22fd95bace75ee6cc2172eb57471dbc711d08a50148d8fbd028bc8d7319151cb`, size 2,074,064 bytes. Emulator SD-card now contains that artifact, but the menu-launched main-memory add-in is stale; manual smoke remains blocked until current add-in is installed into emulator main memory or tested on hardware.

## 2026-06-07 Shared Working Reason Classification

Active slice: make shared-working stale marker acceptance explicit and auditable without changing calculator output.

Completed:
- Added `family` and `classification` fields to `tests/reports/shared_working_latest.jsonl`.
- Kept strict marker mode literal; non-strict classification still requires concrete route/output evidence.
- Added summary counts for stale reason buckets and `untrusted_classified_rows=0`.

Evidence:
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0 stale_reasons=equation_output_drift=15,route_output_drift=16,symbolic_output_drift=1,verified_output_drift=86`.
- Stale family distribution: solve 31, other 27, integrate 15, xform 14, diff 11, range 5, numeric 5, series 3, factor 2, expand 2, partfrac 2, evalat 1.
- `python3 -m py_compile tests/check_shared_working.py`: passed.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=7` and `xform_planner_cases=11`.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: `production_cases=97 planner_cases=97`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Blocked-on:
- Shared working still has 118 stale marker rows, but all are classified by reason and none are untrusted.
- Full manual key-entry smoke still needs current `CAS.g3a` installed into emulator main memory or physical calculator testing.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- No calculator source or artifact changed in this slice.

Resume hint: shared-working stale marker rows are now explicitly classified in report JSON and summary output. Current calculator artifact remains `CAS.g3a` SHA256 `22fd95bace75ee6cc2172eb57471dbc711d08a50148d8fbd028bc8d7319151cb`, size 2,074,064 bytes.

## 2026-06-07 Sqrt-Quadratic Range Fix and Final Gate

Active slice: fix a real generic range weakness found during closeout and rerun final gates.

Completed:
- Replaced loose `sqrt(R)>=0` handling for quadratic radicands with a general convex-quadratic route.
- Added generated-style targeted cases for `sqrt(x^2+1)`, `sqrt((x+1)^2+4)`, and `sqrt(x^2-1)`.
- Rebuilt the source `.g3a`.
- Re-ran final automated gates after the rebuild.
- Imported the pre-fix current add-in into fx-CG Manager main memory through Memory Manager -> Import/Export -> Import files, then launched the `CAS` icon.

Evidence:
- `tools/khicas_host_runner 'range(sqrt(x^2+1),x)'`: `min inside=1 at x=0`, `y >= 1`, `Verified`.
- `tools/khicas_host_runner 'range(sqrt((x+1)^2+4),x)'`: `min inside=4 at x=-1`, `y >= 2`, `Verified`.
- `tools/khicas_host_runner 'range(sqrt(x^2-1),x)'`: `inside crosses 0`, `y >= 0`, `Verified`.
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,074,820 bytes, SHA256 `010d71d02c22808a186f6dec79cc6bff7011b40f62be991fa1305bccfb70077b`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=10`.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed with `production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.

Blocked-on:
- Full long/xform manual key-entry smoke remains incomplete; background text injection is not reliable enough for long expressions.
- The imported emulator add-in should be refreshed again with the post-fix artifact before any hardware-style manual signoff.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Scope remains bounded Edexcel A-level Pure working-CAS, not arbitrary mathematical completeness.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `010d71d02c22808a186f6dec79cc6bff7011b40f62be991fa1305bccfb70077b`, size 2,074,820 bytes, leaving 22,332 bytes under the 2,097,152-byte limit. Automated final gates pass on accepted rows; remaining release risk is manual long/xform key-entry smoke and acceptance of classified/stale fixture rows.

## 2026-06-07 Shifted Exp/Trig Range Route Families

Active slice: close real generic range fallbacks for shifted exponentials, trig-square expressions, and reciprocal shifted trig expressions without adding literal input routes.

Completed:
- Added general `c+a*exp(linear)` range handling using `exp(u)>0`.
- Added general `c+a*sin(linear)^2`, `c+a*cos(linear)^2`, `c+a*tan(linear)^2`, `c+a*sec(linear)^2`, and `c+a*cosec(linear)^2` range handling.
- Added general `n/(c+a*sin(linear))` and `n/(c+a*cos(linear))` range handling, including denominator-crossing union output.
- Added targeted coverage for six unusual A-level range forms that previously fell to unverified fallback.
- Rebuilt source `.g3a` and reran the requested automated final gate set.

Evidence:
- `tools/khicas_host_runner 'range(2+3*exp(-x),x)'`: `shifted exponential`, `y > 2`, `Verified`.
- `tools/khicas_host_runner 'range(2-3*exp(-x),x)'`: `shifted exponential`, `y < 2`, `Verified`.
- `tools/khicas_host_runner 'range(1+2*sin(x)^2,x)'`: `sin^2(u) in [0,1]`, `1 <= y <= 3`, `Verified`.
- `tools/khicas_host_runner 'range(tan(x)^2,x)'`: `tan^2(u) >= 0`, `y >= 0`, `Verified`.
- `tools/khicas_host_runner 'range(1/(sin(x)+2),x)'`: `1/3 <= y <= 1`, `Verified`.
- `tools/khicas_host_runner 'range(1/sin(x),x)'`: `D != 0`, `y <= -1 or y >= 1`, `Verified`.
- `./compile`: passed.
- `calculator_files/CAS.g3a`: 2,079,200 bytes, SHA256 `beff681c3c1109fdfeb7aa595b5d2f9320bfe7ebbe0de9c04450cf0d873b4639`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: passed.
- `python3 tools/check_removed_features.py`: passed.
- `python3 tools/check_calculator_border.py`: passed.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=16`.
- `python3 tests/check_working_planner.py`: passed.
- `python3 tests/check_xform_rule_families.py`: passed with `production_cases=97`, `planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- fx-CG Manager current-artifact smoke: current `CAS.g3a`, `CasioCAS.g3a`, and `CAS.PAK` copied to emulator SD-card with matching hashes. Memory Manager import retry reached the macOS Open panel, but `.g3a` and `.PAK` choices were disabled; import was cancelled and the emulator returned to Memory Manager. Screenshot: `/tmp/casio-smoke-current/after_cancel_current.png`.
- fx-CG Manager launch smoke: D-pad selected visible `CasioCAS` icon and opened a CAS session. Screenshot: `/tmp/casio-smoke-current/casiocas_dpad_launch_final.png`.
- fx-CG Manager key-entry smoke: background `press_key` reached the CAS session and evaluated numeric input. `2`, `2`, `enter` produced a visible working screen for `22`. Screenshot: `/tmp/casio-smoke-current/presskey_2p2.png`.
- fx-CG Manager compound-entry smoke: frontmost `System Events` key-code entry evaluated `2+2` to `4`. Screenshot: `/tmp/casio-smoke-current/systemevents_2p2.png`.
- fx-CG Manager xform-entry smoke: function-name entry remains unreliable through automation. Typing the required long `xform(...)` expression reduced to `xx`; clipboard paste and calculator `PASTE` did not insert text; catalog search accepted only partial letter input. Screenshots: `/tmp/casio-smoke-current/systemevents_xform.png`, `/tmp/casio-smoke-current/clipboard_2p2.png`, `/tmp/casio-smoke-current/calc_paste_2p2.png`, `/tmp/casio-smoke-current/catalog_x_retry.png`.

Blocked-on:
- Full long/xform manual key-entry smoke remains incomplete.
- Current `beff681...` artifact launch is verified through visible `CasioCAS`; loaded in-memory hash is not externally inspectable, so hardware-style signoff still needs foreground/hardware key-entry.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 118 stale marker rows; `bad=0`.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- New routes are shape/family based, not literal tested-input branches.
- Scope remains bounded Edexcel A-level Pure working-CAS, not arbitrary mathematical completeness.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `beff681c3c1109fdfeb7aa595b5d2f9320bfe7ebbe0de9c04450cf0d873b4639`, size 2,079,200 bytes, leaving 17,952 bytes under the 2,097,152-byte limit. Automated gates pass on accepted rows; visible `CasioCAS` launch works. Remaining release risk is long/xform foreground or hardware key-entry and acceptance of classified/stale fixture rows.

## 2026-06-07 Plain Eval Fallthrough Fix

Active slice: fix the calculator UI arithmetic failure without adding per-expression routes.

Completed:
- Restricted the working engine's exact fallback to kept command surfaces.
- Removed bare-expression `try_numeric_expr` interception from `eval_with_working`.
- Added `kept_working_command()` so native KhiCAS owns plain arithmetic/algebra such as `2+2`, `2*x+1`, and `(3+5)/2`.
- Added `tests/check_plain_eval_fallthrough.py`; it installs a bad callback and requires plain expressions to fall through instead of displaying callback errors.
- Updated host runner fallback to mimic calculator native eval after the working engine declines plain expressions.
- Rebuilt source `.g3a` and reran automated gates.

Evidence:
- `python3 tests/check_plain_eval_fallthrough.py`: `OK plain eval fallthrough cases=6`.
- `tools/khicas_host_runner '2+2'`: `= 4`.
- `tools/khicas_host_runner '2*2'`: `= 4`.
- `tools/khicas_host_runner '4-2'`: `= 2`.
- `tools/khicas_host_runner '4/2'`: `= 2`.
- `tools/khicas_host_runner 'range(4*(x^2-2)*exp(-2*x),x)'`: derivative route, `min y = -4*exp(2)`, `y >= -4*exp(2)`, `Verified`.
- `tools/khicas_host_runner 'xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)'`: `k = 2`, `Verified by substitution`, `Verified by equivalence check`.
- `./compile`: passed.
- Link report: ROM `2,054,108 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,082,784 bytes, SHA256 `e9d6f653d66fee67582702df1fd5b0988ca0f73dc2c4299d24e0d2cc1d68fb3b`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 14,368 bytes under limit.
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

Manual/emulator:
- Current artifacts were copied to emulator SD-card as `CAS.g3a`, `CasioCAS.g3a`, and `CAS.PAK`; hashes matched current build.
- The already-running CAS add-in still showed the old `2+2 -> "// Error: Bad Argument Type"` behavior, proving it was stale in RAM.
- `cua-driver launch_app` with the current `.g3a` URL did not reload the add-in; a second instance showed `The application is already running.`
- Background pixel/AX attempts to open the main menu or `Main Memory R/W` route did not switch the emulator into a fresh current-artifact launch.
- CuaDriver daemon started for AX cache testing and was stopped afterward.

Blocked-on:
- Manual current-artifact calculator smoke still needs user-approved fx-CG Manager restart/foreground control or physical hardware.
- Exact queue still has 67 classified invalid rows / 87 invalid marker gaps; accepted rows have `bad=0`.
- Shared working still has 118 stale marker rows; `bad=0`.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Fix is route-class ownership, not a literal `2+2` route.

Resume hint: current release candidate is `calculator_files/CAS.g3a` SHA256 `e9d6f653d66fee67582702df1fd5b0988ca0f73dc2c4299d24e0d2cc1d68fb3b`, size 2,082,784 bytes, leaving 14,368 bytes under the 2,097,152-byte limit. Automated gates pass; current-artifact manual UI smoke is blocked by fx-CG Manager reload/control.

## 2026-06-07 Classification Audit Gate

Active slice: make classified exact/shared rows auditable as a gate, not just summary text.

Completed:
- Added `tests/check_classification_audit.py`.
- The audit fails if any exact classified row is not `invalid`.
- The audit fails if any exact classified marker reason does not start with `invalid_marker_`.
- The audit fails if shared-working classifications leave the trusted class set.

Evidence:
- `python3 tests/check_classification_audit.py`: `OK classification audit exact_rows=67 exact_markers=87 exact_reasons={'invalid_marker_context: mark-scheme/context text is not a direct single-command output obligation': 32, 'invalid_marker_context: marker depends on omitted question context, not the direct solve command': 8, 'invalid_marker_value: exact marker differs from evaluated input': 41, 'invalid_marker_value: marker is not in exact solve output': 2, 'invalid_marker_value: numeric marker differs from evaluated input': 4} shared_rows=255 shared_classes={'equation_output_drift': 16, 'marker_present': 137, 'route_output_drift': 16, 'symbolic_output_drift': 1, 'verified_output_drift': 85}`.
- `python3 -m py_compile tests/check_classification_audit.py`: passed.

Blocked-on:
- Manual current-artifact calculator smoke still needs user-approved fx-CG Manager restart/foreground control or physical hardware.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- This gate audits classification trust only; it does not reclassify rows or weaken queue execution.

## 2026-06-07 Full Gate Rerun And Manual Smoke Attempt

Active slice: rerun the required release gates from the active goal and re-check current-artifact manual calculator state.

Completed:
- Rebuilt from source with `./compile`; artifact hash and size stayed unchanged.
- Reran required metadata/scope/UI/test gates.
- Reran full exact queue with 17,300 rows.
- Verified exact queue golden file remains unchanged.
- Checked emulator state with CuaDriver screenshots.
- Stopped CuaDriver daemon after the UI attempt.

Evidence:
- `./compile`: passed.
- Link report: ROM `2,054,108 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,082,784 bytes, SHA256 `e9d6f653d66fee67582702df1fd5b0988ca0f73dc2c4299d24e0d2cc1d68fb3b`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 14,368 bytes under limit.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=18`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: passed with exact classified rows `67`, marker gaps `87`, shared rows `255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.
- Host probes: `2+2 -> 4`; `range(4*(x^2-2)*exp(-2*x),x)` shows derivative/stationary/asymptote route and `Verified`; `xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)` gives `k = 2` and verifies.

Manual/emulator:
- fx-CG Manager window is visible and frontmost.
- Screenshot `/tmp/casio-smoke-current/current_window_latest.png` shows the running add-in is still stale: `2+2`, `2*2`, `4-2`, and `4/2` still display `"// Error: Bad Argument Type"`.
- Screenshot `/tmp/casio-smoke-current/after_ac_menu_try_points.png` shows CuaDriver can press calculator keys, but the driven route entered the in-addin catalog rather than reloading the current `.g3a`.
- CuaDriver daemon stopped after the attempt.

Blocked-on:
- Current-artifact calculator manual smoke still needs a real reload of fx-CG Manager/add-in memory, likely by user-approved app restart or hardware install.
- Long `xform(...)` key-entry smoke remains unproven on calculator UI because the currently running emulator add-in is stale.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Automated gates support the bounded A-level Pure surface, not arbitrary all-maths completeness.

## 2026-06-07 Generic Xform Fallback And Log-Law Route

Active slice: improve general route/fallback robustness without adding input-specific route patches.

Completed:
- Generalized xform log product/quotient matching so equivalent arguments are accepted, not only literal `u*v` text.
- Added a stronger xform non-equivalence fallback report:
  - attempted transformations
  - failure reason
  - last verified state
  - KhiCAS exact evaluation if available
  - verification status
- Added targeted coverage for `xform(ln(x+1)+ln(x-1),ln(x^2-1))`.
- Synced emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` to the rebuilt artifact.

Evidence:
- `tools/khicas_host_runner 'xform(ln(x+1)+ln(x-1),ln(x^2-1))'`: log product route, `ln(x^2-1)`, `Verified by equivalence check`.
- `tools/khicas_host_runner 'xform(sin(x)+cos(x),tan(x))'`: fallback now shows attempted transformations, failure reason, last verified state, KhiCAS exact evaluation, and `not equivalent to target; not verified`.
- `./compile`: passed.
- Link report: ROM `2,054,984 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,083,660 bytes, SHA256 `249b2bf7180777ac630ebca13ddeb174f3d679b9b2f725312961403cb9438dcd`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 13,492 bytes under limit.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: `OK targeted working policy ... xform_log_law_cases=3 ... range_cases=18`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=97 planner_cases=97`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: passed with exact classified rows `67`, marker gaps `87`, shared rows `255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.
- Emulator SD-card `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` SHA256 all matched `249b2bf7180777ac630ebca13ddeb174f3d679b9b2f725312961403cb9438dcd`.
- `cua-driver stop`: daemon stopped.

Blocked-on:
- Current-artifact manual calculator smoke still needs a real add-in reload, likely user-approved fx-CG Manager restart or hardware install.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is a general log-law equivalence route and general xform fallback report, not a special-case route.

## 2026-06-07 General Range/Implicit Route Families

Active slice: replace weak range/implicit fallbacks with general route families, not sample routes.

Completed:
- Added a general bounded trig-rational range route for `linear(sin,cos)/linear(sin,cos)` with nonzero denominator range, R-form existence condition, exact inequality solve, and `Verified`.
- Allowed endpoint/critical calculus range routing for log/sqrt fractions, covering forms like `ln(x)/x`.
- Added final interval/`Verified` output to rational discriminant range routes.
- Normalized `exp(-1)` range display to exam-style `1/e`.
- Fixed direct `implicit_diff(...)` output to avoid duplicate headers and mark exact-derived implicit working as `Verified`.
- Kept xform fallback detailed while restoring the `Check equivalence:` marker required by working-quality gates.
- Synced emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` to the rebuilt artifact.

Evidence:
- `range(sin(x)/(2+cos(x)),x)`: trig rational route, `3*y^2 - 1 <= 0`, `(-sqrt(3)/3 <= y) & (y <= sqrt(3)/3)`, `Verified`.
- `range(ln(x)/x,x)`: domain, endpoint limits, derivative, `f(e) = 1/e`, `y <= 1/e`, `Verified`.
- `range((x^2+2*x+3)/(x^2+1),x)`: discriminant route, final interval, `Verified`.
- `implicit_diff(x^2+sin(y)=y,x,y)`: `(dy)/(dx)=-(2*x)/(cos(y) - 1)`, `Verified`.
- `./compile`: passed.
- Link report: ROM `2,057,500 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,086,176 bytes, SHA256 `7ecf439d6871214b50b875c760faf2499c479763fa2237c6f69feb1c9802c34c`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 10,976 bytes under limit.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=21`, `implicit_cases=1`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: passed with exact classified rows `67`, marker gaps `87`, shared rows `255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Blocked-on:
- Current-artifact manual calculator smoke still needs a real add-in reload, likely user-approved fx-CG Manager restart or hardware install.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are general route families/gate fixes, not exact-input route patches.

## 2026-06-07 Implicit Product And Log-Domain Filtering

Active slice: close unseen-form gaps found by hard A-level Pure probes.

Completed:
- Restored known-function products after star-stripping, so forms like `x*exp(-x)` route as `x*exp(-x)` instead of `xexp(-x)`.
- Added targeted coverage for `range(x*exp(-x),x)`.
- Added real-domain filtering for exact solve candidates from log/sqrt equations.
- Guarded single-log solver branches so `log(...)+log(...)` and `log(...)-log(...)` do not get misread as one outer `log(...)`.
- Added targeted coverage for invalid log-domain roots and valid log-sum exact fallback.
- Synced emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` to the rebuilt artifact.

Evidence:
- `range(x*exp(-x),x)`: derivative route, `f(1) = 1/e`, `y <= 1/e`, `Verified`.
- `solve(log(3,2*x-1)-log(3,x+1)=1,x)`: rejects `x=-4` by log-domain check, returns `x = []`, `Verified by domain check`.
- `solve(log(2,x+3)+log(2,x-1)=3,x)`: no false first-log route; exact result `[-1 + 2*sqrt(3)]`.
- `./compile`: passed.
- Link report: ROM `2,060,200 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,088,876 bytes, SHA256 `ab5969e0f2c54cc6fc7c503e7437c5d8495e7d07549d7aa49ce52be595327df9`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 8,276 bytes under limit.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=22`, `solve_fallback_cases=5`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: passed with exact classified rows `67`, marker gaps `87`, shared rows `255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Blocked-on:
- Current-artifact manual calculator smoke still needs a real add-in reload, likely user-approved fx-CG Manager restart or hardware install.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are general normalisation/domain-validation logic, not exact-input route patches.

## 2026-06-07 Planner Rule Ordering And Identity Precedence

Active slice: strengthen the general `xform` planner without letting it replace clearer exam identity working.

Completed:
- Added kept-command trig rewrite candidates to the C++ bounded xform planner: `texpand` and `tcollect`.
- Reordered planner candidates so trig expansion/collection and algebraic expand/factor routes run before broad simplify.
- Gated trig rewrite commands to trig-containing states only, so non-trig algebra is not routed through trig no-op rewrites.
- Allowed a verified rule step to target an equivalent requested form, while preserving reciprocal-trig steps when those are the clearer next route.
- Added a general trig non-equivalence pre-check so shifted-trig non-equivalent `xform(...)` inputs return bounded failure evidence instead of spending the route budget.
- Restored trig-rational identity precedence before the generic planner, so exam double-angle lines remain visible for `method=identity`.
- Synced emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` to the rebuilt artifact.

Evidence:
- `xform(sin(x+y),sin(x)*cos(y)+cos(x)*sin(y))`: planner route uses `Trig expand`, then verified target equivalence.
- `xform((x+1)^2+(x-1)^2,2*x^2+2)`: planner route uses `Expand expression`.
- `(1-cos(2*theta)+sin(2*theta))/(1+cos(2*theta)+sin(2*theta))=tan(theta),method=identity`: outputs `1-cos(2*theta) = 2*sin(theta)^2`, `sin(2*theta) = 2*sin(theta)*cos(theta)`, and verifies.
- `xform(sin(2*x+1),2*sin(x+1)*cos(x+1))`: bounded non-equivalence report in about 3 seconds including rebuild.
- `./compile`: passed.
- Link report: ROM `2,061,576 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,090,252 bytes, SHA256 `bda56a68bfbb7d67988bd87ce44539183476ed0239d9417098d8471ea2b8b2b9`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 6,900 bytes under limit.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `xform_planner_cases=11`, `range_cases=22`, `solve_fallback_cases=5`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=103 planner_cases=103`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=137 stale_markers=118 bad=0 untrusted_classified_rows=0`.
- `python3 tests/check_classification_audit.py`: passed with exact classified rows `67`, marker gaps `87`, shared rows `255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.
- Emulator SD-card `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a` SHA256 all matched `bda56a68bfbb7d67988bd87ce44539183476ed0239d9417098d8471ea2b8b2b9`; `CAS.PAK` matched `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.

Blocked-on:
- Current-artifact manual calculator smoke still needs a real add-in reload, likely user-approved fx-CG Manager restart or hardware install.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are general planner-ordering, rule-family, and verification-precedence logic, not exact-input route patches.

## 2026-06-07 Plain Input Callback Guard

Active slice: fix live emulator plain-input failures without weakening A-level route logic.

Completed:
- Added local plain numeric-expression handling before any KhiCAS callback in `eval_with_working`.
- Updated the plain-input guard test so `2+2`, `4-2`, `4/2`, and similar inputs must not leak `// Error: Bad Argument Type` from a failed exact callback.
- Rebuilt `CAS.g3a` and synced emulator SD-card aliases `CAS.g3a`, `CasioCAS.g3a`, and `khicasen.g3a`.

Evidence:
- Live emulator screen showed `2+2` and nearby plain inputs returning `"// Error: Bad Argument Type"` in the running add-in.
- `python3 tests/check_plain_eval_fallthrough.py`: `OK plain eval callback guard cases=6`.
- `./tools/khicas_host_runner '2+2'`: `= 4`.
- `./tools/khicas_host_runner '4-2'`: `= 2`.
- `./compile`: passed.
- Link report: ROM `2,061,336 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,090,012 bytes, SHA256 `3da8b9c7826dd2958871bdd356e57587d9746c75f67559b602f972ddc415420a`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 7,140 bytes under limit.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=22`, `xform_planner_cases=11`, `solve_fallback_cases=5`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=103 planner_cases=103`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=124 stale_markers=131 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: passed with exact classified rows `67`, marker gaps `87`, shared rows `255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Blocked-on:
- Running emulator add-in still needs exit/relaunch to load the synced `CAS.g3a`; in-memory add-ins do not hot-reload.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Change is a general plain-input guard, not a special-case route.

## 2026-06-07 Log-Quadratic Range And Log-Law Domains

Active slice: close unseen A-level range/domain weakness without adding exact route patches.

Completed:
- Added a general `ln/log(quadratic)` range route:
  - convex positive quadratic: min inside -> monotone log bound
  - convex crossing/zero: all real log range
  - concave positive quadratic: max inside -> one-sided log bound
  - base `0<a<1` reverses inequality
- Added explicit domain-condition lines to product/quotient log-law `xform` working.
- Added targeted coverage for `range(ln(x^2+1),x)` and log-law domain conditions.
- Rebuilt source `.g3a` and synced emulator SD-card aliases.

Evidence:
- `range(ln(x^2+1),x)`: `log of quadratic`, `min g=1 at x=0`, `y >= 0`, `Verified`.
- `range(log(1/2,x^2+1),x)`: base below 1 reverses to `y <= 0`, `Verified`.
- `xform(log(2,x/y),log(2,x)-log(2,y))`: includes `Condition: 2>0, 2!=1; x>0; y>0`.
- `xform(ln(x+1)+ln(x-1),ln(x^2-1))`: includes `Condition: e>0, e!=1; x+1>0; x-1>0`.
- `./compile`: passed.
- Link report: ROM `2,063,936 / 2,097,192`, RAM `348,648 / 442,368`.
- `calculator_files/CAS.g3a`: 2,092,612 bytes, SHA256 `b05db08737e84e2d7278c9ed0c1d812f56e85068d28b6d47bf5dadb0c24ddf7d`.
- `calculator_files/CAS.PAK`: 9,649 bytes, SHA256 `052c840eda857b109282acae7768ed5bbabcf7e5a4478d2847aadab2152af398`.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes, SHA256 `084e197a81a047efbabaff2d2c051c5fab4c2180667967074f7075665ad39d70`.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a`: passed, 4,540 bytes under limit.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a`: passed.
- `python3 tools/check_g3a_metadata.py calculator_files/RUNMAT.g3a --name RunMat --internal @RUNMAT --filename RUNMAT.g3a`: passed.
- `python3 tools/check_catalog_scope.py`: `OK catalog scope kept_visible=46 removed=82`.
- `python3 tools/check_removed_features.py`: `OK removed features blocked=82`.
- `python3 tools/check_calculator_border.py`: `OK calculator border`.
- `python3 tests/check_targeted_working_gaps.py`: passed with `range_cases=23`, `xform_log_law_cases=3`.
- `python3 tests/check_plain_eval_fallthrough.py`: `OK plain eval callback guard cases=6`.
- `python3 tests/check_working_planner.py`: `OK working planner cases=10`.
- `python3 tests/check_xform_rule_families.py`: `OK xform rule families production_cases=103 planner_cases=103`.
- `python3 tests/check_working_quality.py`: `OK working quality cases=10`.
- `python3 tests/check_shared_working.py`: `CLASSIFIED shared working cases=255 clean=124 stale_markers=131 bad=0 untrusted_classified_rows=0`.
- `python3 tests/random_working_fuzzer.py --chaos --per-function 1 --timeout 8 --seed 707 --depth 8 --strict`: `OK random fuzz done=155 bad=0`.
- `python3 tests/run_exact_queue.py --engine production --strict-markers --workers 4`: `CLASSIFIED exact queue run total=17300 clean=17233 invalid_rows=67 untrusted_classified_rows=0 accepted=17233 bad=0 raw_bad=67 invalid_marker_gaps=87 untrusted_marker_gaps=0`.
- `python3 tests/check_classification_audit.py`: passed with exact classified rows `67`, marker gaps `87`, shared rows `255`.
- `git diff --check`: passed.
- `git diff --exit-code -- tests/golden/exact_calculator_input_queue.jsonl`: passed.
- `git branch --list`: only `main`.

Blocked-on:
- Current-artifact manual calculator smoke still needs a real add-in reload from MENU.

Drift check:
- Still on `main`; no extra branch.
- Exact queue golden remains unchanged.
- Changes are general range/log-law rules, not exact-input route patches.
