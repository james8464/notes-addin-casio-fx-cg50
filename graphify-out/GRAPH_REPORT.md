# Graph Report - .  (2026-04-11)

## Corpus Check
- 13 files · ~181,134 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1482 nodes · 7585 edges · 16 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 242 edges
2. `num()` - 226 edges
3. `add()` - 140 edges
4. `mul()` - 126 edges
5. `flat()` - 126 edges
6. `same()` - 106 edges
7. `fn()` - 101 edges
8. `CASIOApp` - 99 edges
9. `neg()` - 99 edges
10. `num()` - 96 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 7_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 10_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 12_
- `sim()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 5_
- `full_simplify()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 9_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (292): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+284 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (234): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+226 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (43): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker(), algebra_rewrite_checker() (+35 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (115): apply_runtime_profile(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), cache_store(), clean_expr_text(), clear_engine_caches(), collect_symbol_order() (+107 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (107): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+99 more)

### Community 5 - "Community 5"
Cohesion: 0.12
Nodes (105): add(), angle_reduction_transforms(), build_named_power_term(), collect_same_arg_terms(), derive_cot_quadratic_expr(), direct_ratio_target_rewrite(), div(), divide_terms_by_two_for_display() (+97 more)

### Community 6 - "Community 6"
Cohesion: 0.09
Nodes (88): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), begin_user_action(), _cache_get() (+80 more)

### Community 7 - "Community 7"
Cohesion: 0.08
Nodes (73): cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), direct_single_trig_info(), display_abs(), display_neg() (+65 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (69): angle_text(), append_unique_float(), append_unique_solve_value(), classify_solve_angle_arg(), concise_root_text(), dedupe_values(), depends(), eval_numeric() (+61 more)

### Community 9 - "Community 9"
Cohesion: 0.15
Nodes (41): constant_numeric(), detail_trig_expansion(), detect_transform_var(), expand_embedded_small(), expand_safe_trig_tree(), expand_small(), expand_trig_tree(), extremum_rewrite_to_linear_combo() (+33 more)

### Community 10 - "Community 10"
Cohesion: 0.1
Nodes (37): add_param_coeff_maps(), add_transform_constant_candidate(), allowed_expression_from_terms(), build_named_power_product(), build_rewrite_allowed_info(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), depends_any() (+29 more)

### Community 11 - "Community 11"
Cohesion: 0.15
Nodes (35): bridge_to_target(), cancel_fraction_common_factor_for_display(), equivalent(), finish_verbose_proof(), finish_verbose_proof_structured(), half_sum_diff_args(), match_generic_shift_side(), match_plus_minus_pair() (+27 more)

### Community 12 - "Community 12"
Cohesion: 0.13
Nodes (28): addq(), angle_to_degree(), degree_int(), degree_mod_360(), divq(), exact_num_value(), exact_pi_multiple(), exact_trig_lines() (+20 more)

### Community 13 - "Community 13"
Cohesion: 0.11
Nodes (23): collect_angle_units(), collect_solve_angle_units(), compress_display_list(), contains_pi(), default_no_interval_span(), display_line_short(), equation_angle_mode(), interval_angle_mode() (+15 more)

### Community 14 - "Community 14"
Cohesion: 0.24
Nodes (21): comp(), direct(), expand_vars(), has(), kids(), mk(), normalise(), parse() (+13 more)

### Community 15 - "Community 15"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

## Knowledge Gaps
- **23 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 5` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`, `Community 12`, `Community 13`?**
  _High betweenness centrality (0.008) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 5` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`, `Community 12`?**
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