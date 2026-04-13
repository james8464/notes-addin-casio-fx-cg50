# Graph Report - .  (2026-04-13)

## Corpus Check
- 15 files · ~151,262 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1536 nodes · 7747 edges · 15 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 242 edges
2. `num()` - 226 edges
3. `add()` - 140 edges
4. `mul()` - 126 edges
5. `flat()` - 126 edges
6. `CASIOApp` - 111 edges
7. `same()` - 106 edges
8. `fn()` - 101 edges
9. `neg()` - 99 edges
10. `num()` - 97 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 8 → community 7_
- `function_names_of()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 8 → community 3_
- `sim()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 8 → community 5_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 8 → community 9_
- `solve_rewrite_text()` --calls--> `begin_user_action()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 9_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (292): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+284 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (237): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+229 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (74): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+66 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (175): angle_text(), append_unique_float(), append_unique_solve_value(), apply_runtime_profile(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), clean_expr_text() (+167 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (112): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+104 more)

### Community 5 - "Community 5"
Cohesion: 0.11
Nodes (105): add(), angle_reduction_transforms(), build_named_power_term(), classify_solve_angle_arg(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), divide_terms_by_two_for_display() (+97 more)

### Community 6 - "Community 6"
Cohesion: 0.09
Nodes (88): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), begin_user_action(), _cache_get() (+80 more)

### Community 7 - "Community 7"
Cohesion: 0.08
Nodes (80): addq(), allowed_expression_from_terms(), build_named_power_product(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), direct_double_angle_rewrite(), direct_identity_target_rewrite() (+72 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (69): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cache_store(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params() (+61 more)

### Community 9 - "Community 9"
Cohesion: 0.13
Nodes (49): bridge_to_target(), cancel_fraction_common_factor_for_display(), detail_trig_expansion(), direct_expression_transform_lines(), direct_ratio_target_rewrite(), equivalent(), expand_small(), finish_verbose_proof() (+41 more)

### Community 10 - "Community 10"
Cohesion: 0.14
Nodes (25): angle_to_degree(), collect_symbols(), degree_int(), degree_mod_360(), _equivalent_uncached(), exact_trig_lines(), exact_trig_value(), extract_linear_combo_equation() (+17 more)

### Community 11 - "Community 11"
Cohesion: 0.11
Nodes (23): collect_angle_units(), collect_solve_angle_units(), compress_display_list(), contains_pi(), default_no_interval_span(), display_line_short(), equation_angle_mode(), interval_angle_mode() (+15 more)

### Community 12 - "Community 12"
Cohesion: 0.24
Nodes (21): comp(), direct(), expand_vars(), has(), kids(), mk(), normalise(), parse() (+13 more)

### Community 13 - "Community 13"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 14 - "Community 14"
Cohesion: 1.0
Nodes (2): main(), run_test()

## Knowledge Gaps
- **23 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 5` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.008) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 5` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 10`?**
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
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._