## Golden Fixtures

This folder contains frozen C++ host fixtures for parser, algebra, trig,
calculus, SUVAT, and regression checks. The main runner no longer depends on the
old Python programs.

### Files

- `expr_smoke.txt`: expression parser/formatter cases with expected output.
- `rtf_cases*.jsonl`: frozen exam-bank snapshots kept for C++ host diffing.
- `compare_*.py`: C++ host checks only.

Run all checks:

```bash
python3 c++/tools/run_tests_cpp.py
python3 run_tests.py
```

Useful focused gates:

```bash
python3 run_tests.py tui
python3 c++/tools/golden/check_edexcel_paper1_downloads.py
python3 c++/tools/golden/check_edexcel_paper2_downloads.py
```
