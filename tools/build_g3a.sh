#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${CASIO_KHICAS_IMAGE:-casio-khicas-source:latest}"
SRC_DIR="${ROOT_DIR}/khicas/upstream/giac90_1addin"
OUT_DIR="${ROOT_DIR}/build"
TRANSFER_DIR="${ROOT_DIR}/calculator_files"
TARGET="${CASIO_KHICAS_TARGET:-CAS.g3a}"
PACK_TARGET="${CASIO_HELP_PACK_TARGET:-${TARGET%.*}.PAK}"
MAKE_JOBS="${CASIO_MAKE_JOBS:-1}"

ensure_image() {
  if ! docker image inspect "${IMAGE_TAG}" >/dev/null 2>&1; then
    docker build \
      --platform linux/amd64 \
      -f "${ROOT_DIR}/tools/docker/Dockerfile.khicas-source" \
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
import shutil
import sys

root = Path(sys.argv[1])
try:
    from PIL import Image

    for src, dst in (("unselected.bmp", "khicasio.png"), ("selected.bmp", "khicasio1.png")):
        icon = Image.open(root / src).convert("RGB")
        icon.save(root / dst, optimize=True, compress_level=9)
except Exception:
    shutil.copyfile(root / "logo.png", root / "khicasio.png")
    shutil.copyfile(root / "logo.png", root / "khicasio1.png")
PY
}

mkdir -p "${OUT_DIR}" "${TRANSFER_DIR}"
rm -rf "${OUT_DIR:?}"/* "${TRANSFER_DIR:?}"/*
touch "${TRANSFER_DIR}/.gitkeep"

ensure_image
clean_source_outputs
prepare_icons

docker run --rm \
  --platform linux/amd64 \
  -e CASIO_MAKE_JOBS="${MAKE_JOBS}" \
  -e CASIO_KHICAS_TARGET="${TARGET}" \
  -v "${ROOT_DIR}:/work" \
  -w /work/khicas/upstream/giac90_1addin \
  "${IMAGE_TAG}" \
  bash -lc 'set -euo pipefail; mkdir -p /shared/tmp ~/.wine/drive_c; make clean; rm -f "${CASIO_KHICAS_TARGET}"; make -j"${CASIO_MAKE_JOBS}" "${CASIO_KHICAS_TARGET}"'

cp "${SRC_DIR}/${TARGET}" "${OUT_DIR}/${TARGET}"
for ext in bin elf map; do
  src="${SRC_DIR}/khicasen.${ext}"
  [ ! -f "${src}" ] || cp "${src}" "${OUT_DIR}/khicasen.${ext}"
done

python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/${TARGET}" \
  --name CAS \
  --internal @CAS \
  --filename "${TARGET}"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${TARGET}"
python3 "${ROOT_DIR}/tools/build_help_pack.py" \
  "${ROOT_DIR}/help/functions" \
  "${OUT_DIR}/${PACK_TARGET}"

cp "${OUT_DIR}/${TARGET}" "${TRANSFER_DIR}/${TARGET}"
cp "${OUT_DIR}/${PACK_TARGET}" "${TRANSFER_DIR}/${PACK_TARGET}"
clean_source_outputs
rm -f "${SRC_DIR}/khicasio.png" "${SRC_DIR}/khicasio1.png"

ls -lh "${TRANSFER_DIR}/${TARGET}" "${TRANSFER_DIR}/${PACK_TARGET}"
shasum -a 256 "${TRANSFER_DIR}/${TARGET}" "${TRANSFER_DIR}/${PACK_TARGET}"
