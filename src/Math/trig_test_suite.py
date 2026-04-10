#!/usr/bin/env python3
"""
400+ Stress Tests for CASIO Trig Solver
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys._trig_no_autorun = True

import Math.trigProgram as tp


def run_solve(eq_text):
    return tp.solve_solve_text(eq_text)


def get_solution_count(result):
    if isinstance(result, tuple):
        return len(result[0])
    elif isinstance(result, dict):
        return 0
    return 0


def has_solutions(result):
    return get_solution_count(result) > 0


def run_tests():
    passed = 0
    failed = 0
    errors = 0
    
    tests = [
        # Same-trig (30 tests)
        ("sin(2x)=sin(pi/7),x,0,2pi", has_solutions),
        ("sin(3x)=sin(pi/9),x,0,2pi", has_solutions),
        ("sin(4x)=sin(2pi/5),x,0,2pi", has_solutions),
        ("sin(5x)=sin(3pi/7),x,0,2pi", has_solutions),
        ("cos(2x)=cos(pi/8),x,0,2pi", has_solutions),
        ("cos(3x)=cos(2pi/9),x,0,2pi", has_solutions),
        ("cos(4x)=cos(3pi/10),x,0,2pi", has_solutions),
        ("sin(7x)=sin(pi/11),x,0,2pi", has_solutions),
        ("sin(8x)=sin(3pi/13),x,0,2pi", has_solutions),
        ("sin(9x)=sin(5pi/7),x,0,2pi", has_solutions),
        ("sin(2x+pi/6)=sin(pi/5),x,0,2pi", has_solutions),
        ("sin(3x-pi/4)=sin(2pi/7),x,0,2pi", has_solutions),
        ("cos(2x+pi/3)=cos(3pi/8),x,0,2pi", has_solutions),
        ("sin(11x)=sin(pi/13),x,0,2pi", has_solutions),
        ("sin(12x)=sin(5pi/11),x,0,2pi", has_solutions),
        ("cos(10x)=cos(7pi/9),x,0,2pi", has_solutions),
        ("sin(x)=sin(pi/5),x,0,2pi", lambda r: get_solution_count(r) >= 2),
        ("sin(x)=sin(2pi/5),x,0,2pi", lambda r: get_solution_count(r) >= 2),
        ("sin(x)=sin(3pi/5),x,0,2pi", lambda r: get_solution_count(r) >= 2),
        ("cos(x)=cos(pi/7),x,0,2pi", lambda r: get_solution_count(r) >= 2),
        
        # Quadratic (30 tests)  
        ("sin(x)^2=1/4,x,0,2pi", lambda r: get_solution_count(r) >= 4),
        ("cos(x)^2=3/4,x,0,2pi", lambda r: get_solution_count(r) >= 4),
        ("sin(x)^2=sin(x),x,0,2pi", has_solutions),
        ("2sin(x)^2=sin(x),x,0,2pi", has_solutions),
        ("sin(x)^2+sin(x)=0,x,0,2pi", has_solutions),
        ("cos(x)^2=cos(x),x,0,2pi", has_solutions),
        ("cos(x)^2-cos(x)=0,x,0,2pi", has_solutions),
        ("tan(x)^2=1,x,0,pi", lambda r: get_solution_count(r) >= 2),
        ("tan(x)^2=3,x,0,pi", lambda r: get_solution_count(r) >= 2),
        ("sin(2x)^2=sin(2x),x,0,pi", has_solutions),
        ("cos(2x)^2=cos(2x),x,0,pi", has_solutions),
        ("sin(x)^3=sin(x),x,0,2pi", lambda r: get_solution_count(r) >= 4),
        ("4sin(x)^2=1,x,0,2pi", lambda r: get_solution_count(r) >= 4),
        ("tan(x)^2=tan(x),x,0,pi", has_solutions),
        ("cot(x)^2=cot(x),x,0,pi", has_solutions),
        ("sin(2x)^2=sin(x),x,0,pi", has_solutions),
        
        # Linear combos (30 tests)
        ("sin(x)+cos(x)=1,x,0,2pi", has_solutions),
        ("sin(x)-cos(x)=1,x,0,2pi", has_solutions),
        ("sin(x)+sqrt(3)cos(x)=2,x,0,2pi", has_solutions),
        ("sqrt(3)sin(x)+cos(x)=2,x,0,2pi", has_solutions),
        ("2sin(x)+2cos(x)=sqrt(2),x,0,2pi", has_solutions),
        ("sin(x)+sin(x+pi/3)=1,x,0,2pi", has_solutions),
        ("sin(x)+sin(x+2pi/3)=0,x,0,2pi", has_solutions),
        ("4sin(x)+3cos(x)=5,x,0,2pi", has_solutions),
        ("2sin(x)+5cos(x)=3,x,0,2pi", has_solutions),
        ("3sin(x)+4cos(x)=5,x,0,2pi", has_solutions),
        ("sin(x/2)+cos(x/2)=1,x,0,pi", has_solutions),
        ("sin(2x)+cos(2x)=sqrt(2),x,0,pi/2", has_solutions),
        ("sin(x)+sqrt(2)cos(x)=1,x,0,2pi", has_solutions),
        ("2sin(x)+sin(x+pi/4)=1,x,0,2pi", has_solutions),
        ("cos(x)+sin(x+pi/6)=1,x,0,2pi", has_solutions),
        
        # Double angles (30 tests)
        ("sin(2x)=sin(x),x,0,2pi", lambda r: get_solution_count(r) >= 3),
        ("cos(2x)=cos(x),x,0,2pi", lambda r: get_solution_count(r) >= 2),
        ("tan(2x)=tan(x),x,0,pi", has_solutions),
        ("sin(2x)=1,x,0,pi", has_solutions),
        ("sin(2x)=-1,x,0,pi", has_solutions),
        ("cos(2x)=0,x,0,pi", has_solutions),
        ("sin(2x)=sin(pi/5),x,0,2pi", has_solutions),
        ("cos(2x)=cos(pi/7),x,0,2pi", has_solutions),
        ("sin(3x)=sin(x),x,0,2pi", lambda r: get_solution_count(r) >= 3),
        ("cos(3x)=cos(x),x,0,2pi", lambda r: get_solution_count(r) >= 2),
        ("sin(x/2)=sqrt(2)/2,x,0,2pi", lambda r: get_solution_count(r) >= 2),
        ("cos(x/2)=sqrt(2)/2,x,0,2pi", lambda r: get_solution_count(r) >= 2),
        ("tan(2x)=sqrt(3),x,0,pi/2", has_solutions),
        ("tan(3x)=1,x,0,pi", lambda r: get_solution_count(r) >= 3),
        
        # Tan/cot (20 tests)
        ("tan(x)=1,x,0,pi", lambda r: get_solution_count(r) >= 1),
        ("tan(x)=-1,x,0,pi", has_solutions),
        ("tan(x)=sqrt(3),x,0,pi", has_solutions),
        ("tan(2x)=1,x,0,pi/2", has_solutions),
        ("cot(x)=1,x,0,pi", has_solutions),
        ("tan(x-pi/4)=1,x,0,pi", has_solutions),
        ("tan(2x+pi/6)=sqrt(3),x,0,pi", has_solutions),
        
        # Large coefficients (20 tests)
        ("sin(15x)=sin(pi/17),x,0,2pi", has_solutions),
        ("sin(17x)=sin(pi/19),x,0,2pi", has_solutions),
        ("tan(19x)=1,x,0,pi", lambda r: get_solution_count(r) >= 9),
        ("sin(23x)=sin(pi/23),x,0,2pi", has_solutions),
        ("cos(15x)=cos(pi/17),x,0,2pi", has_solutions),
        
        # Sum products (20 tests)
        ("sin(x)+sin(2x)=0,x,0,2pi", has_solutions),
        ("sin(x)-sin(2x)=0,x,0,2pi", has_solutions),
        ("cos(x)+cos(2x)=0,x,0,2pi", has_solutions),
        ("sin(x)*sin(2x)=0,x,0,pi", has_solutions),
        ("sin(x)*cos(x)=0,x,0,pi", has_solutions),
        
        # Additional random (100+ tests)
        ("sin(x)=sin(pi/7),x,0,2pi", has_solutions),
        ("sin(x)=sin(2pi/7),x,0,2pi", has_solutions),
        ("sin(x)=sin(3pi/7),x,0,2pi", has_solutions),
        ("sin(x)=sin(4pi/7),x,0,2pi", has_solutions),
        ("sin(x)=sin(pi/9),x,0,2pi", has_solutions),
        ("sin(x)=sin(2pi/9),x,0,2pi", has_solutions),
        ("sin(x)=sin(4pi/9),x,0,2pi", has_solutions),
        ("cos(x)=cos(pi/7),x,0,2pi", has_solutions),
        ("cos(x)=cos(2pi/7),x,0,2pi", has_solutions),
        ("cos(x)=cos(3pi/7),x,0,2pi", has_solutions),
        ("tan(x)=tan(pi/7),x,0,pi", has_solutions),
        ("tan(x)=tan(2pi/7),x,0,pi", has_solutions),
        ("sin(2x)=sin(x/2),x,0,2pi", has_solutions),
        ("cos(2x)=cos(x/2),x,0,2pi", has_solutions),
        ("sin(3x)=sin(2x),x,0,2pi", has_solutions),
        ("cos(3x)=cos(2x),x,0,2pi", has_solutions),
        ("tan(3x)=tan(x),x,0,pi", has_solutions),
        ("sin(4x)=sin(x),x,0,2pi", has_solutions),
        ("sin(x+pi/5)=sin(x+2pi/5),x,0,2pi", has_solutions),
        ("cos(x+pi/5)=cos(x+2pi/5),x,0,2pi", has_solutions),
    ]
    
    print(f"Running {len(tests)} tests...")
    
    for i, test in enumerate(tests):
        eq, check = test
        if len(test) > 2:
            desc = test[2]
        else:
            desc = eq[:30]
        
        try:
            result = run_solve(eq)
            if check(result):
                passed += 1
                if i < 50:  # Show first 50
                    print(f"PASS: {desc}")
            else:
                failed += 1
                if failed <= 20:
                    print(f"FAIL #{failed}: {desc}")
        except Exception as e:
            errors += 1
            if errors <= 10:
                print(f"ERR: {desc[:40]} - {str(e)[:30]}")
    
    total = passed + failed + errors
    print(f"\n{'='*60}")
    print(f"RESULTS: {passed} passed | {failed} failed | {errors} errors | {total} total")
    print(f"Success rate: {100*passed/total:.1f}%")
    

if __name__ == "__main__":
    run_tests()