## fx-CG50 add-in (g3a) — native port target

This folder is the start of a **native** fx-CG50 add-in built with **fxSDK + gint**.

### Build prerequisites (one-time)

Install **fxSDK** (which pulls in the SuperH toolchain, gint, fxlibc, OpenLibm, etc.).

Authoritative docs:
- fxSDK: `https://git.planet-casio.com/Lephenixnoir/fxsdk/src/branch/master/README.md`
- gint: `https://git.planet-casio.com/Lephenixnoir/gint/src/branch/master/README.md`

This repo also includes a concrete install/verify guide: `addin/FXSDK_INSTALL.md`.

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
./tools/build_addin.sh
```

This should produce a `.g3a` in the build output folder (exact path depends on fxSDK version/template).

### Install onto calculator

- **Manual**: copy the built `.g3a` onto the calculator’s USB storage.
- **fxSDK send helper** (Linux + UDisks2): `fxsdk send-cg`

### Notes for this project

- The long-term goal is a single `.g3a` exposing the same functionality as the Python engines in `python/src/`.
- We will keep a **host build** of the core engine for fast correctness testing (golden fixtures generated from the current Python outputs).

### Host build (fast local testing)

This repo installs `cmake` via pip (`--user`) on macOS, so `cmake` may live at `$(python3 -c 'import site; print(site.USER_BASE)')/bin`.

Build the host runner from the repo root:

```bash
./tools/build_host.sh
./addin/host/build/casio_host --alg "2x+3=7"
./addin/host/build/casio_host --int "sin(3x+2)"
./addin/host/build/casio_host --trig "cos(5*pi/3)"
```

