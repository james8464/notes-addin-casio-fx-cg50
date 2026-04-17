# Graph Report - .  (2026-04-17)

## Corpus Check
- 15 files · ~159,547 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1578 nodes · 8035 edges · 17 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 244 edges
2. `num()` - 231 edges
3. `add()` - 143 edges
4. `mul()` - 128 edges
5. `flat()` - 127 edges
6. `CASIOApp` - 116 edges
7. `same()` - 107 edges
8. `fn()` - 102 edges
9. `neg()` - 102 edges
10. `num()` - 99 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 6_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 8_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 11_
- `_show()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 14_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 10_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (300): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+292 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (245): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+237 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (77): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+69 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (128): apply_runtime_profile(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), cache_store(), classify_solve_angle_arg(), clean_expr_text(), clear_engine_caches() (+120 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (112): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+104 more)

### Community 5 - "Community 5"
Cohesion: 0.09
Nodes (97): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+89 more)

### Community 6 - "Community 6"
Cohesion: 0.08
Nodes (87): cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), direct_single_trig_info(), divide_terms_by_two_for_display(), expand_fraction() (+79 more)

### Community 7 - "Community 7"
Cohesion: 0.13
Nodes (83): add(), angle_reduction_transforms(), build_named_power_term(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), equation_line(), exact_constant_candidates() (+75 more)

### Community 8 - "Community 8"
Cohesion: 0.1
Nodes (64): add_param_coeff_maps(), add_transform_constant_candidate(), append_unique_value_node(), build_rewrite_allowed_info(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), constant_numeric(), depends_any() (+56 more)

### Community 9 - "Community 9"
Cohesion: 0.11
Nodes (54): angle_text(), append_unique_float(), append_unique_solve_value(), concise_root_text(), dedupe_values(), eval_numeric(), eval_numeric_mode(), exact_angle_value_node() (+46 more)

### Community 10 - "Community 10"
Cohesion: 0.13
Nodes (52): bridge_to_target(), cancel_fraction_common_factor_for_display(), detail_trig_expansion(), direct_ratio_target_rewrite(), equivalent(), expand_safe_trig_tree(), expand_small(), finish_verbose_proof() (+44 more)

### Community 11 - "Community 11"
Cohesion: 0.1
Nodes (37): addq(), allowed_expression_from_terms(), angle_to_degree(), build_named_power_product(), degree_int(), degree_mod_360(), divq(), exact_num_value() (+29 more)

### Community 12 - "Community 12"
Cohesion: 0.11
Nodes (23): collect_angle_units(), collect_solve_angle_units(), compress_display_list(), contains_pi(), default_no_interval_span(), display_line_short(), equation_angle_mode(), interval_angle_mode() (+15 more)

### Community 13 - "Community 13"
Cohesion: 0.24
Nodes (21): comp(), direct(), expand_vars(), has(), kids(), mk(), normalise(), parse() (+13 more)

### Community 14 - "Community 14"
Cohesion: 0.38
Nodes (7): display_abs(), display_neg(), needs_div_brackets(), ordered_sum_text(), pr(), _show(), _show_uncached()

### Community 15 - "Community 15"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 16 - "Community 16"
Cohesion: 1.0
Nodes (2): main(), run_test()

## Knowledge Gaps
- **23 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 6` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`, `Community 12`, `Community 14`?**
  _High betweenness centrality (0.007) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 7` to `Community 3`, `Community 6`, `Community 8`, `Community 9`, `Community 10`, `Community 11`, `Community 14`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **What connects `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.` to the rest of the system?**
  _23 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._