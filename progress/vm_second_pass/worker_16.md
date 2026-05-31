# Worker 16/22 — VM golden queue second-pass audit

**Repo:** `/Users/james/Developer/CASIO`  
**VM root:** `/Volumes/VM`  
**Scope:** MadAsMaths A-level booklet `conv_png` folders sorting alphabetically from start through **`basic_various` inclusive** (15 folders; worker 17 continues with `elementary` onward).

## Enumeration (VM `ls` / rglob)

| # | Category | Booklet stem | PNG pages |
|---|----------|--------------|-----------|
| 1 | basic_calculus | calculus_introduction_exam_questions | 277 |
| 2 | basic_calculus | differentiation_optimization_problems | 49 |
| 3 | basic_calculus | differentiation_practice_i | 71 |
| 4 | basic_calculus | integration_areas_intro | 64 |
| 5 | basic_calculus | integration_intro | 36 |
| 6 | basic_various | 3d_geometric_mensuration | 8 |
| 7 | basic_various | algebraic_fractions | 47 |
| 8 | basic_various | algebraic_fractions_exam_questions | 21 |
| 9 | basic_various | indices_exam_questions | 46 |
| 10 | basic_various | inequalities_exam_questions | 25 |
| 11 | basic_various | reciprocal_functions | 26 |
| 12 | basic_various | surds | 36 |
| 13 | basic_various | surds_student_version | 18 |
| 14 | basic_various | surd_exam_questions | 68 |
| 15 | basic_various | vector_algebra_introduction | 47 |

**Total:** 15 sources (834 PNG pages).

## First-pass baseline

Each source had a single sample question row plus `complete_source_marker`. Drill booklets (e.g. `differentiation_practice_i`, `surds`) contain many one-page multi-part questions not represented in the queue.

## Second-pass method

1. Re-read question/solution PNGs page-by-page for each source.
2. Cross-checked `source_pdf` rows in `tests/golden/exact_calculator_input_queue.jsonl`.
3. Drafted additional testable CAS inputs; verified markers with `tools/khicas_host_runner`.
4. Appended new question rows (`q2`, `q3`, …) and extended existing rows' `inputs[]` where same question had missing parts.
5. Validated schema + worker-local strict markers.

## Gaps found and fixes

### New rows appended (17)

| Source | New question rows | Notes |
|--------|-------------------|-------|
| differentiation_practice_i | q2, q3 | Integer-power `diff()` drills |
| integration_intro | q2 | Term-wise `integrate` where host accepts `sqrt(x)` / `x^n` |
| calculus_introduction_exam_questions | q1, q3 | q1 partial diff; q3 expand + roots + area numeric |
| differentiation_optimization_problems | q1 | Stationary `x` + `V_max` numeric (open-box problem) |
| integration_areas_intro | q1 | Area numeric (same integrand family as calc intro q2) |
| 3d_geometric_mensuration | q2 | Pyramid height numeric |
| algebraic_fractions | q2 | Factor checkpoints for q2 simplification |
| algebraic_fractions_exam_questions | q3, q4 | Quadratic solves after clearing fractions |
| indices_exam_questions | q2 | Index numeric evaluations |
| reciprocal_functions | q2 | x-intercept from translated reciprocal |
| surd_exam_questions | q2 | q2b rationalised surd quotient numeric |
| surds | q2 | q2 surd-simplification numerics |
| surds_student_version | q3 | Student q3 numerics |
| vector_algebra_introduction | q2 | `\|OB\|` numeric |

### Existing rows extended (5)

| Row id | Added inputs |
|--------|--------------|
| `…differentiation_practice_i_q1…` | +2 diff drills (sample q2-style) |
| `…algebraic_fractions_q1…` | +4 factor steps for parts c–d |
| `…surds_q1…` | +5 q2 surd numerics |
| `…surds_student_version_q1…` | +2 q3 numerics |
| `…inequalities_exam_questions_q1…` | +2 part-a vertex/y-intercept numerics; part widened to `a-b` |

Batch artifact: `progress/batches/worker16_second_pass_rows.json`  
Builder/verifier: `tools/_worker16_second_pass_batch.py`

## Post-audit queue coverage (this scope)

| Source | Question rows | Total inputs |
|--------|---------------|--------------|
| calculus_introduction_exam_questions | q1–q3 | 5 |
| differentiation_optimization_problems | q1–q2 | 4 |
| differentiation_practice_i | q1–q3 | 11 |
| integration_areas_intro | q1–q2 | 3 |
| integration_intro | q1–q2 | 3 |
| 3d_geometric_mensuration | q1–q2 | 4 |
| algebraic_fractions | q1–q2 | 9 |
| algebraic_fractions_exam_questions | q1–q4 | 4 |
| indices_exam_questions | q1–q2 | 4 |
| inequalities_exam_questions | q1 | 4 |
| reciprocal_functions | q1–q2 | 3 |
| surd_exam_questions | q1–q2 | 3 |
| surds | q1–q2 | 12 |
| surds_student_version | q1, q3 | 6 |
| vector_algebra_introduction | q1–q2 | 2 |

## Validation

| Check | Result |
|-------|--------|
| `tests/check_exact_calculator_input_queue.py` | OK |
| Worker-16 rows strict host markers (all `review_basis` containing `worker_16` + patched inputs) | **0 failures** |
| Full `tests/run_exact_queue.py --strict-markers` | 13076/13077 ok — **1 pre-existing failure** unrelated to this worker (`madas_iygb_mp1_a_q2_sp2_definite`, marker `10.5` vs host `16.5`) |

## Remaining manual-only gaps (not queued)

- **Drill booklets** (`differentiation_practice_i`, `integration_intro`, `surds`, …): dozens of later questions (q4+) per booklet — mostly repetitive drills; fractional-power `diff()` / compound surd `integrate()` not routed in host (General Pure fallback).
- **calculus_introduction_exam_questions** (277 PNGs): large exam bank; only q1–q3 sampled with CAS checkpoints.
- **Graph/sketch/proof parts**: reciprocal sketches, inequality intervals, optimization model derivations, 3D angle parts b–c, vector angle OAB.
- **indices_exam_questions q2b** algebraic simplification `(4xy²)²/(2x)³` — expand not reliably marker-stable in host.

## Summary

- **Sources re-audited:** 15 / 15 in scope  
- **New question rows:** 17  
- **Extended existing rows:** 5  
- **New verified inputs (approx.):** 40+  
- **Worker-local strict validation:** pass  
- **Resume:** Worker 17 owns `elementary` → `standard_integration` booklet folders.
