# Graph Report - .  (2026-04-26)

## Corpus Check
- 26 files · ~182,838 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1881 nodes · 8602 edges · 24 communities detected
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
Cohesion: 0.04
Nodes (160): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), best_solve_rewrite(), build_menu_pages(), build_named_power_product(), classify_reciprocal_conjugate_binomial() (+152 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (111): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+103 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (109): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+101 more)

### Community 6 - "Community 6"
Cohesion: 0.1
Nodes (106): add(), addq(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), build_named_power_term(), collect_same_arg_terms() (+98 more)

### Community 7 - "Community 7"
Cohesion: 0.07
Nodes (83): add_param_coeff_maps(), add_transform_constant_candidate(), allowed_expression_from_terms(), build_rewrite_allowed_info(), cache_store(), cancel_fraction_common_factor_for_display(), combine_fraction_sum_once(), constant_fit_preserve_named_trig() (+75 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (69): cheap_same(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), direct_single_trig_info(), display_abs(), display_neg(), divq(), exact_pi_multiple() (+61 more)

### Community 9 - "Community 9"
Cohesion: 0.1
Nodes (61): bridge_to_target(), common_denominator_step(), detail_trig_expansion(), direct_ratio_target_rewrite(), domain_restriction_identity_lines(), equivalent(), _equivalent_uncached(), expand_fraction() (+53 more)

### Community 10 - "Community 10"
Cohesion: 0.05
Nodes (35): Enum, RunState, TestStatus, check_ollama_available(), get_ollama_models(), LLMCache, LLMManager, quick_verify() (+27 more)

### Community 11 - "Community 11"
Cohesion: 0.06
Nodes (45): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const() (+37 more)

### Community 12 - "Community 12"
Cohesion: 0.07
Nodes (46): _balance_parens(), begin_user_action(), best_proof_direction(), clean_expr_text(), collect_symbols(), collect_trig_argument_lower_symbols(), compress_display_list(), display_line_short() (+38 more)

### Community 13 - "Community 13"
Cohesion: 0.2
Nodes (24): build_menu_pages(), comp(), direct(), expand_vars(), has(), kids(), mk(), normalise() (+16 more)

### Community 14 - "Community 14"
Cohesion: 0.18
Nodes (18): depends(), extract_polynomial_symbol(), extract_polynomial_trig(), is_int_num(), match_nonzero_reciprocal_factor(), merge_tan_sub_base(), ratio_family_transforms(), reciprocal_family_transforms() (+10 more)

### Community 15 - "Community 15"
Cohesion: 0.16
Nodes (4): is_num(), is_one(), is_zero(), neg()

### Community 16 - "Community 16"
Cohesion: 0.27
Nodes (2): run_cli(), TransformRegressionTests

### Community 17 - "Community 17"
Cohesion: 0.33
Nodes (11): angle_to_degree(), degree_int(), degree_mod_360(), exact_trig_lines(), exact_trig_value(), match_shift_side(), prove_shift_compare(), quadrant_of_degree() (+3 more)

### Community 18 - "Community 18"
Cohesion: 0.67
Nodes (5): _autorun_off_bootstrap(), _bootstrap_mpy_mode(), main(), _run_cpython(), _run_mpy()

### Community 19 - "Community 19"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 20 - "Community 20"
Cohesion: 0.6
Nodes (4): check_one(), main(), Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchai, _read_header()

### Community 21 - "Community 21"
Cohesion: 0.67
Nodes (3): main(), Run a single test and return pass/fail., run_test()

### Community 22 - "Community 22"
Cohesion: 0.67
Nodes (3): apply_runtime_profile(), clear_engine_caches(), _force_low_memory_runtime()

### Community 23 - "Community 23"
Cohesion: 1.0
Nodes (0): 

## Knowledge Gaps
- **75 isolated node(s):** `Run a single test and return pass/fail.`, `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.`, `Simple TTL-based cache for LLM responses.` (+70 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 23`** (2 nodes): `dedupe_intprogram_blocks.py`, `main()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CASIOApp` connect `Community 1` to `Community 10`?**
  _High betweenness centrality (0.137) - this node is a cross-community bridge._
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
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._