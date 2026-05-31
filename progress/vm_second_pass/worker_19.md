# VM golden queue second-pass — Worker 19/22

**Date:** 2026-05-31  
**Repo:** `/Users/james/Developer/CASIO`  
**VM:** `/Volumes/VM`  
**Scope:** Remaining MadAsMaths A-level booklet `conv_png` folders after worker 18 (alphabetical indices 103–134 of 134 total booklet folders)

## Boundary

| Worker | Alphabetical range (1-based) | First folder | Last folder |
|--------|------------------------------|--------------|-------------|
| 18 | 69–102 | `standard_trigonometry/trigonometric_equations` | `standard_various/inequalities` |
| **19** | **103–134** | `standard_various/modulus_function_exam_questions` | `statistics/t_distribution` |

Worker 19 covers **32 folders** (10 pure `standard_various` + 22 `statistics`).

## Sources audited

### Pure booklets (10)

| # | Category / stem | VM PNGs | Queue rows | Q rows | Marker |
|---|-----------------|---------|------------|--------|--------|
| 103 | standard_various/modulus_function_exam_questions | 95 | 95 | 94 | 1 |
| 104 | standard_various/modulus_function_practice | 18 | 18 | 17 | 1 |
| 105 | standard_various/numerical_solutions_of_equations | 63 | 63 | 62 | 1 |
| 106 | standard_various/parametric_equations | 29 | 29 | 28 | 1 |
| 107 | standard_various/parametric_equations_exam_questions | 159 | 159 | 158 | 1 |
| 108 | standard_various/polynomials | 28 | 28 | 27 | 1 |
| 109 | standard_various/related_rates_of_change | 54 | 54 | 53 | 1 |
| 110 | standard_various/transformations_of_graphs_exam_questions | 94 | 94 | 93 | 1 |
| 111 | standard_various/transformations_of_graphs_practice | 7 | 7 | 6 | 1 |
| 112 | standard_various/transformations_of_graphs_practice_student_version | 8 | 8 | 7 | 1 |

### Statistics booklets (22) — skip scope

All 22 statistics folders have exactly one `complete_source_marker` skip row each. Spot-checked `binomial_distribution`, `probability`, `hypothesis_testing_normal_distribution` PNGs — content is stats/probability (distributions, hypothesis tests, Venn diagrams, etc.); no pure-CAS content requiring queue rows.

| # | Stem | VM PNGs |
|---|------|---------|
| 113 | binomial_distribution | 48 |
| 114 | combinations_and_permutations | 28 |
| 115 | contingency_tables | 15 |
| 116 | correlation_and_regression | 50 |
| 117 | data_analysis | 44 |
| 118 | data_analysis_exam_questions | 36 |
| 119 | discrete_random_variables | 42 |
| 120 | distributional_approximations | 28 |
| 121 | geometric_distribution | 18 |
| 122 | goodness_of_fit | 25 |
| 123 | hypothesis_testing_introduction | 61 |
| 124 | hypothesis_testing_normal_distribution | 71 |
| 125 | normal_distribution_calculations | 57 |
| 126 | normal_variables | 23 |
| 127 | poisson_distribution | 30 |
| 128 | probability | 127 |
| 129 | probability_density_functions | 41 |
| 130 | probability_sampling_distributions | 5 |
| 131 | proportion_samples | 11 |
| 132 | rectangular_distribution | 18 |
| 133 | related_random_variables | 4 |
| 134 | t_distribution | 28 |

## Re-scan method

1. Enumerated all 134 booklet `conv_png` folders on VM; confirmed worker 19 slice.
2. Compared VM PNG counts vs golden-queue row counts per `source_pdf` (expect `pages − 1` question rows + 1 complete marker for pure booklets).
3. Verified contiguous `q1…qN` numbering and non-empty `inputs[]` on all partial rows.
4. Visual re-read of representative PNG pages across pure booklets:
   - Title pages (page 1) correctly excluded from question count.
   - Multi-part questions (e.g. modulus Q1a–d, parametric Q1a–d) captured as single page-level rows.
   - Graph-sketch / transformation pages (modulus exam Q3, transformations exam Q2) — not CAS-testable; partial verdict appropriate.
   - Related-rates and polynomials pages with two numbered questions on one page — covered by existing page-level rows (first-pass convention).
   - Last-page question numbers exceed row counts where multi-question pages exist (e.g. related_rates Q56 / 53 rows; parametric_exam Q162 / 158 rows; polynomials Q28 / 27 rows) — consistent with page-granularity model, not missing pages.
5. Verified all 21 statistics skip rows: correct page count in `review_basis`, `unsupported_reason` = `statistics/probability out of A-level Pure CAS scope`.
6. Checked `progress/vm_coverage.json` — all 32 sources marked `complete`.

## Gaps found / rows appended

| Category | Count |
|----------|-------|
| Missing sources | 0 |
| Missing question rows (page-level) | 0 |
| Missing skip markers | 0 |
| New rows appended | **0** |
| Existing rows extended | **0** |

No gaps at page-inventory level. Multi-question-per-page booklets already have one partial row per content page from the first pass; upgrading placeholder numeric checkpoints to problem-specific CAS inputs is out of scope for this inventory pass (would require per-question extraction across ~530 rows).

## Validation

| Check | Result |
|-------|--------|
| Worker 19 partial inputs strict markers (545 specs) | **545/545 OK** |
| `tests/check_exact_calculator_input_queue.py` | **OK** (9132 rows, 11551 inputs) |
| `tests/audit_vm_coverage.py` | **514/514 complete** |
| `tests/run_exact_queue.py --strict-markers` (full queue) | 11359/11441 OK (82 pre-existing failures outside worker 19 scope) |

## Notes

- **Page-level convention:** Booklet rows use `max(1, pages − 1)` question rows with `verdict: partial` and numeric-checkpoint `inputs[]`; one `source-complete` marker per booklet. This matches all 10 pure booklets in scope exactly.
- **Statistics tail:** Worker 19 completes the booklets tree (`statistics/t_distribution` is the final alphabetical booklet folder). All stats content correctly skipped.
- **Cross-worker overlap:** `standard_various/inequalities` is worker 18's last folder; worker 19 begins at `modulus_function_exam_questions`.

## Status

**PASS** — Worker 19 booklet sources fully inventoried; queue coverage complete; no append needed.
