# Graph Report - .  (2026-04-17)

## Corpus Check
- 15 files · ~158,811 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1570 nodes · 8011 edges · 16 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 244 edges
2. `num()` - 231 edges
3. `add()` - 143 edges
4. `mul()` - 128 edges
5. `flat()` - 127 edges
6. `CASIOApp` - 111 edges
7. `same()` - 107 edges
8. `fn()` - 102 edges
9. `neg()` - 102 edges
10. `num()` - 99 edges

## Surprising Connections (you probably didn't know these)
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 10_
- `symbols_of()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 6_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 11_
- `full_simplify()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 9_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 7_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (300): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+292 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (245): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+237 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (74): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+66 more)

### Community 3 - "Community 3"
Cohesion: 0.06
Nodes (112): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+104 more)

### Community 4 - "Community 4"
Cohesion: 0.07
Nodes (101): allowed_expression_from_terms(), build_named_power_product(), cache_store(), cheap_same(), classify_reciprocal_conjugate_binomial(), combine_fraction_sum_once(), common_denominator_step(), depends() (+93 more)

### Community 5 - "Community 5"
Cohesion: 0.09
Nodes (97): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+89 more)

### Community 6 - "Community 6"
Cohesion: 0.04
Nodes (89): append_unique_value_node(), apply_runtime_profile(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), classify_solve_angle_arg(), clean_expr_text(), clear_engine_caches() (+81 more)

### Community 7 - "Community 7"
Cohesion: 0.07
Nodes (93): angle_text(), append_unique_float(), append_unique_solve_value(), compact_lines(), concise_root_text(), constant_numeric(), dedupe_values(), equation_has_trig_content() (+85 more)

### Community 8 - "Community 8"
Cohesion: 0.13
Nodes (77): add(), angle_reduction_transforms(), build_named_power_term(), collect_same_arg_terms(), derive_cot_quadratic_expr(), div(), divide_terms_by_two_for_display(), equation_line() (+69 more)

### Community 9 - "Community 9"
Cohesion: 0.12
Nodes (55): bridge_to_target(), detail_trig_expansion(), direct_ratio_target_rewrite(), equivalent(), _equivalent_uncached(), expand_safe_trig_tree(), expand_small(), finish_verbose_proof() (+47 more)

### Community 10 - "Community 10"
Cohesion: 0.09
Nodes (51): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params() (+43 more)

### Community 11 - "Community 11"
Cohesion: 0.1
Nodes (47): addq(), angle_to_degree(), degree_int(), degree_mod_360(), direct_double_angle_rewrite(), divq(), exact_num_value(), exact_pi_multiple() (+39 more)

### Community 12 - "Community 12"
Cohesion: 0.24
Nodes (21): comp(), direct(), expand_vars(), has(), kids(), mk(), normalise(), parse() (+13 more)

### Community 13 - "Community 13"
Cohesion: 0.11
Nodes (22): collect_angle_units(), collect_solve_angle_units(), compress_display_list(), contains_pi(), default_no_interval_span(), display_line_short(), equation_angle_mode(), interval_angle_mode() (+14 more)

### Community 14 - "Community 14"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 15 - "Community 15"
Cohesion: 1.0
Nodes (2): main(), run_test()

## Knowledge Gaps
- **23 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 4` to `Community 6`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`, `Community 13`?**
  _High betweenness centrality (0.007) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 11` to `Community 4`, `Community 6`, `Community 7`, `Community 8`, `Community 9`, `Community 10`?**
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
  _Cohesion score 0.06 - nodes in this community are weakly interconnected._