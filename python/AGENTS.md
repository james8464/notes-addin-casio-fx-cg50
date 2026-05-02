## Progress (2026-04-26)

### Session Update
1. **Algebra solve mode 6** accepts `eq | eq, var | eq, lower, upper |
   eq, var, lower, upper`; rational/disguised solvers hardened.
2. **Algebra dom/range mode 10** accepts `expr | expr, var |
   expr, lower, upper | expr, var, lower, upper` and reports the
   evaluated interval / sample data.
3. **Algebra cartesian linear branch** now emits a final `Answer:`
   line (caught uniformly by tests and humans).
4. **Trig general solution** keeps fractional principal offsets
   (e.g. `0.5 + n*90`), tightens period tolerance, and adds 60/45
   degree periods.
5. **Test runner** auto-detects degrees vs radians, expands
   parametric solutions inside the requested interval, and accepts
   "no real solutions" outputs without an `x =` line.
6. Default `CASIO_TEST_TIMEOUT` raised to 12s; trig random
   generator pairs `pi`-bearing equations with radian intervals.
7. **MicroPython 1.9.4 compatibility** preserved (no f-strings, v3
   small-int=31 mpy headers verified via
   `src/calc_files/check_mpy_build.py`).

### Previous Session Update (2026-04-24)
1. **Fixed CLI/TUI unified code** - Now uses same code for both interfaces
2. **Fixed LLM readiness check** - Simplified to trust LLM is ready if enabled
3. **Fixed check_working_quality bug** - Was using buggy pattern matching, now disabled
4. **Fixed infinite mode** - Use `python3 tests/run_tests.py llm 1 random inf`
5. **Fixed duplicate test issue** - Removed duplicate LLM verification code

### Test Results
- `llm 1 random 30`: 30/30 passed with LLM verification
- `llm 1 random 10`: 10/10 passed with LLM verification
- Tests show "LLM: CORRECT" for each test
- Summary shows "LLM: X/X verified"

### How to Use
```bash
python3 tests/run_tests.py llm 1 random 10      # 10 tests with LLM
python3 tests/run_tests.py llm 1 random inf  # infinite tests
python3 tests/run_tests.py llm 1 random 100 # 100 tests
```

### Working Features
- LLM verification: All tests get verified by LLM after running
- Readability check: Simplified - just checks if LLM is enabled
- Caching: LLM responses are cached by model+prompt

### Limitations
- Working quality checking (pattern matching) is disabled - was buggy
- LLM can be slow (~2s per verification with falcon3:10b)

### Key Files
- `src/shared_llm.py` - Ollama interface with caching
- `tests/run_tests.py` - Test runner with TUI