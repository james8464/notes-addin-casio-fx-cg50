# Graph Report - .  (2026-04-25)

## Corpus Check
- 21 files · ~224,509 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1832 nodes · 8508 edges · 17 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 6 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 255 edges
2. `num()` - 241 edges
3. `CASIOApp` - 165 edges
4. `add()` - 153 edges
5. `mul()` - 135 edges
6. `flat()` - 134 edges
7. `same()` - 111 edges
8. `fn()` - 107 edges
9. `neg()` - 107 edges
10. `num()` - 101 edges

## Surprising Connections (you probably didn't know these)
- `TestStatus` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `RunState` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `TestRecord` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `CaseSpec` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `RandomTestBatch` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (333): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+325 more)

### Community 1 - "Community 1"
Cohesion: 0.02
Nodes (119): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+111 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (261): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+253 more)

### Community 3 - "Community 3"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 4 - "Community 4"
Cohesion: 0.03
Nodes (110): angle_to_degree(), append_unique_value_node(), apply_runtime_profile(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store(), classify_solve_angle_arg() (+102 more)

### Community 5 - "Community 5"
Cohesion: 0.1
Nodes (109): add(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_named_power_term(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr() (+101 more)

### Community 6 - "Community 6"
Cohesion: 0.07
Nodes (105): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+97 more)

### Community 7 - "Community 7"
Cohesion: 0.06
Nodes (94): allowed_expression_from_terms(), build_named_power_product(), combine_fraction_sum_once(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), direct_single_trig_info(), display_abs(), divq() (+86 more)

### Community 8 - "Community 8"
Cohesion: 0.07
Nodes (86): angle_text(), append_unique_float(), append_unique_solve_value(), concise_root_text(), constant_numeric(), dedupe_values(), display_neg(), div() (+78 more)

### Community 9 - "Community 9"
Cohesion: 0.07
Nodes (69): _balance_parens(), begin_user_action(), collect_symbols(), compact_lines(), detect_transform_var(), direct_expression_transform_lines(), display_target_text(), domain_restriction_identity_lines() (+61 more)

### Community 10 - "Community 10"
Cohesion: 0.11
Nodes (51): bridge_to_target(), build_known_value_branch(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), detail_trig_expansion(), expand_fraction(), expand_small() (+43 more)

### Community 11 - "Community 11"
Cohesion: 0.06
Nodes (43): cheap_same(), compact_duplicate_answer_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const(), is_digit_char() (+35 more)

### Community 12 - "Community 12"
Cohesion: 0.11
Nodes (38): add_param_coeff_maps(), add_transform_constant_candidate(), addq(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), constant_fit_preserve_named_trig(), depends_any() (+30 more)

### Community 13 - "Community 13"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 14 - "Community 14"
Cohesion: 0.27
Nodes (2): run_cli(), TransformRegressionTests

### Community 15 - "Community 15"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 16 - "Community 16"
Cohesion: 0.67
Nodes (3): main(), Run a single test and return pass/fail., run_test()

## Knowledge Gaps
- **71 isolated node(s):** `Run a single test and return pass/fail.`, `Shared LLM Interface for CASIO Test Suite.  Provides Ollama-based verification f`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.`, `Simple TTL-based cache for LLM responses.` (+66 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What connects `Run a single test and return pass/fail.`, `Shared LLM Interface for CASIO Test Suite.  Provides Ollama-based verification f`, `Check if Ollama is installed and a server is running.` to the rest of the system?**
  _71 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Community 4` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 5` be split into smaller, more focused modules?**
  _Cohesion score 0.1 - nodes in this community are weakly interconnected._