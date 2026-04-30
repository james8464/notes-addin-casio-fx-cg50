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

echo ""
echo "Build outputs:"
if compgen -G "build/*.g3a" >/dev/null; then
  for f in build/*.g3a; do
    abs="$(cd "$(dirname "$f")" && pwd)/$(basename "$f")"
    echo "  - ${abs}"
  done
else
  echo "  (no .g3a found under addin/build/)"
fi

echo ""
echo "Next:"
echo "  - Copy the .g3a to the calculator (USB storage) and safely eject."
echo "  - Run: python3 tools/check_g3a_size.py <path/to.g3a>"
echo "  - Follow: addin/DEVICE_SMOKE_CHECKLIST.md"

