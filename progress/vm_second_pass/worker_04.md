# VM golden queue second-pass — Worker 04/22

**Scope:** MadAsMaths papers `c2_n` … `c2_z` only (`MadAsMaths papers/c2_*.pdf` on `/Volumes/VM`).  
**Repo:** `/Users/james/Developer/CASIO`  
**Date:** 2026-05-31

## Summary

| Metric | Value |
|--------|------:|
| Papers re-audited | 14 |
| Queue question rows (pre) | 104 |
| Rows patched | 14 |
| New `inputs[]` entries | 16 |
| Wrong/duplicate inputs fixed | 2 |
| New queue rows (new ids) | 0 |
| Strict host marker failures (scope) | 0 |

Second pass re-read question, marks, and solution PNGs for every paper in range, compared each part to existing `exact_calculator_input_queue.jsonl` rows, and extended rows where mark-scheme steps were CAS-verifiable but missing. Proof-only or diagram-only parts remain `skip` / `partial` with unchanged `unsupported_reason` text.

## Validation

- `python3 tests/check_exact_calculator_input_queue.py` — OK (9145 rows, 12743 inputs after patch)
- Host strict markers for all `c2_n`–`c2_z` non-skip inputs — 0 failures
- Patch script: `progress/vm_second_pass/_apply_worker04.py` (applied once in-tree)

## Fixes (incorrect existing inputs)

| Row id | Issue | Fix |
|--------|-------|-----|
| `madas_iygb_c2_t_q7_exact_inputs` | Input #2 was `diff(108*x-36*x^2+3*x^3,x)` — **c2_x q9** profit model, not Paper T | Replaced with `diff(-20*x^2+800*x+32500,x)` |
| `madas_iygb_c2_r_q6_exact_inputs` | Inputs #3 and #4 were duplicate `method=numeric,38/3` | Replaced #4 with area at stationary point `method=numeric,1/2*(2*sqrt(2))^2+16*sqrt(2)/(2*sqrt(2))` → 12 |

## Additions (missed testable steps)

| Paper | Row | Added inputs (summary) |
|-------|-----|------------------------|
| c2_n | q1 | `expand((2-x/4)^9)` — part (b) substitution |
| c2_n | q3 | `diff(x^3-3*x^2-24*x-1,x)` |
| c2_n | q4 | `integrate(...)` + definite numeric `64/3` evaluation |
| c2_n | q5 | `diff(x-2*x^4,x)`, `x=1/2` via `(1/8)^(1/3)` |
| c2_n | q7 | `asin(2/3)`, `pi-asin(2/3)` for radian solutions |
| c2_n | q10 | `factor(x^2-10*x+9)` for `r^4` substitution |
| c2_o | q1 | `expand((x+2)*(2*x^2-15*x+50)-112)` — confirms `a=-15,b=50,c=-112` |
| c2_p | q1 | `method=numeric,1.02^12` for calculator comparison |
| c2_q | q3 | minimum value `method=numeric,(3/2)^4-2*(3/2)^3+1` |
| c2_t | q3 | `solve(3*a-4=0,a)` boundary `a=4/3` |
| c2_t | q7 | `solve(800-40*x=0,x)` → `x=20` |
| c2_u | q6 | `method=numeric,16*sqrt(9/4)+27/(9/4)` — 36 thousand bees at `t=9/4` |
| c2_y | q4 | critical temperature `method=numeric,3` |

## Per-paper audit notes

### c2_n
All 10 questions covered. Largest gap was **q1(b)** (only part (a) expansion was queued). q3–q5 still `partial` for inequality sketch / integral setup narrative; numeric and CAS steps now denser.

### c2_o
Nine questions covered. q1(d) algebraic division confirmed via expand; long-division presentation remains manual. q2–q9: existing partial rows adequate; circle completing-square and diagram steps stay manual.

### c2_p
Nine questions covered. q1(c) truncation error reasoning still manual; added exact `1.02^12` check.

### c2_q
Ten questions; **q9** re-confirmed `skip` (graph symmetry only). q6 trig simplification to `cos x=1/2` still manual beyond `60°` check.

### c2_r
Eight questions covered. q6 trapezium ordinate choice still manual; stationary-point area now testable.

### c2_s
Nine questions; **q2, q4, q5** re-confirmed proof-only `skip`. q3 trig equation simplification to `sin²x=1/4` still manual (host does not solve that trig form). q1 area formula setup remains manual; numeric `32(π-2)` already present.

### c2_t
Nine questions; **q4, q6** proof `skip` retained. q7 had cross-paper contamination (fixed). q8 circle geometry still manual beyond coordinate solve.

### c2_u
Eight questions covered. q6(b) increasing interval needs `diff` on `sqrt(t)` + `1/t` — host rejects; `(27/8)^(2/3)` boundary already queued.

### c2_v – c2_z
Re-scanned; no additional host-verifiable gaps beyond existing dense partial/testable rows. Complete-source markers unchanged.

## Skips unchanged (confirmed)

- `madas_iygb_c2_q_q9_exact_inputs` — symmetry
- `madas_iygb_c2_s_q2/q4/q5` — proofs
- `madas_iygb_c2_t_q4/q6` — proofs

## Remaining manual-only themes (scope)

- Geometry diagrams, sector layouts, and “show that” proofs
- Inequality interval statements after correct critical values
- Trig equations where host returns generic help (not factorised/solved)
- Model interpretation (units, rounding, which GP branch)

## Worker boundary

Worker **03** owns `c2_a`–`c2_m`; this worker owns **`c2_n`–`c2_z` only**. No overlap with other second-pass workers.
