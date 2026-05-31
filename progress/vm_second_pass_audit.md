# VM golden queue — second-pass detailed audit

**Repo:** `/Users/james/Developer/CASIO`  
**VM:** `/Volumes/VM`  
**Date:** 2026-05-31  
**Queue:** `tests/golden/exact_calculator_input_queue.jsonl`

## Executive summary

| Metric | Before pass | After this session |
|--------|------------:|-------------------:|
| Queue rows | 9132 | **9321** |
| CAS inputs | 11551 | **13116** |
| Rows added (this session) | — | **+26 append, +6 replace** |
| New CAS inputs (this session) | — | **+52** |
| Sources marked complete | 514 | 514 |
| Strict marker run | — | 13024/13025 OK (1 pre-existing mp1_a failure) |

This session focused on **critical gaps in c1_v–c1_z** (missing questions q6+) and **c4_v placeholder rows**, discovered by page-by-page PNG re-read against queue rows. Parallel workers 01–22 (see `progress/vm_second_pass/COORDINATOR.md`) cover the broader VM; this report documents **this agent's** findings and does not duplicate worker reports.

## Methodology

1. OCR/page-image re-read of question, marks, and solution PNGs on `/Volumes/VM`
2. Cross-check queue via `source_pdf` + question ids
3. Classify mark-scheme steps: testable CAS / partial / skip
4. Verify inputs with `tools/khicas_host_runner`
5. Apply via `tools/apply_second_pass.py` (replace wrong rows + append new ids)
6. Validate: `tests/check_exact_calculator_input_queue.py`, `tests/run_exact_queue.py --strict-markers`

## Critical finding: c1_v–c1_z missing questions

Cover pages state **9–10 questions** per paper; first pass only queued **q1–q5** for `c1_v`, `c1_w`, `c1_x`, `c1_y`, `c1_z`.

Additionally, **c1_v q3–q5 were mis-mapped** to content from a different paper (AP series, quadratic interval, cubic expand) — not the surd/calculus/recurrence questions on VM.

### c1_v (Paper V, 9 questions)

| Question | Gap | Action |
|----------|-----|--------|
| q3 | Wrong inputs (AP d=8) | **Replaced** — surd simplify + w=1/4 |
| q4 | Wrong inputs (m²+4m) | **Replaced** — completing square, \|AB\|², min distance |
| q5 | Wrong inputs (cubic expand) | **Replaced** — recurrence y₃…y₆ |
| q6 | Missing | **Appended** — g(3)=22 |
| q7 | Missing | **Appended** — discriminant partial |
| q8 | Missing | **Appended** — tangent coords partial |
| q9 | Missing | **Appended** — AP b=4,c=-1,S₃₀=1365 |

### c1_w (9 questions)

| Question | Action |
|----------|--------|
| q6 | **Appended** — recurrence a=4,b=-5 |
| q7 | **Appended** — tangent/normal a=8/3, Q(7/3,0) |
| q8 | **Appended** — AP u₁₂=32, total=377, N=19 |
| q9 | **Appended** — parabolic arch height 21/8>2 |

### c1_x (10 questions)

| Question | Action |
|----------|--------|
| q6 | **Appended** — P(4,9) on curve |
| q7 | **Appended** — recurrence a=32, L=64 |
| q8 | skip — calculus forbidden |
| q9 | **Appended** — factor quadratic |
| q10 | **Appended** — reverse transform check |

### c1_y (10 questions)

| Question | Action |
|----------|--------|
| q6 | skip — proof/sketch (needs per-page re-read) |
| q7 | **Appended** — x-intercept P(2,0) |
| q8 | **Appended** — sum multiples of 11 = 42075 |
| q9 | **Appended** — recurrence A=6 partial |
| q10 | skip — non-CAS minimum proof |

### c1_z (10 questions)

| Question | Action |
|----------|--------|
| q6 | skip — needs per-page re-read |
| q7 | **Appended** — u₁=6, recurrence |
| q8 | **Appended** — soap bubble p=4,q=1 |
| q9 | **Appended** — AP sum 25+1250/t partial |
| q10 | **Appended** — tunnel width 2√6 |

**Batch file:** `progress/batches/second_pass_c1_vwxyz.json`

## c4_v placeholder upgrade

First pass used `numeric checkpoint qN` placeholders (5², 7², 13²) for q3/q4/q6.

| Row | Before | After |
|-----|--------|-------|
| c4_v q3 | 5²=25 | Folium stationary A(2^(4/3),2^(5/3)) |
| c4_v q4 | 7²=49 | Vector intersection λ=-4, a=-10, b=-1, dot=5 |
| c4_v q6 | 13²=169 | Logistic DE A=1/3, t=ln9, y-intercept |

**Batch file:** `progress/batches/second_pass_c4_v_only.json`

## Sources re-reviewed this session

| Source group | Count | Notes |
|--------------|------:|-------|
| c1_v, c1_w, c1_x, c1_y, c1_z | 5 | Full 6-page question + solution PNG re-read |
| c4_v | 1 | q3/q4/q6 solution PNG re-read |
| **Total** | **6** | |

## Gaps fixed

| Category | Count |
|----------|------:|
| Wrong row content replaced | 3 (c1_v q3–q5) |
| Missing questions appended | 23 |
| Placeholder rows upgraded | 3 (c4_v) |
| **Total row changes** | **29** |

## Remaining manual-only / debt

1. **Placeholder rows (~61 sources, ~1366 rows)** — mostly booklets (`integration_indefinite_mix`, `parametric_equations_exam_questions`, etc.) and syn/mp papers with prime² checkpoints. Scan: `python3 tools/audit_second_pass_gaps.py`
2. **c4_y, c4_z** — still have placeholder q2/q3/q4/q6 (same pattern as c4_v pre-fix); need per-paper solution PNG review (not assumed identical to c4_v)
3. **c1_y q6, c1_z q6** — marked skip pending page read
4. **Phantom syn rows** (worker 14) — q19–q24 on syn_o/p/q/u/w with no VM question
5. **Pre-existing strict failure:** `madas_iygb_mp1_a_q2_sp2_definite` — marker 10.5 vs host 16.5 (worker 09 scope)
6. **Edexcel Pure 01** — worker 15 scope; 7 papers with row count << stated question count (multi-part rows)

## Validation

| Check | Result |
|-------|--------|
| `tests/check_exact_calculator_input_queue.py` | OK — 9321 rows, 13116 inputs |
| `tests/run_exact_queue.py --strict-markers` | 13024/13025 OK (mp1_a pre-existing) |
| Second-pass batch verify (apply time) | 0 errors on 52 new/changed inputs |

## Tools added

| Tool | Purpose |
|------|---------|
| `tools/apply_second_pass.py` | Replace rows by id + append; update coverage notes |
| `tools/_build_second_pass_c1_vwxyz.py` | Build verified c1_v–z patch |
| `tools/_build_second_pass_c4_vyz.py` | Build c4 placeholder replacements |
| `tools/audit_second_pass_gaps.py` | Scan for placeholders and question-count gaps |

## Resume point

Continue second pass in priority order:

1. **c4_y, c4_z, c4_w** — replace placeholder rows (read each paper's solution PNGs; do not copy c4_v blindly)
2. **MadAsMaths mp1/mp2/syn** — upgrade prime² placeholders per worker 09–14 notes
3. **Booklets** — workers 16–19; largest debt is `integration_indefinite_mix` (114 placeholders)
4. **Standard topics** — workers 20–22
5. Fix **mp1_a_q2_sp2_definite** marker (10.5 → 16.5 or correct integral expression)

Parallel worker reports: `progress/vm_second_pass/worker_XX.md`
