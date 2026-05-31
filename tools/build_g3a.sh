#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${CASIO_KHICAS_IMAGE:-casio-khicas-source:latest}"
SRC_DIR="${ROOT_DIR}/khicas/upstream/giac90_1addin"
OUT_DIR="${ROOT_DIR}/build"
TRANSFER_DIR="${ROOT_DIR}/calculator_files"
TARGET="${CASIO_KHICAS_TARGET:-khicas50.g3a}"
RAM_TARGET="${CASIO_KHICAS_RAM_TARGET:-khicas50.ac2}"
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
  find "${SRC_DIR}" -maxdepth 1 -type f \
    \( -name '*.o' -o -name '*.a' -o -name '*.elf' -o -name '*.bin' \
       -o -name '*.g3a' -o -name '*.map' -o -name '*.ac2' -o -name '*.882' \
       -o -name 'dump*' -o -name 'mkhelp' \) \
    -delete
}

mkdir -p "${OUT_DIR}" "${TRANSFER_DIR}"
rm -rf "${OUT_DIR:?}"/* "${TRANSFER_DIR:?}"/*
touch "${TRANSFER_DIR}/.gitkeep"

ensure_image
clean_source_outputs

docker run --rm \
  --platform linux/amd64 \
  -e CASIO_MAKE_JOBS="${MAKE_JOBS}" \
  -e CASIO_KHICAS_TARGET="${TARGET}" \
  -e CASIO_KHICAS_RAM_TARGET="${RAM_TARGET}" \
  -v "${ROOT_DIR}:/work" \
  -w /work/khicas/upstream/giac90_1addin \
  "${IMAGE_TAG}" \
  bash -lc 'set -euo pipefail; mkdir -p /shared/tmp ~/.wine/drive_c; make clean; make -j"${CASIO_MAKE_JOBS}" "${CASIO_KHICAS_TARGET}" "${CASIO_KHICAS_RAM_TARGET}"'

cp "${SRC_DIR}/${TARGET}" "${OUT_DIR}/CasioCAS.g3a"
cp "${SRC_DIR}/${RAM_TARGET}" "${OUT_DIR}/${RAM_TARGET}"
for ext in bin elf map; do
  src="${SRC_DIR}/khicasen.${ext}"
  [ ! -f "${src}" ] || cp "${src}" "${OUT_DIR}/CasioCAS.${ext}"
done

python3 "${ROOT_DIR}/tools/patch_g3a_metadata.py" "${OUT_DIR}/CasioCAS.g3a" \
  --name CasioCAS \
  --internal CASCAS \
  --filename /CasioCAS.g3a
python3 "${ROOT_DIR}/tools/check_g3a_metadata.py" "${OUT_DIR}/CasioCAS.g3a"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/CasioCAS.g3a"
python3 "${ROOT_DIR}/tools/check_g3a_size.py" "${OUT_DIR}/${RAM_TARGET}"
python3 "${ROOT_DIR}/tools/build_pack.py" "${ROOT_DIR}/help" "${OUT_DIR}/CASIOCAS.PAK"

cp "${OUT_DIR}/CasioCAS.g3a" "${TRANSFER_DIR}/CasioCAS.g3a"
cp "${OUT_DIR}/${RAM_TARGET}" "${TRANSFER_DIR}/${RAM_TARGET}"
cp "${OUT_DIR}/CASIOCAS.PAK" "${TRANSFER_DIR}/CASIOCAS.PAK"
clean_source_outputs

ls -lh "${TRANSFER_DIR}/CasioCAS.g3a" "${TRANSFER_DIR}/${RAM_TARGET}" "${TRANSFER_DIR}/CASIOCAS.PAK"
shasum -a 256 "${TRANSFER_DIR}/CasioCAS.g3a"
shasum -a 256 "${TRANSFER_DIR}/${RAM_TARGET}"
