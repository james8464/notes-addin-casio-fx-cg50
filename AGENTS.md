## Progress (2026-04-23)

### Session Update
1. **LLM Integration** - Added Ollama interface with caching, model selection, and verification
2. **Three-way verification** - Added SymPy, CASIO, and LLM verification with consensus logic
3. **TestRecord enhancements** - Added sympy_verdict, llm_verdict, llm_explanation fields
4. **Fixed Ollama command** - Changed from `ollama generate` to `ollama run`
5. **Optimized prompts** - Shortened system prompt for faster LLM responses
6. **Added fastest_model()** - Prefers smaller/faster models (qwen3.5:9b, qwen2.5:14b, gemma4:e4b)
7. **Improved response parsing** - Better handling of reasoning chains (Thinking, etc.)
8. **Performance optimizations** - Skip LLM for passing tests (sample 5%), increase workers to 8
9. **API-based Ollama** - Using HTTP API instead of CLI for faster response extraction

### Test Results
- `random 30`: 30/30 passed
- `random 100`: 100/100 passed
- `tests/test_madasmaths.py`: 38/38 passed
- Syntax compile check: passed

### LLM Verified Working Cases
- sin(30)=0.5 → CORRECT ✓
- cos(0)=1 → CORRECT ✓
- Expand (x+1)^2 = x^2+2x+1 → CORRECT ✓
- Identity sin^2+cos^2=1 → CORRECT ✓

### Working Quality Checking (Limitation Found)
- LLM is NOT reliably detecting bad working (skipped steps, "hence" without justification)
- Both good and bad working get marked as CORRECT
- Root cause: Reasoning models (qwen3.5) are too lenient with working quality evaluation
- Current behavior: LLM verifies ANSWER correctness, not working quality
- Future work: May need stricter prompt engineering or different model

### Performance Optimizations
- Skip LLM for 95% of passing tests (only sample 5%)
- Always verify failed tests
- Dynamic worker count: min(8, active_workers, tests//20)
- Cache TTL: 1 hour

### Known Issues
- LLM ~20s per verification on this machine
- qwen3.5:9b works but reasoning chains need special parsing
- Working quality checking NOT reliable - see above

### Key Files
- `src/shared_llm.py` - Ollama interface with caching, model selection
- `src/shared_helpers.py` - shared predicates/builders
- `tests/run_tests.py` - Test runner with LLM integration

## graphify

This project has a graphify knowledge graph at graphify-out/.

Rules:
- Before answering architecture/codebase questions, read graphify-out/GRAPH_REPORT.md
- After modifying code, run: `python3 -c "from graphify.watch import _rebuild_code; from pathlib import Path; _rebuild_code(Path('.'))"`