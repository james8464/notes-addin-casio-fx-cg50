## fxSDK + gint install (fx-CG50)

This project builds a native `.g3a` using **fxSDK + gint**.

### Recommended setup: Linux (or WSL2)

#### 1) Install build dependencies

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake git python3 python3-pip libpng-dev libusb-1.0-0-dev
```

#### 2) Install GiteaPC

fxSDK recommends installing via **GiteaPC**.

- Follow the official instructions in the fxSDK README: `https://git.planet-casio.com/Lephenixnoir/fxsdk/src/branch/master/README.md`

After installing `giteapc`, verify it works:

```bash
giteapc --version
```

#### 3) Install fxSDK stack

```bash
giteapc install Lephenixnoir/fxsdk
```

If GiteaPC prompts for dependencies (toolchain, `gint`, `fxlibc`, `openlibm`, etc), install them as instructed.

Verify:

```bash
fxsdk --version
fxsdk path
```

#### 4) Hello-world verification

```bash
fxsdk new -t addin-cg50 /tmp/fxsdk-smoke
cd /tmp/fxsdk-smoke
fxsdk build-cg
ls -la build/*.g3a
```

### macOS notes

fxSDK is primarily Linux-focused. On macOS, you can usually still:
- develop/test core logic via the **host build** (`./tools/build_host.sh`)
- build `.g3a` in a **Linux VM** (recommended) and then copy the `.g3a` back to macOS for transfer.

### Transfer `.g3a` to calculator

- **Manual**: plug calculator via USB, mount its storage, copy `.g3a` (typically to the root or Add-In folder depending on your setup).
- **fxSDK send helper**: on Linux, `fxsdk send-cg` (requires `fxlink` + udisks2 support).

### Build this repo’s add-in

From repo root (in the environment where `fxsdk` is installed):

```bash
./tools/build_addin.sh
```

