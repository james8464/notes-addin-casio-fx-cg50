# Graph Report - .  (2026-04-19)

## Corpus Check
- 15 files · ~262,987 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1684 nodes · 8234 edges · 27 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 244 edges
2. `num()` - 231 edges
3. `add()` - 143 edges
4. `mul()` - 128 edges
5. `flat()` - 127 edges
6. `CASIOApp` - 122 edges
7. `same()` - 107 edges
8. `fn()` - 102 edges
9. `neg()` - 102 edges
10. `num()` - 99 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 9 → community 5_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 9 → community 11_
- `sort_key()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 9 → community 3_
- `_show()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 9 → community 13_
- `match_fn_power_term()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 9 → community 7_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (305): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+297 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (250): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+242 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (81): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+73 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (149): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), apply_runtime_profile(), best_solve_rewrite(), build_menu_pages(), build_named_power_product() (+141 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (114): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+106 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (118): bridge_to_target(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), detail_trig_expansion(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), direct_ratio_target_rewrite() (+110 more)

### Community 6 - "Community 6"
Cohesion: 0.08
Nodes (104): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+96 more)

### Community 7 - "Community 7"
Cohesion: 0.12
Nodes (93): add(), angle_reduction_transforms(), build_named_power_term(), collect_same_arg_terms(), derive_cot_quadratic_expr(), div(), divide_terms_by_two_for_display(), equation_line() (+85 more)

### Community 8 - "Community 8"
Cohesion: 0.05
Nodes (34): check_quality(), ExprGrammar, has_working_steps(), main(), no_forbidden(), ProgramSpec, QualityChecker, Universal Test Generator - Generates random tests for ALL programs with comprehe (+26 more)

### Community 9 - "Community 9"
Cohesion: 0.1
Nodes (58): add_param_coeff_maps(), add_transform_constant_candidate(), allowed_expression_from_terms(), build_rewrite_allowed_info(), cache_store(), cancel_fraction_common_factor_for_display(), combine_fraction_sum_once(), constant_fit_preserve_named_trig() (+50 more)

### Community 10 - "Community 10"
Cohesion: 0.09
Nodes (32): _balance_parens(), begin_user_action(), best_proof_direction(), clean_expr_text(), collect_trig_argument_lower_symbols(), direct_expression_transform_lines(), finalize_proof_identity_lines(), finalize_proof_lines() (+24 more)

### Community 11 - "Community 11"
Cohesion: 0.14
Nodes (28): addq(), angle_to_degree(), degree_int(), degree_mod_360(), divq(), exact_num_value(), exact_pi_multiple(), exact_trig_lines() (+20 more)

### Community 12 - "Community 12"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 13 - "Community 13"
Cohesion: 0.15
Nodes (22): collect_symbols(), display_abs(), _equivalent_uncached(), extract_linear_combo_equation(), extract_shift_equation(), extract_shifted_linear_angle(), half_sum_diff_args(), independent_of_names() (+14 more)

### Community 14 - "Community 14"
Cohesion: 0.13
Nodes (20): collect_angle_units(), compress_display_list(), contains_pi(), default_no_interval_span(), display_line_short(), interval_angle_mode(), is_alpha_char(), is_digit_char() (+12 more)

### Community 15 - "Community 15"
Cohesion: 0.26
Nodes (13): depends(), direct_single_trig_info(), extract_polynomial_symbol(), extract_polynomial_trig(), extract_quadratic_trig(), is_int_num(), match_nonzero_reciprocal_factor(), merge_tan_sub_base() (+5 more)

### Community 16 - "Community 16"
Cohesion: 0.47
Nodes (6): match_cot_simple_term(), match_double_angle_arg(), match_double_single_product_term(), match_tan2_cot_product(), match_tan_double_term(), solve_tan2_cot_expr()

### Community 17 - "Community 17"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 18 - "Community 18"
Cohesion: 1.0
Nodes (1): Verify derivative using SymPy.

### Community 19 - "Community 19"
Cohesion: 1.0
Nodes (1): Verify integral using SymPy.

### Community 20 - "Community 20"
Cohesion: 1.0
Nodes (1): Verify solution using SymPy.

### Community 21 - "Community 21"
Cohesion: 1.0
Nodes (1): Numerical derivative using central difference.

### Community 22 - "Community 22"
Cohesion: 1.0
Nodes (1): Verify using inverse operation.

### Community 23 - "Community 23"
Cohesion: 1.0
Nodes (1): Check if output has reasoning markers - LENIENT.

### Community 24 - "Community 24"
Cohesion: 1.0
Nodes (1): Check for forbidden snippets.

### Community 25 - "Community 25"
Cohesion: 1.0
Nodes (1): Check if output has step-by-step working - LENIENT.

### Community 26 - "Community 26"
Cohesion: 1.0
Nodes (1): Run all quality checks - LENIENT VERSION.

## Knowledge Gaps
- **60 isolated node(s):** `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.`, `Generate a random expression.`, `Generate a base expression (no recursion).` (+55 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 18`** (1 nodes): `Verify derivative using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 19`** (1 nodes): `Verify integral using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 20`** (1 nodes): `Verify solution using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 21`** (1 nodes): `Numerical derivative using central difference.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 22`** (1 nodes): `Verify using inverse operation.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 23`** (1 nodes): `Check if output has reasoning markers - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 24`** (1 nodes): `Check for forbidden snippets.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 25`** (1 nodes): `Check if output has step-by-step working - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 26`** (1 nodes): `Run all quality checks - LENIENT VERSION.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 5` to `Community 3`, `Community 7`, `Community 9`, `Community 10`, `Community 11`, `Community 13`, `Community 14`, `Community 15`, `Community 16`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 7` to `Community 3`, `Community 5`, `Community 9`, `Community 10`, `Community 11`, `Community 13`, `Community 15`, `Community 16`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **What connects `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.` to the rest of the system?**
  _60 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._