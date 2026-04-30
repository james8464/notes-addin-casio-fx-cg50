---
name: CG50 g3a port plan
overview: Port the existing MicroPython-friendly symbolic math programs in `python/src/` (trig/algebra/derive/int/SUVAT/boolean) into a single native fx-CG50 add-in built with fxSDK + gint, preserving current behavior and outputs as closely as practical while creating a maintainable C++ architecture and test harness.
todos:
  - id: research_fxsdk_install
    content: Write exact step-by-step fxSDK+gint install instructions (Linux/WSL/macOS), including dependencies and verification commands.
    status: pending
  - id: scaffold_addin
    content: Create fxSDK CMake add-in skeleton with icons, build-cg, and transfer workflow.
    status: pending
  - id: core_ast_parser
    content: Implement C++ AST, rational numbers, normalization, tokenizer/parser matching python behavior.
    status: pending
  - id: core_simplify_sig_cache
    content: Implement signature/equivalence and bounded caches matching shared_cache/shared_helpers policies.
    status: pending
  - id: core_formatting
    content: Port exam-style formatting and printing rules from shared_formatting.py.
    status: pending
  - id: port_modules
    content: Port boolean, suvat, derive, integrate, algebra, trig modules; keep function structure close for verifiability.
    status: pending
  - id: golden_fixtures
    content: Generate golden-output fixtures from existing Python programs and build host-side C++ runner to diff outputs.
    status: pending
  - id: device_ui
    content: "Implement gint UI: menu, input editor, output viewer; connect to module engines."
    status: pending
  - id: size_memory_plan
    content: Add build flags and design decisions to stay under ~2MiB; define fallback external-data plan if exceeded.
    status: pending
  - id: on_device_validation
    content: Run smoke/perf tests on fx-CG50 and iterate until parity is achieved.
    status: pending
isProject: false
---

# fx-CG50: Port Python CASIO programs to one .g3a (fxSDK+gint)

## Goal and non-negotiables
- **Target device**: Casio **fx-CG50**.
- **Deliverable**: **one** installable **`.g3a` add-in** that exposes *all* existing capabilities from your Python programs, ideally with enhancements.
- **Behavioral target**: preserve your current exam-style working + answer outputs and input language.
- **Project-specific source of truth** (current behavior): the Python programs in:
  - `python/src/Math/trigProgram.py` (very large)
  - `python/src/Math/algebraProgram.py`
  - `python/src/Math/deriveProgram.py`
  - `python/src/Math/intProgram.py`
  - `python/src/Math/SUVATprogram.py`
  - `python/src/ComputerScience/booleanProgram.py`
  - shared utilities: `python/src/shared_helpers.py`, `python/src/shared_cache.py`, `python/src/shared_formatting.py`, `python/src/Math/casio_core.py`

## Key constraints (must design around these)
- **.g3a size limit (CG50 main menu visibility)**: community reports indicate **~2 MiB** is the maximum size for a `.g3a` to show in the main menu; larger add-ins can require **external files** (as KhiCAS50 does with `.ac2`). Plan assumes “single `.g3a`” is the goal, but explicitly includes a **fallback track** if the compiled binary exceeds the limit. Source context: discussion in Planète Casio forums about CG50 add-in limits and extra RAM usage.
- **RAM/heap reality**: CG50 add-ins typically have **hundreds of KiB** of “easy” RAM plus access to additional arenas in gint; large symbolic workloads still require careful allocation/caching. (gint and community discussions describe ~600–700 KiB typical and extra RAM regions that can be used intentionally.)
- **No Python runtime**: you’re not “packaging Python”; you are **re-implementing** the engine in C/C++.

## Recommended toolchain and why
- Use **fxSDK + gint** (modern, CG50-native, maintained, best tooling for assets/USB transfer/debug).
- fxSDK provides:
  - `fxsdk new`, `fxsdk build-cg`, `fxsdk send-cg`
  - `fxgxa` to generate/inspect/repair `.g3a` headers and icons
  - `fxconv` for assets
  - `fxlink` for USB send and runtime comms
- References:
  - fxSDK README (install, build-cg, fxgxa, fxconv, fxlink): `https://git.planet-casio.com/Lephenixnoir/fxsdk/src/branch/master/README.md`
  - gint README (linking, build-cg): `https://git.planet-casio.com/Lephenixnoir/gint/src/branch/master/README.md`

## Overall strategy (project-specific)
Your Python code already behaves like a small CAS with:
- A **tuple-based AST**: nodes like `('num', n, d)`, `('sym', 'x')`, `('fn', 'sin', arg)`, `('add', (...))`, `('mul', (...))`, `('pow', base, exp)`, `('div', a, b)`, plus `('const','pi')`/`('const','e')`.
- A shared input normalization layer (unicode → ASCII; `|x|` → `abs(x)`, `√` support, superscripts, inverse trig spellings): primarily in `python/src/Math/casio_core.py` and `python/src/shared_helpers.py`.
- Heavy but carefully bounded caching (`shared_cache.cache_store()` and per-program cache budgets).
- Exam-oriented output formatting (not CAS dumps) via `python/src/shared_formatting.py`.

The best way to port without losing features is:
- Build a **single shared C++ core library** that models the AST, parser, simplifier, equivalence/signature, printers, and memory/caching.
- Port each “Program” (trig/algebra/derive/int/SUVAT/boolean) as a **module** that consumes the core library.
- Put a **menu-driven UI shell** on top that routes user actions to modules, and prints the resulting working/answer lines in the same “exam block” style.

## Deliverable design (what the .g3a will look like)
### App identity
- One add-in icon in the main menu, eg **“CASIO-CAS”** (name configurable).
- Internally, the add-in hosts a menu:
  - Trig
  - Algebra
  - Differentiate
  - Integrate
  - SUVAT
  - Boolean

### UI philosophy (matches your current outputs)
Your Python programs largely produce **lists of lines** (working + “Answer: …”). The UI should therefore:
- Provide a **single-line or multi-line text input editor** (math expressions/equations).
- Provide a **mode selector** (prove/transform/solve/rewrite etc. for trig; solve/transform modes for algebra; etc.).
- Run the computation.
- Display output as a **scrollable line viewer** (monospace-ish font, wrapping strategy).

Implementation detail in plan: use gint’s event-driven key input and text rendering; keep all printed output in an in-memory vector of strings.

## Development environment setup (macOS-friendly)
fxSDK is best supported on Linux, but it can work on macOS with tweaks.

### Option A (most reproducible): Linux container/VM
- Create an Ubuntu VM (or use Docker if USB passthrough isn’t needed for sending files).
- Install dependencies: `cmake`, `make`, `python3`, `python3-pip`, `libpng`, `libusb-1.0`, plus build essentials.
- Install fxSDK stack using **GiteaPC** (recommended by fxSDK):
  - Install GiteaPC
  - `giteapc install Lephenixnoir/fxsdk`
  - Then install required dependencies listed by GiteaPC (compiler `sh-elf-gcc`, `gint`, `fxlibc`, `OpenLibm`, etc.)
- Verify:
  - `fxsdk` runs
  - `fxsdk path` reports toolchain locations
  - `fxsdk new Hello && cd Hello && fxsdk build-cg` produces a `.g3a`

### Option B (native macOS)
- Follow the fxSDK manual build dependencies from the README; expect some package-path tweaks.
- Use macOS package manager for `libpng`, `libusb`, `cmake`, Python, Pillow.

### Transfer to calculator
- Use **USB mass storage** copy, or `fxsdk send-cg` (uses `fxlink` and UDisks2 on Linux; on macOS you may do manual copy via mounted drive).
- Keep your `examples/khicasen.g3a` and `examples/ProbSim.g3a` as “known good” files for validating that your calculator installation flow works.

## Port architecture (detailed mapping from your Python)
### 1) Core expression model (C++)
Implement a minimal, allocation-efficient AST equivalent to your tuple nodes:
- `NodeKind`: Num, Sym, Const, Fn, Add, Mul, Pow, Div
- `Rational` for numbers: store as signed 64-bit numerator + unsigned 64-bit denominator (reduce via gcd when safe; fall back to big-int only if absolutely needed).
- `Node` payload:
  - Num: Rational
  - Sym: interned string id
  - Const: enum {E, PI}
  - Fn: enum function id + child pointer
  - Add/Mul: small-vector of children pointers
  - Pow/Div: two children pointers

Project-specific compatibility requirements:
- Mirror your canonicalization rules used in signatures (`same_by_sig`, `_shared_sig`) from `python/src/shared_helpers.py`.
- Preserve “exp” normalization: your printers map `fn('exp', x)` to `e^(x)`; in C++ treat exp as either FnExp or as Pow(ConstE, x) consistently.

### 2) Input normalization and tokenizer
Port the exact normalization behavior from:
- `python/src/Math/casio_core.py` (most complete)
- plus any extra cases in `python/src/shared_helpers.py`.

Must include:
- Unicode minus/multiply/divide variants to ASCII
- π/Π → `pi`
- degree symbol stripping
- fraction glyphs ½ ¼ ¾
- superscripts like `x²`, `x⁻¹` → `x^2`, `x^-1`
- `√` handling including `√x` → `sqrt(x)` logic
- inverse trig spellings: `sin^-1` → `asin` etc.
- compact function words: `sinx` → `sin(x)` style
- absolute value bars: `|x|` → `abs(x)`

Tokenizer/parser requirements:
- Re-implement your expression grammar as used across modules.
- Ensure identical precedence and associativity to your Python behavior.

### 3) Simplification, equivalence, signatures, caches
Your programs use a lot of:
- `sim()` style simplification
- signature-based equivalence (`sig()`, `same()`, `same_by_sig()`)
- bounded caches tuned for low memory

Plan for C++:
- Implement `Sig` canonicalization that sorts `add`/`mul` children signatures (like `_shared_sig`).
- Implement per-module caches with size budgets mirroring:
  - algebra: `LOWMEM_CACHE_LIMIT_*` values
  - trig: shared budget + `enforce_total_cache_limit`
  - suvat/boolean: smaller
- Implement a lightweight cache eviction similar to `shared_cache.cache_store()` (drop ~1/8 oldest keys when limit exceeded).

### 4) Printers / exam output
Port `python/src/shared_formatting.py` functions:
- `format_equation_human_readable()` for compact readable output
- `format_exam_working()` which guarantees a final `Answer:` line

Ensure output policies match your programs:
- `log` displayed as `ln`
- `abs` sometimes printed as `|x|` depending on your rules
- Parentheses rules in `mul/add/div/pow`

### 5) Program modules (port order and specifics)
Port each program by translating its core public entrypoints (the ones used for UI) into C++ functions.

- **Boolean** (`python/src/ComputerScience/booleanProgram.py`)
  - Smallest, good first module.
  - Its AST is different (`("a"|"o"|"n"|"c"|"v", ...)`) so implement it as a separate mini-engine inside the same add-in.

- **SUVAT** (`python/src/Math/SUVATprogram.py`)
  - Mostly numeric/algebraic manipulation with your same tuple AST patterns.
  - Great early validation for printing + rational handling.

- **Derive** (`python/src/Math/deriveProgram.py`)
  - Implements differentiation rules + tidy/guardrails.
  - Keep your “exam guard” checks to prevent pathological expansions.

- **Integrate** (`python/src/Math/intProgram.py`)
  - Largest algorithmic surface area; includes method ladder and “non-elementary” explanations.

- **Algebra** (`python/src/Math/algebraProgram.py`)
  - Big rewrite/solve feature set; relies on strong simplification.

- **Trig** (`python/src/Math/trigProgram.py`)
  - Very large; many identity routes. Port last after core engine is stable.

Porting technique:
- For each module, create a C++ `*_engine.cpp` that mirrors the Python structure:
  - Keep function names and grouping similar so it’s mechanically verifiable.
  - Convert Python pattern-matching on tuples into `switch(kind)` logic.
  - Replace Python recursion with iterative forms where required to avoid stack issues.

## Testing strategy (how we ensure “no functionality loss”)
### 1) Create a golden-output corpus from the current Python
Use your existing Python code as oracle:
- Inputs should come from your existing `Tests.rtf` and/or `tests/` scripts.
- Build a script that runs each module on a large set of inputs and records:
  - normalized input
  - chosen mode
  - output lines
  - answer line
- Store as JSON fixtures in the repo.

### 2) Build a host (PC) build of the C++ engine
In fxSDK CMake project, add a second target that builds on the host compiler:
- Same core engine code compiled for macOS/Linux
- A CLI that takes `module + mode + input` and prints the same output format

Then CI-style compare:
- C++ host output vs Python golden fixtures
- Allow a small set of normalization tolerances if you intentionally change formatting

### 3) On-device correctness + performance checks
- Install the `.g3a` and run a smoke suite of representative problems.
- Track:
  - runtime per operation
  - peak memory usage (where feasible)
  - failure modes

## Size and memory risk management (specific to “one .g3a”)
### Primary track: keep under ~2 MiB
- Compile with size optimization (`-Os`) and link-time optimization if supported.
- Minimize heavyweight C++ features (exceptions/RTTI if they bloat; prefer `-fno-exceptions` only if compatible with libstdc++ usage).
- Avoid embedding giant static tables; generate identities algorithmically where possible.
- Keep assets small (icons only).

### Fallback track (if binary exceeds limit): external payload while keeping one visible icon
If you cannot fit:
- Keep the **main visible `.g3a`** under the limit, and store additional data in a separate file on storage (eg `.dat`).
- Load optional tables/route data at runtime.
- If even code must be split, accept a second file approach like KhiCAS50 (`.ac2`)—this technically violates “one `.g3a`”, so only do this if you explicitly allow it.

## Build + packaging steps (fxSDK-specific)
1. Create add-in scaffold:
   - `fxsdk new CasioCAS` (CMake)
2. Configure `CMakeLists.txt`:
   - add `find_module(Gint ...)` and link `Gint::Gint`
   - configure `GenerateG3A.cmake` usage so the build emits `.g3a`
3. Provide icons:
   - create unselected/selected 92×64 icons (fxgxa supports PNG)
4. Build:
   - `fxsdk build-cg`
5. Inspect `.g3a`:
   - `fxgxa -d your.g3a` to verify metadata, icon, checksum
6. Send/install:
   - copy to calculator’s root or appropriate folder, or `fxsdk send-cg`

## Implementation sequencing (how we’ll actually convert your repo)
### A) New add-in workspace inside your repo
- Add a new top-level directory under `python/` (or sibling to it) for the C++ add-in, eg `addin/`.
- Keep Python code unchanged as oracle.

### B) Build the C++ core library
- Add `core/` folder with:
  - `node.hpp/.cpp` (AST)
  - `rational.hpp/.cpp`
  - `normalize.cpp` (ported from `casio_core.normalize_text()` + `shared_helpers.normalize_input_text()`)
  - `tokenize.cpp`, `parse.cpp`
  - `sig.cpp`, `simplify.cpp`
  - `format.cpp` (ported from `shared_formatting.py`)
  - `cache.hpp` (bounded caches like `shared_cache.py`)

### C) Port modules one-by-one with golden tests
- `modules/boolean/` first, then `suvat`, then `derive`, `int`, `algebra`, `trig`.

### D) Build the UI shell
- `ui/menu.cpp`: top-level module selection
- `ui/input.cpp`: expression editor (cursor, insert/delete, parentheses help)
- `ui/output.cpp`: scrollable viewer for working lines

## Enhancements (safe, after parity)
Once parity is reached:
- Faster simplification (memoization, structural hashing)
- Better editor UX (syntax highlighting is likely too heavy; but bracket matching is doable)
- Optional numeric approximation toggle
- USB debug channel (fxlink) for logging difficult cases

## What not to port
- Desktop-only / test-only LLM integration (`python/src/shared_llm.py`) is explicitly PC-only.

## References used
- fxSDK README (install/build tools, fxgxa, fxconv, fxlink): `https://git.planet-casio.com/Lephenixnoir/fxsdk/src/branch/master/README.md`
- gint README (build-cg/install/linking overview): `https://git.planet-casio.com/Lephenixnoir/gint/src/branch/master/README.md`
- G3A format background (header layout, checksums, mapping): `https://bible.planet-casio.com/simlo/chm/v20/fxCG20_G3A.htm` and `https://prizm.cemetech.net/G3A_File_Format/`
- Memory/size discussions (practical CG50 constraints, extra RAM, 2 MiB limit): Planète Casio/Cemetech threads surfaced in research (eg `https://www.planet-casio.com/Fr/forums/topic18535-1-gint-programming-questions.html`, KhiCAS threads on Cemetech).
