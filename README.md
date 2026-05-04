## CASIO fx-CG50 CAS port

Two main code areas live here:

- `c++/` - production add-in source, host runner, build scripts, and regression tooling
- `python/` - archived original MicroPython implementation; safe to zip once you no longer need historical reference files

The C++ side is now the active production path.

## Structure

```text
c++/
  addin/      fxSDK/gint fallback add-in and host CMake project
  khicas/     edited KhiCAS source used for the production .g3a
  prizm/      PrizmSDK/libfxcg add-in target
  tools/      build, fuzz, golden, and native test scripts
  third_party/ archived SDK/vendor inputs used by Docker builds

python/
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
```

Build calculator add-ins:

```bash
./compile
```

Transfer files are published only under `calculator_files/`. Build artifacts stay under `c++/prizm/build/`.

## Notes

- Generated files, editor state, virtualenvs, reports, and local graph outputs are intentionally not kept in git.
- `run_tests.py` is C++-only.
- Host build outputs belong under `c++/addin/host/build/`.
