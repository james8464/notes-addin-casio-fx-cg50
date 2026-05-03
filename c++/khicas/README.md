# KhiCAS/Giac Vendor Base

Exact upstream source/reference now lives in `upstream/`.

- `upstream/giac90_1addin/`: Fourier `giac90_1addin.tgz` source tree for English KhiCAS.
- `upstream/reference/khicasen.g3a`: verified Fourier binary matching the user-supplied `inspiration:base/khicasen.g3a`.
- `upstream/compile.txt`: upstream compile instructions.
- `upstream/INSTALL.txt`: upstream license/install notes.

Active `/compile` emits the verified upstream KhiCAS reference by default, then patches in the Eigenmath-style icons from `c++/prizm/assets`. Use `CASIO_PRIZM_MODE=legacy ./c++/tools/build_addin_prizm_docker.sh` to build the previous small native port.

License note: KhiCAS/Giac is upstream GPL-family code; preserve upstream notices when modifying or redistributing.
