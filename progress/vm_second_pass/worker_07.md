# Worker 07/22 — VM golden queue second-pass audit

**Scope:** MadAsMaths papers `c4_a` through `c4_k` (`MadAsMaths papers/c4_X.pdf`)  
**VM root:** `/Volumes/VM`  
**Date:** 2026-05-31

## Executive summary

Re-read all question, marks, and solution PNGs for the six papers present on VM (`c4_a`–`c4_e`, `c4_k`). First-pass queue rows for these papers were **template placeholders** (capped at six generic integration/DE checkpoints) and did **not** match VM solution content or actual question counts.

Second pass appended **31 host-verified rows** (`progress/batches/madas_c4_a_k_second_pass_rows.json`) covering missing questions and VM-aligned `_sp2` corrections. Strict marker run: **all 31 new inputs pass**; three pre-existing failures remain in other workers' scope (`c3_z`, `syn_a`, `syn_m`).

## VM inventory

| Paper | VM folders | Cover Q count | First-pass rows | Gap |
|-------|------------|---------------|-----------------|-----|
| c4_a | questions×6, marks×4, solutions×6 | **10** | q1–q6 (7 rows) | q7–q10 missing; q1–q6 misaligned |
| c4_b | 5 / 5 / 6 | **9** | q1–q5 (6 rows) | q6–q9 missing; q1–q5 misaligned |
| c4_c | 5 / 5 / 7 | **8** | q1–q5 (6 rows) | q6–q8 missing; q1–q5 misaligned |
| c4_d | 6 / 4 / 7 | **8** | q1–q6 (7 rows) | q7–q8 missing; q1–q6 misaligned |
| c4_e | 6 / 4 / 6 | **8** | q1–q6 (7 rows) | q7–q8 missing; q1–q6 misaligned |
| c4_f | **absent** | — | none | no VM PNGs; out of scope |
| c4_g | **absent** | — | none | no VM PNGs |
| c4_h | **absent** | — | none | no VM PNGs |
| c4_i | **absent** | — | none | no VM PNGs |
| c4_j | **absent** | — | none | no VM PNGs |
| c4_k | 6 / 1 / 7 | **9** | q1–q6 (7 rows) | q7–q9 missing; q1–q6 misaligned |

Papers `c4_f`–`c4_j` have no `conv_png` trees under `/Volumes/VM/MadAsMaths papers/` and no entries in `progress/vm_coverage.json`. No queue action taken.

## First-pass defect (all six VM papers)

Existing rows (e.g. `madas_iygb_c4_a_q2_exact_inputs` → `8/3` “area=8/3”) do **not** correspond to VM solutions (e.g. c4_a Q2 is implicit differentiation → tangent `y=4-x`). Same pattern across `c4_b`–`c4_e` and `c4_k`: generic `volume` / `DE` / `parametric` / `ln(n)` placeholders from an automated six-question cap, not page-image review.

**Recommendation for coordinator:** schedule a first-pass row replacement pass (edit-in-place or deprecate stale ids); second pass only **appended** corrective rows with new ids.

## Second-pass additions (31 rows appended)

Batch: `progress/batches/madas_c4_a_k_second_pass_rows.json`  
Tool: `python3 tools/append_queue_rows.py progress/batches/madas_c4_a_k_second_pass_rows.json`

### Missing question coverage

| Paper | New row ids | VM source |
|-------|-------------|-----------|
| c4_a | `q7`, `q8`, `q9`, `q10` | vol `12π`; vectors cos θ; trapezium `≈0.682`; area `3π` |
| c4_b | `q6`, `q7`, `q8`, `q9` | `dr/dt=1/(4π)`; DE constant 200; trap `≈0.096`; vol `π²+2π` |
| c4_c | `q6`, `q6b`, `q7`, `q8` | `r=√(6/π)`; `dr/dt≈0.173`; `A=110`; trap `≈2.415` |
| c4_d | `q7c`, `q8` | `r≈4.0` from `r⁶=4054`; integral `π+2` |
| c4_e | `q7b`, `q7c`, `q8` | trap `≈2.67`; exact `8/3`; vol `π(π+2)/8` |
| c4_k | `q7`, `q8`, `q9` | integral `68`; midpoint `(5,0,2)`; `x=-3,1` |

### VM-aligned `_sp2` corrections (parallel to stale first-pass rows)

| Paper | Row ids | VM checkpoint |
|-------|---------|---------------|
| c4_a | `q4_sp2`, `q5_sp2` | `ln(54)`; `dr/dt=8/π` |
| c4_b | `q1_sp2`, `q4_sp2`, `q5_sp2` | integral `1`; `π²-8`; `a=8` |
| c4_d | `q1_sp2`, `q1b_sp2` | `2ln(3)`; `√3/4` |
| c4_e | `q1_sp2`, `q2_sp2` | `1/9`; `640π` |
| c4_k | `q1_sp2`, `q6_sp2` | `V=π(3+4ln2)`; parametric area `12` |

All inputs verified with `tools/khicas_host_runner` before append.

## Per-paper notes (solution PNG re-scan)

### c4_a (10 questions, 75 marks)
- Q1: binomial expansion — proof/skip (no standalone numeric CAS).
- Q2: implicit diff → gradient `-1`, tangent `y=4-x` (setup manual).
- Q3: separable DE `y³=x-x²+C` (setup manual).
- Q4b: **`ln(54)`** — testable (`q4_sp2`).
- Q5: related rates **`8/π`** (`q5_sp2`).
- Q6: substitution integral (setup manual).
- Q7: volume **`12π`** — testable (new `q7`).
- Q8: lines/intersection/angle — partial cos θ checkpoint (new `q8`).
- Q9: trapezium + exact IBP (new `q9` partial + manual exact part).
- Q10b: cycloid area **`3π`** — testable (new `q10`).

### c4_b (9 questions)
- Q1a integral **`1`**, Q4 **`π²-8`**, Q5b **`a=8,b=-2`**, Q6 **`1/(4π)`**, Q9b vol **`π²+2π`** — all appended.
- Q2 partial fractions + binomial, Q3 implicit stationary points, Q7 DE, Q8 trap — partial/skip where setup dominates.

### c4_c (8 questions)
- Q6–Q8 appended (related rates, DE `A=110`, numerical integration Q8).
- Q1–Q5 first-pass placeholders remain stale; Q3–Q5 on VM are parametric/DE/integration (no `_sp2` added where proof-heavy).

### c4_d (8 questions)
- Q1 integrals and Q8 **`π+2`** appended; Q7c radius appended.
- Q2–Q6 first-pass rows still stale (partial fractions, parametric, etc.).

### c4_e (8 questions)
- Q1 **`1/9`**, Q2 **`640π`**, Q7 trap/exact, Q8 vol appended.
- Q3–Q6 first-pass rows stale.

### c4_k (9 questions)
- Q1 volume **`π(3+4ln2)`** replaces stale `32/3` placeholder via `q1_sp2`.
- Q6 area **`12`**, Q7 integral **`68`**, Q8 midpoint, Q9b roots appended.
- Q2–Q5 first-pass rows stale.

## Validation

```text
python3 tools/append_queue_rows.py progress/batches/madas_c4_a_k_second_pass_rows.json
# appended 31 rows

python3 tests/check_exact_calculator_input_queue.py
# OK exact calculator queue rows=9231 inputs=12868

python3 tests/run_exact_queue.py --strict-markers
# OK exact queue run 12865/12868 ok bad=3
# Failures (pre-existing, outside worker 07 scope):
#   madas_iygb_c3_z_q3_exact_inputs
#   madas_iygb_syn_a_q2_exact_inputs
#   madas_iygb_syn_m_q1_exact_inputs
```

All **31 worker-07 inputs** pass strict markers.

## Outstanding / handoff

1. **Stale first-pass rows** (`madas_iygb_c4_{a..e,k}_q[1-6]_exact_inputs`) should be replaced or marked deprecated — they reference wrong mathematics.
2. **`c4_f`–`c4_j`**: not on VM; worker 08 owns `c4_l`–`c4_u` on VM.
3. **Complete-source markers** for c4_a–k still claim first-pass review; consider updating `review_basis` after coordinator merge.

## Files touched

- `progress/batches/madas_c4_a_k_second_pass_rows.json` (new)
- `tests/golden/exact_calculator_input_queue.jsonl` (+31 rows)
- `progress/vm_second_pass/worker_07.md` (this report)
