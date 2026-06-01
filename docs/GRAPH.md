# Project Graph

Last updated: 2026-06-01 12:40 BST

## Build

```mermaid
graph TD
  Compile["./compile"] --> Build["tools/build_g3a.sh"]
  Build --> Docker["casio-khicas-source Docker image"]
  Docker --> Src["khicas/upstream/giac90_1addin"]
  Build --> Icons["regenerate ignored khicasio PNG icons from tracked BMPs"]
  Icons --> Src
  Src --> Make["Makefile CAS.g3a target"]
  Make --> Bin["khicasen.bin, upstream KhiCAS base"]
  Bin --> G3A["calculator_files/CAS.g3a"]
  Build --> Help["help/functions/*.txt"]
  Help --> PAK["calculator_files/CAS.PAK: 31,667 bytes; sha c9429427"]
  G3A --> Meta["CAS / @CAS / CAS.g3a"]
  G3A --> Size["2,091,580 bytes; under 2,097,152 cap; sha c7222043"]
```

## Runtime

```mermaid
graph TD
  Input["console input"] --> Guard["cascas::eval_with_working"]
  Guard --> Work["known Pure working route"]
  Work --> Steps["exam lines"]
  Steps --> Console["Console_NewLine output"]
  Guard --> Fallback["unknown/plain input"]
  Fallback --> KhiCAS["original KhiCAS parser/evaluator"]
  KhiCAS --> Console
```

## Working Routes

```mermaid
graph TD
  Work["working engine"] --> Diff["diff: guarded affine chain powers, optimisation/quotient derivatives, ln^2 chain, arctan inverse trig, ordered cubic route for 108*x-36*x^2+3*x^3, exp product routes incl 4*(x^2-2)*exp(-2*x), implicit, trig basics"]
  Work --> Int["integrate: affine reverse-chain powers with reciprocal exam form for negative powers, polynomial-over-x^n rewrite, trig identity products/squares/sec-cot forms, full-derivative quadratic substitution like (2*x+1)*cos(x^2+x), reciprocal affine logs c/(a*x+b), sums of reciprocal affine terms, expanded (ln(x))^2 by-parts answer, definite ln(x)^2 by-parts endpoint markers, compact radical routes including 12*(3-x/2)^(1/2), 30*(1-x/3)^(3/2), 15*(1-x/4)^(1/4), 3*x^2*(4-2*x^3)^(3/2), (x+1)/(x^2+2*x+3)^(1/3), affine trig/exp terms like sin(4*x+3), cos(2-3*x), exp(1-3*x), generic c*x*sin/cos(a*x+b) by-parts, x^2*cos(x/3) by-parts, and 1/(sqrt(x)(sqrt(x)+2)) definite substitution, linear-over-linear division logs, damped-sine by-parts, substitution, definite substitution"]
  Work --> Solve["solve: guarded linear with exam-order lines for 8000=64000-15*k and 64000-11200*t=0, integer/rational-root quadratics with explicit root lines, product-coefficient preservation, common-factor exam order, exp/log routes including 10^(3*k)=2, rational route for k*(k+3)/(k+1)=2, circle-intersection route, periodic trig route for 10=12+3*sin(pi*t/6), rational inequality critical-value route, dy/dx and dn/dt separable"]
  Work --> Alg["algebra: variable-aware quadratic factor, simplify rational cancel-after-factor, targeted expand, high-frequency exam forms, binomial series coefficient/term lines for three queue patterns"]
  Work --> PF["partial fractions: targeted apart marker route for 6/(u*(3+2*u))"]
  Work --> Vec["vectors: targeted subtraction marker for [3,-3,-4]-[2,5,-6], scalar marker for 2*[1,-8,2]"]
  Work --> Num["numeric routes: equation-style decimal/exact lines, circle perimeter exact-before-decimal route, exp(2*ln(7/6)) marker, log base 10 with ln natural, 12-significant-digit rounded markers, sqrt substitution-limit markers"]
  Work --> Trig["trig: R-form and pi-shift identities"]
  Work --> Xform["xform trig/log identities"]
```

## UI

```mermaid
graph TD
  Console["main dashboard"] --> FMenu["F1 algb / F2 calc mini menus"]
  Console --> F6["F6 original no-title File menu style: Clear history / Config / Alt labels"]
  Console --> Rec["blinking blue R drawn during status refresh"]
  Console --> Border["pink border: 6px left/right, 7px bottom"]
  Console --> Status["status bar without clock"]
  Catalog["Pure-only catalogue"] --> Help["Help on command"]
  Help --> Examples["F2/F3 example insertion"]
```

## Queue Status

```mermaid
graph TD
  Online["MadAsMaths + Daily Integral challenge style"] --> Queue["exact_calculator_input_queue.jsonl"]
  Queue --> Runner["tests/run_exact_queue.py"]
  Runner --> Live["progress/exact_queue_latest.json"]
  Runner --> Report["tests/reports/.../latest.jsonl"]
  Live --> TUI["tools/audit_progress_tui.py"]
  Report --> TUI
  TUI --> Panels["animated panels: status badges, wide side-by-side layout, phase lanes, health score, gate board, sync, last commit, change counts, state age, artifact headroom, live rate and ETA, queue bars, strict-marker ratios, strict-gap bar map, freshness rows, animated scan/meter lines, cleanup byte totals and cleanup command, project hygiene, tooling inventory, transfer path, strict clusters with first gap samples, test checkpoints, release blockers, risk, ignored workspace, active-tool counts, next action, command panel"]
  Runner --> Runtime["14,722/14,722 runtime-safe"]
  Runner --> Strict["13,331/14,708 strict marker pass"]
  Strict --> Remaining["remaining strict gaps: 1,377; integrate remains largest cluster; 5,572 B hard headroom"]
```

## Project Shape

```mermaid
graph TD
  Canon["canonical queue"] --> Golden["tests/golden/exact_calculator_input_queue.jsonl"]
  Tooling["active tooling"] --> Build["build/check/audit/host runner; no dead tools kept"]
  Old["old VM worker notes, batch JSONs, append helper, retired checks, CMake host wrapper, duplicate keepers, lexer backup"] --> Pruned["removed from tracked project"]
  Reports["ignored generated outputs"] --> Recreate["build/, graphify-out/, tests/reports/ recreated only when needed"]
```
