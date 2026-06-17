#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${CASIO_KHICAS_IMAGE:-casio-khicas-source:latest}"
SRC_DIR="${ROOT_DIR}/khicas/upstream/giac90_1addin"
OUT_DIR="${ROOT_DIR}/build"
TRANSFER_DIR="${ROOT_DIR}/calculator_files"
MAKE_JOBS="${CASIO_MAKE_JOBS:-1}"
IMAGE_VERSION="runmat-v2"
DOCKER_BUILD_SCRIPT="${OUT_DIR}/docker_build_g3a.sh"

usage() {
  cat <<'EOF'
Usage: ./compile [all|cas|casp3|notes|runmat|khicas]...

Targets:
  all     build CAS, CASP3, NOTES, RUNMAT, CAS.PAK
  cas     build CAS.g3a and CAS.PAK
  casp3   build CASP3.g3a
  notes   build NOTES.g3a only
  runmat  build RUNMAT.g3a only
  khicas  build original Khicasen target
EOF
}

want() {
  local needle="$1"
  for t in "${TARGETS[@]}"; do
    [ "$t" = "$needle" ] && return 0
  done
  return 1
}

TARGETS=("$@")
if [ "${#TARGETS[@]}" -eq 0 ]; then
  TARGETS=(all)
fi
for t in "${TARGETS[@]}"; do
  case "$t" in
    all|cas|casp3|notes|runmat|khicas) ;;
    -h|--help|help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done
if want all; then
  expanded=()
  for t in "${TARGETS[@]}"; do
    if [ "$t" = all ]; then
      expanded+=(cas casp3 notes runmat)
    else
      expanded+=("$t")
    fi
  done
  TARGETS=("${expanded[@]}")
fi

ensure_image() {
  if ! docker image inspect "${IMAGE_TAG}" >/dev/null 2>&1; then
    docker build \
      --platform linux/amd64 \
      -f "${ROOT_DIR}/tools/docker/Dockerfile.khicas-source" \
      --label "casio.khicas.image-version=${IMAGE_VERSION}" \
      -t "${IMAGE_TAG}" \
      "${ROOT_DIR}"
  fi
}

clean_source_outputs() {
  find "${SRC_DIR}" -maxdepth 1 \( -type f -o -type l \) \
    \( -name '*.o' -o -name '*.a' -o -name '*.elf' -o -name '*.bin' \
       -o -name '*.g3a' -o -name '*.map' -o -name '*.ac2' -o -name '*.882' \
       -o -name 'dump*' -o -name 'mkhelp' \) \
    -delete
}

prepare_icons() {
  python3 - "${SRC_DIR}" <<'PY'
from pathlib import Path
import sys
from PIL import Image

root = Path(sys.argv[1])
Image.new("RGB", (92, 64), (245, 245, 245)).save(root / "khicasio.png", optimize=True, compress_level=9)
Image.new("RGB", (92, 64), (32, 86, 170)).save(root / "khicasio1.png", optimize=True, compress_level=9)
PY
}

stage_sources() {
  python3 "${ROOT_DIR}/tools/generate_runmat_icons.py"
  cp "${ROOT_DIR}/tools/runmat_mock.cc" "${SRC_DIR}/runmat_mock.cc"
  cp "${ROOT_DIR}/tools/casio_suite_ui.hpp" "${SRC_DIR}/casio_suite_ui.hpp"
  cp "${ROOT_DIR}/tools/p3_engine.hpp" "${SRC_DIR}/p3_engine.hpp"
  cp "${ROOT_DIR}/tools/p3_engine.cpp" "${SRC_DIR}/p3_engine.cpp"
  cp "${ROOT_DIR}/tools/khicas_suite_bridge.hpp" "${SRC_DIR}/khicas_suite_bridge.hpp"
  cp "${ROOT_DIR}/tools/khicas_suite_bridge.cpp" "${SRC_DIR}/khicas_suite_bridge.cpp"
  cp "${ROOT_DIR}/tools/notes_app.cc" "${SRC_DIR}/notes_app.cc"
}

cleanup_staged_sources() {
  rm -f "${SRC_DIR}/khicasio.png" "${SRC_DIR}/khicasio1.png" \
    "${SRC_DIR}/casio_suite_ui.hpp" \
    "${SRC_DIR}/p3_engine.hpp" "${SRC_DIR}/p3_engine.cpp" \
    "${SRC_DIR}/khicas_suite_bridge.hpp" "${SRC_DIR}/khicas_suite_bridge.cpp" \
    "${SRC_DIR}/notes_app.cc"
  clean_source_outputs
}

mkdir -p "${OUT_DIR}" "${TRANSFER_DIR}"
rm -rf "${OUT_DIR:?}"/*
find "${TRANSFER_DIR}" -mindepth 1 \
  \( -name 'NOTES' -o -name '.gitkeep' \) -prune \
  -o -exec rm -rf {} +
touch "${TRANSFER_DIR}/.gitkeep"

ensure_image
clean_source_outputs
prepare_icons
stage_sources

DO_CAS=0; DO_CASP3=0; DO_NOTES=0; DO_RUNMAT=0; DO_KHICAS=0
want cas && DO_CAS=1
want casp3 && DO_CASP3=1
want notes && DO_NOTES=1
want runmat && DO_RUNMAT=1
want khicas && DO_KHICAS=1

cat > "${DOCKER_BUILD_SCRIPT}" <<'SH'
#!/usr/bin/env bash
set -euo pipefail

: "${CASIO_MAKE_JOBS:=1}"
: "${DO_CAS:=0}"
: "${DO_CASP3:=0}"
: "${DO_NOTES:=0}"
: "${DO_RUNMAT:=0}"
: "${DO_KHICAS:=0}"

mkdir -p /shared/tmp ~/.wine/drive_c
rm -rf /tmp/giac90_1addin
cp -a /work/khicas/upstream/giac90_1addin /tmp/giac90_1addin
cd /tmp/giac90_1addin

make clean
[ "${DO_CAS}" = 0 ] || make -j"${CASIO_MAKE_JOBS}" CAS.g3a
[ "${DO_RUNMAT}" = 0 ] || make -j"${CASIO_MAKE_JOBS}" RUNMAT.g3a
[ "${DO_NOTES}" = 0 ] || make -j"${CASIO_MAKE_JOBS}" NOTES.g3a
[ "${DO_KHICAS}" = 0 ] || make -j"${CASIO_MAKE_JOBS}" khicasen.g3a

copy_base() {
  local file="$1"
  [ ! -f "${file}" ] || cp "${file}" /work/khicas/upstream/giac90_1addin/
}
copy_base CAS.g3a
copy_base RUNMAT.g3a
copy_base NOTES.g3a
copy_base khicasen.g3a
for base in khicasen runmat_mock NOTES; do
  for ext in bin elf map; do
    [ ! -f "${base}.${ext}" ] || cp "${base}.${ext}" /work/khicas/upstream/giac90_1addin/
  done
done

if [ "${DO_CASP3}" = 1 ]; then
  tree="/tmp/giac90_1addin_CASP3"
  rm -rf "${tree}"
  cp -a /work/khicas/upstream/giac90_1addin "${tree}"
  cd "${tree}"
  make clean
  cp /work/tools/khicas_suite_bridge.hpp .
  cp /work/tools/khicas_suite_bridge.cpp .
  cp /work/tools/p3_engine.hpp /work/tools/p3_engine.cpp .
  python3 /work/tools/khicas_suite_catalog.py p3 catalogen.cpp
  python3 /work/tools/patch_khicas_suite_main.py main.cc
  python3 - <<'PY'
from pathlib import Path
make = Path("Makefile")
text = make.read_text()
text = text.replace(
    "GUI_OBJS = fileGUI.o menuGUI.o textGUI.o fileProvider.o graphicsProvider.o stringsProvider.o kdisplay.o console.o cascas_working.o main.o",
    "GUI_OBJS = fileGUI.o menuGUI.o textGUI.o fileProvider.o graphicsProvider.o stringsProvider.o kdisplay.o console.o cascas_working.o main.o khicas_suite_bridge.o p3_engine.o",
)
text += r'''

khicas_suite_bridge.o: khicas_suite_bridge.cpp khicas_suite_bridge.hpp p3_engine.hpp
	$(CXX) $(CXXFLAGS) -DSUITE_APP_P3 -c $<

CASP3.g3a: khicasen.bin casp3_icon.png casp3_icon_selected.png
	mkg3a -n basic:CASP3 -n internal:CASP3 -V 1.0.0 -i uns:casp3_icon.png -i sel:casp3_icon_selected.png khicasen.bin $@
	/bin/cp CASP3.g3a ~/.wine/drive_c
'''
make.write_text(text)
PY
  make -j"${CASIO_MAKE_JOBS}" CASP3.g3a
  cp CASP3.g3a /work/khicas/upstream/giac90_1addin/
  cp khicasen.bin /work/khicas/upstream/giac90_1addin/CASP3.bin
  cp khicasen.elf /work/khicas/upstream/giac90_1addin/CASP3.elf
  cp khicasen.map /work/khicas/upstream/giac90_1addin/CASP3.map
fi
SH

docker run --rm \
  --platform linux/amd64 \
  -e CASIO_MAKE_JOBS="${MAKE_JOBS}" \
  -e DO_CAS="${DO_CAS}" \
  -e DO_CASP3="${DO_CASP3}" \
  -e DO_NOTES="${DO_NOTES}" \
  -e DO_RUNMAT="${DO_RUNMAT}" \
  -e DO_KHICAS="${DO_KHICAS}" \
  -v "${ROOT_DIR}:/work" \
  -w /work/khicas/upstream/giac90_1addin \
  "${IMAGE_TAG}" \
  bash /work/build/docker_build_g3a.sh

copy_and_check() {
  local file="$1" name="$2" internal="$3"
  [ -f "${SRC_DIR}/${file}" ] || return 0
  cp "${SRC_DIR}/${file}" "${OUT_DIR}/${file}"
  python3 "${ROOT_DIR}/tools/normalize_g3a_metadata.py" "${OUT_DIR}/${file}"
  python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/${file}" --name "${name}" --internal "${internal}" --filename "${file}"
  python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${file}"
  cp "${OUT_DIR}/${file}" "${TRANSFER_DIR}/${file}"
}

copy_and_check CAS.g3a CAS @CAS
copy_and_check RUNMAT.g3a RunMat @RUNMAT
copy_and_check CASP3.g3a CASP3 @CASP3
copy_and_check NOTES.g3a Notes @NOTES
copy_and_check khicasen.g3a Khicasen @KHICASEN

if [ "${DO_CAS}" = 1 ]; then
  python3 "${ROOT_DIR}/tools/build_help_pack.py" "${ROOT_DIR}/help/functions" "${OUT_DIR}/CAS.PAK"
  cp "${OUT_DIR}/CAS.PAK" "${TRANSFER_DIR}/CAS.PAK"
fi

cleanup_staged_sources
find "${TRANSFER_DIR}" -maxdepth 1 -type f -print | sort | xargs -r ls -lh
