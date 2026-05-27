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
python3 c++/tools/golden/check_online_paper_corpus_inventory.py
python3 c++/tools/golden/check_online_paper_manual_cases.py
python3 c++/tools/golden/check_edexcel_public_paper_corpus.py
python3 c++/tools/golden/check_edexcel_question_audit_coverage.py
python3 c++/tools/golden/check_edexcel_paper1_downloads.py
python3 c++/tools/golden/check_edexcel_paper2_downloads.py
python3 c++/tools/golden/check_edexcel_paper31_downloads.py
python3 c++/tools/golden/check_edexcel_paper32_downloads.py
python3 c++/tools/golden/check_madasmaths_full_audit.py
python3 c++/tools/golden/check_madasmaths_standard_topics_audit.py
python3 c++/tools/golden/check_madasmaths_standard_question_corpus.py
python3 c++/tools/golden/check_madasmaths_standard_manual_cases.py
python3 c++/tools/golden/check_manual_question_triage_notes.py
```

`check_madasmaths_standard_question_corpus.py` verifies generated corpus rows and still host-checks manual cases whose PDFs are outside that generated corpus.

External source setup:

```bash
python3 c++/tools/golden/download_a_level_audit_sources.py --scope all --clean --force
python3 c++/tools/golden/download_online_paper_corpus.py --clean --force
python3 c++/tools/golden/check_a_level_source_downloads.py
python3 c++/tools/golden/check_online_paper_corpus_inventory.py
python3 c++/tools/golden/check_edexcel_public_paper_corpus.py
python3 c++/tools/golden/check_edexcel_question_audit_coverage.py
python3 c++/tools/golden/render_audit_pdf_pages.py --format jpeg ~/Downloads/"Edexcel A Level Maths past papers" --first 2
```

Audit flow graph: `c++/tools/golden/a_level_audit_graph.md`.
Manual cross-source audit tracker: `c++/tools/golden/a_level_audit_tracker.jsonl`.
Parallel manual triage prompt: `c++/tools/golden/manual_question_triage_prompt.md`.
Append-only triage notes: `c++/tools/golden/manual_question_triage_notes.jsonl`.

`download_a_level_audit_sources.py` defaults to normal Edexcel A-level Maths
(`--scope edexcel-9ma0`). Use `--clean` to remove only managed download folders
before redownloading. It writes PDFs outside git:

- `~/Downloads/Edexcel A Level Maths past papers`
- `~/Downloads/Edexcel A Level Maths support materials`

It also writes the ignored manifest/report under
`c++/tests/reports/a_level_source_downloads`. Pearson 2025 9MA0 URLs are probed
but not treated as required until Pearson serves public PDFs.

Use `--scope all` only when deliberately re-auditing broad MadAsMaths packs:

- `~/Downloads/MadAsMaths standard topics`
- `~/Downloads/MadAsMaths A-level booklets`
- `~/Downloads/MadAsMaths papers`

`check_madasmaths_full_audit.py` renders local MP2 A-Z papers/solutions from
`~/Downloads/MadAsMaths papers` into ignored report images, writes the JSONL
ledger, and runs all curated MP2 host markscheme cases. Use `--no-render` for
the lightweight CI gate.

`check_madasmaths_standard_topics_audit.py` scans local standard-topic PDFs from
`~/Downloads/MadAsMaths standard topics`, writes an ignored JSONL question
ledger, and runs curated source-derived host checks. Use `--strict-manual` when
the ledger is expected to have no unaudited calculator-testable rows.

`check_madasmaths_standard_manual_cases.py` runs hand-transcribed cases from
rendered MadAsMaths standard-topic PDFs and records the latest local report.
Line-coordinate booklet coverage is represented by grouped executable algebra
cores plus unsupported-ok notes for sketch/proof-only parts.

`check_a_level_audit_tracker.py` runs every command recorded in
`a_level_audit_tracker.jsonl`; it is a stale-command guard, not proof of full
manual mark-scheme coverage.

`check_edexcel_question_audit_coverage.py` verifies every official Pearson 9MA0
question paper has tracker rows for all inferred question numbers.

`manual_question_triage_notes.jsonl` is for parallel review agents only: record
manual page-image question/mark-scheme notes there, validate with
`check_manual_question_triage_notes.py`, then let the main audit agent convert
useful rows into executable host checks.
