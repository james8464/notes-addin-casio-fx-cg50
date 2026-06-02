# Edexcel Paper Test Sweep

Requested outcome: inspect the Edexcel A Level Maths past-paper set under `/Volumes/VM/Edexcel A Level Maths past papers`, mine useful pure-maths test inputs, run them against the shared working engine, and fix real gaps in the most general way possible.

Scope: pure maths working-output routes, pattern recognition, symbolic manipulation, build-size safety, and calculator build verification.

Non-goals: menu/UI edits, RUNMAT changes, stats/probability support, checker rewrites, or hiding hard cases from the test set.

Baseline refs: upstream KhiCAS-compatible menu/build state, current `cascas_working.cc`, existing OCR reports under `tests/reports/edexcel_papers/`, and user reports that menus are currently working.

Risk hints: whole-object linker pruning can break calculator startup or menus; code-size reductions must avoid menu/UI paths.
