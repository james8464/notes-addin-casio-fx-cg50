# Graph Report - .  (2026-04-23)

## Corpus Check
- 21 files · ~180,829 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1757 nodes · 8324 edges · 17 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 252 edges
2. `num()` - 239 edges
3. `add()` - 151 edges
4. `CASIOApp` - 148 edges
5. `mul()` - 133 edges
6. `flat()` - 132 edges
7. `same()` - 109 edges
8. `neg()` - 106 edges
9. `fn()` - 105 edges
10. `num()` - 101 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 10 → community 4_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 10 → community 11_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 10 → community 8_
- `full_simplify()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 10 → community 6_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
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
Nodes (85): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+77 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (186): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), apply_runtime_profile(), _balance_parens(), begin_user_action(), best_solve_rewrite() (+178 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (124): allowed_expression_from_terms(), cancel_fraction_common_factor_for_display(), cheap_same(), combine_fraction_sum_once(), common_denominator_step(), depends(), direct_double_angle_rewrite(), direct_identity_target_rewrite() (+116 more)

### Community 5 - "Community 5"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 6 - "Community 6"
Cohesion: 0.09
Nodes (119): add(), angle_reduction_transforms(), branch_target_value(), bridge_to_target(), build_known_trig_value_branches(), build_known_value_branch(), collect_same_arg_terms(), derive_cot_quadratic_expr() (+111 more)

### Community 7 - "Community 7"
Cohesion: 0.07
Nodes (105): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+97 more)

### Community 8 - "Community 8"
Cohesion: 0.09
Nodes (46): addq(), angle_to_degree(), build_named_power_product(), build_named_power_term(), degree_int(), degree_mod_360(), display_abs(), divq() (+38 more)

### Community 9 - "Community 9"
Cohesion: 0.06
Nodes (42): cheap_same(), compact_duplicate_answer_lines(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const(), is_digit_char(), is_int_num() (+34 more)

### Community 10 - "Community 10"
Cohesion: 0.07
Nodes (42): best_proof_direction(), cache_store(), _enforce_total_cache_limit(), equation_has_trig_content(), _equivalent_uncached(), expand_safe_trig_tree(), _full_simplify_uncached(), function_names_of() (+34 more)

### Community 11 - "Community 11"
Cohesion: 0.12
Nodes (36): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), collect_symbol_order(), collect_trig_argument_lower_symbols(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params() (+28 more)

### Community 12 - "Community 12"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 13 - "Community 13"
Cohesion: 0.43
Nodes (2): run_cli(), TransformRegressionTests

### Community 14 - "Community 14"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 15 - "Community 15"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 16 - "Community 16"
Cohesion: 0.67
Nodes (3): main(), Run a single test and return pass/fail., run_test()

## Knowledge Gaps
- **48 isolated node(s):** `Run a single test and return pass/fail.`, `Shared helper utilities for CASIO programs.  Keep this file MicroPython v1.9.4 f`, `Check if node is a number.`, `Check if node is a symbol.`, `Check if node is a named constant.` (+43 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 4` to `Community 3`, `Community 6`, `Community 8`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 8` to `Community 3`, `Community 4`, `Community 6`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **What connects `Run a single test and return pass/fail.`, `Shared helper utilities for CASIO programs.  Keep this file MicroPython v1.9.4 f`, `Check if node is a number.` to the rest of the system?**
  _48 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._