#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import re

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "scope"))

from scope_manifest import REMOVED_COMMANDS

SRC = ROOT / "khicas/upstream/giac90_1addin"
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"


def help_leaks() -> list[str]:
    names = {p.stem for p in (ROOT / "help" / "functions").glob("*.txt")}
    return sorted(names.intersection(REMOVED_COMMANDS))


def catalog_leaks() -> list[str]:
    text = (SRC / "catalogen.cpp").read_text(errors="ignore")
    try:
        block = text.split("const catalogFunc completeCat[]", 1)[1].split("\n};", 1)[0]
    except IndexError:
        block = text
    return [
        name for name in REMOVED_COMMANDS
        if re.search(r'"' + re.escape(name) + r'(\(|"|,)', block)
    ]


def runtime_leaks() -> list[str]:
    leaked: list[str] = []
    samples = [
        "stats(1,2,3)",
        "stat([1,2,3])",
        "statistics([1,2,3])",
        "prob(0.5)",
        "probability(0.5)",
        "proba(0.5)",
        "normalcdf(0,1,0)",
        "matrix([[1,2],[3,4]])",
        "plot(sin(x),x)",
        "graph(sin(x))",
        "for i from 1 to 3 do i od",
        "program(test)",
        "turtle()",
        "crypto(123)",
        "options()",
        "proot()",
        "pcoeff()",
        "trigcos(x)",
        "trigsin(x)",
        "trigtan(x)",
        "ceiling(1.2)",
    ]
    for sample in samples:
        out = subprocess.run(
            [str(RUNNER), "--alg", sample],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=10,
        ).stdout
        if "unsupported" not in out:
            leaked.append(sample)
    return leaked


def main() -> int:
    leaks = {
        "help": help_leaks(),
        "catalog": catalog_leaks(),
        "runtime": runtime_leaks(),
    }
    bad = [f"{k}: {', '.join(v[:20])}" for k, v in leaks.items() if v]
    if bad:
        raise SystemExit("FAIL removed feature leaks: " + " | ".join(bad))
    print(f"OK removed features blocked={len(REMOVED_COMMANDS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
