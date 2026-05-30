# CasioCAS A-Level KhiCAS Rebuild

Fresh source import of KhiCAS for Casio fx-CG50, narrowed to Edexcel A-level Pure + Mechanics.

```bash
./compile
python3 tools/check_g3a_size.py calculator_files/CasioCAS.g3a
python3 tools/check_catalog_scope.py
python3 tools/check_removed_features.py
python3 tests/run_exact_queue.py
```

Output:

- `calculator_files/CasioCAS.g3a`
- `calculator_files/CASIOCAS.PAK`
