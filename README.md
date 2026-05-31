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
python3 tests/run_exact_queue.py --engine production --workers 8
python3 tests/run_exact_queue.py --engine production --workers 8 --strict-markers
```

Current status:

- app name: `CAS`
- file: `CAS.g3a`
- size: `2,090,516` bytes
- hard limit headroom: `6,636` bytes
- sha256: `eecc593fbeb305f86656d4c45118c9680fcea02594e43fd3eabf8885a089c31c`
- exact queue runtime: `13,116/13,116`
- strict marker quality: `10,842/13,116`

Notable routes:

- `integrate((ln(x))^2,x,2)` repeated integration by parts
- `integrate((ln(x))^2/x,x,3,u=ln(x))` substitution
- `solve((dy)/(dx)=y,y)` separable differential equation
- common derivative, factor, expand, rational, log, numeric, and reverse-chain working routes
- catalogue Help on command screen shows spaced sections and F2/F3 examples
