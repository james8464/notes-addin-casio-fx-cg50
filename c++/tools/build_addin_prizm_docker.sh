#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_TAG="casio-prizmsdk:latest"
MODE="${CASIO_PRIZM_MODE:-khicas-upstream}"
UPSTREAM_G3A="${ROOT_DIR}/c++/khicas/upstream/reference/khicasen.g3a"
OUT_DIR="${ROOT_DIR}/c++/prizm/build"
OUT_G3A="${OUT_DIR}/CasioCAS.g3a"

echo ""
echo "=== Removing stale Prizm outputs ==="
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.bin"
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.g3a"
rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

if [ "${MODE}" = "khicas-upstream" ]; then
  echo ""
  echo "=== Using exact upstream KhiCAS reference ==="
  if [ ! -f "${UPSTREAM_G3A}" ]; then
    echo "Missing upstream reference: ${UPSTREAM_G3A}" >&2
    exit 1
  fi
  cp "${UPSTREAM_G3A}" "${OUT_G3A}"
  ls -lh "${OUT_G3A}"
  shasum -a 256 "${OUT_G3A}"
  echo "Output (packaged): ${OUT_G3A}"
  echo "Set CASIO_PRIZM_MODE=legacy to build the old small native port."
  exit 0
fi

echo "=== Building PrizmSDK Docker image ==="
docker build \
  -f "${ROOT_DIR}/c++/tools/docker/Dockerfile.prizmsdk" \
  -t "${IMAGE_TAG}" \
  "${ROOT_DIR}"

echo ""
echo "=== Building native Prizm add-in ==="
docker run --rm \
  -v "${ROOT_DIR}:/work" \
  -w /work/c++/prizm \
  "${IMAGE_TAG}" \
  bash -lc '
    echo "FXCGSDK=$FXCGSDK"
    sh3eb-elf-g++ --version | head -1
    mkg3a -h | head -1
    make clean
    make -j"$(nproc)"
  '

echo ""
echo "=== Moving outputs to build directory ==="
if [ -f "${ROOT_DIR}/c++/prizm/CasioCAS.g3a" ]; then
  mv "${ROOT_DIR}/c++/prizm/CasioCAS.g3a" "${OUT_G3A}"
fi
if [ -f "${ROOT_DIR}/c++/prizm/CasioCAS.bin" ]; then
  mv "${ROOT_DIR}/c++/prizm/CasioCAS.bin" "${OUT_DIR}/CasioCAS.bin"
fi

echo ""
echo "=== Build Results ==="
if [ -f "${OUT_G3A}" ]; then
  ls -lh "${OUT_G3A}"
  echo "Output (packaged): ${OUT_G3A}"
else
  echo "Checking for .bin..."
  if [ -f "${OUT_DIR}/CasioCAS.bin" ]; then
    ls -lh "${OUT_DIR}/CasioCAS.bin"
    echo "Output (binary): ${OUT_DIR}/CasioCAS.bin"
  else
    echo "No Prizm output produced"
    exit 1
  fi
fi
