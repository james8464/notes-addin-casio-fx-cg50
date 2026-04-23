# Graph Report - .  (2026-04-23)

## Corpus Check
- 21 files · ~182,989 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1772 nodes · 8398 edges · 19 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 255 edges
2. `num()` - 241 edges
3. `add()` - 153 edges
4. `CASIOApp` - 148 edges
5. `mul()` - 135 edges
6. `flat()` - 134 edges
7. `same()` - 111 edges
8. `fn()` - 107 edges
9. `neg()` - 107 edges
10. `num()` - 101 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 12 → community 4_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 12 → community 9_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 12 → community 7_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 12 → community 3_
- `_solve_solve_text_once()` --calls--> `begin_user_action()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 10 → community 3_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (333): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+325 more)

### Community 1 - "Community 1"
Cohesion: 0.04
Nodes (261): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+253 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (89): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+81 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (157): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), best_solve_rewrite(), classify_solve_angle_arg(), collect_angle_units(), collect_solve_angle_units() (+149 more)

### Community 4 - "Community 4"
Cohesion: 0.09
Nodes (121): add(), angle_reduction_transforms(), collect_same_arg_terms(), depends(), direct_single_trig_info(), divide_terms_by_two_for_display(), equation_line(), exact_first_quadrant() (+113 more)

### Community 5 - "Community 5"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 6 - "Community 6"
Cohesion: 0.07
Nodes (105): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+97 more)

### Community 7 - "Community 7"
Cohesion: 0.08
Nodes (72): bridge_to_target(), cancel_fraction_common_factor_for_display(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), detail_trig_expansion(), direct_expression_transform_lines(), display_abs() (+64 more)

### Community 8 - "Community 8"
Cohesion: 0.11
Nodes (58): branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), derive_cot_quadratic_expr(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), direct_ratio_target_rewrite(), div() (+50 more)

### Community 9 - "Community 9"
Cohesion: 0.1
Nodes (51): add_param_coeff_maps(), add_transform_constant_candidate(), allowed_expression_from_terms(), build_named_power_product(), build_named_power_term(), build_rewrite_allowed_info(), collect_symbol_order(), combine_fraction_sum_once() (+43 more)

### Community 10 - "Community 10"
Cohesion: 0.07
Nodes (46): apply_runtime_profile(), _balance_parens(), begin_user_action(), best_proof_direction(), build_menu_pages(), clean_expr_text(), clear_engine_caches(), collect_trig_argument_lower_symbols() (+38 more)

### Community 11 - "Community 11"
Cohesion: 0.06
Nodes (43): cheap_same(), compact_duplicate_answer_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const(), is_digit_char() (+35 more)

### Community 12 - "Community 12"
Cohesion: 0.09
Nodes (31): cache_store(), _enforce_total_cache_limit(), equation_has_trig_content(), factor_common_term_for_proof(), function_names_of(), _function_names_uncached(), kind_names_of(), _kind_names_uncached() (+23 more)

### Community 13 - "Community 13"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 14 - "Community 14"
Cohesion: 0.16
Nodes (21): addq(), angle_to_degree(), degree_int(), degree_mod_360(), divq(), exact_pi_multiple(), exact_trig_lines(), exact_trig_value() (+13 more)

### Community 15 - "Community 15"
Cohesion: 0.27
Nodes (2): run_cli(), TransformRegressionTests

### Community 16 - "Community 16"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 17 - "Community 17"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 18 - "Community 18"
Cohesion: 0.67
Nodes (3): main(), Run a single test and return pass/fail., run_test()

## Knowledge Gaps
- **49 isolated node(s):** `Run a single test and return pass/fail.`, `Shared helper utilities for CASIO programs.  Keep this file MicroPython v1.9.4 f`, `Check if node is a number.`, `Check if node is a symbol.`, `Check if node is a named constant.` (+44 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 4` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 12`, `Community 14`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **What connects `Run a single test and return pass/fail.`, `Shared helper utilities for CASIO programs.  Keep this file MicroPython v1.9.4 f`, `Check if node is a number.` to the rest of the system?**
  _49 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 4` be split into smaller, more focused modules?**
  _Cohesion score 0.09 - nodes in this community are weakly interconnected._