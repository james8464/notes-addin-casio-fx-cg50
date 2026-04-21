## graphify

This project has a graphify knowledge graph at graphify-out/.

Rules:
- Before answering architecture or codebase questions, read graphify-out/GRAPH_REPORT.md for god nodes and community structure
- If graphify-out/wiki/index.md exists, navigate it instead of reading raw files
- After modifying code files in this session, run `python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"` to keep the graph current

## Progress (2026-04-20)

### Test Results (Final)
- chaos 10000: 9756/10000 passed (98%)
- chaos 5000: 4871/5000 passed (97%)
- chaos 3000: 2924/3000 passed (97%)
- weakness 1000: 975/1000 passed (98%)
- random 300: 294/300 passed (98%)
- hard 500: 486/500 passed (97%)

### Test Throughput
- ~16-20 tests/second with 64 workers
- ~10k tests in ~8-10 minutes
- High-count tests (5000+) run effectively

### Enhancements Made
1. **EQ1= input format** - Added support for `EQ1=` prefix in deriveProgram.py and intProgram.py
2. **Exam reasoning markers** - Expanded _EXAM_REASONING_MARKERS with 25+ new markers
3. **Test generation** - Added generate_weakness_cases() targeting nested_power, deep_fraction, zero_edge, etc.
4. **Trig solvers** - Added solve_cosec_pow4_minus_cot_pow4(), solve_sec_pow4_minus_tan_pow4(), solve_sin_pow_n_plus_cos_pow_n()

### Known Failure Categories (~2-3%)
- SUVAT edge cases (zero acceleration/time physics)
- Trig integration (sin^n * cos^n power reduction)
- Format fuzz edge cases

### Key Files
- src/Math/deriveProgram.py - EQ1= parsing
- src/Math/intProgram.py - EQ1= support
- src/Math/trigProgram.py - New trig solvers
- tests/run_tests.py - Expanded reasoning markers
- tests/universal_test_gen.py - Weakness case generation
