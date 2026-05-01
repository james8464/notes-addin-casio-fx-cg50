#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_TAG="casio-prizmsdk:latest"

echo "=== Building PrizmSDK Docker image ==="
docker build \
  -f "${ROOT_DIR}/c++/tools/docker/Dockerfile.prizmsdk" \
  -t "${IMAGE_TAG}" \
  "${ROOT_DIR}"

echo ""
echo "=== Removing stale Prizm outputs ==="
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.g3a"
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.bin"
rm -rf "${ROOT_DIR}/c++/prizm/build"

echo ""
echo "=== Building native Prizm add-in ==="
docker run --rm \
  -v "${ROOT_DIR}:/work" \
  -w /work/c++/prizm \
  "${IMAGE_TAG}" \
  bash -lc '
    echo "FXCGSDK=$FXCGSDK"
    sh3eb-elf-g++ --version | head -1
    mkg3a -h | head -5
    make clean
    make -j"$(nproc)"
  '

echo ""
echo "=== Build Results ==="
if [[ -f "${ROOT_DIR}/c++/prizm/CasioCAS.g3a" ]]; then
  mkdir -p "${ROOT_DIR}/c++/prizm/build"
  mv "${ROOT_DIR}/c++/prizm/CasioCAS.g3a" "${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a"
fi

if [[ -f "${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a" ]]; then
  ls -lh "${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a"
  echo "Output: ${ROOT_DIR}/c++/prizm/build/CasioCAS.g3a"
else
  echo "No Prizm .g3a produced"
  exit 1
fi
