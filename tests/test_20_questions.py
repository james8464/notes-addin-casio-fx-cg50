#!/usr/bin/env python3
"""Test 20 hard MadAsMaths questions against CASIO programs."""

import subprocess
import sys
from pathlib import Path

PY = Path(__file__).resolve().parents[1] / ".venv" / "bin" / "python3"
ROOT = Path(__file__).resolve().parents[1]

programs = {
    "algebra": "src/Math/algebraProgram.py",
    "trig": "src/Math/trigProgram.py",
    "derive": "src/Math/deriveProgram.py",
    "integrate": "src/Math/intProgram.py",
    "suvat": "src/Math/SUVATprogram.py",
}

questions = [
    ("Q1", "algebra", "6\nx^(2/3)+16/x^(2/3)=17\nx\n", "Hidden Quadratic"),
    ("Q2", "algebra", "6\n(x-2)^2+(y+3)^2=10\n11\ny=k*x-5\nx\n", "Circle-Line Tangency"),
    ("Q3", "algebra", "6\n(k+1)*x^2+2*k*x+(k-2)=0\nx\n", "Discriminant"),
    ("Q4", "algebra", "6\nx^3+x^2-11*x+1=0\nx\n", "Cubic Factorisation"),
    ("Q5", "trig", "1\nsin(3*theta)/sin(theta)-cos(3*theta)/cos(theta)=2\n4\n", "Trig Identity"),
    ("Q6", "trig", "3\ncos(2*theta)=3*sin(theta)+2,theta,0,2pi\n", "Trig Equation"),
    ("Q7", "trig", "3\n4*cos(2*x)^2-7*cos(2*x)-2=0,x,0,180\n", "Multiple Angles"),
    ("Q8", "trig", "1\natan(1/2)+atan(1/3)=pi/4\n4\n", "Inverse Trig"),
    ("Q9", "derive", "1\nx^2*exp(3*x)*sin(2*x)\n", "Triple Product"),
    ("Q10", "derive", "1\nlog(sec(3*x)+tan(3*x))\n", "Chain Rule Log"),
    ("Q11", "derive", "3\nx^2+3*x*y+y^2=11\ny=2\nx=1\n", "Implicit Tangent"),
    ("Q12", "derive", "2\ncos(t)\nsin(2*t)\nt=pi/4\n", "Parametric"),
    ("Q13", "derive", "1\nx^x\n", "Logarithmic Diff"),
    ("Q14", "integrate", "1\nx^2*sin(x)\n", "Repeated Parts"),
    ("Q15", "integrate", "1\n(5*x^2-2*x+11)/((x+1)*(x^2+4))\n", "Partial Fractions"),
    ("Q16", "integrate", "1\ncos(x)^4\n", "Trig Power"),
    ("Q17", "integrate", "1\n1/(1+sqrt(x))\n", "Substitution Surds"),
    ("Q18", "suvat", "8\n2\n6*t-4\n0\n0\n1\n", "Variable Accel"),
    ("Q19", "suvat", "5\n28\n3/4\n18\n", "Projectile"),
    ("Q20", "suvat", "7\nU\n60\n2\n0.6\n", "Advanced SUVAT"),
]

def run_test(qnum, prog, inp, label):
    script = programs.get(prog, "")
    if not script:
        return "SKIP", f"No program: {prog}"
    try:
        result = subprocess.run(
            [str(PY), str(ROOT / script)],
            input=inp,
            text=True,
            capture_output=True,
            timeout=10,
        )
        output = result.stdout
        if "err:" in output.lower() or "traceback" in output.lower():
            return "FAIL", output[:200]
        return "PASS", output[:300]
    except subprocess.TimeoutExpired:
        return "TIMEOUT", "Test timed out"
    except Exception as e:
        return "ERROR", str(e)[:100]

print("=" * 60)
print("Testing 20 Hard MadAsMaths Questions")
print("=" * 60)
results = []
for qnum, prog, inp, label in questions:
    status, output = run_test(qnum, prog, inp, label)
    results.append((qnum, label, status, output))
    print(f"\n{qnum}: {label}")
    print(f"   Status: {status}")
    if status != "PASS":
        print(f"   Output: {output[:150]}")

passed = sum(1 for r in results if r[2] == "PASS")
failed = sum(1 for r in results if r[2] == "FAIL")
skipped = sum(1 for r in results if r[2] == "SKIP")
errors = sum(1 for r in results if r[2] in ("ERROR", "TIMEOUT"))

print("\n" + "=" * 60)
print(f"Summary: {passed}/20 passed, {failed} failed, {errors} errors, {skipped} skipped")
print("=" * 60)