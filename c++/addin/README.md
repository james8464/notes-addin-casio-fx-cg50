## fx-CG50 add-in (g3a) — native port target

This folder contains the **native** fx-CG50 add-in built with **fxSDK + gint**.

### Build prerequisites (one-time)

Install **fxSDK** (which pulls in the SuperH toolchain, gint, fxlibc, OpenLibm, etc.).

Authoritative docs:
- fxSDK: `https://git.planet-casio.com/Lephenixnoir/fxsdk/src/branch/master/README.md`
- gint: `https://git.planet-casio.com/Lephenixnoir/gint/src/branch/master/README.md`

This repo also includes a concrete install/verify guide: `c++/addin/FXSDK_INSTALL.md`.

Typical workflow (Linux/WSL is the smoothest; macOS may need tweaks):
- Install via GiteaPC (recommended by fxSDK): `giteapc install Lephenixnoir/fxsdk`
- Ensure `fxsdk` is on PATH.

Quick verify:

```bash
fxsdk --version
fxsdk new -t addin-cg50 /tmp/fxsdk-smoke && rm -rf /tmp/fxsdk-smoke
```

### Build the add-in

From this directory:

```bash
fxsdk build-cg
```

Or from repo root:

```bash
./c++/tools/build_addin.sh
```

This should produce a `.g3a` under `c++/addin/build-cg/` (the script prints the exact path).

### Build native Prizm UI on macOS with Docker (recommended)

Use the PrizmSDK/libfxcg target for the calculator-style UI:

```bash
./c++/tools/build_addin_prizm_docker.sh
```

Default output is copied from the verified upstream KhiCAS reference for exact UI/menu parity. To build the previous small native port instead:

```bash
CASIO_PRIZM_MODE=legacy ./c++/tools/build_addin_prizm_docker.sh
```

The test TUI defaults to this target and syncs the result to the CASIO emulator SD card:

```bash
CASIO_BACKEND=c python3 python/tests/run_tests.py c compile
```

Default emulator target:

```text
~/Library/Application Support/CASIO/fx-CG Manager PLUS Subscription for fx-CG50series/fx-CG50/EmulatorData/SDCard
```

Set `CASIO_SYNC_EMULATOR=0` to skip this copy, or set `CASIO_EMULATOR_SDCARD_DIR` to override the target.
Set `CASIO_BUILD_ADDIN=docker` only if you need the older fxSDK/gint target.

### Install onto calculator

- **Manual (recommended)**:
  - Plug calculator in via USB, choose “USB storage” mode on-device.
  - Copy the built `.g3a` to the calculator storage (root is fine).
  - Eject/unmount safely, then exit USB mode on the calculator.
- **fxSDK send helper** (Linux + UDisks2): `fxsdk send-cg`

After installing, use the on-device checklist:

- `c++/addin/DEVICE_SMOKE_CHECKLIST.md`

And enforce the size gate:

```bash
python3 c++/tools/check_g3a_size.py c++/addin/build-cg/*.g3a
```

### Notes for this project

- The long-term goal is a single `.g3a` exposing the same functionality as the Python engines in `python/src/`.
- We will keep a **host build** of the core engine for fast correctness testing (golden fixtures generated from the current Python outputs).
- The current device build uses a bounded freestanding solver slice for:
  - one-variable rational polynomial parsing with brackets, powers, implicit multiplication, expansion, and rearranged equations
  - linear/quadratic solving plus small integer-root factor-theorem support for higher polynomials
  - polynomial derivative/integral rules after expansion/collection
  - exact common-angle trig, simple trig identities, and single-function trig equations such as `2sin(x)+1=0`
  - selected SUVAT rearrangements with working lines
- The add-in UI is intentionally dense and KhiCAS-inspired: top status strip,
  blue section labels, black active rows, right-hand scrollbars, and bottom
  soft-key labels. The home screen also supports `F1`-`F6` quick launches.
- Richer host functionality remains available through `casio_host` while device features are ported module by module.
- Architecture graph: `c++/GRAPH.md`.

### Host build (fast local testing)

This repo installs `cmake` via pip (`--user`) on macOS, so `cmake` may live at `$(python3 -c 'import site; print(site.USER_BASE)')/bin`.

Build the host runner from the repo root:

```bash
./c++/tools/build_host.sh
./c++/addin/host/build/casio_host --alg "2x+3=7"
./c++/addin/host/build/casio_host --int "sin(3x+2)"
./c++/addin/host/build/casio_host --trig "cos(5*pi/3)"
```
