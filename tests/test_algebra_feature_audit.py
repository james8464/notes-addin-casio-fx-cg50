import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'src' / 'Math'
PY = '/Users/james/Developer/Python/CASIO/.venv/bin/python'

PASS = 0
FAIL = 0
WARN = 0


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
    return proc.returncode, proc.stdout, proc.stderr


def expect_contains(label, out, needle):
    report(label, needle in out, f'missing {needle!r}\n--- OUT ---\n{out}')


def expect_any(label, out, needles):
    ok = False
    i = 0
    while i < len(needles):
        if needles[i] in out:
            ok = True
            break
        i += 1
    report(label, ok, f'missing any of {needles!r}\n--- OUT ---\n{out}')


print('\n========== ALGEBRA COMPARE AUDIT ==========' )
compare_cases = [
    ('alg_cmp_1', '1\n(x+1)^2\nx^2+2*x+1\n', 'Equivalent'),
    ('alg_cmp_2', '1\nsin(x)^2+cos(x)^2\n1\n', 'Equivalent'),
    ('alg_cmp_3', '1\n(1-cos(2*x))/2\nsin(x)^2\n', 'Equivalent'),
    ('alg_cmp_4', '1\n(x^2-1)/(x-1)\nx+1\n', 'Equivalent'),
    ('alg_cmp_5', '1\nexp(log(x))\nx\n', 'Equivalent'),
    ('alg_cmp_6', '1\nlog(e^x)\nx\n', 'Equivalent'),
    ('alg_cmp_7', '1\n(x+2)*(x+3)\nx^2+5*x+6\n', 'Equivalent'),
    ('alg_cmp_8', '1\n(x-1)*(x+1)\nx^2-1\n', 'Equivalent'),
    ('alg_cmp_9', '1\n(1+cos(2*x))/2\ncos(x)^2\n', 'Equivalent'),
    ('alg_cmp_10', '1\n(sec(x))^2-(tan(x))^2\n1\n', 'Equivalent'),
    ('alg_cmp_11', '1\n(cosec(x))^2-(cot(x))^2\n1\n', 'Equivalent'),
    ('alg_cmp_12', '1\n(x+1)^3\nx^3+3*x^2+3*x+1\n', 'Equivalent'),
    ('alg_cmp_13', '1\n(x+1)^2\nx^2+1\n', 'Not equivalent'),
    ('alg_cmp_14', '1\nsin(x)^2+cos(x)^2\n2\n', 'Not equivalent'),
    ('alg_cmp_15', '1\n(x^2+1)/(x-1)\n(x+1)\n', 'Not equivalent'),
]
for label, user_input, needle in compare_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n========== ALGEBRA TRANSFORM AUDIT ==========' )
transform_cases = [
    ('alg_xform_1', '2\n1-cos(2*x)\n2*sin(x)^2\n', 'Final = 2*(sin(x))^2'),
    ('alg_xform_2', '2\nsin(x)/cos(x)\ntan(x)\n', 'tan(x)'),
    ('alg_xform_3', '2\n1/cos(x)\nsec(x)\n', 'sec(x)'),
    ('alg_xform_4', '2\n1/sin(x)\ncosec(x)\n', 'cosec(x)'),
    ('alg_xform_5', '2\ncos(x)/sin(x)\ncot(x)\n', 'cot(x)'),
    ('alg_xform_6', '2\n(1-cos(x))/sin(x)\ntan(x/2)\n', 'tan(x/2)'),
    ('alg_xform_7', '2\nsin(x)/(1+cos(x))\ntan(x/2)\n', 'tan(x/2)'),
    ('alg_xform_8', '2\n(1+cos(x))/sin(x)\ncot(x/2)\n', 'cot(x/2)'),
    ('alg_xform_9', '2\nsec(x)^2-tan(x)^2\n1\n', '1'),
    ('alg_xform_10', '2\nsin(2*x)\n2*sin(x)*cos(x)\n', '2*sin(x)*cos(x)'),
    ('alg_xform_11', '2\ncos(2*x)\ncos(x)^2-sin(x)^2\n', '(cos(x))^2-(sin(x))^2'),
    ('alg_xform_12', '2\nexp(log(x))\nx\n', 'x'),
    ('alg_xform_13', '2\nlog(e^x)\nx\n', 'x'),
    ('alg_xform_14', '2\n(x+1)^2\nx^2+2*x+1\n', 'Final = x^2+2*x+1'),
    ('alg_xform_15', '2\n(x^2-1)/(x-1)\nx+1\n', 'x+1'),
]
for label, user_input, needle in transform_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n========== ALGEBRA POLY / FACTOR / EXPAND AUDIT ==========' )
poly_cases = [
    ('alg_poly_1', '4\nx^2+5*x+6\n', 'Factor quad'),
    ('alg_poly_2', '4\nx^2-1\n', 'difference of squares'),
    ('alg_poly_3', '4\n(x+1)^2\n', 'Out ='),
    ('alg_poly_4', '4\n(x+2)*(x+3)\n', 'Out ='),
    ('alg_poly_5', '4\nx^2-5*x+6\n', 'Factor quad'),
    ('alg_poly_6', '4\n2*x^2+7*x+3\n', 'Factor quad'),
    ('alg_poly_7', '4\n3*x^2+12*x+12\n', 'Factor quad'),
    ('alg_poly_8', '4\nx^4-1\n', 'difference of squares'),
    ('alg_poly_9', '4\nx^2+1\n', 'Out ='),
    ('alg_poly_10', '4\n(x-3)*(x+5)\n', 'Out ='),
    ('alg_poly_11', '4\n4*x^2-9\n', 'difference of squares'),
    ('alg_poly_12', '4\n9-x^2\n', 'difference of squares'),
    ('alg_poly_13', '4\n(x+1)^3\n', 'Out ='),
    ('alg_poly_14', '4\nx^2+2*x+1\n', 'Factor quad'),
    ('alg_poly_15', '4\n2*x^2+5*x+2\n', 'Factor quad'),
]
for label, user_input, needle in poly_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out.lower(), needle.lower())

print('\n========== ALGEBRA SOLVE AUDIT ==========' )
solve_cases = [
    ('alg_solve_1', '6\nx^2+5*x+6=0\n', 'x = [-3, -2]'),
    ('alg_solve_2', '6\n(x+1)/(x-2)=3\n', 'x = 7/2'),
    ('alg_solve_3', '6\n1/(x+1)+1/(x-2)=0\n', 'x = 1/2'),
    ('alg_solve_4', '6\n2*x+6=0\n', 'x = -3'),
    ('alg_solve_5', '6\nx^2-9=0\n', 'x = [3, -3]'),
    ('alg_solve_6', '6\n2*x^2+7*x+3=0\n', 'x = [-3, -1/2]'),
    ('alg_solve_7', '6\n3*x^2+12*x+12=0\n', 'x = -2'),
    ('alg_solve_8', '6\nx^2+1=0\n', 'Needs poly support'),
    ('alg_solve_9', '6\n0=0\n', 'All x'),
    ('alg_solve_10', '6\n5=0\n', 'No sol'),
    ('alg_solve_11', '6\n(x^2-1)/(x-1)=0\n', 'x = -1'),
    ('alg_solve_12', '6\n(x+2)*(x+3)=0\n', 'x = [-3, -2]'),
    ('alg_solve_13', '6\n4*x^2-9=0\n', 'x = [3/2, -3/2]'),
    ('alg_solve_14', '6\n9-x^2=0\n', 'x = [3, -3]'),
    ('alg_solve_15', '6\n2*x+1=0\n', 'x = -1/2'),
]
for label, user_input, needle in solve_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n========== ALGEBRA COMPOSE / INVERSE / REWRITE AUDIT ==========' )
feature_cases = [
    ('alg_comp_1', '7\n2*x+1\nx^2\n', 'f(g(x))'),
    ('alg_comp_2', '7\n(x^2+1)/(x-1)\n2*x+3\n', 'f(g(x))'),
    ('alg_comp_3', '7\nexp(x)\n3*x-2\n', 'f(g(x))'),
    ('alg_comp_4', '7\nlog(x)\n(x+5)^2\n', 'f(g(x))'),
    ('alg_comp_5', '7\nsqrt(x+1)\n4*x-3\n', 'f(g(x))'),
    ('alg_inv_6', '8\n(2*x+1)/(3*x+4)\n', 'f^-1(x) = (-4*x+1)/(3*x-2)'),
    ('alg_inv_7', '8\n1/(x+1)^3\n', 'f^-1(x) = 1/x^(1/3)-1'),
    ('alg_inv_8', '8\nsqrt((2*x+3)^3)\n', 'f^-1(x) = (x^(2/3)-3)/2'),
    ('alg_inv_9', '8\n3*x-7\n', 'f^-1(x) = (x+7)/3'),
    ('alg_inv_10', '8\nx^2+1\n', 'Unsupported inverse family'),
    ('alg_rw_11', '9\nx^2+5*x+6\n2*x+1\n\n', '= (1/4)*T^2+2*T+15/4'),
    ('alg_rw_12', '9\nx\n1/(x+3)+4\n\n', '= 1/(T-4)-3'),
    ('alg_rw_13', '9\n1-cos(2*x)\nsin(x)\n\n', 'Final = 2*(sin(x))^2'),
    ('alg_rw_14', '9\n1+cos(2*x)\ncos(x)\n\n', 'Final = 2*(cos(x))^2'),
    ('alg_rw_15', '9\nsin(x)^2+cos(x)^2\nsin(x)\ncos(x)\n\n', 'Final = 1'),
]
for label, user_input, needle in feature_cases:
    _code, out, _err = run_cli(user_input)
    expect_contains(label, out, needle)

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed, {WARN} warnings')
print('=' * 60)
if FAIL > 0:
    print('Failures indicate algebra weaknesses or unsupported transformations worth investigation.')
