# Graph Report - .  (2026-04-29)

## Corpus Check
- 32 files · ~347,900 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 2299 nodes · 10194 edges · 27 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 33 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 262 edges
2. `num()` - 248 edges
3. `CASIOApp` - 195 edges
4. `add()` - 155 edges
5. `mul()` - 138 edges
6. `flat()` - 136 edges
7. `sim()` - 121 edges
8. `same()` - 119 edges
9. `neg()` - 113 edges
10. `fn()` - 111 edges

## Surprising Connections (you probably didn't know these)
- `Move between random-run lifecycle states in one controlled place.` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Records to include: harness failure, or LLM disagreement worth review.` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Reset the on-disk log at the start of a run; failures append while tests execute` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Append a compact block for harness failures or verifier disagreements.` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py
- `Keep the harness as the authoritative pass/fail signal.          LLM disagreemen` --uses--> `LLMManager`  [INFERRED]
  tests/run_tests.py → src/shared_llm.py

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (404): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+396 more)

### Community 1 - "Community 1"
Cohesion: 0.01
Nodes (146): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+138 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (299): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+291 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (156): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), best_solve_rewrite(), build_menu_pages(), classify_solve_angle_arg(), collect_angle_units() (+148 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (146): abs_term(), add(), addq(), all_neg_add(), _answer_text(), apply_runtime_profile(), as_rat(), as_rat_display() (+138 more)

### Community 5 - "Community 5"
Cohesion: 0.04
Nodes (135): cache_store(), clear_all_caches(), enforce_total_cache_limit(), Store one cache value and trim gently when the small-device limit is hit., Keep a group of independent caches under one shared memory budget., Clear regular caches and nested per-name cache dictionaries., add(), addq() (+127 more)

### Community 6 - "Community 6"
Cohesion: 0.06
Nodes (122): addq(), angle_to_degree(), build_named_power_product(), build_named_power_term(), cheap_same(), classify_reciprocal_conjugate_binomial(), combine_fraction_sum_once(), common_denominator_step() (+114 more)

### Community 7 - "Community 7"
Cohesion: 0.1
Nodes (113): add(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr() (+105 more)

### Community 8 - "Community 8"
Cohesion: 0.08
Nodes (85): add(), addq(), canonical_form(), _clean_work_expr(), _collect_symbols(), _convert_abs_pipes(), div(), divq() (+77 more)

### Community 9 - "Community 9"
Cohesion: 0.1
Nodes (71): append_identity_difference_working(), bridge_to_target(), detail_trig_expansion(), detect_transform_var(), direct_expression_transform_lines(), direct_ratio_target_rewrite(), equivalent(), _equivalent_uncached() (+63 more)

### Community 10 - "Community 10"
Cohesion: 0.05
Nodes (59): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), compact_working_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char() (+51 more)

### Community 11 - "Community 11"
Cohesion: 0.06
Nodes (56): _balance_parens(), begin_user_action(), best_proof_direction(), clean_expr_text(), collect_symbols(), collect_trig_argument_lower_symbols(), display_target_text(), equation_has_trig_content() (+48 more)

### Community 12 - "Community 12"
Cohesion: 0.08
Nodes (2): run_cli(), TransformRegressionTests

### Community 13 - "Community 13"
Cohesion: 0.11
Nodes (39): add_param_coeff_maps(), add_transform_constant_candidate(), allowed_expression_from_terms(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), constant_fit_preserve_named_trig(), depends_any() (+31 more)

### Community 14 - "Community 14"
Cohesion: 0.19
Nodes (26): build_menu_pages(), cache_set(), comp(), direct(), expand_vars(), has(), kids(), main() (+18 more)

### Community 15 - "Community 15"
Cohesion: 0.11
Nodes (20): cache_store(), compact_working_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), _is_alpha_char(), _is_digit_char(), _is_name_char(), _is_name_start() (+12 more)

### Community 16 - "Community 16"
Cohesion: 0.12
Nodes (26): cache_store(), _enforce_total_cache_limit(), factor_common_term_for_proof(), function_names_of(), _function_names_uncached(), kind_names_of(), _kind_names_uncached(), proof_complexity_score() (+18 more)

### Community 17 - "Community 17"
Cohesion: 0.28
Nodes (8): format_equation_human_readable(), format_exam_working(), numbered_steps(), Pull a leading numeric coefficient out of a multiplication node., Build a simple numbered block and make sure the final line says Answer., Prefix non-empty working lines with 1., 2., 3. for calculator display., Render tuple AST nodes into a compact exam-friendly string., split_coeff()

### Community 18 - "Community 18"
Cohesion: 0.67
Nodes (6): degree_int(), degree_mod_360(), exact_trig_lines(), exact_trig_value(), quadrant_of_degree(), reference_degree()

### Community 19 - "Community 19"
Cohesion: 0.4
Nodes (5): apply_runtime_profile(), casio_hw_sim_from_env(), clear_engine_caches(), _force_low_memory_runtime(), shared_clear_all_caches()

### Community 20 - "Community 20"
Cohesion: 0.6
Nodes (4): check_one(), main(), Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchai, _read_header()

### Community 21 - "Community 21"
Cohesion: 0.7
Nodes (4): _bootstrap_mpy_mode(), main(), _run_cpython(), _run_mpy()

### Community 22 - "Community 22"
Cohesion: 0.83
Nodes (3): run(), _try_import(), _try_mpl()

### Community 23 - "Community 23"
Cohesion: 0.67
Nodes (3): format_equation_human_readable(), Format an equation node into a human-readable string with clear operator precede, Format an equation node into a human-readable string with clear operator precede

### Community 24 - "Community 24"
Cohesion: 0.67
Nodes (3): numeric_eval(), Numeric evaluation for prove/show mode with degree support., Numeric evaluation for prove/show mode with degree support.

### Community 25 - "Community 25"
Cohesion: 1.0
Nodes (0): 

### Community 26 - "Community 26"
Cohesion: 1.0
Nodes (0): 

## Knowledge Gaps
- **181 isolated node(s):** `Store one cache value and trim gently when the small-device limit is hit.`, `Keep a group of independent caches under one shared memory budget.`, `Clear regular caches and nested per-name cache dictionaries.`, `Prefix non-empty working lines with 1., 2., 3. for calculator display.`, `Build a simple numbered block and make sure the final line says Answer.` (+176 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 25`** (2 nodes): `trig.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 26`** (2 nodes): `main.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CASIOApp` connect `Community 1` to `Community 12`?**
  _High betweenness centrality (0.132) - this node is a cross-community bridge._
- **Why does `TransformRegressionTests` connect `Community 12` to `Community 1`?**
  _High betweenness centrality (0.026) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `CASIOApp` (e.g. with `LLMManager` and `RuntimeSourceGuardTests`) actually correct?**
  _`CASIOApp` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Store one cache value and trim gently when the small-device limit is hit.`, `Keep a group of independent caches under one shared memory budget.`, `Clear regular caches and nested per-name cache dictionaries.` to the rest of the system?**
  _181 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.01 - nodes in this community are weakly interconnected._
- **Should `Community 2` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._