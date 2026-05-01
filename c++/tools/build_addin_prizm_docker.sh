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
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.bin"
rm -rf "${ROOT_DIR}/c++/prizm/build"
mkdir -p "${ROOT_DIR}/c++/prizm/build"

echo ""
echo "=== Building native Prizm add-in ==="
docker run --rm \
  -v "${ROOT_DIR}:/work" \
  -w /work/c++/prizm \
  "${IMAGE_TAG}" \
  bash -lc '
    echo "FXCGSDK=$FXCGSDK"
    sh3eb-elf-g++ --version | head -1
    make clean
    make -j"$(nproc)"
  '

echo ""
echo "=== Moving .bin to build directory ==="
if [ -f "${ROOT_DIR}/c++/prizm/CasioCAS.bin" ]; then
  mv "${ROOT_DIR}/c++/prizm/CasioCAS.bin" "${ROOT_DIR}/c++/prizm/build/CasioCAS.bin"
fi

echo ""
echo "=== Build Results ==="
if [ -f "${ROOT_DIR}/c++/prizm/build/CasioCAS.bin" ]; then
  ls -lh "${ROOT_DIR}/c++/prizm/build/CasioCAS.bin"
  echo "Output (binary): ${ROOT_DIR}/c++/prizm/build/CasioCAS.bin"
  echo "[Note: .g3a packaging requires mkg3a, to be integrated in Phase 10]"
else
  echo "No Prizm .bin produced"
  exit 1
fi
