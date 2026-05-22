## CASIO fx-CG50 CAS port

Two main code areas live here:

- `c++/` - production add-in source, host runner, build scripts, and regression tooling
- `python.zip` - archived original MicroPython implementation; kept only for historical reference

The C++ side is now the active production path.

Current exam scope is Edexcel A-level Mathematics 9MA0 only. Further Maths-only
surfaces and raw helper wrappers should stay hidden or return compact
unsupported errors; do not grow `.g3a` for non-syllabus commands.

## Production path

`./compile` builds the real calculator add-in from:

```text
c++/khicas/upstream/giac90_1addin/
```

Host modules under `c++/addin/src/modules/` are fast regression/prototype code. A fix there does **not** change `CasioCAS.g3a` unless the same logic is ported into the KhiCAS source path above.

Primary calculator working overlay:

```text
c++/khicas/upstream/giac90_1addin/main.cc
c++/prizm/help/CASIOCAS.WORK.TPL
c++/prizm/help/CASIOCAS.HLP
```

Use `c++/addin/host/build/device_solver_smoke` only for the separate native/fallback add-in route.

## Structure

```text
c++/
  addin/      fxSDK/gint fallback add-in and host CMake project
  khicas/     edited KhiCAS source used for the production .g3a
  prizm/      Prizm assets, help/template source, and build output
  tools/      build, fuzz, golden, and native test scripts
  third_party/ archived SDK/vendor inputs used by Docker builds

calculator_files/
  generated transfer folder: CasioCAS.g3a + CASIOCAS.PAK

python.zip
  original MicroPython implementation; not needed by C++ test/build commands
```

## Common commands

Run C++ checks:

```bash
python3 run_tests.py
```

Build the C++ host runner:

```bash
./c++/tools/build_host.sh
./c++/addin/host/build/casio_host --alg "2*x+3=7"
```

Run native C++ checks:

```bash
python3 c++/tools/run_tests_cpp.py
python3 c++/tools/golden/check_device_scope.py
```

Download external A-level audit sources:

```bash
python3 c++/tools/golden/download_a_level_audit_sources.py --scope all --clean --force
python3 c++/tools/golden/download_online_paper_corpus.py --clean --force
python3 c++/tools/golden/check_a_level_source_downloads.py
python3 c++/tools/golden/check_edexcel_public_paper_corpus.py
python3 c++/tools/golden/render_audit_pdf_pages.py --format jpeg ~/Downloads/"MadAsMaths standard topics" ~/Downloads/"MadAsMaths A-level booklets" ~/Downloads/"MadAsMaths papers" ~/Downloads/"Edexcel A Level Maths past papers" ~/Downloads/"Edexcel A Level Maths support materials"
python3 c++/tools/golden/check_madasmaths_standard_topics_audit.py
python3 c++/tools/golden/check_madasmaths_full_audit.py --no-render --strict-skips
python3 c++/tools/golden/check_a_level_audit_tracker.py
```

Clean generated/local files (dry-run first):

```bash
python3 c++/tools/clean_project.py
python3 c++/tools/clean_project.py --apply
python3 c++/tools/clean_project.py --reports --apply  # also remove live reports
```

Build calculator add-ins:

```bash
./compile
```

Transfer files are published under `calculator_files/`, then `./compile` copies that folder to `~/Downloads/calculator_files/`. Copy both `CasioCAS.g3a` and `CASIOCAS.PAK` from that folder to the calculator root. Build artifacts stay under `c++/prizm/build/`.

## Notes

- Generated files, editor state, virtualenvs, reports, and local graph outputs are intentionally not kept in git.
- External paper PDFs and rendered page images live under `~/Downloads`; only audit scripts and manual trackers belong in git.
- `run_tests.py` is C++-only.
- Host build outputs belong under `c++/addin/host/build/`.
