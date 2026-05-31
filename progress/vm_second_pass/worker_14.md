# VM golden queue second-pass — Worker 14/22

**Scope:** MadAsMaths papers `syn_n` … `syn_z` (`MadAsMaths papers/syn_*.pdf`)  
**VM root:** `/Volumes/VM`  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Method

1. Enumerated VM `conv_png` folders for all 13 papers; counted question/solution pages.
2. OCR-scanned question PNGs for highest `Question N` label (tesseract) to get true question counts.
3. Compared VM counts vs `progress/batches/madas_syn_*_rows.json` and `tests/golden/exact_calculator_input_queue.jsonl`.
4. Re-read solution PNGs for gaps, phantom rows, and upgradeable placeholder inputs (prime-squared checkpoints from `_build_remaining_vm_papers.py` `fill()`).
5. Verified all syn\_n–z `expected_output_markers` via `tools/khicas_host_runner`.
6. Patched existing rows / appended new rows; re-ran queue checks.

## VM inventory (syn\_n–z)

| Paper | Q pages | Sol pages | OCR max Q | Queue Q rows | Status |
|-------|--------:|----------:|----------:|-------------:|--------|
| syn_n | 9 | 27 | 20 | 20 | OK — detailed first-pass coverage |
| syn_o | 11 | 26 | 18 | 22 | **4 phantom rows** (q19–q22) |
| syn_p | 11 | 29 | 22 | 23 | **1 phantom row** (q23) |
| syn_q | 10 | 30 | 21 | 24 | **3 phantom rows** (q22–q24) |
| syn_r | 10 | 27 | 22 | 22 | OK |
| syn_s | 11 | 25 | 23 | 23 | OK after append (was missing q22–q23) |
| syn_t | 9 | 25 | 21 | 21 | OK after append (was missing q21) |
| syn_u | 10 | 32 | 22 | 24 | **2 phantom rows** (q23–q24) |
| syn_v | 11 | 27 | 22 | 22 | OK |
| syn_w | 11 | 30 | 22 | 23 | **1 phantom row** (q23) |
| syn_x | 1 | 1 | — | 0 (+marker) | Coming Soon placeholder only |
| syn_y | 1 | 1 | — | 0 (+marker) | Coming Soon placeholder only |
| syn_z | 1 | 1 | — | 0 (+marker) | Coming Soon placeholder only |

**Root cause of phantom rows:** `SYN_META` in `tools/_build_remaining_vm_papers.py` over-estimates `nq` (e.g. syn_o `nq=22` vs 18 actual questions). Phantom rows are prime-squared placeholder partials with no matching question on VM.

**syn_x / syn_y / syn_z:** Single “Coming Soon” PNG in both question and solution folders. Existing `*_complete_source_marker` skip rows are adequate; no executable content to queue.

## syn_n (first-pass gold standard)

All 20 questions present with substantive CAS inputs (5 testable, 12 partial, 3 skip). Second-pass re-read of solutions confirmed no missing question rows. Representative checks: q2 `solve(n^2-6*n-112=0,n)`, q16 gradient `800/9`, q18 integral `sqrt(3)/4` — all markers pass.

## Gaps found and fixed

### Appended (3 new rows)

| id | Paper | Notes |
|----|-------|-------|
| `madas_iygb_syn_s_q22_exact_inputs` | syn_s | skip — vector ratio \|AP\|:\|PN\|=6:5 |
| `madas_iygb_syn_s_q23_exact_inputs` | syn_s | skip — Sophie Germain factorization |
| `madas_iygb_syn_t_q21_exact_inputs` | syn_t | partial — `k=5` numeric checkpoint |

Source: `progress/vm_second_pass/worker_14_gaps.json` → `tools/append_queue_rows.py`.

### Patched existing rows (4)

| id | Change |
|----|--------|
| `madas_iygb_syn_o_q2_exact_inputs` | Replaced `3^2` placeholder with `f(ln5)=2`, `f'(ln5)=5/4`, `g'(2)=4/5` |
| `madas_iygb_syn_o_q3_exact_inputs` | Remainder-theorem quadratics → **testable** (`k=3` from both parts) |
| `madas_iygb_syn_o_q4_exact_inputs` | Radius `5`, y-intercept `-16/3` |
| `madas_iygb_syn_s_q21_exact_inputs` | AP working: `d=6`, `48` terms → **testable** |

All patched markers verified on host runner.

## Remaining debt (not fixed this pass)

1. **Phantom rows** (11 total across syn_o, syn_p, syn_q, syn_u, syn_w): rows reference non-existent questions; should be removed or marked obsolete when queue cleanup is allowed.
2. **Placeholder partials** (~150 rows in syn_o–syn_w): still use `method=numeric,{prime}^2` from automated `fill()`. Only syn_o q1 and the rows above were upgraded to solution-derived inputs this pass. Full upgrade needs per-question solution PNG review (same depth as syn_n).
3. **syn_o q1** already testable but could add simultaneous-equation checkpoint for `D(6,-1)` (left as-is; existing markers sufficient).

## Validation

| Check | Result |
|-------|--------|
| `tests/check_exact_calculator_input_queue.py` | OK — 9258 rows, 13025 inputs |
| syn\_n–z host marker scan (all non-skip inputs) | **203/203 OK** |
| `tests/run_exact_queue.py --strict-markers` (full queue) | 13024/13025 OK — sole failure is unrelated `madas_iygb_mp1_a_q2_sp2_definite` (worker 09 scope) |
| `tests/audit_vm_coverage.py` | 514/514 sources complete |

## Verdict counts (syn\_n–z queue rows)

- partial: 175  
- testable: 8  
- skip: 54  
- second-pass touched: 7 rows (4 patches + 3 appends)

## Files touched

- `tests/golden/exact_calculator_input_queue.jsonl` — 4 in-place patches + 3 appends  
- `progress/vm_second_pass/worker_14_gaps.json` — append batch  
- `progress/vm_second_pass/worker_14.md` — this report
