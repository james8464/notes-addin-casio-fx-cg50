#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path
from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"


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
    ("x^2*e^x", ["DI table integration by parts", "D column: x^2, 2x, 2, 0", "e^x*(x^2 - 2*x + 2) + C"]),
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
    ("defint(sin(2*x)/(1+cos(x)),x,0,pi/2)", ["sin(2*x) = 2sin(x)cos(x)", "w = 1 + cos(x)", "2 - 2*log(2)"]),
    ("e^(2*x)*cos(3*x)", ["looping integration by parts", "a=2, b=3", "e^(2*x)*(2*cos(3*x)+3*sin(3*x))/13 + C"]),
    ("defint(sin(x)^n/(sin(x)^n+cos(x)^n),x,0,pi/2)", ["King property symmetry", "2I = Integral_0^(pi/2) 1 dx = pi/2", "pi/4"]),
    ("defint(sin(x)^5/(sin(x)^5+cos(x)^5),x,0,pi/2)", ["King property symmetry", "2I = Integral_0^(pi/2) 1 dx = pi/2", "pi/4"]),
    ("1/(2+cos(x))", ["Weierstrass substitution", "Integral [2/(t^2 + 3)]", "2/sqrt(3)*atan(tan(x/2)/sqrt(3)) + C"]),
    ("defint(log(sin(x)),x,0,pi/2)", ["log-trig symmetry", "2I = I - (pi/2)log(2)", "-pi*log(2)/2"]),
    ("1/(x^4+1)", ["Sophie Germain partial fractions", "x^4+1=(x^2+sqrt(2)*x+1)(x^2-sqrt(2)*x+1)", "atan(x*sqrt(2)+1)+atan(x*sqrt(2)-1)"]),
    ("(x^2+1)/(x^4+3*x^2+1)", ["algebraic symmetry substitution", "u=x-1/x", "atan((x-1/x)/sqrt(5))/sqrt(5) + C"]),
    ("(x^2-1)/(x^4+5*x^2+1)", ["algebraic symmetry substitution", "u=x+1/x", "atan((x+1/x)/sqrt(3))/sqrt(3) + C"]),
    ("sqrt(1-x^2)", ["reference triangle trig substitution", "opposite=x, hypotenuse=1", "(x*sqrt(1-x^2)+asin(x))/2 + C"]),
    ("sqrt(x^2+1)", ["reference triangle trig substitution", "opposite=x, adjacent=1", "(x*sqrt(x^2+1) + log(abs(x+sqrt(x^2+1))))/2 + C"]),
    ("sqrt(x^2-1)", ["reference triangle trig substitution", "hypotenuse=x, adjacent=1", "(x*sqrt(x^2-1)-log(abs(x+sqrt(x^2-1))))/2 + C"]),
    ("1/(x^3+1)", ["partial fractions with irreducible quadratic", "A/(x+1)+(Bx+C)/(x^2-x+1)", "log(abs(x+1))/3 - log(abs(x^2-x+1))/6"]),
    ("1/(x^4-1)", ["partial fractions with irreducible quadratic", "A/(x-1)+B/(x+1)+(Cx+D)/(x^2+1)", "log(abs((x-1)/(x+1)))/4 - atan(x)/2 + C"]),
    ("x^2/(x*sin(x)+cos(x))^2", ["parts with hidden derivative", "D' = x*cos(x)", "(sin(x)-x*cos(x))/(x*sin(x)+cos(x)) + C"]),
    ("defint(log(x)/(1+x^2),x,0,inf)", ["reciprocal substitution symmetry", "Tail becomes Integral_0^1 -log(t)/(1+t^2) dt", "0"]),
    ("sqrt(1-sin(x))", ["trig half-angle rewrite", "sqrt(1-sin(x))=sqrt(2)*sin(pi/4-x/2)", "2*sqrt(2)*cos(pi/4-x/2) + C"]),
    ("e^x*(1/x-1/x^2)", ["function plus derivative trick", "f'(x)=-1/x^2", "e^x/x + C"]),
    ("1/(x*sqrt(1+x^n))", ["fractional substitution", "1/(u^2-1)=1/2*(1/(u-1)-1/(u+1))", "log(abs((sqrt(1+x^n)-1)/(sqrt(1+x^n)+1)))/n + C"]),
    ("1/(x*sqrt(1+x^5))", ["fractional substitution", "5*x^4 dx = 2u du", "log(abs((sqrt(1+x^5)-1)/(sqrt(1+x^5)+1)))/5 + C"]),
]


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")

    bad = 0
    for expr, needles in CASES:
        proc = subprocess.run([str(HOST), "--int", expr], cwd=str(REPO), text=True, capture_output=True, timeout=12)
        out = proc.stdout + proc.stderr
        ok = proc.returncode == 0 and "not recognised" not in out.lower()
        # First marker is the old prose route label; current output is intentionally math-first.
        ok = ok and markers_present(out, needles[1:])
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
