#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/addin/host/build"
PYTHON_BIN="${CASIO_PYTHON:-${PYTHON:-python3}}"

cmake_bin="$(command -v cmake || true)"
if [[ -z "${cmake_bin}" ]]; then
  user_base="$("${PYTHON_BIN}" -c 'import site; print(site.USER_BASE)')"
  if [[ -x "${user_base}/bin/cmake" ]]; then
    cmake_bin="${user_base}/bin/cmake"
  else
    echo "cmake not found; installing the Python cmake wheel into ${user_base}..."
    "${PYTHON_BIN}" -m pip install --user cmake
    if [[ -x "${user_base}/bin/cmake" ]]; then
      cmake_bin="${user_base}/bin/cmake"
    else
      echo "cmake install finished but ${user_base}/bin/cmake is not executable."
      echo "Install manually with: python3 -m pip install --user cmake"
      exit 1
    fi
  fi
fi

mkdir -p "${BUILD_DIR}"
"${cmake_bin}" -S "${ROOT_DIR}/addin/host" -B "${BUILD_DIR}"
"${cmake_bin}" --build "${BUILD_DIR}" -j
"${BUILD_DIR}/device_solver_smoke"
echo "Built: ${BUILD_DIR}/casio_host"
