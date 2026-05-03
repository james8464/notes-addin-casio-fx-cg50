#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


def compact(text: str) -> str:
    return (text or "").replace(" ", "").replace("\r", "")


CASES: list[tuple[str, list[str]]] = [
    ("e^x*cos(x)", ["looping integration by parts", "2I = e^x*(cos(x)+sin(x))", "e^x*(cos(x)+sin(x))/2 + C"]),
    ("sec(x)^3", ["parts plus tan^2 identity", "tan(x)^2 = sec(x)^2 - 1", "(sec(x)*tan(x) + log(abs(sec(x)+tan(x))))/2 + C"]),
    ("1/(1+sqrt(x))", ["reverse substitution u=sqrt(x)", "2 - 2/(1+u)", "2*sqrt(x) - 2*log(abs(1+sqrt(x))) + C"]),
    ("x^2*ln(x)", ["integration by parts", "du=1/x", "x^3*log(x)/3 - x^3/9 + C"]),
    ("e^x/(1+e^(2x))", ["substitution u=e^x", "Integral(1/(1+u^2))", "atan(e^x) + C"]),
    ("1/(x*ln(x))", ["logarithmic reverse chain", "Integral(1/u)", "log(abs(log(x))) + C"]),
    ("sin(x)^4", ["double-angle power reduction", "sin(x)^4 = 3/8 - cos(2x)/2 + cos(4x)/8", "3*x/8 - sin(2*x)/4 + sin(4*x)/32 + C"]),
    ("1/(x^2*(x+1))", ["partial fractions repeated linear", "A=-1", "-log(abs(x)) - 1/x + log(abs(x+1)) + C"]),
    ("(2*x+5)/(x^2+5*x-7)", ["reverse-chain log", "D' = 2x+5", "log(abs(x^2+5*x-7)) + C"]),
    ("1/sqrt(1-x^2)", ["trig substitution x=sin(t)", "Back-substitute t=asin(x)", "asin(x) + C"]),
    ("1/(x^2+4)", ["atan standard form", "x=2tan(t)", "atan(x/2)/2 + C"]),
    ("x^2*e^x", ["repeated integration by parts", "Parts again", "e^x*(x^2 - 2*x + 2) + C"]),
    ("tan(x)^3", ["tan identity", "tan(x)(sec(x)^2 - 1)", "tan(x)^2/2 - log(abs(sec(x))) + C"]),
    ("x*sqrt(x-1)", ["substitution u=x-1", "u^(3/2)+u^(1/2)", "2*(x-1)^(5/2)/5 + 2*(x-1)^(3/2)/3 + C"]),
    ("(x^2+1)/(x-1)", ["algebraic division", "x + 1 + 2/(x-1)", "x^2/2 + x + 2*log(abs(x-1)) + C"]),
    ("cos(x)^3", ["odd cosine power", "u=sin(x)", "sin(x) - sin(x)^3/3 + C"]),
    ("ln(x)", ["parts with hidden 1", "dv=dx", "x*log(x) - x + C"]),
    ("1/(x*(x^2+1))", ["partial fractions with irreducible quadratic", "A=1", "log(abs(x)) - log(abs(x^2+1))/2 + C"]),
    ("1/(x^2+4*x+13)", ["complete square then atan", "(x+2)^2 + 9", "atan((x+2)/3)/3 + C"]),
    ("e^(sqrt(x))", ["substitution then parts", "dx=2u du", "2*e^(sqrt(x))*(sqrt(x)-1) + C"]),
    ("1/(1+cos(x))", ["trig conjugate", "1-cos(x)^2 = sin(x)^2", "-cot(x) + cosec(x) + C"]),
    ("sin(ln(x))", ["log substitution then looping parts", "2I=e^u*(sin(u)-cos(u))", "x*(sin(log(x))-cos(log(x)))/2 + C"]),
    ("1/(x*sqrt(x^2-1))", ["sec substitution", "x=sec(t)", "acos(1/x) + C"]),
]


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")

    bad = 0
    for expr, needles in CASES:
        proc = subprocess.run([str(HOST), "--int", expr], cwd=str(REPO), text=True, capture_output=True, timeout=12)
        out = proc.stdout + proc.stderr
        flat = compact(out)
        ok = proc.returncode == 0 and "not recognised" not in out.lower()
        ok = ok and all(compact(n) in flat for n in needles)
        if ok:
            print("OK", expr)
            continue
        bad += 1
        print("FAIL", expr)
        print(out)

    print("done mismatches", bad, "/", len(CASES))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
