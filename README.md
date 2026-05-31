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
- size: `2,097,140` bytes
- hard limit headroom: `12` bytes
- sha256: `d3dbb97550c0223bee8b8098ee61ae2d821e9904ce571a2632499d8fb80532b5`
- exact queue runtime: `13,558/13,558`
- strict marker quality: `12,418/13,558`
- online challenge source coverage: MadAsMaths exact rows in queue; Daily Integral challenge format inspected from `https://dailyintegral.com/play/hard`

Notable routes:

- `integrate((ln(x))^2,x,2)` repeated integration by parts
- `integrate((ln(x))^2/x,x,3,u=ln(x))` substitution
- `solve((dy)/(dx)=y,y)` separable differential equation
- generic affine chain/reverse-chain power routes
- trig/exp term integration, quadratic solve/factor/expand, log/numeric routes
- catalogue Help on command screen shows spaced sections and F2/F3 examples
- `tools/audit_progress_tui.py` shows queue done/right progress bars during full runs
