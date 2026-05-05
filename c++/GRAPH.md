# CasioCAS Project Graph

Use this file as the compact AI map. Prefer this over rereading the repo from
scratch.

## Build Paths

```mermaid
graph TD
  Root["repo root"] --> Compile["./compile"]
  Compile --> Docker["Docker: casio-khicas-source"]
  Docker --> Src["c++/khicas/upstream/giac90_1addin"]
  Src --> G3A["c++/prizm/build/CasioCAS.g3a"]
  Assets["c++/prizm/assets/*.png"] --> Src
  HLP["c++/prizm/help/CASIOCAS.HLP"] --> Transfer
  G3A --> Patch["patch_g3a_metadata.py"]
  Patch --> Transfer["calculator_files/"]

  Legacy["CASIO_PRIZM_MODE=legacy"] --> Prizm["c++/prizm/src + c++/addin/src/device"]
  Ref["CASIO_PRIZM_MODE=khicas-reference"] --> UpG3A["khicas/reference/khicasen.g3a"]
```

Patch production calculator behavior in:
- `c++/khicas/upstream/giac90_1addin/main.cc`
- `c++/khicas/upstream/giac90_1addin/catalogen.cpp`
- `c++/prizm/help/CASIOCAS.HLP` for offloaded F6 help text

Patch host/golden behavior in:
- `c++/addin/src/core/*`
- `c++/addin/src/modules/*`
- `c++/addin/src/device/*`

Important: host module fixes are not automatically calculator fixes unless
mirrored into the KhiCAS source path.

## Calculator Runtime

```mermaid
graph TD
  UI["KhiCAS shell/UI"] --> Cat["catalogen.cpp"]
  Cat --> All["All catalogue filtered by hidden rules"]
  Cat --> Help["F6 help: static + CASIOCAS.HLP"]
  Cat --> Method["diff/int method popup"]
  Method --> Insert["insert command shell text"]

  Shell["main.cc input"] --> Rewrite["alias/method rewrite"]
  Rewrite --> Giac["Giac exact engine"]
  Giac --> Ans["answer string"]
  Ans --> Gate["show working? old Python scope only"]
  Gate --> Sink["cascas_working_sink capped lines/chars"]
  Sink --> WorkScreen["fullscreen working"]
  Sink --> History["shell result lines"]
```

Working output scope:
- yes: `diff`, `implicit_diff`, `param_diff`, `integrate`, `defint`, `de_solve`,
  `solve`, `solve_trig`, `domain`, `range`, `compare`, `rewrite`, `fitconst`,
  Python-parity maths helpers
- no/answer-first: most raw KhiCAS commands such as `tcollect`, `texpand`,
  `factor`, `simplify`, unless wrapped by project routes

## Host Test Engine

```mermaid
graph TD
  Host["c++/addin/host/casio_host"] --> Core["core: tokenize/parse/simplify/format"]
  Core --> Mods["modules"]
  Mods --> Alg["algebra"]
  Mods --> Der["derive"]
  Mods --> Int["integrate"]
  Mods --> Trig["trig"]
  Mods --> Stats["stats"]
  Mods --> Suv["suvat"]
  Mods --> Bool["boolean"]
  Host --> Out["exam-style text"]

  Smoke["device_solver_smoke"] --> Dev["device_solver.cpp"]
  Dev --> Fixed["fixed buffers / no heap-heavy STL"]
```

Host output quality rules:
- final answer line should be maths only
- no `Chk:`, `Answer: int(...)`, `Answer: d/dx(...)`, parser tracebacks, or
  generic calculator-debug text
- full working only where project scope requires it

## Working Logic

```mermaid
graph TD
  In["input"] --> Norm["normalise symbols/forms"]
  Norm --> Class["classify feature + method"]
  Class --> Auto["auto route"]
  Class --> Forced["forced method route"]
  Auto --> Exact["Giac/host exact answer"]
  Forced --> Exact
  Exact --> Steps["exam_work / cascas_working_text"]
  Steps --> Verify["symbolic/numeric/LLM gates"]
  Verify --> Out["working + final answer"]
```

High-value step generators:
- algebra: expand, factor, collect, complete square, clear denominators,
  equate coefficients, partial fractions
- differentiation: first principles, chain, product, quotient, logdiff,
  implicit, parametric, second derivative
- integration: direct, reverse chain, substitution, parts, DI/tabular,
  partial fractions, trig identities, Weierstrass, symmetry, DE separation
- trig: sin/cos/tan rewrites, R-form, sum/product identities, power reduction,
  bounded/general solution checks
- domain/range: denominator/radical/log/inverse-trig guards, sampling only as
  support

## Test Gates

```mermaid
graph TD
  Run["python3 c++/tools/run_tests_cpp.py"] --> BuildHost["build_host.sh"]
  BuildHost --> Checks["checks + golden bank"]
  Checks --> MP2["mp2_ab..vwx manual goldens"]
  Checks --> Stress["centurion/rigor/singularity/stress files"]
  Checks --> Fuzz["fuzz regressions"]
  Checks --> Markers["g3a metadata/working markers if built"]

  TUI["python3 run_tests.py tui"] --> Random["/random"]
  TUI --> Infinite["/infinite"]
  TUI --> LLM["/llm 2"]
  Random --> Engine["random_engine"]
  Infinite --> Engine
  Engine --> Reports["c++/tests/reports/adversarial_runs/*"]
```

Quick commands:

```bash
./c++/tools/build_host.sh
python3 c++/tools/run_tests_cpp.py
./compile
python3 run_tests.py tui
```

## Adversarial Random Engine

```mermaid
graph TD
  Manifest["All catalogue manifest"] --> Graph["ConceptGraph"]
  Graph --> Sched["coverage scheduler"]
  Sched --> Gen["seeded solvable kernel"]
  Gen --> Trans["hostile transforms"]
  Trans --> Case["casio_host command"]
  Case --> Oracle["SymPy/metamorphic/heuristic"]
  Oracle --> LLM["exam-marker LLM optional"]
  LLM --> Report["case_id JSON + replay_index"]
  Report --> Graph
```

Node key:
`function | parameter_signature | method | topic | transform_chain | difficulty | oracle`

Random graph resets each `/random` or `/infinite` session.

## ROM / Storage

```mermaid
graph TD
  ROM["2MB practical g3a pressure"] --> Keep["core CAS + exam routes"]
  ROM --> Offload["external CASIOCAS.HLP"]
  ROM --> Hide["catalogue hide rules"]
  ROM --> Prune["link/object pruning only after compile gate"]
  Keep --> Calc["CasioCAS.g3a"]
  Offload --> Files["calculator_files/CASIOCAS.HLP"]
```

Safe-ish size levers:
- keep verbose help/examples in `CASIOCAS.HLP`
- hide/remove non-scope UI surfaces first
- remove linked legacy objects only one at a time with full gates
- preserve core Giac paths used by solve/diff/int/trig/matrices/stats

## Patch Rules

```mermaid
graph TD
  Bug["weak working / wrong answer"] --> Repro["host command or TUI case_id"]
  Repro --> Test["add/extend golden or random-engine test"]
  Test --> PatchHost["patch addin/src if host issue"]
  Test --> PatchCalc["mirror in main.cc/catalogen.cpp if device issue"]
  PatchHost --> Gate["run_tests_cpp.py"]
  PatchCalc --> Compile["./compile + g3a checks"]
  Gate --> Commit["git commit"]
  Compile --> Commit
```

Never claim calculator behavior from host-only evidence. If the `.g3a` matters,
run `./compile` and `check_g3a_*`.
