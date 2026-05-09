#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_TAG="casio-prizmsdk:latest"
SOURCE_IMAGE_TAG="casio-khicas-source:latest"
MODE="${CASIO_PRIZM_MODE:-khicas-source}"
UPSTREAM_G3A="${ROOT_DIR}/c++/khicas/upstream/reference/khicasen.g3a"
KHICAS_SRC="${ROOT_DIR}/c++/khicas/upstream/giac90_1addin"
OUT_DIR="${ROOT_DIR}/c++/prizm/build"
OUT_G3A="${OUT_DIR}/CasioCAS.g3a"
TRANSFER_DIR="${ROOT_DIR}/calculator_files"
TRANSFER_G3A="${TRANSFER_DIR}/CasioCAS.g3a"
HELP_SRC="${ROOT_DIR}/c++/prizm/help/CASIOCAS.HLP"
TEMPLATE_DIR="${ROOT_DIR}/c++/prizm/help"
OUT_PAK="${OUT_DIR}/CASIOCAS.PAK"
TRANSFER_PAK="${TRANSFER_DIR}/CASIOCAS.PAK"
ICON_SEL="${ROOT_DIR}/c++/prizm/assets/selected.bmp"
ICON_UNSEL="${ROOT_DIR}/c++/prizm/assets/unselected.bmp"
ICON_SEL_PNG="${ROOT_DIR}/c++/prizm/assets/selected.png"
ICON_UNSEL_PNG="${ROOT_DIR}/c++/prizm/assets/unselected.png"

ensure_prizm_image() {
  if ! docker image inspect "${IMAGE_TAG}" >/dev/null 2>&1; then
    echo "=== Building PrizmSDK Docker image ==="
    docker build \
      -f "${ROOT_DIR}/c++/tools/docker/Dockerfile.prizmsdk" \
      -t "${IMAGE_TAG}" \
      "${ROOT_DIR}"
  fi
}

ensure_source_image() {
  if ! docker image inspect "${SOURCE_IMAGE_TAG}" >/dev/null 2>&1; then
    echo "=== Building KhiCAS source Docker image (amd64) ==="
    docker build \
      --platform linux/amd64 \
      -f "${ROOT_DIR}/c++/tools/docker/Dockerfile.khicas-source" \
      -t "${SOURCE_IMAGE_TAG}" \
      "${ROOT_DIR}"
  fi
}

clean_khicas_source_outputs() {
  find "${KHICAS_SRC}" -maxdepth 1 -type f \
    \( -name '*.o' -o -name 'libcas.a' -o -name 'libgui.a' \
       -o -name 'khicasen.elf' -o -name 'khicasen.bin' \
       -o -name 'khicasen.g3a' -o -name 'khicasen.map' \
       -o -name 'dumpen_t' \) \
    -delete
}

publish_transfer_g3a() {
  mkdir -p "${TRANSFER_DIR}"
  cp "${OUT_G3A}" "${TRANSFER_G3A}"
  echo "Calculator file: ${TRANSFER_G3A}"
}

publish_external_pack() {
  if [ -f "${HELP_SRC}" ]; then
    template_sources=()
    while IFS= read -r template_source; do
      template_sources+=("${template_source}")
    done < <(find "${TEMPLATE_DIR}" -maxdepth 1 -type f -name 'CASIOCAS*.TPL' | sort)
    if [ "${#template_sources[@]}" -gt 0 ]; then
      python3 "${ROOT_DIR}/c++/tools/build_external_pack.py" "${HELP_SRC}" "${template_sources[@]}" "${OUT_PAK}"
    else
      python3 "${ROOT_DIR}/c++/tools/build_external_pack.py" "${HELP_SRC}" "${OUT_PAK}"
    fi
    mkdir -p "${TRANSFER_DIR}"
    cp "${OUT_PAK}" "${TRANSFER_PAK}"
    echo "External pack: ${OUT_PAK}"
    echo "Calculator external pack: ${TRANSFER_PAK}"
  fi
}

report_transfer_sizes() {
  if [ -f "${TRANSFER_G3A}" ]; then
    local g3a_size
    g3a_size="$(wc -c < "${TRANSFER_G3A}" | tr -d ' ')"
    echo "Calculator .g3a bytes: ${g3a_size}"
    if [ "${g3a_size}" -gt 2000000 ]; then
      echo "WARN: .g3a is close to the practical 2MB add-in limit; keep help/data external." >&2
    fi
  fi
  if [ -d "${TRANSFER_DIR}" ]; then
    echo "Calculator transfer folder:"
    find "${TRANSFER_DIR}" -maxdepth 1 -type f -print0 | xargs -0 ls -lh
  fi
}

publish_downloads_transfer() {
  local downloads_root="${CASIO_DOWNLOADS_DIR:-${HOME}/Downloads}"
  local downloads_transfer="${downloads_root}/calculator_files"
  if [ ! -d "${downloads_root}" ]; then
    echo "Downloads folder not found; transfer files ready in ${TRANSFER_DIR}."
    return 0
  fi
  rm -rf "${downloads_transfer}"
  cp -R "${TRANSFER_DIR}" "${downloads_transfer}"
  echo "Downloads transfer folder: ${downloads_transfer}"
}

report_addin_size() {
  python3 "${ROOT_DIR}/c++/tools/check_g3a_size.py" "${OUT_G3A}"
  python3 "${ROOT_DIR}/c++/tools/size_report.py" --baseline current
}

echo ""
echo "=== Removing stale Prizm outputs ==="
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.bin"
rm -f "${ROOT_DIR}/c++/prizm/CasioCAS.g3a"
rm -f "${ROOT_DIR}/CasioCAS.g3a"
rm -f "${ROOT_DIR}/CASIOCAS.HLP"
rm -f "${ROOT_DIR}/CASIOCAS.PAK"
rm -rf "${TRANSFER_DIR}"
rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

if [ "${MODE}" = "khicas-source" ]; then
  echo ""
  echo "=== Building edited KhiCAS source ==="
  if [ ! -f "${ICON_SEL_PNG}" ] || [ ! -f "${ICON_UNSEL_PNG}" ]; then
    echo "Missing Eigenmath-style PNG icon assets in c++/prizm/assets" >&2
    exit 1
  fi
  clean_khicas_source_outputs
  ensure_source_image
  docker run --rm \
    --platform linux/amd64 \
    -v "${ROOT_DIR}:/work" \
    -w /work/c++/khicas/upstream/giac90_1addin \
    "${SOURCE_IMAGE_TAG}" \
    bash -lc '
      set -euo pipefail
      make clean
      make -j"$(nproc)" khicasen.g3a ICON_UNS=/work/c++/prizm/assets/unselected.png ICON_SEL=/work/c++/prizm/assets/selected.png
    '
  cp "${KHICAS_SRC}/khicasen.g3a" "${OUT_G3A}"
  cp "${KHICAS_SRC}/khicasen.bin" "${OUT_DIR}/CasioCAS.bin"
  cp "${KHICAS_SRC}/khicasen.elf" "${OUT_DIR}/CasioCAS.elf"
  cp "${KHICAS_SRC}/khicasen.map" "${OUT_DIR}/CasioCAS.map"
  cp "${KHICAS_SRC}/dumpen_t" "${OUT_DIR}/dumpen_t"
  clean_khicas_source_outputs
  python3 "${ROOT_DIR}/c++/tools/patch_g3a_metadata.py" "${OUT_G3A}" \
    --name "CasioCAS" \
    --internal "CASCAS" \
    --filename "CasioCAS.g3a"
  python3 "${ROOT_DIR}/c++/tools/check_g3a_metadata.py" "${OUT_G3A}"
  report_addin_size
  publish_transfer_g3a
  publish_external_pack
  publish_downloads_transfer
  report_transfer_sizes
  ls -lh "${OUT_G3A}"
  [ ! -f "${OUT_PAK}" ] || ls -lh "${OUT_PAK}"
  shasum -a 256 "${OUT_G3A}"
  echo "Output (source-built): ${OUT_G3A}"
  exit 0
fi

if [ "${MODE}" = "khicas-reference" ] || [ "${MODE}" = "khicas-upstream" ]; then
  echo ""
  echo "=== Using exact upstream KhiCAS reference ==="
  if [ ! -f "${UPSTREAM_G3A}" ]; then
    echo "Missing upstream reference: ${UPSTREAM_G3A}" >&2
    exit 1
  fi
  if [ ! -f "${ICON_SEL}" ] || [ ! -f "${ICON_UNSEL}" ]; then
    echo "Missing Eigenmath-style icon assets in c++/prizm/assets" >&2
    exit 1
  fi
  cp "${UPSTREAM_G3A}" "${OUT_G3A}"
  ensure_prizm_image
  docker run --rm \
    -v "${ROOT_DIR}:/work" \
    -w /work \
    "${IMAGE_TAG}" \
    g3a-updateicon "c++/prizm/build/CasioCAS.g3a" \
      "c++/prizm/assets/selected.bmp" \
      "c++/prizm/assets/unselected.bmp"
  python3 "${ROOT_DIR}/c++/tools/patch_g3a_metadata.py" "${OUT_G3A}" \
    --name "CasioCAS" \
    --internal "CASCAS" \
    --filename "CasioCAS.g3a"
  python3 "${ROOT_DIR}/c++/tools/check_g3a_metadata.py" "${OUT_G3A}"
  report_addin_size
  publish_transfer_g3a
  publish_external_pack
  publish_downloads_transfer
  report_transfer_sizes
  ls -lh "${OUT_G3A}"
  [ ! -f "${OUT_PAK}" ] || ls -lh "${OUT_PAK}"
  shasum -a 256 "${OUT_G3A}"
  echo "Output (packaged): ${OUT_G3A}"
  exit 0
fi

ensure_prizm_image

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
  python3 "${ROOT_DIR}/c++/tools/check_g3a_metadata.py" "${OUT_G3A}"
  report_addin_size
  publish_transfer_g3a
  publish_external_pack
  publish_downloads_transfer
  report_transfer_sizes
  ls -lh "${OUT_G3A}"
  [ ! -f "${OUT_PAK}" ] || ls -lh "${OUT_PAK}"
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
