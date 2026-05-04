#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "c++/tools/build_addin_prizm_docker.sh"


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def main() -> int:
    src = SCRIPT.read_text(errors="ignore")
    markers = [
        'TRANSFER_DIR="${ROOT_DIR}/calculator_files"',
        'TRANSFER_G3A="${TRANSFER_DIR}/CasioCAS.g3a"',
        'TRANSFER_HELP="${TRANSFER_DIR}/CASIOCAS.HLP"',
        'cp "${OUT_G3A}" "${TRANSFER_G3A}"',
        'cp "${HELP_SRC}" "${TRANSFER_HELP}"',
        'rm -f "${ROOT_DIR}/CasioCAS.g3a"',
        'rm -f "${ROOT_DIR}/CASIOCAS.HLP"',
        'rm -rf "${TRANSFER_DIR}"',
    ]
    missing = [m for m in markers if m not in src]
    if missing:
        return fail("calculator transfer packaging missing: " + ", ".join(missing))
    forbidden = [
        'cp "${OUT_G3A}" "${ROOT_DIR}/CasioCAS.g3a"',
        'cp "${HELP_SRC}" "${ROOT_DIR}/CASIOCAS.HLP"',
        'ROOT_G3A=',
        'ROOT_HELP=',
    ]
    present = [m for m in forbidden if m in src]
    if present:
        return fail("root transfer output still present: " + ", ".join(present))
    print("OK build packaging")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
