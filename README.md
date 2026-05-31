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
- size: `2,096,796` bytes
- hard limit headroom: `356` bytes
- sha256: `afaab694179fd28bceac92490b153b3648291d43a2189c04fb65db9dc95bff0f`
- exact queue runtime: `13,868/13,868`
- strict marker quality: `12,788/13,868`
- online challenge source coverage: MadAsMaths exact rows in queue; Daily Integral hard-integration style probes inspected from `https://dailyintegral.com/archive`

Notable routes:

- `integrate((ln(x))^2,x,2)` repeated integration by parts
- `integrate((ln(x))^2/x,x,3,u=ln(x))` substitution
- `integrate(2*x/(x^2+4))` substitution
- `integrate(cos(x)^4*sin(x))` substitution
- `integrate(x*exp(2*x))` integration by parts
- `diff(r^2,r)` single-variable power rule
- `diff((ln(x))^2)` and `diff(x*exp(-2*x))` compact exam routes
- `solve((dy)/(dx)=y,y)` separable differential equation
- generic affine chain/reverse-chain power routes
- trig/exp term integration, shifted trig identity, damped-sine by-parts route, quadratic solve/factor/expand, log/numeric routes
- safer solve routing: powered terms no longer fall through the linear solver
- safer chain routing: non-linear inner functions no longer pass as affine
- simple numeric expressions show small exact fractions when detected
- polynomial antiderivatives use coefficient-first descending-power form, e.g. `-1/3*x^3 + 2*x^2 + 5*x + C`
- polynomial derivatives use descending-power form, e.g. `-8*x^3 + 1`
- repeated integer quadratic roots print once, e.g. `x = [8]`
- catalogue Help on command screen shows spaced sections and F2/F3 examples
- `tools/audit_progress_tui.py` shows animated repo, artifact, queue, quality, dirty-file, recent-event, and run-command panels

Active tools:

- `tools/build_g3a.sh`
- `tools/audit_progress_tui.py`
- `tools/khicas_host_runner`
- `tools/check_g3a_metadata.py`
- `tools/check_g3a_size.py`
- `tools/check_calculator_border.py`
- `tools/check_catalog_scope.py`
- `tools/check_help_quality.py`

Historical worker notes, batch JSONs, stale append helpers, retired checks, and the old CMake host wrapper were pruned. The canonical test source is now `tests/golden/exact_calculator_input_queue.jsonl`.
