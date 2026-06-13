# NOTES.g3a README

## Purpose

`NOTES.g3a` is a read-only notes browser for text files stored on the calculator. It deliberately does not open images.

## Files

- Calculator app: `/Users/james/Developer/CASIO/calculator_files/NOTES.g3a`

## Supported Files

- Folders.
- `.txt` files.

Unsupported:

- Images.
- Rich text.
- Editing.

## Folder Layout

Copy folders and `.txt` files to calculator storage. The app starts at:

`\\fls0\\`

Example:

```text
\\fls0\\Maths\\Pure\\integration.txt
\\fls0\\Maths\\Stats\\normal.txt
\\fls0\\CS\\FloatingPoint\\examples.txt
```

## Controls

Main browser:

- `UP/DOWN`: move selection.
- `EXE` or `F1`: open selected folder/text file.
- `F2`: search all text files.
- `F6` or `EXIT`: parent folder.
- `MENU` or `AC`: quit.

Text viewer:

- `UP/DOWN`: scroll.
- `F1`: find inside current opened file.
- `F2`, `F6`, `EXIT`, or `AC`: back.

## Search All Text Files

Press `F2` in the main browser.

Search covers:

- folder names in the path,
- file names,
- contents of every `.txt` file in every folder under `\\fls0\\`.

Result labels:

- `name:` means the match was in file name or path.
- `text:` means the match was in file contents.

Example search:

```text
normal
```

Possible result:

```text
name: normal.txt
\\fls0\\Maths\\Stats\\normal.txt
text: notes.txt
\\fls0\\Revision\\paper3\\notes.txt
```

## Search Algorithm

The search uses Knuth-Morris-Pratt string matching:

- query preprocessing: `O(m)`,
- each searched string/file stream: `O(n)`,
- whole search: `O(total path characters + total text bytes + m)`.

Why KMP:

- It has linear worst-case time.
- It does not re-check already matched characters after mismatch.
- It can stream through files chunk by chunk while preserving match state across chunk boundaries.

Practical speed choices:

- The query is normalised to lower-case once.
- File name/path is searched before opening file contents.
- If file name/path matches, the file is not opened.
- File content is read in 8 KB chunks.
- Search exits as soon as a match is found.
- Search stops after the results page is full.

References:

- KMP/prefix-function: https://cp-algorithms.com/string/prefix-function.html
- KMP worst-case: https://www.geeksforgeeks.org/dsa/kmp-algorithm-for-pattern-searching/
- BMH worst-case comparison: https://www.boost.org/doc/libs/latest/libs/algorithm/doc/html/the_boost_algorithm_library/Searching/BoyerMooreHorspool.html

## Limits

- Search depth is capped to avoid runaway traversal.
- Result list is capped to fit the calculator page buffer.
- Very long text files can be opened only up to the viewer buffer, but all-file search scans the full file contents in chunks.
