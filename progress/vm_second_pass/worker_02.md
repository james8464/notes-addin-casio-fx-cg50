# VM golden queue second-pass — Worker 02/22

**Scope:** MadAsMaths papers `c1_n` … `c1_z` only (`MadAsMaths papers/c1_*.pdf` on `/Volumes/VM`).  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

| Metric | Value |
|--------|------:|
| Papers re-audited | 13 |
| Queue rows (incl. complete markers) | 116 |
| Rows patched | 16 |
| New `inputs[]` entries | 21 |
| Wrong inputs fixed | 1 |
| New queue row ids | 0 |
| Host strict markers (c1_n–c1_z, post-patch) | **159/159 OK** |
| Schema check | **OK** (9249 rows, 12972 inputs) |

Second pass re-read question, marks, and solution PNGs for every paper in range, compared each part to existing `exact_calculator_input_queue.jsonl` rows, and extended rows where mark-scheme steps were CAS-verifiable but missing. Proof-only, diagram-only, and surd/index-law presentation steps remain `skip` / `partial`.

## Validation

- `python3 tests/check_exact_calculator_input_queue.py` — OK (9249 rows, 12972 inputs after patch)
- `python3 progress/vm_second_pass/_apply_worker02.py` — host verify 0 errors before write
- `python3 tests/run_exact_queue.py --strict-markers` — **0 failures** in c1_n–c1_z scope (1 unrelated mp1_a failure elsewhere)

## Fixes (incorrect existing inputs)

| Row id | Issue | Fix |
|--------|-------|-----|
| `madas_iygb_c1_r_q12_exact_inputs` | Input #1 was `diff(3*x^2-6*x+2,x)` — derivative of wrong function | Replaced with `diff(x^3-3*x^2+2*x+9,x)` → `3*x^2 - 6*x + 2` |

## Additions (missed testable steps)

| Paper | Row | Added inputs (summary) |
|-------|-----|------------------------|
| c1_n | q5 | `method=numeric,sqrt(160)` — \|AB\|=4√10 in part (c) |
| c1_n | q7 | `method=numeric,5^2` — intersection (5,25) |
| c1_n | q9 | `method=numeric,2*(0.5)+1/(0.5)` — A(½,3) |
| c1_o | q6 | `method=numeric,20*107` — k=20 trial under S_k≤1200 |
| c1_o | q8 | `method=numeric,2*(2)^3-6*(2)^2+3*2+5` — P(2,3) |
| c1_o | q9 | `solve(-7=-2+C,C)`, `method=numeric,-3*(-1)-5` — C=−5, tangent (−1,−2) |
| c1_p | q9 | gradient `1/2`, `sqrt(20)` for \|AB\| |
| c1_p | q10 | `integrate(sqrt(x)+24/x^2,x)`, `solve(1/3=16/3-6+C,C)` — antiderivative + C=1 |
| c1_p | q11 | `method=numeric,-0.5*(0.5)-0.5` — q=−¾ at p=½ |
| c1_q | q10 | factor roots, y at x=2, parallel tangent x=2, S y-coordinate |
| c1_r | q11 | `method=numeric,144+144` — discriminant always positive |
| c1_t | q8 | `solve(10*k-880=0,k)` — k=88 |
| c1_u | q1 | `method=numeric,2*(2-2)^2+6` — minimum value 6 |
| c1_w | q3 | `solve(3*x^2-10*x+3=0,x)` — stationary points |
| c1_z | q2 | `solve(2*x-12=0,x)` — line x-intercept x=6 |

## Per-paper audit notes

### c1_n (10 questions)
All questions covered. Largest gaps were **q9** (point A y-value) and **q5(c)** (scaled length √160). q4 graph sketches remain skip.

### c1_o (9 questions)
Dense first pass. Added **q6(d)** trial arithmetic, **q8** point evaluation, **q9** constant and tangent y. q2 proof identity skip unchanged.

### c1_p (11 questions)
**q9–q11** extended with coordinate/differentiation numeric steps. q6 translation sketches skip unchanged.

### c1_q (10 questions)
**q10** had four missing CAS steps (roots, stationary y, parallel x, intersection y). q1 surd skip unchanged.

### c1_r (12 questions)
Fixed **q12** wrong diff target. q3 fractional-power integration skip; q7 transformation proof skip.

### c1_s (11 questions)
Existing partial rows adequate after PNG review; q4 recurrence proof and q9 algebraic proof remain skip.

### c1_t (10 questions)
**q8** second method for k=88 now queued. q1 differential identity and q7 index proof skip.

### c1_u – c1_z (5–6 questions each)
Short papers re-confirmed complete. **c1_u q1** minimum value added; **c1_z q2** line intercept added. c1_x q4 coordinate proof skip unchanged.

## Skips unchanged (confirmed)

- Graph/transformation sketches: c1_n q4, c1_p q6, c1_s q11, etc.
- Proof identities: c1_o q2, c1_s q4/q9, c1_t q1/q7, c1_x q4
- Surd/index manual simplification: c1_q q1, c1_t q5, c1_y q1/q2

## Remaining manual-only themes (scope)

- `diff(…+1/x…)` forms rejected by host (generic Pure help) — left manual
- Inequality interval statements after correct critical values
- Geometry diagrams, transformations, and “show that” proofs
- Branch choice for trig/surd presentations

## Worker boundary

Worker **01** owns `c1_a`–`c1_m`; this worker owns **`c1_n`–`c1_z` only**.

## Patch artifact

Reproducible patch: `progress/vm_second_pass/_apply_worker02.py` (applied once in-tree)

## Appended input records (jsonl)

```jsonl
{"row":"madas_iygb_c1_n_q5_exact_inputs","module":"algebra","input":"method=numeric,sqrt(160)","expected_output_markers":["12.6491106407"]}
{"row":"madas_iygb_c1_n_q7_exact_inputs","module":"algebra","input":"method=numeric,5^2","expected_output_markers":["25"]}
{"row":"madas_iygb_c1_n_q9_exact_inputs","module":"algebra","input":"method=numeric,2*(0.5)+1/(0.5)","expected_output_markers":["3"]}
{"row":"madas_iygb_c1_o_q6_exact_inputs","module":"algebra","input":"method=numeric,20*107","expected_output_markers":["2140"]}
{"row":"madas_iygb_c1_o_q8_exact_inputs","module":"algebra","input":"method=numeric,2*(2)^3-6*(2)^2+3*2+5","expected_output_markers":["3"]}
{"row":"madas_iygb_c1_o_q9_exact_inputs","module":"algebra","input":"solve(-7=-2+C,C)","expected_output_markers":["C = [-5]"]}
{"row":"madas_iygb_c1_o_q9_exact_inputs","module":"algebra","input":"method=numeric,-3*(-1)-5","expected_output_markers":["-2"]}
{"row":"madas_iygb_c1_p_q9_exact_inputs","module":"algebra","input":"method=numeric,(7-5)/(4-0)","expected_output_markers":["0.5"]}
{"row":"madas_iygb_c1_p_q9_exact_inputs","module":"algebra","input":"method=numeric,sqrt(20)","expected_output_markers":["4.472135955"]}
{"row":"madas_iygb_c1_p_q10_exact_inputs","module":"integrate","input":"integrate(sqrt(x)+24/x^2,x)","expected_output_markers":["2/3*(x)^(3/2) - 24*x^-1"]}
{"row":"madas_iygb_c1_p_q10_exact_inputs","module":"algebra","input":"solve(1/3=16/3-6+C,C)","expected_output_markers":["C = [1]"]}
{"row":"madas_iygb_c1_p_q11_exact_inputs","module":"algebra","input":"method=numeric,-0.5*(0.5)-0.5","expected_output_markers":["-0.75"]}
{"row":"madas_iygb_c1_q_q10_exact_inputs","module":"algebra","input":"factor(x^2-12*x+35)","expected_output_markers":["(x - 5)*(x - 7)"]}
{"row":"madas_iygb_c1_q_q10_exact_inputs","module":"algebra","input":"method=numeric,0.25*(4-24+35)","expected_output_markers":["3.75"]}
{"row":"madas_iygb_c1_q_q10_exact_inputs","module":"algebra","input":"solve(2*x-12=-8,x)","expected_output_markers":["x = [2]"]}
{"row":"madas_iygb_c1_q_q10_exact_inputs","module":"algebra","input":"method=numeric,0.5*(9/2)-7/2","expected_output_markers":["-1.25"]}
{"row":"madas_iygb_c1_r_q11_exact_inputs","module":"algebra","input":"method=numeric,144+144","expected_output_markers":["288"]}
{"row":"madas_iygb_c1_r_q12_exact_inputs","module":"derive","input":"diff(x^3-3*x^2+2*x+9,x)","expected_output_markers":["3*x^2 - 6*x + 2"],"note":"replacement"}
{"row":"madas_iygb_c1_t_q8_exact_inputs","module":"algebra","input":"solve(10*k-880=0,k)","expected_output_markers":["k = [88]"]}
{"row":"madas_iygb_c1_u_q1_exact_inputs","module":"algebra","input":"method=numeric,2*(2-2)^2+6","expected_output_markers":["6"]}
{"row":"madas_iygb_c1_w_q3_exact_inputs","module":"algebra","input":"solve(3*x^2-10*x+3=0,x)","expected_output_markers":["x = [3, 1/3]"]}
{"row":"madas_iygb_c1_z_q2_exact_inputs","module":"algebra","input":"solve(2*x-12=0,x)","expected_output_markers":["x = [6]"]}
```
