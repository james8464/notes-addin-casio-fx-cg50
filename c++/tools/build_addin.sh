#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Check if fxsdk is installed
if ! command -v fxsdk >/dev/null 2>&1; then
  echo "❌ fxsdk not found in PATH"
  echo ""
  echo "Install fxSDK on macOS using one of these methods:"
  echo ""
  echo "Method 1: Using GiteaPC (recommended)"
  echo "  curl -fsSL https://git.planet-casio.com/Lephenixnoir/GiteaPC/raw/branch/master/install.sh | sh"
  echo "  export PATH=\"\$PATH:\$HOME/.local/bin\""  
  echo "  giteapc install Lephenixnoir/fxsdk"
  echo ""
  echo "Method 2: Manual build from source"
  echo "  git clone https://git.planet-casio.com/Lephenixnoir/fxsdk"
  echo "  cd fxsdk && mkdir build && cd build && cmake .. && make install"
  echo ""
  exit 1
fi

echo "=== Building add-in with fxSDK ==="
fxsdk --version
echo ""

cd "${ROOT_DIR}/c++/addin"
echo "Building from: $(pwd)"
fxsdk build-cg

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
