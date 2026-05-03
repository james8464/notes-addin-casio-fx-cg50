# Upstream KhiCAS Import

Source: `https://www-fourier.univ-grenoble-alpes.fr/~parisse/casio/`

Imported:
- `giac90_1addin.tgz` -> `giac90_1addin/`
- `khicasen.g3a` -> `reference/khicasen.g3a`
- `khicas50.g3a` -> `reference/khicas50.g3a`
- `emucas50.g3a` -> `reference/emucas50.g3a`

Verified reference:

```text
khicasen.g3a sha256=e926c9a8e3111c51786e444f2b6f59104362c960925fd0704668f73a8987bfdf
```

Build note:
- Upstream `compile.txt` requires the Fourier Casio toolchain/libs.
- Project `/compile` uses the verified reference binary by default for exact KhiCAS UI.
- Use `CASIO_PRIZM_MODE=legacy` for the old native-port build.
