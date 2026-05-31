# VM Golden Queue Second-Pass — Worker 01/22

**Scope:** MadAsMaths `c1_a` … `c1_m` (`MadAsMaths papers/c1_X.pdf`)  
**VM PNG roots:** `/Volumes/VM/MadAsMaths papers/c1_X conv_png`, `c1_X_marks conv_png`, `c1_X_solutions conv_png`  
**Batch:** `progress/vm_second_pass/worker_01_patch.json` applied via `tools/apply_second_pass_patches.py`

## Summary

| Metric | Count |
|--------|------:|
| Papers re-read | 13 |
| Existing rows updated | 29 |
| New rows appended | 2 |
| New/extended CAS inputs (worker batch) | 73 verified |
| Skip-only rows left unchanged | graph/transformation rows on c1_a/b/c/d/e/g/h/i/j/k/l/m q3–q7 etc. |

**Validation**
- `python3 tests/check_exact_calculator_input_queue.py` → OK (9233 rows, 12909 inputs)
- Worker-batch strict markers → **73/73 OK** (`khicas_host_runner`, same logic as `run_exact_queue --strict-markers`)

## Gaps found (by paper)

### c1_a
- **q8:** missing y-coordinate of midpoint M `(3+1)/2 = 2`
- **q9(c):** missing numeric check `9×5² = 225` for `Sn = 9n²`
- **q10(c):** missing factorisation `4x³−3x+1 = (x+1)(2x−1)²`

### c1_b
- **q9(a):** missing `a₅ = 5 − 18/5 = 7/5`
- **q10(c):** missing factor `x⁴−3x²+2`
- **q11(a):** missing tangent gradient at P `4×0−1 = −1`

### c1_c
- **q5(a):** missing factor `9x²+18x−7`
- **q8(c):** missing factor `n²−76n+1300`
- **q9(b):** missing x-intercept solve `2x−14=0`

### c1_d
- **q1:** missing final numeric `70√3`
- **q2:** missing index evaluation `(6+2)^(−2/3) = 1/4`

### c1_e–h
- No material gaps beyond existing partial coverage; skip rows confirmed graph/sketch only.

### c1_f
- **q7(c):** missing area `87.75` and x-intercept `Q(−6,0)`

### c1_i (largest gap)
- **q4–q10** were placeholder skips (`unsupported_reason: partial`); filled with surd, coordinate, calculus, recurrence, inequality inputs
- **q11, q12** missing entirely → **2 new rows** added (integration + factor; distance `|DP|=7.5`)

### c1_j
- **q5, q6(a):** filled from skip — inequality factor and gradient `2/3`

### c1_k
- **q4–q6:** filled from skip — max `1/f`, inequality factor, AP sum `830`

### c1_l
- **q4, q5:** filled from skip — AP sum `75`, discriminant `−55`, max `y=9`

### c1_m
- **q5–q7:** filled from skip — simultaneous roots, area inequality factor, AP terms `u₁₂`, `S₄₈`, `a=65`

## Rows added / extended

### Updated (29 ids, extended `inputs[]` or skip→partial/testable)
`madas_iygb_c1_a_q8`, `c1_a_q9`, `c1_a_q10`, `c1_b_q9`, `c1_b_q10`, `c1_b_q11`, `c1_c_q5`, `c1_c_q8`, `c1_c_q9`, `c1_d_q1`, `c1_d_q2`, `c1_f_q7`, `c1_i_q4`–`q10`, `c1_j_q5`, `c1_j_q6`, `c1_k_q4`–`q6`, `c1_l_q4`, `c1_l_q5`, `c1_m_q5`–`q7`

### Appended (new ids)
| id | question | inputs |
|----|----------|-------:|
| `madas_iygb_c1_i_q11_exact_inputs` | q11 a-b | 3 |
| `madas_iygb_c1_i_q12_exact_inputs` | q12 all | 1 |

## Skipped (confirmed skip-only)
Graph transformations / sketches only — not re-opened: e.g. c1_a q5/q7, c1_b q5, c1_c q6, c1_d q3/q8, c1_e q3/q10, c1_g q1, c1_h q9/q10, c1_i q3, c1_j q3, c1_k q7–q10, c1_l q1/q6–q10, c1_m q8–q10.
