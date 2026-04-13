#!/usr/bin/env python3
"""Quick standalone test runner - generates and runs tests without TUI"""

import subprocess
import sys
import random
import re

PROGRAMS = {
    'algebra': ('src/Math/algebraProgram.py', [
        ('1\n(x+1)^2\nx^2+2x+1\n', 'expr'),
        ('6\nx^2-4=0\n', 'x ='),
    ]),
    'trig': ('src/Math/trigProgram.py', [
        ('1\nsin(x)^2+cos(x)^2=1\n1\n', 'lhs'),
        ('1\nsin(2*x)=2*sin(x)*cos(x)\n1\n', 'lhs'),
    ]),
    'derive': ('src/Math/deriveProgram.py', [
        ('1\nsin(x)\n', 'dy/dx'),
        ('1\nx^2\n', 'dy/dx'),
        ('1\nexp(x)\n', 'dy/dx'),
        ('1\nlog(x)\n', 'dy/dx'),
        ('1\nsin(2*x)\n', 'dy/dx'),
    ]),
    'integrate': ('src/Math/intProgram.py', [
        ('1\nx^2\n1\n', '+ C'),
        ('1\nsin(x)\n1\n', '+ C'),
        ('1\nexp(x)\n1\n', '+ C'),
        ('1\nx*sin(x)\n5\n', '+ C'),
        ('1\nx*exp(x)\n5\n', '+ C'),
    ]),
    # Skip SUVAT - requires complex non-CLI input
}

def run_test(prog_path, inp, must_contain):
    result = subprocess.run(
        [sys.executable, prog_path],
        input=inp,
        capture_output=True,
        text=True,
        timeout=30
    )
    return must_contain.lower() in result.stdout.lower(), result.stdout, result.stderr

def main():
    random.seed(42)
    total = 0
    passed = 0
    failed = 0
    
    print("CASIO Quick Test Suite")
    print("=" * 50)
    print()
    
    for prog_name, (prog_path, test_cases) in PROGRAMS.items():
        print(f"\n[{prog_name.upper()}]")
        for inp, must_contain in test_cases:
            total += 1
            ok, stdout, stderr = run_test(prog_path, inp, must_contain)
            if ok:
                passed += 1
                print(f"  ✓ {inp[:30].replace(chr(10), ' ')[:30]}...")
            else:
                failed += 1
                print(f"  ✗ {inp[:30].replace(chr(10), ' ')[:30]}...")
                print(f"      Expected: {must_contain}")
                # Show what we got
                lines = stdout.split('\n')[:5]
                for line in lines:
                    if line.strip():
                        print(f"      Got: {line[:60]}")
        
    print()
    print("=" * 50)
    print(f"Results: {passed}/{total} passed ({100*passed/total:.1f}%)")
    print(f"Failed: {failed}")
    
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main())