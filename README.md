## CASIO fx-CG50 CAS port

Two codebases live here:

- `python/` - original MicroPython-oriented oracle implementation, deployment launchers, and the shared Textual test harness
- `c++/` - native add-in implementation, host runner, build scripts, and native regression tooling

The Python side is the reference behavior. The C++ side is the production `.g3a` direction.

## Structure

```text
python/
  src/        core Python programs and calculator launchers
  docs/       MicroPython/device notes
  tests/      shared TUI + Python-side regression helpers

c++/
  addin/      fxSDK/gint fallback add-in and host CMake project
  prizm/      PrizmSDK/libfxcg add-in target
  tools/      build, fuzz, golden, and native test scripts
  third_party/ archived SDK/vendor inputs used by Docker builds
```

## Common commands

Run the shared TUI:

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
```

Build calculator add-ins:

```bash
./c++/tools/build_addin_prizm_docker.sh
./c++/tools/build_addin_docker.sh
```

## Notes

- Generated files, editor state, virtualenvs, reports, and local graph outputs are intentionally not kept in git.
- `python/tests/run_tests.py` is still the single cross-backend test UI because it depends directly on the Python oracle modules.
- Host build outputs belong under `c++/addin/host/build/`.
