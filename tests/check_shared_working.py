#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    ("diff((x^2)*tan(y)=9,x)", "(dy)/(dx)=(-18x)/(x^4+81)"),
    ("diff(sin(x),x)", "Differentiate: sin(x)"),
    ("int(2*x+3,x)", "Integrate term by term"),
    ("integrate(2*x+3,x)", "Integrate term by term"),
    ("range(x^2+4*x+7)", "y >= 3"),
    ("range((x^2-1)/(x^2+1))", "-1 <= y < 1"),
    ("range(abs(x-3)+2)", "y >= 2"),
    ("range(sqrt(x-1)+3)", "y >= 3"),
    ("xform((x+1)^2,x^2+2*x+1)", "normal(((x+1)^2)-(x^2+2*x+1)) = 0"),
    ("xform(sin(x)^2+cos(x)^2,1)", "sin(x)^2+cos(x)^2 = 1"),
    ("xform(log(2,x),ln(x)/ln(2))", "change of base"),
    ("log(2,8)", "Answer: 3"),
    ("suvat(3,2,5)", "s = u*t + 1/2*a*t^2"),
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
