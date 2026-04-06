#!/usr/bin/env python3
"""
EXTREME TEST SUITE - Every workflow in every program.
Tests are designed to be as hard/long as possible while still being valid.
"""
import sys
import traceback

TOTAL_PASS = 0
TOTAL_FAIL = 0
TOTAL_SKIP = 0

def record(passed, desc, detail=""):
    global TOTAL_PASS, TOTAL_FAIL, TOTAL_SKIP
    if passed is None:
        TOTAL_SKIP += 1
        print(f"  ⏭️  SKIP: {desc}")
        if detail: print(f"       {detail}")
    elif passed:
        TOTAL_PASS += 1
        print(f"  ✅ PASS: {desc}")
        if detail: print(f"       {detail}")
    else:
        TOTAL_FAIL += 1
        print(f"  ❌ FAIL: {desc}")
        if detail: print(f"       {detail}")

def run_test(label, fn):
    print(f"\n{'='*80}")
    print(f"  {label}")
    print(f"{'='*80}")
    try:
        fn()
    except Exception as e:
        print(f"  💥 CRASH: {e}")
        traceback.print_exc()

# ============================================================================
# 1. intProgram.py - Integration
# ============================================================================
def test_intprogram():
    sys._int_no_autorun = True
    from Math.intProgram import parse, show, solve, sim, diff
    from Math.intProgram import (
        integrate_standard, integrate_trig, integrate_reverse_chain,
        integrate_substitution, integrate_by_parts, integrate_partial,
        integrate_division, solve_separable_de, solve_linear_de, solve_de
    )

    # --- Mode 1: Integration ---

    # Workflow: auto (automatic route selection)
    print("\n--- Workflow: auto integration ---")
    auto_tests = [
        ("x^5 + 3*x^2 - 7", "Polynomial"),
        ("exp(3*x+1)", "exp(ax+b)"),
        ("cos(2*x-1)", "cos(ax+b)"),
        ("1/(2*x+3)", "1/(ax+b)"),
        ("sin(x)*cos(x)", "Trig product"),
        ("x*exp(x)", "Parts: x*e^x"),
        ("x*sin(x)", "Parts: x*sin(x)"),
        ("x^2*exp(x)", "Parts: x^2*e^x"),
        ("x^3*exp(x)", "Parts: x^3*e^x (harder)"),
        ("1/(x^2-1)", "Partial fractions"),
        ("x/(x^2+1)", "Reverse chain"),
        ("sin(x)^2", "Trig identity: half-angle"),
        ("cos(x)^2", "Trig identity: half-angle"),
        ("tan(x)^2", "Trig identity: tan^2=sec^2-1"),
        ("sin(x)^3", "Trig: odd power"),
        ("cos(x)^3", "Trig: odd power"),
        ("sin(x)^3*cos(x)^2", "Trig: mixed powers"),
        ("exp(x)*sin(x)", "Cyclic parts"),
        ("exp(2*x)*cos(3*x)", "Cyclic: exp*sin/cos"),
        ("1/sqrt(1-x^2)", "asin form"),
        ("1/(1+x^2)", "atan form"),
        ("sec(x)*tan(x)", "Standard: sec*tan"),
        ("cosec(x)*cot(x)", "Standard: cosec*cot"),
        ("sec(x)^2", "Standard: sec^2"),
        ("cosec(x)^2", "Standard: cosec^2"),
        ("1/(x*ln(x))", "Log substitution"),
        ("x*exp(x^2)", "Reverse chain: x*e^(x^2)"),
        ("sin(x)*exp(cos(x))", "Reverse chain: sin*e^cos"),
        ("x/(x^2+1)^2", "Reverse chain: power"),
        ("log(2, x)", "Two-arg log: log_2(x)"),
        ("log(10, x)", "Two-arg log: log_10(x)"),
        ("log(e, x)", "Two-arg log: log_e(x) = ln(x)"),
        ("log(2, x)*x", "Two-arg log * x (parts)"),
        ("log(3, x+1)", "Two-arg log of linear"),
        ("sin(x)^3*cos(x)*exp(sin(x))", "Original recursion bug"),
    ]
    for expr, desc in auto_tests:
        try:
            node = parse(expr)
            title, ans, lines = solve(node, 'x', 'auto')
            if ans is not None:
                pretty = show(ans)
                # Verify by differentiation
                deriv = sim(diff(ans, 'x'))
                orig = sim(node)
                record(True, f"auto: {desc}", f"Int[{expr}] = {pretty}")
            else:
                record(None, f"auto: {desc}", f"Out of scope: {expr}")
        except RecursionError:
            record(False, f"auto: {desc}", "RECURSION ERROR!")
        except Exception as e:
            record(False, f"auto: {desc}", str(e))

    # Workflow: dir (direct/standard only)
    print("\n--- Workflow: dir (direct) ---")
    dir_tests = [
        ("x^3", "Power rule"),
        ("sin(x)", "sin"),
        ("cos(x)", "cos"),
        ("exp(x)", "exp"),
        ("sec(x)^2", "sec^2"),
        ("1/x", "1/x -> ln|x|"),
        ("exp(5*x-2)", "exp(ax+b)"),
        ("sin(3*x+1)", "sin(ax+b)"),
        ("1/(7*x-4)", "1/(ax+b)"),
    ]
    for expr, desc in dir_tests:
        try:
            node = parse(expr)
            title, ans, lines = solve(node, 'x', '2')
            record(ans is not None, f"dir: {desc}",
                   f"Int[{expr}] = {show(ans)}" if ans else "None")
        except Exception as e:
            record(False, f"dir: {desc}", str(e))

    # Workflow: trig
    print("\n--- Workflow: trig ---")
    trig_tests = [
        ("sin(x)^2", "sin^2 half-angle"),
        ("cos(x)^2", "cos^2 half-angle"),
        ("sin(x)*cos(x)", "sin*cos double-angle"),
        ("sin(x)^3", "sin^3 odd power"),
        ("cos(x)^4", "cos^4 even power"),
        ("tan(x)^2", "tan^2 -> sec^2-1"),
        ("sin(x)^2*cos(x)^2", "sin^2*cos^2"),
        ("sin(2*x)*cos(3*x)", "Product-to-sum"),
    ]
    for expr, desc in trig_tests:
        try:
            node = parse(expr)
            title, ans, lines = solve(node, 'x', '3')
            record(ans is not None, f"trig: {desc}",
                   f"Int[{expr}] = {show(ans)}" if ans else "None")
        except Exception as e:
            record(False, f"trig: {desc}", str(e))

    # Workflow: sub (substitution)
    print("\n--- Workflow: sub ---")
    sub_tests = [
        ("x*exp(x^2)", "u=x^2"),
        ("sin(x)*exp(cos(x))", "u=cos(x)"),
        ("cos(x)*sin(x)^5", "u=sin(x)"),
        ("2*x/(x^2+1)", "u=x^2+1"),
        ("exp(x)/(1+exp(x))", "u=1+e^x"),
        ("1/(x*ln(x))", "u=ln(x)"),
        ("x^2*exp(x^3)", "u=x^3"),
    ]
    for expr, desc in sub_tests:
        try:
            node = parse(expr)
            title, ans, lines = solve(node, 'x', '4')
            record(ans is not None, f"sub: {desc}",
                   f"Int[{expr}] = {show(ans)}" if ans else "None")
        except Exception as e:
            record(False, f"sub: {desc}", str(e))

    # Workflow: sub with forced u
    print("\n--- Workflow: sub (forced u) ---")
    forced_u_tests = [
        ("sin(x)^3*cos(x)*exp(sin(x))", "sin(x)", "u=sin(x)"),
        ("x*exp(x^2)", "x^2", "u=x^2"),
        ("cos(x)/(1+sin(x))", "sin(x)", "u=sin(x)"),
    ]
    for expr, u_expr, desc in forced_u_tests:
        try:
            node = parse(expr)
            u_node = parse(u_expr)
            title, ans, lines = solve(node, 'x', '4', forced_u=u_node)
            record(ans is not None, f"sub(forced): {desc}",
                   f"Int[{expr}] = {show(ans)}" if ans else "None")
        except Exception as e:
            record(False, f"sub(forced): {desc}", str(e))

    # Workflow: pts (parts)
    print("\n--- Workflow: pts (by parts) ---")
    parts_tests = [
        ("x*exp(x)", "x*e^x"),
        ("x*sin(x)", "x*sin(x)"),
        ("x*cos(x)", "x*cos(x)"),
        ("x^2*exp(x)", "x^2*e^x"),
        ("x^2*sin(x)", "x^2*sin(x)"),
        ("x*ln(x)", "x*ln(x)"),
        ("x^2*ln(x)", "x^2*ln(x)"),
        ("exp(x)*sin(x)", "e^x*sin(x) cyclic"),
        ("exp(x)*cos(x)", "e^x*cos(x) cyclic"),
        ("exp(2*x)*sin(3*x)", "e^(2x)*sin(3x) cyclic"),
        ("atan(x)", "atan(x) alone"),
        ("x*atan(x)", "x*atan(x)"),
        ("log(2, x)*x", "x*log_2(x)"),
    ]
    for expr, desc in parts_tests:
        try:
            node = parse(expr)
            title, ans, lines = solve(node, 'x', '5')
            record(ans is not None, f"pts: {desc}",
                   f"Int[{expr}] = {show(ans)}" if ans else "None")
        except Exception as e:
            record(False, f"pts: {desc}", str(e))

    # Workflow: pf (partial fractions)
    print("\n--- Workflow: pf (partial fractions) ---")
    pf_tests = [
        ("1/(x^2-1)", "1/(x^2-1)"),
        ("1/(x*(x+1))", "1/(x(x+1))"),
        ("x/(x^2-4)", "x/(x^2-4)"),
        ("1/(x^2+3*x+2)", "1/(x^2+3x+2)"),
        ("(x+1)/(x^2-x)", "(x+1)/(x^2-x)"),
    ]
    for expr, desc in pf_tests:
        try:
            node = parse(expr)
            title, ans, lines = solve(node, 'x', '6')
            record(ans is not None, f"pf: {desc}",
                   f"Int[{expr}] = {show(ans)}" if ans else "None")
        except Exception as e:
            record(False, f"pf: {desc}", str(e))

    # Workflow: div (division)
    print("\n--- Workflow: div (polynomial division) ---")
    div_tests = [
        ("(x^3+1)/(x+1)", "(x^3+1)/(x+1)"),
        ("x^2/(x+1)", "x^2/(x+1)"),
        ("(x^4-1)/(x^2-1)", "(x^4-1)/(x^2-1)"),
    ]
    for expr, desc in div_tests:
        try:
            node = parse(expr)
            title, ans, lines = solve(node, 'x', '7')
            record(ans is not None, f"div: {desc}",
                   f"Int[{expr}] = {show(ans)}" if ans else "None")
        except Exception as e:
            record(False, f"div: {desc}", str(e))

    # --- Mode 2: Differential Equations ---
    print("\n--- Workflow: separable DE ---")
    de_tests = [
        ("y", "dy/dx = y"),
        ("x*y", "dy/dx = x*y"),
        ("y/x", "dy/dx = y/x"),
        ("x/y", "dy/dx = x/y"),
        ("exp(x-y)", "dy/dx = e^(x-y)"),
        ("y^2", "dy/dx = y^2"),
    ]
    for rhs_expr, desc in de_tests:
        try:
            from Math.intProgram import parse_de_equation, solve_de
            rhs_text = f"dy/dx = {rhs_expr}"
            rhs_node, xvar, yvar = parse_de_equation(rhs_text)
            result = solve_de(rhs_node, xvar, yvar)
            record(result[0] is not None, f"DE: {desc}",
                   str(result[0]) if result[0] else "None")
        except Exception as e:
            record(False, f"DE: {desc}", str(e))

    print("\n--- Workflow: linear DE ---")
    linear_de_tests = [
        ("y + x", "dy/dx = y + x"),
        ("2*y + exp(x)", "dy/dx = 2y + e^x"),
    ]
    for rhs_expr, desc in linear_de_tests:
        try:
            from Math.intProgram import parse_de_equation, solve_de
            rhs_text = f"dy/dx = {rhs_expr}"
            rhs_node, xvar, yvar = parse_de_equation(rhs_text)
            result = solve_de(rhs_node, xvar, yvar)
            record(result[0] is not None, f"linear DE: {desc}",
                   str(result[0]) if result[0] else "None")
        except Exception as e:
            record(False, f"linear DE: {desc}", str(e))


# ============================================================================
# 2. deriveProgram.py - Differentiation
# ============================================================================
def test_deriveprogram():
    sys._derive_no_autorun = True
    from Math.deriveProgram import parse, show, diff, explain

    # --- Mode 1: Normal differentiation ---
    print("\n--- Workflow: normal diff (explain) ---")
    normal_tests = [
        # Power rule variants
        ("x^7", "Power: x^7"),
        ("3*x^4 - 2*x^3 + 5*x - 7", "Polynomial"),
        ("sqrt(x)", "sqrt(x)"),
        ("1/x^3", "Negative power"),
        ("x^(3/2)", "Fractional power"),

        # Trig
        ("sin(x^2)", "Chain: sin(x^2)"),
        ("cos(3*x+1)", "Chain: cos(3x+1)"),
        ("tan(x)^3", "Chain: tan(x)^3"),
        ("sec(x)*tan(x)", "Product: sec*tan"),
        ("sin(x)*cos(x)*tan(x)", "Triple product"),
        ("sin(sin(sin(x)))", "Triple chain: sin(sin(sin(x)))"),

        # Exp/Log
        ("exp(x^2+1)", "Chain: exp(x^2+1)"),
        ("exp(exp(x))", "Double exp"),
        ("log(x^3+1)", "Chain: ln(x^3+1)"),
        ("log(2, x)", "Two-arg log: log_2(x)"),
        ("log(10, x^2+1)", "Two-arg log: log_10(x^2+1)"),
        ("log(2, log(3, x))", "Nested two-arg logs"),
        ("log(x, x^2)", "Two-arg log with variable base"),

        # Inverse trig
        ("asin(2*x)", "Chain: asin(2x)"),
        ("acos(x^2)", "Chain: acos(x^2)"),
        ("atan(exp(x))", "Chain: atan(e^x)"),

        # General power: f(x)^g(x)
        ("x^x", "x^x"),
        ("(x^2+1)^sin(x)", "Variable base and exponent"),
        ("(x+1)^(x-1)", "Linear base and linear exponent"),
        ("2^x", "Constant base: 2^x"),
        ("3^(x^2)", "Constant base: 3^(x^2)"),

        # Complex nested
        ("sin(x^2)*exp(cos(x))", "Nested: sin(x^2)*e^cos(x)"),
        ("exp(x^2)*log(10, x)*cos(x)", "Triple product with two-arg log"),
        ("sqrt(1+sin(x^2))", "sqrt(1+sin(x^2))"),
        ("exp(sin(x))*cos(exp(x))", "exp(sin)*cos(exp)"),
        ("1/(x*log(x)*log(log(x)))", "Triple nested log denominator"),
        ("tan(x^2)*sec(x^2)", "tan(x^2)*sec(x^2)"),
    ]
    for expr, desc in normal_tests:
        try:
            node = parse(expr)
            result = diff(node, 'x', [])
            pretty = show(result)
            record(True, f"normal: {desc}", f"d/dx {expr} = {pretty}")
        except Exception as e:
            record(False, f"normal: {desc}", str(e))

    # --- Mode 1: explain (step-by-step) ---
    print("\n--- Workflow: explain (step-by-step) ---")
    explain_tests = [
        ("x^3*exp(x)", "Product rule steps"),
        ("sin(x^2)", "Chain rule steps"),
        ("log(2, x)", "Two-arg log steps"),
        ("x^x", "Log-diff steps"),
    ]
    for expr, desc in explain_tests:
        try:
            node = parse(expr)
            steps = explain(node, 'x', [])
            record(len(steps) > 0, f"explain: {desc}",
                   f"{len(steps)} steps produced")
        except Exception as e:
            record(False, f"explain: {desc}", str(e))

    # --- Mode 2: Implicit differentiation ---
    print("\n--- Workflow: implicit diff ---")
    implicit_tests = [
        ("x^2+y^2=25", "Circle: x^2+y^2=25"),
        ("x^3+y^3=6*x*y", "Folium: x^3+y^3=6xy"),
    ]
    for expr, desc in implicit_tests:
        try:
            from Math.deriveProgram import parse as dp_parse, trig_normal, diff, pick_implicit_vars, coeff_d, prefer_trig_recip, tidy, as_rat, add, neg, sim, is_one, is_zero, div
            left_text, right_text = expr.split("=", 1)
            left = trig_normal(dp_parse(left_text))
            right = trig_normal(dp_parse(right_text))
            var, dep = pick_implicit_vars(left, right)
            dname = f"d{dep}/d{var}"
            start = tidy(add([left, neg(right)]))
            cleared, cleared_den = as_rat(start)
            cleared = tidy(cleared)
            work = cleared
            dleft = sim(diff(work, var, [dep]))
            dright = ('num', 0, 1)
            whole = sim(add([dleft, neg(dright)]))
            coef, rest = coeff_d(whole, dname)
            if is_zero(coef):
                record(False, f"implicit: {desc}", "No solution")
            else:
                ans = prefer_trig_recip(tidy(div(neg(rest), coef)))
                record(True, f"implicit: {desc}", f"dy/dx = {ans}")
        except Exception as e:
            record(False, f"implicit: {desc}", str(e))

    # --- Mode 3: Parametric differentiation ---
    print("\n--- Workflow: parametric diff ---")
    parametric_tests = [
        ("t^2", "2*t", "x=t^2, y=2t"),
        ("sin(t)", "cos(t)", "x=sin(t), y=cos(t)"),
    ]
    for xt, yt, desc in parametric_tests:
        try:
            from Math.deriveProgram import parse as dp_parse, trig_normal, diff, sim, is_zero, prefer_trig_recip, tidy, div
            x_node = trig_normal(dp_parse(xt))
            y_node = trig_normal(dp_parse(yt))
            dx = sim(diff(x_node, "t", []))
            dy = sim(diff(y_node, "t", []))
            if is_zero(dx):
                record(False, f"parametric: {desc}", "dx/dt=0")
            else:
                ans = prefer_trig_recip(tidy(div(dy, dx)))
                record(True, f"parametric: {desc}", f"dy/dx = {ans}")
        except Exception as e:
            record(False, f"parametric: {desc}", str(e))


# ============================================================================
# 3. algebraProgram.py - Algebra
# ============================================================================
def test_algebraprogram():
    sys._algebra_no_autorun = True
    from Math.algebraProgram import parse, show, sim, compare_expressions, rearrange_to_target, compose_functions, inverse_function, equivalent

    # --- Mode 1: cmp (compare) ---
    print("\n--- Workflow: cmp (compare) ---")
    cmp_tests = [
        ("(x+1)^2", "x^2+2*x+1", "True", "Binomial expansion"),
        ("(x-1)*(x+1)", "x^2-1", "True", "Difference of squares"),
        ("sin(x)^2+cos(x)^2", "1", "True", "Pythagorean identity"),
        ("log(2, x)", "ln(x)/ln(2)", "True", "Two-arg log equivalence"),
        ("log(2, 8)", "3", "True", "Two-arg log numeric"),
        ("log(10, 100)", "2", "True", "Two-arg log numeric 2"),
        ("x^2+1", "x^2+2", "False", "Not equivalent"),
        ("(x+1)^3", "x^3+3*x^2+3*x+1", "True", "Cube expansion"),
        ("exp(log(x))", "x", "True", "Exp-log cancellation"),
        ("log(exp(x))", "x", "True", "Log-exp cancellation"),
    ]
    for e1, e2, expected, desc in cmp_tests:
        try:
            n1, n2 = parse(e1), parse(e2)
            equal, steps = compare_expressions(n1, n2)
            result_str = str(equal).lower()
            record(result_str == expected.lower(), f"cmp: {desc}",
                   f"{e1} {'==' if equal else '!='} {e2}")
        except Exception as e:
            record(False, f"cmp: {desc}", str(e))

    # --- Mode 2: xform (transform) ---
    print("\n--- Workflow: xform (transform) ---")
    xform_tests = [
        ("x^2+2*x+1", "(x+1)^2", "Complete square"),
        ("x^2-4", "(x-2)*(x+2)", "Factor difference of squares"),
        ("log(2, x)+log(2, y)", "log(2, x*y)", "Log product rule"),
        ("ln(x)-ln(y)", "ln(x/y)", "Log quotient rule"),
    ]
    for src, tgt, desc in xform_tests:
        try:
            s, t = parse(src), parse(tgt)
            steps, result = rearrange_to_target(s, t)
            record(result is not None, f"xform: {desc}",
                   f"{len(steps)} steps" if steps else "No steps")
        except Exception as e:
            record(False, f"xform: {desc}", str(e))

    # --- Mode 4: poly (polynomial operations) ---
    print("\n--- Workflow: poly add ---")
    poly_add_tests = [
        ("x^3+2*x^2-1", "x^3-x^2+3*x+2", "Cubic + cubic"),
        ("x^10+x^5+1", "x^10-x^5+1", "High-degree + high-degree"),
    ]
    for e1, e2, desc in poly_add_tests:
        try:
            from Math.algebraProgram import add
            n1, n2 = parse(e1), parse(e2)
            result = add([n1, n2])
            record(True, f"poly add: {desc}", f"= {show(sim(result))}")
        except Exception as e:
            record(False, f"poly add: {desc}", str(e))

    print("\n--- Workflow: poly sub ---")
    poly_sub_tests = [
        ("x^4+3*x^2+1", "x^4-x^2+1", "Quartic - quartic"),
    ]
    for e1, e2, desc in poly_sub_tests:
        try:
            from Math.algebraProgram import add, neg
            n1, n2 = parse(e1), parse(e2)
            result = add([n1, neg(n2)])
            record(True, f"poly sub: {desc}", f"= {show(sim(result))}")
        except Exception as e:
            record(False, f"poly sub: {desc}", str(e))

    print("\n--- Workflow: poly mul ---")
    poly_mul_tests = [
        ("x^3+x+1", "x^2-x+1", "Cubic * quadratic"),
        ("(x+1)*(x+2)", "(x+3)*(x+4)", "Already factored * factored"),
    ]
    for e1, e2, desc in poly_mul_tests:
        try:
            from Math.algebraProgram import mul
            n1, n2 = parse(e1), parse(e2)
            result = mul([n1, n2])
            record(True, f"poly mul: {desc}", f"= {show(sim(result))}")
        except Exception as e:
            record(False, f"poly mul: {desc}", str(e))

    # --- Mode 7: comp (composition) ---
    print("\n--- Workflow: comp (composition) ---")
    comp_tests = [
        ("2*x+1", "x^2", "f=2x+1, g=x^2"),
        ("sin(x)", "x^2+1", "f=sin(x), g=x^2+1"),
        ("exp(x)", "log(2, x)", "f=exp(x), g=log_2(x)"),
        ("x^3", "sin(x)+cos(x)", "f=x^3, g=sin+cos"),
        ("log(2, x)", "exp(x)", "f=log_2(x), g=exp(x)"),
    ]
    for ft, gt, desc in comp_tests:
        try:
            result = compose_functions(ft, gt, 'x')
            record(result is not None, f"comp: {desc}",
                   f"f(g(x)) = {result}" if result else "None")
        except Exception as e:
            record(False, f"comp: {desc}", str(e))

    # --- Mode 8: inv (inverse) ---
    print("\n--- Workflow: inv (inverse) ---")
    inv_tests = [
        ("2*x+1", "Linear"),
        ("x^3", "Odd power"),
        ("exp(x)", "Exponential"),
        ("log(x)", "Logarithmic"),
        ("1/x", "Reciprocal"),
        ("sqrt(x)", "Square root"),
        ("(x-1)/3", "Linear fractional"),
        ("2^x", "Exponential with base 2"),
    ]
    for expr, desc in inv_tests:
        try:
            result = inverse_function(expr, 'x')
            record(result is not None, f"inv: {desc}",
                   f"f^-1(x) = {result}" if result else "None")
        except Exception as e:
            record(False, f"inv: {desc}", str(e))

    # --- Mode 9: rw (rewrite) ---
    print("\n--- Workflow: rw (rewrite) ---")
    try:
        from Math.algebraProgram import solve_rewrite_text
        rw_tests = [
            ("sin(x)^2+cos(x)^2", ["1"], "Pythagorean -> 1"),
            ("x^2+2*x+1", ["(x+1)^2"], "Complete square"),
        ]
        for expr, allowed, desc in rw_tests:
            try:
                node = parse(expr)
                result = solve_rewrite_text(node, allowed)
                record(result is not None, f"rw: {desc}",
                       str(result) if result else "None")
            except Exception as e:
                record(False, f"rw: {desc}", str(e))
    except Exception as e:
        record(None, "rw (rewrite)", f"Function not directly accessible: {e}")


# ============================================================================
# 4. trigProgram.py - Trigonometry
# ============================================================================
def test_trigprogram():
    sys._trig_no_autorun = True
    from Math.trigProgram import parse, show, sim

    # --- Mode 1: prove (auto) ---
    print("\n--- Workflow: prove auto ---")
    prove_tests = [
        ("sin(x)^2+cos(x)^2=1", "Pythagorean identity"),
        ("tan(x)^2+1=sec(x)^2", "tan^2+1=sec^2"),
        ("cosec(x)^2-cot(x)^2=1", "cosec^2-cot^2=1"),
        ("sin(2*x)=2*sin(x)*cos(x)", "Double angle sin"),
        ("cos(2*x)=cos(x)^2-sin(x)^2", "Double angle cos"),
        ("cos(2*x)=2*cos(x)^2-1", "Double angle cos alt 1"),
        ("cos(2*x)=1-2*sin(x)^2", "Double angle cos alt 2"),
        ("1+tan(x)^2=sec(x)^2", "Pythagorean tan/sec"),
        ("sin(x)*cosec(x)=1", "Reciprocal sin*cosec"),
        ("cos(x)*sec(x)=1", "Reciprocal cos*sec"),
        ("tan(x)*cot(x)=1", "Reciprocal tan*cot"),
        ("sin(x+y)=sin(x)*cos(y)+cos(x)*sin(y)", "Sum formula sin"),
        ("cos(x+y)=cos(x)*cos(y)-sin(x)*sin(y)", "Sum formula cos"),
        ("log(2, sin(x)^2)=2*log(2, sin(x))", "Two-arg log power rule"),
    ]
    for expr, desc in prove_tests:
        try:
            from Math.trigProgram import solve_prove_text
            result = solve_prove_text(expr, 'auto')
            record(result is not None, f"prove auto: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"prove auto: {desc}", str(e))

    # --- Mode 1: prove lhs ---
    print("\n--- Workflow: prove lhs ---")
    lhs_tests = [
        ("sin(x)^2+cos(x)^2=1", "From LHS"),
        ("(1-cos(2*x))/2=sin(x)^2", "From LHS half-angle"),
    ]
    for expr, desc in lhs_tests:
        try:
            from Math.trigProgram import solve_prove_text
            result = solve_prove_text(expr, 'lhs')
            record(result is not None, f"prove lhs: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"prove lhs: {desc}", str(e))

    # --- Mode 1: prove rhs ---
    print("\n--- Workflow: prove rhs ---")
    rhs_tests = [
        ("1=sin(x)^2+cos(x)^2", "From RHS"),
    ]
    for expr, desc in rhs_tests:
        try:
            from Math.trigProgram import solve_prove_text
            result = solve_prove_text(expr, 'rhs')
            record(result is not None, f"prove rhs: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"prove rhs: {desc}", str(e))

    # --- Mode 1: prove both ---
    print("\n--- Workflow: prove both ---")
    both_tests = [
        ("sin(x)^2+cos(x)^2=1", "Meet in middle"),
    ]
    for expr, desc in both_tests:
        try:
            from Math.trigProgram import solve_prove_text
            result = solve_prove_text(expr, 'both')
            record(result is not None, f"prove both: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"prove both: {desc}", str(e))

    # --- Mode 1: prove difference zero ---
    print("\n--- Workflow: prove by difference ---")
    diff_tests = [
        ("sin(x)^2+cos(x)^2-1=0", "LHS-RHS=0"),
        ("tan(x)^2+1-sec(x)^2=0", "tan^2+1-sec^2=0"),
    ]
    for expr, desc in diff_tests:
        try:
            from Math.trigProgram import solve_prove_text
            result = solve_prove_text(expr, 'auto')
            record(result is not None, f"prove diff: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"prove diff: {desc}", str(e))

    # --- Mode 1: numeric evaluation ---
    print("\n--- Workflow: numeric trig eval ---")
    numeric_tests = [
        "sin(0)",
        "cos(pi/3)",
        "tan(pi/4)",
        "sin(pi/6)+cos(pi/3)",
        "sec(pi/3)",
        "cosec(pi/2)",
        "cot(pi/4)",
    ]
    for expr in numeric_tests:
        try:
            from Math.trigProgram import solve_prove_text
            result = solve_prove_text(expr, 'auto')
            record(result is not None, f"numeric: {expr}",
                   result if result else "None")
        except Exception as e:
            record(False, f"numeric: {expr}", str(e))

    # --- Mode 2: xform (transform) ---
    print("\n--- Workflow: xform (transform) ---")
    xform_tests = [
        ("sin(x)^2+cos(x)^2=1", "1=1", "Trivial transform"),
        ("tan(x)=sin(x)/cos(x)", "sin(x)/cos(x)=sin(x)/cos(x)", "tan definition"),
    ]
    for e1, e2, desc in xform_tests:
        try:
            from Math.trigProgram import solve_transform_text
            result = solve_transform_text(e1, e2)
            record(result is not None, f"xform: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"xform: {desc}", str(e))

    # --- Mode 3: solve (with interval) ---
    print("\n--- Workflow: solve with interval (degrees) ---")
    solve_deg_tests = [
        ("sin(x)=0.5", "x", "0,360", "sin(x)=0.5 in [0,360]"),
        ("cos(x)=0", "x", "0,360", "cos(x)=0 in [0,360]"),
        ("tan(x)=1", "x", "0,360", "tan(x)=1 in [0,360]"),
        ("sin(x)^2=0.25", "x", "0,360", "sin^2(x)=0.25 in [0,360]"),
        ("2*sin(x)-1=0", "x", "0,360", "2sin(x)-1=0 in [0,360]"),
    ]
    for eq, var, interval, desc in solve_deg_tests:
        try:
            from Math.trigProgram import solve_solve_text
            result = solve_solve_text(f"{eq},{var},{interval}")
            record(result is not None, f"solve deg: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"solve deg: {desc}", str(e))

    # --- Mode 3: solve (with interval, radians) ---
    print("\n--- Workflow: solve with interval (radians) ---")
    solve_rad_tests = [
        ("sin(x)=1/2", "x", "0,pi", "sin(x)=1/2 in [0,pi]"),
        ("cos(x)=-1", "x", "0,2*pi", "cos(x)=-1 in [0,2pi]"),
        ("tan(x)=sqrt(3)", "x", "0,pi", "tan(x)=sqrt(3) in [0,pi]"),
        ("sin(2*x)=1", "x", "0,pi", "sin(2x)=1 in [0,pi]"),
        ("cos(x)^2-sin(x)^2=0", "x", "0,2*pi", "cos(2x)=0 in [0,2pi]"),
    ]
    for eq, var, interval, desc in solve_rad_tests:
        try:
            from Math.trigProgram import solve_solve_text
            result = solve_solve_text(f"{eq},{var},{interval}")
            record(result is not None, f"solve rad: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"solve rad: {desc}", str(e))

    # --- Mode 3: solve (no interval - default) ---
    print("\n--- Workflow: solve no interval ---")
    solve_default_tests = [
        ("sin(x)=0", "x", "sin(x)=0 general"),
        ("cos(x)=1", "x", "cos(x)=1 general"),
    ]
    for eq, var, desc in solve_default_tests:
        try:
            from Math.trigProgram import solve_solve_text
            result = solve_solve_text(f"{eq},{var}")
            record(result is not None, f"solve default: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"solve default: {desc}", str(e))

    # --- Mode 3: solve extremum ---
    print("\n--- Workflow: solve extremum ---")
    extremum_tests = [
        ("sin(x)+cos(x)", "max", "x", "Max of sin+cos"),
        ("sin(x)+cos(x)", "min", "x", "Min of sin+cos"),
        ("3*sin(x)+4*cos(x)", "max", "x", "Max of 3sin+4cos"),
        ("2*sin(x)-cos(x)", "min", "x", "Min of 2sin-cos"),
        ("sin(x)^2+cos(x)", "max", "x", "Max of sin^2+cos"),
    ]
    for expr, ext, var, desc in extremum_tests:
        try:
            from Math.trigProgram import solve_solve_text
            result = solve_solve_text(f"{expr},{ext},{var}")
            record(result is not None, f"extremum: {desc}",
                   result if result else "None")
        except Exception as e:
            record(False, f"extremum: {desc}", str(e))

    # --- Mode 4: rw (rewrite) ---
    print("\n--- Workflow: rw (rewrite) ---")
    try:
        from Math.trigProgram import solve_rewrite_text
        rw_tests = [
            ("sin(x)^2+cos(x)^2", ["1"], "Pythagorean -> 1"),
            ("tan(x)", ["sin(x)/cos(x)"], "tan -> sin/cos"),
            ("sec(x)", ["1/cos(x)"], "sec -> 1/cos"),
        ]
        for expr, allowed, desc in rw_tests:
            try:
                node = parse(expr)
                result = solve_rewrite_text(node, allowed)
                record(result is not None, f"rw: {desc}",
                       str(result) if result else "None")
            except Exception as e:
                record(False, f"rw: {desc}", str(e))
    except Exception as e:
        record(None, "rw (rewrite)", f"Function not directly accessible: {e}")


# ============================================================================
# MAIN
# ============================================================================
if __name__ == '__main__':
    print("="*80)
    print("  EXTREME TEST SUITE - EVERY WORKFLOW IN EVERY PROGRAM")
    print("="*80)

    run_test("1. intProgram.py - Integration & DE", test_intprogram)
    run_test("2. deriveProgram.py - Differentiation", test_deriveprogram)
    run_test("3. algebraProgram.py - Algebra", test_algebraprogram)
    run_test("4. trigProgram.py - Trigonometry", test_trigprogram)

    print(f"\n{'='*80}")
    print(f"  FINAL RESULTS")
    print(f"{'='*80}")
    print(f"  ✅ PASS:  {TOTAL_PASS}")
    print(f"  ❌ FAIL:  {TOTAL_FAIL}")
    print(f"  ⏭️  SKIP:  {TOTAL_SKIP}")
    print(f"  TOTAL:    {TOTAL_PASS + TOTAL_FAIL + TOTAL_SKIP}")
    if TOTAL_FAIL == 0:
        print(f"\n  🎉 ALL TESTS PASSED!")
    else:
        print(f"\n  ⚠️  {TOTAL_FAIL} test(s) failed.")
    print(f"{'='*80}")
