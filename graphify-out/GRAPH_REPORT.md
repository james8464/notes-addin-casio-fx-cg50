# Graph Report - .  (2026-04-10)

## Corpus Check
- 21 files · ~141,087 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1452 nodes · 7355 edges · 15 communities detected
- Extraction: 100% EXTRACTED · 0% INFERRED · 0% AMBIGUOUS
- Token cost: 0 input · 0 output

## God Nodes (most connected - your core abstractions)
1. `sim()` - 239 edges
2. `num()` - 221 edges
3. `add()` - 137 edges
4. `mul()` - 123 edges
5. `flat()` - 123 edges
6. `same()` - 102 edges
7. `fn()` - 98 edges
8. `neg()` - 98 edges
9. `num()` - 95 edges
10. `CASIOApp` - 89 edges

## Surprising Connections (you probably didn't know these)
- `flat()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 6 → community 8_
- `sig()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 6 → community 10_
- `split_num_factor()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 6 → community 12_
- `sim()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 6 → community 3_
- `full_simplify()` --calls--> `cache_store()`  [EXTRACTED]
  src/Math/trigProgram.py → src/Math/trigProgram.py  _Bridges community 6 → community 9_

## Communities

### Community 0 - "Algebra Module"
Cohesion: 0.02
Nodes (290): add(), add_term_texts(), addq(), algebra_factor_text(), algebra_mode_3_lines(), algebra_mode_3_text(), algebra_mode_6_lines(), algebra_mode_6_text() (+282 more)

### Community 1 - "Integration Module"
Cohesion: 0.05
Nodes (228): add(), addq(), all_neg_add(), apply_runtime_profile(), auto_integral_routes(), auto_route_division(), auto_route_parts(), auto_route_reverse() (+220 more)

### Community 2 - "Test Suite"
Cohesion: 0.04
Nodes (38): App, Enum, algebra_comp_checker(), algebra_compare_checker(), algebra_complete_square_checker(), algebra_expand_checker(), algebra_inverse_checker(), algebra_rewrite_checker() (+30 more)

### Community 3 - "Trig Solve Functions"
Cohesion: 0.11
Nodes (116): add(), angle_reduction_transforms(), collect_same_arg_terms(), depends(), derive_cot_quadratic_expr(), div(), divide_terms_by_two_for_display(), equation_line() (+108 more)

### Community 4 - "SUVAT Physics"
Cohesion: 0.06
Nodes (107): add(), addq(), all_neg_add(), begin_user_action(), _build_a(), _build_a2(), _build_a3(), _build_a4() (+99 more)

### Community 5 - "Trig Value Solving"
Cohesion: 0.06
Nodes (108): angle_text(), append_unique_solve_value(), begin_user_action(), compact_lines(), concise_root_text(), constant_numeric(), dedupe_values(), direct_expression_transform_lines() (+100 more)

### Community 6 - "Trig Core Functions"
Cohesion: 0.04
Nodes (90): append_unique_float(), apply_runtime_profile(), best_proof_direction(), best_solve_rewrite(), build_named_power_product(), build_named_power_term(), cache_store(), classify_solve_angle_arg() (+82 more)

### Community 7 - "Derivative Module"
Cohesion: 0.09
Nodes (87): abs_term(), add(), addq(), all_neg_add(), apply_runtime_profile(), as_rat(), begin_user_action(), _cache_get() (+79 more)

### Community 8 - "Trig Match/Factor"
Cohesion: 0.09
Nodes (66): cheap_same(), classify_reciprocal_conjugate_binomial(), combine_fraction_sum_once(), common_denominator_step(), direct_double_angle_rewrite(), direct_single_trig_info(), display_abs(), divq() (+58 more)

### Community 9 - "Boolean Logic"
Cohesion: 0.11
Nodes (58): bridge_to_target(), detail_trig_expansion(), equivalent(), _equivalent_uncached(), expand_safe_trig_tree(), expand_small(), finish_verbose_proof(), format_rewrite_lines() (+50 more)

### Community 10 - "Math Utils"
Cohesion: 0.1
Nodes (45): add_param_coeff_maps(), add_transform_constant_candidate(), build_rewrite_allowed_info(), cancel_fraction_common_factor_for_display(), collect_symbol_order(), collect_trig_argument_lower_symbols(), constant_fit_preserve_named_trig(), depends_any() (+37 more)

### Community 11 - "Data Structures"
Cohesion: 0.24
Nodes (21): comp(), direct(), expand_vars(), has(), kids(), mk(), normalise(), parse() (+13 more)

### Community 12 - "Helper Functions"
Cohesion: 0.19
Nodes (21): addq(), angle_to_degree(), degree_int(), degree_mod_360(), exact_num_value(), exact_trig_lines(), exact_trig_value(), expand_powered_monomial() (+13 more)

### Community 13 - "Parser/CLI"
Cohesion: 0.19
Nodes (14): collect_angle_units(), compress_display_list(), contains_pi(), display_line_short(), is_alpha_char(), is_digit_char(), is_name_char(), is_name_start() (+6 more)

### Community 14 - "UI/Display"
Cohesion: 0.7
Nodes (4): get_solution_count(), has_solutions(), run_solve(), run_tests()

## Knowledge Gaps
- **23 isolated node(s):** `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.`, `Estimate significant figures from input text.`, `Detect keyword presets in input text. Returns dict of variable overrides.` (+18 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `sim()` connect `Trig Solve Functions` to `Trig Value Solving`, `Trig Core Functions`, `Trig Match/Factor`, `Boolean Logic`, `Math Utils`, `Helper Functions`, `Parser/CLI`?**
  _High betweenness centrality (0.008) - this node is a cross-community bridge._
- **Why does `num()` connect `Trig Solve Functions` to `Trig Value Solving`, `Trig Core Functions`, `Trig Match/Factor`, `Boolean Logic`, `Math Utils`, `Helper Functions`?**
  _High betweenness centrality (0.007) - this node is a cross-community bridge._
- **What connects `Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder)`, `Convert a numeric AST node to float. Returns None if not purely numeric.`, `Format a float to a clean decimal string.` to the rest of the system?**
  _23 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Algebra Module` be split into smaller, more focused modules?**
  _Cohesion score 0.02 - nodes in this community are weakly interconnected._
- **Should `Integration Module` be split into smaller, more focused modules?**
  _Cohesion score 0.05 - nodes in this community are weakly interconnected._
- **Should `Test Suite` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Trig Solve Functions` be split into smaller, more focused modules?**
  _Cohesion score 0.11 - nodes in this community are weakly interconnected._