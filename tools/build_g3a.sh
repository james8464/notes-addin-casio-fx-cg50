#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${CASIO_KHICAS_IMAGE:-casio-khicas-source:latest}"
SRC_DIR="${ROOT_DIR}/khicas/upstream/giac90_1addin"
OUT_DIR="${ROOT_DIR}/build"
TRANSFER_DIR="${ROOT_DIR}/calculator_files"
TARGET="${CASIO_KHICAS_TARGET:-CAS.g3a}"
RUNMAT_TARGET="${CASIO_RUNMAT_TARGET:-RUNMAT.g3a}"
P3_TARGET="${CASIO_P3_TARGET:-CASP3.g3a}"
CS_TARGET="${CASIO_CS_TARGET:-CSCALC.g3a}"
NOTES_TARGET="${CASIO_NOTES_TARGET:-NOTES.g3a}"
PACK_TARGET="${CASIO_HELP_PACK_TARGET:-${TARGET%.*}.PAK}"
MAKE_JOBS="${CASIO_MAKE_JOBS:-1}"
IMAGE_VERSION="runmat-v2"
DOCKER_BUILD_SCRIPT="${OUT_DIR}/docker_build_g3a.sh"

ensure_image() {
  local current_version=""
  current_version="$(docker image inspect -f '{{ index .Config.Labels "casio.khicas.image-version" }}' "${IMAGE_TAG}" 2>/dev/null || true)"
  if [ "${current_version}" != "${IMAGE_VERSION}" ]; then
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
import re
import sys

root = Path(sys.argv[1])
from PIL import Image

def draw_icon(selected: bool) -> Image.Image:
    return Image.new("RGB", (92, 64), (32, 86, 170) if selected else (245, 245, 245))

draw_icon(False).save(root / "khicasio.png", optimize=True, compress_level=9)
draw_icon(True).save(root / "khicasio1.png", optimize=True, compress_level=9)
PY
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
python3 "${ROOT_DIR}/tools/generate_runmat_icons.py"
cp "${ROOT_DIR}/tools/runmat_mock.cc" "${SRC_DIR}/runmat_mock.cc"
cp "${ROOT_DIR}/tools/casio_suite_ui.hpp" "${SRC_DIR}/casio_suite_ui.hpp"
cp "${ROOT_DIR}/tools/p3_engine.hpp" "${SRC_DIR}/p3_engine.hpp"
cp "${ROOT_DIR}/tools/p3_engine.cpp" "${SRC_DIR}/p3_engine.cpp"
cp "${ROOT_DIR}/tools/cscalc_engine.hpp" "${SRC_DIR}/cscalc_engine.hpp"
cp "${ROOT_DIR}/tools/cscalc_engine.cpp" "${SRC_DIR}/cscalc_engine.cpp"
cp "${ROOT_DIR}/tools/khicas_suite_bridge.hpp" "${SRC_DIR}/khicas_suite_bridge.hpp"
cp "${ROOT_DIR}/tools/khicas_suite_bridge.cpp" "${SRC_DIR}/khicas_suite_bridge.cpp"
cp "${ROOT_DIR}/tools/notes_app.cc" "${SRC_DIR}/notes_app.cc"

cat > "${DOCKER_BUILD_SCRIPT}" <<'SH'
#!/usr/bin/env bash
set -euo pipefail

: "${CASIO_KHICAS_TARGET:=CAS.g3a}"
: "${CASIO_RUNMAT_TARGET:=RUNMAT.g3a}"
: "${CASIO_P3_TARGET:=CASP3.g3a}"
: "${CASIO_CS_TARGET:=CSCALC.g3a}"
: "${CASIO_NOTES_TARGET:=NOTES.g3a}"
: "${CASIO_MAKE_JOBS:=1}"

mkdir -p /shared/tmp ~/.wine/drive_c
rm -rf /tmp/giac90_1addin
cp -a /work/khicas/upstream/giac90_1addin /tmp/giac90_1addin
cd /tmp/giac90_1addin

make clean
rm -f "${CASIO_KHICAS_TARGET}" "${CASIO_RUNMAT_TARGET}" "${CASIO_NOTES_TARGET}"
make -j"${CASIO_MAKE_JOBS}" "${CASIO_KHICAS_TARGET}" "${CASIO_RUNMAT_TARGET}" "${CASIO_NOTES_TARGET}"

build_suite_app() {
  local app="$1"
  local target="$2"
  local macro="$3"
  local engine_obj="$4"
  local catalog_kind="$5"
  local icon="$6"
  local icon_sel="$7"
  local tree="/tmp/giac90_1addin_${app}"
  rm -rf "${tree}"
  cp -a /work/khicas/upstream/giac90_1addin "${tree}"
  cd "${tree}"
  make clean
  cp /work/tools/khicas_suite_bridge.hpp .
  cp /work/tools/khicas_suite_bridge.cpp .
  cp /work/tools/p3_engine.hpp /work/tools/p3_engine.cpp .
  cp /work/tools/cscalc_engine.hpp /work/tools/cscalc_engine.cpp .
  python3 /work/tools/khicas_suite_catalog.py "${catalog_kind}" catalogen.cpp
  python3 /work/tools/patch_khicas_suite_main.py main.cc
  python3 - "${target}" "${macro}" "${engine_obj}" "${icon}" "${icon_sel}" <<'PY'
from pathlib import Path
import re
import sys

target, macro, engine_obj, icon, icon_sel = sys.argv[1:]
basic_name = target[:-4]
if basic_name == "CSCALC":
    basic_name = "CSCalc"
make = Path("Makefile")
text = make.read_text()
for stale in (
    r"\ncasio_p3_app\.o:.*?(?=\n\S)",
    r"\ncasio_cscalc_app\.o:.*?(?=\n\S)",
    r"\nCASP3\.elf:.*?(?=\n\S)",
    r"\nCASP3\.bin:.*?(?=\n\S)",
    r"\nCASP3\.g3a:.*?(?=\n\S)",
    r"\nCSCALC\.elf:.*?(?=\n\S)",
    r"\nCSCALC\.bin:.*?(?=\n\S)",
    r"\nCSCALC\.g3a:.*?(?=\n\S)",
):
    text = re.sub(stale, "\n", text, flags=re.S)
text = text.replace(
    "GUI_OBJS = fileGUI.o menuGUI.o textGUI.o fileProvider.o graphicsProvider.o stringsProvider.o kdisplay.o console.o cascas_working.o main.o",
    f"GUI_OBJS = fileGUI.o menuGUI.o textGUI.o fileProvider.o graphicsProvider.o stringsProvider.o kdisplay.o console.o cascas_working.o main.o khicas_suite_bridge.o {engine_obj}",
)
engine_rule = ""
if f"\n{engine_obj}:" not in text:
    engine_rule = f"""

{engine_obj}: {engine_obj[:-2]}.cpp {engine_obj[:-2]}.hpp
\t$(CXX) $(RUNMAT_CXXFLAGS) -c $< -o $@
"""
text += f"""

khicas_suite_bridge.o: khicas_suite_bridge.cpp khicas_suite_bridge.hpp p3_engine.hpp cscalc_engine.hpp
\t$(CXX) $(CXXFLAGS) -D{macro} -c $<
{engine_rule}

{target}: khicasen.bin {icon} {icon_sel}
\tmkg3a -n basic:{basic_name} -n internal:{target[:-4].upper()} -V 1.0.0 -i uns:{icon} -i sel:{icon_sel} khicasen.bin $@
\t/bin/cp {target} ~/.wine/drive_c
"""
make.write_text(text)
PY
  rm -f "${target}"
  make -j"${CASIO_MAKE_JOBS}" "${target}"
  cp "${target}" "/work/khicas/upstream/giac90_1addin/${target}"
  cp khicasen.bin "/work/khicas/upstream/giac90_1addin/${app}.bin"
  cp khicasen.elf "/work/khicas/upstream/giac90_1addin/${app}.elf"
  cp khicasen.map "/work/khicas/upstream/giac90_1addin/${app}.map"
  cd /tmp/giac90_1addin
}

build_suite_app CASP3 "${CASIO_P3_TARGET}" SUITE_APP_P3 p3_engine.o p3 casp3_icon.png casp3_icon_selected.png
build_suite_app CSCALC "${CASIO_CS_TARGET}" SUITE_APP_CS cscalc_engine.o cs cscalc_icon.png cscalc_icon_selected.png

cp "${CASIO_KHICAS_TARGET}" /work/khicas/upstream/giac90_1addin/
cp "${CASIO_RUNMAT_TARGET}" /work/khicas/upstream/giac90_1addin/
cp "${CASIO_NOTES_TARGET}" /work/khicas/upstream/giac90_1addin/
cp khicasen.bin khicasen.elf khicasen.map /work/khicas/upstream/giac90_1addin/
cp runmat_mock.bin runmat_mock.elf runmat_mock.map /work/khicas/upstream/giac90_1addin/
for app in NOTES; do
  cp "${app}.bin" "${app}.elf" "${app}.map" /work/khicas/upstream/giac90_1addin/
done
SH

docker run --rm \
  --platform linux/amd64 \
  -e CASIO_MAKE_JOBS="${MAKE_JOBS}" \
  -e CASIO_KHICAS_TARGET="${TARGET}" \
  -e CASIO_RUNMAT_TARGET="${RUNMAT_TARGET}" \
  -e CASIO_P3_TARGET="${P3_TARGET}" \
  -e CASIO_CS_TARGET="${CS_TARGET}" \
  -e CASIO_NOTES_TARGET="${NOTES_TARGET}" \
  -v "${ROOT_DIR}:/work" \
  -w /work/khicas/upstream/giac90_1addin \
  "${IMAGE_TAG}" \
  bash /work/build/docker_build_g3a.sh

cp "${SRC_DIR}/${TARGET}" "${OUT_DIR}/${TARGET}"
cp "${SRC_DIR}/${RUNMAT_TARGET}" "${OUT_DIR}/${RUNMAT_TARGET}"
cp "${SRC_DIR}/${P3_TARGET}" "${OUT_DIR}/${P3_TARGET}"
cp "${SRC_DIR}/${CS_TARGET}" "${OUT_DIR}/${CS_TARGET}"
cp "${SRC_DIR}/${NOTES_TARGET}" "${OUT_DIR}/${NOTES_TARGET}"
python3 "${ROOT_DIR}/tools/normalize_g3a_metadata.py" \
  "${OUT_DIR}/${TARGET}" \
  "${OUT_DIR}/${RUNMAT_TARGET}" \
  "${OUT_DIR}/${P3_TARGET}" \
  "${OUT_DIR}/${CS_TARGET}" \
  "${OUT_DIR}/${NOTES_TARGET}"
for ext in bin elf map; do
  src="${SRC_DIR}/khicasen.${ext}"
  [ ! -f "${src}" ] || cp "${src}" "${OUT_DIR}/khicasen.${ext}"
done
for ext in bin elf map; do
  src="${SRC_DIR}/runmat_mock.${ext}"
  [ ! -f "${src}" ] || cp "${src}" "${OUT_DIR}/runmat_mock.${ext}"
done
for app in CASP3 CSCALC NOTES; do
  for ext in bin elf map; do
    src="${SRC_DIR}/${app}.${ext}"
    [ ! -f "${src}" ] || cp "${src}" "${OUT_DIR}/${app}.${ext}"
  done
done

python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/${TARGET}" \
  --name CAS \
  --internal @CAS \
  --filename "${TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/${RUNMAT_TARGET}" \
  --name RunMat \
  --internal @RUNMAT \
  --filename "${RUNMAT_TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${RUNMAT_TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/${P3_TARGET}" \
  --name CASP3 \
  --internal @CASP3 \
  --filename "${P3_TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${P3_TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/${CS_TARGET}" \
  --name CSCalc \
  --internal @CSCALC \
  --filename "${CS_TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${CS_TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/${NOTES_TARGET}" \
  --name Notes \
  --internal @NOTES \
  --filename "${NOTES_TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${NOTES_TARGET}"
python3 "${ROOT_DIR}/tools/build_help_pack.py" \
  "${ROOT_DIR}/help/functions" \
  "${OUT_DIR}/${PACK_TARGET}"

cp "${OUT_DIR}/${TARGET}" "${TRANSFER_DIR}/${TARGET}"
cp "${OUT_DIR}/${RUNMAT_TARGET}" "${TRANSFER_DIR}/${RUNMAT_TARGET}"
cp "${OUT_DIR}/${P3_TARGET}" "${TRANSFER_DIR}/${P3_TARGET}"
cp "${OUT_DIR}/${CS_TARGET}" "${TRANSFER_DIR}/${CS_TARGET}"
cp "${OUT_DIR}/${NOTES_TARGET}" "${TRANSFER_DIR}/${NOTES_TARGET}"
cp "${OUT_DIR}/${PACK_TARGET}" "${TRANSFER_DIR}/${PACK_TARGET}"
clean_source_outputs
rm -f "${SRC_DIR}/khicasio.png" "${SRC_DIR}/khicasio1.png" \
  "${SRC_DIR}/casio_suite_ui.hpp" \
  "${SRC_DIR}/p3_engine.hpp" "${SRC_DIR}/p3_engine.cpp" \
  "${SRC_DIR}/cscalc_engine.hpp" "${SRC_DIR}/cscalc_engine.cpp" \
  "${SRC_DIR}/khicas_suite_bridge.hpp" "${SRC_DIR}/khicas_suite_bridge.cpp" \
  "${SRC_DIR}/notes_app.cc"

ls -lh "${TRANSFER_DIR}/${TARGET}" "${TRANSFER_DIR}/${RUNMAT_TARGET}" "${TRANSFER_DIR}/${P3_TARGET}" "${TRANSFER_DIR}/${CS_TARGET}" "${TRANSFER_DIR}/${NOTES_TARGET}" "${TRANSFER_DIR}/${PACK_TARGET}"
shasum -a 256 "${TRANSFER_DIR}/${TARGET}" "${TRANSFER_DIR}/${RUNMAT_TARGET}" "${TRANSFER_DIR}/${P3_TARGET}" "${TRANSFER_DIR}/${CS_TARGET}" "${TRANSFER_DIR}/${NOTES_TARGET}" "${TRANSFER_DIR}/${PACK_TARGET}"
