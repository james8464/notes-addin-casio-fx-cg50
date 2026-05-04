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
        'rm -rf "${TRANSFER_DIR}"',
    ]
    missing = [m for m in markers if m not in src]
    if missing:
        return fail("calculator transfer packaging missing: " + ", ".join(missing))
    print("OK build packaging")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
