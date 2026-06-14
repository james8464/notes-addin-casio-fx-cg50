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
- `PAGEUP` or `F3`: page up.
- `PAGEDOWN` or `F4`: page down.
- `LEFT/RIGHT`: horizontal scroll for wide table-like lines.
- `F1`: next match for the active search query.
- `F2`: previous match for the active search query.
- `F5`: enter a new search inside the current file.
- `F6`, `EXIT`, or `AC`: back.

## Search All Text Files

Press `F2` in the main browser.

Search covers:

- folder names in the path,
- file names,
- contents of every `.txt` file in every folder under `\\fls0\\`.

The last search query is kept. Pressing `F2` again pre-fills the previous query, so it can be edited or rerun without retyping.

Result labels:

- `N` means the match was in the file name or path.
- `T` means the match was in the file contents.
- `L<number>` shows the first matching content line when available.

Example search:

```text
normal
```

Possible result:

```text
N normal.txt
T notes.txt L14
```

Search results are ranked:

- file-name matches first,
- path/folder matches next,
- content matches after that,
- repeated content matches rank higher,
- earlier content matches rank higher.

Search result controls:

- `UP/DOWN`: move through ranked results.
- `EXE` or `F1`: open the selected result.
- `F2`: start a new search with the previous query pre-filled.
- `PAGEUP` or `F3`: jump up through results.
- `PAGEDOWN` or `F4`: jump down through results.
- `F6`, `EXIT`, or `AC`: return to the folder browser.

Opening a content result opens the file and jumps to the first displayed matching line. `F1` and `F2` then move between matches without asking for the query again.

## Text Display

Text is rendered in the same small calculator-style font used by the suite UI, so more lines fit on screen.

The viewer:

- wraps ordinary text at word boundaries,
- preserves indentation,
- turns tabs into spaces,
- keeps table-like lines unwrapped,
- supports horizontal scrolling for wide tables with `LEFT/RIGHT`,
- shows the current displayed line count at the top.

Table-like lines are detected when a line contains repeated column separators such as tabs, multiple `|` characters, several commas, or repeated spaces.

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
- Content scanning records the first matching line and counts repeated matches up to a small cap for ranking.
- Search stops when the result list is full.
- `EXIT` or `AC` can cancel a scan.

References:

- KMP/prefix-function: https://cp-algorithms.com/string/prefix-function.html
- KMP worst-case: https://www.geeksforgeeks.org/dsa/kmp-algorithm-for-pattern-searching/
- BMH worst-case comparison: https://www.boost.org/doc/libs/latest/libs/algorithm/doc/html/the_boost_algorithm_library/Searching/BoyerMooreHorspool.html

## Limits

- Search depth is capped to avoid runaway traversal.
- Result list is capped at 64 ranked results.
- Opened text is cached up to 8 KB for viewing.
- Very long text files can be opened only up to the viewer buffer, but all-file search scans the full file contents in chunks.
- The viewer stores up to 160 displayed lines after wrapping.
