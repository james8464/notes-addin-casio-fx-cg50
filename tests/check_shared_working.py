#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    ("diff((x^2)*tan(y)=9,x)", "(dy)/(dx)=(-18x)/(x^4+81)"),
    ("range(x^2+4*x+7)", "y >= 3"),
    ("xform((x+1)^2,x^2+2*x+1)", "normal(((x+1)^2)-(x^2+2*x+1)) = 0"),
    ("log(2,8)", "Answer: 3"),
]


def main() -> int:
    bad = []
    for expr, marker in CASES:
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        if proc.returncode != 0 or marker not in out:
            bad.append((expr, marker, proc.returncode, out[:500]))
    if bad:
        for expr, marker, code, out in bad:
            print(f"FAIL {expr!r} code={code} missing={marker!r}\n{out}")
        return 1
    print(f"OK shared working cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
