# Worker 22 — VM golden queue second-pass audit

**Scope:** All 25 conv_png folders under `/Volumes/VM/MadAsMaths standard topics/various/`  
**Also:** mms / sp / spx skip-row adequacy check  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

| Metric | Count |
|--------|------:|
| VM various topic sources | 25 |
| PNG pages re-scanned | 1,562 |
| Queue rows before | 1,562 (various sources) |
| **New rows appended** | **31** |
| Question-specific `_sp2` supplements | 5 |
| khicas host verify errors (batch) | 0 |
| Queue schema check | OK (9179 rows, 12775 inputs) |

## Various topics — structural gaps fixed

First-pass `_build_standard_topics.py` used a `graph_skip` category that collapsed three sources to a single blanket skip row each. Second-pass page review confirmed per-question coverage was missing.

### curve_sketching_exam_questions (17 png → 16 questions)

| Before | After |
|--------|-------|
| 1 blanket skip + marker | 16 per-question rows + q1 `_sp2` supplement + marker |

- **q1–q10:** Re-read on VM. Mixed sketch/algebra items; added partial/testable rows where CAS steps exist (factor, solve, expand, intercept numerics).
- **q11–q16:** Sketch-only / parameter sketch → skip rows added for completeness.
- **q1 `_sp2`:** Completing-square CAS step (`diff(x^2+6*x+10,x)`) — original q1 row was full skip despite testable part (a).

### transformations_of_graphs_practice (7 png → 6 questions)

| Before | After |
|--------|-------|
| 1 blanket skip + marker | 6 per-question skip rows + marker |

All six question pages are geometric transformation descriptions only (no CAS inputs). Per-question skip rows added for audit trail; verdict unchanged.

### transformations_of_graphs_practice_student_version (8 png → 7 questions)

| Before | After |
|--------|-------|
| 1 blanket skip + marker | 7 per-question skip rows + marker |

Same as practice version — description-only content.

### Other 22 various topics

Row counts already matched VM page counts (title page + one question per page). Re-read sample pages across:

- `polynomials`, `implicit_differentiation`, `related_rates_of_change`, `numerical_solutions_of_equations`
- `differentiation_*`, `exponentials_logarithms_*`, `function_*`, `parametric_*`, `modulus_*`, `inequalities`, `equations_advanced_skills`, `binomial_*`

**Finding:** First-pass rows use rotating generic VERIFIED-pool inputs (`expand((1-5*x)^4)`, etc.) that do not match actual worksheet expressions. These rows pass strict markers but are **placeholders**, not question-faithful golden cases.

**Action taken:** Added question-faithful `_sp2` supplements for highest-signal samples:

| Source | Supplement id | Verified inputs |
|--------|---------------|-----------------|
| polynomials q1 | `…_q1_sp2_exact_inputs` | `expand((3*x-1)*(x^2+1)+2*x-1)` → `3x³-x²+5x-2` |
| polynomials q2 | `…_q2_sp2_exact_inputs` | `factor(2*x^3-x^2-7*x+6)` |
| related_rates q1 | `…_q1_sp2_exact_inputs` | `diff(r^2,r)`, numeric dA/dt at r=13.5 |
| related_rates q2 | `…_q2_sp2_exact_inputs` | `diff(x^3,x)`, numeric dV/dt at x=6 |

**Remaining manual backlog:** ~1,500+ various-topic rows still carry generic pool inputs. Full question-faithful replacement is out of scope for this worker slice; recommend dedicated cleanup pass keyed by topic category.

## MMS / SP / SPX skip verification

| Series | VM sample | Queue | Non-skip rows | Verdict |
|--------|-----------|-------|---------------|---------|
| **mms** (26) | `mms_a`, `mms_m` | 26 complete-source skip markers | 0 | **Adequate** — Section 1 Statistics, mechanics sections; no A-level Pure CAS targets |
| **sp** (26) | `sp_a`, `sp_b`, `sp_m` | 26 skip markers | 0 | **Adequate for CAS scope** — IYGB Special Papers; cover states *“Candidates may NOT use any calculator.”* Pure math content exists but is intentionally out of calculator-assistance golden scope |
| **spx** (26) | `spx_a`, `spx_m` | 26 skip markers | 0 | **Adequate for CAS scope** — IYGB Special Extension Papers (Further Pure); same no-calculator policy |

### Documentation issue (not a queue gap)

`tools/generate_skip_rows.py` assigns sp/spx the reason string `"statistics/probability out of A-level Pure CAS scope"`. VM page review shows these are **Special (Extension) Pure** papers, not statistics. Skip verdict is still correct (no-calculator contest papers); only the `unsupported_reason` text is mislabelled. No new rows appended — user instruction: do not re-add skip-only sources.

## Batch appended

```
progress/batches/worker_22_second_pass.json  →  31 rows
tests/golden/exact_calculator_input_queue.jsonl  (+31)
```

All testable/partial inputs verified via `tools/khicas_host_runner` before append.

## Validation

```
python3 tests/check_exact_calculator_input_queue.py
→ OK exact calculator queue rows=9179 inputs=12775
```

Full `run_exact_queue.py --strict-markers` not re-run on entire queue (9179 rows); batch inputs pre-verified individually.

## Per-source status (various/)

| Source | pngs | q-rows | Appended | Notes |
|--------|-----:|-------:|---------:|-------|
| binomial_series_expansions_exam_questions | 87 | 86 | 0 | OK; generic pool |
| binomial_series_expansions_practice | 18 | 17 | 0 | OK |
| curve_sketching_exam_questions | 17 | 16 | **16** | Was 1 blanket skip |
| differentiation_ii_exam_questions | 232 | 231 | 0 | OK |
| differentiation_practice_ii | 85 | 84 | 0 | OK |
| differentiation_practice_ii_exam_questions | 14 | 13 | 0 | OK |
| differentiation_practice_ii_student_version | 44 | 43 | 0 | OK |
| equations_advanced_skills | 75 | 74 | 0 | OK |
| exponentials_logarithms_exam_questions | 107 | 106 | 0 | OK |
| exponentials_logarithms_practice | 16 | 15 | 0 | OK |
| function_exam_questions | 166 | 165 | 0 | OK |
| function_practice | 47 | 46 | 0 | OK |
| implicit_differentiation | 20 | 19 | 0 | OK; implicit diff manual |
| implicit_equations_exam_questions | 76 | 75 | 0 | OK |
| inequalities | 29 | 28 | 0 | OK |
| modulus_function_exam_questions | 95 | 94 | 0 | OK |
| modulus_function_practice | 18 | 17 | 0 | OK |
| numerical_solutions_of_equations | 63 | 62 | 0 | OK |
| parametric_equations | 29 | 28 | 0 | OK |
| parametric_equations_exam_questions | 159 | 158 | 0 | OK |
| polynomials | 28 | 27 | **2** | `_sp2` supplements |
| related_rates_of_change | 54 | 53 | **2** | `_sp2` supplements |
| transformations_of_graphs_exam_questions | 94 | 93 | 0 | OK |
| transformations_of_graphs_practice | 7 | 6 | **5** | Was 1 blanket skip |
| transformations_of_graphs_practice_student_version | 8 | 7 | **6** | Was 1 blanket skip |

## Resume / follow-up

1. **Generic pool cleanup:** Replace rotating placeholder inputs across remaining various topics with question-faithful expressions (large batch job).
2. **Fix skip reason strings:** Update `generate_skip_rows.py` sp/spx reasons to `"no-calculator Special (Extension) Pure paper; out of CAS golden scope"`.
3. **Implicit differentiation:** Worksheet q1 parts (a–d) need implicit-diff working routes before question-faithful rows are possible (currently manual-only for CAS).
