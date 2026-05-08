#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
HOST = ROOT / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = Path(__file__).with_name("host_smoke_cases.txt")


def run_host(mode: str, expr: str) -> list[str]:
    flag = {"int": "--int", "alg": "--alg", "trig": "--trig", "derive": "--derive"}[mode]
    p = subprocess.run([str(HOST), flag, expr], text=True, capture_output=True)
    if p.returncode != 0:
        raise RuntimeError(f"host failed ({mode} {expr!r}):\n{p.stderr}\n{p.stdout}")
    return [ln.rstrip("\n") for ln in p.stdout.splitlines()]


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary at {HOST}. Build with ./c++/tools/build_host.sh")

    failures: list[str] = []
    for raw in CASES.read_text().splitlines():
        line = raw.strip("\n")
        if not line or line.startswith("#"):
            continue
        try:
            mode, expr, expected = line.split("\t", 2)
        except ValueError:
            raise SystemExit(f"Bad case line (need 3 tab-separated fields): {raw!r}")

        out = run_host(mode, expr)
        ok = False
        expected_line = expected.strip()
        if expected_line.lower().startswith("answer:"):
            expected_line = expected_line.split(":", 1)[1].strip()
        if mode == "alg":
            ok = any(ln.strip() == expected_line for ln in out)
        else:
            # Allow richer multi-line exam working; require the final answer line.
            ok = any(ln.strip() == expected_line for ln in out)

        if not ok:
            failures.append(
                f"{mode} {expr!r}\n  expected: {expected}\n  got:\n    "
                + "\n    ".join(out[:20])
            )

    if failures:
        print("\n\n".join(failures))
        return 1

    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
