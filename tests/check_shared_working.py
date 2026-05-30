#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    ("diff((x^2)*tan(y)=9,x)", "(dy)/(dx)=(-18x)/(x^4+81)"),
    ("diff(sin(x),x)", "Differentiate: sin(x)"),
    ("diff(x^3,x)", "Power rule: d/dx x^n = n*x^(n-1)"),
    ("diff(x^2*sin(x),x)", "Product rule: d(uv)/dx = u*v' + v*u'"),
    ("diff(sin(3*x+1),x)", "Chain rule: u=3*x+1"),
    ("diff(sin(x)/x,x)", "Quotient rule: d(u/v)/dx=(v*u'-u*v')/v^2"),
    ("int(2*x+3,x)", "Integrate term by term"),
    ("int(sin(x)^2,x)", "sin(x)^2 = (1-cos(2*x))/2"),
    ("int(cos(x),x)", "int(cos(x)) dx = sin(x) + C"),
    ("int(sec(x)^2,x)", "int(sec(x)^2) dx = tan(x) + C"),
    ("int(1/x,x)", "int(1/x) dx = ln(abs(x)) + C"),
    ("integrate(2*x+3,x)", "Integrate term by term"),
    ("range(x^2+4*x+7)", "y >= 3"),
    ("range(x/(x^2+4),x)", "1 - 16*y^2 >= 0"),
    ("range(log(2*x+3),x)", "non-constant linear input covers all positive log arguments"),
    ("range((x^2-1)/(x^2+1))", "-1 <= y < 1"),
    ("range(abs(x-3)+2)", "y >= 2"),
    ("range(sqrt(x-1)+3)", "y >= 3"),
    ("diff(log(1/(sqrt(x^2+1)-x)),x)", "Rationalise: 1/(sqrt(x^2+1)-x) = sqrt(x^2+1)+x"),
    ("xform((x+1)^2,x^2+2*x+1)", "normal(((x+1)^2)-(x^2+2*x+1)) = 0"),
    ("xform((sin(x)-cos(x)+1)/(sin(x)+cos(x)-1),sec(x)+tan(x))", "t=tan(x/2)"),
    ("xform(sin(x)^2+cos(x)^2,1)", "sin(x)^2+cos(x)^2 = 1"),
    ("xform(log(2,x),ln(x)/ln(2))", "change of base"),
    ("xform(sec(x)^2,1+tan(x)^2)", "sec(x)^2 = 1 + tan(x)^2"),
    ("xform(cosec(x)^2,1+cot(x)^2)", "cosec(x)^2 = 1 + cot(x)^2"),
    ("xform(cot(x),cos(x)/sin(x))", "cot(x)=cos(x)/sin(x)"),
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
