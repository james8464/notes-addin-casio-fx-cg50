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
python3 tools/audit_progress_tui.py --fps 8
```

Current status:

- app name: `CAS`
- file: `CAS.g3a`
- size: `2,096,596` bytes
- hard limit headroom: `556` bytes
- sha256: `2c3ccceb2e5bba16c6a12260a56cfff06215b787d83fd7a3acfcd3f9c1222616`
- exact queue runtime: `13,592/13,592`
- strict marker quality: `12,658/13,592`
- online challenge source coverage: MadAsMaths exact rows in queue; Daily Integral hard-integration style probes inspected from `https://dailyintegral.com/archive`

Notable routes:

- `integrate((ln(x))^2,x,2)` repeated integration by parts
- `integrate((ln(x))^2/x,x,3,u=ln(x))` substitution
- `integrate(2*x/(x^2+4))` substitution
- `integrate(cos(x)^4*sin(x))` substitution
- `integrate(x*exp(2*x))` integration by parts
- `diff(r^2,r)` single-variable power rule
- `solve((dy)/(dx)=y,y)` separable differential equation
- generic affine chain/reverse-chain power routes
- trig/exp term integration, quadratic solve/factor/expand, log/numeric routes
- safer solve routing: powered terms no longer fall through the linear solver
- safer chain routing: non-linear inner functions no longer pass as affine
- simple numeric expressions show small exact fractions when detected
- catalogue Help on command screen shows spaced sections and F2/F3 examples
- `tools/audit_progress_tui.py` shows queue done/right progress bars during full runs
