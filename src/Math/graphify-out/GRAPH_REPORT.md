# Graph Report - src/Math  (2026-04-17)

## Corpus Check
- 6 files · ~128,826 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1351 nodes · 7400 edges · 21 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 244 edges
2. `num()` - 231 edges
3. `add()` - 143 edges
4. `mul()` - 128 edges
5. `flat()` - 127 edges
6. `same()` - 107 edges
7. `fn()` - 102 edges
8. `neg()` - 102 edges
9. `num()` - 99 edges
10. `sim()` - 88 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/algebraProgram.py → src/Math/algebraProgram.py  _Bridges community 11 → community 7_
- `depends_on()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/algebraProgram.py → src/Math/algebraProgram.py  _Bridges community 11 → community 16_
- `show()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/algebraProgram.py → src/Math/algebraProgram.py  _Bridges community 11 → community 12_
- `num()` --calls--> `gcd()`  [EXTRACTED]
  src/Math/algebraProgram.py → src/Math/algebraProgram.py  _Bridges community 20 → community 7_
- `num_text()` --calls--> `num()`  [EXTRACTED]
  src/Math/algebraProgram.py → src/Math/algebraProgram.py  _Bridges community 7 → community 1_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.05
Nodes (245): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+237 more)

### Community 1 - "Community 1"
Cohesion: 0.02
Nodes (93): add_term_texts(), algebra_factor_text(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text(), algebra_mode_9_lines(), algebra_mode_9_text(), algebra_solve_text() (+85 more)

### Community 2 - "Community 2"
Cohesion: 0.05
Nodes (128): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), apply_runtime_profile(), best_solve_rewrite(), build_named_power_product(), build_named_power_term() (+120 more)

### Community 3 - "Community 3"
Cohesion: 0.06
Nodes (112): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+104 more)

### Community 4 - "Community 4"
Cohesion: 0.09
Nodes (99): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+91 more)

### Community 5 - "Community 5"
Cohesion: 0.11
Nodes (99): add(), angle_reduction_transforms(), classify_solve_angle_arg(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), direct_ratio_target_rewrite(), divide_terms_by_two_for_display() (+91 more)

### Community 6 - "Community 6"
Cohesion: 0.08
Nodes (72): addq(), allowed_expression_from_terms(), cheap_same(), classify_reciprocal_conjugate_binomial(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), direct_single_trig_info(), display_abs() (+64 more)

### Community 7 - "Community 7"
Cohesion: 0.16
Nodes (66): add(), addq(), apply_values(), build_quadratic_from_roots(), cancel_fraction_factor(), cartesian_equation_lines(), cartesian_from_param_exprs(), cartesian_linear_pair() (+58 more)

### Community 8 - "Community 8"
Cohesion: 0.13
Nodes (57): bridge_to_target(), common_denominator_step(), detail_trig_expansion(), equivalent(), _equivalent_uncached(), expand_small(), finish_verbose_proof(), _full_simplify_uncached() (+49 more)

### Community 9 - "Community 9"
Cohesion: 0.07
Nodes (51): begin_user_action(), best_proof_direction(), clean_expr_text(), collect_symbols(), collect_trig_argument_lower_symbols(), compact_lines(), direct_expression_transform_lines(), drop_trailing_solution_line() (+43 more)

### Community 10 - "Community 10"
Cohesion: 0.12
Nodes (50): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params() (+42 more)

### Community 11 - "Community 11"
Cohesion: 0.1
Nodes (42): build_rewrite_allowed_info(), cache_store(), canonical_compare_form(), combine_fractions(), equivalent(), expand_all_pow_sqrt(), expand_for_solving(), expand_mul_distribute() (+34 more)

### Community 12 - "Community 12"
Cohesion: 0.08
Nodes (36): algebra_mode_3_lines(), apply_runtime_profile(), begin_user_action(), binomial_coefficient(), cartesian_parametric_lines(), clear_all_caches(), comp_cli(), compare_expressions() (+28 more)

### Community 13 - "Community 13"
Cohesion: 0.08
Nodes (34): angle_to_degree(), collect_angle_units(), collect_solve_angle_units(), compress_display_list(), contains_pi(), default_no_interval_span(), degree_int(), degree_mod_360() (+26 more)

### Community 14 - "Community 14"
Cohesion: 0.08
Nodes (31): algebra_rewrite_term_text(), cartesian_depends(), choose_primary_var(), choose_rewrite_symbol(), choose_rewrite_var(), choose_template_sample_values(), choose_unused_symbol(), collect_symbol_names() (+23 more)

### Community 15 - "Community 15"
Cohesion: 0.12
Nodes (27): cache_store(), equation_has_trig_content(), factor_common_term_for_proof(), factor_common_term_once(), factorisation_transforms(), function_names_of(), _function_names_uncached(), kind_names_of() (+19 more)

### Community 16 - "Community 16"
Cohesion: 0.18
Nodes (23): depends_any(), depends_on(), E(), expr_depends_on(), inverse_cli(), inverse_function(), match_const_base_exp_linear_in_var(), match_const_base_log_linear_in_var() (+15 more)

### Community 17 - "Community 17"
Cohesion: 0.2
Nodes (17): half_angle_expr(), _is_cos_squared_term(), is_lowercase_symbol_name(), _is_sin_squared_term(), match_cos_squared_term(), match_cot_squared_fraction(), match_one_pm_cos(), match_one_pm_cos_norm() (+9 more)

### Community 18 - "Community 18"
Cohesion: 0.4
Nodes (5): divq(), mulq(), negq(), solve_linear_system(), subq()

### Community 19 - "Community 19"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 20 - "Community 20"
Cohesion: 0.67
Nodes (4): gcd(), lcm(), positive_divisors(), rational_root_candidates()

## Knowledge Gaps
- **24 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+19 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 5` to `Community 2`, `Community 6`, `Community 8`, `Community 9`, `Community 10`, `Community 13`, `Community 15`, `Community 17`?**
  _High betweenness centrality (0.010) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 5` to `Community 2`, `Community 6`, `Community 8`, `Community 9`, `Community 10`, `Community 13`, `Community 15`, `Community 17`?**
  _High betweenness centrality (0.009) - this node is a cross-community bridge._
- **Why does `add()` connect `Community 5` to `Community 2`, `Community 6`, `Community 8`, `Community 9`, `Community 10`, `Community 15`, `Community 17`?**
  _High betweenness centrality (0.002) - this node is a cross-community bridge._
- **What connects `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.` to the rest of the system?**
  _24 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._