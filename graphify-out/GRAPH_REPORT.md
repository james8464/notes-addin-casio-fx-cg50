# Graph Report - .  (2026-04-20)

## Corpus Check
- 16 files · ~315,182 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1713 nodes · 8352 edges · 26 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 251 edges
2. `num()` - 237 edges
3. `add()` - 149 edges
4. `mul()` - 133 edges
5. `flat()` - 132 edges
6. `CASIOApp` - 122 edges
7. `same()` - 108 edges
8. `neg()` - 106 edges
9. `fn()` - 104 edges
10. `num()` - 99 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 6_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 8_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 12_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 11_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 7_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (311): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+303 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (253): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+245 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (81): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+73 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (114): apply_runtime_profile(), _balance_parens(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store(), classify_solve_angle_arg() (+106 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (116): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+108 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (107): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+99 more)

### Community 6 - "Community 6"
Cohesion: 0.08
Nodes (95): allowed_expression_from_terms(), build_named_power_product(), cheap_same(), classify_reciprocal_conjugate_binomial(), combine_fraction_sum_once(), depends(), direct_double_angle_rewrite(), direct_identity_target_rewrite() (+87 more)

### Community 7 - "Community 7"
Cohesion: 0.06
Nodes (93): angle_text(), append_unique_float(), append_unique_solve_value(), concise_root_text(), constant_numeric(), dedupe_values(), drop_trailing_solution_line(), estimate_numeric_scan_samples() (+85 more)

### Community 8 - "Community 8"
Cohesion: 0.07
Nodes (76): add_param_coeff_maps(), add_transform_constant_candidate(), append_unique_value_node(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), constant_fit_preserve_named_trig(), depends_any(), div() (+68 more)

### Community 9 - "Community 9"
Cohesion: 0.13
Nodes (76): add(), angle_reduction_transforms(), build_named_power_term(), collect_same_arg_terms(), derive_cot_quadratic_expr(), direct_ratio_target_rewrite(), divide_terms_by_two_for_display(), equation_line() (+68 more)

### Community 10 - "Community 10"
Cohesion: 0.05
Nodes (35): check_quality(), ExprGrammar, has_working_steps(), main(), no_forbidden(), ProgramSpec, QualityChecker, Universal Test Generator - Generates random tests for ALL programs with comprehe (+27 more)

### Community 11 - "Community 11"
Cohesion: 0.15
Nodes (43): bridge_to_target(), common_denominator_step(), detail_trig_expansion(), equivalent(), finish_verbose_proof(), finish_verbose_proof_structured(), half_angle_expand(), half_sum_diff_args() (+35 more)

### Community 12 - "Community 12"
Cohesion: 0.13
Nodes (30): addq(), angle_to_degree(), degree_int(), degree_mod_360(), divq(), exact_num_value(), exact_pi_multiple(), exact_trig_lines() (+22 more)

### Community 13 - "Community 13"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 14 - "Community 14"
Cohesion: 0.12
Nodes (21): collect_angle_units(), collect_solve_angle_units(), compress_display_list(), contains_pi(), default_no_interval_span(), display_line_short(), equation_angle_mode(), interval_angle_mode() (+13 more)

### Community 15 - "Community 15"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 16 - "Community 16"
Cohesion: 1.0
Nodes (2): format_equation_human_readable(), split_coeff()

### Community 17 - "Community 17"
Cohesion: 1.0
Nodes (1): Verify derivative using SymPy.

### Community 18 - "Community 18"
Cohesion: 1.0
Nodes (1): Verify integral using SymPy.

### Community 19 - "Community 19"
Cohesion: 1.0
Nodes (1): Verify solution using SymPy.

### Community 20 - "Community 20"
Cohesion: 1.0
Nodes (1): Numerical derivative using central difference.

### Community 21 - "Community 21"
Cohesion: 1.0
Nodes (1): Verify using inverse operation.

### Community 22 - "Community 22"
Cohesion: 1.0
Nodes (1): Check if output has reasoning markers - LENIENT.

### Community 23 - "Community 23"
Cohesion: 1.0
Nodes (1): Check for forbidden snippets.

### Community 24 - "Community 24"
Cohesion: 1.0
Nodes (1): Check if output has step-by-step working - LENIENT.

### Community 25 - "Community 25"
Cohesion: 1.0
Nodes (1): Run all quality checks - LENIENT VERSION.

## Knowledge Gaps
- **70 isolated node(s):** `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.`, `Generate a random expression.`, `Generate a base expression (no recursion).` (+65 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 17`** (1 nodes): `Verify derivative using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 18`** (1 nodes): `Verify integral using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 19`** (1 nodes): `Verify solution using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 20`** (1 nodes): `Numerical derivative using central difference.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 21`** (1 nodes): `Verify using inverse operation.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 22`** (1 nodes): `Check if output has reasoning markers - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 23`** (1 nodes): `Check for forbidden snippets.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 24`** (1 nodes): `Check if output has step-by-step working - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 25`** (1 nodes): `Run all quality checks - LENIENT VERSION.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 6` to `Community 3`, `Community 7`, `Community 8`, `Community 9`, `Community 11`, `Community 12`, `Community 14`?**
  _High betweenness centrality (0.007) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 9` to `Community 3`, `Community 6`, `Community 7`, `Community 8`, `Community 11`, `Community 12`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
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