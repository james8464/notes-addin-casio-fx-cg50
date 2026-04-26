# Graph Report - .  (2026-04-26)

## Corpus Check
- 25 files · ~182,670 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1881 nodes · 8608 edges · 21 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 7 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 255 edges
2. `num()` - 241 edges
3. `CASIOApp` - 173 edges
4. `add()` - 153 edges
5. `mul()` - 135 edges
6. `flat()` - 134 edges
7. `same()` - 111 edges
8. `fn()` - 107 edges
9. `neg()` - 107 edges
10. `sim()` - 101 edges

## Surprising Connections (you probably didn't know these)
- `Call LLM for a reference answer; chance set by CASIO_LLM_GENERATION_CHANCE. Tags` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `TestRecord` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `CaseSpec` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `RandomTestBatch` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `CASIOApp` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (335): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+327 more)

### Community 1 - "Community 1"
Cohesion: 0.02
Nodes (86): App, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker(), algebra_rewrite_checker() (+78 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (260): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+252 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (129): apply_runtime_profile(), _balance_parens(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), clean_expr_text(), clear_engine_caches() (+121 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (109): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+101 more)

### Community 6 - "Community 6"
Cohesion: 0.1
Nodes (107): add(), addq(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), build_named_power_term(), collect_same_arg_terms() (+99 more)

### Community 7 - "Community 7"
Cohesion: 0.07
Nodes (98): add_transform_constant_candidate(), allowed_expression_from_terms(), build_rewrite_allowed_info(), cache_store(), cancel_fraction_common_factor_for_display(), combine_fraction_sum_once(), direct_double_angle_rewrite(), direct_identity_target_rewrite() (+90 more)

### Community 8 - "Community 8"
Cohesion: 0.06
Nodes (98): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), classify_solve_angle_arg(), concise_root_text(), constant_numeric(), dedupe_values() (+90 more)

### Community 9 - "Community 9"
Cohesion: 0.09
Nodes (69): angle_to_degree(), bridge_to_target(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), degree_int(), degree_mod_360(), detail_trig_expansion() (+61 more)

### Community 10 - "Community 10"
Cohesion: 0.05
Nodes (35): Enum, RunState, TestStatus, check_ollama_available(), get_ollama_models(), LLMCache, LLMManager, quick_verify() (+27 more)

### Community 11 - "Community 11"
Cohesion: 0.06
Nodes (45): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const() (+37 more)

### Community 12 - "Community 12"
Cohesion: 0.09
Nodes (33): build_named_power_product(), display_abs(), display_neg(), factor_common_term_once(), factor_trig_power_pair_for_proof(), format_equation_human_readable(), half_angle_expr(), _is_cos_squared_term() (+25 more)

### Community 13 - "Community 13"
Cohesion: 0.19
Nodes (25): build_menu_pages(), cache_set(), comp(), direct(), expand_vars(), has(), kids(), mk() (+17 more)

### Community 14 - "Community 14"
Cohesion: 0.16
Nodes (23): add_param_coeff_maps(), collect_symbol_order(), collect_trig_argument_lower_symbols(), constant_fit_preserve_named_trig(), depends_any(), detect_template_params(), detect_transform_var(), extract_linear_param_expr() (+15 more)

### Community 15 - "Community 15"
Cohesion: 0.16
Nodes (4): is_num(), is_one(), is_zero(), neg()

### Community 16 - "Community 16"
Cohesion: 0.27
Nodes (2): run_cli(), TransformRegressionTests

### Community 17 - "Community 17"
Cohesion: 0.67
Nodes (5): _autorun_off_bootstrap(), _bootstrap_mpy_mode(), main(), _run_cpython(), _run_mpy()

### Community 18 - "Community 18"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 19 - "Community 19"
Cohesion: 0.6
Nodes (4): check_one(), main(), Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchai, _read_header()

### Community 20 - "Community 20"
Cohesion: 0.67
Nodes (3): main(), Run a single test and return pass/fail., run_test()

## Knowledge Gaps
- **75 isolated node(s):** `Run a single test and return pass/fail.`, `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.`, `Simple TTL-based cache for LLM responses.` (+70 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CASIOApp` connect `Community 1` to `Community 10`?**
  _High betweenness centrality (0.138) - this node is a cross-community bridge._
- **Why does `LLMManager` connect `Community 10` to `Community 1`?**
  _High betweenness centrality (0.028) - this node is a cross-community bridge._
- **What connects `Run a single test and return pass/fail.`, `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.` to the rest of the system?**
  _75 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Community 3` be split into smaller, more focused modules?**
  _Cohesion score 0.03 - nodes in this community are weakly interconnected._