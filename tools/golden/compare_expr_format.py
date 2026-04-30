#!/usr/bin/env python3
"""
Compare Python (oracle) vs C++ host parse/format for expressions.

Python oracle:
  python/src/Math/casio_core.py :: parse_expr(), format_expr()

C++ host:
  addin/host (casio_host) prints lines:
    NORM: ...
    FMT: ...
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
PY_SRC = REPO / "python" / "src"
PY_MATH = PY_SRC / "Math"


def py_oracle(expr: str) -> tuple[str, str]:
    sys.path.insert(0, str(PY_SRC))
    sys.path.insert(0, str(PY_MATH))
    from Math import casio_core  # type: ignore

    node = casio_core.parse_expr(expr)
    return casio_core.normalize_text(expr), casio_core.format_expr(node)


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
    ap.add_argument("--host", default=str(REPO / "addin" / "host" / "build" / "casio_host"))
    ap.add_argument("--file", default=str(REPO / "tools" / "golden" / "expr_smoke.txt"))
    args = ap.parse_args()

    host_bin = Path(args.host)
    if not host_bin.exists():
        raise SystemExit(f"Host binary not found: {host_bin}\nBuild it with cmake in addin/host.")

    exprs = []
    for ln in Path(args.file).read_text(encoding="utf-8", errors="replace").splitlines():
        s = ln.strip()
        if not s or s.startswith("#"):
            continue
        exprs.append(s)

    bad = 0
    for e in exprs:
        py_norm, py_fmt = py_oracle(e)
        c_norm, c_fmt = run_host(host_bin, e)
        ok = (py_fmt == c_fmt)
        if not ok:
            bad += 1
            print("MISMATCH:", e)
            print("  PY NORM:", py_norm)
            print("  C  NORM:", c_norm)
            print("  PY FMT :", py_fmt)
            print("  C  FMT :", c_fmt)
            print("")
        else:
            print("OK:", e, "->", c_fmt)

    print(f"Done. mismatches={bad}/{len(exprs)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

