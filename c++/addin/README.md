## fx-CG50 add-in notes

The production calculator build is the repo-root `./compile` flow, which builds the edited KhiCAS/libfxcg source and publishes transfer files under `calculator_files/`.

This folder keeps the fxSDK/gint fallback add-in plus the host/device solver code used by tests.

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

Default output is built from the edited KhiCAS source in `c++/khicas/upstream/giac90_1addin` and packaged with the Eigenmath-style icons from `c++/prizm/assets`. To package the unmodified upstream reference binary instead:

```bash
CASIO_PRIZM_MODE=khicas-reference ./c++/tools/build_addin_prizm_docker.sh
```

To build the previous small native port instead:

```bash
CASIO_PRIZM_MODE=legacy ./c++/tools/build_addin_prizm_docker.sh
```

The C++ compile script builds the edited KhiCAS-source PrizmSDK/libfxcg target for Casio fx-CG50 and writes `calculator_files/CasioCAS.g3a` plus `calculator_files/CASIOCAS.PAK`:

```bash
./compile
```

The TUI `/compile` command uses the same fx-CG50 target and does not modify emulator files.

### Install onto calculator

- **Manual (recommended)**:
  - Plug calculator in via USB, choose “USB storage” mode on-device.
  - Copy `calculator_files/CasioCAS.g3a` and `calculator_files/CASIOCAS.PAK` to calculator storage root.
  - Eject/unmount safely, then exit USB mode on the calculator.
- **fxSDK send helper** (Linux + UDisks2): `fxsdk send-cg`

After installing, use the on-device checklist:

- `c++/addin/DEVICE_SMOKE_CHECKLIST.md`

And enforce the size gate:

```bash
python3 c++/tools/check_g3a_size.py c++/prizm/build/CasioCAS.g3a
```

### Notes for this project

- The active production path is one visible `.g3a` plus the external `CASIOCAS.PAK` help/template pack.
- We keep a **host build** of the core engine for fast correctness testing with frozen C++ fixtures.
- The legacy/fallback device build uses a bounded freestanding solver slice for:
  - one-variable rational polynomial parsing with brackets, powers, implicit multiplication, expansion, and rearranged equations
  - linear/quadratic solving plus small integer-root factor-theorem support for higher polynomials
  - polynomial derivative/integral rules after expansion/collection
  - exact common-angle trig, simple trig identities, and single-function trig equations such as `2sin(x)+1=0`
  - selected SUVAT rearrangements with working lines
- The add-in UI is intentionally dense and KhiCAS-inspired: top status strip,
  blue section labels, black active rows, right-hand scrollbars, and bottom
  soft-key labels. The home screen also supports `F1`-`F6` quick launches.
- Calculator fixes must be mirrored into the edited KhiCAS source path; host-only fixes do not prove `.g3a` behavior.
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
