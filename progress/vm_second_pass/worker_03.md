# VM second-pass audit — Worker 03/22

**Scope:** MadAsMaths papers `c2_a` through `c2_m` (13 papers)  
**VM source:** `/Volumes/VM/MadAsMaths papers/{c2_X,c2_X_marks,c2_X_solutions} conv_png/`  
**Queue:** `tests/golden/exact_calculator_input_queue.jsonl`  
**Date:** 2026-05-31

## Summary

| Metric | Before | After |
|--------|--------|-------|
| Queue rows (c2_a–m) | 145 | 151 |
| Testable inputs | 283 | 296 |
| Strict marker pass | 283/283 | **296/296** |
| Rows replaced | — | 1 |
| Rows appended (_sp2_) | — | 6 |

Re-read all question, marks, and solution PNGs for each paper. Compared every question/part to existing queue rows. Host-verified all markers via `tools/khicas_host_runner`. Applied patch `progress/batches/madas_c2_a_m_second_pass_rows.json`.

## Strict marker validation

```
c2_a–m: 296/296 strict OK (0 failures)
check_exact_calculator_input_queue: OK (9264 rows, 13038 inputs)
```

All pre-existing markers were already host-valid. Three markers in the new `c2_b_q4_sp2` row required correction during apply (numeric precision alignment with host output).

## Corrections

### `c2_k` Q1b — wrong CAS equation (replaced)

**Problem:** Row used `solve(y^2-16=0,y)` → `y = [4, -4]`, but the mark scheme substitutes `x=0` into `(x+3)²+(y-1)²=25` giving `(y-1)²=16` → **a = 5 or −3**.

**Fix:** Replaced input #4 with `solve((y-1)^2=16,y)` → `y = [5, -3]`. Markers now match both the solution PNG and `expected_final`.

## Missed CAS steps appended (_sp2_ rows)

| ID | Paper | Question | Steps added |
|----|-------|----------|-------------|
| `madas_iygb_c2_a_q7_sp2_exact_inputs` | c2_a | Q7d | Arc length `4*1.2 = 24/5` |
| `madas_iygb_c2_b_q4_sp2_exact_inputs` | c2_b | Q4b | Sector area, triangle area, segment difference (decomposed) |
| `madas_iygb_c2_b_q8_sp2_exact_inputs` | c2_b | Q8 | A₂ definite integral eval `81/2−72+27−(1/2−8/3+3) = −16/3` |
| `madas_iygb_c2_k_q2_sp2_exact_inputs` | c2_k | Q2 | y at x=−5; f''(−5)=−0.4 (local max) |
| `madas_iygb_c2_m_q2_sp2_exact_inputs` | c2_m | Q2a–b | `complete_square` for x and y; radius²=25 |
| `madas_iygb_c2_m_q4_sp2_exact_inputs` | c2_m | Q4 | First/second/third derivatives via `diff(...)` |

## Per-paper audit

Each paper has Q1–Q10 (Q11 on c2_h, c2_i, c2_j, c2_m) plus a `*_complete_source_marker` skip row.

| Paper | Questions | Queue rows | Inputs | Second-pass notes |
|-------|-----------|------------|--------|-------------------|
| c2_a | 10 | 12 | 36 | Complete. Added Q7d arc length. Q4 derive/solve remain manual (unsupported). |
| c2_b | 10 | 13 | 26 | Added Q4b decomposed segment + Q8 A₂ integral. Q6b/c, Q7a sketch manual. |
| c2_c | 10 | 11 | 20 | Complete. Binomial, factor, logs, trig covered. |
| c2_d | 10 | 11 | 20 | Complete. No missed numeric steps found. |
| c2_e | 9 | 10 | 22 | Complete (no Q10 on paper). Q3 decreasing interval manual. |
| c2_f | 10 | 11 | 22 | Complete. |
| c2_g | 9 | 10 | 17 | Complete. Q4 skip (log laws in p,q) adequate. |
| c2_h | 11 | 12 | 22 | Complete. Q11 sketch-only partial adequate. |
| c2_i | 11 | 12 | 24 | Complete. Q1 antiderivative setup manual. |
| c2_j | 11 | 12 | 19 | Complete. |
| c2_k | 10 | 12 | 26 | **Q1b fixed.** Q2 max point added. |
| c2_l | 10 | 11 | 17 | Complete. Q9 radian filtering manual. |
| c2_m | 11 | 14 | 25 | Q2 centre + Q4 inflection diffs added. Q1 factor already present. |

## Skip / partial rows verified adequate

- **c2_g Q4** (`skip`, 0 inputs): log-law proof in terms of p, q — no single host input; skip retained.
- **All `*_complete_source_marker` rows** (13): diagram setup, proof wording, interval/branch selection, and rounding judgements correctly marked manual via `unsupported_reason`.
- **Partial rows with 1–2 inputs:** Thin input count reflects manual setup (trapezium tables, sketches, degree-mode branch selection). No additional host-testable arithmetic found beyond what is already queued, except the _sp2_ additions above.

## Residual manual (not queueable)

Common patterns left as manual across the scope:

- Trapezium-rule **y-value tables** (student reads from question or computes off-page)
- **Degree-mode** trig general solutions and range filtering
- **Diagram / sketch** reasoning (shaded regions, geometry setup)
- **Show-that** algebraic eliminations (e.g. c2_b Q10a surface-area → volume)
- **Exact π forms** and exam rounding judgements

## Files touched

- `progress/batches/madas_c2_a_m_second_pass_rows.json` — patch source
- `tests/golden/exact_calculator_input_queue.jsonl` — 1 replace, 6 append
- `progress/vm_coverage.json` — updated last_batch for c2_a, c2_b, c2_k, c2_m

## Worker boundary

Worker 04 owns `c2_n` through `c2_z`.
