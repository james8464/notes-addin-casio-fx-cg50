#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_TAG="${CASIO_NOTES_IMAGE:-casio-notes-source:latest}"
SRC_DIR="${ROOT_DIR}/khicas/upstream/giac90_1addin"
OUT_DIR="${ROOT_DIR}/build"
TRANSFER_DIR="${ROOT_DIR}/calculator_files"
MAKE_JOBS="${CASIO_MAKE_JOBS:-1}"
IMAGE_VERSION="notes-v1"
DOCKER_BUILD_SCRIPT="${OUT_DIR}/docker_build_notes.sh"

usage() {
  cat <<'EOF'
Usage: ./compile [notes]

Targets:
  notes   build NOTES.g3a only
EOF
}

want() {
  local needle="$1"
  for t in "${TARGETS[@]}"; do
    [ "$t" = "$needle" ] && return 0
  done
  return 1
}

TARGETS=("$@")
if [ "${#TARGETS[@]}" -eq 0 ]; then
  TARGETS=(notes)
fi
for t in "${TARGETS[@]}"; do
  case "$t" in
    notes) ;;
    -h|--help|help) usage; exit 0 ;;
    *) usage >&2; exit 2 ;;
  esac
done

ensure_image() {
  if ! docker image inspect "${IMAGE_TAG}" >/dev/null 2>&1; then
    docker build \
      --platform linux/amd64 \
      -f "${ROOT_DIR}/tools/docker/Dockerfile.notes-source" \
      --label "casio.notes.image-version=${IMAGE_VERSION}" \
      -t "${IMAGE_TAG}" \
      "${ROOT_DIR}"
  fi
}

clean_build_outputs() {
  find "${SRC_DIR}" -maxdepth 1 \( -type f -o -type l \) \
    \( -name '*.o' -o -name '*.a' -o -name '*.elf' -o -name '*.bin' \
       -o -name '*.g3a' -o -name '*.map' \) \
    -delete
}

stage_sources() {
  cp "${ROOT_DIR}/apps/notes/notes_app.cc" "${SRC_DIR}/notes_app.cc"
  cp "${ROOT_DIR}/shared/casio/casio_suite_ui.hpp" "${SRC_DIR}/casio_suite_ui.hpp"
}

cleanup_staged_sources() {
  rm -f "${SRC_DIR}/casio_suite_ui.hpp" "${SRC_DIR}/notes_app.cc"
  clean_build_outputs
}

mkdir -p "${OUT_DIR}" "${TRANSFER_DIR}"
rm -rf "${OUT_DIR:?}"/*
touch "${TRANSFER_DIR}/.gitkeep"
find "${TRANSFER_DIR}" -mindepth 1 \
  \( -name 'NOTES' -o -name '.gitkeep' \) -prune \
  -o -exec rm -rf {} +

cat > "${DOCKER_BUILD_SCRIPT}" <<'SH'
#!/usr/bin/env bash
set -euo pipefail
: "${CASIO_MAKE_JOBS:=1}"
mkdir -p /shared/tmp
rm -rf /tmp/giac90_1addin
cp -a /work/khicas/upstream/giac90_1addin /tmp/giac90_1addin
cd /tmp/giac90_1addin
make clean
make -j"${CASIO_MAKE_JOBS}" NOTES.g3a
for f in NOTES.g3a NOTES.bin NOTES.elf NOTES.map; do
  [ ! -f "${f}" ] || cp "${f}" /work/khicas/upstream/giac90_1addin/
done
SH

chmod +x "${DOCKER_BUILD_SCRIPT}"
ensure_image
clean_build_outputs
stage_sources

docker run --rm \
  --platform linux/amd64 \
  -e CASIO_MAKE_JOBS="${MAKE_JOBS}" \
  -v "${ROOT_DIR}:/work" \
  -w /work/khicas/upstream/giac90_1addin \
  "${IMAGE_TAG}" \
  bash /work/build/docker_build_notes.sh

cp "${SRC_DIR}/NOTES.g3a" "${OUT_DIR}/NOTES.g3a"
python3 "${ROOT_DIR}/tools/build/normalize_g3a_metadata.py" "${OUT_DIR}/NOTES.g3a"
python3 "${ROOT_DIR}/tools/checks/check_g3a_metadata.py" "${OUT_DIR}/NOTES.g3a" --name Notes --internal @NOTES --filename NOTES.g3a
python3 "${ROOT_DIR}/tools/checks/check_g3a_size.py" "${OUT_DIR}/NOTES.g3a"
cp "${OUT_DIR}/NOTES.g3a" "${TRANSFER_DIR}/NOTES.g3a"

cleanup_staged_sources
