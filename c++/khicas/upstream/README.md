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
- Project `/compile` now builds this edited source by default with `c++/tools/docker/Dockerfile.khicas-source`.
- Project `/compile` also patches metadata/icon and writes calculator transfer files under top-level `calculator_files/`.
- Use `CASIO_PRIZM_MODE=khicas-reference` for the verified reference binary.
- Use `CASIO_PRIZM_MODE=legacy` for the old native-port build.
