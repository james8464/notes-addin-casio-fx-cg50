#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="casio-fxsdk:latest"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker not found. Install Docker Desktop then retry."
  exit 1
fi

echo "Building fxSDK Docker image (cached)..."
docker build \
  -f "${ROOT_DIR}/tools/docker/Dockerfile.fxsdk" \
  -t "${IMAGE_TAG}" \
  "${ROOT_DIR}"

echo ""
echo "Building add-in in container..."
docker run --rm \
  -v "${ROOT_DIR}:/work" \
  -w /work/addin \
  "${IMAGE_TAG}" \
  bash -lc "fxsdk build-cg"

echo ""
echo "Build outputs:"
if compgen -G "${ROOT_DIR}/addin/build/*.g3a" >/dev/null; then
  for f in "${ROOT_DIR}"/addin/build/*.g3a; do
    echo "  - ${f}"
  done
else
  echo "  (no .g3a found under addin/build/)"
fi

echo ""
echo "Next:"
echo "  - Run: python3 tools/check_g3a_size.py addin/build/*.g3a"
echo "  - Copy the .g3a to the calculator and safely eject."
echo "  - Follow: addin/DEVICE_SMOKE_CHECKLIST.md"

