# Graph Report - .  (2026-04-19)

## Corpus Check
- 15 files · ~202,090 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1606 nodes · 8128 edges · 15 communities detected
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
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 4_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 10_
- `symbols_of()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 9_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 8_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 11 → community 3_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (303): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+295 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (248): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+240 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (81): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+73 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (156): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), apply_runtime_profile(), _balance_parens(), best_solve_rewrite(), build_menu_pages() (+148 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (134): allowed_expression_from_terms(), bridge_to_target(), cancel_fraction_common_factor_for_display(), cheap_same(), classify_reciprocal_conjugate_binomial(), combine_fraction_sum_once(), common_denominator_step(), detail_trig_expansion() (+126 more)

### Community 5 - "Community 5"
Cohesion: 0.06
Nodes (112): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+104 more)

### Community 6 - "Community 6"
Cohesion: 0.08
Nodes (102): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+94 more)

### Community 7 - "Community 7"
Cohesion: 0.12
Nodes (97): add(), angle_reduction_transforms(), build_named_power_term(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), div(), divide_terms_by_two_for_display() (+89 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (45): addq(), angle_to_degree(), build_named_power_product(), degree_int(), degree_mod_360(), divq(), exact_num_value(), exact_pi_multiple() (+37 more)

### Community 9 - "Community 9"
Cohesion: 0.07
Nodes (41): begin_user_action(), best_proof_direction(), clean_expr_text(), collect_symbol_order(), collect_symbols(), collect_trig_argument_lower_symbols(), detect_template_params(), detect_transform_var() (+33 more)

### Community 10 - "Community 10"
Cohesion: 0.13
Nodes (34): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), constant_fit_preserve_named_trig(), depends_any(), extract_linear_param_expr(), extract_template_allowed_terms_raw(), extract_template_basis_terms() (+26 more)

### Community 11 - "Community 11"
Cohesion: 0.1
Nodes (32): cache_store(), equation_has_trig_content(), extract_polynomial_trig(), factor_common_term_for_proof(), factor_common_term_once(), factorisation_transforms(), function_names_of(), _function_names_uncached() (+24 more)

### Community 12 - "Community 12"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 13 - "Community 13"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 14 - "Community 14"
Cohesion: 1.0
Nodes (2): main(), run_test()

## Knowledge Gaps
- **24 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+19 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 4` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.007) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 7` to `Community 3`, `Community 4`, `Community 8`, `Community 9`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **What connects `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.` to the rest of the system?**
  _24 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._