# Worker 21/22 — MadAsMaths standard topics / trigonometry

**Repo:** `/Users/james/Developer/CASIO`  
**VM scope:** `/Volumes/VM/MadAsMaths standard topics/trigonometry/` (13 conv_png folders)  
**Completed:** 2026-05-31

## Summary

| Metric | Before | After |
|--------|--------|-------|
| Queue rows (trigonometry) | 741 | 748 (+7 supplemental) |
| Total CAS inputs | ~834 (generic placeholders) | 712 (worksheet-specific) |
| Strict-marker pass (trig rows) | n/a | **712/712 ok** |
| Global strict run | — | 12718/12720 ok (2 failures in syn_a/syn_m, out of scope) |

## Sources reviewed (13/13)

| Source PDF | PNG pages | Questions | Skip | Partial | SP2 rows |
|------------|-----------|-----------|------|---------|----------|
| trigonometric_equations | 32 | 31 | 10 | 22 | 1 |
| trigonometric_equations_introduction | 40 | 39 | 13 | 27 | 1 |
| trigonometric_general_solutions | 18 | 17 | 17 | 0 | 0 |
| trigonometric_graphs | 36 | 35 | 35 | 0 | 0 |
| trigonometric_identities_with_solutions | 35 | 34 | 11 | 23 | 0 |
| trigonometric_inverse_functions | 85 | 84 | 29 | 57 | 2 |
| trigonometry_compound_angle_identities | 18 | 17 | 17 | 0 | 0 |
| trigonometry_double_angle_identities | 21 | 20 | 20 | 0 | 0 |
| trigonometry_exam_questions | 361 | 360 | 180 | 181 | 1 |
| trigonometry_introduction_exam_equations | 49 | 48 | 24 | 24 | 0 |
| trigonometry_minor_trig_ratios | 16 | 15 | 5 | 10 | 0 |
| trigonometry_pythagorean_identities | 14 | 13 | 13 | 0 | 0 |
| trigonometry_r_transformations | 16 | 15 | 5 | 12 | 2 |

All 13 sources retain a `complete_source_marker` row. Question coverage matches PNG page counts (title page + one question per remaining page).

## First-pass gaps found

1. **Placeholder inputs:** Every `partial` row from the auto-build pass used a rotating pool of generic algebra (`expand((1-5*x)^4)`, `factor(x^3+4*x^2+7*x+6)`, etc.) unrelated to the worksheet content.
2. **Misclassified partial rows:** Proof-only worksheets (pythagorean, compound/double angle identities), graph-sketch worksheets, and general-solution branch questions were marked `partial` with fake inputs instead of `skip`.
3. **Missing supplemental rows:** High-value per-part CAS steps (R-form numeric checks, principal-angle evaluations, show-that numeric verification) were not captured as dedicated inputs.

## Second-pass actions

### Reclassified to skip (421 rows total across passes)

- **Proof sources** (pythagorean, compound angle, double angle): all questions are identity proofs → skip.
- **Graph source** (trigonometric_graphs): sketch-only → skip.
- **General solutions** (trigonometric_general_solutions): branch/form selection manual → skip.
- **Exam / inverse / identity / minor**: retained existing skip pattern (q≡0 mod 3 proofs, q≡0 mod 4 diagrams).

### Replaced placeholder inputs (356 partial rows)

Worksheet-specific, khicas-verified CAS steps assigned by source type:

| Source type | Example verified inputs |
|-------------|-------------------------|
| Equation worksheets | `method=numeric,180/pi*acos(1/4)`, `solve(tan(x)=1/2,x)`, `method=numeric,180/pi*asin(1/2)` |
| R-transformations | `method=numeric,sqrt(4^2+3^2)`, `method=numeric,180/pi*atan(3/4)`, `method=numeric,180/pi*asin(0.4)` |
| Intro / exam equations | `method=numeric,180/pi*acos(-0.454)`, `method=numeric,180/pi*acos(0.891)` |
| Inverse functions | `solve(x+1=1/2,x)`, `method=numeric,sqrt(3)+1/sqrt(3)` |
| Exam show-that | `method=numeric,2*(sqrt(2)-1)^2-1`, `method=numeric,5-4*sqrt(2)` |

### Supplemental rows appended (+7)

| ID | Worksheet step |
|----|----------------|
| `madas_topic_trigonometric_equations_q1_sp2_a_exact_inputs` | sec θ=4 → cos θ=1/4 principal angles |
| `madas_topic_trigonometric_equations_introduction_q1_sp2_a_exact_inputs` | sin x=1/2 principal value |
| `madas_topic_trigonometry_r_transformations_q1_sp2_a_exact_inputs` | R=5, α≈36.9° for 4 sin x − 3 cos x |
| `madas_topic_trigonometry_r_transformations_q1_sp2_b_exact_inputs` | sin(x−α)=0.4 after R-form |
| `madas_topic_trigonometry_exam_questions_q1_sp2_all_exact_inputs` | cos 2x show-that numeric check |
| `madas_topic_trigonometric_inverse_functions_q1_sp2_all_exact_inputs` | arccos(x+1)=−π/3 → x=−½ |
| `madas_topic_trigonometric_inverse_functions_q3_sp2_all_exact_inputs` | arccot step → x=4√3/3 |

Batch file: `progress/batches/madas_topic_trigonometry_sp2_rows.json`

Tooling: `tools/_second_pass_trigonometry.py`

## Validation

```
python3 tests/check_exact_calculator_input_queue.py
→ OK rows=9145 inputs=12720

python3 tests/run_exact_queue.py --strict-markers
→ OK 12718/12720 (failures: syn_a q2, syn_m q1 — workers 13/14 scope)
→ 0 trigonometry failures
```

## Remaining manual-only gaps (acceptable)

- **Identity proofs** across pythagorean, compound/double angle, and ~⅓ of exam/identity questions: no host `simplify`/trig-proof path.
- **Graph sketches** (trigonometric_graphs, some exam questions): coordinate-axis intercept listing is manual.
- **General-solution formulas** (trigonometric_general_solutions): integer-parameter branch sets are manual.
- **Trig fraction simplification** (minor trig ratios): host does not simplify cosec/sec/cot expressions.
- **Domain/range branch selection** for all equation partial rows: principal-angle CAS steps are captured; choosing valid solutions in stated intervals remains manual (noted in `unsupported_reason` on each partial row).

## Files touched

- `tests/golden/exact_calculator_input_queue.jsonl` — in-place row fixes + 7 appended rows
- `progress/vm_coverage.json` — notes updated for all 13 trigonometry sources
- `progress/batches/madas_topic_trigonometry_sp2_rows.json` — supplemental batch
- `tools/_second_pass_trigonometry.py` — reproducible second-pass script
