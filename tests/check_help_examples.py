#!/usr/bin/env python3
from __future__ import annotations

import re
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HELP = ROOT / "help/functions"
RUNNER = ROOT / "tools/host/khicas_host_runner"


def examples() -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    for path in sorted(HELP.glob("*.txt")):
        match = re.search(r"^Examples:\s*(.+)$", path.read_text(errors="ignore"), re.M)
        if not match:
            continue
        for expr in match.group(1).split(";"):
            expr = expr.strip()
            if expr:
                out.append((path.stem, expr))
    return out


def main() -> int:
    bad: list[tuple[str, str, str]] = []
    env = dict(os.environ, CASCAS_HOST_PRODUCTION="1")
    for name, expr in examples():
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True, env=env)
        text = (proc.stdout or "") + (proc.stderr or "")
        if "Err: unsupported" in text or "Check: normal" in text or proc.returncode:
            bad.append((name, expr, text.splitlines()[0] if text.splitlines() else ""))
    if bad:
        for name, expr, first in bad[:80]:
            print(f"FAIL {name}: {expr} -> {first}")
        return 1
    print(f"OK help examples={len(examples())}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
