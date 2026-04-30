#!/usr/bin/env bash
set -euo pipefail

# Repo root (this script lives in c++/tools/)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_TAG="casio-fxsdk:latest"

# Diagnostics
echo "=== Docker Build Diagnostics ==="
echo "Host: $(uname -s)"
docker --version || (echo "ERROR: Docker not found"; exit 1)
echo ""

if ! command -v docker >/dev/null 2>&1; then
  echo "docker not found. Install Docker Desktop then retry."
  exit 1
fi

echo "=== Building fxSDK Docker image ==="
docker build \
  -f "${ROOT_DIR}/c++/tools/docker/Dockerfile.fxsdk" \
  -t "${IMAGE_TAG}" \
  "${ROOT_DIR}"

echo ""
echo "=== Building add-in in container ==="
echo "=== Removing stale add-in outputs ==="
if [[ -d "${ROOT_DIR}/c++/addin/build-cg" ]]; then
  stale_outputs=()
  while IFS= read -r f; do
    stale_outputs+=("${f}")
  done < <(
    find "${ROOT_DIR}/c++/addin/build-cg" -maxdepth 1 -type f \
      \( -name '*.g3a' -o -name 'CasioCAS' -o -name 'CasioCAS.bin' \) \
      | sort
  )
  if [[ ${#stale_outputs[@]} -gt 0 ]]; then
    for f in "${stale_outputs[@]}"; do
      echo "Removing stale output: ${f}"
      rm -f "${f}"
    done
  else
    echo "No existing .g3a output found; creating a fresh one."
  fi
else
  echo "No build-cg directory yet; fxsdk will create it."
fi

docker run --rm \
  -v "${ROOT_DIR}:/work" \
  -w /work/c++/addin \
  "${IMAGE_TAG}" \
  bash -lc "
    echo 'Container environment check:' && \
    giteapc --help > /dev/null && echo '✓ giteapc available' || echo '✗ giteapc missing' && \
    fxsdk --version && echo '✓ fxsdk available' || echo '✗ fxsdk missing' && \
    echo '' && \
    echo 'Building add-in...' && \
    fxsdk build-cg
  "

echo ""
echo "=== Build Results ==="
g3a_files=()
while IFS= read -r f; do
  g3a_files+=("${f}")
done < <(find "${ROOT_DIR}/c++/addin/build-cg" -type f -name '*.g3a' | sort)

if [[ ${#g3a_files[@]} -gt 0 ]]; then
  for f in "${g3a_files[@]}"; do
    echo "✓ Output: ${f}"
  done
else
  echo "✗ No .g3a files found under c++/addin/build-cg/"
  exit 1
fi

echo ""
echo "=== Next Steps ==="
echo "1. Verify size:     python3 c++/tools/check_g3a_size.py c++/addin/build-cg/*.g3a"
echo "2. Transfer:        Copy .g3a to calculator"
echo "3. Verify:          Follow c++/addin/DEVICE_SMOKE_CHECKLIST.md"
