# Graph Report - .  (2026-04-30)

## Corpus Check
- 74 files · ~410,080 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 2615 nodes · 11317 edges · 28 communities detected
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
- `run()` --calls--> `exact_trig()`  [EXTRACTED]
  python/src/calc_files/trig.py → addin/src/modules/trig/trig.cpp
- `run()` --calls--> `split_csv()`  [EXTRACTED]
  python/src/calc_files/trig.py → addin/src/modules/trig/trig.cpp
- `run()` --calls--> `solve_simple_trig_eq()`  [EXTRACTED]
  python/src/calc_files/trig.py → addin/src/modules/trig/trig.cpp
- `run()` --calls--> `diff()`  [EXTRACTED]
  python/src/calc_files/derive.py → addin/src/modules/derive/derive.cpp
- `run()` --calls--> `split_csv()`  [EXTRACTED]
  python/src/calc_files/derive.py → addin/src/modules/derive/derive.cpp

## Communities

### Community 0 - "Community 0"
Cohesion: 0.02
Nodes (596): add(), add_param_coeff_maps(), add_transform_constant_candidate(), addq(), allowed_expression_from_terms(), angle_reduction_transforms(), angle_text(), angle_to_degree() (+588 more)

### Community 1 - "Community 1"
Cohesion: 0.02
Nodes (441): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+433 more)

### Community 2 - "Community 2"
Cohesion: 0.01
Nodes (142): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_compare_output_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker() (+134 more)

### Community 3 - "Community 3"
Cohesion: 0.04
Nodes (310): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_cyclic_parts(), auto_route_division(), auto_route_parts() (+302 more)

### Community 4 - "Community 4"
Cohesion: 0.05
Nodes (164): abs_term(), add(), addq(), all_neg_add(), _answer_text(), apply_runtime_profile(), as_rat(), as_rat_display() (+156 more)

### Community 5 - "Community 5"
Cohesion: 0.03
Nodes (118): add_poly(), as_int64(), is_square_i64(), is_zero(), mul_poly(), poly_of(), r_add(), r_div() (+110 more)

### Community 6 - "Community 6"
Cohesion: 0.04
Nodes (135): cache_store(), clear_all_caches(), enforce_total_cache_limit(), Store one cache value and trim gently when the small-device limit is hit., Keep a group of independent caches under one shared memory budget., Clear regular caches and nested per-name cache dictionaries., add(), addq() (+127 more)

### Community 7 - "Community 7"
Cohesion: 0.08
Nodes (85): add(), addq(), canonical_form(), _clean_work_expr(), _collect_symbols(), _convert_abs_pipes(), div(), divq() (+77 more)

### Community 8 - "Community 8"
Cohesion: 0.05
Nodes (2): run_cli(), TransformRegressionTests

### Community 9 - "Community 9"
Cohesion: 0.05
Nodes (63): casio_hw_sim_from_env(), cheap_same(), compact_duplicate_answer_lines(), compact_working_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), fn(), is_alpha_char() (+55 more)

### Community 10 - "Community 10"
Cohesion: 0.11
Nodes (20): cache_store(), compact_working_lines(), _convert_abs_pipes(), ensure_reasoning_marker(), _is_alpha_char(), _is_digit_char(), _is_name_char(), _is_name_start() (+12 more)

### Community 11 - "Community 11"
Cohesion: 0.23
Nodes (25): comp(), expand_vars(), has(), kids(), make1(), make_const(), make_var(), mk() (+17 more)

### Community 12 - "Community 12"
Cohesion: 0.22
Nodes (26): build_menu_pages(), cache_set(), comp(), direct(), expand_vars(), has(), kids(), main() (+18 more)

### Community 13 - "Community 13"
Cohesion: 0.25
Nodes (14): convert_rtf_to_txt(), _eval_newton_consistency(), _extract_answer_roots(), extract_expected_answer(), main(), map_to_program_input(), normalize(), normalize_compact() (+6 more)

### Community 14 - "Community 14"
Cohesion: 0.32
Nodes (11): extract_alg_roots_numeric(), extract_alg_solutions(), extract_answer_lines(), extract_answer_list_numeric(), main(), map_fixture_to_host(), normalize_compact(), Convert the python program stdin payload to our host CLI input.     Only handles (+3 more)

### Community 15 - "Community 15"
Cohesion: 0.28
Nodes (8): format_equation_human_readable(), format_exam_working(), numbered_steps(), Pull a leading numeric coefficient out of a multiplication node., Build a simple numbered block and make sure the final line says Answer., Prefix non-empty working lines with 1., 2., 3. for calculator display., Render tuple AST nodes into a compact exam-friendly string., split_coeff()

### Community 16 - "Community 16"
Cohesion: 0.6
Nodes (5): extract_answer(), main(), norm(), run_cpp(), run_python()

### Community 17 - "Community 17"
Cohesion: 0.6
Nodes (5): extract_answer(), main(), norm(), run_cpp(), run_python()

### Community 18 - "Community 18"
Cohesion: 0.7
Nodes (4): extract_exact_answer(), main(), run_cpp(), run_python()

### Community 19 - "Community 19"
Cohesion: 0.7
Nodes (4): _import_rtf_runner(), main(), _read_txt_lines(), _write_jsonl()

### Community 20 - "Community 20"
Cohesion: 0.6
Nodes (4): check_one(), main(), Verify compiled .mpy files match the Casio fx-CG50 / MicroPython v1.9.4 toolchai, _read_header()

### Community 21 - "Community 21"
Cohesion: 0.7
Nodes (4): _bootstrap_mpy_mode(), main(), _run_cpython(), _run_mpy()

### Community 22 - "Community 22"
Cohesion: 0.83
Nodes (3): main(), py_oracle(), run_host()

### Community 23 - "Community 23"
Cohesion: 0.83
Nodes (3): run(), _try_import(), _try_mpl()

### Community 24 - "Community 24"
Cohesion: 1.0
Nodes (2): main(), run()

### Community 25 - "Community 25"
Cohesion: 1.0
Nodes (2): main(), run_host()

### Community 26 - "Community 26"
Cohesion: 0.67
Nodes (3): format_equation_human_readable(), Format an equation node into a human-readable string with clear operator precede, Format an equation node into a human-readable string with clear operator precede

### Community 27 - "Community 27"
Cohesion: 0.67
Nodes (3): numeric_eval(), Numeric evaluation for prove/show mode with degree support., Numeric evaluation for prove/show mode with degree support.

## Knowledge Gaps
- **189 isolated node(s):** `Convert the python program stdin payload to our host CLI input.     Only handles`, `Arena`, `Quick numeric check: evaluate eq_expr at expected_x.     eq_expr should be an ex`, `Returns (script_relpath, stdin_payload) for program invocation.`, `Store one cache value and trim gently when the small-device limit is hit.` (+184 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CASIOApp` connect `Community 2` to `Community 8`?**
  _High betweenness centrality (0.115) - this node is a cross-community bridge._
- **Why does `TransformRegressionTests` connect `Community 8` to `Community 2`?**
  _High betweenness centrality (0.034) - this node is a cross-community bridge._
- **Why does `LLMManager` connect `Community 2` to `Community 8`?**
  _High betweenness centrality (0.032) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `CASIOApp` (e.g. with `LLMManager` and `RuntimeSourceGuardTests`) actually correct?**
  _`CASIOApp` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Convert the python program stdin payload to our host CLI input.     Only handles`, `Arena`, `Quick numeric check: evaluate eq_expr at expected_x.     eq_expr should be an ex` to the rest of the system?**
  _189 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Community 1` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._