# Manual Question Triage Agent Prompt

Goal: review downloaded A-level Maths PDFs and mark schemes, then append compact notes to:

`c++/tools/golden/manual_question_triage_notes.jsonl`

Do not edit existing executable tests unless explicitly asked. Do not touch files the main audit agent is editing.

## Work Loop

1. Pick an unclaimed downloaded PDF or a small batch.
2. Render pages to images:

```bash
python3 c++/tools/golden/render_audit_pdf_pages.py --format jpeg "<PDF parent folder>"
```

3. Manually inspect page images. Do not trust OCR for maths.
4. Find the matching mark scheme or worked solution.
5. For every question or useful subpart, append one JSONL row.
6. Validate:

```bash
python3 c++/tools/golden/check_manual_question_triage_notes.py
```

7. Commit only your appended triage rows and any small docs updates.

## 10 Hour Control Policy

- Work in 4-question or 2-page batches so progress is visible every commit.
- Use existing KhiCAS/Giac commands for all maths: `solve`, `factor`, `expand`, `simplify`, `diff`, `int`, `subst`, `normalcdf`, `binomial`, etc.
- Do not write custom maths engines, bespoke solvers, or duplicate CAS logic.
- Custom project code is only for parsing the question, selecting KhiCAS commands, and emitting mark-scheme-style working stages.
- Treat mark schemes as examples of sufficient working, not exact string targets.
- If ETA in `python3 c++/tools/audit_progress_tui.py --once` exceeds 10h, split work across more triage agents by disjoint PDF folders.
- No quality-loss shortcut is allowed: only reduce ETA by adding non-overlapping agents, shrinking batches, and converting obvious KhiCAS-backed questions first.
- Each agent must commit small append-only JSONL batches so progress is measurable by the TUI.

## JSONL Schema

One line per question/subpart:

```json
{"id":"source_slug_q1a","agent":"codex-triage-1","source_pdf":"relative/or/download/name.pdf","mark_scheme":"relative/or/download/ms.pdf","question":"1(a)","verdict":"testable","topic":"integration by substitution","review_basis":"manual page-image review","question_text":"compact exact transcription of the usable maths only","mark_scheme_working":["u=...","du/dx=...","integral=..."],"candidate_commands":[["--int","..."]],"expected_equivalent":["..."],"unsupported_reason":"","notes":"short"}
```

Verdicts:
- `testable`: whole subpart can become a host test.
- `partial`: only part of the working/result can become a host test.
- `skip`: not useful for CAS testing; include `unsupported_reason`.

Rules:
- Keep rows compact.
- Transcribe only needed maths and mark-scheme working.
- Preserve exact constants, domains, intervals, and answer form.
- If a diagram is essential, use `skip` or `partial`.
- Never mark a row testable unless a deterministic command/result is clear.
- Prefer many small rows over one vague row.

## Handoff

Primary audit agent should consume this file, convert good rows into executable checks, then leave the original triage row as evidence.
