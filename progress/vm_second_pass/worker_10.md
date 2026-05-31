# Worker 10 — VM golden queue second-pass audit

**Worker:** 10/22  
**Scope:** MadAsMaths papers `mp1_n` through `mp1_z` (14 papers)  
**VM root:** `/Volumes/VM/MadAsMaths papers/`  
**Date:** 2026-05-31  
**Review basis tag:** `second-pass VM re-scan worker_10: solution PNGs mp1_n–mp1_z`

## Method

1. **Cover-page question-count check** — read VM question PNG page 1 for each paper; compare stated question count vs batch row coverage.
2. **Solution PNG re-scan** — read worked-solution PNGs on VM for papers with count mismatches; extract testable CAS/numeric checkpoints.
3. **Strict validate** — run every non-skip input in batch files through `tools/khicas_host_runner`; require `rc=0` and all `expected_output_markers` present in output.
4. **Queue sync** — ensure `progress/batches/madas_mp1_*_rows.json` IDs match `tests/golden/exact_calculator_input_queue.jsonl` exactly; append/replace gaps.

## VM inventory (question / solution PNG counts)

| Paper | Q pages | Solution pages | Cover questions | Batch q-rows (before → after) |
|-------|---------|----------------|-----------------|-------------------------------|
| mp1_n | 8 | 16 | 15 | 15 → 15 |
| mp1_o | 7 | 19 | 12 | 12 → 12 |
| mp1_p | 8 | 16 | 11 | 11 → 11 |
| mp1_q | 5 | 14 | 11 | 11 → 11 |
| mp1_r | 7 | 16 | 11 | 11 → 11 |
| mp1_s | 7 | 22 | **13** | **10 → 13** |
| mp1_t | 5 | 24 | **12** | **9 → 12** |
| mp1_u | 6 | 18 | 12 | 12 → 12 |
| mp1_v | 6 | 16 | 13 | 13 → 13 |
| mp1_w | 5 | 14 | 12 | 12 → 12 |
| mp1_x | 7 | 17 | 12 | 12 → 12 |
| mp1_y | 7 | 15 | 10 | 10 → 10 |
| mp1_z | 6 | 17 | 12 | 12 → 12 |

## Gaps found and appended

### mp1_t — missing q10–q12 (first pass stopped at q9)

Cover states 12 questions; batch had 9 question rows and a false-complete marker claiming 9 questions.

| ID | Verdict | Key CAS inputs |
|----|---------|----------------|
| `madas_iygb_mp1_t_q10_exact_inputs` | testable | `solve(2*(x-2)^2=0,x)` → x=2; `solve(2*(x-2)^2=2,x)` → x=1,3 |
| `madas_iygb_mp1_t_q11_exact_inputs` | partial | R(3,2); `solve(b^2-b-20=0,b)`; areas 17 and 8.5 |
| `madas_iygb_mp1_t_q12_exact_inputs` | partial | t=2 repeated root; D=-48; tangent point (4,11) |

Source marker updated: 5 question pages, 24 solution pages, **12 questions**.

### mp1_s — missing q11–q13 (first pass stopped at q10)

Cover states 13 questions; batch had 10 question rows and a false-complete marker claiming 10 questions.

| ID | Verdict | Key CAS inputs |
|----|---------|----------------|
| `madas_iygb_mp1_s_q11_exact_inputs` | skip | triangular-number proof only |
| `madas_iygb_mp1_s_q12_exact_inputs` | partial | x=±(2−ln3)/(1−ln2); A=e²/3 branch |
| `madas_iygb_mp1_s_q13_exact_inputs` | testable | minimum translated area 9/4 at t=3/2 |

Source marker updated: 7 question pages, 22 solution pages, **13 questions**.

## Strict validation (post-patch)

| Metric | Value |
|--------|-------|
| Papers audited | 14 |
| Total queue/batch rows | **169** (was 163) |
| Golden ↔ batch ID parity | exact match |
| Bad inputs (all papers) | **0** |
| New rows appended | **6** (3 mp1_t + 3 mp1_s) |
| Markers corrected | 2 (mp1_s, mp1_t) |

Verdict breakdown (all 14 papers):

- testable: 56  
- partial: 76  
- skip: 23  
- source-complete markers: 14  

## Other papers (no structural gaps)

Papers `mp1_n`, `mp1_o`, `mp1_p`, `mp1_q`, `mp1_r`, `mp1_u`, `mp1_v`, `mp1_w`, `mp1_x`, `mp1_y`, `mp1_z` — cover question counts match batch coverage; all existing inputs pass host-runner validation. No supplemental `_sp2_` rows were required beyond the two truncated papers above.

## Files changed

- `progress/batches/madas_mp1_s_rows.json` — +3 question rows, marker fix
- `progress/batches/madas_mp1_t_rows.json` — +3 question rows, marker fix
- `tests/golden/exact_calculator_input_queue.jsonl` — resynced mp1_s/mp1_t (removed 21 stale rows, wrote 27 current rows)
- `progress/vm_coverage.json` — mp1_s/mp1_t `last_batch: worker_10_second_pass`

## Residual manual-only content (by design)

Across the scope, skipped or partial rows correctly flag: graph sketches, proof-only questions, degree-range branch selection, vector/diagram setup, and show-that derivations without executable CAS checkpoints. No further testable numeric gaps identified on solution PNG re-scan for the remaining 12 papers.
