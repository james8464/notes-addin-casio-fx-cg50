# CAS fx-CG50 Pure Build

Artifact:

- `/Users/james/Developer/CASIO/calculator_files/CAS.g3a`
- no `.ac2`
- no external `.pak`

Build:

```bash
./compile
python3 tools/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a
python3 tools/check_g3a_size.py calculator_files/CAS.g3a
python3 tools/check_catalog_scope.py
python3 tools/check_calculator_border.py calculator_files/CAS.g3a
python3 tests/check_help_examples.py
python3 tests/run_exact_queue.py --engine production --workers 8
python3 tests/run_exact_queue.py --engine production --workers 8 --strict-markers
python3 tools/audit_progress_tui.py --fps 12
```

Current status:

- app name: `CAS`
- file: `CAS.g3a`
- size: `2,097,108` bytes
- hard limit headroom: `44` bytes
- sha256: `f275da8b28ab40d6edd74d17bfc119eec4def640486c8ff9ac0b42543f496cb4`
- exact queue runtime: `14,256/14,256`
- strict marker quality: `13,012/14,256`
- online challenge source coverage: MadAsMaths exact rows in queue; Daily Integral hard-integration style probes inspected from `https://dailyintegral.com/archive`

Notable routes:

- `integrate((ln(x))^2,x,2)` repeated integration by parts
- `integrate((ln(x))^2,x)` returns the expanded mark-scheme form `x*ln(x)^2 - 2*x*ln(x) + 2*x + C`
- `defint(ln(x)^2,x,2,4),method=parts` shows by-parts setup, antiderivative, and endpoint substitution markers
- `integrate(12*(3-x/2)^(1/2),x)` compact reverse-chain radical route
- `integrate(30*(1-x/3)^(3/2),x)` and `15*(1-x/4)^(1/4)` compact reverse-chain radical routes
- `integrate((ln(x))^2/x,x,3,u=ln(x))` substitution
- `integrate(2*x/(x^2+4))` substitution
- `integrate(cos(x)^4*sin(x))` substitution
- `integrate(x*exp(2*x))` integration by parts
- `integrate(x*sin(4*x))` compact integration by parts
- `diff(r^2,r)` single-variable power rule
- `diff(arctan(x))` compact inverse trig derivative route
- `diff((ln(x))^2)`, `diff(x*exp(-2*x))`, and `diff(4*(x^2-2)*exp(-2*x))` compact exam routes
- optimisation derivative routes for `1/2*x^2+16*sqrt(2)/x` and `x-16*sqrt(2)*x^-2`
- quotient-simplify derivative route for `(x^2+4)/(4*x)`
- `solve((dy)/(dx)=y,y)` separable differential equation
- `solve(dn/dt=k*n,n,t)` now shows the separation step and spaced logarithm line
- `solve(1/4-1/x^2>0,x)` shows the critical values `N = 0: x = -2, 2` and final interval `x < -2 or x > 2`
- `solve(10^(3*k)=2,k)` shows the log-route exact answer `k = [ln(2)/(3*ln(10))]`
- `solve(10=12+3*sin(pi*t/6),t)` shows the periodic trig route `u = -2/3` and both period-12 branches
- `solve(4-exp(2*x)=2,x)` and `solve((2-exp(2*x))^2=0,x)` show the logarithm step result `x = [1/2*ln(2)]`
- generic affine chain/reverse-chain power routes
- trig/exp term integration, shifted trig identity, damped-sine by-parts route, quadratic solve/factor/expand, log/numeric routes
- safer solve routing: powered terms no longer fall through the linear solver
- low-quality generic `Poly: Factor, set=0` fallback was removed so unknown polynomial solves can fall through to original KhiCAS instead
- safer chain routing: non-linear inner functions no longer pass as affine
- numeric `log(x)` now follows A-level/Casio base-10 convention while `ln(x)` remains natural log
- simple numeric expressions show small exact fractions when detected
- numeric routes emit equation-style `=` lines, 12-significant-digit rounded markers, and substitution-limit square-root steps such as `sqrt(5-1) -> sqrt(4)` without duplicate fixed-decimal spam
- polynomial antiderivatives use coefficient-first descending-power form, e.g. `-1/3*x^3 + 2*x^2 + 5*x + C`
- `integrate(x^2-1,x)` uses the mark-scheme form `x^3/3 - x + C`
- `integrate(x*sqrt(x+1),x,0,3)` uses substitution and returns `116/15`
- linear-over-linear integrals such as `integrate((4*x+3)/(2*x-1),x)` use algebraic division and return `2*x + 5/2*ln(abs(2*x - 1)) + C`
- polynomial derivatives use descending-power form, e.g. `-8*x^3 + 1`
- repeated integer quadratic roots print once, e.g. `x = [8]`
- distinct integer quadratic roots show explicit root lines before list answer, e.g. `k = 1 or k = -2`
- negative-leading integer quadratics now print roots in the exam-friendly order expected by the queue, e.g. `x = [3, 11]`
- catalogue Help on command screen shows spaced sections and F2/F3 examples
- `/Users/james/Developer/CASIO/tools/audit_progress_tui.py` shows animated status badges, side-by-side panels on wide terminals, phase lanes, health score, repo sync, last commit, change counts, state age, artifact headroom, live queue rate/ETA, pass/fail bars, animated scan lines, cleanup byte totals, project hygiene, transfer path, quality clusters, release blockers, risk, ignored workspace, active-tool counts, next action, recent events, and run-command panels

Active tools:

- `tools/build_g3a.sh` regenerates ignored KhiCAS icon PNGs from tracked BMPs before Make runs
- `tools/audit_progress_tui.py`
- `tools/khicas_host_runner`
- `tools/check_g3a_metadata.py`
- `tools/check_g3a_size.py`
- `tools/check_calculator_border.py`
- `tools/check_catalog_scope.py`
- `tools/check_help_quality.py`

All current `tools/` files are referenced by build, tests, audit, or docs. Historical worker notes, batch JSONs, stale append helpers, retired checks, and the old CMake host wrapper were pruned. The canonical test source is now `tests/golden/exact_calculator_input_queue.jsonl`.
Empty `.gitkeep` placeholders were removed from non-empty tracked folders. Active `tools/` scripts are all still referenced by build, tests, or audit docs.
Generated caches are ignored and were removed from the working tree; transfer/report artifacts are kept ignored because they are used for calculator transfer and live audit status.
