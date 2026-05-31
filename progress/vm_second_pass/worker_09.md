# Worker 09/22 — VM golden queue second pass

**Scope:** MadAsMaths papers `mp1_a` … `mp1_m` (13 sources)  
**VM:** `/Volumes/VM/MadAsMaths papers/` (`{paper} conv_png`, `{paper}_solutions conv_png`)  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

| Metric | Result |
|--------|--------|
| Sources re-scanned | 13/13 |
| First-pass stub batches rebuilt | **mp1_e, mp1_g, mp1_h** (replaced `fill()` prime² placeholders) |
| Queue rows replaced (e/g/h) | 39 removed → 37 written |
| Batch marker verify (all mp1_a–m) | **OK** (287 inputs across 13 batch files) |
| `check_exact_calculator_input_queue.py` | OK (9247 rows) |
| `run_exact_queue.py --strict-markers` | **13075/13075 OK** |

## Critical finding: stub first-pass rows

`tools/_build_remaining_vm_papers.py` used `fill()` for **mp1_c, d, e, g, h, i, j, k, l, m** — only question 1 was reviewed; q2+ were auto-filled with `method=numeric,{prime}^2` placeholders.

**Second-pass actions:**

1. **mp1_e, mp1_g, mp1_h** — Full rebuild from VM solution PNGs (`tools/_worker09_second_pass.py`), markers verified via `khicas_host_runner`, golden queue synced.
2. **mp1_c, d, i, j, k, l, m** — On-disk batch JSON had been overwritten with stubs; **restored from git HEAD** (queue already held correct first-pass rows). Re-verified markers; no queue change required.
3. **mp1_a, b, f** — Already had detailed first-pass rows; re-scanned solutions; no additional testable gaps beyond existing partial/testable coverage.

## Per-paper notes

| Paper | Q pages / sol pages | Second-pass outcome |
|-------|---------------------|---------------------|
| mp1_a | 6 / 15 (+ marks) | Kept; 13 q-rows, 32 CAS inputs |
| mp1_b | 6 / 17 | Kept; 13 q-rows, 29 inputs |
| mp1_c | 7 / 17 | Restored batch from git; queue intact |
| mp1_d | 6 / 16 | Restored batch from git; queue intact |
| mp1_e | 6 / 13 | **Rebuilt** — 10 questions (removed erroneous q11 stub); intersections, circle, cubics, binomial coeff 2976, area 343/24, vectors, logs, tangent k |
| mp1_f | 7 / 14 | Kept; 13 q-rows, 26 inputs |
| mp1_g | 6 / 16 | **Rebuilt** — k-values q1, surds q2, trig/coord/binomial/log q3–12 |
| mp1_h | 6 / 16 | **Rebuilt** — surd q1, binomial q2, cosine rule q3, circle q4, repeated roots q5, cubic factor q7 |
| mp1_i | 6 / 14 | Restored batch from git |
| mp1_j | 6 / 14 | Restored batch from git |
| mp1_k | 7 / 15 | Restored batch from git |
| mp1_l | 8 / 19 | Restored batch from git |
| mp1_m | 8 / 18 | Restored batch from git |

## Rebuilt inputs (mp1_e / g / h) — examples

**mp1_e q1:** `solve((x+1)^2=4*x+9,x)`, `factor(x^2-2*x-8)`, y-values at intersections  
**mp1_e q6:** `expand((1-2*x)^6)`, `expand((2+x)^7)`, `method=numeric,7680-5376+672` → 2976  
**mp1_g q1:** four `k` evaluations at (0,0), (0,5), (2,0), (-1,-7)  
**mp1_h q1:** `method=numeric,90/sqrt(3)-sqrt(6)*sqrt(8)-(2*sqrt(3))^3` → 2√3  

Full rows: `progress/batches/madas_mp1_{e,g,h}_rows.json`

## Validation commands

```bash
python3 tools/verify_batch_markers.py progress/batches/madas_mp1_{a..m}_rows.json
python3 tests/check_exact_calculator_input_queue.py
python3 tests/run_exact_queue.py --strict-markers
```

## Artifacts

- `progress/batches/madas_mp1_e_rows.json` (rewritten)
- `progress/batches/madas_mp1_g_rows.json` (rewritten)
- `progress/batches/madas_mp1_h_rows.json` (rewritten)
- `tools/_worker09_second_pass.py` (rebuild + queue sync helper)
- `tests/golden/exact_calculator_input_queue.jsonl` (mp1_e/g/h rows replaced)

## Remaining manual (by design)

Sketches, proof-only items, inequality/degree branch selection, and transformation descriptions stay `partial`/`skip` with `unsupported_reason` — consistent with first-pass policy.

## Follow-up (out of scope)

- **mp1_n … mp1_z** → Worker 10  
- **mp2** → Workers 11–12  
- Consider removing `fill()` from `_build_remaining_vm_papers.py` or flagging stub batches in CI so stub regressions are caught early.
