# Casio fx-CG50 (MicroPython 1.9.4) — deployment audit and checklist

## Scope (25 `*.py` files in this repo)

| Path | Role | On calculator? |
|------|------|----------------|
| `src/Math/algebraProgram.py` | Core algebra | **Yes** (as `.mpy`) |
| `src/Math/trigProgram.py` | Core trig | **Yes** |
| `src/Math/deriveProgram.py` | Core derive | **Yes** |
| `src/Math/intProgram.py` | Core integrate | **Yes** |
| `src/Math/SUVATprogram.py` | Core SUVAT | **Yes** |
| `src/ComputerScience/booleanProgram.py` | Core boolean | **Yes** |
| `src/calc_files/shared_fallback.py` | Fallback when `shared_*` missing | **Optional** (import tries this name) |
| `src/calc_files/*.py` (launchers, `main`, `compat_probe`, `check_mpy_build`, `run_device_sim`) | IO helpers | **Launchers + probe: yes**; PC tools: no |
| `src/shared_*.py` | Shared with PC test imports | **No** (not required if fallback path used) |
| `tests/*.py` | Harness | **No** (CPython + sympy + textual) |

## Critical blockers (addressed in repo)

1. **Import-time interactive `main` (boolean)** — `src/ComputerScience/booleanProgram.py` ran the menu on `import` (lines 731+). **Fix:** `def main():` + `if __name__ == "__main__": main()` only. Each `src/calc_files/<name>.py` launcher defines `run()` → `<name>Program.main()`. On REPL: `from algebra import *` then `run()`.
2. **`SKIP_AUTORUN` vs scripted import** — `algebraProgram` / `deriveProgram` / `intProgram` / `SUVATprogram` used `SKIP_AUTORUN` to avoid `main` when `len(sys.argv)>1`, which is fragile and unlike explicit `__main__`. **Fix:** `run = main` and `if __name__ == "__main__": main()` only (matches `trigProgram`).

## Likely runtime incompatibilities (by pattern)

| Pattern | Files | uPy 1.9.4 / Casio | Notes |
|---------|--------|-------------------|--------|
| `f"..."` | `tests/run_tests.py`, `src/shared_llm.py` (was; now reduced) | Tests: N/A on device. LLM: keep off device. | Calculator `src/Math/*` has **no** f-strings (verified by scan). |
| `dataclasses` / `Enum` | `tests/run_tests.py` | Not in firmware | Harness only. |
| `pathlib.Path` | `tests/*`, `src/calc_files/run_device_sim.py`, `src/shared_llm.py` | Unlikely on device | Do not copy those tools to the calculator. |
| `import sympy` | `tests/run_tests.py` | Not available | Oracle math on PC only. |
| `textual` TUI | `tests/run_tests.py` | Not available | PC only. |
| `eval()` on user expressions | `tests/run_tests.py` (numeric harness) | Avoid on device | Not used by `*Program` calculators. |
| `__import__('trigProgram')` | `src/Math/algebraProgram.py` | **OK** if `trigProgram.mpy` on path | Name must match compiled module. |
| `from __future__ import print_function` | `src/calc_files/run_device_sim.py` | Harmless on Py3 | PC helper only. |

## Memory / performance (high level)

- **`src/Math/*Program.py`:** Large single modules; import binds many globals. `.mpy` reduces **parse/compile** RAM on import vs raw `.py`; **runtime** speed is similar. Keep `LOW_MEMORY_RUNTIME` / `casio_hw_sim_from_env()` paths in mind; avoid growing caches on device.
- **Recursion / deep trees:** Symbolic `sim()`-style code can recurse; stack is small on device. Prefer existing depth limits where present.
- **Do not** add `gc.collect()` inside `src/Math` unless a measured leak; it can add jitter. `compat_probe.run()` is the right place to *observe* `gc` after cold import.

## Deployment / filename risks

- Use **short ASCII** names: `trig.py`, `main.py`, `trigProgram.mpy` (already used).
- Casio editor ~300 lines × 255 cols: **keep `main.py` and launchers tiny**; edit big logic only on PC, ship `.mpy` only.
- If both `foo.py` and `foo.mpy` exist, behaviour depends on port `sys.path` order. **Rule:** for production, copy **only** `.mpy` (+ tiny launcher) to avoid wrong module picked.

## `.mpy` build (macOS, mpy-cross from MP **1.9.4**)

1. Build `mpy-cross` from MicroPython 1.9.4 source (bytecode v3, small-int-31 is typical for this project).
2. For each program (example flags used in this repo):
   - `mpy-cross -msmall-int-bits=31 -mno-unicode -o src/calc_files/trigProgram.mpy src/Math/trigProgram.py`
   - For very large programs: add `-X heapsize=52428800` to `mpy-cross` *only* if your build supports it (see `tests/run_tests.py` `action_compile`).
3. Run on PC: `python3 src/calc_files/check_mpy_build.py` and confirm all expected headers.
4. Copy to calculator **folder that is on `sys.path`** (per Casio app docs), typically alongside launchers.
5. Test import on device: `import compat_probe; compat_probe.run()` then `import main; main.run()` (after setting `_MODULE` in `main.py`).

Optional: compile tiny helpers with the same flags:
- `mpy-cross ... -o compat_probe.mpy src/calc_files/compat_probe.py`
- `mpy-cross ... -o main.mpy src/calc_files/main.py`

## `compat_probe.py`

See `src/calc_files/compat_probe.py` — prints `sys.version`, `sys.path`, `sys.implementation`, `gc` stats when present, and tries `math`, `random`, `casioplot`, `turtle`, `matplotlib.pyplot`.
