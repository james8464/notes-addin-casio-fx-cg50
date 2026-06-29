## Repo Rules

- Caveman style.
- Prefer Graphiffy/Mermaid for maps.
- Source-built `.g3a`; do not patch upstream binaries.
- Keep `.g3a` under `2,097,152` bytes.

## Build

`./compile notes` — builds `NOTES.g3a`.

Requires Docker (`tools/docker/Dockerfile.notes-source`). The orchestrator is `tools/build/build_g3a.sh`: stages `apps/notes/notes_app.cc` and `shared/casio/casio_suite_ui.hpp` into the upstream tree, runs `make NOTES.g3a` inside a linux/amd64 container, copies artifacts back, normalizes metadata, and runs checks.

## Test

No test framework — all tests are standalone Python scripts:

- `python3 tests/check_notes_integrity.py` — content format constraints
- `python3 tests/check_notes_render_layout.py` — render constants match source
- `python3 tests/check_notes_docx_coverage.py` — DOCX coverage (requires `python-docx`)
- `python3 tests/check_multi_app_suite.py` — source integrity

## Key files

| File | Role |
|---|---|
| `apps/notes/notes_app.cc` | Notes add-in source (1657 lines) |
| `shared/casio/casio_suite_ui.hpp` | Shared Casio UI helpers |
| `khicas/upstream/giac90_1addin/Makefile` | SH3 make for NOTES.g3a |
| `khicas/upstream/giac90_1addin/prizm.ld` | Linker script |
| `tools/build/build_g3a.sh` | Build orchestrator (Docker in/out) |
| `tools/docker/Dockerfile.notes-source` | Docker image with SH3 cross-compiler |

## Git

`.g3a`, `build/`, `calculator_files/*` (except `NOTES/` and `.gitkeep`), `tests/reports/` are gitignored.
