# VM golden queue second-pass — Worker 11/22

**Scope:** MadAsMaths papers `mp2_a` … `mp2_m` (`MadAsMaths papers/mp2_{letter}.pdf`)  
**VM root:** `/Volumes/VM/MadAsMaths papers/`  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

Re-read all question and solution PNG folders for 13 MP2 practice papers (A–M). Compared every question/part against live golden-queue rows. **4 rows extended (+8 inputs, 1 replacement)** after solution re-scan; **0 new row ids**. Strict validation: **12917/12917 markers OK** (full queue after patch).

| Paper | Q pages | Sol pages | Questions (cover) | Queue q-rows | Inputs (before→after) |
|-------|---------|-----------|-------------------|--------------|------------------------|
| mp2_a | 7 | 18 | 13 | 13 | 31 → **32** |
| mp2_b | 6 | 16 | 13 | 13 | 25 → **30** |
| mp2_c | 7 | 13 | 11 | 11 | 22 |
| mp2_d | 6 | 14 | 12 | 12 | 23 |
| mp2_e | 6 | 15 | 12 | 12 | 20 |
| mp2_f | 7 | 14 | 13 | 13 | 24 |
| mp2_g | 6 | 16 | 12 | 12 | 13 |
| mp2_h | 7 | 15 | 11 | 11 | 19 → **20** |
| mp2_i | 7 | 14 | 12 | 12 | 19 |
| mp2_j | 7 | 16 | 12 | 12 | 19 → **20** |
| mp2_k | 7 | 17 | 11 | 11 | 18 |
| mp2_l | 8 | 17 | 12 | 12 | 20 |
| mp2_m | 6 | 17 | 13 | 13 | 21 |
| **Total** | | | **153** | **153** | **274 → 282** |

All 13 sources have `madas_iygb_mp2_{letter}_complete_source_marker` rows. Question counts match cover sheets and queue coverage.

## Method

1. Enumerated `mp2_{a..m} conv_png` and `mp2_{a..m}_solutions conv_png` on VM.
2. Read cover pages for declared question counts; read all question pages and solution pages (spot-deep on partial/skip rows).
3. Cross-checked live queue (`tests/golden/exact_calculator_input_queue.jsonl`) — authoritative over stale batch JSON for j/k/l/m.
4. Host-verified all pre-existing mp2_a–m inputs (274/274 OK before patch).
5. Applied patches via `progress/vm_second_pass/_apply_worker11.py`.
6. Ran `tests/check_exact_calculator_input_queue.py` + `tests/run_exact_queue.py --strict-markers`.

## Gaps found and patched

### mp2_a q12 (parts a–d) — exact integral part (c)

**Issue:** Trapezium estimate (b) and ln 2 deduction (d) captured; substitution exact value **1/16(−1+2 ln 2)** from solution page 16–17 was missing.

**Added input:**
- `method=numeric,(1/16)*(-1+2*ln(2))` → `0.02414339757`

### mp2_b q4 (parts a–e) — vector coordinates

**Issue:** First input `1+4-2+1-2` summed unrelated components (marker `2`) rather than D(3,0,−2). Solution pages 4–5 give explicit E(1,2,12), F(0,2,9).

**Replacement + additions:**
- Replaced input #1: `method=numeric,1+4-2` → D_x = 3
- Added: D_y `1+0-1` → 0, D_z `2+1-5` → −2
- Added: F_x `1-1` → 0, F_y `1+1` → 2, F_z `2+7` → 9
- Kept midpoint FC x-check `(0+4)/2` → 2

### mp2_h q8 (parts a–d) — f(f(x)) = −47

**Issue:** Expansion and x = −2 present; factorisation step on VM solution page 9 not recorded.

**Added input:**
- `factor(x^4-3*x^2-4)` → `(x + 2)*(x - 2)*(x^2 + 1)`

### mp2_j q12 — binomial approximation quadratic

**Issue:** `solve(20*x^2-41*x+2=0,x)` present; factorisation on solution page 16 omitted.

**Added input:**
- `factor(20*x^2-41*x+2)` → `(20*x - 1)*(x - 2)`

## Skip / manual-only rows confirmed (no CAS gap)

| Paper | Question | Reason |
|-------|----------|--------|
| mp2_g | q2 | Proof by contradiction (integer solutions) |
| mp2_i | q4 | Proof by contradiction |
| mp2_j | q2, q6, q10 | Product-rule / related-rates / implicit-diff show-that proofs |
| mp2_k | q5, q11 | Identity + first-principles / third-derivative proofs |
| mp2_l | q11 | Implicit differentiation show-that |
| mp2_m | q2, q3 | Graph transformation description; proof by contradiction |

All other partial rows reviewed: remaining parts are proof, diagram, domain/range sketch, or setup steps correctly marked manual via `unsupported_reason`.

## Batch JSON drift (informational)

`progress/batches/madas_mp2_{j,k,l,m}_rows.json` are **stale** vs live queue (32 row-level input-count mismatches). Queue rows from `_build_mp2_jklm_batches.py` are correct; batch files were partially overwritten by generic `fill()` placeholders. **No queue action required** — recommend re-exporting batch JSON from queue when consolidating workers.

## Validation

```
python3 progress/vm_second_pass/_apply_worker11.py
→ patched 4 rows, +8 inputs, 1 replacements

python3 tests/check_exact_calculator_input_queue.py
→ OK exact calculator queue rows=9233 inputs=12917

python3 tests/run_exact_queue.py --strict-markers
→ OK exact queue run 12917/12917 ok bad=0
```

Pre-patch mp2_a–m spot check: **274/274** host markers OK.

## Files touched

- `tests/golden/exact_calculator_input_queue.jsonl` — 4 rows updated in place
- `progress/vm_second_pass/_apply_worker11.py` — patch script (reproducible)
- `progress/vm_second_pass/worker_11.md` — this report

## Conclusion

MP2 papers A–M are **complete** on VM with **153/153** questions represented. Second pass found **4 partial rows** with missed testable numeric/CAS steps; all patched and strict-validated. No missing questions, no new source markers required.
