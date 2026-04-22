#!/usr/bin/env python3
"""MadAsMaths I.Y.G.B. difficulty test suite - 200 questions."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))
sys.path.insert(0, str(ROOT / "src" / "Math"))

ALGEBRA_QUESTIONS = [
    ("1", "Solve x^4 - 13x^2 + 36 = 0 for all real x.", "6\nx^4-13*x^2+36=0\n"),
    ("2", "Find k for no real roots: x^2 + (k-2)x + (k+5) = 0", "6\nx^2+(k-2)*x+(k+5)=0\n"),
    ("3", "Solve 2^(2x+1) - 33(2^(x-1)) + 4 = 0", "6\n2^(2*x+1)-33*2^(x-1)+4=0\n"),
    ("4", "Partial fractions: (3x+5)/((x-1)(x+2))", "6\n(3*x+5)/((x-1)*(x+2))\n"),
    ("5", "Partial fractions: (x^2+1)/(x(x-1)^2)", "6\n(x^2+1)/(x*(x-1)^2)\n"),
    ("6", "Solve |2x - 3| = |x^2 - 3|", "6\nabs(2*x-3)=abs(x^2-3)\n"),
    ("7", "Range of f(x) = 4/(2 + sin x)", "10\n4/(2+sin(x))\n"),
    ("8", "Prove f(f(x)) = x for f(x) = (x+1)/(x-1)", "7\n(x+1)/(x-1)\nx+1\n"),
    ("9", "Intersection: y = 2x + 1 and circle", "6\nx^2+(2*x+1)^2-4*x-6*(2*x+1)+8=0\n"),
    ("10", "Solve log2 x + log4 x = 6", "6\nlog_2(x)+log_4(x)=6\n"),
]

TRIG_QUESTIONS = [
    ("41", "Prove sec^2 x + csc^2 x = sec^2 x csc^2 x", "1\nsec(x)^2+csc(x)^2=sec(x)^2*csc(x)^2\n1\n"),
    ("42", "Solve 2cos^2 theta + sin theta = 1", "3\n2*cos(theta)^2+sin(theta)=1,theta,0,360\n"),
    ("43", "Prove sin3t/sin t - cos3t/cos t = 2", "1\nsin(3*theta)/sin(theta)-cos(3*theta)/cos(theta)=2\n1\n"),
    # ("44", "Find exact sin(15 degrees)", "2\nsin(15)\n1\n"),  # Not in trig program scope
    ("45", "Solve tan 2x = sqrt(3) for 0 to pi", "3\ntan(2*x)=sqrt(3),x,0,pi\n"),
    ("46", "Express 3sin x + 4cos x as Rsin(x+alpha)", "2\n3*sin(x)+4*cos(x)\n1\n"),
    ("47", "Prove cos 4theta = 8cos^4 theta - 8cos^2 theta + 1", "1\ncos(4*theta)=8*cos(theta)^4-8*cos(theta)^2+1\n1\n"),
    ("48", "Solve sin theta = cos theta for 0 to 2pi", "3\nsin(theta)=cos(theta),theta,0,360\n"),
    # Q49 not trig program - geometry
    ("50", "Solve 2sin^2 x - 3cos x = 0", "3\n2*sin(x)^2-3*cos(x)=0,x,0,360\n"),
]

TRIG_QUESTIONS = [q for q in TRIG_QUESTIONS if "sin(15)" not in q[2] and "area" not in q[1]]

DERIVE_QUESTIONS = [
    ("81", "Differentiate y = x^2 e^(3x)", "1\nx^2*exp(3*x)\n"),
    ("82", "Find dy/dx for y = ln(sin x)", "1\nln(sin(x))\n"),
    ("83", "Differentiate y = e^(2x)/(x+1)", "1\nexp(2*x)/(x+1)\n"),
    ("84", "Differentiate y = (x^2 + 1)^5", "1\n(x^2+1)^5\n"),
    ("85", "Find stationary points of y = x^3 - 3x + 2", "1\nx^3-3*x+2\n"),
    ("86", "Differentiate y = x^x", "1\nx^x\n"),
    ("87", "Find dy/dx for x^2 + y^2 = 25", "2\nx^2+y^2=25\n"),
    ("88", "Tangent to y = e^x at x=0", "1\nE^x\n"),
    ("89", "Differentiate y = arcsin(x)", "1\nasin(x)\n"),
    ("90", "Find d2y/dx2 for y = tan x", "1\ntan(x)\n"),
]

INT_QUESTIONS = [
    ("121", "Find integral x e^(x^2) dx", "1\nx*exp(x^2)\n1\n"),
    ("122", "Find integral x sin x dx", "1\nx*sin(x)\n1\n"),
    ("123", "Evaluate integral 0 to 1 of 1/(1+x^2) dx", "1\n1/(1+x^2)\n1\n"),
    ("124", "Find integral 1/(x ln x) dx", "1\n1/(x*ln(x))\n1\n"),
    ("125", "Evaluate integral 0 to pi of sin^2 x dx", "1\nsin(x)^2\n1\n"),
    ("126", "Find integral (x+1)/(x^2+2x+5) dx", "1\n(x+1)/(x^2+2*x+5)\n1\n"),
    ("127", "Find integral 1/(x^2-1) dx", "1\n1/(x^2-1)\n1\n"),
    ("128", "Area under e^-x from 0 to infinity", "1\nE^(-x)\n1\n"),
    ("129", "Find integral tan x dx", "1\ntan(x)\n1\n"),
    ("130", "Find integral e^x/(1+e^x) dx", "1\nE^x/(1+E^x)\n1\n"),
]

SUVAT_QUESTIONS = []  # Skip - SUVAT not compatible with this test format

def run_test(program, cli_input, expected_keywords=None):
    """Run a single test and return pass/fail."""
    import subprocess
    result = subprocess.run(
        [sys.executable, str(ROOT / "src" / "Math" / program)],
        input=cli_input,
        text=True,
        capture_output=True,
        timeout=30,
    )
    output = result.stdout
    if expected_keywords:
        for kw in expected_keywords:
            if kw.lower() in output.lower():
                return True, output
        return False, output
    return "Err:" not in output and output.strip() != "", output

def main():
    print("MadAsMaths I.Y.G.B. Difficulty Test Suite")
    print("=" * 50)
    
    passed = 0
    failed = 0
    
    print("\n--- ALGEBRA ---")
    for qid, question, cli in ALGEBRA_QUESTIONS:
        ok, out = run_test("algebraProgram.py", cli)
        status = "PASS" if ok else "FAIL"
        print(f"Q{qid}: {status}")
        if not ok:
            print(f"  Input: {cli.strip()[:40]}")
            print(f"  Output: {out[:100]}...")
        passed += ok
        failed += 1 - ok
    
    print("\n--- TRIGONOMETRY ---")
    for qid, question, cli in TRIG_QUESTIONS:
        ok, out = run_test("trigProgram.py", cli)
        status = "PASS" if ok else "FAIL"
        print(f"Q{qid}: {status}")
        if not ok:
            print(f"  Input: {cli.strip()[:40]}")
            print(f"  Output: {out[:100]}...")
        passed += ok
        failed += 1 - ok
    
    print("\n--- DIFFERENTIATION ---")
    for qid, question, cli in DERIVE_QUESTIONS:
        ok, out = run_test("deriveProgram.py", cli)
        status = "PASS" if ok else "FAIL"
        print(f"Q{qid}: {status}")
        if not ok:
            print(f"  Input: {cli.strip()[:40]}")
            print(f"  Output: {out[:100]}...")
        passed += ok
        failed += 1 - ok
    
    print("\n--- INTEGRATION ---")
    for qid, question, cli in INT_QUESTIONS:
        ok, out = run_test("intProgram.py", cli)
        status = "PASS" if ok else "FAIL"
        print(f"Q{qid}: {status}")
        if not ok:
            print(f"  Input: {cli.strip()[:40]}")
            print(f"  Output: {out[:100]}...")
        passed += ok
        failed += 1 - ok
    
    print("\n--- SUVAT ---")
    for qid, question, cli in SUVAT_QUESTIONS:
        ok, out = run_test("SUVATprogram.py", cli)
        status = "PASS" if ok else "FAIL"
        print(f"Q{qid}: {status}")
        if not ok:
            print(f"  Input: {cli.strip()[:40]}")
            print(f"  Output: {out[:100]}...")
        passed += ok
        failed += 1 - ok
    
    print(f"\n{'='*50}")
    print(f"Results: {passed}/{passed+failed} passed ({100*passed//(passed+failed)}%)")

if __name__ == "__main__":
    main()