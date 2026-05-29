# KhiCAS/Giac Vendor Base

Edited upstream source/reference lives in `upstream/`.

- `upstream/giac90_1addin/`: Fourier `giac90_1addin.tgz` source tree for English KhiCAS.
- `upstream/reference/khicasen.g3a`: verified Fourier reference binary.
- `upstream/compile.txt`: upstream compile instructions.
- `upstream/INSTALL.txt`: upstream license/install notes.

Active `/compile` builds the edited KhiCAS source by default with a Linux amd64 Docker image and packages the Eigenmath-style icons from `c++/prizm/assets`. Use `CASIO_PRIZM_MODE=khicas-reference ./c++/tools/build_addin_prizm_docker.sh` to package the verified upstream reference binary, or `CASIO_PRIZM_MODE=legacy ./c++/tools/build_addin_prizm_docker.sh` to build the previous small native port.

`c++/tools/build_khicas_host_runner_docker.sh` is host audit tooling only. It compiles the same KhiCAS CAS sources into a Linux CLI runner, without changing `./compile`, the production Makefile, or `.g3a` packaging.

Transfer files are copied to the top-level `calculator_files/` folder:
- `CasioCAS.g3a` - visible add-in executable.
- `CASIOCAS.PAK` - external F6 help and working-template pack generated from `c++/prizm/help/CASIOCAS.HLP` and `c++/prizm/help/CASIOCAS*.TPL`.

License note: KhiCAS/Giac is upstream GPL-family code; preserve upstream notices when modifying or redistributing.
