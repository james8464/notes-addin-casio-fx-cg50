# Graph Report - .  (2026-04-22)

## Corpus Check
- 20 files · ~755,385 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1749 nodes · 8259 edges · 17 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 251 edges
2. `num()` - 236 edges
3. `add()` - 149 edges
4. `CASIOApp` - 148 edges
5. `mul()` - 132 edges
6. `flat()` - 132 edges
7. `same()` - 108 edges
8. `neg()` - 104 edges
9. `fn()` - 103 edges
10. `num()` - 101 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 6_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 11_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 10_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 9_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 4 → community 8_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (332): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+324 more)

### Community 1 - "Community 1"
Cohesion: 0.04
Nodes (261): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+253 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (84): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+76 more)

### Community 3 - "Community 3"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 4 - "Community 4"
Cohesion: 0.04
Nodes (105): apply_runtime_profile(), _balance_parens(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store(), classify_solve_angle_arg(), clean_expr_text() (+97 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (105): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+97 more)

### Community 6 - "Community 6"
Cohesion: 0.1
Nodes (100): add(), collect_same_arg_terms(), depends(), direct_single_trig_info(), divide_terms_by_two_for_display(), equation_line(), expand_fraction(), expand_negative_add_terms() (+92 more)

### Community 7 - "Community 7"
Cohesion: 0.06
Nodes (98): angle_reduction_transforms(), bridge_to_target(), cancel_fraction_common_factor_for_display(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), detail_trig_expansion(), direct_double_angle_rewrite() (+90 more)

### Community 8 - "Community 8"
Cohesion: 0.07
Nodes (82): angle_text(), append_unique_float(), append_unique_value_node(), concise_root_text(), constant_numeric(), dedupe_values(), estimate_numeric_scan_samples(), estimate_solve_periods() (+74 more)

### Community 9 - "Community 9"
Cohesion: 0.09
Nodes (61): append_unique_solve_value(), begin_user_action(), collect_symbols(), compact_lines(), derive_cot_quadratic_expr(), direct_expression_transform_lines(), direct_ratio_target_rewrite(), equivalent() (+53 more)

### Community 10 - "Community 10"
Cohesion: 0.09
Nodes (50): addq(), angle_to_degree(), build_named_power_product(), build_named_power_term(), degree_int(), degree_mod_360(), divq(), exact_first_quadrant() (+42 more)

### Community 11 - "Community 11"
Cohesion: 0.12
Nodes (49): add_param_coeff_maps(), add_transform_constant_candidate(), allowed_expression_from_terms(), build_rewrite_allowed_info(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params() (+41 more)

### Community 12 - "Community 12"
Cohesion: 0.06
Nodes (42): cheap_same(), compact_duplicate_answer_lines(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const(), is_digit_char(), is_int_num() (+34 more)

### Community 13 - "Community 13"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

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

- **Why does `sim()` connect `Community 6` to `Community 4`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 10` to `Community 4`, `Community 6`, `Community 7`, `Community 8`, `Community 9`, `Community 11`?**
  _High betweenness centrality (0.005) - this node is a cross-community bridge._
- **What connects `Run a single test and return pass/fail.`, `Shared helper utilities for CASIO programs.  Keep this file MicroPython v1.9.4 f`, `Check if node is a number.` to the rest of the system?**
  _48 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._