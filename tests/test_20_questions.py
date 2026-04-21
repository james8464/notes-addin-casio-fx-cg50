#!/usr/bin/env python3
"""Manual test runner for the 20 exam questions."""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "src" / "Math"
PY = sys.executable

def run_program(prog, inp):
    result = subprocess.run(
        [PY, str(ROOT / prog)],
        input=inp,
        text=True,
        capture_output=True,
        timeout=30,
    )
    return result.stdout

print("="*60)
print("Q1: ALGEBRA - Hidden Quadratic: 2x + sqrt(x) - 15 = 0")
print("="*60)
print(run_program("algebraProgram.py", "6\n2*x + sqrt(x) - 15\n"))

print("="*60)
print("Q2: ALGEBRA - Circle-Line Intersection")
print("y = 3x - 10, x^2 + y^2 - 4x - 2y - 20 = 0")
print("="*60)
print(run_program("algebraProgram.py", "6\ny = 3*x - 10\nx^2 + y^2 - 4*x - 2*y - 20 = 0\n"))

print("="*60)
print("Q3: ALGEBRA - Discriminant (find k)")
print("kx^2 + 4x + (k-3) = 0 has real roots")
print("="*60)
print(run_program("algebraProgram.py", "6\nk*x^2 + 4*x + (k-3)\n"))

print("="*60)
print("Q4: ALGEBRA - Completing Square")
print("11 - 8x - 2x^2")
print("="*60)
print(run_program("algebraProgram.py", "5\n11 - 8*x - 2*x^2\n"))

print("="*60)
print("Q5: TRIG - Prove Identity")
print("(1+sinθ)/cosθ + cosθ/(1+sinθ) = 2secθ")
print("="*60)
print(run_program("trigProgram.py", "1\n(1+sin(x))/cos(x) + cos(x)/(1+sin(x))\n1\n"))

print("="*60)
print("Q6: TRIG - Solve 3cos²θ + 8sinθ = 0")
print("0° ≤ θ ≤ 360°")
print("="*60)
print(run_program("trigProgram.py", "2\n3*cos(x)^2 + 8*sin(x)\n1\nd\n0\n360\n"))

print("="*60)
print("Q7: TRIG - Solve sin(3x - π/6) = 1/2")
print("0 ≤ x ≤ π")
print("="*60)
print(run_program("trigProgram.py", "2\nsin(3*x - pi/6)\n1\n0.5\nr\n0\n3.14159\n"))

print("="*60)
print("Q8: TRIG - Solve 2tan²x + 3secx = 0")
print("0 ≤ x ≤ 2π")
print("="*60)
print(run_program("trigProgram.py", "2\n2*tan(x)^2 + 3*sec(x)\n1\nr\n0\n6.283\n"))

print("="*60)
print("Q9: DERIVE - Product Rule: x³ln(4x)")
print("="*60)
print(run_program("deriveProgram.py", "1\nx^3*ln(4*x)\n"))

print("="*60)
print("Q10: DERIVE - Chain Rule: √(1+cos²x)")
print("="*60)
print(run_program("deriveProgram.py", "1\nsqrt(1+cos(x)^2)\n"))

print("="*60)
print("Q11: DERIVE - Quotient: e^(2x)/sin(3x)")
print("="*60)
print(run_program("deriveProgram.py", "1\ne^(2*x)/sin(3*x)\n"))

print("="*60)
print("Q12: DERIVE - Implicit: 2x² - 3xy + y³ = 7 at (2,1)")
print("="*60)
print(run_program("deriveProgram.py", "2\n2*x^2 - 3*x*y + y^3 = 7\n"))

print("="*60)
print("Q13: DERIVE - Parametric: x=ln(t+2), y=1/(t+1)")
print("="*60)
print(run_program("deriveProgram.py", "3\nln(t+2)\n1/(t+1)\n"))

print("="*60)
print("Q14: INTEGRATE - By Parts: x²ln(x)")
print("="*60)
print(run_program("intProgram.py", "1\nx^2*log(x)\n\n"))

print("="*60)
print("Q15: INTEGRATE - By Parts twice: e^(2x)sin(x)")
print("="*60)
print(run_program("intProgram.py", "1\ne^(2*x)*sin(x)\n\n"))

print("="*60)
print("Q16: INTEGRATE - Partial Fractions")
print("(2x-1)/((x-1)*(2x-3))")
print("="*60)
print(run_program("intProgram.py", "1\n(2*x-1)/((x-1)*(2*x-3))\n\n"))

print("="*60)
print("Q17: INTEGRATE - Trig Power: cos³x")
print("="*60)
print(run_program("intProgram.py", "1\ncos(x)^3\n\n"))

print("="*60)
print("Q18: INTEGRATE - Substitution: x/√(2x+1)")
print("="*60)
print(run_program("intProgram.py", "1\nx/sqrt(2*x+1)\n4\n2*x+1\n"))

print("="*60)
print("Q19: SUVAT - Projectile: u=28m/s at 30°")
print("Find time to max height")
print("="*60)
print(run_program("SUVATprogram.py", "\n28*sin(30/180*pi)\n\n\n-9.8\nt\n"))

print("="*60)
print("Q20: SUVAT - Find u: a=0.5, t=12, v=10")
print("="*60)
print(run_program("SUVATprogram.py", "\n\nu=10\n0.5\n12\n"))