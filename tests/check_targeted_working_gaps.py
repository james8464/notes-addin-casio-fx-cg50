#!/usr/bin/env python3
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

CASES = [
    ("integrate(2*x+3,x)", ["Integrate term by term:", "Use int(a*x^n)", "Answer: x^2 + 3*x + C"]),
    ("integrate(9x)", ["Use int(a*x^n)", "int(9x) dx", "Answer: 9/2*x^2 + C"]),
    ("integrate(x*exp(x),x)", ["Use integration by parts", "Let u=x, dv=e^x dx", "du=dx, v=e^x"]),
    ("integrate(x*cos(x),x)", ["Use integration by parts", "Let u=x, dv=cos(x) dx", "du=dx, v=sin(x)"]),
    ("integrate((ln(x))^2)", ["Let u = ln(x)^2, dv = dx", "Let u = ln(x), dv = dx"]),
]


def main() -> int:
    bad = []
    for expr, markers in CASES:
        p = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (p.stdout or "") + (p.stderr or "")
        missing = [m for m in markers if m not in out]
        if p.returncode or missing:
            bad.append((expr, p.returncode, missing, out[:600]))
    for expr, code, missing, out in bad:
        print(f"FAIL {expr!r} code={code} missing={missing}\n{out}")
    if bad:
        return 1
    print(f"OK targeted working gaps={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
