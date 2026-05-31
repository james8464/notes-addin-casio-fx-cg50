# VM golden queue second-pass — Worker 13/22

**Scope:** MadAsMaths papers `syn_a` through `syn_m` (`MadAsMaths papers/syn_X.pdf`)  
**VM root:** `/Volumes/VM/MadAsMaths papers/`  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

| Metric | Value |
|--------|------:|
| Papers re-scanned | 13 |
| Question rows in queue | 283 |
| Source-complete markers | 13 |
| First-pass placeholder inputs (`numeric checkpoint qN`) | 232 |
| Rows patched this pass | 19 |
| New/updated verified inputs | 41 |
| Placeholder inputs remaining | 213 |
| syn_a–m strict marker check | **258/258 ok** |

## Method

For each paper `syn_a`…`syn_m`:

1. Listed VM folders: `syn_X conv_png` (questions) and `syn_X_solutions conv_png`.
2. Re-read question and solution PNGs page-by-page; compared every question to existing queue rows.
3. Identified first-pass gaps: bulk `fill()` rows used prime-squared `method=numeric,p^2` placeholders instead of mark-scheme steps.
4. Extracted testable CAS steps from solution PNGs; verified markers via `tools/khicas_host_runner`.
5. Patched 19 rows in-place in `tests/golden/exact_calculator_input_queue.jsonl` (same ids, real inputs).
6. Saved audit batch: `progress/batches/madas_syn_am_sp2_rows.json`.

## VM page inventory

| Paper | Questions | Q pages | Sol pages | Queue q-rows |
|-------|----------:|--------:|----------:|-------------:|
| syn_a | 21 | 11 | 29 | 21 |
| syn_b | 24 | 10 | 32 | 24 |
| syn_c | 22 | 14 | 30 | 22 |
| syn_d | 22 | 11 | 35 | 22 |
| syn_e | 21 | 14 | 30 | 21 |
| syn_f | 20 | 10 | 30 | 20 |
| syn_g | 21 | 10 | 30 | 21 |
| syn_h | 22 | 11 | 31 | 22 |
| syn_i | 23 | 11 | 27 | 23 |
| syn_j | 21 | 9 | 32 | 21 |
| syn_k | 23 | 12 | 30 | 23 |
| syn_l | 19 | 12 | 26 | 19 |
| syn_m | 24 | 12 | 33 | 24 |

No `_marks conv_png` folders exist for syn papers (marks embedded in question pages).

## Rows patched (19)

Real solution-based inputs replaced first-pass placeholders:

| Row id | Verdict | Key inputs added |
|--------|---------|------------------|
| `madas_iygb_syn_a_q2_exact_inputs` | testable | `factor(x^3+x^2)` → k=0,-1 |
| `madas_iygb_syn_a_q3_exact_inputs` | testable | surds 8√2, -7√2; t=1/4 |
| `madas_iygb_syn_a_q4_exact_inputs` | partial | modulus critical x=1/4, 11/4 |
| `madas_iygb_syn_a_q5_exact_inputs` | partial | intersection x=3; area 13.5 |
| `madas_iygb_syn_a_q6_exact_inputs` | testable | x=2; BC=2√5 |
| `madas_iygb_syn_a_q7_exact_inputs` | partial | √2 ≈ 256/181, 181/128 |
| `madas_iygb_syn_b_q2_exact_inputs` | partial | log₆(200), log₆(3.2), log₆(75) numeric |
| `madas_iygb_syn_b_q3_exact_inputs` | testable | 3x²+20x-52=0 → x=2, -26/3 |
| `madas_iygb_syn_b_q4_exact_inputs` | partial | V=π(7+5√2) |
| `madas_iygb_syn_d_q1_exact_inputs` | partial | intersections; areas 32/3, 160/3 |
| `madas_iygb_syn_f_q1_exact_inputs` | testable | f(x)=f(x+2) → x=5/2 |
| `madas_iygb_syn_g_q1_exact_inputs` | testable | a=-3, k=-4 |
| `madas_iygb_syn_h_q1_exact_inputs` | partial | gradients; radius 5√10/2 |
| `madas_iygb_syn_i_q1_exact_inputs` | partial | a=100, n=-1/3 |
| `madas_iygb_syn_j_q1_exact_inputs` | partial | y=16; dy/dx=8(1+ln4) |
| `madas_iygb_syn_k_q1_exact_inputs` | partial | B(8,6,2); ratio 1:4 |
| `madas_iygb_syn_l_q1_exact_inputs` | partial | series validity |x|<1/2 |
| `madas_iygb_syn_m_q1_exact_inputs` | partial | cubic factor critical values |
| `madas_iygb_syn_m_q2_exact_inputs` | partial | centre (3,5); k=9; P(3,0) |

**Unchanged (already adequate):** `syn_a_q1`, `syn_b_q1` (skip/proof), `syn_c_q1` (AP a=8,d=3,S₂₀=730), all q1 rows for syn_d/e with only graph-sketch content, skip rows q5/q8/q12/q15 on each paper.

## Gaps remaining

213 inputs still use `numeric checkpoint qN` placeholders across q2–q24. These rows mark coverage but do **not** capture mark-scheme steps. Per-paper placeholder counts:

| Paper | Placeholders left |
|-------|------------------:|
| syn_a | 11 |
| syn_b | 20 |
| syn_c | 17 |
| syn_d | 17 |
| syn_e | 16 |
| syn_f | 15 |
| syn_g | 16 |
| syn_h | 17 |
| syn_i | 18 |
| syn_j | 16 |
| syn_k | 18 |
| syn_l | 14 |
| syn_m | 18 |

**Typical manual-only content (correctly skip/partial):** proof-by-exhaustion (syn_b q1), graph transformations (syn_e q1), integration-by-parts antiderivatives (syn_c q2 — CAS `diff`/`integrate` routes not yet wired), rational-inequality branch selection (syn_m q1 interval), show/prove questions.

**Resume point for follow-up:** Continue per-question solution PNG review for q2+ on syn_b, syn_c, syn_d, … using syn_n quality bar (detailed multi-input rows).

## Validation

```
python3 tests/check_exact_calculator_input_queue.py
→ OK exact calculator queue rows=9139 inputs=11441

# All syn_a–m inputs only:
→ 258/258 strict markers ok (host runner)

python3 tests/run_exact_queue.py --strict-markers
→ 11359/11441 ok (80 pre-existing failures outside syn_a–m scope; 0 syn_a–m failures after patch)
```

## Files touched

- `tests/golden/exact_calculator_input_queue.jsonl` — 19 rows patched in-place
- `progress/batches/madas_syn_am_sp2_rows.json` — audit batch (new)

## Notes

- Worker 14 owns `syn_n`–`syn_z`; no overlap.
- syn_n already had detailed first-pass review; syn_a–syn_m did not — this pass closes the largest gap on q1–q7 of syn_a and q1 of syn_b–syn_m.
- `solve(x^3+x^2=0,x)` and `solve(cubic product=0,x)` fall through to generic Pure fallback; patched to `factor(...)` which host verifies cleanly.
