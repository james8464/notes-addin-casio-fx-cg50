# Graph Report - .  (2026-04-30)

## Corpus Check
- 33 files · ~363,239 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 2403 nodes · 10825 edges · 27 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS · INFERRED: 37 edges (avg confidence: 0.5)
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 263 edges
2. `num()` - 250 edges
3. `CASIOApp` - 195 edges
4. `add()` - 157 edges
5. `sim()` - 149 edges
6. `mul()` - 140 edges
7. `flat()` - 137 edges
8. `same()` - 120 edges
9. `neg()` - 115 edges
10. `num()` - 113 edges

## Surprising Connections (you probably didn't know these)
- `TransformRegressionTests` --uses--> `TestStatus`  [INFERRED]
  python/tests/test_specific_regressions.py → python/tests/run_tests.py
- `TransformRegressionTests` --uses--> `TestRecord`  [INFERRED]
  python/tests/test_specific_regressions.py → python/tests/run_tests.py
- `TransformRegressionTests` --uses--> `CASIOApp`  [INFERRED]
  python/tests/test_specific_regressions.py → python/tests/run_tests.py
- `Move between random-run lifecycle states in one controlled place.` --uses--> `LLMManager`  [INFERRED]
  python/tests/run_tests.py → python/src/shared_llm.py
- `Records to include: harness failure, or LLM disagreement worth review.` --uses--> `LLMManager`  [INFERRED]
  python/tests/run_tests.py → python/src/shared_llm.py

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (441): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+433 more)

### Community 1 - "Community 1"
Cohesion: 0.01
Nodes (142): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+134 more)

### Community 2 - "Community 2"
Cohesion: 0.04
Nodes (310): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+302 more)

### Community 3 - "Community 3"
Cohesion: 0.03
Nodes (183): angle_text(), append_unique_float(), append_unique_solve_value(), append_unique_value_node(), apply_runtime_profile(), _balance_parens(), begin_user_action(), best_proof_direction() (+175 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (164): abs_term(), add(), addq(), all_neg_add(), _answer_text(), apply_runtime_profile(), as_rat(), as_rat_display() (+156 more)

### Community 5 - "Community 5"
Cohesion: 0.04
Nodes (135): cache_store(), clear_all_caches(), enforce_total_cache_limit(), Store one cache value and trim gently when the small-device limit is hit., Keep a group of independent caches under one shared memory budget., Clear regular caches and nested per-name cache dictionaries., add(), addq() (+127 more)

### Community 6 - "Community 6"
Cohesion: 0.1
Nodes (116): add(), angle_reduction_transforms(), branch_target_value(), build_known_trig_value_branches(), build_known_value_branch(), build_named_power_term(), collect_same_arg_terms(), depends() (+108 more)

### Community 7 - "Community 7"
Cohesion: 0.08
Nodes (85): add(), addq(), canonical_form(), _clean_work_expr(), _collect_symbols(), _convert_abs_pipes(), div(), divq() (+77 more)

### Community 8 - "Community 8"
Cohesion: 0.07
Nodes (82): build_named_power_product(), cheap_same(), classify_reciprocal_conjugate_binomial(), common_denominator_step(), direct_double_angle_rewrite(), direct_single_trig_info(), display_abs(), extract_binomial() (+74 more)

### Community 9 - "Community 9"
Cohesion: 0.05
Nodes (2): run_cli(), TransformRegressionTests

### Community 10 - "Community 10"
Cohesion: 0.05
Nodes (63): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), compact_working_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char() (+55 more)

### Community 11 - "Community 11"
Cohesion: 0.08
Nodes (61): append_identity_difference_working(), best_solve_rewrite(), domain_restriction_identity_lines(), equation_has_trig_content(), expand_embedded_small(), expand_fraction(), expand_product(), expand_safe_trig_tree() (+53 more)

### Community 12 - "Community 12"
Cohesion: 0.1
Nodes (58): allowed_expression_from_terms(), bridge_to_target(), detail_trig_expansion(), direct_identity_target_rewrite(), direct_ratio_target_rewrite(), direct_trig_family_line(), equivalent(), _equivalent_uncached() (+50 more)

### Community 13 - "Community 13"
Cohesion: 0.09
Nodes (45): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), collect_trig_argument_lower_symbols(), combine_fraction_sum_once(), constant_fit_preserve_named_trig() (+37 more)

### Community 14 - "Community 14"
Cohesion: 0.19
Nodes (26): build_menu_pages(), cache_set(), comp(), direct(), expand_vars(), has(), kids(), main() (+18 more)

### Community 15 - "Community 15"
Cohesion: 0.11
Nodes (20): cache_store(), compact_working_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), _is_alpha_char(), _is_digit_char(), _is_name_char(), _is_name_start() (+12 more)

### Community 16 - "Community 16"
Cohesion: 0.15
Nodes (27): addq(), angle_to_degree(), degree_int(), degree_mod_360(), divq(), exact_num_value(), exact_pi_multiple(), exact_trig_lines() (+19 more)

### Community 17 - "Community 17"
Cohesion: 0.13
Nodes (24): cache_store(), _enforce_total_cache_limit(), factor_common_term_for_proof(), function_names_of(), _function_names_uncached(), kind_names_of(), _kind_names_uncached(), proof_complexity_score() (+16 more)

### Community 18 - "Community 18"
Cohesion: 0.25
Nodes (14): convert_rtf_to_txt(), _eval_newton_consistency(), _extract_answer_roots(), extract_expected_answer(), main(), map_to_program_input(), normalize(), normalize_compact() (+6 more)

### Community 19 - "Community 19"
Cohesion: 0.28
Nodes (8): format_equation_human_readable(), format_exam_working(), numbered_steps(), Pull a leading numeric coefficient out of a multiplication node., Build a simple numbered block and make sure the final line says Answer., Prefix non-empty working lines with 1., 2., 3. for calculator display., Render tuple AST nodes into a compact exam-friendly string., split_coeff()

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
- **187 isolated node(s):** `Quick numeric check: evaluate eq_expr at expected_x.     eq_expr should be an ex`, `Returns (script_relpath, stdin_payload) for program invocation.`, `Store one cache value and trim gently when the small-device limit is hit.`, `Keep a group of independent caches under one shared memory budget.`, `Clear regular caches and nested per-name cache dictionaries.` (+182 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 25`** (2 nodes): `trig.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 26`** (2 nodes): `main.py`, `run()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CASIOApp` connect `Community 1` to `Community 9`?**
  _High betweenness centrality (0.125) - this node is a cross-community bridge._
- **Why does `TransformRegressionTests` connect `Community 9` to `Community 1`?**
  _High betweenness centrality (0.037) - this node is a cross-community bridge._
- **Why does `LLMManager` connect `Community 1` to `Community 9`?**
  _High betweenness centrality (0.035) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `CASIOApp` (e.g. with `LLMManager` and `RuntimeSourceGuardTests`) actually correct?**
  _`CASIOApp` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Quick numeric check: evaluate eq_expr at expected_x.     eq_expr should be an ex`, `Returns (script_relpath, stdin_payload) for program invocation.`, `Store one cache value and trim gently when the small-device limit is hit.` to the rest of the system?**
  _187 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.01 - nodes in this community are weakly interconnected._