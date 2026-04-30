## Prompt: Port `deriveProgram.py` to C/C++ (fx-CG50 add-in)

You are an AI coding agent working in this repo:
- **Repo root**: `/Users/james/Developer/Python/CASIO`
- Python source-of-truth lives under `python/src/`
- Native add-in port lives under `addin/`

Goal: convert the Python differentiation engine:
- `python/src/Math/deriveProgram.py`

into an equivalent **C/C++** module inside the add-in, reusing the existing C++ core (parser/AST/formatter) in `addin/src/core/`.

### Non-negotiables
- **Behavior parity**: outputs should match Python as closely as possible (exam-style working + final derivative forms).
- **Input language parity**: keep same normalization rules and compact syntax (eg `sin^-1x`, `sinx`, `|x|`, `√x`, superscripts).
- **No internet needed**: rely only on code in this repo.
- **Do not commit build artifacts** (`addin/host/build`, `addin/build`, `.DS_Store`, `.idea`).

### What already exists (use it)
#### C++ math core (already implemented)
Located in `addin/src/core/`:
- `normalize_text()` in `normalize.{hpp,cpp}`
- `tokenize_expr()` in `tokenize.{hpp,cpp}`
- `parse_expr()` in `parse.{hpp,cpp}`
- minimal `simplify()` + constructors in `simplify.{hpp,cpp}`
- `format_expr()` in `format_expr.{hpp,cpp}` (smoke-checked vs Python `casio_core.format_expr`)
- `format_equation_human_readable()` and `format_exam_working()` in `format_exam.{hpp,cpp}`

#### Host runner (fast testing)
- Build (macOS): use cmake installed via pip at `/Users/james/Library/Python/3.14/bin/cmake`
- Host CMake project: `addin/host/`
- Binary: `addin/host/build/casio_host`

#### Golden tools
- `tools/golden/generate_fixtures.py` (RTF mapping for other modules)
- `tools/golden/compare_expr_format.py` (Python vs C++ parse/format smoke)

### Derive scope (what to port)
Port the **user-facing flows** in `deriveProgram.py`:
- Normal derivative mode (d/dx)
- Second derivative mode where supported (d2y/dx2)
- Any other modes exposed by the CLI prompts in `deriveProgram.py`

The port should provide a clean C++ entrypoint similar to:

```cpp
namespace casio::derive {
  struct Request {
    int mode;            // mirrors Python menu selection
    std::string expr;    // raw input expression (unicode allowed)
  };

  // Return lines ready for on-calc viewer (exam-style).
  std::vector<std::string> run(casio::Arena &arena, Request const &req);
}
```

Then the add-in UI will call this from `addin/src/main.cpp` later.

### Implementation strategy (recommended)
1) **Identify Python entrypoints**
   - Find the interactive `main()`/`run()` logic in `python/src/Math/deriveProgram.py`
   - Extract the core functions used for each menu option

2) **Port in layers**
   - Port only the minimal subset needed to satisfy one regression test at a time.
   - Keep function names and structure close to Python to reduce mistakes.

3) **Use the shared C++ AST**
   - Use `casio::Arena` + `NodeId` from `addin/src/core/arena.hpp` and constructors in `simplify.hpp` (`num/sym/add/mul/div/power/fn/neg/simplify`).
   - Parse with `casio::parse_expr(arena, expr)` (this includes normalization).

4) **Printing**
   - Use `format_expr()` for internal canonical text.
   - Use `format_equation_human_readable()` for exam-style text where Python uses it.

5) **Guardrails**
   - Mirror Python’s “exam guard” complexity checks to avoid hangs.

### Testing requirements (must do)
Use Python’s existing regression tests as the oracle.

#### Primary regression suite
Run:
```bash
python3 -m unittest -q python/tests/test_specific_regressions.py
```

Key derive expectations are in that file (examples):
- compact inverse trig input: `sin^-1x`
- product/chain/quotient formatting preferences
- `|x|` derivative note about undefined points

#### Add derive-focused host comparison
Add a new tool:
- `tools/golden/compare_derive.py`

It should:
- run Python `python/src/Math/deriveProgram.py` with stdin to produce output
- run a new host flag (you add) like:
  - `casio_host --derive "MODE,EXPR"`
- compare the key final derivative line(s) and some working lines.

This lets you iterate without needing the calculator.

### Deliverables (code changes)
Add files:
- `addin/src/modules/derive/derive.hpp`
- `addin/src/modules/derive/derive.cpp`

Wire host runner:
- Update `addin/host/CMakeLists.txt` to link `../src/modules/derive/derive.cpp`
- Update `addin/host/main.cpp` to add `--derive` flag

Do **not** wire into on-calc UI yet unless minimal and stable.

### Quality bar for “done enough to merge”
- Host `--derive` works on at least 10 curated cases (from regressions).
- No new failures in `python/tests/test_specific_regressions.py` (Python oracle stays passing).
- Derive module compiles cleanly in host build.

### Notes on size/perf
- Prefer `-Os` friendly code (avoid giant templates).
- Avoid recursion depth explosions; Python has `MAX_NESTING_DEPTH` type guards.

