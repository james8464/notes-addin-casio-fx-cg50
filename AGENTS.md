## Progress (2026-04-22)

### Session Update
1. **Consolidated shared helpers** - Moved common predicates, `same_by_sig`, `fn`, `neg`, reasoning-marker, and duplicate-answer compaction into `src/shared_helpers.py`.
2. **Cleaned output labels** - Replaced duplicate `Solution:`/`Final =` style conclusions with `Answer:` in algebra, trig, integration, and docs examples.
3. **Improved transform/rewrite robustness** - Algebra xform now handles rational trig target coefficient fitting; trig rewrite accepts equivalent allowed terms under format fuzz.
4. **Improved exam-style differentiation** - Reciprocal trig derivatives (`sec`, `cosec`, `cot`) now show conversion to sin/cos and quotient-rule working instead of shortcut rules.
5. **Compacted integration output** - CLI integration output is constrained to short method/formula/working/answer lines.
6. **Fixed domain solving** - Square-root domain restrictions now solve the displayed inequality when possible.
7. **Hardened random format fuzzing** - Fuzzed CLI cases now recalculate answer checkers from the fuzzed input, and plain-mode failures include input/check/output snippets.
8. **Fixed fuzzed trig/algebra regressions** - Algebra half-angle rewrites preserve scaled arguments, product identities compare correctly under `sec^2(A)-tan^2(A)`, and trig identity checks use the project parser before numeric fallback.

### Current Test Results
- `random 30`: 30/30 passed
- `random 100`: 100/100 passed
- `random 500`: 482/482 passed after per-run dedupe
- `random 1000`: 956/956 passed after per-run dedupe
- `random 9999` consistency pass 1: 9468/9468 passed after per-run dedupe
- `random 9999` consistency pass 2: 9464/9464 passed after per-run dedupe
- `random 9999` consistency pass 3: 9466/9466 passed after per-run dedupe
- `tests/test_madasmaths.py`: 38/38 passed
- Syntax compile check: passed
- MicroPython target f-string token scan: 0
- MicroPython target walrus/match/type-annotation AST scan: 0
- Section header marker scan: 0

### Known Remaining Work
- No current automated random-suite blocker is known.
- The explicit 100-question prompt list is not present as a standalone checked-in test file; current validation uses `tests/test_madasmaths.py` plus the generated random/chaos suites.
- LLM three-way verification: consensus algorithm (SymPy + CASIO + LLM) not yet implemented - currently runs all three but treats LLM as separate; may need to adjust final pass/fail based on consensus

## LLM Integration (2026-04-23)

### Implemented
- `src/shared_llm.py` - Ollama interface with caching, model selection, verification
- `tests/run_tests.py` - Added LLM commands (`/llm`, `/llm status`, `/llm disable`, `/llm select`)
- `tests/run_tests.py` - Three-way verification in `run_case_specs()` - LLM verification runs in parallel after CASIO tests
- `TestRecord` - Added `llm_verdict` and `llm_explanation` fields
- `render_summary` - Shows LLM verification stats with cache hit rate

### Architecture
- `LLMManager` class in shared_llm.py handles Ollama communication
- `LLMCache` provides TTL-based response caching (3600s TTL, 1000 max entries)
- ThreadPoolExecutor runs LLM verification in parallel (max 4 workers)
- Tests run CASIO first, then batch LLM verification afterward

### Next Steps for Consensus
- Implement consensus logic: pass if >=2 of (SymPy, CASIO, LLM) agree
- May need to update `add_test` or `run_case_specs` to apply consensus verdict
- Consider adding LLM verdict indicator in test output (e.g., "● LLM✓")

### Completed (2026-04-23)
- `TestRecord` - Added `sympy_verdict` field for SymPy verification results
- `run_case_specs()` - Now tracks SymPy verdict during test execution via `sympy_expressions_equivalent()`
- Three-way consensus ready: CASIO passed, SymPy equivalent → pass; if LLM available and disagrees, flag for review

### Key Files
- `src/shared_helpers.py` - shared predicates/builders/output helpers
- `src/shared_llm.py` - Ollama interface with caching, model selection, verification
- `src/Math/algebraProgram.py` - transform, solve, domain, answer formatting
- `src/Math/trigProgram.py` - rewrite/transform formatting and allowed-term robustness
- `src/Math/deriveProgram.py` - reciprocal trig quotient-rule working
- `src/Math/intProgram.py` - compact integration output
- `tests/run_tests.py` - quality gates updated for `Answer:` output, compact integration, and LLM integration
- `tests/test_madasmaths.py` - MadAsMaths regression coverage

## graphify

This project has a graphify knowledge graph at graphify-out/.

Rules:
- Before answering architecture or codebase questions, read graphify-out/GRAPH_REPORT.md for god nodes and community structure
- If graphify-out/wiki/index.md exists, navigate it instead of reading raw files
- After modifying code files in this session, run `python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"` to keep the graph current
