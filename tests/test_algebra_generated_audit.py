import random
import subprocess
import sys
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/Math')
PY = '/Users/james/Developer/Python/CASIO/.venv/bin/python'
sys.path.insert(0, str(ROOT))
sys._algebra_no_autorun = True

import algebraProgram as ap

PASS = 0
FAIL = 0

RNG = random.Random(81231)


def report(label, passed, detail=''):
    global PASS, FAIL
    if passed:
        PASS += 1
        print('  PASS:', label)
    else:
        FAIL += 1
        print('  FAIL:', label, detail)


def run_cli(user_input):
    proc = subprocess.run([PY, str(ROOT / 'algebraProgram.py')], input=user_input, text=True, capture_output=True, check=False)
    return proc.stdout


def pick_nonzero(low, high):
    while True:
        value = RNG.randint(low, high)
        if value != 0:
            return value


def expand_positive(expr_text):
    expr = ap.parse(expr_text)
    expr = ap.sim(expr)
    if expr[0] != 'pow' or not ap.is_int_num(expr[2]) or expr[2][1] < 0 or expr[2][2] != 1:
        return ap.show(ap.maybe_expand_for_compare(expr))
    base = expr[1]
    n = expr[2][1]
    out = ap.num(1)
    i = 0
    while i < n:
        out = ap.sim(ap.expand_mul_distribute(ap.mul([out, base])))
        i += 1
    return ap.show(out)


def complete_square_result(a, b, c):
    shift = ap.sim(ap.div(ap.num(b), ap.mul([ap.num(2), ap.num(a)])))
    square = ap.sim(ap.add([ap.sym('x'), shift]))
    constant = ap.sim(ap.sub(ap.num(c), ap.div(ap.power(ap.num(b), ap.num(2)), ap.mul([ap.num(4), ap.num(a)]))))
    parts = [ap.mul([ap.num(a), ap.power(square, ap.num(2))])]
    if not ap.is_zero(constant):
        parts.append(constant)
    return ap.show(ap.sim(ap.add(parts)))


def root_list_text(values):
    nodes = []
    i = 0
    while i < len(values):
        nodes.append(ap.num(values[i]))
        i += 1
    return ap.format_solution_line('x', nodes)


def factor_term_from_root(root):
    if root < 0:
        return f'(x+{-root})'
    if root > 0:
        return f'(x-{root})'
    return '(x)'


def compare_cases():
    cases = []
    i = 0
    while i < 6:
        a = pick_nonzero(1, 4)
        b = pick_nonzero(-4, 4)
        expr1 = f'({a}*x+{b})^2'
        expr2 = expand_positive(expr1)
        cases.append((f'alg_cmp_gen_{i + 1}', f'1\n{expr1}\n{expr2}\n', 'Equivalent'))
        i += 1
    cases.extend([
        ('alg_cmp_gen_7', '1\nsin(x)^2+cos(x)^2\n1\n', 'Equivalent'),
        ('alg_cmp_gen_8', '1\n(1-cos(2*x))/2\nsin(x)^2\n', 'Equivalent'),
        ('alg_cmp_gen_9', '1\n(x+1)^2\nx^2+1\n', 'Not equivalent'),
        ('alg_cmp_gen_10', '1\n(1+cos(2*x))/2\nsin(x)^2\n', 'Not equivalent'),
    ])
    return cases


def transform_cases():
    return [
        ('alg_xform_gen_1', '2\n1-cos(2*x)\n2*sin(x)^2\n', 'Final = 2*(sin(x))^2'),
        ('alg_xform_gen_2', '2\n1+cos(2*x)\n2*cos(x)^2\n', 'Final = 2*(cos(x))^2'),
        ('alg_xform_gen_3', '2\nsin(x)/cos(x)\ntan(x)\n', 'Final = tan(x)'),
        ('alg_xform_gen_4', '2\ncos(x)/sin(x)\ncot(x)\n', 'Final = cot(x)'),
        ('alg_xform_gen_5', '2\n1/cos(x)\nsec(x)\n', 'Final = sec(x)'),
        ('alg_xform_gen_6', '2\n1/sin(x)\ncosec(x)\n', 'Final = cosec(x)'),
        ('alg_xform_gen_7', '2\n(1-cos(x))/sin(x)\ntan(x/2)\n', 'Final = tan(x/2)'),
        ('alg_xform_gen_8', '2\nsin(x)/(1+cos(x))\ntan(x/2)\n', 'Final = tan(x/2)'),
        ('alg_xform_gen_9', '2\n(x+2)*(x-3)\nx^2-x-6\n', 'Final = x^2-x-6'),
        ('alg_xform_gen_10', '2\nexp(log(x+3))\nx+3\n', 'Final = x+3'),
    ]


def expand_cases():
    cases = []
    i = 0
    while i < 10:
        a = pick_nonzero(1, 4)
        b = pick_nonzero(-4, 4)
        n = RNG.randint(2, 5)
        expr = f'({a}*x+{b})^{n}'
        cases.append((f'alg_exp_gen_{i + 1}', expr, expand_positive(expr)))
        i += 1
    return cases


def factor_cases():
    cases = []
    roots = [(-4, -1), (-3, 2), (-5, 1), (3, -2), (4, 1)]
    i = 0
    while i < len(roots):
        r1, r2 = roots[i]
        product = ap.mul([ap.add([ap.sym('x'), ap.num(-r1)]), ap.add([ap.sym('x'), ap.num(-r2)])])
        expr = ap.show(ap.sim(ap.expand_mul_distribute(product)))
        cases.append((f'alg_poly_gen_{i + 1}', expr, [factor_term_from_root(r1), factor_term_from_root(r2)]))
        i += 1
    cases.extend([
        ('alg_poly_gen_6', 'x^2-16', ['(x+4)', '(x-4)']),
        ('alg_poly_gen_7', '25-x^2', ['(x+5)', '(-x+5)']),
        ('alg_poly_gen_8', '9*y^2-1', ['(3*y+1)', '(3*y-1)']),
        ('alg_poly_gen_9', '4*x^2-49', ['(2*x+7)', '(2*x-7)']),
        ('alg_poly_gen_10', 'x^2+7*x+12', ['(x+3)', '(x+4)']),
    ])
    return cases


def complete_square_cases():
    coeffs = [
        (1, 6, 2),
        (2, 8, -3),
        (3, -12, 5),
        (4, 4, -7),
        (5, -10, 9),
        (2, -6, -1),
        (3, 9, 7),
        (1, -8, 3),
        (6, 12, -5),
        (7, -14, 4),
    ]
    cases = []
    i = 0
    while i < len(coeffs):
        a, b, c = coeffs[i]
        cases.append((f'alg_csq_gen_{i + 1}', f'{a}*x^2+{b}*x+{c}', complete_square_result(a, b, c)))
        i += 1
    return cases


def solve_cases():
    return [
        ('alg_solve_gen_1', '6\nx^2-5*x+6=0\n', ['x = [2, 3]', 'x = [3, 2]']),
        ('alg_solve_gen_2', '6\nx^2+x-6=0\n', ['x = [-3, 2]', 'x = [2, -3]']),
        ('alg_solve_gen_3', '6\n2*x^2-5*x-3=0\n', ['-1/2', '3']),
        ('alg_solve_gen_4', '6\n3*x^2+6*x=0\n', ['x = [0, -2]', 'x = [-2, 0]']),
        ('alg_solve_gen_5', '6\n4*x-12=0\n', ['x = 3']),
        ('alg_solve_gen_6', '6\nx^2-16=0\n', ['4', '-4']),
        ('alg_solve_gen_7', '6\n9-x^2=0\n', ['3', '-3']),
        ('alg_solve_gen_8', '6\n(x+2)*(x-5)=0\n', ['-2', '5']),
        ('alg_solve_gen_9', '6\n(x+1)/(x-2)=2\n', ['x = 5']),
        ('alg_solve_gen_10', '6\n1/(x+1)+1/(x-3)=0\n', ['x = 1']),
    ]


def compose_cases():
    return [
        ('alg_comp_gen_1', '7\n2*x+3\nx^2-1\n', 'f(g(x)) = 2*x^2+1'),
        ('alg_comp_gen_2', '7\nx^2+4*x+1\n3*x-2\n', 'f(g(x)) = (3*x-2)^2+12*x-7'),
        ('alg_comp_gen_3', '7\nexp(x)\n2*x-1\n', 'f(g(x)) = e^(2*x-1)'),
        ('alg_comp_gen_4', '7\nlog(x)\n(x+3)^2\n', 'f(g(x)) = 2*ln(x+3)'),
        ('alg_comp_gen_5', '7\nsqrt(x+5)\n4*x-1\n', 'f(g(x)) = sqrt(4*x+4)'),
        ('alg_comp_gen_6', '7\n(2*x+1)/(x+4)\nx^2\n', 'f(g(x)) = (2*x^2+1)/(x^2+4)'),
        ('alg_comp_gen_7', '7\nx^3-1\nx+2\n', 'f(g(x)) = (x+2)^3-1'),
        ('alg_comp_gen_8', '7\n1/(x+1)\n3*x-2\n', 'f(g(x)) = 1/(3*x-1)'),
        ('alg_comp_gen_9', '7\n(x+1)^2\n2*x+5\n', 'f(g(x)) = (2*x+6)^2'),
        ('alg_comp_gen_10', '7\nsin(x)\n3*x-1\n', 'f(g(x)) = sin(3*x-1)'),
    ]


def inverse_cases():
    return [
        ('alg_inv_gen_1', '8\n3*x-7\n', 'f^-1(x) = (x+7)/3'),
        ('alg_inv_gen_2', '8\n(2*x+1)/(3*x+4)\n', 'f^-1(x) = (-4*x+1)/(3*x-2)'),
        ('alg_inv_gen_3', '8\n1/(x+2)^3\n', 'f^-1(x) = 1/x^(1/3)-2'),
        ('alg_inv_gen_4', '8\nsqrt((2*x+5)^3)\n', 'f^-1(x) = (x^(2/3)-5)/2'),
        ('alg_inv_gen_5', '8\n5*x+1\n', 'f^-1(x) = (x-1)/5'),
        ('alg_inv_gen_6', '8\n(4*x-3)/(2*x+5)\n', 'f^-1(x) = (-5*x-3)/(2*x-4)'),
        ('alg_inv_gen_7', '8\n1/(3*x-1)^3\n', 'f^-1(x) = (1/x^(1/3)+1)/3'),
        ('alg_inv_gen_8', '8\nsqrt((x+4)^3)\n', 'f^-1(x) = x^(2/3)-4'),
        ('alg_inv_gen_9', '8\n7*x-9\n', 'f^-1(x) = (x+9)/7'),
        ('alg_inv_gen_10', '8\nx^2+1\n', 'Unsupported inverse family'),
    ]


def rewrite_cases():
    return [
        ('alg_rw_gen_1', '9\nx^2+5*x+6\n2*x+1\n\n', '= (1/4)*T^2+2*T+15/4'),
        ('alg_rw_gen_2', '9\nx^2+7*x+10\nx+2\n\n', '= T^2+3*T'),
        ('alg_rw_gen_3', '9\nx\n1/(x+3)+4\n\n', '= 1/(T-4)-3'),
        ('alg_rw_gen_4', '9\n1-cos(2*x)\nsin(x)\n\n', 'Final = 2*(sin(x))^2'),
        ('alg_rw_gen_5', '9\n1+cos(2*x)\ncos(x)\n\n', 'Final = 2*(cos(x))^2'),
        ('alg_rw_gen_6', '9\nsin(x)^2+cos(x)^2\nsin(x)\ncos(x)\n\n', 'Final = 1'),
        ('alg_rw_gen_7', '9\nx^2+9*x+14\nx+4\n\n', '= T^2+T-6'),
        ('alg_rw_gen_8', '9\nx^2+3*x+1\n2*x+3\n\n', '= (1/4)*T^2-5/4'),
        ('alg_rw_gen_9', '9\n(x^2-1)/(x-1)\nx+1\n\n', '= T'),
        ('alg_rw_gen_10', '9\nexp(log(x+5))\nx+5\n\n', '= T'),
    ]


print('\n========== GENERATED ALGEBRA COMPARE AUDIT ==========')
for label, user_input, needle in compare_cases():
    out = run_cli(user_input)
    report(label, needle in out and 'Simplify both' in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA TRANSFORM AUDIT ==========')
for label, user_input, needle in transform_cases():
    out = run_cli(user_input)
    report(label, needle in out and 'Final =' in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA EXPAND AUDIT ==========')
for label, expr, expected in expand_cases():
    out = run_cli(f'3\n{expr}\n\n')
    ok = 'Binomial expansion' in out and ('Out = ' + expected) in out
    report(label, ok, f'expected Out = {expected}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA POLY AUDIT ==========')
for label, expr, needles in factor_cases():
    out = run_cli(f'4\n{expr}\n')
    ok = 'Input =' in out
    i = 0
    while i < len(needles):
        if needles[i] not in out:
            ok = False
            break
        i += 1
    report(label, ok, f'missing factors {needles!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA COMPLETE-SQUARE AUDIT ==========')
for label, expr, expected in complete_square_cases():
    out = run_cli(f'5\n{expr}\n')
    ok = 'Complete square:' in out and ('Ans = ' + expected) in out
    report(label, ok, f'expected Ans = {expected}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA SOLVE AUDIT ==========')
for label, user_input, needles in solve_cases():
    out = run_cli(user_input)
    ok = 'Equation =' in out or 'x =' in out
    i = 0
    found = False
    while i < len(needles):
        if needles[i] in out:
            found = True
            break
        i += 1
    ok = ok and found
    report(label, ok, f'missing any of {needles!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA COMPOSE AUDIT ==========')
for label, user_input, needle in compose_cases():
    out = run_cli(user_input)
    report(label, needle in out and 'f(g(x)) =' in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA INVERSE AUDIT ==========')
for label, user_input, needle in inverse_cases():
    out = run_cli(user_input)
    report(label, needle in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n========== GENERATED ALGEBRA REWRITE AUDIT ==========')
for label, user_input, needle in rewrite_cases():
    out = run_cli(user_input)
    report(label, needle in out, f'missing {needle!r}\n--- OUT ---\n{out}')

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed')
print('=' * 60)
if FAIL > 0:
    print('Generated algebra stress cases exposed algebra or working-style gaps.')
