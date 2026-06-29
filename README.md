# NOTES.g3a — Casio fx-CG50 Notes Browser

Read-only notes browser for `.txt` files on the fx-CG50 calculator.

## Artifact

- `calculator_files/NOTES.g3a` — install by copying to the calculator
- `calculator_files/NOTES/` — place your `.txt` note files here

## Build

```bash
./compile notes
```

Requires Docker (`tools/docker/Dockerfile.notes-source`). Build runs inside a linux/amd64 container with the SH3 cross-compiler.

## Source layout

```
apps/notes/notes_app.cc       Notes add-in source
shared/casio/                 Shared Casio UI helpers
khicas/upstream/giac90_1addin Minimal build tree (Makefile, linker script, icons)
tools/build/                  Build orchestrator and metadata normalisation
tools/checks/                 Post-build validation (metadata, size)
tools/docker/                 Docker image for SH3 cross-compilation
tools/notes/                  One-off notes conversion utilities
```

## Test

All tests are standalone Python scripts:

```bash
python3 tests/check_notes_integrity.py
python3 tests/check_notes_render_layout.py
python3 tests/check_notes_docx_coverage.py    # requires python-docx
python3 tests/check_multi_app_suite.py
```
