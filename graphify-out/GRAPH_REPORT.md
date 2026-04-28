# Graph Report - .  (2026-04-28)

## Corpus Check
- 33 files · ~336,605 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 2121 nodes · 9674 edges · 28 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 19 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 255 edges
2. `num()` - 242 edges
3. `CASIOApp` - 191 edges
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
- `Append a compact block for harness failures or verifier disagreements.` --uses--> `LLMManager`  [INFERRED]
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
Nodes (374): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+366 more)

### Community 1 - "Community 1"
Cohesion: 0.02
Nodes (100): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+92 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (278): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+270 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (129): append_unique_value_node(), _balance_parens(), begin_user_action(), best_proof_direction(), build_menu_pages(), cache_store(), clean_expr_text(), collect_angle_units() (+121 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (121): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+113 more)

### Community 5 - "Community 5"
Cohesion: 0.06
Nodes (124): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), as_rat_display(), begin_user_action() (+116 more)

### Community 6 - "Community 6"
Cohesion: 0.11
Nodes (94): add(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), collect_same_arg_terms(), collect_trig_argument_lower_symbols(), constant_numeric() (+86 more)

### Community 7 - "Community 7"
Cohesion: 0.07
Nodes (82): add_param_coeff_maps(), add_transform_constant_candidate(), addq(), angle_to_degree(), build_named_power_product(), build_named_power_term(), build_rewrite_allowed_info(), combine_fraction_sum_once() (+74 more)

### Community 8 - "Community 8"
Cohesion: 0.09
Nodes (74): best_solve_rewrite(), classify_solve_angle_arg(), depends(), direct_single_trig_info(), divide_terms_by_two_for_display(), eval_numeric(), expand_fraction(), expand_product() (+66 more)

### Community 9 - "Community 9"
Cohesion: 0.11
Nodes (69): add(), addq(), canonical_form(), _collect_symbols(), _convert_abs_pipes(), div(), divq(), equivalence_lines() (+61 more)

### Community 10 - "Community 10"
Cohesion: 0.09
Nodes (68): bridge_to_target(), cancel_fraction_common_factor_for_display(), detail_trig_expansion(), direct_expression_transform_lines(), direct_ratio_target_rewrite(), domain_restriction_identity_lines(), equivalent(), _equivalent_uncached() (+60 more)

### Community 11 - "Community 11"
Cohesion: 0.09
Nodes (60): angle_text(), append_unique_float(), append_unique_solve_value(), compact_lines(), concise_root_text(), dedupe_values(), eval_numeric_mode(), exact_angle_node_text() (+52 more)

### Community 12 - "Community 12"
Cohesion: 0.05
Nodes (55): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char(), is_const() (+47 more)

### Community 13 - "Community 13"
Cohesion: 0.05
Nodes (32): Records to include: harness failure, or LLM disagreement worth review., Keep the harness as the authoritative pass/fail signal.          LLM disagreemen, check_ollama_available(), get_ollama_models(), LLMCache, LLMManager, quick_verify(), Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol (+24 more)

### Community 14 - "Community 14"
Cohesion: 0.11
Nodes (46): allowed_expression_from_terms(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), direct_double_angle_rewrite(), direct_identity_target_rewrite(), extract_binomial(), factor_difference_of_squares_for_display() (+38 more)

### Community 15 - "Community 15"
Cohesion: 0.19
Nodes (26): build_menu_pages(), cache_set(), comp(), direct(), expand_vars(), has(), kids(), main() (+18 more)

### Community 16 - "Community 16"
Cohesion: 0.13
Nodes (14): _convert_abs_pipes(), _is_alpha_char(), _is_digit_char(), _is_name_char(), _is_name_start(), is_num(), is_one(), is_zero() (+6 more)

### Community 17 - "Community 17"
Cohesion: 0.18
Nodes (2): run_cli(), TransformRegressionTests

### Community 18 - "Community 18"
Cohesion: 0.23
Nodes (12): display_abs(), display_neg(), match_cosec_cot_double_proof(), match_reciprocal_minus_base_proof(), match_simple_trig_term(), match_simple_trig_term_norm(), needs_div_brackets(), ordered_sum_text() (+4 more)

### Community 19 - "Community 19"
Cohesion: 0.6
Nodes (4): format_equation_human_readable(), format_exam_working(), numbered_steps(), split_coeff()

### Community 20 - "Community 20"
Cohesion: 0.4
Nodes (5): apply_runtime_profile(), casio_hw_sim_from_env(), clear_engine_caches(), _force_low_memory_runtime(), shared_clear_all_caches()

### Community 21 - "Community 21"
Cohesion: 0.6
Nodes (4): check_one(), main(), Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchai, _read_header()

### Community 22 - "Community 22"
Cohesion: 0.7
Nodes (4): _bootstrap_mpy_mode(), main(), _run_cpython(), _run_mpy()

### Community 23 - "Community 23"
Cohesion: 0.83
Nodes (3): run(), _try_import(), _try_mpl()

### Community 24 - "Community 24"
Cohesion: 0.67
Nodes (3): numeric_eval(), Numeric evaluation for prove/show mode with degree support., Numeric evaluation for prove/show mode with degree support.

### Community 25 - "Community 25"
Cohesion: 0.67
Nodes (3): format_equation_human_readable(), Format an equation node into a human-readable string with clear operator precede, Format an equation node into a human-readable string with clear operator precede

### Community 26 - "Community 26"
Cohesion: 1.0
Nodes (0): 

### Community 27 - "Community 27"
Cohesion: 1.0
Nodes (0): 

## Knowledge Gaps
- **100 isolated node(s):** `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.`, `Simple TTL-based cache for LLM responses.`, `Create cache key; hash full prompt to avoid collision on long/ similar tails.` (+95 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 26`** (2 nodes): `trig.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 27`** (2 nodes): `main.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CASIOApp` connect `Community 1` to `Community 13`?**
  _High betweenness centrality (0.137) - this node is a cross-community bridge._
- **Why does `LLMManager` connect `Community 13` to `Community 1`?**
  _High betweenness centrality (0.032) - this node is a cross-community bridge._
- **Are the 3 inferred relationships involving `CASIOApp` (e.g. with `LLMManager` and `RuntimeSourceGuardTests`) actually correct?**
  _`CASIOApp` has 3 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Shared LLM Interface for CASIO Test Suite - PC ONLY.  This module connects to Ol`, `Check if Ollama is installed and a server is running.`, `Get list of available Ollama models.` to the rest of the system?**
  _100 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._