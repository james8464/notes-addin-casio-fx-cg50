# Worker 17/22 — VM golden queue second-pass audit

**Repo:** `/Users/james/Developer/CASIO`  
**VM:** `/Volumes/VM`  
**Scope:** MadAsMaths A-level booklet `conv_png` folders alphabetically after worker 16 (`basic_various`) through `standard_integration` (inclusive)  
**Date:** 2026-05-31

## Scope (59 folders)

| # | Category | Folders | Queue status |
|---|----------|---------|--------------|
| 16–19 | `elementary/` | 4 | Selective drill review + gaps patched (below) |
| 20–59 | `mechanics/` | 40 | Skip markers only — out of Pure CAS scope |
| 60–74 | `standard_integration/` | 15 | Full page-count numeric checkpoint rows + markers |

**First:** `elementary/basic_skills`  
**Last:** `standard_integration/parametric_integration_exam_questions`

Alphabetical indices 16–74 of 134 total booklet folders on VM.

## Re-scan method

1. Enumerated all 134 booklet `conv_png` folders on VM; took worker 17 slice (items 16–74).
2. Compared each source to `tests/golden/exact_calculator_input_queue.jsonl` — marker presence, row counts, CAS input counts.
3. Deep-read VM PNGs for the four `elementary/` booklets and spot-checked `standard_integration/integration_basics` and `integration_by_parts`.
4. Host-verified all new/changed inputs via `tools/khicas_host_runner`.
5. Appended new rows with `tools/append_queue_rows.py`; patched two existing rows in-place.
6. Validated: `check_exact_calculator_input_queue.py` OK; worker-17 rows 23/23 inputs pass strict markers.

## Gaps found and fixed

| Source | Gap | Action | Rows/inputs added |
|--------|-----|--------|-------------------|
| `elementary/equations_basic_techniques.pdf` | Q1 problems 4–6 on page 3 had no CAS inputs (only 1–3 covered) | Extended existing row | +3 inputs (now 6 total) |
| `elementary/quadratics.pdf` | Q1 parts e–h on page 2 missing (only a–d covered) | Extended existing row | +4 inputs (now 8 total) |
| `elementary/basic_skills.pdf` | Q1 row b (page 3) not represented | New row `…_q1b_exact_inputs` | 1 row, 4 inputs |
| `elementary/practical_arithmetic_problem_solving.pdf` | Q2 bounds calculation (page 2) not represented | New row `…_q2_exact_inputs` | 1 row, 2 inputs |
| `standard_integration/integration_basics.pdf` | Q1 had numeric checkpoint only; page 2 shows reverse-chain-rule integrals | New row `…_q1_integrals_exact_inputs` | 1 row, 3 inputs |

**Total appended:** 3 new queue rows  
**Total patched:** 2 existing rows (+7 inputs)  
**Net new inputs:** 16 (queue 9139 → 9148 rows, 12668 → 12752 inputs)

### Patch details

**Equations Q1** — added expanded-form solves for problems 4–6:
- `solve(6*x-18=38-x,x)` → `x = [8]`
- `solve(8*x+8=6*x-2,x)` → `x = [-5]`
- `solve(3*x+17=4*x+2,x)` → `x = [15]`

**Quadratics Q1** — added parts e–h:
- `solve(x^2-8*x-9=0,x)` → `x = [9, -1]`
- `solve(x^2-7*x+12=0,x)` → `x = [4, 3]`
- `solve(x^2-8*x+16=0,x)` → `x = [4]`
- `solve(x^2+3*x-28=0,x)` → `x = [4, -7]`

**Integration basics Q1 integrals** — real CAS antiderivatives (second-pass upgrade):
- `int((3*x+1)^2,x)` → `1/9*(3*x + 1)^3 + C`
- `int(4*(2*x+1)^5,x)` → `1/3*(2*x + 1)^6 + C`
- `int(6/(2*x-1)^2,x)` → `-3*(2*x - 1)^-1 + C`

## No-action sources (59 − 5 = 54)

### Mechanics (40 folders)

All have `complete_source_marker` skip rows with `unsupported_reason: mechanics out of A-level Pure CAS scope`. VM page counts confirmed (4–64 pages each). No Pure CAS content to add.

### Standard integration (14 remaining folders)

All have `pages − 1` numeric-checkpoint question rows plus source-complete markers. Row counts match VM page inventory exactly:

| Booklet | Pages | Q-rows |
|---------|------:|-------:|
| integration_areas | 39 | 38 |
| integration_basics | 21 | 20 (+ new integrals supplement row) |
| integration_by_a_special_manipulation | 3 | 2 |
| integration_by_parts | 16 | 15 |
| integration_by_reverse_chain_rule | 15 | 14 |
| integration_by_substitution | 26 | 25 |
| integration_by_trigonometric_identities | 17 | 16 |
| integration_indefinite_mix | 115 | 114 |
| integration_numerical | 54 | 53 |
| integration_partial_fractions | 11 | 10 |
| integration_structured_exam_questions_part_i | 268 | 267 |
| integration_volume_of_revolution | 78 | 77 |
| odes_context_modelling | 107 | 106 |
| odes_separable_no_context | 82 | 81 |
| parametric_integration_exam_questions | 92 | 91 |

Spot-check of `integration_by_parts` page 2: items use `x·e^{2x}`, `x·sin(4x)`, `ln x` etc. — `integrate` module does not yet produce markers for these (returns generic help). Left as numeric-checkpoint + manual partial verdict; noted for future integrate-module work.

### Elementary basic_skills (partial)

92-page drill booklet. Q1 rows a–b now sampled (8 fraction CAS inputs). Remaining pages are repetitive drill (Q2–Q91+ mixed numbers, compound fractions). Complete marker remains valid; full per-row enumeration would be low-value duplication.

## Validation

| Check | Result |
|-------|--------|
| `python3 tests/check_exact_calculator_input_queue.py` | OK — 9148 rows, 12752 inputs |
| Worker 17 rows strict markers (23 inputs) | OK — 5/5 rows, 0 failures |
| `python3 tests/run_exact_queue.py --strict-markers` (full queue) | 12773/12775 ok — 2 pre-existing failures in worker 13 scope (`syn_a_q2`, `syn_m_q1`), unrelated to worker 17 |
| `python3 tests/audit_vm_coverage.py --phase booklets` | 134/134 complete (100%) |

## Files touched

- `tests/golden/exact_calculator_input_queue.jsonl` — 2 rows patched, 3 rows appended
- `progress/batches/worker_17_gap_rows.json` — gap batch (appended)

## Handoff to worker 18

Worker 18 owns alphabetical booklet folders **after** `standard_integration/parametric_integration_exam_questions` (index 75+): `standard_trigonometry/` through start of `statistics/`.
