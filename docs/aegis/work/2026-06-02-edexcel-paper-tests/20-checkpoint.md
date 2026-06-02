# Checkpoint

Current todo:
- Keep calculator menu/UI untouched.
- Use the existing Edexcel OCR reports to mine/run paper-derived pure tests.
- Patch only general math/working routes exposed by paper tests.
- Compile and size-check calculator outputs.
- Commit and push relevant files.

Active slice: paper-derived route validation and final evidence.

Completed:
- `/Volumes/VM/Edexcel A Level Maths past papers` has 1,263 files; 1,256 PDF/image paper assets are indexed.
- Existing OCR page index covers 1,256 pages across 30 paper/mark-scheme groups: 712 question pages, 544 mark-scheme pages.
- 227 paper-derived candidate tests were mined.
- Current sweep reached 211/227 acceptable after fixing real route gaps.
- Remaining 16 are raw expressions, vectors, native/exempt functions, sums, or numeric substitutions rather than failed working routes.
- Targeted working gaps passed after current working-route edits.
- `./compile` passed and copied `CAS.g3a`, `CAS.PAK`, and `RUNMAT.g3a` to `calculator_files/`.

Blocked-on:
- `tests/run_exact_queue.py --workers 4` fails on the existing queue schema at row 10088 because 553 rows use `source` instead of `source_pdf`. Checker/queue edits are out of scope for this slice.

Drift check:
- Still serves the paper-test task.
- Stayed inside math/build-size boundary.
- No menu/status/F-key edits in this slice.
- No new fallback route added for the fixed paper cases; inputs now dispatch to existing `diff`, `solve`, and `xform` routes.

Resume hint: do not edit menu/UI files. Size headroom is only 1,268 bytes, so future logic additions need matching safe code-size savings.
