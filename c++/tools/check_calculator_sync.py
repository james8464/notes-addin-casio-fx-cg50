#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SYNC = ROOT / "c++/tools/sync_calculator_volume.py"


def fail(msg: str) -> int:
    print("FAIL " + msg)
    return 1


def write(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8")


def run(cmd: list[str], env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    merged = os.environ.copy()
    if env:
        merged.update(env)
    return subprocess.run(cmd, cwd=ROOT, env=merged, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        base = Path(td)
        transfer = base / "transfer"
        volume = base / "CASIO"
        transfer.mkdir()
        volume.mkdir()
        write(transfer / "CasioCAS.g3a", "new-g3a")
        write(transfer / "CASIOCAS.PAK", "new-pak")
        write(volume / "CasioCAS.g3a", "old-g3a")
        write(volume / "CASIOCAS.PAK", "old-pak")
        write(volume / "OTHER.TXT", "keep")

        res = run(["python3", str(SYNC), str(transfer), "--volume", str(volume)])
        if res.returncode != 0:
            return fail(res.stdout)
        if (volume / "CasioCAS.g3a").read_text() != "new-g3a":
            return fail("g3a not replaced")
        if (volume / "CASIOCAS.PAK").read_text() != "new-pak":
            return fail("pak not replaced")
        if (volume / "OTHER.TXT").read_text() != "keep":
            return fail("unrelated file changed")

        write(volume / "CasioCAS.g3a", "old-again")
        res = run(["python3", str(SYNC), str(transfer), "--volume", str(volume)], {"CASIO_AUTO_SYNC": "0"})
        if res.returncode != 0:
            return fail(res.stdout)
        if (volume / "CasioCAS.g3a").read_text() != "old-again":
            return fail("disabled sync still replaced file")

    print("OK calculator sync")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
