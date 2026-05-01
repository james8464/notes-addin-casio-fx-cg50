## CASIO fx-CG50 CAS port (Python → native `.g3a`)

This repository is a **Casio fx-CG50** calculator project that:

- Preserves the original **Python/MicroPython-style** “exam working + Answer:” programs (as an oracle / legacy reference).
- Implements a new **native C++ symbolic engine** intended to ship as a **single `.g3a` add-in**. The default calculator build now targets PrizmSDK/libfxcg for native Casio OS UI/input; the older fxSDK + gint target remains as a fallback.
- Provides **one test harness UI** (a Textual TUI) that can test **either backend** (Python or C++) and switch at runtime.

The overarching goal is **near 1:1 functional parity** (where practical) between the legacy Python programs and the new native implementation, with robust fallbacks and strict memory/size constraints appropriate for the fx-CG50.

---

## Top-level layout (only `python/` and `c++/`)

- `python/`
  - Legacy/oracle programs (the original engines).
  - The shared Textual test harness: `python/tests/run_tests.py` (drives both backends).
- `c++/`
  - `c++/prizm/` — the default native fx-CG50 add-in source (PrizmSDK/libfxcg target).
  - `c++/addin/` — the legacy fallback fx-CG50 add-in source (fxSDK + gint target).
  - `c++/addin/host/` — a desktop “host” binary (`casio_host`) for fast testing.
  - `c++/tools/` — golden tests, fuzz/regressions, build scripts (host + add-in).
  - `c++/tests/reports/` — session logs for C++ backend random runs.
  - `c++/docs/` — design/porting plans and supporting documentation.
  - `c++/graphify-out/` — generated Graphify report and graph JSON. Cache files are local-only.

---

## The single test harness (Textual TUI)

### Run it

From repo root:

```bash
python3 python/tests/run_tests.py
```

### Switch backends inside the TUI

- `/switch python`
  - Runs cases using the **Python oracle** programs.
  - Session report path: `python/tests/reports/failure_report_latest.txt`

- `/switch c`
  - Runs cases using the **C++ host binary** (`casio_host --stdin-program ...`).
  - Session report path: `c++/tests/reports/failure_report_latest.txt`

### Compile/build from inside the TUI

Use:

- `/compile`

Behavior depends on backend:

- **Python backend**: rebuilds `.mpy` files into the legacy `calc_files` workflow (legacy support).
- **C++ backend**: builds:
  - the desktop host (`c++/addin/host/build/casio_host`)
  - the native PrizmSDK/libfxcg fx-CG50 add-in (`c++/prizm/build/CasioCAS.g3a`) using Docker by default on macOS

> You *can* use standalone scripts (see below), but the intent is: most day-to-day work happens through the TUI.

---

## Native build system (C++ host + `.g3a`)

### C++ host binary (fast local runner)

This is used to run the native engine on your computer for rapid iteration:

```bash
./c++/tools/build_host.sh
./c++/addin/host/build/casio_host --alg "2*x+3=7"
```

The TUI’s C++ backend uses this host binary under the hood via `--stdin-program`.

### `.g3a` add-in build on macOS (Docker)

Recommended workflow on macOS is `/switch c` then `/compile` in the TUI. This builds the PrizmSDK/libfxcg add-in that uses Casio OS UI/input syscalls.

Standalone PrizmSDK build:

```bash
./c++/tools/build_addin_prizm_docker.sh
```

Expected output: a `.g3a` under `c++/prizm/build/` (the script prints the exact path).

Legacy fxSDK + gint fallback:


```bash
./c++/tools/build_addin_docker.sh
```

Expected fallback output: a `.g3a` under `c++/addin/build-cg/`.

#### Docker troubleshooting checklist

If `./c++/tools/build_addin_docker.sh` fails, check:

- Docker Desktop is **running** (open the app once after install).
- The CLI works:

```bash
docker --version
docker ps
```

If `docker` is still “not found”, Docker Desktop hasn’t installed the CLI into your PATH yet, or it hasn’t finished initializing.

---

## Why is the TUI still under `python/`?

The file `python/tests/run_tests.py` started life as the **legacy Python test harness**, and it still imports/uses a lot of Python-oracle-specific utilities (SymPy-based checking, numeric checkers, LLM prompts, etc.).

We extended it so it can also drive the native C++ backend, but it remains physically in `python/` because:

- it directly depends on `python/src/...` modules as the oracle backend
- keeping it close to the oracle reduces import/path friction

For a more neutral entrypoint, use the root-level launcher:

```bash
python3 run_tests.py
```

---

## Accuracy gates (what “green” means here)

For the C++ backend random runs we enforce:

- **Robustness gate**: no crashes; stable output with an `Answer:` line.
- **Correctness gate (where judgeable)**: match Python-oracle results using strict or numeric-equivalence checks depending on the feature.

The random generators are backend-aware so the C++ backend is tested against what it actually supports (rather than Python-only modes).

---

## “Standalone” commands you’ll still see (and why)

Even if you primarily use the TUI, these are still useful for automation and debugging:

- `python3 c++/tools/run_tests_cpp.py`
  - Runs the complete native regression suite (golden fixtures + fuzz regressions).
  - This is effectively the “CI entrypoint” for the C++ side.

- `./c++/tools/build_host.sh`
  - Quick rebuild of the C++ host binary without entering the TUI.

- `./c++/tools/build_addin_docker.sh`
  - Deterministic `.g3a` build on macOS (Docker).

The TUI’s `/compile` calls these under the hood for the C++ backend, but having them as standalone scripts keeps the project maintainable.

---

## Key files (high signal)

- **Single test harness**: `python/tests/run_tests.py`
- **Native host entrypoint**: `c++/addin/host/main.cpp`
- **Native add-in UI**: `c++/addin/src/ui/*`
- **Native modules**:
  - `c++/addin/src/modules/algebra/*`
  - `c++/addin/src/modules/trig/*`
  - `c++/addin/src/modules/derive/*`
  - `c++/addin/src/modules/integrate/*`
  - `c++/addin/src/modules/suvat/*`
- **Core symbolic engine**: `c++/addin/src/core/*` (parser, normalize, simplify, formatting, etc.)
- **Native regression runner**: `c++/tools/run_tests_cpp.py`

---

## Notes for other AI agents

- The Python backend is the **oracle**. Don’t change it unless explicitly requested.
- The C++ backend must be:
  - **budgeted** (node limits, recursion guards, bounded caches)
  - **stable** (never throw raw parse errors to the user; return an exam-style fallback with `Answer:`)
  - **small enough** to ship on-device (practical `.g3a` size constraints)
