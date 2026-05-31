# Worker 06 — VM second-pass audit

**Scope:** MadAsMaths papers `c3_n` … `c3_z` (14 papers)  
**VM root:** `/Volumes/VM/MadAsMaths papers/`  
**Queue:** `tests/golden/exact_calculator_input_queue.jsonl`  
**Patch batch:** `progress/batches/worker_06_second_pass_patches.json`

## Summary

| Metric | Before | After |
|--------|--------|-------|
| Papers | 14 | 14 |
| Question rows (excl. markers) | 99 | 102 (+3 new) |
| Executable inputs (c3_n–z) | 136 | 160 (+24) |
| Strict host verify (c3_n–z) | not run | **160/160 OK** |
| Queue schema check | — | **OK** |

## Critical first-pass errors found

### Mis-mapped papers (question content did not match VM)

| Paper | Issue |
|-------|--------|
| **c3_t** | All q1–q7 rows mapped to wrong paper template (cubic iteration, harmonic form, etc.). q8–q9 are **phantom** — cover page states **7 questions**; only 4 question PNGs exist. |
| **c3_z** | q1–q7 mapped to wrong template (factor/iteration/tan15/modelling). Actual paper: implicit log diff, trig proof, quotient turning point, arctan iteration, mushroom model, cos inverse, sum-to-product. q8 missing. |
| **c3_s** | q3–q4 wrong (iteration/tan15). Actual q3 = trig equation; q4 = `\|x²−16\|+2x` sketch. q5–q7 **missing** entirely. |

### Marker / input bugs (well-mapped papers)

| Row | Fix |
|-----|-----|
| `c3_n_q5` | `log(2)/2` → `ln(2)/2` (first pass used base-10 log; answer is `x=½ ln 2`) |
| `c3_r_q1` | Working label `f(4)=15` → `f(4)=13` (marker was already correct) |

## Actions taken

### Patched existing rows (27)

- **c3_n:** +7 inputs (iteration x₂, alpha bracket signs, discriminant √3025, harmonic α); fixed q5 ln marker
- **c3_o:** +3 inputs (alpha tighten, graph point y-values)
- **c3_p:** +1 input (iteration x₂)
- **c3_q:** +2 inputs (root bracket f(3), f(4))
- **c3_s:** remapped q3→skip, q4→cusps (−4,−8)/(4,8)
- **c3_t:** remapped q1–q7 to actual paper T content; q8/q9→skip (phantom)
- **c3_z:** remapped q1–q7; fixed q5 modelling inputs

### New rows appended (4)

| ID | Content |
|----|---------|
| `madas_iygb_c3_s_q5_exact_inputs` | Composite fg/gf numeric checks at x=½ |
| `madas_iygb_c3_s_q6_exact_inputs` | tan θ=½, tan(3θ+5φ)=−4/3 |
| `madas_iygb_c3_s_q7_exact_inputs` | Circle optimisation θ=π/3, BD length |
| `madas_iygb_c3_z_q8_exact_inputs` | Graph transform cusp y=√2 at x=−1 |

## Paper-by-paper notes

| Paper | Q pages | Sol pages | Status |
|-------|---------|-----------|--------|
| c3_n | 6 | 7 | OK — extended iteration/alpha/harmonic |
| c3_o | 7 | 6 | OK — extended alpha + graph coords |
| c3_p | 6 | 7 | OK — extended iteration |
| c3_q | 5 | 6 | OK — extended root bracket |
| c3_r | 6 | 7 | OK — label fix only |
| c3_s | 4 | 7 | **Fixed** — remapped q3–4, added q5–7 |
| c3_t | 4 | 10 | **Remapped** — full q1–7 rewrite; q8–9 phantom |
| c3_u | 5 | 8 | OK — no gaps found |
| c3_v | 5 | 9 | OK — no gaps found |
| c3_w | 5 | 9 | OK — no gaps found |
| c3_x | 5 | 8 | OK — no gaps found |
| c3_y | 6 | 9 | OK — no gaps found |
| c3_z | 6 | 8 | **Remapped** — full q1–7 rewrite; added q8 |

## Validation

```text
python3 tests/check_exact_calculator_input_queue.py
→ OK exact calculator queue rows=9233 inputs=12909

Worker-scoped strict host run (c3_n–c3_z, all non-skip inputs):
→ 160/160 OK
```

All new/changed markers verified via `tools/khicas_host_runner` before commit.

## Residual manual (not queueable)

- Long division / partial-fraction proofs (c3_n/o/p/w/x/y q1)
- Product/quotient/log implicit differentiation proofs
- Trig identity proofs (c3_t q2–3/q7, c3_z q2/q7, c3_s q3)
- Modulus / composite-function graph sketches
- Geometry diagrams (c3_n q9, c3_t q6, c3_s q7)

## Files changed

- `tests/golden/exact_calculator_input_queue.jsonl` — 27 patches + 4 appends
- `progress/batches/worker_06_second_pass_patches.json` — reproducible patch spec

**Worker 06 complete.** 2026-05-31
