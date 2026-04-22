# Graph Report - .  (2026-04-22)

## Corpus Check
- 20 files · ~753,082 words
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
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 9_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 8_
- `split_coeff()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 7_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 11_
- `match_fn_power_term()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 6_

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
Cohesion: 0.03
Nodes (146): apply_runtime_profile(), _balance_parens(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store(), classify_solve_angle_arg() (+138 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (105): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+97 more)

### Community 6 - "Community 6"
Cohesion: 0.11
Nodes (98): add(), angle_reduction_transforms(), build_named_power_term(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), direct_double_angle_rewrite(), div() (+90 more)

### Community 7 - "Community 7"
Cohesion: 0.06
Nodes (88): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), concise_root_text(), dedupe_values(), direct_single_trig_info(), _equivalent_uncached() (+80 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (77): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), combine_fraction_sum_once(), common_denominator_step(), constant_fit_preserve_named_trig() (+69 more)

### Community 9 - "Community 9"
Cohesion: 0.06
Nodes (75): allowed_expression_from_terms(), build_named_power_product(), cheap_same(), classify_reciprocal_conjugate_binomial(), direct_identity_target_rewrite(), display_abs(), display_neg(), expand_fraction() (+67 more)

### Community 10 - "Community 10"
Cohesion: 0.06
Nodes (42): cheap_same(), compact_duplicate_answer_lines(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const(), is_digit_char(), is_int_num() (+34 more)

### Community 11 - "Community 11"
Cohesion: 0.16
Nodes (41): bridge_to_target(), detail_trig_expansion(), direct_ratio_target_rewrite(), equivalent(), expand_small(), finish_verbose_proof(), format_rewrite_lines(), _full_simplify_uncached() (+33 more)

### Community 12 - "Community 12"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 13 - "Community 13"
Cohesion: 0.17
Nodes (20): addq(), angle_to_degree(), degree_int(), degree_mod_360(), divq(), exact_pi_multiple(), exact_trig_lines(), exact_trig_value() (+12 more)

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

- **Why does `sim()` connect `Community 8` to `Community 3`, `Community 6`, `Community 7`, `Community 9`, `Community 11`, `Community 13`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 6` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 11`, `Community 13`?**
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
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._