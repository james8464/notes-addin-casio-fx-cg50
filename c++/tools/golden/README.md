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
python3 c++/tools/golden/check_madasmaths_full_audit.py
python3 c++/tools/golden/check_madasmaths_standard_topics_audit.py
python3 c++/tools/golden/check_madasmaths_standard_question_corpus.py
python3 c++/tools/golden/check_madasmaths_standard_manual_cases.py
python3 c++/tools/golden/check_manual_question_triage_notes.py
```

`check_madasmaths_standard_question_corpus.py` verifies generated corpus rows and still host-checks manual cases whose PDFs are outside that generated corpus.

`manual_question_triage_notes.jsonl` is for broad notes only. Do not use it as
the executable audit source.

`exact_calculator_input_queue.jsonl` is the fast audit intake queue. Each row
must store literal calculator text a user could paste into the program, not host
flags or paraphrased prompts. For multi-line exam answers, record all mark-
scheme working lines needed for full-credit judgement beside the input. Validate
with `check_exact_calculator_input_queue.py`, then batch run with
`run_exact_calculator_input_queue.py --strict-markers`. Add one
`coverage:"complete"` source marker after all visible question, mark-scheme and
worked-solution pages for a PDF have been manually reviewed; download coverage
counts those exact-queue complete markers directly.
