# Worker 08 — VM golden queue second-pass audit

**Scope:** MadAsMaths papers `c4_l` through `c4_u` (11 papers)  
**VM root:** `/Volumes/VM/MadAsMaths papers/`  
**Worker:** 8/22 (c4 second half; worker 07 owns `c4_a`–`c4_k`)  
**Date:** 2026-05-31

## Summary

Re-read all question, marks, and solution PNG folders for each paper. Compared every question against first-pass queue rows in `tests/golden/exact_calculator_input_queue.jsonl`.

**Critical finding:** First-pass rows for this range were **template placeholders** from `_build_remaining_vm_papers.py` (generic volume/integral/DE/log patterns). They do **not** match the actual VM solution content. Second-pass rows add worksheet-specific CAS checkpoints tied to solution PNGs.

| Metric | Count |
|--------|------:|
| Papers audited | 11 |
| First-pass question rows (pre-audit) | 58 |
| Second-pass rows appended | **36** |
| Strict host-runner verified | 36/36 |
| Queue schema check | OK (9300 rows) |
| Strict queue run (full queue) | 13074/13075 ok (1 pre-existing failure: `madas_iygb_mp1_a_q2_sp2_definite`, worker 09) |

## Per-paper findings

### c4_l (8 questions, 6 solution pages)
- **First-pass gap:** Only q1–q5 recorded; q6–q8 missing. All five rows used wrong template values (e.g. q1 `4/3` volume, q5 `ln(5)`).
- **Second-pass added:** q1a/b integrals, q4b `k=e^{-3/2}`, q5a area 48, q5b `72π²`, q6e kite `20√42`, q7b `ln3+2`, q8c `t=(9/5)ln4`.
- **Manual/skip:** q2 binomial expansion, q3 parametric proof, q6 vector setup.

### c4_m (9 questions, 7 solution pages)
- **First-pass gap:** q6–q9 missing; q1–q5 template placeholders.
- **Second-pass added:** q2b coeff −27, q3 `√5` rate, q5b volume `π(13+6ln3)`, q8b `y=ln6`, q9a area `72π`, q9c gold area ≈106.
- **Manual/skip:** q1 parts, q4 trig integral, q6 DE separation, q7 vectors.

### c4_n (8 questions, 8 solution pages)
- **First-pass gap:** q5–q8 missing; q1–q4 templates.
- **Second-pass added:** q1 integral =1, q3a `dy/dx=-3`, q4 `dy/dt=6/5`, q6d λ=−3, q7 volume checkpoint.
- **Manual/skip:** q2 binomial, q5 implicit vertical-tangent proof, q8 DE.

### c4_o (6 questions, 7 solution pages)
- **First-pass:** All six rows present but template values.
- **Second-pass added:** q1b `4ln(7/2)`, q2b `√6` approx, q3a-ii tangent gradient −3/4.
- **Manual/skip:** q4–q6 (not fully scanned to numeric checkpoint in this pass).

### c4_p (6 questions, 8 solution pages)
- **First-pass:** Template rows q1–q6.
- **Second-pass added:** q1a x² coeff 481, q2b `k=6`.
- **Manual/skip:** q3–q6 remaining parts.

### c4_q (7 questions, 7 solution pages)
- **First-pass:** q1–q7 template rows.
- **Second-pass added:** q1b `−3+8ln2`, q2a `n=4/3`, `a=3/2`, q2b `b=−1/6`.
- **Manual/skip:** q3–q7 (parametric, vectors, DE — setup manual).

### c4_r (6 questions, 7 solution pages)
- **First-pass:** Template rows.
- **Second-pass added:** q2 `dA/dt=81π` at r=13.5, q3 trapezium ≈1.34.
- **Manual/skip:** q1 substitution antiderivative, q4–q6.

### c4_s (8 questions, 13 solution pages)
- **First-pass:** q1–q8 including q8 proof skip (correct).
- **Second-pass added:** q2b area `8/3` confirmation.
- **Manual/skip:** q1 proof (gradient always negative), q8 proof.

### c4_t (6 questions, 15 solution pages)
- **First-pass:** Template rows.
- **Second-pass added:** q1b-i `dy/dt=1/(15π^{1/3})` at t=60, q2 area `16/3`.
- **Manual/skip:** q3–q6 (long solutions; mostly setup/proof).

### c4_u (6 questions, 8 solution pages)
- **First-pass:** Template rows.
- **Second-pass added:** q2 integral `2−ln3`, q3b limiting N→2, q4 implicit gradient at (4,2).
- **Manual/skip:** q1 binomial, q5–q6.

## Artifacts

| File | Purpose |
|------|---------|
| `progress/batches/madas_c4_l_u_second_pass_rows.json` | 36 verified second-pass rows |
| `progress/batches/madas_c4_q_q2_a_gap.json` | Gap fill: q2a `a=3/2` |
| `tools/_build_c4_l_u_second_pass.py` | Build + strict verify script |

## Validation

```text
python3 tools/_build_c4_l_u_second_pass.py          # 36/36 markers OK
python3 tools/append_queue_rows.py progress/batches/madas_c4_l_u_second_pass_rows.json
python3 tools/append_queue_rows.py progress/batches/madas_c4_q_q2_a_gap.json
python3 tests/check_exact_calculator_input_queue.py   # OK rows=9300
python3 tests/run_exact_queue.py --strict-markers     # 13074/13075 ok
```

All worker_08 rows use `review_basis: second-pass VM re-scan worker_08: solution PNGs c4_l–c4_u`.

## Residual gaps (not appended)

- **First-pass template rows** (58 rows) remain in queue with incorrect `expected_final` labels; superseded by `_sp2_*` rows but not removed (coordinator policy: append-only).
- **Proof/sketch questions** across all papers: binomial validity ranges, parametric proofs, vector geometry diagrams, DE separation steps.
- **c4_s, c4_t** later questions (q3+) need deeper per-part review if full CAS coverage is required.
- **c4_n q8, c4_o q4–q6, c4_p q3–q6, c4_q q3–q7, c4_r q4–q6, c4_u q5–q6:** solution PNGs scanned; numeric checkpoints not yet extracted for every part.

## Recommendation

When consolidating after all 22 workers finish, consider marking first-pass template rows for c4_l–c4_u with a `superseded_by` note or replacing placeholder `inputs[]` on the original row ids (requires coordinator merge policy).
