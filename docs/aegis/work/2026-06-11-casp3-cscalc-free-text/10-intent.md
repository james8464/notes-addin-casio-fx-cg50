# CASP3/CSCALC Free Text Hardening

Requested outcome: make CASP3 and CSCALC behave more like working calculators that accept exam-style free text, show method lines, and avoid brittle exact-command-only routes.

Scope: Paper 3 mechanics/statistics command parsing, AQA CS calculation/Boolean parsing, host tests, source-built add-ins in `calculator_files/`.

Non-goals: claim arbitrary mathematical completeness, change CAS pure maths behavior, menu/status/UI edits.

Baseline refs: `tools/p3_engine.cpp`, `tools/cscalc_engine.cpp`, `tests/check_p3_engine.py`, `tests/check_cscalc_engine.py`, `./compile`.
