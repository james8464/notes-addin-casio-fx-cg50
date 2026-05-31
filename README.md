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
- size: `2,095,616` bytes
- hard limit headroom: `1,536` bytes
- sha256: `f0bff7bf6949bc9e8c12316e06346b902ee958ec532e0f75b7244cf51ed4688d`
- exact queue runtime: `13,572/13,572`
- strict marker quality: `12,638/13,572`
- online challenge source coverage: MadAsMaths exact rows in queue; Daily Integral challenge format inspected from `https://dailyintegral.com/play/hard`

Notable routes:

- `integrate((ln(x))^2,x,2)` repeated integration by parts
- `integrate((ln(x))^2/x,x,3,u=ln(x))` substitution
- `solve((dy)/(dx)=y,y)` separable differential equation
- generic affine chain/reverse-chain power routes
- trig/exp term integration, quadratic solve/factor/expand, log/numeric routes
- safer solve routing: powered terms no longer fall through the linear solver
- simple numeric expressions show small exact fractions when detected
- catalogue Help on command screen shows spaced sections and F2/F3 examples
- `tools/audit_progress_tui.py` shows queue done/right progress bars during full runs
