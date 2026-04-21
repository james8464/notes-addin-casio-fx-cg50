#!/usr/bin/env python3
"""Manual test runner for external exam questions."""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "src" / "Math"
PY = sys.executable

def run_algebra(eq):
    result = subprocess.run(
        [PY, str(ROOT / "algebraProgram.py")],
        input=f"6\n{eq}\n",
        text=True,
        capture_output=True,
        timeout=30,
    )
    return result.stdout

def run_trig(eq, degrees=True):
    mode = "d" if degrees else "r"
    result = subprocess.run(
        [PY, str(ROOT / "trigProgram.py")],
        input=f"2\n{eq}\n{mode}\n",
        text=True,
        capture_output=True,
        timeout=30,
    )
    return result.stdout

def run_derive(expr):
    result = subprocess.run(
        [PY, str(ROOT / "deriveProgram.py")],
        input=f"1\n{expr}\n",
        text=True,
        capture_output=True,
        timeout=30,
    )
    return result.stdout

def run_integrate(expr):
    result = subprocess.run(
        [PY, str(ROOT / "intProgram.py")],
        input=f"1\n{expr}\n\n",
        text=True,
        capture_output=True,
        timeout=30,
    )
    return result.stdout

def run_suvat(s, u, v, a, t):
    inputs = [s or "", u or "", v or "", a or "", t or ""]
    result = subprocess.run(
        [PY, str(ROOT / "SUVATprogram.py")],
        input="\n".join(inputs) + "\n",
        text=True,
        capture_output=True,
        timeout=30,
    )
    return result.stdout

QUESTIONS = [
    ("ALGEBRA", "Solve x^4 - 13*x^2 + 36 = 0", "6\nx^4 - 13*x^2 + 36\n", run_algebra),
    ("ALGEBRA", "Solve x^2 - 5*x + 6 = 0", "6\nx^2 - 5*x + 6\n", run_algebra),
    ("ALGEBRA", "Complete square: 2*x^2 + 8*x + 5", "5\n2*x^2 + 8*x + 5\n", run_algebra),
    ("TRIG", "Solve sin(x) = 0.5 for 0-360", "2\nsin(x)\n1\n0.5\nd\n0\n360\n", run_trig),
    ("TRIG", "Prove sec^2(x) - tan^2(x) = 1", "1\nsec(x)^2 - tan(x)^2\n1\n", run_trig),
    ("DERIVE", "Differentiate x^2*exp(x)", "1\nx^2*exp(x)\n", run_derive),
    ("DERIVE", "Differentiate (2*x + 1)^5", "1\n(2*x+1)^5\n", run_derive),
    ("INTEGRATE", "Integrate x*exp(x)", "1\nx*exp(x)\n\n", run_integrate),
    ("INTEGRATE", "Integrate sin(x)^2", "1\nsin(x)^2\n\n", run_integrate),
    ("SUVAT", "Find t: u=0, v=30, a=6, find t", "\n0\n30\n6\n\n", run_suvat),
]

if __name__ == "__main__":
    for i, (topic, desc, inp, func) in enumerate(QUESTIONS, 1):
        print(f"\n{'='*60}")
        print(f"Q{i}: {topic} - {desc}")
        print(f"{'='*60}")
        try:
            out = func(inp) if topic != "SUVAT" else func("", "0", "30", "6", "")
            print(out)
        except Exception as e:
            print(f"ERROR: {e}")