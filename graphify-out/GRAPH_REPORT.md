# Graph Report - .  (2026-04-10)

## Corpus Check
- 13 files · ~142,254 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1459 nodes · 7415 edges · 15 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 240 edges
2. `num()` - 222 edges
3. `add()` - 137 edges
4. `mul()` - 123 edges
5. `flat()` - 123 edges
6. `same()` - 104 edges
7. `fn()` - 99 edges
8. `neg()` - 98 edges
9. `num()` - 96 edges
10. `CASIOApp` - 91 edges

## Surprising Connections (you probably didn't know these)
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 7 → community 11_
- `symbols_of()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 7 → community 3_
- `sim()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 7 → community 9_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 7 → community 8_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 7 → community 6_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (290): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+282 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (229): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+221 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (42): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker(), algebra_rewrite_checker() (+34 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (107): append_unique_float(), apply_runtime_profile(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_named_power_product(), build_named_power_term(), classify_solve_angle_arg() (+99 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (107): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+99 more)

### Community 5 - "Community 5"
Cohesion: 0.09
Nodes (88): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), begin_user_action(), _cache_get() (+80 more)

### Community 6 - "Community 6"
Cohesion: 0.07
Nodes (91): angle_text(), append_unique_solve_value(), compact_lines(), concise_root_text(), constant_numeric(), dedupe_values(), display_neg(), estimate_numeric_scan_samples() (+83 more)

### Community 7 - "Community 7"
Cohesion: 0.08
Nodes (90): addq(), angle_to_degree(), cache_store(), combine_fraction_sum_once(), depends(), direct_single_trig_info(), display_abs(), divq() (+82 more)

### Community 8 - "Community 8"
Cohesion: 0.12
Nodes (85): add(), angle_reduction_transforms(), collect_same_arg_terms(), derive_cot_quadratic_expr(), div(), divide_terms_by_two_for_display(), equation_line(), equivalent() (+77 more)

### Community 9 - "Community 9"
Cohesion: 0.1
Nodes (62): bridge_to_target(), detail_trig_expansion(), direct_ratio_target_rewrite(), expand_fraction(), expand_product(), expand_safe_trig_tree(), expand_small(), expand_square() (+54 more)

### Community 10 - "Community 10"
Cohesion: 0.12
Nodes (42): cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), direct_double_angle_rewrite(), extract_binomial(), factor_difference_of_squares_for_display(), factor_difference_once(), factor_trig_power_pair_for_proof() (+34 more)

### Community 11 - "Community 11"
Cohesion: 0.14
Nodes (36): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params() (+28 more)

### Community 12 - "Community 12"
Cohesion: 0.24
Nodes (21): comp(), direct(), expand_vars(), has(), kids(), mk(), normalise(), parse() (+13 more)

### Community 13 - "Community 13"
Cohesion: 0.67
Nodes (6): degree_int(), degree_mod_360(), exact_trig_lines(), exact_trig_value(), quadrant_of_degree(), reference_degree()

### Community 14 - "Community 14"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

## Knowledge Gaps
- **23 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 9` to `Community 3`, `Community 6`, `Community 7`, `Community 8`, `Community 10`, `Community 11`, `Community 13`?**
  _High betweenness centrality (0.008) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 7` to `Community 3`, `Community 6`, `Community 8`, `Community 9`, `Community 10`, `Community 11`, `Community 13`?**
  _High betweenness centrality (0.007) - this node is a cross-community bridge._
- **What connects `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.` to the rest of the system?**
  _23 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._