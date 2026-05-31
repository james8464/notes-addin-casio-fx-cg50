# VM golden queue second-pass — Worker 15/22

**Scope:** Edexcel Pure Paper 01 question folders + Pure-related support materials  
**VM roots:** `/Volumes/VM/Edexcel A Level Maths past papers`, `/Volumes/VM/Edexcel A Level Maths support materials`  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

| Metric | Value |
|--------|------:|
| Question papers re-read | 7 |
| Mark-scheme folders reviewed | 7 |
| Support-material sources (Pure only) | 3 |
| Queue rows in scope | 122 |
| Rows replaced this pass | 25 |
| New rows appended | 0 |
| CAS inputs before pass | 186 |
| CAS inputs after pass | 238 |
| Net new verified inputs | +52 |
| Strict marker check (worker scope) | **238/238 ok** |

**Validation**
- `python3 tests/check_exact_calculator_input_queue.py` → OK (9249 rows, 13003 inputs)
- Worker-scope strict markers via `khicas_host_runner` → **238/238 ok**
- Patch applied via `python3 tools/apply_second_pass.py progress/batches/edexcel_worker15_second_pass_patch.json`

## Method

1. Inventoried all VM `9ma0-01` / `9MA0_01` / `EAM319ma0-01` question + RMS folders and Pure SAM support folders.
2. Loaded existing golden-queue rows; skipped stats/mech/out-of-scope sources already marked in `edexcel_skip_all.json`.
3. Strict-validated all 186 pre-pass inputs (all passed).
4. Re-read question PNGs and mark-scheme PNGs for first-pass sparse papers (`2018`, `20220608`, `20230607`, `20240605`, `EAM319`).
5. Cross-checked against already-rich parallel papers (`9MA0_01_que_20201008`, `9MA0_01_que_20211007`) where Q10–Q15 content matches (substitution integral, intersecting circles, periodic sequence, balloon model, implicit curve).
6. Built verified patch (`progress/batches/edexcel_worker15_second_pass_patch.json`); applied in-place row replacements only (same ids).

## VM page inventory

| Source | Q PNGs | MS PNGs | Queue q-rows | Inputs |
|--------|-------:|--------:|-------------:|-------:|
| `9ma0-01-que-2018` | 44 | 30 (`9ma0-01-ms-2018`) | 14 | 28 |
| `9ma0-01-que-20220608` | 48 | 30 (`9ma0-01-rms-20220818`) | 16 | 33 |
| `9ma0-01-que-20230607` | 44 | 30 (`9ma0-01-rms-20230817`) | 15 | 29 |
| `9ma0-01-que-20240605` | 44 | 30 (`9ma0-01-rms-20240815`) | 15 | 26 |
| `9MA0_01_que_20201008` | 52 | *(no local RMS folder)* | 15 | 42 |
| `9MA0_01_que_20211007` | 52 | 27 (`9MA0_01_rms_20211216`) | 15 | 51 |
| `EAM319ma0-01-que-20230607` | 44 | 30 (`EAM329ma0-01-rms-20230817`) | 15 | 29 |

**Already adequate (no patch):** `9MA0_01_que_20201008`, `9MA0_01_que_20211007` — detailed multi-input rows from prior phase-4 scan.

**Mark-scheme-only skip markers (unchanged):** `9MA0_01_rms_20190815`, `9MA0_01_rms_20211216`, `9ma0-01-ms-2018`, all `9ma0-01-rms-202*` RMS folders, `EAM329ma0-01-rms-20230817`.

## Support materials (Pure)

| Source | Pages | Verdict |
|--------|------:|---------|
| `Sample_Assessment_Materials_Model_Answers_Pure.pdf` | 87 | skip — multi-paper reference; not a single exam row |
| `a-level-l3-mathematics-sams.pdf` | 142 | skip — full SAM document spans Pure/Stats/Mech |
| `A_level_Mathematics_Sample_Assessment_Materials_Exemplification.pdf` | — | skip — examiner guidance, not executable steps |

**Skipped by design (not in worker scope):** `Sample_Assessment_Materials_Model_Answers_Statistics.pdf`, `Sample_Assessment_Materials_Model_Answers_Mechanics.pdf` (already queued as stats/mech skip rows).

## Rows patched (25)

### `9ma0-01-que-20230607` + `EAM319ma0-01-que-20230607` (mirror)
| Row | Change |
|-----|--------|
| `…_q2_cubic_factor` | +`factor(4*x^3+5*x^2-10*x)` for part (b)(ii) |
| `…_q10_substitution_integral` | 1→4 inputs (limits, `apart`, `exp(2*ln(7/6))`) |
| `…_q11_circles` | 1→4 inputs (intersection solve, two `acos` angles, perimeter numeric) |
| `…_q13_sequence` | 1→3 inputs (recurrence solve, `k^2+k-2`, sum `-80`) |
| `…_q14_related_rates` | 1→3 inputs (`k=11200/3`, `t=40/7`, empty-time solve) |
| `…_q15_implicit_curve` | 1→3 inputs (first derivative, second derivative, inflection solve) |

### `9ma0-01-que-20240605`
Same Q10–Q14 enrichment as above (Q15 left as proof/discriminant row — content differs from Oct-2020 Q15 implicit curve).

### `9ma0-01-que-2018`
| Row | Change |
|-----|--------|
| `…_q4_ln_iteration` | +`method=numeric,2*ln(8-3.5)-3.5` (midpoint sign check) |
| `…_q7_parametric_integrals` | +`integrate(2/(3*x-1),x)` indefinite step |

### `9ma0-01-que-20220608`
Enriched from verified Oct-2021 parallel where question content matches:
| Row | Inputs |
|-----|-------:|
| `…_q9_partial_fractions` | 1→4 |
| `…_q10_trig_identity` | 1→2 |
| `…_q11_trapezium` | 1→3 |
| `…_q12_quadratic_model` | 1→5 |
| `…_q13_parametric_proof` | 1→3 |
| `…_q14_derivative_sqrt` | 1→2 |
| `…_q15_proof_algebra` | 1→2 |

## Gaps remaining (correctly partial/skip)

- **Q1–Q9 on 20230607/20240605:** different question mix from Oct-2020 paper; first-pass rows retained. Further inputs need per-question MS review (e.g. power-rule integral antiderivative route not yet supported by CAS — returns General Pure method).
- **Q1–Q8 on 20220608:** sparse single-input rows; Oct-2021 paper is a different exam so question numbers do not align for bulk copy.
- **2018 Q1–Q3, Q5–Q6, Q8–Q14:** proof wording, diagram setup, validity intervals, root-selection — contextual/manual.
- **Support SAMs:** reference documents; individual SAM Pure questions not broken into queue rows (would duplicate future SAM paper work).

## Files touched

- `tests/golden/exact_calculator_input_queue.jsonl` — 25 rows replaced in-place
- `progress/batches/edexcel_worker15_second_pass_patch.json` — audit patch (new)
- `progress/vm_coverage.json` — `last_batch` notes updated for patched sources
