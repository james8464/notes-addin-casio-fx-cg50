#!/usr/bin/env python3
from __future__ import annotations

import subprocess

from check_shared_working import (
    CASES,
    ROOT,
    RUNNER,
    classification_reason,
    marker_present,
)


def main() -> int:
    bad: list[str] = []
    clean = 0
    classified = 0
    for expr, marker in CASES:
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        if proc.returncode == 0 and marker_present(marker, out, False):
            clean += 1
            continue
        if proc.returncode == 0 and classification_reason(expr, out):
            classified += 1
            continue
        bad.append(f"rc={proc.returncode} missing={marker!r} input={expr}")
    if bad:
        print("FAIL shared core without golden")
        print("\n".join(bad[:50]))
        return 1
    print(f"OK shared core without golden cases={len(CASES)} clean={clean} classified={classified}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
