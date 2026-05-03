#!/usr/bin/env python3
"""Compare C++ host parse/format output against frozen expected fixtures."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


def run_host(host_bin: Path, expr: str) -> tuple[str, str]:
    p = subprocess.run([str(host_bin), expr], capture_output=True, text=True)
    out = p.stdout.splitlines()
    norm = ""
    fmt = ""
    for ln in out:
        if ln.startswith("NORM: "):
            norm = ln[6:]
        elif ln.startswith("FMT: "):
            fmt = ln[5:]
    if p.returncode != 0 and not fmt:
        raise RuntimeError("host failed: " + "\n".join(out))
    return norm, fmt


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    ap.add_argument("--file", default=str(REPO / "c++" / "tools" / "golden" / "expr_smoke.txt"))
    args = ap.parse_args()

    host_bin = Path(args.host)
    if not host_bin.exists():
        raise SystemExit(f"Host binary not found: {host_bin}\nBuild it with cmake in addin/host.")

    cases: list[tuple[str, str]] = []
    for ln in Path(args.file).read_text(encoding="utf-8", errors="replace").splitlines():
        s = ln.strip()
        if not s or s.startswith("#"):
            continue
        if "\t" not in s:
            raise SystemExit(f"Fixture needs TAB expected output: {s}")
        expr, expected = s.split("\t", 1)
        cases.append((expr.strip(), expected.strip()))

    bad = 0
    for e, expected_fmt in cases:
        c_norm, c_fmt = run_host(host_bin, e)
        ok = (expected_fmt == c_fmt)
        if not ok:
            bad += 1
            print("MISMATCH:", e)
            print("  C  NORM:", c_norm)
            print("  WANT  :", expected_fmt)
            print("  C  FMT :", c_fmt)
            print("")
        else:
            print("OK:", e, "->", c_fmt)

    print(f"Done. mismatches={bad}/{len(cases)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
