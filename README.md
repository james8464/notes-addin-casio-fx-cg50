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

Verbose F6 help and worked-template records are packed into `CASIOCAS.PAK`;
keep long help text in `c++/prizm/help/`, not inside `.g3a` source strings.

Use `c++/addin/host/build/device_solver_smoke` only for the separate native/fallback add-in route.

`c++/tools/build_khicas_host_runner_docker.sh` builds a separate source-based KhiCAS core runner for audit triage. It does not participate in `.g3a` packaging; calculator behavior still comes from `./compile` and the KhiCAS source tree.

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

Production exam-working fixes go in `c++/khicas/upstream/giac90_1addin/`.
Use `c++/addin/src/modules/` as the step-order spec; GIAC remains the maths
engine for the calculator build. Current production overlays include direct
table integrals, affine substitution hints, binomial `coeff(...)` working, and
geometric/exponential threshold inequality hints; answers still come from GIAC.

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



Manual question triage notes are tracked in `c++/tools/golden/manual_question_triage_notes.jsonl`.

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
After any host/prototype solver change, run `./compile` before trusting calculator parity; `calculator_files/CasioCAS.g3a` must be the source-built KhiCAS add-in, not only a host/native smoke result.

## Notes

- Generated files, editor state, virtualenvs, reports, and local graph outputs are intentionally not kept in git.
- External paper PDFs and rendered page images are temporary audit inputs; only manifests, scripts, and manual trackers belong in git.
- `run_tests.py` is C++-only.
- Host build outputs belong under `c++/addin/host/build/`.
