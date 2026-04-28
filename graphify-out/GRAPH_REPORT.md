# Graph Report - .  (2026-04-28)

## Corpus Check
- 33 files · ~332,840 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 2054 nodes · 9488 edges · 24 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 19 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 255 edges
2. `num()` - 241 edges
3. `CASIOApp` - 190 edges
4. `add()` - 153 edges
5. `mul()` - 135 edges
6. `flat()` - 134 edges
7. `sim()` - 112 edges
8. `same()` - 112 edges
9. `fn()` - 108 edges
10. `neg()` - 108 edges

## Surprising Connections (you probably didn't know these)
- `Reset the on-disk log at the start of a run; failures append while tests execute` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Append a compact block for failing or LLM-flagged tests (avoids a huge all-pass` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Keep the /random and /infinite status bar in sync (TUI only, not plain/CLI).` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Largest-remainder apportioning of n_total to len(weights) buckets (exam-style, j` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Call LLM for a reference answer; chance set by CASIO_LLM_GENERATION_CHANCE. Tags` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (363): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+355 more)

### Community 1 - "Community 1"
Cohesion: 0.02
Nodes (104): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+96 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (278): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+270 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (173): append_unique_value_node(), _balance_parens(), begin_user_action(), best_proof_direction(), best_solve_rewrite(), build_menu_pages(), cache_store(), classify_solve_angle_arg() (+165 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (121): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+113 more)

### Community 5 - "Community 5"
Cohesion: 0.07
Nodes (119): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+111 more)

### Community 6 - "Community 6"
Cohesion: 0.11
Nodes (104): add(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), build_named_power_term(), collect_same_arg_terms(), depends() (+96 more)

### Community 7 - "Community 7"
Cohesion: 0.06
Nodes (100): addq(), angle_to_degree(), build_named_power_product(), cheap_same(), classify_reciprocal_conjugate_binomial(), degree_int(), degree_mod_360(), direct_double_angle_rewrite() (+92 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (84): allowed_expression_from_terms(), bridge_to_target(), common_denominator_step(), detail_trig_expansion(), direct_identity_target_rewrite(), direct_ratio_target_rewrite(), equivalent(), expand_fraction() (+76 more)

### Community 9 - "Community 9"
Cohesion: 0.09
Nodes (63): angle_text(), append_unique_float(), append_unique_solve_value(), concise_root_text(), dedupe_values(), direct_single_trig_info(), eval_numeric(), eval_numeric_mode() (+55 more)

### Community 10 - "Community 10"
Cohesion: 0.05
Nodes (32): Records to include: harness failure, or LLM incorrect / needs review., Set record.status and record.passed from LLM-weighted final_verdict; update feat, check_ollama_available(), get_ollama_models(), LLMCache, LLMManager, quick_verify(), Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol (+24 more)

### Community 11 - "Community 11"
Cohesion: 0.16
Nodes (43): add(), addq(), div(), divq(), equivalent(), _extract_outer_and_trig(), _find_exp_reciprocal_case(), flat() (+35 more)

### Community 12 - "Community 12"
Cohesion: 0.06
Nodes (45): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const() (+37 more)

### Community 13 - "Community 13"
Cohesion: 0.13
Nodes (41): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), combine_fraction_sum_once(), constant_fit_preserve_named_trig(), depends_any() (+33 more)

### Community 14 - "Community 14"
Cohesion: 0.19
Nodes (26): build_menu_pages(), cache_set(), comp(), direct(), expand_vars(), has(), kids(), main() (+18 more)

### Community 15 - "Community 15"
Cohesion: 0.15
Nodes (8): _convert_abs_pipes(), is_num(), is_one(), is_zero(), neg(), normalize_input_text(), _previous_significant_char(), _should_open_abs_pipe()

### Community 16 - "Community 16"
Cohesion: 0.25
Nodes (2): run_cli(), TransformRegressionTests

### Community 17 - "Community 17"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 18 - "Community 18"
Cohesion: 0.4
Nodes (5): apply_runtime_profile(), casio_hw_sim_from_env(), clear_engine_caches(), _force_low_memory_runtime(), shared_clear_all_caches()

### Community 19 - "Community 19"
Cohesion: 0.6
Nodes (4): check_one(), main(), Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchai, _read_header()

### Community 20 - "Community 20"
Cohesion: 0.7
Nodes (4): _bootstrap_mpy_mode(), main(), _run_cpython(), _run_mpy()

### Community 21 - "Community 21"
Cohesion: 0.83
Nodes (3): run(), _try_import(), _try_mpl()

### Community 22 - "Community 22"
Cohesion: 1.0
Nodes (0): 

### Community 23 - "Community 23"
Cohesion: 1.0
Nodes (0): 

## Knowledge Gaps
- **79 isolated node(s):** `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.`, `Simple TTL-based cache for LLM responses.`, `Create cache key; hash full prompt to avoid collision on long/ similar tails.` (+74 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 22`** (2 nodes): `trig.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 23`** (2 nodes): `main.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CASIOApp` connect `Community 1` to `Community 10`?**
  _High betweenness centrality (0.141) - this node is a cross-community bridge._
- **Why does `LLMManager` connect `Community 10` to `Community 1`?**
  _High betweenness centrality (0.033) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `CASIOApp` (e.g. with `LLMManager` and `RuntimeSourceGuardTests`) actually correct?**
  _`CASIOApp` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.` to the rest of the system?**
  _79 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._