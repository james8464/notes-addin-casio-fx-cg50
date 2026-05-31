# Worker 18 — VM golden queue second-pass audit

**Worker:** 18/22  
**Scope:** MadAsMaths A-level booklet `conv_png` folders **#69–#102** (34 sources), alphabetically after worker 17  
**VM root:** `/Volumes/VM`  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Range boundary

| Worker | First folder | Last folder | Count |
|--------|--------------|-------------|-------|
| 16 | `basic_calculus/calculus_introduction_exam_questions` | `mechanics/m2_m3_elastic_strings_springs` | 34 |
| 17 | `mechanics/m2_m3_impulse` | `standard_integration/integration_numerical` | 34 |
| **18** | **`standard_integration/integration_partial_fractions`** | **`standard_various/inequalities`** | **34** |
| 19 | `standard_various/modulus_function_exam_questions` | `statistics/t_distribution` | 32 |

## Sources audited (34)

### standard_integration (6)
- integration_partial_fractions
- integration_structured_exam_questions_part_i
- integration_volume_of_revolution
- odes_context_modelling
- odes_separable_no_context
- parametric_integration_exam_questions

### standard_trigonometry (13)
- trigonometric_equations
- trigonometric_equations_introduction
- trigonometric_general_solutions
- trigonometric_graphs
- trigonometric_identities_with_solutions
- trigonometric_inverse_functions
- trigonometry_compound_angle_identities
- trigonometry_double_angle_identities
- trigonometry_exam_questions
- trigonometry_introduction_exam_equations
- trigonometry_minor_trig_ratios
- trigonometry_pythagorean_identities
- trigonometry_r_transformations

### standard_various (15)
- binomial_series_expansions_exam_questions
- binomial_series_expansions_practice
- curve_sketching_exam_questions
- differentiation_ii_exam_questions
- differentiation_practice_ii
- differentiation_practice_ii_exam_questions
- differentiation_practice_ii_student_version
- equations_advanced_skills
- exponentials_logarithms_exam_questions
- exponentials_logarithms_practice
- function_exam_questions
- function_practice
- implicit_differentiation
- implicit_equations_exam_questions
- inequalities

## First-pass inventory (pre-upgrade)

| Check | Result |
|-------|--------|
| VM `conv_png` folders present | 34/34 |
| Complete-source markers in queue | 34/34 |
| Question rows vs `pages − 1` | 34/34 matched |
| Missing question-index gaps | **0** |
| Real (non-placeholder) CAS inputs | **0** (all `method=numeric,N^2` checkpoints) |

## Second-pass actions

1. **Cross-validated VM PNG page counts** against matching `MadAsMaths standard topics/*` folders — 33/34 identical page counts; `trigonometry_exam_questions` booklet is 340 pages vs 361 on standard topics tree.
2. **Upgraded all 34 sources** by adapting pre-verified `madas_topic_*` batch rows to booklet `source_pdf` / `madas_booklet_*` ids (same worksheet content on VM).
3. **Special handling:**
   - `trigonometry_exam_questions`: upgraded q1–q339 only (booklet PDF shorter than topic tree).
   - `curve_sketching_exam_questions`: expanded topic’s single graph-skip review to 16 per-page skip rows (exam sketch descriptions, no CAS).
4. **Khicas verification:** all upgraded non-skip inputs re-run via `tools/khicas_host_runner` before queue write.
5. **Queue write:** replaced 2391 placeholder rows; batch saved at `progress/batches/worker18_booklet_upgrades.json`.

## Post-upgrade totals

| Metric | Value |
|--------|-------|
| Sources | 34 |
| Queue rows replaced | 2391 |
| Question rows | 2388 |
| Verdict testable | 1380 |
| Verdict partial | 411 |
| Verdict skip (per-question) | 566 |
| Real CAS input steps | 2573 |
| Remaining placeholder numeric steps | 766 (mostly trig partial rows using verified numeric checkpoints alongside symbolic steps) |
| Complete-source markers | 34/34 |

## Validation

| Test | Result |
|------|--------|
| `python3 tests/check_exact_calculator_input_queue.py` | **PASS** (9145 rows, 12720 inputs) |
| `python3 tests/run_exact_queue.py --strict-markers` | **12718/12720 ok** |
| Worker-18 scoped failures | **0** |
| Global failures (pre-existing, out of scope) | 2 — `madas_iygb_syn_a_q2_exact_inputs`, `madas_iygb_syn_m_q1_exact_inputs` (workers 13/14) |

## Gaps appended

No new question-index rows were required (first-pass page coverage was complete). **2391 rows upgraded** from placeholder checkpoints to PNG-reviewed topic-batch inputs (testable / partial / skip as appropriate).

## Notes for consolidation

- Booklet rows for worker 18 now mirror the reviewed standard-topic batches; workers 20–22 re-audit the same mathematical content under `MadAsMaths standard topics/` paths — no duplicate booklet rows needed there.
- Residual `method=numeric,*` markers in trig partial rows are intentional verified numeric checkpoints bundled with symbolic inputs in the topic batches.
- `trigonometry_exam_questions` topic tree has 21 extra pages (q340–q360) not present in the A-level booklet PDF on VM.
