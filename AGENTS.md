## graphify

This project has a graphify knowledge graph at graphify-out/.

Rules:
- Before answering architecture or codebase questions, read graphify-out/GRAPH_REPORT.md for god nodes and community structure
- If graphify-out/wiki/index.md exists, navigate it instead of reading raw files
- After modifying code files in this session, run `python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"` to keep the graph current

## Progress (2026-04-21)

### Today's Work (2026-04-23)
1. **Made working out exam-style** - Simplified verbose output to give exam marks:
   - "No exact symbolic solution found. Using numerical scan with detailed working:" → "Let f(x) = ... Find where f(x) = 0 by scanning..."
   - "Tried all exact solvers. Using polynomial method" → "Rewrite using sin^2 + cos^2 = 1 identity"

2. **Tested radical equation solver** - `x + 3*sqrt(x) = 10` returns "Needs poly support" (solver exists but not parsing correctly)

### Runtime Optimizations Added
1. **Total cache memory enforcement** - Added `_TOTAL_CACHE_LIMIT` (16384) and `_enforce_total_cache_limit()` in trigProgram.py to prevent memory exhaustion when multiple caches compete for heap
2. **Recursion depth limiting** - Added `_SOLVE_REWRITE_MAX_DEPTH` (12) to `best_solve_rewrite()` to prevent stack explosion with deeply nested expressions
3. **Parentheses edge case handling** - Enhanced `_balance_parens()` to handle early close parens and empty string returns
4. **Batch processing for ultra-large tests** - Added batch_size and skip_quality flags to `run_case_specs()` for counts >5000 with total >10000

### Test Throughput
- ~16-20 tests/second with 64 workers
- ~10k tests in ~8-10 minutes
- High-count tests (5000+) run effectively
- Large tests now auto-skip quality checks to reduce overhead
- ETA now displayed during and after test runs

### University-Level Questions Added (from A-Level/STEP/University exams)
- Hidden quadratics: x^4 - 13x^2 + 36 = 0 type (substitution)
- Trig identities: sec^4 - tan^4 = 1 + 2tan^2
- Trig solving: tan^2 + sec^2 + 5sec = 2 in 0-360°
- Differentiation: product+chain rule combinations
- Integration: parts twice, partial fractions
- SUVAT: projectile motion + edge cases

### Test Results (Prior Run)
- chaos 10000: 9756/10000 passed (98%)
- chaos 5000: 4871/5000 passed (97%)
- chaos 3000: 2924/3000 passed (97%)
- weakness 1000: 975/1000 passed (98%)
- random 300: 294/300 passed (98%)
- hard 500: 486/500 passed (97%)

### Known Failure Categories (~2-3%)
- SUVAT edge cases (zero acceleration/time physics)
- Trig integration (sin^n * cos^n power reduction)
- Format fuzz edge cases

### Key Files
- src/Math/trigProgram.py - Cache memory enforcement, recursion depth, balance_parens
- src/Math/algebraProgram.py - Cache bounded eviction
- tests/run_tests.py - Batch processing, university case generation
