# Worker 20 — VM golden queue second-pass audit

**Scope:** All 24 `MadAsMaths standard topics/integration/` sources on `/Volumes/VM`  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

| Metric | Value |
|--------|------:|
| Sources re-audited | 24 |
| Question rows in queue (pre) | 1,050 |
| Calculator inputs in queue (pre) | 1,912 |
| **Second-pass rows appended** | **6** |
| **Second-pass inputs appended** | **52** |
| Queue after append | 9,145 rows / 12,720 inputs |
| Strict host verify (sp2 only) | 52/52 ok |
| Strict queue run (full) | 12,718/12,720 ok (2 pre-existing syn failures, 0 sp2) |
| Schema check | `check_exact_calculator_input_queue.py` OK |
| VM coverage | 514/514 complete |

## Critical first-pass finding

All integration standard-topic rows were auto-built by `tools/_build_standard_topics.py`, which **cycles a 17-item VERIFIED pool** (`pick_inputs`) to assign **two generic recycled integrals per question page**. Inputs do **not** correspond to the integrals printed on the VM PNG pages.

Example — `integration_basics` page 2 (Question 1, ten drill items):

| Page item | Actual integral (VM PNG) | First-pass queue input |
|-----------|--------------------------|------------------------|
| 1 | `∫(3x+1)² dx` | `integrate((3*x+1)^2,x)` ✓ (coincidence) |
| 2 | `∫4(2x+1)⁵ dx` | `integrate(4*(2*x+1)^5,x)` ✓ (coincidence) |
| 3–10 | unique reverse-chain-rule items | **missing** — queue recycles unrelated pool items on later question rows |

The same pattern affects all drill-style topics (`integration_basics`, `integration_by_*`, `integration_indefinite_mix`, `odes_*`, etc.) and mis-maps exam-style topics (`integration_structured_exam_questions_part_i`, `parametric_integration_exam_questions`).

## Re-audit methodology

1. Enumerated all 24 `* conv_png` folders; confirmed question-row counts match `png_count − 1` (title page convention).
2. Page-image review (PNG read) on representative and high-priority sources:
   - Drill sheets: `integration_basics` q1–q7, `integration_by_a_special_manipulation` q1–q2, `integration_partial_fractions` q1
   - Exam sheets: `integration_structured_exam_questions_part_i` q6, `parametric_integration_exam_questions` section header
   - Condense/student variants sampled
3. Cross-checked queue rows via `source_pdf` grep; classified gaps:
   - **Missing drill items** (8 of 10 items per page not captured)
   - **Wrong mapping** (exam q6 mapped to `sin(2x)`/`cos(3x)` instead of partial fractions + `defint`)
   - **False auto-skips** (`"exam" in name and n % 4 == 0` rule → 66 structured-exam + 22 parametric skips without page review)
4. Built khicas-verified supplemental rows (`tools/_worker20_sp2_integration.py`); appended via `tools/append_queue_rows.py`.
5. Validated schema + strict markers.

## Rows appended (second pass)

Batch: `progress/batches/worker20_sp2_integration_rows.json`

| ID | Source | Question | Inputs | Notes |
|----|--------|----------|-------:|-------|
| `madas_topic_integration_by_a_special_manipulation_q1_sp2_drill_exact_inputs` | special_manipulation | q1 | 10 | All page-2 drill items |
| `madas_topic_integration_by_a_special_manipulation_q2_sp2_drill_exact_inputs` | special_manipulation | q2 | 10 | All page-3 drill items |
| `madas_topic_integration_by_a_special_manipulation_student_version_q1_sp2_drill_exact_inputs` | special_manipulation (student) | q1 | 10 | Full student q1 |
| `madas_topic_integration_basics_q1_sp2_drill_exact_inputs` | integration_basics | q1 | 10 | Page 2; item 9 uses giac marker `3*(1-4x)^-5` (worksheet prints `6*…`, likely typo) |
| `madas_topic_integration_basics_q2_sp2_drill_exact_inputs` | integration_basics | q2 | 10 | Page 3 |
| `madas_topic_integration_partial_fractions_q1_sp2_drill_exact_inputs` | partial_fractions | q1 | 2 | Items 1–2 via decomposed integrands; items 3–10 need partfrac route (host integrator does not decompose arbitrary rationals inline) |

## Per-source audit status

| Source | PNG q | Queue q | First-pass issue | Second-pass action |
|--------|------:|--------:|------------------|-------------------|
| integration_areas | 38 | 38 | Recycled pool inputs | **Remaining:** ~380 drill items |
| integration_basics | 20 | 20 | Recycled pool | **Done q1–q2** (20 inputs); q3–q20 remaining (~180 items) |
| integration_basics_student_version | 20 | 20 | Same as basics | Remaining (~200 items) |
| integration_by_a_special_manipulation | 2 | 2 | Recycled pool | **Complete** (20 inputs) |
| integration_by_a_special_manipulation_student_version | 1 | 1 | Recycled pool | **Complete** (10 inputs) |
| integration_by_parts | 15 | 15 | Recycled pool | Remaining (~150 items) |
| integration_by_parts_student_version | 9 | 9 | Recycled pool | Remaining (~90 items) |
| integration_by_reverse_chain_rule | 14 | 14 | Recycled pool | Remaining (~140 items) |
| integration_by_reverse_chain_rule_student_version | 9 | 9 | Recycled pool | Remaining (~90 items) |
| integration_by_substitution | 25 | 25 | Recycled pool | Remaining (~250 items) |
| integration_by_substitution_student_version | 14 | 14 | Recycled pool | Remaining (~140 items) |
| integration_by_trigonometric_identities | 16 | 16 | Recycled pool | Remaining (~160 items) |
| integration_by_trigonometric_identities_student_version | 9 | 9 | Recycled pool | Remaining (~90 items) |
| integration_indefinite_mix | 114 | 114 | Recycled pool | Remaining (~1,140 items) — largest drill gap |
| integration_indefinite_mix_student_version | 31 | 31 | Recycled pool | Remaining (~310 items) |
| integration_indefinite_mix_student_version_condense | 17 | 17 | All `partial`, recycled pool | Remaining (~170 items); upgrade verdict once real inputs added |
| integration_numerical | 53 | 53 | Recycled pool / numeric methods | Remaining; trapezium/Simpson may stay `partial` |
| integration_partial_fractions | 10 | 10 | Recycled pool | **Started q1** (2/10 via decomposed form) |
| integration_partial_fractions_student_version | 5 | 5 | Recycled pool | Remaining (~50 items) |
| integration_structured_exam_questions_part_i | 267 | 267 | Wrong mapping + 66 false skips | **Reviewed q6** — needs partfrac/`defint` rows; 66 skip rows need reclassification |
| integration_volume_of_revolution | 77 | 77 | Recycled pool | Remaining; `defint` with π likely testable |
| odes_context_modelling | 106 | 106 | Recycled pool | Remaining; separable ODE solves may be testable |
| odes_separable_no_context | 81 | 81 | Recycled pool | Remaining |
| parametric_integration_exam_questions | 91 | 91 | Wrong mapping + 22 false skips | **Reviewed** section header; non-skipped rows still use recycled pool |

## Skip-row review (exam topics)

Auto-skip rule in `_build_standard_topics.py` (`"exam" in name and n % 4 == 0`) produced:

- `integration_structured_exam_questions_part_i`: **66 skip rows** (e.g. q4, q8, …)
- `parametric_integration_exam_questions`: **22 skip rows**

Sample re-read of **structured exam q6** (page 5): genuine testable parts — partial-fraction constants A=3, B=1; definite integral `∫₀⁴ (5x+13)/((2x+1)(x+4)) dx = ln 54`. Current queue row maps unrelated `integrate(sin(2x))` / `integrate(cos(3x))`. **Skip rows at q4/q8/etc. are not justified without per-page review** — many contain standard CAS steps.

## Manual-only gaps (no action)

- Proof/show-that wording (`Question 7` substitution proof on structured exam page 5)
- Diagram/setup steps on parametric area questions
- Numerical method choice/rounding judgements on `integration_numerical`
- Volume-of-revolution sketch/setup where axis interpretation is manual

## Validation

```
python3 tools/_worker20_sp2_integration.py          # build + verify → 6 rows, 52 inputs
python3 tools/append_queue_rows.py progress/batches/worker20_sp2_integration_rows.json
python3 tests/check_exact_calculator_input_queue.py
# OK exact calculator queue rows=9145 inputs=12720

python3 tests/run_exact_queue.py --strict-markers
# OK exact queue run 12718/12720 ok bad=2
# Failures (pre-existing, syn scope): syn_a_q2, syn_m_q1 — not worker 20

sp2-only host verify: 52/52 ok
```

## Resume point / recommended follow-up

1. Continue drill extraction for `integration_basics` q3–q20 (pages 4–21 already partially read; data in worker notes).
2. Extend `_worker20_sp2_integration.py` pattern to remaining drill topics (~900 question pages × ~8 items ≈ **7,200 inputs**).
3. Replace exam-topic rows with page-accurate part-level inputs; downgrade false skips after page review.
4. Add partfrac-assisted rows for `integration_partial_fractions` items 3–10 (decompose then `integrate`).
5. Consider regenerating batch files from page-specific data rather than patching recycled first-pass rows.

## Files touched

- `progress/batches/worker20_sp2_integration_rows.json` (new)
- `tools/_worker20_sp2_integration.py` (new builder/verifier)
- `tests/golden/exact_calculator_input_queue.jsonl` (+6 rows)
- `progress/vm_second_pass/worker_20.md` (this report)
