## Golden fixtures (Python oracle)

This folder contains tools to snapshot the current Python engines’ behavior into
machine-readable fixtures. The C++ `.g3a` port will be validated by diffing its
outputs against these fixtures.

### Generate from `Tests.rtf`

On macOS (uses `textutil`), from repo root:

```bash
python3 tools/golden/generate_fixtures.py --out tools/golden/rtf_cases.jsonl
```

Options:
- `--limit N`: only map N cases (useful while iterating)
- `--timeout-s N`: per-case timeout

Non-macOS:
- Pre-generate `python/Tests.txt` next to `python/Tests.rtf` (same content as macOS `textutil` would produce), then run the generator.

