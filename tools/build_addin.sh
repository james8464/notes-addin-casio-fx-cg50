#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v fxsdk >/dev/null 2>&1; then
  echo "fxsdk not found on PATH."
  echo "Install fxSDK (recommended via GiteaPC) then retry."
  echo "Docs: https://git.planet-casio.com/Lephenixnoir/fxsdk/src/branch/master/README.md"
  exit 1
fi

cd "${ROOT_DIR}/addin"
fxsdk build-cg
echo "Built add-in (look under addin/build/ for .g3a)."

