# KhiCAS/Giac Vendor Base

Edited upstream source/reference lives in `upstream/`.

- `upstream/giac90_1addin/`: Fourier `giac90_1addin.tgz` source tree for English KhiCAS.
- `upstream/reference/khicasen.g3a`: verified Fourier binary matching the user-supplied `inspiration:base/khicasen.g3a`.
- `upstream/compile.txt`: upstream compile instructions.
- `upstream/INSTALL.txt`: upstream license/install notes.

Active `/compile` builds the edited KhiCAS source by default with a Linux amd64 Docker image and packages the Eigenmath-style icons from `c++/prizm/assets`. Use `CASIO_PRIZM_MODE=khicas-reference ./c++/tools/build_addin_prizm_docker.sh` to package the verified upstream reference binary, or `CASIO_PRIZM_MODE=legacy ./c++/tools/build_addin_prizm_docker.sh` to build the previous small native port.

Build output is copied to repo-root `CasioCAS.g3a`.

License note: KhiCAS/Giac is upstream GPL-family code; preserve upstream notices when modifying or redistributing.
