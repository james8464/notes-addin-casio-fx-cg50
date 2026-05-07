#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "c++/tools/build_addin_prizm_docker.sh"
PRUNE = ROOT / "c++/tools/prove_prune_objects.py"
PACK = ROOT / "c++/tools/build_external_pack.py"
CHECK_PACK = ROOT / "c++/tools/check_external_pack.py"
SIZE_REPORT = ROOT / "c++/tools/size_report.py"
FLAGS = ROOT / "c++/tools/prove_size_flags.py"
SYNC = ROOT / "c++/tools/sync_calculator_volume.py"


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def main() -> int:
    src = SCRIPT.read_text(errors="ignore")
    markers = [
        'TRANSFER_DIR="${ROOT_DIR}/calculator_files"',
        'TRANSFER_G3A="${TRANSFER_DIR}/CasioCAS.g3a"',
        'TRANSFER_PAK="${TRANSFER_DIR}/CASIOCAS.PAK"',
        'TEMPLATE_SRC="${ROOT_DIR}/c++/prizm/help/CASIOCAS.TPL"',
        'cp "${OUT_G3A}" "${TRANSFER_G3A}"',
        'python3 "${ROOT_DIR}/c++/tools/build_external_pack.py"',
        'python3 "${ROOT_DIR}/c++/tools/size_report.py" --baseline current',
        'python3 "${ROOT_DIR}/c++/tools/sync_calculator_volume.py" "${TRANSFER_DIR}"',
        'cp "${OUT_PAK}" "${TRANSFER_PAK}"',
        'rm -f "${ROOT_DIR}/CasioCAS.g3a"',
        'rm -f "${ROOT_DIR}/CASIOCAS.HLP"',
        'rm -f "${ROOT_DIR}/CASIOCAS.PAK"',
        'rm -rf "${TRANSFER_DIR}"',
    ]
    missing = [m for m in markers if m not in src]
    if missing:
        return fail("calculator transfer packaging missing: " + ", ".join(missing))
    forbidden = [
        'cp "${OUT_G3A}" "${ROOT_DIR}/CasioCAS.g3a"',
        'cp "${HELP_SRC}" "${ROOT_DIR}/CASIOCAS.HLP"',
        'cp "${HELP_SRC}" "${TRANSFER_HELP}"',
        'TRANSFER_HELP="${TRANSFER_DIR}/CASIOCAS.HLP"',
        'TRANSFER_HELP_GZ="${TRANSFER_DIR}/CASIOCAS.HLP.gz"',
        'ROOT_G3A=',
        'ROOT_HELP=',
    ]
    present = [m for m in forbidden if m in src]
    if present:
        return fail("root transfer output still present: " + ", ".join(present))
    if not PRUNE.exists():
        return fail("compiled-feature prune proof script missing")
    prune = PRUNE.read_text(errors="ignore")
    for marker in ["discover_makefile_objects", "--candidate", "run_tests_cpp.py", "restore_makefile"]:
        if marker not in prune:
            return fail("prune proof script marker missing: " + marker)
    for tool in [PACK, CHECK_PACK, SIZE_REPORT, FLAGS, SYNC]:
        if not tool.exists():
            return fail("missing size/external-data tool: " + tool.name)
    sync = SYNC.read_text(errors="ignore")
    for marker in ["CASIO_CALCULATOR_VOLUME", "CASIO_AUTO_SYNC", "CasioCAS.g3a", "CASIOCAS.PAK", "/Volumes"]:
        if marker not in sync:
            return fail("calculator sync marker missing: " + marker)
    flags = FLAGS.read_text(errors="ignore")
    for marker in ["-fwhole-program", "-Wl,-s", "restore_makefile", "run_tests_cpp.py"]:
        if marker not in flags:
            return fail("flag proof script marker missing: " + marker)
    catalog = (ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp").read_text(errors="ignore")
    for marker in ['CASCAS_HELP_FILE="\\\\\\\\fls0\\\\CASIOCAS.PAK"', "CCP1", "Copy CASIOCAS.PAK"]:
        if marker not in catalog:
            return fail("catalog PAK marker missing: " + marker)
    print("OK build packaging")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
