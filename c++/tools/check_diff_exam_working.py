#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / "c++" / "addin" / "host" / "build" / "casio_host"

CASES = [
    ("wrapped explicit var", ["--derive", "diff((2x+ln(x))^3,x)"]),
    ("wrapped default var", ["--derive", "diff((2x+ln(x))^3)"]),
    ("plain expression", ["--derive", "(2x+ln(x))^3,x"]),
]

FORBIDDEN = [
    "pick rule",
    "chain/prod",
    "simp",
    "Expected )",
    "d/dx(diff(",
    "limite(",
    "product rule on the product part",
    "Inside u, use chain rule",
]

REQUIRED = [
    "u = 2*x + log(x)",
    "du/dx =",
    "dy/dx = 3*u^2*du/dx",
    "3*(2*x + log(x))^2",
]


def run_case(name, args):
    proc = subprocess.run([str(HOST), *args], cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout
    bad = []
    if proc.returncode:
        bad.append(f"exit {proc.returncode}")
    low = out.lower()
    for marker in FORBIDDEN:
        if marker.lower() in low:
            bad.append(f"forbidden: {marker}")
    for marker in REQUIRED:
        if marker not in out:
            bad.append(f"missing: {marker}")
    if bad:
        print(f"FAIL {name}: {', '.join(bad)}")
        print(out)
        return False
    print(f"PASS {name}")
    return True


def main():
    if not HOST.exists():
        print(f"missing host: {HOST}", file=sys.stderr)
        return 2
    ok = all(run_case(name, args) for name, args in CASES)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
