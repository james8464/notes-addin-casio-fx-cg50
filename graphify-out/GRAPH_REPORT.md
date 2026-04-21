# Graph Report - .  (2026-04-21)

## Corpus Check
- 18 files · ~320,570 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1776 nodes · 8583 edges · 29 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 251 edges
2. `num()` - 237 edges
3. `add()` - 149 edges
4. `CASIOApp` - 148 edges
5. `mul()` - 133 edges
6. `flat()` - 132 edges
7. `same()` - 108 edges
8. `neg()` - 106 edges
9. `fn()` - 104 edges
10. `num()` - 100 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 7_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 10_
- `sim()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 6_
- `full_simplify()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 8_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 3_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (330): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+322 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (257): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+249 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (81): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+73 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (163): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), apply_runtime_profile(), _balance_parens(), begin_user_action(), best_solve_rewrite() (+155 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (116): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+108 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (107): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+99 more)

### Community 6 - "Community 6"
Cohesion: 0.1
Nodes (110): add(), angle_reduction_transforms(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), div(), divide_terms_by_two_for_display(), equation_line() (+102 more)

### Community 7 - "Community 7"
Cohesion: 0.07
Nodes (107): addq(), allowed_expression_from_terms(), build_named_power_product(), build_named_power_term(), cheap_same(), classify_reciprocal_conjugate_binomial(), combine_fraction_sum_once(), direct_double_angle_rewrite() (+99 more)

### Community 8 - "Community 8"
Cohesion: 0.09
Nodes (69): best_proof_direction(), bridge_to_target(), clean_expr_text(), common_denominator_step(), detail_trig_expansion(), direct_ratio_target_rewrite(), equivalent(), _equivalent_uncached() (+61 more)

### Community 9 - "Community 9"
Cohesion: 0.05
Nodes (35): check_quality(), ExprGrammar, has_working_steps(), main(), no_forbidden(), ProgramSpec, QualityChecker, Universal Test Generator - Generates random tests for ALL programs with comprehe (+27 more)

### Community 10 - "Community 10"
Cohesion: 0.11
Nodes (42): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), collect_trig_argument_lower_symbols(), constant_fit_preserve_named_trig(), depends_any() (+34 more)

### Community 11 - "Community 11"
Cohesion: 0.1
Nodes (31): cache_store(), _enforce_total_cache_limit(), equation_has_trig_content(), factor_common_term_for_proof(), factor_common_term_once(), factorisation_transforms(), function_names_of(), _function_names_uncached() (+23 more)

### Community 12 - "Community 12"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 13 - "Community 13"
Cohesion: 0.33
Nodes (10): is_lowercase_symbol_name(), match_cot_squared_fraction(), match_one_pm_cos(), match_one_pm_cos_norm(), match_scaled_div(), match_tan_squared_fraction(), rewrite_allowed_only(), rewrite_burden() (+2 more)

### Community 14 - "Community 14"
Cohesion: 0.42
Nodes (9): collect_symbols(), extract_linear_combo_equation(), extract_shift_equation(), extract_shifted_linear_angle(), independent_of_names(), match_shift_target(), split_equation_text_raw_or_zero(), split_identity_text_raw() (+1 more)

### Community 15 - "Community 15"
Cohesion: 0.54
Nodes (8): angle_to_degree(), degree_int(), degree_mod_360(), exact_trig_lines(), exact_trig_value(), quadrant_of_degree(), reference_degree(), replace_exact_trig()

### Community 16 - "Community 16"
Cohesion: 0.33
Nodes (0): 

### Community 17 - "Community 17"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 18 - "Community 18"
Cohesion: 1.0
Nodes (2): format_equation_human_readable(), split_coeff()

### Community 19 - "Community 19"
Cohesion: 1.0
Nodes (0): 

### Community 20 - "Community 20"
Cohesion: 1.0
Nodes (1): Verify derivative using SymPy.

### Community 21 - "Community 21"
Cohesion: 1.0
Nodes (1): Verify integral using SymPy.

### Community 22 - "Community 22"
Cohesion: 1.0
Nodes (1): Verify solution using SymPy.

### Community 23 - "Community 23"
Cohesion: 1.0
Nodes (1): Numerical derivative using central difference.

### Community 24 - "Community 24"
Cohesion: 1.0
Nodes (1): Verify using inverse operation.

### Community 25 - "Community 25"
Cohesion: 1.0
Nodes (1): Check if output has reasoning markers - LENIENT.

### Community 26 - "Community 26"
Cohesion: 1.0
Nodes (1): Check for forbidden snippets.

### Community 27 - "Community 27"
Cohesion: 1.0
Nodes (1): Check if output has step-by-step working - LENIENT.

### Community 28 - "Community 28"
Cohesion: 1.0
Nodes (1): Run all quality checks - LENIENT VERSION.

## Knowledge Gaps
- **70 isolated node(s):** `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.`, `Generate a random expression.`, `Generate a base expression (no recursion).` (+65 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 19`** (2 nodes): `test_20_questions.py`, `run_test()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 20`** (1 nodes): `Verify derivative using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 21`** (1 nodes): `Verify integral using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 22`** (1 nodes): `Verify solution using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 23`** (1 nodes): `Numerical derivative using central difference.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 24`** (1 nodes): `Verify using inverse operation.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 25`** (1 nodes): `Check if output has reasoning markers - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 26`** (1 nodes): `Check for forbidden snippets.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 27`** (1 nodes): `Check if output has step-by-step working - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 28`** (1 nodes): `Run all quality checks - LENIENT VERSION.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 6` to `Community 3`, `Community 7`, `Community 8`, `Community 10`, `Community 11`, `Community 13`, `Community 14`, `Community 15`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 7` to `Community 3`, `Community 6`, `Community 8`, `Community 10`, `Community 11`, `Community 13`, `Community 14`, `Community 15`?**
  _High betweenness centrality (0.005) - this node is a cross-community bridge._
- **What connects `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.` to the rest of the system?**
  _70 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._