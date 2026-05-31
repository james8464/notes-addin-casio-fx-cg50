# VM golden queue second-pass — Worker 05/22

**Scope:** MadAsMaths papers `c3_a` … `c3_m` (`MadAsMaths papers/c3_*.pdf`)  
**Repo:** `/Users/james/Developer/CASIO`  
**VM:** `/Volumes/VM/MadAsMaths papers/`  
**Date:** 2026-05-31

## Summary

| Metric | Value |
|--------|------:|
| Papers re-scanned | 13 |
| Question rows in queue (excl. markers) | 118 |
| Source-complete markers | 13 |
| PNG page counts vs `review_basis` | All match |
| Queue rows patched (inputs added/replaced) | 10 |
| New supplemental row IDs | 0 |
| Host strict-marker check (c3_a–m, post-patch) | **161/161 OK** |
| Schema check (`check_exact_calculator_input_queue.py`) | **OK** (9231 rows) |

Second pass re-read VM question/marks/solution PNGs against existing queue rows. Most papers already had q1–q9 (or q1–q8 / q10 where applicable) with appropriate skip/partial/testable splits. Gaps were filled with host-verified `method=numeric` / `ln` steps; one row had the **wrong cubic** and was corrected.

## VM inventory (conv_png)

| Paper | Q pages | Marks | Solutions | Questions in queue |
|-------|--------:|------:|----------:|-------------------:|
| c3_a | 6 | 4 | 5 | q1–q9 |
| c3_b | 6 | 5 | 5 | q1–q9 |
| c3_c | 6 | 5 | 6 | q1–q9 |
| c3_d | 6 | 5 | 6 | q1–q9 |
| c3_e | 6 | 5 | 6 | q1–q9 |
| c3_f | 6 | 4 | 5 | q1–q10 |
| c3_g | 6 | 4 | 6 | q1–q8 |
| c3_h | 6 | 5 | 6 | q1–q9 |
| c3_i | 5 | 1 | 7 | q1–q10 |
| c3_j | 6 | 1 | 8 | q1–q8 |
| c3_k | 7 | 1 | 7 | q1–q9 |
| c3_l | 6 | 1 | 7 | q1–q10 |
| c3_m | 6 | 1 | 6 | q1–q9 |

Cover pages confirm 8- or 9-question papers where applicable (e.g. c3_g/c3_j: 8 questions; c3_h/c3_k: 9).

## Gaps found and fixed

### Critical correction

- **`madas_iygb_c3_h_q1_exact_inputs`** — Inputs used `x³+3x−5` (Paper L’s equation). Paper H Q1 is `x³−x²−6x−6=0` with root in **(3,4)**. Replaced inputs with `f(3)`, `f(4)`, iteration step `x₁` from `x₀=3.3`, and sign-change checks at 3.336905 / 3.336915.

### Iteration / sign-change additions

| Row | Change |
|-----|--------|
| `madas_iygb_c3_a_q1_exact_inputs` | +`f(0)`, `f(1)` bracket values |
| `madas_iygb_c3_a_q2_exact_inputs` | +normal gradient `−4` |
| `madas_iygb_c3_c_q2_exact_inputs` | +`x₁` from `(2−e^{−4})²`, sign change at 3.92105/3.92115 |
| `madas_iygb_c3_e_q2_exact_inputs` | +`x₁=(1/3)ln(21.5)` (natural log; `log` is base-10 on host) |
| `madas_iygb_c3_m_q2_exact_inputs` | +`x₂`, sign-change at 0.254095/0.254105 |
| `madas_iygb_c3_j_q3_exact_inputs` | Replaced incorrect `(1/1.22)^(1/3)+1` with iteration + sign-change steps |
| `madas_iygb_c3_j_q1_exact_inputs` | Replaced spurious `1/(2(1+0)²)` with `y(1)=1`, `dy/dx(1)=0` |
| `madas_iygb_c3_j_q4_exact_inputs` | +`f(π)=0` |
| `madas_iygb_c3_h_q2_exact_inputs` | +`T` at `t=30` |

Patch applied via `tools/_second_pass_worker05_patch.py`; affected rows note `vm_second_pass worker_05` in `review_basis`.

## Remaining manual-only (no change)

Unchanged and still correctly classified as **skip** or **partial** with diagram/proof/branch/iteration-layout reasons:

- Graph transformation sketches (e.g. c3_a q6, c3_b q3, c3_j q2)
- Trig identity proofs (c3_c q8, c3_d q3, c3_j q6–q7)
- Harmonic-form layout and degree-branch selection (many partial rows)
- `diff(x*cos(2x),x)` / `diff(sin(x)/x,x)` — host `--derive` does not emit markers (c3_a q5 b/c left as single chain-rule row for part a only)

No missing question numbers vs VM for any paper in scope.

## Validation

1. **Batch cross-check:** `progress/batches/madas_c3_{a..m}_rows.json` — all 13 batches `verify_batch_markers.py` OK before patch; input *counts* matched queue; 43 rows differed in input *strings* (prior `fix_failing_queue_inputs` drift).
2. **Post-patch host verify:** All 161 non-skip inputs under c3_a–m pass strict substring markers (`--alg` / `--derive`).
3. **Schema:** `python3 tests/check_exact_calculator_input_queue.py` → OK (9231 rows, 12887 inputs).

Full-repo `run_exact_queue.py --strict-markers` not re-run (9000+ rows); worker scope strictly validated as above.

## Papers with no queue edits

c3_b, c3_d, c3_f, c3_g, c3_i, c3_k, c3_l — coverage adequate after image re-scan; existing partial/skip boundaries accepted.

## Resume / handoff

Worker 06 owns `c3_n`–`c3_z`. No blockers.
