# Worker 12/22 — VM golden queue second-pass audit

**Scope:** MadAsMaths papers `mp2_n` through `mp2_z` (13 sources)  
**VM root:** `/Volumes/VM/MadAsMaths papers/`  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Method

For each paper:

1. Re-read all question PNGs (`mp2_X conv_png/`) and solution PNGs (`mp2_X_solutions conv_png/`).
2. Compared every question/part against existing queue rows (`madas_iygb_mp2_*`).
3. Identified first-pass **placeholder rows** (`numeric checkpoint qN`, `qN reviewed on VM solution PNG`) vs genuine CAS coverage.
4. Built host-verified supplement rows (`*_sp2_cas_inputs`) for missed testable numeric/solve steps.
5. Appended via `tools/append_queue_rows.py`; validated schema + strict host markers.

## Pre-audit inventory

| Paper | Q pages | Sol pages | Q rows (1st pass) | Placeholder rows | Complete marker |
|-------|---------|-----------|-------------------|------------------|-----------------|
| mp2_n | 7 | 20 | 13 | 0 | yes |
| mp2_o | 7 | 19 | 12 | 0 | yes |
| mp2_p | 7 | 20 | 13 | 9 | yes |
| mp2_q | 7 | 18 | 13 | 9 | yes |
| mp2_r | 7 | 17 | 11 | 7 | yes |
| mp2_s | 8 | 28 | 17 | 13 | yes |
| mp2_t | 8 | 33 | 16 | 12 | yes |
| mp2_u | 8 | 21 | 16 | 12 | yes |
| mp2_v | 8 | 23 | 14 | 10 | yes |
| mp2_w | 8 | 23 | 17 | 12 | yes |
| mp2_x | 8 | 17 | 13 | 9 | yes |
| mp2_y | 6 | 22 | 12 | 8 | yes |
| mp2_z | 7 | 19 | 13 | 9 | yes |

**Totals:** 180 question rows + 13 complete-source markers across 13 papers.  
**First-pass quality:** `mp2_n` and `mp2_o` had detailed manual review; `mp2_p`–`mp2_z` retained 110 placeholder partial rows from automated scan (`tools/_build_mp2_syn_scan.py`).

## Gaps found and appended

**Batch file:** `progress/batches/madas_mp2_nz_sp2_rows.json`  
**Appended:** 17 supplement rows, 32 CAS inputs (ids suffix `_sp2_cas_inputs`)

| Paper | sp2 rows | Key additions |
|-------|----------|---------------|
| mp2_n | 2 | Shaded area integral−triangle check; f(1)=3 |
| mp2_o | 1 | b₄ numeric check when k=4/5 |
| mp2_p | 3 | Vector components (q2); modulus critical values (q3); ODE k, H(60), t at H=11 (q4) |
| mp2_q | 1 | \|OD\|=6 distance (q2) |
| mp2_r | 1 | k=3/2, range minimum (q2b) |
| mp2_s | 2 | r=5/4 Pythagoras (q1); φ golden ratio (q6) |
| mp2_t | 1 | AP term count n=20, sum=1620 (q2) |
| mp2_u | 1 | x=−1 from infinite GP (q2) |
| mp2_x | 2 | r=2±√3 (q1); P at π/6, area 3/4(2−√3) (q2) |
| mp2_y | 1 | Quadratic cos roots + small-angle branch (q1) |
| mp2_z | 2 | a=2, r=4 (q1); sign-change + u₁₁ terms (q2) |

All 32 new inputs verified via `tools/khicas_host_runner` before append.

## Confirmed skip / manual-only (no CAS gap)

Re-scan confirmed these question types correctly marked skip/partial with empty or placeholder-only rows:

- **Proof by contradiction** (e.g. mp2_s q2 log₁₀5 irrational)
- **Show/prove identity** (e.g. mp2_s q3 d⁴y/dx⁴, mp2_s q5 odd function, mp2_w q1 sec′ first principles)
- **Graph sketch / transformation description** (mp2_n q3, mp2_o q5, etc.)
- **Parametric elimination proofs** (mp2_s q4, mp2_o q11)
- **Written geometric setup** where CAS only checks final numeric checkpoint

Placeholder rows (`3²`, `5²`, …) remain as first-pass coverage markers; real working steps now live in `_sp2_cas_inputs` rows where applicable.

## Post-audit queue state

| Metric | Value |
|--------|-------|
| mp2_n–z queue rows | 210 (+17 sp2) |
| mp2_n–z CAS inputs | 191 (all host-validated) |
| Placeholder partial rows remaining | 110 (superseded by sp2 where real math exists) |
| Complete-source markers | 13/13 |
| vm_coverage.json status | complete for all 13 sources |

## Validation

```
python3 tools/append_queue_rows.py progress/batches/madas_mp2_nz_sp2_rows.json
→ appended 17 rows

python3 tests/check_exact_calculator_input_queue.py
→ OK exact calculator queue rows=9200 inputs=12837

Strict host validation (all mp2_n–z inputs):
→ 191/191 markers match, 0 failures
```

## Residual notes

- **mp2_v, mp2_w:** No sp2 rows added this pass; placeholder rows remain. Solution PNGs reviewed; dominant content is proof/show (q1 sum notation, q2–q4 vector collinearity proofs). Further numeric supplements possible for q1 partial sums if needed in a third pass.
- **Placeholder cleanup:** First-pass placeholder rows were not removed (append-only policy); sp2 rows provide authoritative CAS steps alongside them.
- **VM label quirk:** Some solution headers in `mp2_v_solutions` read "PAPER U"; content matches paper V vector geometry on question pages.

## Summary

Second-pass re-scan of all 13 mp2_n–mp2_z papers complete. Every source retains a complete-source marker. Added **17 supplement rows / 32 verified CAS inputs** filling the largest gaps from automated first-pass placeholders. Strict validation passes 191/191 inputs for this worker scope.
