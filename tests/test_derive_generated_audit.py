import math
import random
import subprocess
import sys
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/Math')
PY = '/Users/james/Developer/Python/CASIO/.venv/bin/python'
sys.path.insert(0, str(ROOT))
sys._derive_no_autorun = True

import deriveProgram as dp

PASS = 0
FAIL = 0

RNG = random.Random(54017)


def report(label, passed, detail=''):
    global PASS, FAIL
    if passed:
        PASS += 1
        print('  PASS:', label)
    else:
        FAIL += 1
        print('  FAIL:', label, detail)


def run_cli(user_input):
    proc = subprocess.run([PY, str(ROOT / 'deriveProgram.py')], input=user_input, text=True, capture_output=True, check=False)
    return proc.stdout


def eval_ast(node, env):
    kind = node[0]
    if kind == 'num':
        return node[1] / node[2]
    if kind == 'sym':
        return env[node[1]]
    if kind == 'const':
        if node[1] == 'pi':
            return math.pi
        if node[1] == 'e':
            return math.e
        raise ValueError(node)
    if kind == 'add':
        total = 0.0
        i = 0
        while i < len(node[1]):
            total += eval_ast(node[1][i], env)
            i += 1
        return total
    if kind == 'mul':
        total = 1.0
        i = 0
        while i < len(node[1]):
            total *= eval_ast(node[1][i], env)
            i += 1
        return total
    if kind == 'div':
        return eval_ast(node[1], env) / eval_ast(node[2], env)
    if kind == 'pow':
        return eval_ast(node[1], env) ** eval_ast(node[2], env)
    if kind == 'fn':
        name = node[1]
        arg = eval_ast(node[2], env)
        if name == 'sin':
            return math.sin(arg)
        if name == 'cos':
            return math.cos(arg)
        if name == 'tan':
            return math.tan(arg)
        if name == 'sec':
            return 1.0 / math.cos(arg)
        if name == 'cosec':
            return 1.0 / math.sin(arg)
        if name == 'cot':
            return math.cos(arg) / math.sin(arg)
        if name == 'asin':
            return math.asin(arg)
        if name == 'acos':
            return math.acos(arg)
        if name == 'atan':
            return math.atan(arg)
        if name == 'log':
            return math.log(arg)
        if name == 'log10':
            return math.log10(arg)
        if name == 'sqrt':
            return math.sqrt(arg)
        if name == 'exp':
            return math.exp(arg)
        if name == 'abs':
            return abs(arg)
        if name == 'sinh':
            return math.sinh(arg)
        if name == 'cosh':
            return math.cosh(arg)
        if name == 'tanh':
            return math.tanh(arg)
        raise ValueError(name)
    raise ValueError(node)


def finite_diff(expr, var, x, h=1e-6):
    return (eval_ast(expr, {var: x + h}) - eval_ast(expr, {var: x - h})) / (2.0 * h)


def normal_points():
    return [-1.1, -0.4, 0.6, 1.3]


def param_points():
    return [-1.0, -0.4, 0.5, 1.1]


def pick_nonzero(low, high):
    while True:
        value = RNG.randint(low, high)
        if value != 0:
            return value


def build_normal_cases():
    cases = []
    i = 0
    while i < 10:
        if i % 3 == 0:
            a = pick_nonzero(2, 5)
            b = pick_nonzero(-5, 5)
            n = RNG.randint(2, 4)
            k = pick_nonzero(2, 4)
            c = RNG.randint(-3, 3)
            expr = f'({a}*x^{n}+{b})*sin({k}*x+{c})'
            marker = 'Product rule'
        elif i % 3 == 1:
            a = pick_nonzero(1, 4)
            b = pick_nonzero(-5, 5)
            n = RNG.randint(2, 4)
            c = pick_nonzero(1, 4)
            d = RNG.randint(3, 7)
            expr = f'({a}*x^{n}+{b})/({c}*x+{d})'
            marker = 'Quotient rule'
        else:
            a = pick_nonzero(2, 4)
            b = RNG.randint(-3, 3)
            n = RNG.randint(2, 4)
            func = ['sin', 'cos', 'exp'][i % 3]
            if func == 'exp':
                expr = f'exp({a}*x+{b})'
            else:
                expr = f'{func}(({a}*x+{b})^{n})'
            marker = 'Chain rule'
        cases.append((f'derive_normal_gen_{i + 1}', expr, marker))
        i += 1
    return cases


def build_implicit_cases():
    return [
        ('derive_implicit_gen_1', 'x^2+y^2=29', '(-x)/y'),
        ('derive_implicit_gen_2', 'x*y=18', '(-y)/x'),
        ('derive_implicit_gen_3', 'y^3=2*x+7', '2/(3*y^2)'),
        ('derive_implicit_gen_4', 'x+exp(y)=11', '(-1)/exp(y)'),
        ('derive_implicit_gen_5', 'tan(y)=3*x-2', '3/sec(y)^2'),
        ('derive_implicit_gen_6', 'sqrt(y)+2*x=9', '-4*sqrt(y)'),
        ('derive_implicit_gen_7', 'x^2+3*x*y+y^2=14', '(-2*x-3*y)/(3*x+2*y)'),
        ('derive_implicit_gen_8', '(x-2)^2+(y+1)^2=13', '(-(x-2))/(y+1)'),
        ('derive_implicit_gen_9', 'log(y)+4*x=1', '-4*y'),
        ('derive_implicit_gen_10', 'sin(y)=2*x+1', '2/cos(y)'),
    ]


def build_param_cases():
    cases = []
    i = 0
    while i < 10:
        if i % 2 == 0:
            a = pick_nonzero(2, 4)
            b = pick_nonzero(-4, 4)
            xt = f'{a}*t^3+{b}*t'
            yt = f't^4+{a}*t^2+{abs(b) + 1}'
        else:
            a = pick_nonzero(2, 4)
            b = RNG.randint(-2, 2)
            xt = f'exp(t)+{a}*sin(t)'
            yt = f'cos(t)+({a}*t+{b})^3'
        cases.append((f'derive_param_gen_{i + 1}', xt, yt))
        i += 1
    return cases


def run_normal_case(label, expr_text, marker):
    try:
        expr = dp.parse(expr_text)
        deriv = dp.tidy(dp.diff(expr, 'x', {}))
        ok = True
        detail = ''
        for x in normal_points():
            got = eval_ast(deriv, {'x': x})
            want = finite_diff(expr, 'x', x)
            if not math.isfinite(got) or not math.isfinite(want) or abs(got - want) > 1e-4 * (1.0 + abs(got) + abs(want)):
                ok = False
                detail = f'x={x} got={got} want={want} d={dp.show(deriv)}'
                break
        out = run_cli(f'1\n{expr_text}\n')
        if marker not in out or 'dy/dx =' not in out:
            ok = False
            detail = f'missing working marker {marker!r}\n--- OUT ---\n{out}'
        report(label, ok, detail)
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


def run_implicit_case(label, equation_text, expected_text):
    try:
        left_text, right_text = equation_text.split('=', 1)
        left = dp.trig_normal(dp.parse(left_text))
        right = dp.trig_normal(dp.parse(right_text))
        var, dep = dp.pick_implicit_vars(left, right)
        dname = 'd' + dep + '/d' + var
        start = dp.tidy(dp.add([left, dp.neg(right)]))
        cleared, _ = dp.as_rat(start)
        whole = dp.sim(dp.diff(dp.tidy(cleared), var, [dep]))
        coef, rest = dp.coeff_d(whole, dname)
        ans = dp.prefer_trig_recip(dp.tidy(dp.div(dp.neg(rest), coef)))
        want = dp.parse(expected_text)
        samples = [
            {'x': -1.2, 'y': -0.8},
            {'x': -0.4, 'y': 0.9},
            {'x': 0.6, 'y': 1.3},
            {'x': 1.4, 'y': -1.1},
        ]
        ok = True
        detail = dp.show(ans)
        valid = 0
        i = 0
        while i < len(samples):
            try:
                got = eval_ast(ans, samples[i])
                target = eval_ast(want, samples[i])
            except Exception:
                i += 1
                continue
            valid += 1
            if not math.isfinite(got) or not math.isfinite(target) or abs(got - target) > 1e-6 * (1.0 + abs(got) + abs(target)):
                ok = False
                detail = f'got {dp.show(ans)} expected {expected_text}'
                break
            i += 1
        out = run_cli(f'2\n{equation_text}\n')
        if 'Make dy/dx' not in out or 'dy/dx =' not in out:
            ok = False
            detail = f'weak implicit working\n--- OUT ---\n{out}'
        if valid == 0:
            ok = False
            detail = 'no valid samples'
        report(label, ok, detail)
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


def run_param_case(label, xt_text, yt_text):
    try:
        xt = dp.parse(xt_text)
        yt = dp.parse(yt_text)
        dx = dp.tidy(dp.diff(xt, 't', []))
        dy = dp.tidy(dp.diff(yt, 't', []))
        ans = dp.tidy(dp.div(dy, dx))
        ok = True
        detail = ''
        for t in param_points():
            got = eval_ast(ans, {'t': t})
            want = finite_diff(yt, 't', t) / finite_diff(xt, 't', t)
            if not math.isfinite(got) or not math.isfinite(want) or abs(got - want) > 1e-4 * (1.0 + abs(got) + abs(want)):
                ok = False
                detail = f't={t} got={got} want={want} ans={dp.show(ans)}'
                break
        out = run_cli(f'3\n{xt_text}\n{yt_text}\n')
        if 'dx/dt =' not in out or 'dy/dt =' not in out or 'dy/dx =' not in out:
            ok = False
            detail = f'weak parametric working\n--- OUT ---\n{out}'
        report(label, ok, detail)
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


print('\n========== GENERATED DERIVE NORMAL AUDIT ==========')
for case in build_normal_cases():
    run_normal_case(*case)

print('\n========== GENERATED DERIVE IMPLICIT AUDIT ==========')
for case in build_implicit_cases():
    run_implicit_case(*case)

print('\n========== GENERATED DERIVE PARAMETRIC AUDIT ==========')
for case in build_param_cases():
    run_param_case(*case)

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed')
print('=' * 60)
if FAIL > 0:
    print('Generated derivative stress cases exposed differentiation or working-style gaps.')
