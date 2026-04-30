#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/addin/host/build"

cmake_bin="$(command -v cmake || true)"
if [[ -z "${cmake_bin}" ]]; then
  user_base="$(python3 -c 'import site; print(site.USER_BASE)')"
  if [[ -x "${user_base}/bin/cmake" ]]; then
    cmake_bin="${user_base}/bin/cmake"
  else
    echo "cmake not found."
    echo "Install with: python3 -m pip install --user cmake"
    echo "Then ensure: ${user_base}/bin is on PATH"
    exit 1
  fi
fi

mkdir -p "${BUILD_DIR}"
"${cmake_bin}" -S "${ROOT_DIR}/addin/host" -B "${BUILD_DIR}"
"${cmake_bin}" --build "${BUILD_DIR}" -j
"${BUILD_DIR}/device_solver_smoke"
echo "Built: ${BUILD_DIR}/casio_host"
