# Evidence

Coverage:
- Source folder: `/Volumes/VM/Edexcel A Level Maths past papers`.
- Files found: 1,263 total; 1,256 PDF/image assets.
- OCR page index: 1,256 rows, 30 paper/mark-scheme groups, 712 question pages, 544 mark-scheme pages.
- Candidate tests mined from OCR/queue reports: 227.

Paper-derived sweep:
- Command: inline runner over `tests/reports/edexcel_papers/candidate_tests.jsonl` using `tools/khicas_host_runner`.
- Result after fixes: `{'ok': 211, 'thin': 16}`.
- Fixed real route gaps:
  - `(...)=...,method=identity` now routes to `xform(left,right)`.
  - `eq,var,...,method=identity` now routes to `solve(eq,var)`.
  - `expr,var` paper shorthand now routes conservatively to `diff(expr,var)` when the second item is a single variable and the expression contains it.
  - Shared derivative core now handles constant-product derivatives and `e^(u)` chain derivatives.
- Remaining 16 thin entries are native/exempt fragments: raw values/expressions, vectors, `sqrt`, `factor`, `sum`, and numeric substitutions.

Targeted tests:
- `python3 tests/check_targeted_working_gaps.py`
- Result: `OK targeted working gaps=150`.

Build/size:
- `./compile` passed.
- `calculator_files/CAS.g3a`: 2,095,884 bytes.
- Limit: 2,097,152 bytes.
- Headroom: 1,268 bytes.
- `calculator_files/CAS.PAK`: 32,631 bytes, 54 records.
- `calculator_files/RUNMAT.g3a`: 30,216 bytes.
- `python3 tools/check_g3a_size.py calculator_files/CAS.g3a` passed.
- `python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a` passed.
- CAS.g3a SHA256: `14b36c7f9a062b585a309dd798febcc0b3432a1ae88313f839efe8e6b03afe1d`.
- CAS.PAK SHA256: `bd932b510da9823566db28e5c76a00b6bf1615dbe82ade4aa3bb0654fe499507`.
- RUNMAT.g3a SHA256: `f38b673a0316473d7038da3a075d5bb8a12f875ea98258e5d86e09d54733fbbf`.

Blocked evidence:
- `python3 tests/run_exact_queue.py --workers 4` failed before running cases because 553 existing queue rows use `source` instead of `source_pdf`; first missing row is 10088.
- `git diff --check` passed.
