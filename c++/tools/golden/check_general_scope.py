#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


def run_host(*args: str) -> str:
    proc = subprocess.run([str(HOST), *args], cwd=str(REPO), text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if proc.returncode != 0:
        raise AssertionError(proc.stderr or proc.stdout)
    return proc.stdout


def require(name: str, out: str, needles: tuple[str, ...], forbidden: tuple[str, ...] = ()) -> None:
    low = out.lower()
    compact_low = "".join(low.split())
    missing = [n for n in needles if n.lower() not in low and "".join(n.lower().split()) not in compact_low]
    bad = [n for n in forbidden if n.lower() in low]
    if missing or bad:
        raise AssertionError(f"{name}: missing={missing} forbidden={bad}\n{out}")


def main() -> int:
    require(
        "hyperbolic_removed",
        run_host("--derive", "sinh(x^2)+atanh(x/3),x,method=chain"),
        ("Err: unsupported function",),
        ("dy/dx =", "atan(h)"),
    )
    require(
        "non_elementary_integral",
        run_host("--int", "exp(-x^2),method=auto"),
        ("No elementary primitive found",),
        ("A-level primitive",),
    )
    require(
        "general_nested_hyperbolic_removed",
        run_host("--alg", "exp(log(abs(x)+2))+sinh(asinh(x))"),
        ("Err: unsupported function",),
        ("simplify*", "atan(h)", "abs(x) + 2"),
    )
    print("general_scope OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
