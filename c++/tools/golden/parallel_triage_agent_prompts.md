# Parallel Triage Agent Prompts

Use one prompt per separate Codex chat. Give each chat a disjoint folder or PDF range.

## Monitor Prompt

```text
Every 30 minutes, run:
python3 c++/tools/audit_progress_tui.py --once --no-color

If ETA is above 10 hours, report the current rate, remaining sources, and how many additional non-overlapping triage lanes are needed. Do not reduce quality. The only acceptable speedups are more parallel agents, smaller commits, and prioritising questions that map cleanly to existing KhiCAS/Giac commands.
```

## Triage Lane Prompt

```text
Follow /Users/james/Developer/Python/CASIO/c++/tools/golden/manual_question_triage_prompt.md exactly.

Lane ownership: <PUT ONE DISJOINT PDF FOLDER OR PDF LIST HERE>
Agent id: <codex-triage-N>

Manually inspect rendered page images and matching mark schemes. Do not use OCR for maths. Append compact rows only to:
/Users/james/Developer/Python/CASIO/c++/tools/golden/manual_question_triage_notes.jsonl

For every testable/partial row, express the maths using existing KhiCAS/Giac commands only: solve, factor, expand, simplify, diff, int, subst, normalcdf, binomial, etc. Do not invent custom maths engines. Custom code should only parse the question and emit mark-scheme-style working stages.

Work in 2-page or 4-question batches. After each batch:
python3 c++/tools/golden/check_manual_question_triage_notes.py
git diff --check -- c++/tools/golden/manual_question_triage_notes.jsonl
git add c++/tools/golden/manual_question_triage_notes.jsonl
git commit -m "Triage <folder-or-pdf> <range>"
git push

Do not edit executable tests. Do not touch files the main audit agent is editing. Do not overlap another lane's PDFs.
```

## Implementation Lane Prompt

```text
Implement only from validated rows in:
/Users/james/Developer/Python/CASIO/c++/tools/golden/manual_question_triage_notes.jsonl

For every app feature, delegate all maths to existing KhiCAS/Giac commands. Do not write bespoke solvers. Custom code may classify the question, call KhiCAS/Giac, and format mark-scheme-style working lines.

Add or update executable tests from rows marked testable/partial. Preserve unsupported rows as evidence. If a row needs maths not already handled by KhiCAS/Giac, mark it unsupported or partial; do not fake it.

Verify with:
python3 c++/tools/golden/check_manual_question_triage_notes.py
./c++/tools/build_host.sh
python3 c++/tools/run_tests_cpp.py
```
