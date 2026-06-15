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

Create a top-level folder called `NOTES` in calculator storage. Copy all note folders and `.txt` files inside it.

The app starts at:

`\\fls0\\NOTES\\`

It cannot browse or search outside this folder.

Example:

```text
\\fls0\\NOTES\\Maths\\Pure\\integration.txt
\\fls0\\NOTES\\Maths\\Stats\\normal.txt
\\fls0\\NOTES\\CS\\FloatingPoint\\examples.txt
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
- `LEFT/RIGHT`: horizontal scroll for wide lines and tables.
- `F1`: next search match if searching, otherwise next page.
- `F2`: previous search match if searching, otherwise previous page.
- `F5`: enter a new search inside the current file.
- `F6`, `EXIT`, or `AC`: back.

## Search All Text Files

Press `F2` in the main browser.

Search covers:

- folder names in the path,
- file names,
- contents of every `.txt` file in every folder under `\\fls0\\NOTES\\`.

The last search query is kept. Pressing `F2` again pre-fills the previous query, so it can be edited or rerun without retyping.

Result labels:

- `NAME` means the match was in the file name or path.
- `TEXT<number>` means the match was in the file contents and shows the number of counted hits.

Example search:

```text
normal
```

Possible result:

```text
NAME normal.txt
TEXT3 notes.txt
```

Search results are ranked:

- file-name matches first,
- path/folder matches next,
- content matches add relevance even when the file name/path also matches,
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

Text is rendered with a deliberately small markdown-like formatter using the calculator mini font. This keeps the viewer predictable and avoids spacing issues on the fx-CG50 screen.

The viewer:

- shows the current folder/file name in the top title bar,
- recognises `# heading` and `## heading` lines,
- recognises `- item` and `* item` bullets,
- wraps ordinary lines at word boundaries,
- preserves indentation,
- turns tabs into spaces,
- keeps table-like/code lines unwrapped,
- supports horizontal scrolling for wide lines and tables with `LEFT/RIGHT`,
- marks search hits with a simple yellow marker in the left margin,
- does not show line numbers in the file viewer.

`F1/F2` move between search matches when a search is active. Otherwise, they page down/up.

Table-like lines are detected when a line contains explicit column separators such as tabs, multiple `|` characters, or ASCII table borders. Ordinary prose with commas or repeated spaces is not treated as a table.

Formatter design:

- one file read into a fixed-size buffer,
- one pass over the file to split display lines,
- word wrapping by fixed character budget,
- no per-word background redraws,
- wide-line horizontal scroll is capped at the longest source line,
- all search/file searches use linear KMP matching.

The folder browser ignores calculator/macOS system files such as `.DS_Store`, `.Trashes`, `.fseventsd`, and `._*.txt` AppleDouble files. Folders are shown before files, and entries are sorted alphabetically.

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
- File name/path and file contents both contribute to ranking.
- File content is read in 16 KB chunks.
- Content scanning records the first matching line and counts repeated matches up to a small cap for ranking.
- The full notes tree is still scanned so the best 64 ranked results are kept.

References:

- KMP/prefix-function: https://cp-algorithms.com/string/prefix-function.html
- KMP worst-case: https://www.geeksforgeeks.org/dsa/kmp-algorithm-for-pattern-searching/
- BMH worst-case comparison: https://www.boost.org/doc/libs/latest/libs/algorithm/doc/html/the_boost_algorithm_library/Searching/BoyerMooreHorspool.html
- fx-CG add-in text rendering inspiration: https://github.com/Konsl/utilities-fxcg50

## Limits

- Search depth is capped to avoid runaway traversal.
- Result list is capped at 64 ranked results.
- Opened text is cached up to 16 KB for viewing.
- Very long text files can be opened only up to the viewer buffer, but all-file search scans the full file contents in chunks.
- The viewer stores up to 320 displayed lines after wrapping.
