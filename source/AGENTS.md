## Repo Rules

- Caveman style.
- Prefer Graphiffy/Mermaid for maps.
- Source-built `.g3a`; do not patch upstream binaries.
- Keep `.g3a` under `2,097,152` bytes.

## Build

`./source/compile notes` — builds `NOTES.g3a`.

Requires Docker (`source/tools/docker/Dockerfile.notes-source`). The orchestrator is `source/tools/build/build_g3a.sh`: stages `apps/notes/notes_app.cc` and `shared/casio/casio_suite_ui.hpp` into the upstream tree, runs `make NOTES.g3a` inside a linux/amd64 container, copies artifacts back, normalizes metadata, and runs checks.

## Test

No test framework — all tests are standalone Python scripts:

- `python3 source/tests/check_notes_integrity.py` — content format constraints
- `python3 source/tests/check_notes_render_layout.py` — render constants match source
- `python3 source/tests/check_notes_docx_coverage.py` — DOCX coverage (requires `python-docx`)
- `python3 source/tests/check_multi_app_suite.py` — source integrity

## Key files

| File | Role |
|---|---|
| `source/apps/notes/notes_app.cc` | Notes add-in source |
| `source/shared/casio/casio_suite_ui.hpp` | Shared Casio UI helpers |
| `source/khicas/upstream/giac90_1addin/Makefile` | SH3 make for NOTES.g3a |
| `source/khicas/upstream/giac90_1addin/prizm.ld` | Linker script |
| `source/tools/build/build_g3a.sh` | Build orchestrator (Docker in/out) |
| `source/tools/docker/Dockerfile.notes-source` | Docker image with SH3 cross-compiler |

## Git

`.g3a`, `calculator/` (except `NOTES/`), `source/build/`, `source/tests/reports/` are gitignored.
