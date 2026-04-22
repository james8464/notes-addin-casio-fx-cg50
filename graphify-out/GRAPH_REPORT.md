# Graph Report - .  (2026-04-22)

## Corpus Check
- 22 files · ~756,466 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1825 nodes · 8357 edges · 28 communities detected
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
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 4_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 11_
- `sim()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 7_
- `equivalent()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 10_
- `final_angle_text()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 3 → community 8_

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (332): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+324 more)

### Community 1 - "Community 1"
Cohesion: 0.04
Nodes (261): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+253 more)

### Community 2 - "Community 2"
Cohesion: 0.03
Nodes (88): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+80 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (123): append_unique_value_node(), apply_runtime_profile(), _balance_parens(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store() (+115 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (122): addq(), angle_to_degree(), build_named_power_product(), build_named_power_term(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), depends() (+114 more)

### Community 5 - "Community 5"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 6 - "Community 6"
Cohesion: 0.07
Nodes (105): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+97 more)

### Community 7 - "Community 7"
Cohesion: 0.1
Nodes (98): add(), angle_reduction_transforms(), collect_same_arg_terms(), derive_cot_quadratic_expr(), div(), divide_terms_by_two_for_display(), equation_line(), exact_constant_candidates() (+90 more)

### Community 8 - "Community 8"
Cohesion: 0.07
Nodes (93): angle_text(), append_unique_float(), append_unique_solve_value(), compact_lines(), concise_root_text(), constant_numeric(), dedupe_values(), display_neg() (+85 more)

### Community 9 - "Community 9"
Cohesion: 0.05
Nodes (35): check_quality(), ExprGrammar, has_working_steps(), main(), no_forbidden(), ProgramSpec, QualityChecker, Universal Test Generator - Generates random tests for ALL programs with comprehe (+27 more)

### Community 10 - "Community 10"
Cohesion: 0.1
Nodes (59): allowed_expression_from_terms(), bridge_to_target(), cancel_fraction_common_factor_for_display(), detail_trig_expansion(), direct_expression_transform_lines(), direct_ratio_target_rewrite(), equivalent(), expand_safe_trig_tree() (+51 more)

### Community 11 - "Community 11"
Cohesion: 0.13
Nodes (41): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), collect_symbol_order(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params() (+33 more)

### Community 12 - "Community 12"
Cohesion: 0.07
Nodes (38): cheap_same(), compact_duplicate_answer_lines(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const(), is_digit_char(), is_int_num() (+30 more)

### Community 13 - "Community 13"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 14 - "Community 14"
Cohesion: 0.36
Nodes (9): degree_int(), degree_mod_360(), exact_trig_lines(), exact_trig_value(), prove_shift_compare(), quadrant_of_degree(), reference_degree(), shift_placeholder() (+1 more)

### Community 15 - "Community 15"
Cohesion: 0.33
Nodes (0): 

### Community 16 - "Community 16"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 17 - "Community 17"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

### Community 18 - "Community 18"
Cohesion: 1.0
Nodes (0): 

### Community 19 - "Community 19"
Cohesion: 1.0
Nodes (1): Verify derivative using SymPy.

### Community 20 - "Community 20"
Cohesion: 1.0
Nodes (1): Verify integral using SymPy.

### Community 21 - "Community 21"
Cohesion: 1.0
Nodes (1): Verify solution using SymPy.

### Community 22 - "Community 22"
Cohesion: 1.0
Nodes (1): Numerical derivative using central difference.

### Community 23 - "Community 23"
Cohesion: 1.0
Nodes (1): Verify using inverse operation.

### Community 24 - "Community 24"
Cohesion: 1.0
Nodes (1): Check if output has reasoning markers - LENIENT.

### Community 25 - "Community 25"
Cohesion: 1.0
Nodes (1): Check for forbidden snippets.

### Community 26 - "Community 26"
Cohesion: 1.0
Nodes (1): Check if output has step-by-step working - LENIENT.

### Community 27 - "Community 27"
Cohesion: 1.0
Nodes (1): Run all quality checks - LENIENT VERSION.

## Knowledge Gaps
- **80 isolated node(s):** `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.`, `Generate a random expression.`, `Generate a base expression (no recursion).` (+75 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 18`** (2 nodes): `test_20_questions.py`, `run_test()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 19`** (1 nodes): `Verify derivative using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 20`** (1 nodes): `Verify integral using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 21`** (1 nodes): `Verify solution using SymPy.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 22`** (1 nodes): `Numerical derivative using central difference.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 23`** (1 nodes): `Verify using inverse operation.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 24`** (1 nodes): `Check if output has reasoning markers - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 25`** (1 nodes): `Check for forbidden snippets.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 26`** (1 nodes): `Check if output has step-by-step working - LENIENT.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 27`** (1 nodes): `Run all quality checks - LENIENT VERSION.`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Community 7` to `Community 3`, `Community 4`, `Community 8`, `Community 10`, `Community 11`, `Community 14`?**
  _High betweenness centrality (0.006) - this node is a cross-community bridge._
- **Why does `num()` connect `Community 4` to `Community 3`, `Community 7`, `Community 8`, `Community 10`, `Community 11`, `Community 14`?**
  _High betweenness centrality (0.005) - this node is a cross-community bridge._
- **What connects `Universal Test Generator - Generates random tests for ALL programs with comprehe`, `Recursive grammar for generating random mathematical expressions.`, `Set scaling parameters.` to the rest of the system?**
  _80 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._