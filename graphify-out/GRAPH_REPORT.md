# Graph Report - /Users/james/Developer/Python/CASIO  (2026-04-26)

## Corpus Check
- 25 files · ~186,923 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1974 nodes · 9250 edges · 20 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 13 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 255 edges
2. `num()` - 241 edges
3. `CASIOApp` - 183 edges
4. `add()` - 153 edges
5. `mul()` - 135 edges
6. `flat()` - 134 edges
7. `sim()` - 112 edges
8. `same()` - 112 edges
9. `fn()` - 108 edges
10. `neg()` - 108 edges

## Surprising Connections (you probably didn't know these)
- `Records to include: harness failure, or LLM incorrect / needs review.` --uses--> `LLMManager`  [INFERRED]
  /Users/james/Developer/Python/CASIO/tests/run_tests.py → src/shared_llm.py
- `Set record.status and record.passed from LLM-weighted final_verdict; update feat` --uses--> `LLMManager`  [INFERRED]
  /Users/james/Developer/Python/CASIO/tests/run_tests.py → src/shared_llm.py
- `Keep the /random and /infinite status bar in sync (TUI only, not plain/CLI).` --uses--> `LLMManager`  [INFERRED]
  /Users/james/Developer/Python/CASIO/tests/run_tests.py → src/shared_llm.py
- `Call LLM for a reference answer; chance set by CASIO_LLM_GENERATION_CHANCE. Tags` --uses--> `LLMManager`  [INFERRED]
  /Users/james/Developer/Python/CASIO/tests/run_tests.py → src/shared_llm.py
- `TestStatus` --uses--> `LLMManager`  [INFERRED]
  /Users/james/Developer/Python/CASIO/tests/run_tests.py → src/shared_llm.py

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (363): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+355 more)

### Community 1 - "Community 1"
Cohesion: 0.02
Nodes (129): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+121 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (274): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+266 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (133): append_unique_value_node(), _balance_parens(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store(), clean_expr_text() (+125 more)

### Community 4 - "Community 4"
Cohesion: 0.06
Nodes (136): allowed_expression_from_terms(), bridge_to_target(), build_named_power_product(), cancel_fraction_common_factor_for_display(), cheap_same(), classify_reciprocal_conjugate_binomial(), combine_fraction_sum_once(), common_denominator_step() (+128 more)

### Community 5 - "Community 5"
Cohesion: 0.05
Nodes (121): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+113 more)

### Community 6 - "Community 6"
Cohesion: 0.07
Nodes (119): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+111 more)

### Community 7 - "Community 7"
Cohesion: 0.06
Nodes (106): angle_text(), append_unique_float(), append_unique_solve_value(), compact_lines(), concise_root_text(), constant_numeric(), dedupe_values(), detect_transform_var() (+98 more)

### Community 8 - "Community 8"
Cohesion: 0.12
Nodes (97): add(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), build_named_power_term(), collect_same_arg_terms(), derive_cot_quadratic_expr() (+89 more)

### Community 9 - "Community 9"
Cohesion: 0.08
Nodes (48): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), constant_fit_preserve_named_trig(), depends_any(), extract_linear_param_expr(), extract_template_allowed_terms_raw(), extract_template_basis_terms() (+40 more)

### Community 10 - "Community 10"
Cohesion: 0.06
Nodes (45): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const() (+37 more)

### Community 11 - "Community 11"
Cohesion: 0.14
Nodes (27): addq(), angle_to_degree(), degree_int(), degree_mod_360(), divq(), exact_num_value(), exact_pi_multiple(), exact_trig_lines() (+19 more)

### Community 12 - "Community 12"
Cohesion: 0.19
Nodes (25): build_menu_pages(), cache_set(), comp(), direct(), expand_vars(), has(), kids(), mk() (+17 more)

### Community 13 - "Community 13"
Cohesion: 0.23
Nodes (18): classify_solve_angle_arg(), depends(), direct_single_trig_info(), extract_polynomial_symbol(), extract_polynomial_trig(), extract_quadratic_trig(), factor_common_term_once(), is_int_num() (+10 more)

### Community 14 - "Community 14"
Cohesion: 0.15
Nodes (8): _convert_abs_pipes(), is_num(), is_one(), is_zero(), neg(), normalize_input_text(), _previous_significant_char(), _should_open_abs_pipe()

### Community 15 - "Community 15"
Cohesion: 0.27
Nodes (2): run_cli(), TransformRegressionTests

### Community 16 - "Community 16"
Cohesion: 0.67
Nodes (5): _autorun_off_bootstrap(), _bootstrap_mpy_mode(), main(), _run_cpython(), _run_mpy()

### Community 17 - "Community 17"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 18 - "Community 18"
Cohesion: 0.4
Nodes (5): apply_runtime_profile(), casio_hw_sim_from_env(), clear_engine_caches(), _force_low_memory_runtime(), shared_clear_all_caches()

### Community 19 - "Community 19"
Cohesion: 0.6
Nodes (4): check_one(), main(), Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchai, _read_header()

## Knowledge Gaps
- **75 isolated node(s):** `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.`, `Simple TTL-based cache for LLM responses.`, `Create cache key; hash full prompt to avoid collision on long/ similar tails.` (+70 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Are the 2 inferred relationships involving `CASIOApp` (e.g. with `LLMManager` and `HarnessLlmStatusTests`) actually correct?**
  _`CASIOApp` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.` to the rest of the system?**
  _75 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._
- **Should `Community 4` be split into smaller, more focused modules?**
  _Cohesion score 0.06 - nodes in this community are weakly interconnected._