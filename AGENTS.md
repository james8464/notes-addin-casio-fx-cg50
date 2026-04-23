## Progress (2026-04-23)

### Session Update
1. **LLM Integration** - Added Ollama interface with caching, model selection, and verification
2. **Three-way verification** - Added SymPy, CASIO, and LLM verification with consensus logic
3. **TestRecord enhancements** - Added sympy_verdict, llm_verdict, llm_explanation fields
4. **Fixed Ollama command** - Changed from `ollama generate` to `ollama run`

### Current Test Results
- `random 30`: 30/30 passed
- `random 100`: 100/100 passed
- `tests/test_madasmaths.py`: 38/38 passed
- Syntax compile check: passed

### Key Files
- `src/shared_llm.py` - Ollama interface with caching, model selection
- `src/shared_helpers.py` - shared predicates/builders
- `tests/run_tests.py` - Test runner with LLM integration

## graphify

This project has a graphify knowledge graph at graphify-out/.

Rules:
- Before answering architecture/codebase questions, read graphify-out/GRAPH_REPORT.md
- After modifying code, run: `python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"`