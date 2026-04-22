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

### Key Files
- `src/shared_helpers.py` - shared predicates/builders/output helpers
- `src/Math/algebraProgram.py` - transform, solve, domain, answer formatting
- `src/Math/trigProgram.py` - rewrite/transform formatting and allowed-term robustness
- `src/Math/deriveProgram.py` - reciprocal trig quotient-rule working
- `src/Math/intProgram.py` - compact integration output
- `tests/run_tests.py` - quality gates updated for `Answer:` output and compact integration
- `tests/test_madasmaths.py` - MadAsMaths regression coverage
