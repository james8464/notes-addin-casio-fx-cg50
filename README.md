# NOTES — Casio fx-CG50 Notes Browser

A read-only `.txt` file browser for the fx-CG50 (Prizm). Browse folders, read notes with markdown-like formatting, and search all files at once.

## Repo layout

```
source/       Build environment, source code, tools, tests
calculator/   NOTES.g3a app file, ready to copy to your calculator
README.md     You are here
```

## Install

1. Copy `calculator/NOTES.g3a` to the calculator's `@MainMem\` folder (via FA-124 or USB mass storage).
2. Create a folder called `NOTES` in `\fls0\` on the calculator and put your `.txt` files in it.
3. Launch **NOTES** from the Main Menu.

There is an empty `calculator/NOTES\` folder ready for you to fill with your own `.txt` files. Only `.txt` files are supported. Organise them into subfolders however you like:

```
\fls0\NOTES\Maths\Pure\integration.txt
\fls0\NOTES\French\vocab.txt
\fls0\NOTES\CS\FloatingPoint\examples.txt
```

## Controls

### File browser

| Button | Action |
|--------|--------|
| `UP` / `DOWN` | Move selection |
| `EXE` or `F1` | Open folder / file |
| `F2` | Search all `.txt` files |
| `F6` or `EXIT` | Parent folder |
| `MENU` or `AC` | Quit |

### Text viewer

| Button | Action |
|--------|--------|
| `UP` / `DOWN` | Scroll line by line |
| `LEFT` / `RIGHT` | Horizontal scroll (unwrap wide lines) |
| `F3` | Page up |
| `F4` | Page down |
| `F1` | Next search match / next page |
| `F2` | Previous search match / previous page |
| `F5` | Find text in current file |
| `F6` or `EXIT` | Back to file browser |

## Features

- **Markdown formatting**: headings (`#`–`####`), bullets (`-` / `*`), numbered lists, blockquotes (`>`), horizontal rules (`---`)
- **Word wrapping**: ordinary prose wraps at word boundaries
- **Preserve mode**: code-like lines (long unbroken strings) display unwrapped; press `RIGHT` to shift view horizontally, `LEFT` to shift back
- **In-file search** (`F5`): case-insensitive, highlights matches, `F1`/`F2` to jump between hits
- **Search all files** (`F2` in browser): scans filenames, folder names, and file contents; results ranked by relevance (name match > content match > more hits = higher)
- **Search history**: last query is remembered; `UP`/`DOWN` in the search box recalls past queries
- **Long filenames**: auto-scroll marquee in the title bar
- **File limits**: 16 KB per file, 768 displayed lines, 64 search results
- **System file filtering**: ignores `.DS_Store`, `._*`, `.Trashes`, etc.
- **Alphabetical sort**: folders listed before files

## Build from source

```bash
./source/compile notes
```

Requires Docker. See `source/tools/docker/Dockerfile.notes-source` for the cross-compiler image.
