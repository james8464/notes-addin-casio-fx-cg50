import math
import sys
from pathlib import Path

ROOT = Path('/Users/james/Developer/Python/CASIO/src/Math')
sys.path.insert(0, str(ROOT))
sys._int_no_autorun = True

import intProgram as ip

PASS = 0
FAIL = 0


def report(label, passed, detail=''):
    global PASS, FAIL
    if passed:
        PASS += 1
        print('  PASS:', label)
    else:
        FAIL += 1
        print('  FAIL:', label, detail)


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
        table = {
            'sin': math.sin,
            'cos': math.cos,
            'tan': math.tan,
            'sec': lambda x: 1.0 / math.cos(x),
            'cosec': lambda x: 1.0 / math.sin(x),
            'cot': lambda x: math.cos(x) / math.sin(x),
            'asin': math.asin,
            'acos': math.acos,
            'atan': math.atan,
            'exp': math.exp,
            'log': math.log,
            'sqrt': math.sqrt,
            'abs': abs,
            'sinh': math.sinh,
            'cosh': math.cosh,
            'tanh': math.tanh,
        }
        return table[name](arg)
    raise ValueError(kind)


def sample_points(base):
    pts = [base]
    if base != 0:
        pts.append(base * 0.8)
        pts.append(base * 1.2)
    else:
        pts.append(0.35)
        pts.append(0.6)
    out = []
    i = 0
    while i < len(pts):
        duplicate = False
        j = 0
        while j < len(out):
            if abs(out[j] - pts[i]) < 1e-9:
                duplicate = True
                break
            j += 1
        if not duplicate:
            out.append(pts[i])
        i += 1
    return out


def finite_diff(node, var, env, h=1e-6):
    left = dict(env)
    right = dict(env)
    left[var] = env[var] - h
    right[var] = env[var] + h
    return (eval_ast(node, right) - eval_ast(node, left)) / (2.0 * h)


def working_quality(title, lines):
    if lines is None or len(lines) == 0:
        return False, 'no working lines'
    bad_markers = ('Attempt ', 'Choose the simplest successful attempt.')
    i = 0
    while i < len(lines):
        j = 0
        while j < len(bad_markers):
            if bad_markers[j] in lines[i]:
                return False, 'debug-style working'
            j += 1
        i += 1
    if title in ('std', 'f(ax+b)'):
        ok = False
        i = 0
        while i < len(lines):
            if 'standard' in lines[i].lower() or 'consider y =' in lines[i].lower() or 'integrate each term separately' in lines[i].lower() or 'complete the square' in lines[i].lower():
                ok = True
                break
            i += 1
        return ok, 'direct integration lines too thin'
    if title in ('Reverse chain rule', 'Integration by substitution'):
        has_u = False
        has_du = False
        i = 0
        while i < len(lines):
            if 'u =' in lines[i]:
                has_u = True
            if 'du/dx =' in lines[i]:
                has_du = True
            i += 1
        return has_u and has_du, 'substitution working incomplete'
    if title == 'Integration by parts':
        has_u = False
        has_v = False
        has_i = False
        i = 0
        while i < len(lines):
            if 'u =' in lines[i]:
                has_u = True
            if 'v =' in lines[i]:
                has_v = True
            if 'I =' in lines[i]:
                has_i = True
            i += 1
        return has_u and has_v and has_i, 'parts working incomplete'
    if title == 'Using trigonometric identities':
        has_use = False
        has_int = False
        i = 0
        while i < len(lines):
            low = lines[i].lower()
            if 'use ' in low or 'rationalise the denominator' in low:
                has_use = True
            if 'Int[' in lines[i] or 'Integrate each term separately' in lines[i]:
                has_int = True
            i += 1
        return has_use and has_int, 'trig working incomplete'
    if title == 'Partial fractions':
        i = 0
        while i < len(lines):
            low = lines[i].lower()
            if 'partial fractions' in low or 'divide the numerator' in low:
                return True, ''
            i += 1
        return False, 'partial fractions working incomplete'
    if title == 'Polynomial division':
        i = 0
        while i < len(lines):
            if 'Divide the numerator' in lines[i]:
                return True, ''
            i += 1
        return False, 'division working incomplete'
    return True, ''


QUESTIONS = [
    (1, 'x^2*e^(3*x)', 0.4, {}),
    (2, 'x*sin(2*x)', 0.7, {}),
    (3, 'x/(x^2+1)^2', 0.6, {}),
    (4, '1/(x^2+x)', 1.6, {}),
    (5, 'log(x^2+1)', 0.8, {}),
    (6, 'sin(x)^3*cos(x)^2', 0.7, {}),
    (7, 'e^x/(e^(2*x)+1)', 0.3, {}),
    (8, 'log(x)/x^2', 1.7, {}),
    (9, '1/(x*sqrt(x^2-1))', 1.6, {}),
    (10, 'x*e^(-x^2)', 0.9, {}),
    (11, 'tan(x)^3*sec(x)', 0.4, {}),
    (12, '2*x^3/(x^2-4)', 3.1, {}),
    (13, 'sqrt(1-x^2)', 0.3, {}),
    (14, 'x*sqrt(4-x^2)', 1.0, {}),
    (15, 'log(x)^2', 1.6, {}),
    (16, 'log(x)/x^3', 1.5, {}),
    (17, 'sec(x)^4', 0.2, {}),
    (18, '(x^2+1)/(x^3+3*x)', 1.2, {}),
    (19, 'e^(2*x)*sin(x)', 0.4, {}),
    (20, 'x/sqrt(x+1)', 0.6, {}),
    (21, '1/(x^2-6*x+9)', 0.5, {}),
    (22, 'sin(x)^2*cos(x)^3', 0.5, {}),
    (23, 'x*sec(x)^2', 0.4, {}),
    (24, '1/(x*log(x))', 2.5, {}),
    (25, '(x+2)/(x^2+4*x+5)', 0.4, {}),
    (26, 'e^(sqrt(x))/sqrt(x)', 2.0, {}),
    (27, 'cos(sqrt(x))', 1.4, {}),
    (28, 'sin(x)/(1+cos(x)^2)', 0.7, {}),
    (29, 'x^3*e^(x^2)', 0.6, {}),
    (30, '1/sqrt(1-x^2)', 0.4, {}),
    (31, '4*x^3/(x^4+1)', 0.6, {}),
    (32, 'x*cos(x)', 0.8, {}),
    (33, 'x^2/sqrt(9-x^2)', 1.5, {}),
    (34, '1/(x^2+x-2)', 2.5, {}),
    (35, 'atan(x)', 0.7, {}),
    (36, '1/(x+1)^3', 0.6, {}),
    (37, 'x^5*sqrt(x^3+1)', 1.0, {}),
    (38, 'e^(sin(x))*cos(x)', 0.4, {}),
    (39, '1/(x^2*sqrt(x^2+4))', 1.3, {}),
    (40, 'cos(log(x))', 1.7, {}),
    (41, 'sqrt(x)/(x-1)', 1.4, {}),
    (42, 'e^(2*x)', 0.3, {}),
    (43, 'sec(x)^2/(tan(x)^2+4)', 0.4, {}),
    (44, 'x^3/(x^2+1)', 0.6, {}),
    (45, 'asin(x)', 0.2, {}),
    (46, 'log(x)/x', 2.0, {}),
    (47, 'x*tan(x)^2', 0.3, {}),
    (48, '1/(x*(log(x))^2)', 3.0, {}),
    (49, '1/sqrt(x*(1+x))', 0.8, {}),
    (50, 'tan(x)^5*sec(x)^2', 0.4, {}),
    (51, 'x^3/(x^4+1)', 0.5, {}),
    (52, 'x*asin(x)', 0.3, {}),
    (53, 'e^(x^(1/3))', 1.5, {}),
    (54, 'x/(x^2+1)^2', 0.7, {}),
    (55, 'sin(x)*cos(x)/(1-sin(x)^2)', 0.4, {}),
    (56, '(x^2-3)/x^3', 1.7, {}),
    (57, '1/(x^3-1)', 1.7, {}),
    (58, 'sin(2*x)*cos(x)', 0.4, {}),
    (59, 'log(x^2+2*x+2)', 0.5, {}),
    (60, '1/(x^2+2*x+5)', 0.4, {}),
    (61, '(x+1)/sqrt(4-x^2)', 0.6, {}),
    (62, 'e^x/(e^x+1)', 0.3, {}),
    (63, 'cos(x)^4', 0.6, {}),
    (64, 'sqrt(x)/(x+1)', 0.9, {}),
    (65, 'x^2*log(x)', 1.7, {}),
    (66, '1/(x^2-1)', 2.1, {}),
    (67, '(2*x^2+1)/(x^2*(x^2+1))', 1.3, {}),
    (68, 'sin(x)/cos(x)^2', 0.4, {}),
    (69, 'x^3*e^(2*x)', 0.4, {}),
    (70, 'e^(sqrt(x))', 1.3, {}),
    (71, 'log(log(x))/x', 3.0, {}),
    (72, '1/((x-1)^(2/3))', 2.3, {}),
    (73, 'sin(log(x))', 1.5, {}),
    (74, 'cos(x)/(1+sin(x)^2)', 0.3, {}),
    (75, 'sqrt(x^2-9)/x', 4.0, {}),
    (76, 'x*sqrt(x-1)', 1.5, {}),
    (77, '1/(e^x+e^(-x))', 0.5, {}),
    (78, 'x*e^(-3*x)', 1.2, {}),
    (79, 'x^4/(x^2+1)', 0.5, {}),
    (80, 'x*sec(x)^2', 0.3, {}),
    (81, 'e^x*sqrt(1-e^(2*x))', -0.5, {}),
    (82, 'x^2*log(x)', 1.6, {}),
    (83, 'cos(x)^3/sin(x)^4', 0.8, {}),
    (84, 'x^2*e^x', -0.3, {}),
    (85, 'log(x+sqrt(x^2-1))', 2.0, {}),
    (86, 'x^2/(1+x^2)^2', 0.6, {}),
    (87, 'e^x/(1-e^(2*x))', -0.5, {}),
    (88, 'sin(x)^3', 0.7, {}),
    (89, 'x*asin(x^2)', 0.5, {}),
    (90, 'atan(x)/(1+x^2)', 0.6, {}),
    (91, '1/(x*sqrt(x+1))', 1.4, {}),
    (92, '1/(x*(x^2+1))', 1.3, {}),
    (93, 'sec(x)^3', 0.4, {}),
    (94, '1/(1+cos(x))', 0.5, {}),
    (95, 'x^3/sqrt(x^2+1)', 0.8, {}),
    (96, 'x/sqrt(4-x^2)', 0.7, {}),
    (97, 'x*log(x+1)', 0.8, {}),
    (98, 'e^(2*x)*sqrt(e^x+1)', 0.2, {}),
    (99, 'sin(2*x)/(1+cos(x))', 0.7, {}),
    (100, '1/(x^3+x)', 1.4, {}),
]


print('\n========== 100 SCREENSHOT INTEGRATION AUDIT ==========')
correct_count = 0
quality_count = 0
unsupported = []

for number, expr_text, base_sample, extra_env in QUESTIONS:
    label = f'q{number}'
    if hasattr(ip, 'begin_user_action'):
        ip.begin_user_action()
    try:
        node, var = ip.parse_input(expr_text)
        title, ans, lines, reason = ip.solve_result_or_reason(node, var, '1')
        if ans is None:
            unsupported.append((number, expr_text, reason))
            report(label, False, f'unsupported: {reason}')
            continue

        ok = True
        detail = ''
        valid = 0
        pts = sample_points(base_sample)
        i = 0
        while i < len(pts):
            env = {var: pts[i]}
            for key, value in extra_env.items():
                env[key] = value
            try:
                got = finite_diff(ans, var, env)
                want = eval_ast(node, env)
            except Exception:
                i += 1
                continue
            valid += 1
            if not math.isfinite(got) or not math.isfinite(want) or abs(got - want) > 1e-5 * (1.0 + abs(got) + abs(want)):
                ok = False
                detail = 'numerical derivative does not match integrand'
                break
            i += 1
        if valid == 0:
            ok = False
            detail = 'no valid numeric sample'

        quality_ok = False
        quality_detail = ''
        if ok:
            quality_ok, quality_detail = working_quality(title, lines)
            if quality_ok:
                correct_count += 1
                quality_count += 1
                report(label, True)
            else:
                correct_count += 1
                report(label, False, 'working: ' + quality_detail + ' | title=' + title + ' | lines=' + repr(lines))
        else:
            report(label, False, detail + ' | title=' + title + ' | ans=' + ip.pretty(ans))
    except Exception as err:
        report(label, False, f'CRASH {type(err).__name__}: {err}')

print('\n' + '=' * 60)
print(f'Correct antiderivatives: {correct_count}/100')
print(f'Full-mark working standard: {quality_count}/100')
print(f'Total failures: {FAIL}')
print('=' * 60)
if unsupported:
    print('Unsupported / out-of-scope cases:')
    i = 0
    while i < len(unsupported):
        number, expr_text, reason = unsupported[i]
        print(f'  {number}. {expr_text} -> {reason}')
        i += 1
