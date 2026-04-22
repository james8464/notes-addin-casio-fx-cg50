## graphify

This project has a graphify knowledge graph at graphify-out/.

Rules:
- Before answering architecture or codebase questions, read graphify-out/GRAPH_REPORT.md for god nodes and community structure
- If graphify-out/wiki/index.md exists, navigate it instead of reading raw files
- After modifying code files in this session, run `python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"` to keep the graph current

## Progress (2026-04-22)

### Session Update
1. **Consolidated shared helpers** - Moved common predicates, `same_by_sig`, `fn`, `neg`, reasoning-marker, and duplicate-answer compaction into `src/shared_helpers.py`.
2. **Cleaned output labels** - Replaced duplicate `Solution:`/`Final =` style conclusions with `Answer:` in algebra, trig, integration, and docs examples.
3. **Improved transform/rewrite robustness** - Algebra xform now handles rational trig target coefficient fitting; trig rewrite accepts equivalent allowed terms under format fuzz.
4. **Improved exam-style differentiation** - Reciprocal trig derivatives (`sec`, `cosec`, `cot`) now show conversion to sin/cos and quotient-rule working instead of shortcut rules.
5. **Compacted integration output** - CLI integration output is constrained to short method/formula/working/answer lines.
6. **Fixed domain solving** - Square-root domain restrictions now solve the displayed inequality when possible.

### Current Test Results
- `random 30`: 30/30 passed
- `random 100`: 100/100 passed
- `random 500`: 482/482 passed after per-run dedupe
- Syntax compile check: passed
- MicroPython target f-string token scan: 0
- Section header marker scan: 0

### Known Remaining Work
- Full `/random 9999` repeated three times was not run in this session.
- The 100-question university suite was not fully implemented as a new feature set in this cleanup pass.

### Key Files
- `src/shared_helpers.py` - shared predicates/builders/output helpers
- `src/Math/algebraProgram.py` - transform, solve, domain, answer formatting
- `src/Math/trigProgram.py` - rewrite/transform formatting and allowed-term robustness
- `src/Math/deriveProgram.py` - reciprocal trig quotient-rule working
- `src/Math/intProgram.py` - compact integration output
- `tests/run_tests.py` - quality gates updated for `Answer:` output and compact integration
