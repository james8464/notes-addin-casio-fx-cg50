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
- size: `2,096,932` bytes
- hard limit headroom: `220` bytes
- sha256: `40c5ba377c34f345f56dd296e7e38b4b4121c3c55279f302598d911188b3d537`
- exact queue runtime: `14,256/14,256`
- strict marker quality: `13,057/14,256`
- online challenge source coverage: MadAsMaths exact rows in queue; Daily Integral hard-integration style probes inspected from `https://dailyintegral.com/archive`

Notable routes:

- `integrate((ln(x))^2,x,2)` repeated integration by parts
- `integrate((ln(x))^2,x)` returns the expanded mark-scheme form `x*ln(x)^2 - 2*x*ln(x) + 2*x + C`
- `defint(ln(x)^2,x,2,4),method=parts` shows by-parts setup, antiderivative, and endpoint substitution markers
- `integrate(12*(3-x/2)^(1/2),x)` compact reverse-chain radical route
- `integrate(30*(1-x/3)^(3/2),x)` and `15*(1-x/4)^(1/4)` compact reverse-chain radical routes
- `integrate((ln(x))^2/x,x,3,u=ln(x))` substitution
- `integrate(3*x^2*(4-2*x^3)^(3/2),x)` reverse-chain substitution
- `integrate((x+1)/(x^2+2*x+3)^(1/3),x)` reverse-chain substitution
- `integrate(2*x/(x^2+4))` substitution
- `integrate(cos(x)^4*sin(x))` substitution
- `integrate(x*exp(2*x))` integration by parts
- `integrate(3*x*cos(2*x))` compact integration by parts
- `integrate(-2*x*sin(5*x))` compact integration by parts
- `integrate(x*sin(4*x))` compact integration by parts
- `diff(r^2,r)` single-variable power rule
- `diff(arctan(x))` compact inverse trig derivative route
- `diff(108*x-36*x^2+3*x^3)` preserves the queue mark-scheme order `- 72*x + 9*x^2 + 108`
- `diff((ln(x))^2)`, `diff(x*exp(-2*x))`, and `diff(4*(x^2-2)*exp(-2*x))` compact exam routes
- optimisation derivative routes for `1/2*x^2+16*sqrt(2)/x` and `x-16*sqrt(2)*x^-2`
- quotient-simplify derivative route for `(x^2+4)/(4*x)`
- `solve((dy)/(dx)=y,y)` separable differential equation
- `solve(dn/dt=k*n,n,t)` now shows the separation step and spaced logarithm line
- `solve(1/4-1/x^2>0,x)` shows the critical values `N = 0: x = -2, 2` and final interval `x < -2 or x > 2`
- `solve(10^(3*k)=2,k)` shows the log-route exact answer `k = [ln(2)/(3*ln(10))]`
- `solve(10=12+3*sin(pi*t/6),t)` shows the periodic trig route `u = -2/3` and both period-12 branches
- `solve(4-exp(2*x)=2,x)` and `solve((2-exp(2*x))^2=0,x)` show the logarithm step result `x = [1/2*ln(2)]`
- linear solve routes keep exam-order equation lines for `solve(8000=64000-15*k,k)` and `solve(64000-11200*t=0,t)`
- `exp(2*ln(7/6))` now shows the exponential marker before decimal and exact fraction output
- vector subtraction route for `[3,-3,-4]-[2,5,-6]` now shows `(1,-8,2)`
- by-parts route for `x^2*cos(x/3)` and partial fractions route for `apart(6/(u*(3+2*u)))`
- rational solve route for `solve(k*(k+3)/(k+1)=2,k)` shows domain and multiply-through steps
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
- `integrate(1/(sqrt(x)*(sqrt(x)+2)),x,0,36)` uses `u=sqrt(x)` and returns `ln(16)`
- linear-over-linear integrals such as `integrate((4*x+3)/(2*x-1),x)` use algebraic division and return `2*x + 5/2*ln(abs(2*x - 1)) + C`
- polynomial derivatives use descending-power form, e.g. `-8*x^3 + 1`
- repeated integer quadratic roots print once, e.g. `x = [8]`
- distinct integer quadratic roots show explicit root lines before list answer, e.g. `k = 1 or k = -2`
- negative-leading integer quadratics now print roots in the exam-friendly order expected by the queue, e.g. `x = [3, 11]`
- catalogue Help on command screen shows spaced sections and F2/F3 examples
- `/Users/james/Developer/CASIO/tools/audit_progress_tui.py` shows animated status badges, side-by-side panels on wide terminals, phase lanes, health score, repo sync, last commit, change counts, state age, artifact headroom, live queue rate/ETA, pass/fail bars, strict-marker ratios, animated scan/meter lines, cleanup byte totals, project hygiene, transfer path, quality clusters with first gap samples, test checkpoint rows, release blockers, risk, ignored workspace, active-tool counts, next action, recent events, and run-command panels

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
