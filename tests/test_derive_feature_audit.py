import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / 'src' / 'Math'
sys.path.insert(0, str(ROOT))
sys._derive_no_autorun = True

import deriveProgram as dp

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


def warn(label, detail=''):
    global WARN
    WARN += 1
    print('  WARN:', label, detail)


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


def run_normal_case(label, expr_text, points):
    try:
        expr = dp.parse(expr_text)
        deriv = dp.diff(expr, 'x', {})
        deriv = dp.tidy(deriv)
        ok = True
        detail = ''
        i = 0
        while i < len(points):
            x = points[i]
            got = eval_ast(deriv, {'x': x})
            want = finite_diff(expr, 'x', x)
            if not math.isfinite(got) or not math.isfinite(want) or abs(got - want) > 1e-4 * (1.0 + abs(got) + abs(want)):
                ok = False
                detail = f'x={x} got={got} want={want} d={dp.show(deriv)}'
                break
            i += 1
        report(label, ok, detail)
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


def run_param_case(label, xt_text, yt_text, points):
    try:
        xt = dp.parse(xt_text)
        yt = dp.parse(yt_text)
        dx = dp.tidy(dp.diff(xt, 't', []))
        dy = dp.tidy(dp.diff(yt, 't', []))
        ans = dp.tidy(dp.div(dy, dx))
        ok = True
        detail = ''
        i = 0
        while i < len(points):
            t = points[i]
            got = eval_ast(ans, {'t': t})
            want = finite_diff(yt, 't', t) / finite_diff(xt, 't', t)
            if not math.isfinite(got) or not math.isfinite(want) or abs(got - want) > 1e-4 * (1.0 + abs(got) + abs(want)):
                ok = False
                detail = f't={t} got={got} want={want} ans={dp.show(ans)}'
                break
            i += 1
        report(label, ok, detail)
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


def run_implicit_case(label, equation_text, expected_fragment):
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
        want = dp.parse(expected_fragment)
        samples = [
            {'x': -1.3, 'y': -0.9},
            {'x': -0.4, 'y': 0.8},
            {'x': 0.7, 'y': 1.4},
            {'x': 1.6, 'y': -1.1},
        ]
        ok = True
        valid = 0
        detail = dp.show(ans)
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
                detail = f'got {dp.show(ans)} expected {expected_fragment}'
                break
            i += 1
        if valid == 0:
            ok = False
            detail = f'no valid samples for {dp.show(ans)}'
        report(label, ok, detail)
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')


print('\n========== DERIVE NORMAL MODE AUDIT ==========' )
normal_cases = [
    ('normal_poly_1', 'x^7-3*x^3+5*x-8', [-2.1, -0.6, 0.8, 1.7]),
    ('normal_trig_prod_2', 'sin(x)^3*cos(x)', [-1.2, -0.2, 0.4, 1.1]),
    ('normal_exp_3', 'exp(3*x-2)', [-1.0, 0.0, 0.9, 1.6]),
    ('normal_log_4', 'log(x^2+3*x+5)', [-1.2, -0.3, 0.5, 1.4]),
    ('normal_sqrt_5', 'sqrt(2*x+9)', [-2.0, -1.0, 0.5, 3.0]),
    ('normal_atan_6', 'atan(3*x-1)', [-1.1, -0.4, 0.7, 1.3]),
    ('normal_asin_7', 'asin(x/3)', [-2.0, -1.0, 0.5, 2.0]),
    ('normal_acos_8', 'acos((x+1)/4)', [-2.0, -1.2, 0.0, 2.0]),
    ('normal_frac_9', '(x^2+1)/(x+3)', [-2.5, -1.5, 0.0, 2.0]),
    ('normal_x_pow_x_10', 'x^x', [0.4, 0.8, 1.3, 2.0]),
    ('normal_sin_pow_x_11', '(sin(x)+2)^x', [0.2, 0.7, 1.1, 1.8]),
    ('normal_x_pow_sin_12', 'x^(sin(x)+2)', [0.7, 1.2, 1.8, 2.4]),
    ('normal_mix_13', 'exp(x)*cos(x)+x^3/(x^2+1)', [-1.1, -0.4, 0.6, 1.5]),
    ('normal_log10_14', 'log10(x^2+4*x+8)', [-1.5, -0.5, 0.8, 1.7]),
    ('normal_hyperbolic_15', 'sinh(x)+cosh(2*x)-tanh(x)', [-1.0, -0.3, 0.4, 1.1]),
]
for case in normal_cases:
    run_normal_case(*case)

print('\n========== DERIVE IMPLICIT MODE AUDIT ==========' )
implicit_cases = [
    ('implicit_circle_1', 'x^2+y^2=25', '(-x)/y'),
    ('implicit_xy_2', 'x*y=12', '(-y)/x'),
    ('implicit_parabola_3', 'y^2=x+1', '1/(2*y)'),
    ('implicit_exp_4', 'x+exp(y)=7', '(-1)/exp(y)'),
    ('implicit_log_5', 'x+log(y)=0', '-y'),
    ('implicit_sin_6', 'sin(y)+x=0', '(-1)/cos(y)'),
    ('implicit_cos_7', 'cos(y)=x', '(-1)/sin(y)'),
    ('implicit_tan_8', 'tan(y)=x^2+1', '(2*x)/(sec(y)^2)'),
    ('implicit_mix_9', 'x^2+x*y+y^2=7', '(-2*x-y)/(x+2*y)'),
    ('implicit_shift_10', '(x+1)^2+(y-2)^2=10', '(-(x+1))/(y-2)'),
    ('implicit_linear_11', '3*x+4*y=9', '-3/4'),
    ('implicit_poly_12', 'x^3+y^3=1', '-x^2/y^2'),
    ('implicit_frac_13', 'y/(x+1)=3', '3'),
    ('implicit_sqrt_14', 'sqrt(y)+x=5', '-2*sqrt(y)'),
    ('implicit_hyper_15', 'x+sinh(y)=0', '(-1)/cosh(y)'),
]
for case in implicit_cases:
    run_implicit_case(*case)

print('\n========== DERIVE PARAMETRIC MODE AUDIT ==========' )
param_cases = [
    ('param_poly_1', 't^3-3*t', 't^4+t', [-1.3, -0.6, 0.7, 1.4]),
    ('param_exp_trig_2', 'exp(t)', 'sin(t)', [-1.0, -0.3, 0.4, 1.1]),
    ('param_rational_3', '(t^2+1)/(t+3)', 't^2-t', [-1.5, -0.2, 0.7, 1.6]),
    ('param_sqrt_4', 'sqrt(t+5)', 't^2+1', [-3.5, -1.5, 0.0, 2.0]),
    ('param_mix_5', 'sin(t)+t^2', 'cos(t)+exp(t)', [-1.1, -0.4, 0.6, 1.3]),
    ('param_log_6', 'log(t+4)', 't^3-2*t', [-2.5, -1.0, 0.3, 1.8]),
    ('param_trig_7', 'tan(t)', 'sec(t)', [-0.8, -0.4, 0.2, 0.7]),
    ('param_poly_8', '2*t^2+3*t+1', 't^5-t', [-1.1, -0.6, 0.8, 1.2]),
    ('param_hyper_9', 'sinh(t)', 'cosh(t)', [-1.0, -0.2, 0.3, 1.0]),
    ('param_abs_10', 't^2+1', 'abs(t)+t^3', [-2.0, -0.8, 0.9, 1.7]),
    ('param_frac_11', '1/(t+4)', '(t+1)/(t+2)', [-2.5, -1.2, 0.1, 1.1]),
    ('param_pow_12', 't^4+1', '(t+2)^3', [-1.4, -0.5, 0.5, 1.4]),
    ('param_sincos_13', 'sin(2*t)', 'cos(3*t)', [-0.7, -0.2, 0.3, 0.8]),
    ('param_exp_14', 'exp(2*t)', 'exp(t)+t', [-1.0, -0.2, 0.6, 1.0]),
    ('param_combo_15', 't^2+sin(t)+3', 't^3+cos(t)+1', [-1.0, -0.3, 0.5, 1.1]),
]
for case in param_cases:
    run_param_case(*case)

print('\n' + '=' * 60)
print(f'RESULTS: {PASS} passed, {FAIL} failed, {WARN} warnings')
print('=' * 60)
if FAIL > 0:
    print('Failures indicate derivative weaknesses or unsupported cases worth investigation.')
