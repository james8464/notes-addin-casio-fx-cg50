# NOTES.g3a — Casio fx-CG50 Notes Browser

Read-only notes browser for `.txt` files on the fx-CG50 (Prizm). Browse folders, read notes, and search all files instantly.

## Quick start

### 1. Install the app

Download `NOTES.g3a` from the [latest release](https://github.com/james8464/casio_program/releases/latest).

Copy it to your calculator using **FA-124** or Casio **Link Module**:

- **FA-124:** open the `.g3a`, select your calculator under "Connected devices", click the arrow to send.
- **USB Mass Storage:** drag `NOTES.g3a` to the calculator's `@MainMem\` folder.

### 2. Place your notes

Create a top-level folder called `NOTES` in calculator storage. Copy your `.txt` files inside it, organised however you like:

```
\\fls0\\NOTES\\
\\fls0\\NOTES\\Maths\\Pure\\integration.txt
\\fls0\\NOTES\\Maths\\Stats\\normal.txt
\\fls0\\NOTES\\CS\\FloatingPoint\\examples.txt
```

Only `.txt` files are supported. No images, no rich text, no editing.

### 3. Launch

Find `NOTES` on the calculator's Main Menu and press `EXE`.

## Controls

| Button | Action |
|---|---|
| `UP` / `DOWN` | Move selection |
| `EXE` or `F1` | Open folder / file |
| `F2` | Search all text files |
| `F6` or `EXIT` | Back / parent folder |
| `MENU` or `AC` | Quit |

In the **text viewer**:

| Button | Action |
|---|---|
| `UP` / `DOWN` | Scroll |
| `F3` / `F4` | Page up / down |
| `LEFT` / `RIGHT` | Horizontal scroll (wide lines and tables) |
| `F1` / `F2` | Next / previous search match |
| `F5` | New search inside current file |
| `F6` or `EXIT` | Back |

Search results are ranked — file-name matches first, content matches next, repeated matches rank higher.

## What's in the repo

```
apps/notes/notes_app.cc         Source code
khicas/upstream/giac90_1addin/  Build infrastructure (Makefile, linker script, icons)
tools/build/                    Build scripts
tools/docker/                   SH3 cross-compiler Docker image
calculator_files/NOTES/         Example notes to copy to your calculator
```

## Build from source

```bash
./compile notes
```

Requires Docker. See `tools/docker/Dockerfile.notes-source` for the cross-compiler image.
