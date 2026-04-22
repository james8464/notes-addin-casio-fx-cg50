# Graph Report - .  (2026-04-22)

## Corpus Check
- 22 files · ~751,263 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1820 nodes · 8718 edges · 27 communities detected
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
10. `num()` - 102 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 7_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 10_
- `sim()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 5_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 8_
- `make_add()` --calls--> `num()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 7 → community 10_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (334): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+326 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (263): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_poly_quad_exp() (+255 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (88): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+80 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (140): apply_runtime_profile(), _balance_parens(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store(), classify_solve_angle_arg() (+132 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (116): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+108 more)

### Community 5 - "Community 5"
Cohesion: 0.1
Nodes (111): add(), angle_reduction_transforms(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), direct_expression_transform_lines(), direct_ratio_target_rewrite(), div() (+103 more)

### Community 6 - "Community 6"
Cohesion: 0.07
Nodes (107): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+99 more)

### Community 7 - "Community 7"
Cohesion: 0.07
Nodes (106): addq(), allowed_expression_from_terms(), build_named_power_product(), build_named_power_term(), cheap_same(), classify_reciprocal_conjugate_binomial(), direct_double_angle_rewrite(), direct_identity_target_rewrite() (+98 more)

### Community 8 - "Community 8"
Cohesion: 0.07
Nodes (89): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), compact_lines(), concise_root_text(), constant_numeric(), dedupe_values() (+81 more)

### Community 9 - "Community 9"
Cohesion: 0.05
Nodes (35): check_quality(), ExprGrammar, has_working_steps(), main(), no_forbidden(), ProgramSpec, QualityChecker, Universal Test Generator - Generates random tests for ALL programs with comprehe (+27 more)

### Community 10 - "Community 10"
Cohesion: 0.09
Nodes (55): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), depends_any(), expand_embedded_small() (+47 more)

### Community 11 - "Community 11"
Cohesion: 0.1
Nodes (48): angle_to_degree(), bridge_to_target(), common_denominator_step(), degree_int(), degree_mod_360(), detail_trig_expansion(), exact_trig_lines(), exact_trig_value() (+40 more)

### Community 12 - "Community 12"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 13 - "Community 13"
Cohesion: 0.13
Nodes (18): cheap_same(), is_neg(), is_neg_one(), is_num(), is_num_token_start(), is_one(), is_sym(), is_zero() (+10 more)

### Community 14 - "Community 14"
Cohesion: 0.33
Nodes (0): 

### Community 15 - "Community 15"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 16 - "Community 16"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 17 - "Community 17"
Cohesion: 1.0
Nodes (0): 

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
- **78 isolated node(s):** `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.`, `Generate a random expression.`, `Generate a base expression (no recursion).` (+73 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 17`** (2 nodes): `test_20_questions.py`, `run_test()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
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

- **Why does `sim()` connect `Community 5` to `Community 3`, `Community 7`, `Community 8`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 7` to `Community 3`, `Community 5`, `Community 8`, `Community 10`, `Community 11`?**
  _High betweenness centrality (0.005) - this node is a cross-community bridge._
- **What connects `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.` to the rest of the system?**
  _78 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._