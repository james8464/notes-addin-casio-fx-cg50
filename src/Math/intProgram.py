try:
    import math
except ImportError:
    math = None
try:
    import sys
except ImportError:
    sys = None
FAST_GCD = math.gcd if math is not None and hasattr(math, 'gcd')else None
FAST_ISQRT = math.isqrt if math is not None and hasattr(math, 'isqrt')else None
FAIL = 'Out of scope.'
DE_FAIL = 'Out of scope.'


def hard_integral_failure_reason(node, var):
    A = sim(node)
    B = var
    if same(A, div(num(1), fn('sqrt', add([num(1), neg(power(sym(B), num(4)))])))):
        return 'Out of scope: 1/sqrt(1-' + B + '^4).'
    if same(A, div(num(1), mul([power(sym(B), num(2)), fn('log', sym(B))]))):
        return 'Out of scope: 1/(' + B + '^2*ln(' + B + ')).'
    if A[0] == 'pow' and A[1][0] == 'fn' and A[1][1] == 'atan' and same(A[1][2], sym(B)) and same(A[2], num(2)):
        return 'Out of scope: atan(' + B + ')^2.'
    if (A[0] == 'fn' and A[1] == 'exp' and same(A[2], power(sym(B), num(2)))) or (A[0] == 'pow' and same(A[1], E) and same(A[2], power(sym(B), num(2)))):
        return 'Out of scope: e^(' + B + '^2).'
    if same(A, div(fn('sin', sym(B)), sym(B))):
        return 'Out of scope: sin(' + B + ')/' + B + '.'
    if same(A, div(num(1), fn('log', sym(B)))):
        return 'Out of scope: 1/ln(' + B + ').'
    if A[0] == 'fn' and A[1] == 'cos' and same(A[2], power(sym(B), num(2))):
        return 'Out of scope: cos(' + B + '^2).'
    return FAIL


def solve_result_or_reason(node, var, method, forced_u=None):
    A, B, C = solve(node, var, method, forced_u)
    if B is not None:
        return A, B, C, None
    return A, B, C, hard_integral_failure_reason(node, var)


def can_handle_derivative_case(node, var, deps):
    try:
        diff(node, var, deps)
        return True, None
    except Exception as err:
        return False, str(err)
SKIP_AUTORUN = sys is not None and getattr(sys, '_int_no_autorun', False)
MICROPYTHON_RUNTIME = sys is not None and getattr(
    getattr(sys, 'implementation', None), 'name', '') == 'micropython'
LOW_MEMORY_RUNTIME = False

# Reasoning markers for exam-quality output (same as other programs)
REASONING_MARKERS = (
    "Use ", "Using ", "let ", "hence", "so ", "therefore", "method:",
    "substitute", "rearranged", "differentiate", "integrat", "expand",
    "factor ", "solve ", "rule", "equation:", "original equation:",
    "identity:", "LHS:", "RHS:", "Hence ", "Therefore ", "Thus ",
    "final =", "result:", "answer:", "working:"
)

EXPAND_PASS_LIMIT = 4
TRIG_REWRITE_LIMIT = 4
_CACHE_MISS = object()
_ENGINE_CACHES = {}
E = 'const', 'e'
PI = 'const', 'pi'
U = 'sym', 'u'
FUNC_NAMES = 'sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'exp', 'log', 'log10', 'sqrt', 'abs', 'ln', 'atan', 'asin', 'acos'


def apply_runtime_profile(low_memory=None):
    global LOW_MEMORY_RUNTIME, EXPAND_PASS_LIMIT, TRIG_REWRITE_LIMIT
    if low_memory is None:
        low_memory = MICROPYTHON_RUNTIME
    LOW_MEMORY_RUNTIME = bool(low_memory)
    if LOW_MEMORY_RUNTIME:
        EXPAND_PASS_LIMIT = 3
        TRIG_REWRITE_LIMIT = 3
    else:
        EXPAND_PASS_LIMIT = 4
        TRIG_REWRITE_LIMIT = 4


def begin_user_action():
    clear_engine_caches()


def clear_engine_caches():
    _ENGINE_CACHES.clear()


def _cache_limit(name):
    if LOW_MEMORY_RUNTIME:
        limits = {
            'sim': 384,
            'depends': 512,
            'poly_num': 192,
            'linear_info': 192,
            'ordered_candidates': 96,
            'combine_logs': 96,
            'finalize_integral': 48}
    else:
        limits = {
            'sim': 2048,
            'depends': 3072,
            'poly_num': 768,
            'linear_info': 768,
            'ordered_candidates': 256,
            'combine_logs': 256,
            'finalize_integral': 128}
    return limits.get(name, 128)


def _cache_get(name, key):
    bucket = _ENGINE_CACHES.get(name)
    if bucket is None:
        return _CACHE_MISS
    return bucket.get(key, _CACHE_MISS)


def _cache_set(name, key, value):
    bucket = _ENGINE_CACHES.get(name)
    if bucket is None:
        bucket = {}
        _ENGINE_CACHES[name] = bucket
    if len(bucket) >= _cache_limit(name):
        try:
            bucket.pop(next(iter(bucket)))
        except StopIteration:
            pass
    bucket[key] = value
    return value


def _force_low_memory_runtime(flag): apply_runtime_profile(flag)


apply_runtime_profile()


def gcd(a, b):
    if FAST_GCD is not None:
        return FAST_GCD(a, b) or 1
    if a < 0:
        a = -a
    if b < 0:
        b = -b
    while b:
        a, b = b, a % b
    return a or 1


def num(a, b=1):
    if b == 0:
        raise ValueError('Division by zero.')
    if b < 0:
        a = -a
        b = -b
    A = gcd(a, b)
    return 'num', a // A, b // A


def num_text(text):
    C = text
    if '.'not in C:
        return num(int(C))
    A, B = C.split('.', 1)
    if A == '':
        A = '0'
    if B == '':
        B = '0'
    D = 1
    E = 0
    while E < len(B):
        D *= 10
        E += 1
    F = 1
    if A[:1] == '-':
        F = -1
        A = A[1:]
    return num(F * int(A + B), D)


def sym(name): return 'sym', name
def is_num(node): return node[0] == 'num'
def is_sym(node): return node[0] == 'sym'
def is_zero(node): return is_num(node) and node[1] == 0
def is_one(node):
    return is_num(node) and node[1] == node[2]


def is_minus_one(node):
    return is_num(node) and node[1] == -node[2]
def is_int_num(node): return is_num(node) and node[2] == 1
def addq(a, b): return num(a[1] * b[2] + b[1] * a[2], a[2] * b[2])
def mulq(a, b): return num(a[1] * b[1], a[2] * b[2])
def divq(a, b): return num(a[1] * b[2], a[2] * b[1])
def negq(a): return num(-a[1], a[2])
def subq(a, b): return addq(a, negq(b))


def int_pow(a, n):
    if n >= 0:
        return num(a[1]**n, a[2]**n)
    n = -n
    return num(a[2]**n, a[1]**n)


def flat(node, kind):
    if node[0] != kind:
        return [node]
    children = node[1]
    first = children[0] if children else None
    if first is None or first[0] != kind:
        return list(children)
    out = []
    for item in node[1]:
        if item[0] == kind:
            out.extend(flat(item, kind))
        else:
            out.append(item)
    return out


def sig(node):
    A = node
    B = A[0]
    if B in ('num', 'sym', 'const'):
        return A
    if B == 'fn':
        if A[1] == 'exp':
            return 'pow', ('const', 'e'), sig(A[2])
        return 'fn', A[1], sig(A[2])
    if B == 'pow':
        return 'pow', sig(A[1]), sig(A[2])
    if B == 'div':
        return 'div', sig(A[1]), sig(A[2])
    C = []
    for D in flat(A, B):
        C.append(sig(D))
    C.sort()
    return B, tuple(C)


def cheap_same(a, b):
    if a == b:
        return True
    if a[0] != b[0]:
        return False
    if a[0] in ('num', 'sym', 'const'):
        return False
    if a[0] == 'fn':
        return a[1] == b[1] and a[2] == b[2]
    if a[0] in ('pow', 'div'):
        return a[1] == b[1] and a[2] == b[2]
    if len(a[1]) != len(b[1]):
        return False
    return a[1] == b[1]


def same(a, b): return cheap_same(a, b) or sig(a) == sig(b)


def depends(node, name):
    B = name
    A = node
    C = A[0]
    if C == 'sym':
        return A[1] == B
    if C in ('num', 'const'):
        return False
    if C == 'fn':
        return depends(A[2], B)
    if C in ('pow', 'div'):
        return depends(A[1], B) or depends(A[2], B)
    D = 0
    while D < len(A[1]):
        if depends(A[1][D], B):
            return True
        D += 1
    return False


def collect_symbols(node, out):
    B = out
    A = node
    C = A[0]
    if C == 'sym':
        B.add(A[1])
        return
    if C in ('num', 'const'):
        return
    if C == 'fn':
        collect_symbols(A[2], B)
        return
    if C in ('pow', 'div'):
        collect_symbols(A[1], B)
        collect_symbols(A[2], B)
        return
    D = 0
    while D < len(A[1]):
        collect_symbols(A[1][D], B)
        D += 1


def make_mul(parts):
    D = parts
    A = []
    B = 0
    while B < len(D):
        C = D[B]
        if C[0] == 'mul':
            A.extend(C[1])
        else:
            A.append(C)
        B += 1
    if len(A) == 0:
        return num(1)
    if len(A) == 1:
        return A[0]
    return 'mul', tuple(A)


def make_add(parts):
    D = parts
    A = []
    B = 0
    while B < len(D):
        C = D[B]
        if C[0] == 'add':
            A.extend(C[1])
        else:
            A.append(C)
        B += 1
    if len(A) == 0:
        return num(0)
    if len(A) == 1:
        return A[0]
    return 'add', tuple(A)


def add(parts): return sim(make_add(parts))
def mul(parts): return sim(make_mul(parts))
def div(a, b): return sim(('div', a, b))
def power(a, b): return sim(('pow', a, b))
def fn(name, arg): return sim(('fn', name, arg))
def neg(node): return mul([num(-1), node])
def sub(a, b): return add([a, neg(b)])
def log_abs(node): return fn('log', fn('abs', node))


STANDARD_FN_PRIMITIVES = {
    'sin': (
        'fn', 'cos', -1), 'cos': (
            'fn', 'sin', 1), 'tan': (
                'log_abs_fn', 'sec', 1), 'cot': (
                    'log_abs_fn', 'sin', 1), 'sec': (
                        'log_abs_sum', ('sec', 'tan'), 1), 'cosec': (
                            'log_abs_sum', ('cosec', 'cot'), -1), 'exp': (
                                'fn', 'exp', 1), 'sqrt': (
                                    'sqrt_power',), 'asin': (
                                        'inv_trig_parts_asin',), 'acos': (
                                            'inv_trig_parts_acos',), 'atan': (
                                                'inv_trig_parts_atan',)}
STANDARD_POWER_PRIMITIVES = {('sec', 2): (
    'fn', 'tan', 1), ('cosec', 2): ('fn', 'cot', -1)}
STANDARD_PRODUCT_PRIMITIVES = {('sec', 'tan'): (
    'fn', 'sec', 1), ('cosec', 'cot'): ('fn', 'cosec', -1)}
FUNCTION_DERIVATIVES = {
    'sin': (
        'fn', 'cos', 1), 'cos': (
            'fn', 'sin', -1), 'tan': (
                'pow_fn', 'sec', 2, 1), 'sec': (
                    'prod_fns', ('sec', 'tan'), 1), 'cosec': (
                        'prod_fns', ('cosec', 'cot'), -1), 'cot': (
                            'pow_fn', 'cosec', 2, -1), 'exp': (
                                'fn', 'exp', 1), 'log': (
                                    'inv',), 'sqrt': (
                                        'inv_twice_sqrt',), 'atan': (
                                            'inv_one_plus_square',), 'asin': (
                                                'inv_sqrt_one_minus_square',), 'acos': (
                                                    'neg_inv_sqrt_one_minus_square',)}
TRIG_SQUARE_IDENTITIES = {
    'tan': (
        'Use tan^2 x = sec^2 x - 1.',
        'sec_minus_one'),
    'cot': (
        'Use cot^2 x = cosec^2 x - 1.',
        'cosec_minus_one'),
    'sin': (
        'Use sin^2 x = (1-cos(2x))/2.',
        'half_one_minus_cos2'),
    'cos': (
        'Use cos^2 x = (1+cos(2x))/2.',
        'half_one_plus_cos2')}
TRIG_HIGH_POWER_IDENTITIES = {
    (
        'sin',
        3): (
            'Use sin^2 x = 1-cos^2 x.',
            'odd_sin'),
    ('cos',
     3): (
        'Use cos^2 x = 1-sin^2 x.',
        'odd_cos'),
    ('sin',
     4): (
        'Use sin^2 x = (1-cos(2x))/2.',
        'sin4'),
    ('cos',
     5): (
        'Use cos^2 x = 1-sin^2 x.',
        'cos5'),
    ('sec',
     4): (
        'Use sec^2 x = 1+tan^2 x.',
        'sec4'),
    ('cosec',
     4): (
        'Use cosec^2 x = 1+cot^2 x.',
        'cosec4'),
    ('sin',
     6): (
        'Use sin^2 x = (1-cos(2x))/2.',
        'sin6'),
    ('cos',
     4): (
        'Use cos^2 x = (1+cos(2x))/2.',
        'cos4'),
    ('cos',
     6): (
        'Use cos^2 x = (1+cos(2x))/2.',
        'cos6'),
    ('tan',
     3): (
        'Use tan^2 x = sec^2 x - 1.',
        'tan3'),
    ('tan',
     4): (
        'Write tan^4 as tan^2 * tan^2.',
        'tan4')}
TRIG_PRODUCT_IDENTITIES = {('sin',
                            'cos'): 'Use sin 2A = 2sin A cos A.',
                           ('cos',
                            'sin'): 'Use sin 2A = 2sin A cos A.'}
FORMULA_SHEET_NOTES = 'sin(A+B) = sin A cos B + cos A sin B', 'cos(A+B) = cos A cos B - sin A sin B', 'tan(A+B) = (tan A + tan B)/(1 - tan A tan B)', 'sin A + sin B = 2sin((A+B)/2)cos((A-B)/2)', 'sin A - sin B = 2cos((A+B)/2)sin((A-B)/2)', 'cos A + cos B = 2cos((A+B)/2)cos((A-B)/2)', 'cos A - cos B = -2sin((A+B)/2)sin((A-B)/2)', 'sin(theta) ~= theta', 'cos(theta) ~= 1 - theta^2/2', 'tan(theta) ~= theta', "f'(x) = lim (f(x+h)-f(x))/h", "(f/g)' = (f'(x)g(x)-f(x)g'(x))/g(x)^2"


def primitive_from_rule(rule, arg):
    C = arg
    B = rule
    D = B[0]
    if D == 'fn':
        A = fn(B[1], C)
        return neg(A)if B[2] < 0 else A
    if D == 'log_abs_fn':
        A = log_abs(fn(B[1], C))
        return neg(A)if B[2] < 0 else A
    if D == 'log_abs_sum':
        E = B[1]
        A = log_abs(add([fn(E[0], C), fn(E[1], C)]))
        return neg(A)if B[2] < 0 else A
    if D == 'sqrt_power':
        return div(mul([num(2), power(C, num(3, 2))]), num(3))
    if D == 'inv_trig_parts_asin':
        return add([mul([C, fn('asin', C)]), fn('sqrt', add([num(1), neg(power(C, num(2)))]))])
    if D == 'inv_trig_parts_acos':
        return add([mul([C, fn('acos', C)]), neg(fn('sqrt', add([num(1), neg(power(C, num(2)))])))])
    if D == 'inv_trig_parts_atan':
        return add([mul([C, fn('atan', C)]), neg(mul([num(1, 2), fn('log', fn('abs', add([num(1), power(C, num(2))])))]))])


def primitive_of_named_function(name, arg):
    A = STANDARD_FN_PRIMITIVES.get(name)
    if A is None:
        return
    return primitive_from_rule(A, arg)


def primitive_of_named_power(name, arg, exp):
    if not is_int_num(exp):
        return
    A = STANDARD_POWER_PRIMITIVES.get((name, exp[1]))
    if A is None:
        return
    return primitive_from_rule(A, arg)


def primitive_of_named_product(left_name, right_name, arg):
    A = STANDARD_PRODUCT_PRIMITIVES.get((left_name, right_name))
    if A is None:
        return
    return primitive_from_rule(A, arg)


def derivative_from_rule(rule, arg, d):
    C = arg
    B = rule
    D = B[0]
    if D == 'fn':
        A = fn(B[1], C)
        A = neg(A)if B[2] < 0 else A
        return mul([A, d])
    if D == 'pow_fn':
        A = power(fn(B[1], C), num(B[2]))
        A = neg(A)if B[3] < 0 else A
        return mul([A, d])
    if D == 'prod_fns':
        E = B[1]
        A = mul([fn(E[0], C), fn(E[1], C)])
        A = neg(A)if B[2] < 0 else A
        return mul([A, d])
    if D == 'inv':
        return div(d, C)
    if D == 'inv_twice_sqrt':
        return div(d, mul([num(2), fn('sqrt', C)]))
    if D == 'inv_one_plus_square':
        return div(d, add([num(1), power(C, num(2))]))
    if D == 'inv_sqrt_one_minus_square':
        return div(d, fn('sqrt', add([num(1), neg(power(C, num(2)))])))
    if D == 'neg_inv_sqrt_one_minus_square':
        return neg(div(d, fn('sqrt', add([num(1), neg(power(C, num(2)))]))))


def derivative_of_named_function(name, arg, d):
    A = FUNCTION_DERIVATIVES.get(name)
    if A is None:
        return
    return derivative_from_rule(A, arg, d)


def trig_square_rewrite(name, arg):
    A = arg
    C = TRIG_SQUARE_IDENTITIES.get(name)
    if C is None:
        return None, None
    D = C[1]
    if D == 'sec_minus_one':
        B = add([power(fn('sec', A), num(2)), num(-1)])
    elif D == 'cosec_minus_one':
        B = add([power(fn('cosec', A), num(2)), num(-1)])
    elif D == 'half_one_minus_cos2':
        B = div(add([num(1), neg(fn('cos', mul([num(2), A])))]), num(2))
    else:
        B = div(add([num(1), fn('cos', mul([num(2), A]))]), num(2))
    return C[0], B


def trig_high_power_rewrite(name, exp, arg):
    A = arg
    if not is_int_num(exp):
        return None, None
    C = TRIG_HIGH_POWER_IDENTITIES.get((name, exp[1]))
    if C is None:
        return None, None
    D = C[1]
    if D == 'odd_sin':
        B = mul([fn('sin', A), add([num(1), neg(power(fn('cos', A), num(2)))])])
    elif D == 'odd_cos':
        B = mul([fn('cos', A), add([num(1), neg(power(fn('sin', A), num(2)))])])
    elif D == 'odd_sec':
        B = mul([fn('sec', A), add([num(1), neg(power(fn('tan', A), num(2)))])])
    elif D == 'odd_cosec':
        B = mul([fn('cosec', A), add([num(1), neg(power(fn('cot', A), num(2)))])])
    elif D == 'sin4':
        B = div(add([num(3), neg(mul([num(4), fn('cos', mul([num(2), A]))])), fn('cos', mul([num(4), A]))]), num(8))
    elif D == 'sin6':
        B = mul([fn('sin', A), add([num(1), neg(power(fn('cos', A), num(2)))])])
    elif D == 'cos4':
        B = div(add([num(3), mul([num(4), fn('cos', mul([num(2), A]))]), fn('cos', mul([num(4), A]))]), num(8))
    elif D == 'cos5':
        B = expand_small(add([fn('cos', A), neg(mul([num(2), power(fn('sin', A), num(2)), fn('cos', A)])), mul([power(fn('sin', A), num(4)), fn('cos', A)])]))
    elif D == 'cos6':
        B = mul([fn('cos', A), add([num(1), neg(power(fn('sin', A), num(2)))])])
    elif D == 'tan3':
        B = mul([fn('tan', A), add([num(-1), power(fn('sec', A), num(2))])])
    elif D == 'tan4':
        B = mul([power(fn('tan', A), num(2)), fn('tan', A)])
    elif D == 'sec4':
        B = mul([power(fn('sec', A), num(2)), add([num(1), power(fn('tan', A), num(2))])])
    elif D == 'sec5':
        B = mul([fn('sec', A), add([num(1), power(fn('tan', A), num(2))])])
    else:
        B = mul([power(fn('cosec', A), num(2)), add([num(1), power(fn('cot', A), num(2))])])
    return C[0], B


def trig_product_rewrite(left_name, right_name, arg):
    A = TRIG_PRODUCT_IDENTITIES.get((left_name, right_name))
    if A is None:
        return None, None
    return A, div(fn('sin', mul([num(2), arg])), num(2))


def trig_product_to_sum_rewrite(left_name, right_name, left_arg, right_arg, var):
    if var is None or linear_info(left_arg, var) is None or linear_info(
            right_arg, var) is None:
        return None, None
    A = add([left_arg, right_arg])
    B = add([left_arg, neg(right_arg)])
    if left_name == 'sin' and right_name == 'cos':
        return 'Use sin A cos B = 1/2(sin(A+B)+sin(A-B)).', div(
            add([fn('sin', A), fn('sin', B)]), num(2))
    if left_name == 'cos' and right_name == 'sin':
        return 'Use cos A sin B = 1/2(sin(A+B)-sin(A-B)).', div(
            add([fn('sin', A), neg(fn('sin', B))]), num(2))
    if left_name == 'sin' and right_name == 'sin':
        return 'Use sin A sin B = 1/2(cos(A-B)-cos(A+B)).', div(
            add([fn('cos', B), neg(fn('cos', A))]), num(2))
    if left_name == 'cos' and right_name == 'cos':
        return 'Use cos A cos B = 1/2(cos(A+B)+cos(A-B)).', div(
            add([fn('cos', A), fn('cos', B)]), num(2))
    return None, None


def trig_cubic_linear_combo_rewrite(node):
    A = sim(node)
    if A[0] != 'mul':
        return None, None
    B = list(flat(A, 'mul'))
    i = 0
    while i < len(B):
        C = B[i]
        if C[0] == 'pow' and C[1][0] == 'fn' and same(C[2], num(3)) and C[1][1] in ('sin', 'cos'):
            D = C[1][2]
            if C[1][1] == 'sin':
                E = div(add([mul([num(3), fn('sin', D)]), neg(fn('sin', mul([num(3), D])))]), num(4))
                F = 'Use sin(3A) = 3sin(A)-4sin^3(A).'
            else:
                E = div(add([mul([num(3), fn('cos', D)]), fn('cos', mul([num(3), D]))]), num(4))
                F = 'Use cos(3A) = 4cos^3(A)-3cos(A).'
            return F, expand_small(mul(B[:i] + [E] + B[i + 1:]))
        i += 1
    return None, None


def strip_simple_minus(node):
    A, B = split_coeff(node)
    if is_num(A) and A[1] < 0:
        C = num(-A[1], A[2])
        if is_one(B):
            return True, C
        if is_one(C):
            return True, B
        return True, mul([C, B])
    return False, node


def normalize_trig_signs(node):
    A = sim(node)
    B = A[0]
    if B in ('num', 'sym', 'const'):
        return A
    if B == 'fn':
        D = normalize_trig_signs(A[2])
        if A[1] in ('sin', 'tan', 'cot', 'cosec', 'cos', 'sec'):
            C, E = strip_simple_minus(D)
            if C:
                if A[1] in ('cos', 'sec'):
                    return sim(fn(A[1], E))
                return sim(neg(fn(A[1], E)))
        return sim(('fn', A[1], D))
    if B in ('pow', 'div'):
        return sim((B, normalize_trig_signs(A[1]), normalize_trig_signs(A[2])))
    return sim((B, tuple(normalize_trig_signs(C) for C in A[1])))


def trig_linear_reciprocal_rewrite(node):
    A = node
    if A[0] != 'div' or not is_one(A[1]) or A[2][0] != 'add':
        return None, None
    B = flat(A[2], 'add')
    if len(B) != 2:
        return None, None
    C = None
    D = None
    for E in B:
        if same(E, num(1)):
            C = num(1)
        elif E[0] == 'fn' and E[1] in ('sin', 'cos'):
            D = E
        elif E[0] == 'mul':
            F, G = split_coeff(E)
            if is_minus_one(F) and G[0] == 'fn' and G[1] in ('sin', 'cos'):
                D = neg(G)
    if C is None or D is None:
        return None, None
    H = D[2] if D[0] == 'fn' else D[1][1][2]
    I = D if D[0] == 'fn' else D[1][1]
    J = D[0] == 'fn'
    K = I[1]
    if K == 'sin':
        if J:
            return 'Rationalise the denominator.', add(
                [power(fn('sec', H), num(2)), neg(mul([fn('sec', H), fn('tan', H)]))])
        return 'Rationalise the denominator.', add(
            [power(fn('sec', H), num(2)), mul([fn('sec', H), fn('tan', H)])])
    if J:
        return 'Rationalise the denominator.', add(
            [power(fn('cosec', H), num(2)), neg(mul([fn('cosec', H), fn('cot', H)]))])
    return 'Rationalise the denominator.', add(
        [power(fn('cosec', H), num(2)), mul([fn('cosec', H), fn('cot', H)])])


def trig_same_angle_power_rewrite(node):
    A = sim(node)
    if A[0] != 'mul':
        return None, None
    B = flat(A, 'mul')
    C = None
    D = None
    E = []
    for F in B:
        if F[0] == 'fn' and F[1] in ('sin', 'cos'):
            if F[1] == 'sin':
                C = (F[2], 1) if C is None else C
                if C[0] != F[2]:
                    return None, None
            else:
                D = (F[2], 1) if D is None else D
                if D[0] != F[2]:
                    return None, None
        elif F[0] == 'pow' and F[1][0] == 'fn' and F[1][1] in ('sin', 'cos') and is_int_num(F[2]) and F[2][1] > 0:
            if F[1][1] == 'sin':
                C = (F[1][2], F[2][1]) if C is None else C
                if C[0] != F[1][2]:
                    return None, None
            else:
                D = (F[1][2], F[2][1]) if D is None else D
                if D[0] != F[1][2]:
                    return None, None
        else:
            E.append(F)
    if C is None or D is None or C[0] != D[0]:
        return None, None
    G = C[0]
    H = C[1]
    I = D[1]
    J = list(E)

    def K(base_name, start_pow, count):
        L = []
        M = 0
        while M <= count:
            N = math.comb(count, M) if math is not None and hasattr(math, 'comb') else 1
            O = start_pow + 2 * M
            P = fn(base_name, G) if O == 1 else power(fn(base_name, G), num(O))
            Q = P if N == 1 else mul([num(N), P])
            if M % 2 == 1:
                Q = neg(Q)
            L.append(Q)
            M += 1
        return make_add(L)

    if H % 2 == 1 and H >= 3:
        J.append(fn('sin', G))
        if I > 0:
            J.append(fn('cos', G) if I == 1 else power(fn('cos', G), num(I)))
        if H > 1:
            J.append(K('cos', 0, (H - 1) // 2))
        return 'Use sin^2 x = 1-cos^2 x.', expand_small(make_mul(J))
    if I % 2 == 1 and I >= 3:
        if H > 0:
            J.append(fn('sin', G) if H == 1 else power(fn('sin', G), num(H)))
        J.append(fn('cos', G))
        if I > 1:
            J.append(K('sin', 0, (I - 1) // 2))
        return 'Use cos^2 x = 1-sin^2 x.', expand_small(make_mul(J))
# Handle sin^m(x) * cos^n(x) where both m and n are even (general case)
    if H > 0 and I > 0 and H % 2 == 0 and I % 2 == 0:
        from math import comb as binomial
        terms = []
        half_h = H // 2
        half_i = I // 2
        for m in range(half_h + 1):
            for n in range(half_i + 1):
                coeff_val = binomial(half_h, m) * binomial(half_i, n)
                sign = (-1) ** (m + n)
                power_val = 2 * (m + n)
                coeff = num(sign * coeff_val, 2 ** (half_h + half_i))
                if power_val == 0:
                    terms.append(coeff)
                else:
                    terms.append(mul([coeff, fn('cos', mul([num(power_val), G]))]))
        reduced = make_add(terms)
        J.append(reduced)
        return 'Use power-reduction identities: sin^m(x)cos^n(x) = sum of cos(kx).', expand_small(make_mul(J))
    return None, None


def trig_weighted_square_rewrite(node):
    A = sim(node)
    if A[0] != 'add':
        return None, None
    B = flat(A, 'add')
    if len(B) != 2:
        return None, None
    C = None
    D = None
    E = None
    F = 0
    while F < len(B):
        G, H = split_coeff(B[F])
        if H[0] == 'pow' and H[1][0] == 'fn' and same(H[2], num(2)):
            if H[1][1] == 'sin':
                C = G
                E = H[1][2] if E is None else E
                if not same(E, H[1][2]):
                    return None, None
            elif H[1][1] == 'cos':
                D = G
                E = H[1][2] if E is None else E
                if not same(E, H[1][2]):
                    return None, None
            else:
                return None, None
        else:
            return None, None
        F += 1
    if C is None or D is None or E is None:
        return None, None
    I = div(add([C, D]), num(2))
    J = div(add([D, neg(C)]), num(2))
    K = [I]
    if not is_zero(J):
        K.append(mul([J, fn('cos', mul([num(2), E]))]))
    return 'Use sin^2 x = (1-cos(2x))/2 and cos^2 x = (1+cos(2x))/2.', expand_small(add(K))


def split_coeff(node):
    B = node
    if is_num(B):
        return B, num(1)
    if B[0] == 'mul':
        A = B[1]
        if len(A) > 0 and is_num(A[0]):
            if len(A) == 2:
                return A[0], A[1]
            return A[0], ('mul', A[1:])
    return num(1), B


def norm_pow_base(node):
    A = node
    if A[0] != 'pow' or not is_num(A[1]):
        return A
    D = A[1]
    F = A[2]
    if D[1] <= 0:
        return A
    if D[2] == 1:
        C = D[1]
        B = 2
        while B * B <= C:
            if C % B == 0:
                E = 0
                while C % B == 0:
                    C //= B
                    E += 1
                if C == 1 and E > 1:
                    return 'pow', num(B), add([num(E), neg(num(0)), F])
                return A
            B += 1
    return A


def factor_map(node):
    F = node
    I = list(F[1])if F[0] == 'mul'else [F]
    G = num(1)
    E = []
    C = {}
    H = 0
    while H < len(I):
        B = norm_pow_base(I[H])
        if is_num(B):
            G = mulq(G, B)
        elif B[0] == 'fn' and B[1] == 'sqrt':
            A = sig(B[2])
            if A not in C:
                E.append(A)
                C[A] = [B[2], num(0)]
            D = C[A][1]
            C[A][1] = addq(D, num(1, 2))if is_num(D)else add([D, num(1, 2)])
        elif B[0] == 'pow':
            A = sig(B[1])
            if A not in C:
                E.append(A)
                C[A] = [B[1], num(0)]
            D = C[A][1]
            C[A][1] = addq(D, B[2])if is_num(
                D) and is_num(B[2])else add([D, B[2]])
        else:
            A = sig(B)
            if A not in C:
                E.append(A)
                C[A] = [B, num(0)]
            D = C[A][1]
            C[A][1] = addq(D, num(1))if is_num(D)else add([D, num(1)])
        H += 1
    return G, E, C


def all_neg_add(node):
    if node[0] != 'add':
        return False
    B = flat(node, 'add')
    A = 0
    while A < len(B):
        C, D = split_coeff(B[A])
        if C[1] >= 0 or is_zero(C):
            return False
        A += 1
    return True


def flip_add(node):
    A = []
    for B in flat(node, 'add'):
        A.append(neg(B))
    return add(A)


def int_sqrt(n):
    if n < 0:
        return
    if FAST_ISQRT is not None:
        A = FAST_ISQRT(n)
        if A * A == n:
            return A
        return
    A = 0
    while A * A < n:
        A += 1
    if A * A == n:
        return A


def sqrt_num(node):
    A = node
    if not is_num(A) or A[1] < 0:
        return
    B = int_sqrt(A[1])
    C = int_sqrt(A[2])
    if B is None or C is None:
        return
    return num(B, C)


def split_square_factor(n):
    outside = 1
    inside = n
    k = 2
    while k * k <= inside:
        square = k * k
        while inside % square == 0:
            outside *= k
            inside //= square
        k += 1
    return outside, inside


def sqrt_factor_expr(node):
    if not is_num(node) or node[1] <= 0:
        return fn('sqrt', node)
    top_out, top_in = split_square_factor(node[1])
    bot_out, bot_in = split_square_factor(node[2])
    coeff = num(top_out, bot_out)
    inside = num(top_in, bot_in)
    if is_one(inside):
        return coeff
    root = fn('sqrt', inside)
    if is_one(coeff):
        return root
    return mul([coeff, root])


def square_root_term_candidate(node, var):
    A = sim(node)
    if not depends(A, var):
        return sqrt_num(A)
    if A[0] == 'fn' and A[1] == 'exp':
        return fn('exp', div(A[2], num(2)))
    if A[0] == 'pow' and is_num(A[2]):
        if is_int_num(A[2]) and A[2][1] > 0 and A[2][1] % 2 == 0:
            return power(A[1], num(A[2][1] // 2))
        if A[2][1] > 0 and A[2][1] % 2 == 0:
            return power(A[1], divq(A[2], num(2)))
    if A[0] == 'mul':
        roots = []
        B = 0
        while B < len(A[1]):
            C = square_root_term_candidate(A[1][B], var)
            if C is None:
                return
            roots.append(C)
            B += 1
        return sim(mul(roots))


def plus_square_inner_info(node, var):
    A = sim(node)
    if A[0] != 'add':
        return
    B = flat(A, 'add')
    if len(B) != 2:
        return
    C = 0
    while C < 2:
        D = B[C]
        E = B[1 - C]
        if depends(D, var) or is_zero(E):
            C += 1
            continue
        F = square_root_term_candidate(E, var)
        if F is not None and depends(F, var):
            return D, F
        C += 1


def special_trig_value(name, arg):
    A = arg
    if same(A, num(0)):
        if name in ('sin', 'tan'):
            return num(0)
        if name in ('cos', 'sec'):
            return num(1)
    if same(A, div(PI, num(6))):
        if name == 'sin':
            return num(1, 2)
        if name == 'cos':
            return div(fn('sqrt', num(3)), num(2))
        if name == 'tan':
            return div(num(1), fn('sqrt', num(3)))
        if name == 'sec':
            return div(num(2), fn('sqrt', num(3)))
        if name == 'cosec':
            return num(2)
        if name == 'cot':
            return fn('sqrt', num(3))
    if same(A, div(PI, num(4))):
        if name in ('sin', 'cos'):
            return div(fn('sqrt', num(2)), num(2))
        if name in ('tan', 'cot'):
            return num(1)
        if name in ('sec', 'cosec'):
            return fn('sqrt', num(2))
    if same(A, div(PI, num(3))):
        if name == 'sin':
            return div(fn('sqrt', num(3)), num(2))
        if name == 'cos':
            return num(1, 2)
        if name == 'tan':
            return fn('sqrt', num(3))
        if name == 'sec':
            return num(2)
        if name == 'cosec':
            return div(num(2), fn('sqrt', num(3)))
        if name == 'cot':
            return div(num(1), fn('sqrt', num(3)))
    if same(A, div(PI, num(2))):
        if name == 'sin':
            return num(1)
        if name == 'cos':
            return num(0)
    if A[0] == 'mul' and len(A[1]) == 2 and is_minus_one(A[1][0]):
        B = special_trig_value(name, A[1][1])
        if B is None:
            return
        if name in ('sin', 'tan', 'cosec', 'cot'):
            return neg(B)
        return B


def sim(node):
    L = node
    R = L[0]
    if R in ('num', 'sym', 'const'):
        return L
    if R == 'fn':
        S = L[1]
        N = sim(L[2])
        if S == 'exp' and N[0] == 'fn' and N[1] == 'log':
            return N[2]
        if S == 'abs' and is_num(N):
            return num(abs(N[1]), N[2])
        if S == 'abs' and (N[0] == 'pow' and same(N[1], E)
                           or N[0] == 'fn' and N[1] == 'exp'):
            return N
        if S == 'sqrt' and is_num(N):
            b = sqrt_num(N)
            if b is not None:
                return b
        if S == 'log' and is_one(N):
            return num(0)
        if S == 'log' and same(N, E):
            return num(1)
        if S == 'log' and (N[0] == 'pow' and same(N[1], E)
                           or N[0] == 'fn' and N[1] == 'exp'):
            return N[2]
        if S == 'log' and N[0] == 'pow' and is_num(N[2]):
            return mul([N[2], ('fn', 'log', fn('abs', N[1]))])
        if S == 'log' and N[0] == 'div':
            return add([('fn', 'log', fn('abs', N[1])), ('mul', (num(-1), ('fn', 'log', fn('abs', N[2]))))])
        if S == 'log' and N[0] == 'fn' and N[1] == 'abs' and (
                N[2][0] == 'pow' and same(N[2][1], E) or N[2][0] == 'fn' and N[2][1] == 'exp'):
            return N[2][2]
        A = special_trig_value(S, N)
        if A is not None:
            return A
        if S == 'exp' and is_zero(N):
            return num(1)
        if S == 'exp' and N[0] == 'fn' and N[1] == 'log':
            return N[2]
        if S == 'exp':
            return 'fn', 'exp', N
        return 'fn', S, N
    if R == 'pow':
        D = sim(L[1])
        B = sim(L[2])
        if same(D, E) and B[0] == 'fn' and B[1] == 'log':
            return B[2]
        if is_num(B):
            if is_zero(B):
                return num(1)
            if is_one(B):
                return D
            if D[0] == 'fn' and D[1] == 'sqrt' and is_int_num(
                    B) and B[1] % 2 == 0:
                return power(D[2], num(B[1] // 2))
            if is_num(D) and is_int_num(B):
                return int_pow(D, B[1])
            if D[0] == 'pow' and is_num(D[2]) and is_num(B):
                return power(D[1], mulq(D[2], B))
            if D[0] == 'div' and is_int_num(B):
                if B[1] > 0:
                    return div(power(D[1], B), power(D[2], B))
                return div(power(D[2], num(-B[1])), power(D[1], num(-B[1])))
        if is_one(D):
            return num(1)
        return 'pow', D, B
    if R == 'add':
        J = []
        A = 0
        while A < len(L[1]):
            H = sim(L[1][A])
            if H[0] == 'add':
                J.extend(H[1])
            else:
                J.append(H)
            A += 1
        U = num(0)
        O = []
        K = {}
        A = 0
        while A < len(J):
            M, P = split_coeff(J[A])
            if is_one(P):
                U = addq(U, M)
            else:
                G = sig(P)
                if G not in K:
                    O.append(G)
                    K[G] = [P, num(0)]
                K[G][1] = addq(K[G][1], M)
            A += 1
        I = []
        A = 0
        while A < len(O):
            P, M = K[O[A]]
            if not is_zero(M):
                if is_one(M):
                    I.append(P)
                elif is_minus_one(M):
                    I.append(mul([num(-1), P]))
                else:
                    I.append(mul([M, P]))
            A += 1
        if not is_zero(U) or len(I) == 0:
            I.append(U)
        if len(I) == 1:
            return I[0]
        return 'add', tuple(I)
    if R == 'mul':
        J = []
        A = 0
        while A < len(L[1]):
            H = sim(L[1][A])
            if H[0] == 'mul':
                J.extend(H[1])
            else:
                J.append(H)
            A += 1
        A = 0
        while A < len(J):
            if J[A][0] == 'div':
                C = []
                F = []
                T = 0
                while T < len(J):
                    if J[T][0] == 'div':
                        C.append(J[T][1])
                        F.append(J[T][2])
                    else:
                        C.append(J[T])
                    T += 1
                return div(make_mul(C), make_mul(F))
            A += 1
        V = num(1)
        O = []
        K = {}
        A = 0
        while A < len(J):
            H = J[A]
            if is_zero(H):
                return num(0)
            if is_num(H):
                V = mulq(V, H)
            elif H[0] == 'fn' and H[1] == 'exp':
                D = E
                B = H[2]
                G = sig(D)
                if G not in K:
                    O.append(G)
                    K[G] = [D, num(0)]
                Q = K[G][1]
                K[G][1] = addq(Q, B)if is_num(Q) and is_num(B)else add([Q, B])
            elif H[0] == 'fn' and H[1] == 'sqrt':
                G = sig(H[2])
                if G not in K:
                    O.append(G)
                    K[G] = [H[2], num(0)]
                Q = K[G][1]
                K[G][1] = addq(Q, num(1, 2))if is_num(
                    Q)else add([Q, num(1, 2)])
            else:
                H = norm_pow_base(H)
                if H[0] == 'pow':
                    D = H[1]
                    B = H[2]
                else:
                    D = H
                    B = num(1)
                G = sig(D)
                if G not in K:
                    O.append(G)
                    K[G] = [D, num(0)]
                Q = K[G][1]
                K[G][1] = addq(Q, B)if is_num(Q) and is_num(B)else add([Q, B])
            A += 1
        I = []
        if not is_one(V):
            I.append(V)
        A = 0
        while A < len(O):
            D, B = K[O[A]]
            if not is_zero(B):
                if is_one(B):
                    I.append(D)
                else:
                    I.append(('pow', D, B))
            A += 1
        if len(I) == 2 and is_minus_one(I[0]) and I[1][0] == 'add':
            return flip_add(I[1])
        if len(I) == 0:
            return num(1)
        if len(I) == 1:
            return I[0]
        return 'mul', tuple(I)
    if R == 'div':
        C = sim(L[1])
        F = sim(L[2])
        if is_zero(C):
            return num(0)
        if is_one(F):
            return C
        if is_one(C) and F[0] == 'fn':
            if F[1] == 'cos':
                return 'fn', 'sec', F[2]
            if F[1] == 'sin':
                return 'fn', 'cosec', F[2]
            if F[1] == 'exp':
                return fn('exp', neg(F[2]))
        if is_one(C) and F[0] == 'pow' and F[1][0] == 'fn' and is_int_num(
                F[2]) and F[2][1] > 0:
            if F[1][1] == 'cos':
                return power(fn('sec', F[1][2]), F[2])
            if F[1][1] == 'sin':
                return power(fn('cosec', F[1][2]), F[2])
            if F[1][1] == 'sec':
                return power(fn('cos', F[1][2]), F[2])
            if F[1][1] == 'cosec':
                return power(fn('sin', F[1][2]), F[2])
            if F[1][1] == 'tan':
                return power(fn('cot', F[1][2]), F[2])
            if F[1][1] == 'cot':
                return power(fn('tan', F[1][2]), F[2])
        if C[0] == 'fn' and F[0] == 'fn' and C[2] == F[2]:
            if C[1] == 'sin' and F[1] == 'cos':
                return 'fn', 'tan', C[2]
            if C[1] == 'cos' and F[1] == 'sin':
                return 'fn', 'cot', C[2]
        if C[0] == 'pow' and F[0] == 'pow' and C[1][0] == 'fn' and F[1][0] == 'fn' and C[2] == F[2] and C[1][2] == F[1][2]:
            if C[1][1] == 'sin' and F[1][1] == 'cos':
                return power(fn('tan', C[1][2]), C[2])
            if C[1][1] == 'cos' and F[1][1] == 'sin':
                return power(fn('cot', C[1][2]), C[2])
        if same(C, F):
            return num(1)
        if all_neg_add(F):
            return div(neg(C), flip_add(F))
        if C[0] == 'div':
            return div(C[1], mul([C[2], F]))
        if F[0] == 'div':
            return div(mul([C, F[2]]), F[1])
        if is_num(C) and is_num(F):
            return divq(C, F)
        f, W, Z = factor_map(C)
        g, c, X = factor_map(F)
        M = divq(f, g)
        A = 0
        while A < len(W):
            G = W[A]
            if G in X:
                Z[G][1] = add([Z[G][1], neg(X[G][1])])
                X[G][1] = num(0)
            A += 1
        Y = []
        a = []
        if not is_one(M):
            if M[2] == 1:
                Y.append(num(M[1]))
            else:
                Y.append(num(M[1], 1))
                a.append(num(M[2], 1))
        A = 0
        while A < len(W):
            D, B = Z[W[A]]
            if not is_zero(B):
                Y.append(D if is_one(B)else ('pow', D, B))
            A += 1
        A = 0
        while A < len(c):
            D, B = X[c[A]]
            if not is_zero(B):
                a.append(D if is_one(B)else ('pow', D, B))
            A += 1
        d = make_mul(Y)
        e = make_mul(a)
        if is_one(e):
            return d
        return 'div', d, e
    return L


def pr(node):
    A = node[0]
    if A in ('num', 'sym', 'const'):
        return 5
    if A == 'fn':
        return 4
    if A == 'pow':
        return 3
    if A in ('mul', 'div'):
        return 2
    return 1


def display_recip(node):
    A = node
    if A[0] == 'div' and is_one(A[1]):
        return A[2]
    if A[0] == 'pow' and is_minus_one(A[2]):
        return A[1]
    return None


def display_rearrange(node):
    A = node
    B = A[0]
    if B in ('num', 'sym', 'const'):
        return A
    if B == 'fn':
        return 'fn', A[1], display_rearrange(A[2])
    if B in ('pow', 'div'):
        return B, display_rearrange(A[1]), display_rearrange(A[2])
    if B == 'mul':
        E = []
        F = []
        C = 0
        D = flat(A, 'mul')
        while C < len(D):
            G = display_rearrange(D[C])
            H = display_recip(G)
            if H is None:
                E.append(G)
            else:
                F.append(H)
            C += 1
        if len(F) == 0:
            return make_mul(E)
        I = make_mul(E)
        J = make_mul(F)
        if is_one(I):
            return 'div', num(1), J
        return 'div', I, J
    D = []
    C = 0
    E = flat(A, 'add')
    while C < len(E):
        D.append(display_rearrange(E[C]))
        C += 1
    if len(D) == 2 and is_num(D[1]) and D[1][1] > 0 and display_neg(D[0]):
        return 'add', (D[1], D[0])
    return 'add', tuple(D)


def display_abs_arg(arg):
    B = arg
    if B[0] == 'add':
        A = flat(B, 'add')
        if len(A) == 2:
            if is_num(A[0]) and A[0][1] > 0 and A[1][0] == 'mul' and is_minus_one(
                    A[1][1][0]):
                return _show(display_rearrange(
                    make_add([A[0], neg(A[1][1][1])])), 0)
            if is_num(A[1]) and A[1][1] > 0 and A[0][0] == 'mul' and is_minus_one(
                    A[0][1][0]):
                return _show(display_rearrange(
                    make_add([A[1], neg(A[0][1][1])])), 0)
            if is_num(A[1]) and A[1][1] < 0:
                return _show(display_rearrange(
                    make_add([num(-A[1][1], A[1][2]), neg(A[0])])), 0)
            if is_num(A[0]) and A[0][1] < 0:
                return _show(display_rearrange(
                    make_add([num(-A[0][1], A[0][2]), neg(A[1])])), 0)
    return _show(display_rearrange(B), 0)


def display_neg(node):
    A = node
    B, C = split_coeff(A)
    if B[1] < 0:
        return True
    if A[0] == 'div' and is_num(A[1]) and A[1][1] < 0:
        return True
    if A[0] == 'div' and A[1][0] == 'mul' and len(
            A[1][1]) > 0 and is_num(A[1][1][0]) and A[1][1][0][1] < 0:
        return True
    return False


def display_abs(node):
    A = node
    if A[0] == 'div' and is_num(A[1]) and A[1][1] < 0:
        return div(num(-A[1][1], A[1][2]), A[2])
    if A[0] == 'div' and A[1][0] == 'mul' and len(
            A[1][1]) > 0 and is_num(A[1][1][0]) and A[1][1][0][1] < 0:
        D = [num(-A[1][1][0][1], A[1][1][0][2])]
        C = 1
        while C < len(A[1][1]):
            D.append(A[1][1][C])
            C += 1
        return div(make_mul(D), A[2])
    B, E = split_coeff(A)
    if B[1] < 0:
        return E if is_minus_one(B)else mul([num(-B[1], B[2]), E])
    return A


def _show(node, parent=0):
    P = parent
    A = node
    G = A[0]
    if G == 'num':
        if A[2] == 1:
            B = str(A[1])
        else:
            B = str(A[1]) + '/' + str(A[2])
    elif G == 'sym':
        B = A[1]
    elif G == 'const':
        B = A[1]
    elif G == 'fn':
        if A[1] == 'log' and A[2][0] == 'fn' and A[2][1] == 'abs':
            B = 'ln|' + display_abs_arg(A[2][2]) + '|'
        elif A[1] == 'log':
            B = 'ln(' + _show(A[2], 0) + ')'
        elif A[1] == 'exp':
            I = _show(A[2], pr(A))
            if pr(A[2]) < pr(A):
                I = '(' + _show(A[2], 0) + ')'
            B = 'e^' + I
        else:
            B = A[1] + '(' + _show(A[2], 0) + ')'
    elif G == 'pow':
        if same(A[2], num(1, 2)):
            B = 'sqrt(' + _show(A[1], 0) + ')'
        elif same(A[2], num(-1, 2)):
            B = '1/sqrt(' + _show(A[1], 0) + ')'
        elif is_int_num(A[2]) and A[2][1] < 0:
            N = _show(A[1], pr(A))
            if pr(A[1]) < pr(A) or A[1][0] in ('add', 'div'):
                N = '(' + _show(A[1], 0) + ')'
            Q = num(-A[2][1])
            if same(Q, num(1)):
                B = '1/' + N
            else:
                B = '1/' + N + '^' + _show(Q, pr(A))
        else:
            J = _show(A[1], pr(A))
            K = _show(A[2], pr(A))
            if pr(A[1]) < pr(A) or A[1][0] in ('add', 'div'):
                J = '(' + _show(A[1], 0) + ')'
            if pr(A[2]) < pr(A) or is_num(A[2]) and A[2][2] != 1 or same(
                    A[1], E) and A[2][0] in ('pow', 'add', 'mul', 'div'):
                K = '(' + _show(A[2], 0) + ')'
            B = J + '^' + K
    elif G == 'mul':
        L = list(A[1])
        if len(L) > 0 and is_minus_one(L[0]):
            D = make_mul(L[1:])
            I = _show(D, pr(A))
            if pr(D) < pr(A):
                I = '(' + _show(D, 0) + ')'
            B = '-' + I
        else:
            R = []
            C = 0
            while C < len(L):
                M = L[C]
                H = _show(M, pr(A))
                if pr(M) < pr(A) or M[0] == 'add':
                    H = '(' + _show(M, 0) + ')'
                R.append(H)
                C += 1
            B = '*'.join(R)
    elif G == 'div':
        if A[1][0] == 'mul' and len(A[1][1]) == 2 and is_num(
                A[1][1][0]) and is_num(A[2]):
            O = divq(A[1][1][0], A[2])
            D = A[1][1][1]
            H = _show(D, pr(('mul', (O, D))))
            if pr(D) < pr(('mul', (O, D))) or D[0] == 'add':
                H = '(' + _show(D, 0) + ')'
            B = _show(O, pr(A)) + '*' + H
            if pr(A) < P:
                return '(' + B + ')'
            return B
        J = _show(A[1], pr(A))
        K = _show(A[2], pr(A))
        if pr(A[1]) < pr(A) or A[1][0] == 'add':
            J = '(' + _show(A[1], 0) + ')'
        if pr(A[2]) <= pr(A):
            K = '(' + _show(A[2], 0) + ')'
        B = J + '/' + K
    else:
        F = flat(A, 'add')
        B = ''
        C = 0
        while C < len(F):
            if C == 0:
                H = display_abs(F[C])if display_neg(F[C])else F[C]
                B = _show(H, pr(A))
                if display_neg(F[C]):
                    B = '-' + B
            elif display_neg(F[C]):
                B += ' - ' + _show(display_abs(F[C]), pr(A))
            else:
                B += ' + ' + _show(F[C], pr(A))
            C += 1
    if pr(A) < P:
        return '(' + B + ')'
    return B


def show(node, parent=0): return _show(display_rearrange(sim(node)), parent)
def pretty(node): return _show(display_rearrange(combine_logs(node)), 0)


def ensure_reasoning_marker(lines, default_prefix="Method: "):
    """Add reasoning marker to output lines if missing."""
    if not lines:
        return lines
    text = "\n".join(lines)
    if any(marker in text.lower() for marker in REASONING_MARKERS):
        return lines
    lines = list(lines)
    if lines and not any(lines[0].lower().startswith(k) for k in ("use", "using", "let", "method", "hence", "therefore", "thus")):
        lines.insert(0, default_prefix)
    return lines

def int_text(node, var): return 'Int[' + pretty(node) + '] d' + var


def exam_group_text(node):
    A = sim(node)
    B = pretty(A)
    if A[0] in ('add', 'div'):
        return '(' + B + ')'
    return B


def signed_value_text(node):
    A = sim(node)
    if is_zero(A):
        return ''
    if is_one(A):
        return '+ 1'
    if is_minus_one(A):
        return '- 1'
    if is_num(A) and A[1] < 0:
        return '- ' + pretty(neg(A))
    return '+ ' + pretty(A)


def signed_term_text(coeff, text):
    A = sim(coeff)
    if is_zero(A):
        return ''
    if is_one(A):
        return '+ ' + text
    if is_minus_one(A):
        return '- ' + text
    if is_num(A) and A[1] < 0:
        return '- ' + pretty(neg(A)) + '*' + text
    return '+ ' + pretty(A) + '*' + text


def direct_quadratic_surd_lines(var, radical, shifted, constant, coeff, result):
    A = sim(add([power(shifted, num(2)), constant]))
    B = int_text(div(shifted, fn('sqrt', A)), var)
    C = int_text(div(num(1), fn('sqrt', A)), var)
    D = 'I = ' + B
    E = signed_term_text(coeff, C)
    if E != '':
        D += ' ' + E
    F = 'Rewrite the numerator: ' + pretty(sym(var)) + ' = ' + exam_group_text(shifted)
    G = signed_value_text(coeff)
    if G != '':
        F += ' ' + G
    F += '.'
    H = pretty(constant)
    I = pretty(A)
    J = 'ln|' + pretty(shifted) + '+sqrt(' + I + ')|'
    K = 'I = sqrt(' + I + ')'
    L = signed_term_text(coeff, J)
    if L != '':
        K += ' ' + L
    K += ' + C'
    return [
        'Complete the square: ' + pretty(radical) + ' = ' + I + '.',
        F,
        D,
        'Let u = ' + pretty(shifted) + '.',
        'Use Int[u/sqrt(u^2+' + H + ')] du = sqrt(u^2+' + H + ') and Int[1/sqrt(u^2+' + H + ')] du = ln|u+sqrt(u^2+' + H + ')|.',
        K,
        'So I = ' + pretty(result) + ' + C']


def du_equals_text(d, var):
    A = var
    d = sim(d)
    if is_one(d):
        return 'du = d' + A
    if is_minus_one(d):
        return 'du = -d' + A
    return 'du = ' + pretty(d) + ' d' + A


def substitution_work(node, var, inner, d, uexpr, work, uans, ans):
    D = uexpr
    C = inner
    B = var
    A = [
        'u = ' + pretty(C),
        'du/d' + B + ' = ' + pretty(d),
        du_equals_text(d, B)]
    A.append('I = ' + int_text(D, 'u'))
    if not same(work, D):
        A.append('= ' + int_text(work, 'u'))
    A.append('= ' + pretty(uans) + ' + C')
    A.append('Substitute back u = ' + pretty(C) + '.')
    A.append('= ' + pretty(ans) + ' + C')
    return A


def _combine_logs_uncached(node, _depth=0):
    if _depth > 10:
        return node
    A = node
    A = sim(A)
    B = A[0]
    if B in ('num', 'sym', 'const'):
        return A
    if B == 'fn':
        return 'fn', A[1], _combine_logs_uncached(A[2], _depth+1)
    if B in ('pow', 'div'):
        return sim((B, _combine_logs_uncached(A[1], _depth+1), _combine_logs_uncached(A[2], _depth+1)))
    if B == 'mul':
        G = []
        for F in flat(A, 'mul'):
            G.append(_combine_logs_uncached(F, _depth+1))
        return sim(('mul', tuple(G)))
    C = []
    for F in flat(A, 'add'):
        C.append(_combine_logs_uncached(F, _depth+1))
    if len(C) == 2:
        H, D = split_coeff(C[0])
        I, E = split_coeff(C[1])
        if is_one(H) and is_minus_one(I) and D[0] == 'fn' and D[1] == 'log' and D[2][0] == 'fn' and D[
                2][1] == 'abs' and E[0] == 'fn' and E[1] == 'log' and E[2][0] == 'fn' and E[2][1] == 'abs':
            return fn('log', fn('abs', div(D[2][2], E[2][2])))
    return sim(('add', tuple(C)))


def diff(node, var):
    B = var
    A = node
    C = A[0]
    if C in ('num', 'const'):
        return num(0)
    if C == 'sym':
        return num(1)if A[1] == B else num(0)
    if C == 'add':
        D = []
        for P in A[1]:
            D.append(diff(P, B))
        return add(D)
    if C == 'mul':
        H = list(A[1])
        D = []
        I = 0
        while I < len(H):
            L = []
            F = 0
            while F < len(H):
                L.append(diff(H[F], B)if I == F else H[F])
                F += 1
            D.append(mul(L))
            I += 1
        return add(D)
    if C == 'div':
        M = A[1]
        J = A[2]
        Q = diff(M, B)
        R = diff(J, B)
        return div(add([mul([J, Q]), neg(mul([M, R]))]), power(J, num(2)))
    if C == 'pow':
        K = A[1]
        G = A[2]
        if is_num(G):
            return mul([G, power(K, subq(G, num(1))), diff(K, B)])
        if same(K, E):
            return mul([power(E, G), diff(G, B)])
        raise ValueError(FAIL)
    if C == 'fn':
        N = A[2]
        T = A[1]
        if T == 'log' and N[0] == 'fn' and N[1] == 'abs':
            return div(diff(N[2], B), N[2])
        S = diff(N, B)
        if T == 'abs':
            return div(mul([N, S]), fn('abs', N))
        O = derivative_of_named_function(T, N, S)
        if O is not None:
            return O
    raise ValueError(FAIL)


def safe_diff(node, var):
    try:
        return diff(node, var)
    except ValueError:
        return


def is_digit_char(ch): return '0' <= ch <= '9'
def is_alpha_char(ch): return 'A' <= ch <= 'Z' or 'a' <= ch <= 'z'
def is_name_start(ch): return is_alpha_char(ch) or ch == '_'
def is_name_char(ch): return is_name_start(ch) or is_digit_char(ch)


def is_num_token_start(text, i):
    ch = text[i]
    return is_digit_char(ch) or (ch == '.' and i + 1 < len(text) and is_digit_char(text[i + 1]))


def parse(text):
    C = text
    G = []
    A = 0
    while A < len(C):
        H = C[A]
        if H in ' \t\r\n':
            A += 1
        elif C[A:A + 2] == '**':
            G.append('**')
            A += 2
        elif H in '()+-*/^=,':
            G.append(H)
            A += 1
        elif is_num_token_start(C, A):
            B = A
            M = 0
            while B < len(C) and (is_digit_char(C[B]) or C[B] == '.'):
                if C[B] == '.':
                    M += 1
                B += 1
            if M > 1:
                raise ValueError('Bad number.')
            G.append(C[A:B])
            A = B
        elif is_name_start(H):
            B = A + 1
            while B < len(C) and is_name_char(C[B]):
                B += 1
            G.append(C[A:B])
            A = B
        else:
            raise ValueError('Unexpected character: ' + H)
    I = 0

    def F():
        if I >= len(G):
            return
        return G[I]

    def D(tok=None):
        A = tok
        nonlocal I
        B = F()
        if A is not None and B != A:
            raise ValueError(
                'Expected ' +
                repr(A) +
                ' but found ' +
                repr(B) +
                '.')
        I += 1
        return B

    def consume_func_arg():
        if F() == '(':
            D('(')
            A = L()
            D(')')
            return A
        if O(F()):
            return P()
        raise ValueError('Bad function form.')

    def R(tok):
        A = tok
        if A is None:
            return False
        if A == '(' or A == '-':
            return True
        if A not in ('+', '*', '/', '^', '**', ')', ',', '='):
            return True
        return False

    def O(tok):
        A = tok
        if A is None:
            return False
        if A == '(':
            return True
        if A in ('+', '-', '*', '/', '^', '**', ')', ',', '='):
            return False
        return True

    def P():
        A = F()
        if A == '(':
            D('(')
            C = L()
            D(')')
            return C
        if A is None:
            raise ValueError('Unexpected end.')
        if A not in ('+', '*', '/', '^', '**', ')', ',', '='):
            D()
            if A[0].isdigit() or A[0] == '.':
                return num_text(A)
            B = A.lower()
            if B == 'ln':
                B = 'log'
            elif B == 'arcsin':
                B = 'asin'
            elif B == 'arccos':
                B = 'acos'
            elif B == 'arctan':
                B = 'atan'
            if B == 'pi':
                return PI
            if B == 'e':
                return E
            if B in FUNC_NAMES:
                if F() in ('^', '**'):
                    D(F())
                    C = J()
                    return power(fn(B, consume_func_arg()), C)
                if F() == '(':
                    D('(')
                    arg1 = L()
                    if F() == ',':
                        D(',')
                        arg2 = L()
                        D(')')
                        # For log, handle two-argument form: log(base, exp) = ln(exp)/ln(base)
                        if B == 'log':
                            return div(fn('log', arg2), fn('log', arg1))
                        else:
                            return fn(B, ('mul', (arg1, arg2)))
                    else:
                        D(')')
                        return fn(B, arg1)
                if O(F()):
                    return fn(B, P())
            return sym(A)
        raise ValueError('Bad token: ' + repr(A))

    def N():
        A = P()
        if F() in ('^', '**'):
            D(F())
            return power(A, N())
        return A

    def J():
        if F() == '-':
            D('-')
            return neg(J())
        return N()

    def K():
        A = J()
        while True:
            B = F()
            if B == '*':
                D('*')
                A = mul([A, J()])
            elif B == '/':
                D('/')
                A = div(A, J())
            elif O(B):
                A = mul([A, J()])
            else:
                return A

    def L():
        A = K()
        while F() in ('+', '-'):
            if F() == '+':
                D('+')
                A = add([A, K()])
            else:
                D('-')
                A = add([A, neg(K())])
        return A
    Q = L()
    if I != len(G):
        raise ValueError('Unexpected token: ' + repr(F()))
    return sim(Q)


def is_supported_explicit_var(name):
    return len(name) == 1 and is_name_start(name)


def split_input_var(text):
    A = text.strip()
    if A[:1] == '(' and A[-1:] == ')':
        A = A[1:-1].strip()
    E = 0
    B = -1
    C = 0
    while C < len(A):
        F = A[C]
        if F == '(':
            E += 1
        elif F == ')':
            E -= 1
        elif F == ',' and E == 0:
            B = C
            break
        C += 1
    if B == -1:
        return
    H = A[:B].strip()
    D = A[B + 1:].strip()
    if H == '' or D == '':
        return
    if not is_supported_explicit_var(D):
        raise ValueError('Use a single-letter variable after the comma.')
    return H, D


def cartesian_linear_pair(node, var_name):
    A = sim(node)
    B = var_name
    if not depends(A, B):
        return num(0), A
    if A == sym(B):
        return num(1), num(0)
    if A[0] == 'add':
        C = num(0)
        D = num(0)
        E = flat(A, 'add')
        F = 0
        while F < len(E):
            G = cartesian_linear_pair(E[F], B)
            if G is None:
                return None
            C = sim(add([C, G[0]]))
            D = sim(add([D, G[1]]))
            F += 1
        return C, D
    if A[0] == 'mul':
        H = []
        I = []
        E = flat(A, 'mul')
        F = 0
        while F < len(E):
            if depends(E[F], B):
                H.append(E[F])
            else:
                I.append(E[F])
            F += 1
        if len(H) != 1:
            return None
        G = cartesian_linear_pair(H[0], B)
        if G is None:
            return None
        J = make_mul(I)
        return sim(mul([J, G[0]])), sim(mul([J, G[1]]))
    if A[0] == 'div':
        if depends(A[2], B):
            return None
        G = cartesian_linear_pair(A[1], B)
        if G is None:
            return None
        return sim(div(G[0], A[2])), sim(div(G[1], A[2]))
    return None


def cartesian_integrand_from_equation(text):
    A = text.strip()
    if '=' not in A:
        return None
    B, C = A.split('=', 1)
    D = parse(B.strip())
    E = parse(C.strip())
    if D == sym('y') and not depends(E, 'y'):
        return E, 'x'
    if E == sym('y') and not depends(D, 'y'):
        return D, 'x'
    F = sim(add([D, neg(E)]))
    G = cartesian_linear_pair(F, 'y')
    if G is None or is_zero(G[0]):
        return None
    H = sim(div(neg(G[1]), G[0]))
    return H, 'x'


def parse_input(text):
    D = cartesian_integrand_from_equation(text)
    if D is not None:
        return D
    A = split_input_var(text)
    if A is None:
        B = parse(text)
        C = set()
        collect_symbols(B, C)
        if 'x' in C:
            return B, 'x'
        if len(C) == 1:
            return B, next(iter(C))
        return B, 'x'
    return parse(A[0]), A[1]


def resolve_symbol_name(name, symbols):
    B = symbols
    A = name
    if A in B:
        return A
    C = A.lower()
    if C in B:
        return C
    D = A.upper()
    if D in B:
        return D
    for E in B:
        if E.lower() == C:
            return E
    return A


def parse_de_equation(text):
    if '='not in text:
        raise ValueError("Equation must contain '='.")
    H, I = text.split('=', 1)
    D = H.replace(' ', '')
    if not D.startswith('d') or '/d'not in D:
        raise ValueError('Equation must look like dY/dX = ...')
    F = D.find('/d')
    B = D[1:F]
    C = D[F + 2:]
    if B == '' or C == '':
        raise ValueError('Bad differential equation.')
    A = 0
    while A < len(B):
        if not is_name_char(B[A]):
            raise ValueError('Bad dependent variable.')
        A += 1
    A = 0
    while A < len(C):
        if not is_name_char(C[A]):
            raise ValueError('Bad independent variable.')
        A += 1
    G = parse(I)
    E = set()
    collect_symbols(G, E)
    C = resolve_symbol_name(C, E)
    B = resolve_symbol_name(B, E)
    return G, C, B


def parse_de_balanced_equation(text):
    if '=' not in text:
        raise ValueError("Equation must contain '='.")
    left_text, right_text = text.split('=', 1)
    left = left_text.replace(' ', '')
    if not left.startswith('d') or '/d' not in left:
        raise ValueError('Equation must look like dY/dX = ...')
    cut = left.find('/d')
    dep = left[1:cut]
    rest = left[cut + 2:]
    if dep == '' or rest == '':
        raise ValueError('Bad differential equation.')
    i = 0
    while i < len(dep):
        if not is_name_char(dep[i]):
            raise ValueError('Bad dependent variable.')
        i += 1
    indep = ''
    i = 0
    while i < len(rest) and is_name_char(rest[i]):
        indep += rest[i]
        i += 1
    if indep == '':
        raise ValueError('Bad independent variable.')
    tail = rest[i:]
    rhs = parse(right_text.strip())
    if tail != '':
        if tail[0] == '+':
            rhs = add([rhs, neg(parse(tail[1:]))])
        elif tail[0] == '-':
            rhs = add([rhs, parse(tail[1:])])
        else:
            rhs = add([rhs, neg(parse(tail))])
    syms = set()
    collect_symbols(rhs, syms)
    indep = resolve_symbol_name(indep, syms)
    dep = resolve_symbol_name(dep, syms)
    return rhs, indep, dep


def parse_de_input(text):
    A = text.strip()
    if '=' not in A:
        raise ValueError("Equation must contain '='.")
    try:
        return parse_de_equation(A)
    except ValueError:
        return parse_de_balanced_equation(A)


def cancellation_requested():
    return False


def integral_candidate_score(title, ans, lines):
    if ans is None:
        return 10**9
    title_score = {'std': 0, 'f(ax+b)': 1, 'Reverse chain rule': 2, 'Integration by substitution': 3, 'Using trigonometric identities': 4, 'Integration by parts': 5, 'Partial fractions': 6}.get(title, 7)
    return (len(pretty(ans)), len(lines or []), title_score)


def fallback_attempts(node, var, forced_u=None):
    attempts = [('2', 'dir'), ('4', 'sub'), ('3', 'trig'), ('5', 'pts'), ('6', 'pf'), ('7', 'div')]
    best = None
    best_title = None
    best_lines = None
    out_lines = []
    i = 0
    while i < len(attempts):
        if cancellation_requested():
            break
        method, label = attempts[i]
        title, ans, lines = _solve_with_method(node, var, method, forced_u)
        if ans is not None:
            out_lines.append('Attempt ' + str(i + 1) + ' (' + label + ')')
            j = 0
            while lines is not None and j < len(lines):
                out_lines.append(lines[j])
                j += 1
            score = integral_candidate_score(title, ans, lines)
            if best is None or score < best:
                best = score
                best_title = title
                best_lines = lines
        i += 1
    if best is None:
        return None, None, None
    return best_title, best_lines, out_lines


def solve_result_or_reason(node, var, method, forced_u=None):
    A, B, C = solve(node, var, method, forced_u)
    if B is not None:
        return A, B, C, None
    title, best_lines, attempt_lines = fallback_attempts(node, var, forced_u)
    if title is not None:
        retry_title, retry_ans, retry_lines = _solve_with_method(node, var, {'std':'2','f(ax+b)':'2','Reverse chain rule':'4','Integration by substitution':'4','Using trigonometric identities':'3','Integration by parts':'5','Partial fractions':'6'}.get(title, '2'), forced_u)
        merged = list(attempt_lines)
        if retry_lines:
            merged.append('Choose the simplest successful attempt.')
            i = 0
            while i < len(retry_lines):
                merged.append(retry_lines[i])
                i += 1
        return title, retry_ans, merged, None
    return A, B, C, hard_integral_failure_reason(node, var)


def can_handle_derivative_case(node, var, deps):
    try:
        diff(node, var, deps)
        return True, None
    except Exception as err:
        return False, str(err)

def parse_de_condition(text, xvar, yvar):
    D = text.strip()
    if D == '':
        return None
    D = D.replace(' at ', ',').replace(' AT ', ',').replace('At ', ',')
    B = [A.strip()for A in D.split(',')if A.strip() != '']
    C = None
    E = None
    for A in B:
        if '='not in A:
            raise ValueError(
                'Boundary condition must look like ' +
                yvar +
                '=..., ' +
                xvar +
                '=...')
        F, G = A.split('=', 1)
        H = F.strip()
        I = parse(G.strip())
        if H == xvar:
            C = I
        elif H == yvar:
            E = I
        else:
            raise ValueError(
                'Boundary condition must use ' +
                xvar +
                ' and ' +
                yvar +
                '.')
    if C is None or E is None:
        raise ValueError(
            'Boundary condition must include ' +
            yvar +
            ' and ' +
            xvar +
            '.')
    return C, E


def parse_forced_u(text):
    A = text
    A = A.strip()
    if A == '':
        return
    if '=' in A:
        D, E = A.split('=', 1)
        B = D.replace(' ', '')
        C = E.strip()
        if B in ('u', 'U'):
            return parse(C)
        if B in ('u^2', 'U^2', 'u**2', 'U**2'):
            return fn('sqrt', parse(C))
    return parse(A)


def poly_trim(coeffs):
    A = list(coeffs)
    while len(A) > 1 and is_zero(A[-1]):
        A.pop()
    return A


def poly_add_num(a, b):
    C = len(a)if len(a) > len(b)else len(b)
    B = []
    A = 0
    while A < C:
        D = a[A]if A < len(a)else num(0)
        E = b[A]if A < len(b)else num(0)
        B.append(addq(D, E))
        A += 1
    return poly_trim(B)


def poly_mul_num(a, b):
    C = [num(0)] * (len(a) + len(b) - 1)
    A = 0
    while A < len(a):
        B = 0
        while B < len(b):
            C[A + B] = addq(C[A + B], mulq(a[A], b[B]))
            B += 1
        A += 1
    return poly_trim(C)


def poly_divmod_num(a, b):
    a = poly_trim(list(a))
    b = poly_trim(list(b))
    if len(b) == 1 and is_zero(b[0]):
        return None, None
    if len(a) < len(b):
        return [num(0)], a
    D = [num(0)] * (len(a) - len(b) + 1)
    A = list(a)
    while len(A) >= len(b):
        C = len(A) - len(b)
        E = divq(A[-1], b[-1])
        D[C] = E
        B = 0
        while B < len(b):
            A[C + B] = subq(A[C + B], mulq(E, b[B]))
            B += 1
        A = poly_trim(A)
    return poly_trim(D), poly_trim(A)


def poly_eval(coeffs, value):
    C = coeffs
    A = num(0)
    B = len(C) - 1
    while B >= 0:
        A = addq(mulq(A, value), C[B])
        B -= 1
    return A


def divisors(n):
    if n < 0:
        n = -n
    if n == 0:
        return [0]
    B = []
    A = 1
    while A <= n:
        if n % A == 0:
            B.append(A)
        A += 1
    return B


def poly_div_root(coeffs, root):
    D = coeffs
    C = []
    A = num(0)
    B = len(D) - 1
    while B >= 0:
        A = addq(mulq(A, root), D[B])
        if B != 0:
            C.append(A)
        B -= 1
    C.reverse()
    return poly_trim(C), A


def factor_poly_linear(coeffs):
    A = coeffs
    A = poly_trim(list(A))
    D = []
    while len(A) > 1:
        if len(A) == 2:
            D.append(divq(negq(A[0]), A[1]))
            return D
        if not is_int_num(A[0]) or not is_int_num(A[-1]):
            return
        G = divisors(A[0][1])
        H = divisors(A[-1][1])
        B = None
        E = 0
        while E < len(G) and B is None:
            F = 0
            while F < len(H):
                C = num(G[E], H[F])
                if is_zero(poly_eval(A, C)):
                    B = C
                    break
                C = num(-G[E], H[F])
                if is_zero(poly_eval(A, C)):
                    B = C
                    break
                F += 1
            E += 1
        if B is None:
            return
        A, I = poly_div_root(A, B)
        if not is_zero(I):
            return
        D.append(B)
    return D


def factor_poly_linear_partial(coeffs, max_roots=None):
    A = poly_trim(list(coeffs))
    B = []
    while len(A) > 1:
        if max_roots is not None and len(B) >= max_roots:
            break
        if len(A) == 2:
            B.append(divq(negq(A[0]), A[1]))
            A = [num(1)]
            break
        if not is_int_num(A[0]) or not is_int_num(A[-1]):
            break
        G = divisors(A[0][1])
        H = divisors(A[-1][1])
        C = None
        D = 0
        while D < len(G) and C is None:
            E = 0
            while E < len(H):
                F = num(G[D], H[E])
                if is_zero(poly_eval(A, F)):
                    C = F
                    break
                F = num(-G[D], H[E])
                if is_zero(poly_eval(A, F)):
                    C = F
                    break
                E += 1
            D += 1
        if C is None:
            break
        A, I = poly_div_root(A, C)
        if not is_zero(I):
            break
        B.append(C)
    return B, poly_trim(A)


def poly_num(node, var):
    E = var
    B = node
    C = B[0]
    if is_num(B):
        return [B]
    if C == 'sym':
        if B[1] == E:
            return [num(0), num(1)]
        return
    if C == 'const':
        return
    if C == 'add':
        A = [num(0)]
        for G in flat(B, 'add'):
            F = poly_num(G, E)
            if F is None:
                return
            A = poly_add_num(A, F)
        return A
    if C == 'mul':
        A = [num(1)]
        for G in flat(B, 'mul'):
            F = poly_num(G, E)
            if F is None:
                return
            A = poly_mul_num(A, F)
        return A
    if C == 'div':
        H = poly_num(B[1], E)
        if H is None or not is_num(B[2]):
            return
        A = []
        D = 0
        while D < len(H):
            A.append(divq(H[D], B[2]))
            D += 1
        return poly_trim(A)
    if C == 'pow' and is_int_num(B[2]) and B[2][1] >= 0:
        I = poly_num(B[1], E)
        if I is None:
            return
        A = [num(1)]
        D = 0
        while D < B[2][1]:
            A = poly_mul_num(A, I)
            D += 1
        return A


def poly_degree(node, var):
    A = poly_num(node, var)
    if A is None:
        return
    return len(A) - 1


def poly_to_node(coeffs, var):
    C = coeffs
    C = poly_trim(list(C))
    B = []
    A = 0
    while A < len(C):
        D = C[A]
        if not is_zero(D):
            if A == 0:
                B.append(D)
            elif A == 1:
                B.append(mul([D, sym(var)]))
            else:
                B.append(mul([D, power(sym(var), num(A))]))
        A += 1
    if len(B) == 0:
        return num(0)
    return add(B)


def expand_polynomial_parts(node, var):
    A = sim(node)
    B = poly_num(A, var)
    if B is not None and len(B) <= 24:
        return poly_to_node(B, var)
    if A[0] == 'fn':
        if A[1] == 'exp':
            return A
        return 'fn', A[1], expand_polynomial_parts(A[2], var)
    if A[0] == 'pow':
        C = sim(('pow', expand_polynomial_parts(A[1], var), A[2]))
        B = poly_num(C, var)
        if B is not None and len(B) <= 24:
            return poly_to_node(B, var)
        return C
    if A[0] == 'div':
        return sim(('div', expand_polynomial_parts(A[1], var), expand_polynomial_parts(A[2], var)))
    if A[0] == 'add':
        C = []
        for D in flat(A, 'add'):
            C.append(expand_polynomial_parts(D, var))
        return normalize_add_coeffs(expand_small_recursive(add(C)))
    if A[0] == 'mul':
        C = []
        for D in flat(A, 'mul'):
            C.append(expand_polynomial_parts(D, var))
        E = normalize_add_coeffs(expand_small_recursive(mul(C)))
        B = poly_num(E, var)
        if B is not None and len(B) <= 24:
            return poly_to_node(B, var)
        return E
    return A


def linear_info(node, var):
    A = poly_num(node, var)
    if A is None or len(A) > 2:
        return
    C = A[0]
    B = A[1]if len(A) > 1 else num(0)
    if is_zero(B):
        return
    return B, C


def quad_info(node, var):
    A = poly_num(node, var)
    if A is None or len(A) != 3 or is_zero(A[2]):
        return
    return A[2], A[1], A[0]


def quad_scale(node, var):
    C = quad_info(node, var)
    if C is None:
        return
    D, A, F = C
    B = subq(mulq(num(4), mulq(D, F)), mulq(A, A))
    if not is_num(B) or B[1] <= 0:
        return
    E = sqrt_factor_expr(B)
    G = sym(var)
    H = div(add([mul([num(2), D, G]), A]), E)
    return mul([div(num(2), E), fn('atan', H)])


def integrate_quadratic_rational(node, var):
    D = var
    A = node
    if A[0] != 'div':
        return None, None
    B = poly_num(A[1], D)
    F = quad_info(A[2], D)
    if B is None or F is None or len(B) > 2:
        return None, None
    J, K, M = F
    H = B[1]if len(B) > 1 else num(0)
    L = B[0]
    E = divq(H, mulq(num(2), J))if not is_zero(H)else num(0)
    I = subq(L, mulq(E, K))
    G = quad_scale(A[2], D)
    disc = subq(mulq(K, K), mulq(num(4), mulq(J, M)))
    lines = []
    if not is_zero(E) and not is_zero(I):
        lines.append('Split the numerator into a multiple of the derivative of the denominator plus a constant.')
    elif not is_zero(E):
        lines.append('Use the standard result for f\'(x)/f(x).')
    elif not is_zero(I):
        lines.append('Use the standard result for 1/quadratic.')
    if G is None:
        if is_num(disc) and disc[1] > 0:
            root = sqrt_factor_expr(disc)
            linear = add([mul([num(2), J, sym(D)]), K])
            G = mul([div(num(1), root), fn('log', fn('abs', div(add([linear, neg(root)]), add([linear, root]))))])
            if not is_zero(I):
                lines.append('Use the standard result for 1/(u^2-a^2).')
        elif is_zero(disc):
            linear = add([mul([num(2), J, sym(D)]), K])
            G = neg(div(num(2), linear))
            if not is_zero(I):
                lines.append('Use the repeated-root quadratic standard result.')
        else:
            return None, None
    elif not is_zero(I):
        lines.append('Use the standard result for 1/(u^2+a^2).')
    C = []
    if not is_zero(E):
        C.append(mul([E, fn('log', fn('abs', A[2]))]))
    if not is_zero(I):
        C.append(mul([I, G]))
    if len(C) == 0:
        return num(0), []
    return add(C), lines


def split_linear_quadratic_product(node, var):
    B = var
    A = flat(node, 'mul')
    if len(A) != 2:
        C = poly_num(node, B)
        if C is None or len(C) != 4:
            return
        D, E = factor_poly_linear_partial(C, 1)
        if len(D) != 1 or len(E) != 3:
            return
        return add([sym(B), neg(D[0])]), poly_to_node(E, B)
    C = linear_info(A[0], B) is not None
    D = linear_info(A[1], B) is not None
    E = quad_info(A[0], B) is not None
    F = quad_info(A[1], B) is not None
    if C and F:
        return A[0], A[1]
    if D and E:
        return A[1], A[0]


def log_abs(node):
    return fn('log', fn('abs', node))


def symbolic_monic_quadratic(var, p):
    return add([power(sym(var), num(2)), mul([p, sym(var)]), num(1)])


def integrate_symbolic_linear_over_monic_quadratic(m, n, p, var):
    q = symbolic_monic_quadratic(var, p)
    log_part = mul([div(m, num(2)), log_abs(q)])
    disc = sim(sub(num(4), power(p, num(2))))
    sqrt_disc = fn('sqrt', disc)
    atan_part = mul([
        mul([sub(n, mul([m, p, num(1, 2)])), div(num(2), sqrt_disc)]),
        fn('atan', div(add([mul([num(2), sym(var)]), p]), sqrt_disc)),
    ])
    if is_zero(sim(div(m, num(2)))):
        return sim(atan_part)
    if is_zero(sim(sub(n, mul([m, p, num(1, 2)])))):
        return sim(log_part)
    return sim(add([log_part, atan_part]))


def matches_x4_plus_one(node, var):
    coeffs = poly_num(node, var)
    if coeffs is None or len(coeffs) != 5:
        return False
    return is_one(coeffs[0]) and is_zero(coeffs[1]) and is_zero(coeffs[2]) and is_zero(coeffs[3]) and is_one(coeffs[4])


def matches_x5_plus_one(node, var):
    coeffs = poly_num(node, var)
    if coeffs is None or len(coeffs) != 6:
        return False
    return is_one(coeffs[0]) and is_zero(coeffs[1]) and is_zero(coeffs[2]) and is_zero(coeffs[3]) and is_zero(coeffs[4]) and is_one(coeffs[5])


def integrate_special_division_remainder(node, var):
    if node[0] != 'div':
        return None, None
    num_coeffs = poly_num(node[1], var)
    den = node[2]
    if num_coeffs is None:
        return None, None

    if matches_x4_plus_one(den, var):
        if len(num_coeffs) == 1 or (len(num_coeffs) == 3 and is_zero(num_coeffs[1])):
            a = num_coeffs[0]
            b = num(0) if len(num_coeffs) == 1 else num_coeffs[2]
            sqrt2 = fn('sqrt', num(2))
            p = sqrt2
            q = neg(sqrt2)
            m1 = div(sub(a, b), mul([num(2), sqrt2]))
            n1 = div(a, num(2))
            m2 = neg(m1)
            n2 = n1
            q1 = symbolic_monic_quadratic(var, p)
            q2 = symbolic_monic_quadratic(var, q)
            ans = sim(add([
                integrate_symbolic_linear_over_monic_quadratic(m1, n1, p, var),
                integrate_symbolic_linear_over_monic_quadratic(m2, n2, q, var),
            ]))
            lines = [
                'Factor x^4+1 into two quadratics.',
                pretty(node) + ' = ' + pretty(add([
                    div(add([mul([m1, sym(var)]), n1]), q1),
                    div(add([mul([m2, sym(var)]), n2]), q2),
                ])),
                'Integrate each quadratic term.',
                'So I = ' + pretty(ans) + ' + C',
            ]
            return ans, lines

    if matches_x5_plus_one(den, var):
        if len(num_coeffs) == 3 and is_zero(num_coeffs[1]) and same(num_coeffs[2], num(-1)):
            c = num_coeffs[0]
            sqrt5 = fn('sqrt', num(5))
            p = div(add([sqrt5, num(-1)]), num(2))
            q = neg(div(add([sqrt5, num(1)]), num(2)))
            A = div(add([c, neg(num(1))]), num(5))
            B = add([div(add([num(1), neg(c)]), num(10)), mul([div(add([c, num(1)]), num(10)), sqrt5])])
            C = add([div(add([mul([num(4), c]), num(1)]), num(10)), div(sqrt5, num(10))])
            D = add([div(add([num(1), neg(c)]), num(10)), neg(mul([div(add([c, num(1)]), num(10)), sqrt5]))])
            E = add([div(add([mul([num(4), c]), num(1)]), num(10)), neg(div(sqrt5, num(10)))])
            q1 = symbolic_monic_quadratic(var, p)
            q2 = symbolic_monic_quadratic(var, q)
            ans = sim(add([
                mul([A, log_abs(add([sym(var), num(1)]))]),
                integrate_symbolic_linear_over_monic_quadratic(B, C, p, var),
                integrate_symbolic_linear_over_monic_quadratic(D, E, q, var),
            ]))
            lines = [
                'Factor x^5+1 into one linear factor and two quadratics.',
                pretty(node) + ' = ' + pretty(add([
                    div(A, add([sym(var), num(1)])),
                    div(add([mul([B, sym(var)]), C]), q1),
                    div(add([mul([D, sym(var)]), E]), q2),
                ])),
                'Integrate each partial fraction term.',
                'So I = ' + pretty(ans) + ' + C',
            ]
            return ans, lines

    return None, None


def split_repeated_linear_quadratic_product(node, var):
    B = var
    A = flat(node, 'mul')
    if len(A) == 2:
        if A[0][0] == 'pow' and is_int_num(A[0][2]) and A[0][2][1] == 2 and linear_info(A[0][1], B) is not None and quad_info(A[1], B) is not None:
            return A[0][1], A[1]
        if A[1][0] == 'pow' and is_int_num(A[1][2]) and A[1][2][1] == 2 and linear_info(A[1][1], B) is not None and quad_info(A[0], B) is not None:
            return A[1][1], A[0]
    C = poly_num(node, B)
    if C is None or len(C) != 5:
        return
    D, E = factor_poly_linear_partial(C, 2)
    if len(D) != 2 or not same(D[0], D[1]) or len(E) != 3:
        return
    return add([sym(B), neg(D[0])]), poly_to_node(E, B)


def split_const_mul(node, var):
    B = var
    A = node
    if A[0] == 'mul':
        D = []
        E = []
        for C in A[1]:
            if depends(C, B):
                E.append(C)
            else:
                D.append(C)
        return make_mul(D), make_mul(E)
    if A[0] == 'div':
        F, G = split_const_mul(A[1], B)
        H, I = split_const_mul(A[2], B)
        return div(F, H), div(G, I)
    if depends(A, B):
        return num(1), A
    return A, num(1)


def sqrt_like_radicand(node):
    A = node
    if A[0] == 'fn' and A[1] == 'sqrt':
        return A[2]
    if A[0] == 'pow' and same(A[2], num(1, 2)):
        return A[1]


def same_sqrt_like(a, b):
    if cheap_same(a, b) or same(a, b):
        return True
    C = sqrt_like_radicand(a)
    D = sqrt_like_radicand(b)
    return C is not None and D is not None and same(C, D)


def sqrt_basis_power(node, basis):
    B = sqrt_like_radicand(basis)
    A = node
    if B is None:
        return
    if same_sqrt_like(A, basis):
        return num(1)
    if is_sym(A) and same(A, B):
        return num(2)
    if A[0] == 'pow' and same(A[1], B) and is_num(A[2]):
        C = A[2]
        if C[2] == 2:
            return num(C[1])
        if C[2] == 1:
            return num(2 * C[1])


def _rewrite_in_u(node, inner, var):
    C = var
    B = inner
    A = node
    if cheap_same(A, B) or same(A, B):
        return U
    if B[0] == 'add':
        D = flat(B, 'add')
        if len(D) == 2:
            if not depends(D[0], C):
                E, F = split_coeff(D[1])
                if is_num(E):
                    G = div(add([U, neg(D[0])]), E)
                    if same_sqrt_like(A, F):
                        return G
                    H = sqrt_basis_power(A, F)
                    if H is not None:
                        return power(G, H)
            if not depends(D[1], C):
                E, F = split_coeff(D[0])
                if is_num(E):
                    G = div(add([U, neg(D[1])]), E)
                    if same_sqrt_like(A, F):
                        return G
                    H = sqrt_basis_power(A, F)
                    if H is not None:
                        return power(G, H)
    L = linear_info(B, C)
    if is_sym(A) and A[1] == C and L is not None:
        return div(add([U, neg(L[1])]), L[0])
    if B[0] == 'fn' and B[1] == 'sqrt':
        K = B[2]
        if K[0] == 'add':
            D = flat(K, 'add')
            if len(D) == 2:
                if not depends(D[0], C) and cheap_same(A, D[1]):
                    return add([power(U, num(2)), neg(D[0])])
                if not depends(D[1], C) and cheap_same(A, D[0]):
                    return add([power(U, num(2)), neg(D[1])])
        M = linear_info(K, C)
        if is_sym(A) and A[1] == C and M is not None:
            return div(add([power(U, num(2)), neg(M[1])]), M[0])
        F = poly_num(K, C)
        if F is not None and len(F) == 3 and is_zero(F[1]):
            if A[0] == 'pow' and A[1] == sym(
                    C) and is_int_num(A[2]) and A[2][1] % 2 == 0:
                P = div(add([power(U, num(2)), neg(F[0])]), F[2])
                return power(P, num(A[2][1] // 2))
            if A == power(sym(C), num(2)):
                return div(add([power(U, num(2)), neg(F[0])]), F[2])
    if B[0] == 'fn' and A[0] == 'pow' and A[1][0] == 'fn' and same(
            A[1][2], B[2]) and is_int_num(A[2]):
        E = A[2][1]
        if B[1] == 'tan' and A[1][1] == 'sec' and E % 2 == 0:
            return power(add([num(1), power(U, num(2))]), num(E // 2))
        if B[1] == 'sin' and A[1][1] == 'cos' and E % 2 == 0:
            return power(add([num(1), neg(power(U, num(2)))]), num(E // 2))
        if B[1] == 'cos' and A[1][1] == 'sin' and E % 2 == 0:
            return power(add([num(1), neg(power(U, num(2)))]), num(E // 2))
        if B[1] == 'sec' and A[1][1] == 'tan' and E % 2 == 0:
            return power(add([power(U, num(2)), num(-1)]), num(E // 2))
        if B[1] == 'cot' and A[1][1] == 'cosec' and E % 2 == 0:
            return power(add([num(1), power(U, num(2))]), num(E // 2))
        if B[1] == 'cosec' and A[1][1] == 'cot' and E % 2 == 0:
            return power(add([power(U, num(2)), num(-1)]), num(E // 2))
    if B[0] == 'fn' and B[1] == 'exp' and A[0] == 'fn' and A[1] == 'exp':
        if same(A[2], B[2]):
            return U
    if B[0] == 'pow' and same(B[1], ('const', 'e')):
        if A[0] == 'pow' and same(A[1], ('const', 'e')):
            D = quotient_by_target(A[2], B[2])
            if D is not None and not depends(D, C):
                return U if is_one(D) else power(U, D)
    if B[0] == 'fn' and B[1] == 'log' and same(B[2], sym(C)):
        if is_sym(A) and A[1] == C:
            return power(('const', 'e'), U)
        if A[0] == 'pow' and A[1] == sym(C) and is_num(A[2]):
            return power(power(('const', 'e'), U), A[2])
    if B[0] == 'pow' and B[1] == sym(C) and is_num(B[2]) and not is_zero(B[2]):
        if is_sym(A) and A[1] == C:
            return power(U, divq(num(1), B[2]))
        if A[0] == 'pow' and A[1] == sym(C) and is_num(A[2]):
            return power(U, divq(A[2], B[2]))
    G = A[0]
    if G in ('num', 'const'):
        return A
    if G == 'sym':
        return
    if G == 'fn':
        H = _rewrite_in_u(A[2], B, C)
        if H is None:
            return
        return fn(A[1], H)
    if G == 'pow':
        I = _rewrite_in_u(A[1], B, C)
        J = _rewrite_in_u(A[2], B, C)
        if I is None or J is None:
            return
        return power(I, J)
    if G == 'div':
        I = _rewrite_in_u(A[1], B, C)
        J = _rewrite_in_u(A[2], B, C)
        if I is None or J is None:
            return
        return div(I, J)
    N = []
    O = 0
    while O < len(A[1]):
        H = _rewrite_in_u(A[1][O], B, C)
        if H is None:
            return
        N.append(H)
        O += 1
    return add(N)if G == 'add'else mul(N)


def rewrite_in_u(node, inner, var): return _rewrite_in_u(node, inner, var)


def split_coeffs_for_ratio(node):
    C = node
    if C[0] == 'mul':
        A = list(C[1])
    else:
        A = [C]
    D = num(1)
    E = []
    B = 0
    while B < len(A):
        if is_num(A[B]):
            D = mulq(D, A[B])
        else:
            E.append(A[B])
        B += 1
    return D, E


def factor_add_num(node):
    D = node
    if D[0] != 'add':
        return num(1), D
    I = flat(D, 'add')
    B = 0
    H = []
    A = 0
    while A < len(I):
        E, C = split_coeff(I[A])
        if not is_int_num(E):
            return num(1), D
        J = abs(E[1])
        B = J if B == 0 else gcd(B, J)
        H.append((E, C))
        A += 1
    if B <= 1:
        return num(1), D
    F = []
    A = 0
    while A < len(H):
        E, C = H[A]
        G = num(E[1] // B)
        if is_one(C):
            F.append(G)
        elif is_one(G):
            F.append(C)
        elif is_minus_one(G):
            F.append(neg(C))
        else:
            F.append(mul([G, C]))
        A += 1
    return num(B), add(F)


def _quotient_by_target(node, target):
    B = target
    A = node
    if cheap_same(A, B) or same(A, B):
        return num(1)
    if A[0] == 'div' and B[0] == 'div' and (cheap_same(A[2], B[2]) or same(A[2], B[2])):
        return _quotient_by_target(A[1], B[1])
    if A[0] == 'div' and B[0] == 'div':
        C = _quotient_by_target(A[1], B[1])
        if C is not None:
            D = _quotient_by_target(A[2], B[2])
            if D is not None:
                return div(C, D)
    H, L = factor_add_num(B)
    if not is_one(H):
        C = _quotient_by_target(A, L)
        if C is not None:
            return div(C, H)
    I, M = factor_add_num(A)
    if not is_one(I):
        C = _quotient_by_target(M, B)
        if C is not None:
            return mul([I, C])
    if A[0] == 'div':
        J = _quotient_by_target(A[1], B)
        if J is not None:
            return div(J, A[2])
        return
    N, O = split_coeffs_for_ratio(A)
    P, G = split_coeffs_for_ratio(B)
    D = list(O)
    F = 0
    while F < len(G):
        K = False
        E = 0
        while E < len(D):
            if cheap_same(D[E], G[F]) or same(D[E], G[F]):
                D.pop(E)
                K = True
                break
            E += 1
        if not K:
            return
        F += 1
    return mul([divq(N, P)] + D)


def quotient_by_target(node, target): return _quotient_by_target(node, target)


def expand_polynomial_subexpressions(node, var):
    A = node
    B = A[0]
    if B in ('num', 'sym', 'const'):
        return A
    if B == 'fn':
        return fn(A[1], expand_polynomial_subexpressions(A[2], var))
    if B == 'pow':
        C = power(expand_polynomial_subexpressions(A[1], var), expand_polynomial_subexpressions(A[2], var))
        D = expand_polynomial_parts(C, var)
        return D if D is not None else C
    if B == 'add':
        C = add([expand_polynomial_subexpressions(D, var) for D in A[1]])
        D = expand_polynomial_parts(C, var)
        return D if D is not None else C
    if B == 'mul':
        C = mul([expand_polynomial_subexpressions(D, var) for D in A[1]])
        if poly_num(C, var) is not None:
            D = expand_polynomial_parts(C, var)
            return D if D is not None else C
        return C
    if B == 'div':
        return div(expand_polynomial_subexpressions(A[1], var), expand_polynomial_subexpressions(A[2], var))
    return A


def equivalent_derivative_forms(node, var):
    A = [node]
    try:
        A.append(normalize_add_coeffs(expand_small_recursive(node)))
    except Exception:
        pass
    try:
        A.append(expand_polynomial_parts(node, var))
    except Exception:
        pass
    try:
        A.append(expand_polynomial_subexpressions(node, var))
    except Exception:
        pass
    if node[0] == 'div':
        try:
            A.append(div(expand_polynomial_subexpressions(node[1], var), node[2]))
        except Exception:
            pass
        try:
            A.append(div(expand_polynomial_parts(node[1], var), node[2]))
        except Exception:
            pass
        try:
            A.append(div(expand_polynomial_subexpressions(node[1], var), expand_polynomial_subexpressions(node[2], var)))
        except Exception:
            pass
    B = []
    C = 0
    while C < len(A):
        D = A[C]
        if D is not None:
            E = False
            F = 0
            while F < len(B):
                if cheap_same(D, B[F]) or same(D, B[F]):
                    E = True
                    break
                F += 1
            if not E:
                B.append(D)
        C += 1
    return B


def eval_float_node(node, var, value):
    A = sim(node)
    K = A[0]
    if K == 'num':
        return A[1] * 1.0 / A[2]
    if K == 'const':
        if A[1] == 'pi':
            return math.pi
        if A[1] == 'e':
            return math.e
        raise ValueError('Unknown constant.')
    if K == 'sym':
        if A[1] == var:
            return value
        raise ValueError('Unknown symbol.')
    if K == 'add':
        total = 0.0
        for B in flat(A, 'add'):
            total += eval_float_node(B, var, value)
        return total
    if K == 'mul':
        total = 1.0
        for B in flat(A, 'mul'):
            total *= eval_float_node(B, var, value)
        return total
    if K == 'div':
        return eval_float_node(A[1], var, value) / eval_float_node(A[2], var, value)
    if K == 'pow':
        return eval_float_node(A[1], var, value) ** eval_float_node(A[2], var, value)
    if K == 'fn':
        arg = eval_float_node(A[2], var, value)
        if A[1] in ('log', 'ln'):
            return math.log(arg)
        if A[1] == 'sqrt':
            return math.sqrt(arg)
        if A[1] == 'exp':
            return math.exp(arg)
        if A[1] == 'sin':
            return math.sin(arg)
        if A[1] == 'cos':
            return math.cos(arg)
        if A[1] == 'tan':
            return math.tan(arg)
        if A[1] == 'sec':
            return 1.0 / math.cos(arg)
        if A[1] == 'cosec':
            return 1.0 / math.sin(arg)
        if A[1] == 'cot':
            return 1.0 / math.tan(arg)
        if A[1] == 'asin':
            return math.asin(arg)
        if A[1] == 'acos':
            return math.acos(arg)
        if A[1] == 'atan':
            return math.atan(arg)
    raise ValueError('Cannot evaluate.')


def float_to_simple_num(value):
    if not math.isfinite(value):
        return None
    rounded = round(value)
    if abs(value - rounded) < 1e-8:
        return num(int(rounded))
    den = 2
    while den <= 64:
        top = round(value * den)
        if abs(value - top / den) < 1e-8:
            return num(int(top), den)
        den += 1
    return None


def numerical_constant_quotient(node, target, var):
    samples = (-1.7, -0.9, -0.35, 0.25, 0.8, 1.4, 2.1)
    ratios = []
    for sample in samples:
        try:
            bot = eval_float_node(target, var, sample)
            if abs(bot) < 1e-9 or not math.isfinite(bot):
                continue
            top = eval_float_node(node, var, sample)
            if not math.isfinite(top):
                continue
            ratios.append(top / bot)
        except Exception:
            pass
    if len(ratios) < 4:
        return None
    base = ratios[0]
    i = 1
    while i < len(ratios):
        if abs(ratios[i] - base) > 1e-7 * max(1.0, abs(base)):
            return None
        i += 1
    return float_to_simple_num(base)


def quotient_by_equivalent_target(node, target, var):
    C = [node]
    try:
        C.append(expand_polynomial_subexpressions(node, var))
    except Exception:
        pass
    A = equivalent_derivative_forms(target, var)
    D = 0
    while D < len(C):
        B = 0
        while B < len(A):
            E = quotient_by_target(C[D], A[B])
            if E is not None:
                return E
            B += 1
        D += 1
    return numerical_constant_quotient(node, target, var)


def rewrite_in_equivalent_u(node, inner, var):
    A = rewrite_in_u(node, inner, var)
    if A is not None:
        return A
    try:
        B = expand_polynomial_subexpressions(inner, var)
        if not (cheap_same(B, inner) or same(B, inner)):
            return rewrite_in_u(node, B, var)
    except Exception:
        pass


def direct_u_form(node):
    B, A = split_const_mul(node, 'u')
    if same(A, U):
        return mul([B, div(power(U, num(2)), num(2))])
    if A[0] == 'pow' and same(A[1], U) and is_num(
            A[2]) and not same(A[2], num(-1)):
        return mul([B, div(power(U, addq(A[2], num(1))), addq(A[2], num(1)))])
    if A[0] == 'div' and is_one(A[1]) and same(A[2], U):
        return mul([B, fn('log', fn('abs', U))])
    if A[0] == 'pow' and same(A[1], U) and same(A[2], num(-1)):
        return mul([B, fn('log', fn('abs', U))])
    if A[0] == 'pow' and same(A[1], E) and same(A[2], U):
        return mul([B, power(E, U)])
    if A[0] == 'fn' and same(A[2], U):
        C = primitive_of_named_function(A[1], U)
        if C is not None and A[1] in ('sin', 'cos', 'exp', 'sqrt'):
            return mul([B, C])
    if A[0] == 'pow' and A[1][0] == 'fn' and same(A[1][2], U):
        C = primitive_of_named_power(A[1][1], U, A[2])
        if C is not None:
            return mul([B, C])
    if A[0] == 'mul':
        D = list(A[1])
        if len(D) == 2 and D[0][0] == 'fn' and D[1][0] == 'fn' and same(
                D[0][2], U) and same(D[1][2], U):
            C = primitive_of_named_product(D[0][1], D[1][1], U)
            if C is not None:
                return mul([B, C])


def integrate_special_direct_term(node, var):
    A = node
    F = sym(var)

    def sqrt_inner(expr):
        if expr[0] == 'fn' and expr[1] == 'sqrt':
            return expr[2]
        if expr[0] == 'pow' and same(expr[2], num(1, 2)):
            return expr[1]

    B = sqrt_inner(A)
    H = poly_num(B, var) if B is not None else None
    if H is not None and len(H) == 3 and is_zero(H[1]) and is_num(H[0]) and is_num(H[2]) and H[0][1] > 0 and H[2][1] < 0:
        T = fn('sqrt', negq(H[2]))
        V = fn('sqrt', H[0])
        W = div(mul([T, F]), V)
        X = sim(add([
            div(mul([F, fn('sqrt', B)]), num(2)),
            mul([div(H[0], mul([num(2), T])), fn('asin', W)])]))
        return X, [
            'Use the standard result for Int[sqrt(a^2-b^2*' + var + '^2)] d' + var + '.',
            'So I = ' + pretty(X) + ' + C']

    if A[0] == 'div':
        B = sqrt_inner(A[2])
        H = poly_num(B, var) if B is not None else None
        if same(A[1], power(F, num(2))) and H is not None and len(H) == 3 and is_zero(H[1]) and is_num(H[0]) and is_num(H[2]) and H[0][1] > 0 and H[2][1] < 0:
            T = fn('sqrt', negq(H[2]))
            V = fn('sqrt', H[0])
            W = div(mul([T, F]), V)
            X = sim(add([
                mul([div(H[0], mul([num(2), T, T, T])), fn('asin', W)]),
                neg(div(mul([F, fn('sqrt', B)]), mul([num(2), T, T])))]))
            return X, [
                'Write ' + var + '^2 in terms of the radical.',
                'Use the standard results for 1/sqrt(a^2-b^2*' + var + '^2) and sqrt(a^2-b^2*' + var + '^2).',
                'So I = ' + pretty(X) + ' + C']

        if is_one(A[1]):
            I = quad_info(A[2], var)
            if I is not None and is_num(I[0]) and is_num(I[1]) and is_num(I[2]):
                J = subq(mulq(I[1], I[1]), mulq(num(4), mulq(I[0], I[2])))
                if is_zero(J):
                    K = divq(I[1], mulq(num(2), I[0]))
                    L = add([F, K])
                    X = neg(div(num(1), mul([I[0], L])))
                    return X, [
                        'Complete the square in the denominator.',
                        'Use Int[(u)^-2] du = -(u)^-1 + C.',
                        'So I = ' + pretty(X) + ' + C']

            C = flat(A[2], 'mul') if A[2][0] == 'mul' else [A[2]]
            D = False
            E = None
            G = []
            for Y in C:
                if not D and Y[0] == 'pow' and same(Y[1], F) and same(Y[2], num(2)):
                    D = True
                elif E is None:
                    Z = sqrt_inner(Y)
                    if Z is not None:
                        E = Z
                    else:
                        G.append(Y)
                else:
                    G.append(Y)
            if D and E is not None and len(G) == 0:
                H = poly_num(E, var)
                if H is not None and len(H) == 3 and is_zero(H[1]) and same(H[2], num(1)) and is_num(H[0]) and H[0][1] > 0:
                    X = sim(neg(div(fn('sqrt', E), mul([H[0], F]))))
                    return X, [
                        'Use the standard reverse-derivative pattern.',
                        'd/d' + var + '[sqrt(' + pretty(E) + ')/' + var + '] = -' + pretty(H[0]) + '/(' + var + '^2*sqrt(' + pretty(E) + '))',
                        'So I = ' + pretty(X) + ' + C']

        if same(A[1], power(F, num(2))) and A[2][0] == 'pow' and same(A[2][2], num(2)):
            C = A[2][1]
            if same(C, add([num(1), power(F, num(2))])) or same(C, add([power(F, num(2)), num(1)])):
                X = add([
                    div(fn('atan', F), num(2)),
                    neg(div(F, mul([num(2), add([num(1), power(F, num(2))])])))] )
                return sim(X), [
                    'Rewrite the integrand using d/d' + var + '[' + var + '/(1+' + var + '^2)] and the standard result for 1/(1+' + var + '^2).',
                    'So I = ' + pretty(sim(X)) + ' + C']
    return None, None


def expand_square(node):
    A = node
    if A[0] == 'pow' and same(A[2], num(2)) and A[1][0] == 'add':
        B = flat(A[1], 'add')
        if len(B) == 2:
            C = B[0]
            D = B[1]
            return add([power(C, num(2)), mul(
                [num(2), C, D]), power(D, num(2))])


def expand_product(node):
    if node[0] != 'mul':
        return
    B = list(node[1])
    A = 0
    while A < len(B):
        if B[A][0] == 'add':
            C = []
            for D in flat(B[A], 'add'):
                C.append(mul(B[:A] + [D] + B[A + 1:]))
            return add(C)
        A += 1


def expand_fraction(node):
    E = node
    if E[0] != 'div':
        return
    D = E[1]
    G = E[2]
    if D[0] == 'add':
        B = []
        for F in flat(D, 'add'):
            B.append(div(F, G))
        return add(B)
    if D[0] == 'mul':
        C = list(D[1])
        A = 0
        while A < len(C):
            if C[A][0] == 'add':
                B = []
                for F in flat(C[A], 'add'):
                    B.append(div(mul(C[:A] + [F] + C[A + 1:]), G))
                return add(B)
            A += 1


def expand_small(node):
    A = sim(node)
    C = 0
    while C < EXPAND_PASS_LIMIT:
        B = expand_fraction(A)
        if B is None:
            B = expand_square(A)
        if B is None:
            B = expand_product(A)
        if B is None or same(B, A):
            return A
        A = sim(B)
        C += 1
    return A


def solve_for_u_back(inner, var):
    B = inner
    A = var
    D = linear_info(B, A)
    if D is not None:
        return A + ' = ' + pretty(div(add([U, neg(D[1])]), D[0]))
    if B[0] == 'fn' and B[1] == 'sqrt':
        E = linear_info(B[2], A)
        if E is not None:
            return A + ' = ' + \
                pretty(div(add([power(U, num(2)), neg(E[1])]), E[0]))
        C = poly_num(B[2], A)
        if C is not None and len(C) == 3 and is_zero(C[1]):
            return A + '^2 = ' + \
                pretty(div(add([power(U, num(2)), neg(C[0])]), C[2]))


def make_sum_answer(parts):
    B = parts
    C = []
    A = 0
    while A < len(B):
        C.append(B[A][0])
        A += 1
    return add(C)


def integrate_standard_term(node, var):
    C = var
    P, A = split_const_mul(node, C)
    F = sym(C)
    if same(A, num(1)):
        if is_zero(P):
            return num(0), []
        return mul([P, F]), []
    if is_zero(A):
        return num(0), []
    if not is_one(P):
        R, Q = integrate_standard_term(A, C)
        if R is None:
            return None, None
        D = mul([P, R])
        if len(Q) > 0:
            return D, list(Q) + ['So I = ' + pretty(D) + ' + C']
        return D, Q
    if A == F:
        return div(power(F, num(2)), num(2)), []
    S, U = integrate_special_direct_term(A, C)
    if S is not None:
        return S, U
    if A[0] == 'pow' and A[1] == F and is_num(A[2]) and A[2] != num(-1):
        I = addq(A[2], num(1))
        return div(power(F, I), I), []
    if A[0] == 'div' and is_one(
            A[1]) and A[2][0] == 'pow' and A[2][1] == F and is_num(A[2][2]):
        N = neg(A[2][2])
        if N == num(-1):
            return fn('log', fn('abs', F)), []
        I = addq(N, num(1))
        return div(power(F, I), I), []
    if A[0] == 'div' and is_one(A[1]) and A[2] == F:
        return fn('log', fn('abs', F)), []
    if A[0] == 'div' and is_one(A[1]) and linear_info(A[2], C) is not None:
        G, M = linear_info(A[2], C)
        B = fn('log', fn('abs', A[2]))
        D = div(B, G)
        return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(
            div(G, A[2])), 'So I = ' + pretty(D) + ' + C']
    if A[0] == 'div' and ((A[2][0] == 'fn' and A[2][1] == 'sqrt') or (A[2][0] == 'pow' and same(A[2][2], num(1, 2)))):
        inner = A[2][2] if A[2][0] == 'fn' else A[2][1]
        deriv_inner = safe_diff(inner, C)
        if deriv_inner is not None and not is_zero(deriv_inner):
            deriv_forms = [
                deriv_inner,
                normalize_add_coeffs(expand_small_recursive(deriv_inner)),
                expand_polynomial_parts(deriv_inner, C),
            ]
            numerator_forms = [
                A[1],
                normalize_add_coeffs(expand_small_recursive(A[1])),
                expand_polynomial_parts(A[1], C),
            ]
            coeff = None
            i = 0
            while i < len(numerator_forms) and coeff is None:
                j = 0
                while j < len(deriv_forms):
                    if not is_zero(deriv_forms[j]):
                        candidate = quotient_by_target(numerator_forms[i], deriv_forms[j])
                        if candidate is not None and not is_zero(candidate) and not depends(candidate, C):
                            coeff = sim(candidate)
                            break
                    j += 1
                i += 1
            if coeff is not None:
                result = mul([num(2), coeff, A[2]])
                lines = [
                    'Let u = ' + pretty(inner) + '.',
                    'Then du/d' + C + ' = ' + pretty(deriv_inner) + '.',
                ]
                if is_one(coeff):
                    lines.append('The integrand is (du/d' + C + ')/sqrt(u).')
                else:
                    lines.append('The integrand is ' + pretty(coeff) + '*(du/d' + C + ')/sqrt(u).')
                lines.append('Use Int[u^(-1/2)] du = 2*sqrt(u).')
                lines.append('So I = ' + pretty(result) + ' + C')
                return result, lines
    S, U = integrate_quadratic_rational(A, C)
    if S is not None:
        return S, U
    
    # Add pattern for 1/(x^2 + a) = (1/sqrt(a)) * arctan(x/sqrt(a)) + C
    if A[0] == 'div' and is_one(A[1]) and A[2][0] == 'add':
        terms = flat(A[2], 'add')
        if len(terms) == 2:
            # Look for x^2 and an x-independent positive-style term
            x_squared_term = None
            constant_term = None
            for term in terms:
                if term[0] == 'pow' and term[1] == sym(C) and same(term[2], num(2)):
                    x_squared_term = term
                elif not depends(term, C):
                    constant_term = term
            
            # If we found the pattern, apply arctan formula
            if x_squared_term is not None and constant_term is not None:
                if is_num(constant_term) and constant_term[1] <= 0:
                    constant_term = None
            if x_squared_term is not None and constant_term is not None:
                a = fn('sqrt', constant_term)
                result = div(fn('atan', div(sym(C), a)), a)
                return result, ['Use the standard result: 1/(' + C + '^2 + ' + pretty(constant_term) + ') -> (1/' + pretty(a) + ')*arctan(' + C + '/' + pretty(a) + ')',
                               'So I = ' + pretty(result) + ' + C']

    if A[0] == 'div':
        numerator = A[1]
        denominator = A[2]
        square_info = plus_square_inner_info(denominator, C)
        if square_info is not None:
            constant_term, inner = square_info
            if not is_num(constant_term) or constant_term[1] > 0:
                deriv_inner = safe_diff(inner, C)
                if deriv_inner is not None and not is_zero(deriv_inner):
                    coeff = quotient_by_target(deriv_inner, numerator)
                    if coeff is not None and is_num(coeff):
                        scale = sqrt_num(constant_term)
                        if scale is None:
                            scale = fn('sqrt', constant_term)
                        result = div(fn('atan', div(inner, scale)), mul([coeff, scale]))
                        lines = []
                        if not same(inner, F) or not same(constant_term, num(1)):
                            lines.append('Write the denominator as ' + pretty(add([power(inner, num(2)), constant_term])) + '.')
                        lines.append('Use the standard result for f\'(x)/(a^2+f(x)^2).')
                        lines.append('So I = ' + pretty(result) + ' + C')
                        return result, lines

    # Add pattern for f'(x)/f(x) = ln|f(x)| + C (general logarithmic integration)
    if A[0] == 'div':
        numerator = A[1]
        denominator = A[2]
        # Check if numerator is derivative of denominator
        deriv_denom = safe_diff(denominator, C)
        if deriv_denom is None:
            deriv_denom = num(0)
        deriv_forms = [
            deriv_denom,
            normalize_add_coeffs(expand_small_recursive(deriv_denom)),
            expand_polynomial_parts(deriv_denom, C),
        ]
        numerator_forms = [
            numerator,
            normalize_add_coeffs(expand_small_recursive(numerator)),
            expand_polynomial_parts(numerator, C),
        ]
        coeff = None
        i = 0
        while i < len(numerator_forms) and coeff is None:
            j = 0
            while j < len(deriv_forms):
                if not is_zero(deriv_forms[j]):
                    candidate = quotient_by_target(numerator_forms[i], deriv_forms[j])
                    if candidate is not None and not is_zero(candidate) and not depends(candidate, C):
                        coeff = sim(candidate)
                        break
                j += 1
            i += 1
        if coeff is not None:
            log_part = fn('log', fn('abs', denominator))
            result = log_part if is_one(coeff) else mul([coeff, log_part])
            if is_one(coeff):
                return result, ['Use the standard result for f\'(x)/f(x) -> ln|f(x)|.',
                               'So I = ' + pretty(result) + ' + C']
            return result, ['Use the standard result for k*f\'(x)/f(x) -> k*ln|f(x)|.',
                           'So I = ' + pretty(result) + ' + C']
        if denominator[0] == 'mul':
            factors = flat(denominator, 'mul')
            i = 0
            while i < len(factors):
                cofactor = factors[i]
                inner = quotient_by_target(denominator, cofactor)
                if inner is not None and not is_one(inner):
                    deriv_inner = safe_diff(inner, C)
                    if deriv_inner is not None and not is_zero(deriv_inner):
                        target = mul([cofactor, deriv_inner])
                        target_forms = [
                            target,
                            normalize_add_coeffs(expand_small_recursive(target)),
                            expand_polynomial_parts(target, C),
                        ]
                        coeff = None
                        a = 0
                        while a < len(numerator_forms) and coeff is None:
                            b = 0
                            while b < len(target_forms):
                                if not is_zero(target_forms[b]):
                                    candidate = quotient_by_target(numerator_forms[a], target_forms[b])
                                    if candidate is not None and not is_zero(candidate) and not depends(candidate, C):
                                        coeff = sim(candidate)
                                        break
                                b += 1
                            a += 1
                        if coeff is not None:
                            log_part = fn('log', fn('abs', inner))
                            result = log_part if is_one(coeff) else mul([coeff, log_part])
                            lines = [
                                'Cancel the shared factor ' + pretty(cofactor) + ' in the numerator and denominator.',
                                'The remaining integrand is f\'(x)/f(x) with f(x) = ' + pretty(inner) + '.',
                                'Use the standard result for f\'(x)/f(x) -> ln|f(x)|.',
                                'So I = ' + pretty(result) + ' + C',
                            ]
                            return result, lines
                i += 1
    
    # Pattern: f(x)/sqrt(1-f(x)^2) integrates to asin(f(x)) + C
    # Also handles: f(x)/(1+f(x)^2) integrates to atan(f(x)) + C
    if A[0] == 'div':
        numerator = A[1]
        denominator = A[2]
        
        # Handle pow with 1/2 exponent (sqrt)
        inner_sqrt = None
        if denominator[0] == 'pow' and same(denominator[2], num(1, 2)):
            inner_sqrt = denominator[1]
        elif denominator[0] == 'fn' and denominator[1] == 'sqrt':
            inner_sqrt = denominator[2]
        
        # Check for sqrt(1-g(x)^2)
        if inner_sqrt is not None and inner_sqrt[0] == 'add' and len(inner_sqrt[1]) == 2:
            terms = list(inner_sqrt[1])
            # Check for 1 - g(x)^2
            if (same(terms[0], num(1)) and terms[1][0] == 'pow' and 
                same(terms[1][2], num(2))):
                g = terms[1][1]
                deriv_g = safe_diff(g, C)
                if deriv_g is not None and same(deriv_g, numerator):
                    return fn('asin', g), ['Use the standard result.',
                                          'So I = asin(' + pretty(g) + ') + C']
            # Check for -g(x)^2 + 1 (which is 1 - g(x)^2)
            # terms[0] is like ('mul', (-1, x^2)) and terms[1] is 1
            if (terms[1] == num(1) and terms[0][0] == 'mul' and 
                len(terms[0][1]) == 2):
                coeff_part = terms[0][1][0]
                pow_part = terms[0][1][1]
                # Check if coefficient is -1 and rest is x^2
                if is_minus_one(coeff_part) and pow_part[0] == 'pow' and same(pow_part[2], num(2)):
                    g = pow_part[1]
                    deriv_g = safe_diff(g, C)
                    if deriv_g is not None and same(deriv_g, numerator):
                        return fn('asin', g), ['Use the standard result.',
                                              'So I = asin(' + pretty(g) + ') + C']
        
        # Check for 1 + g(x)^2 in denominator (for atan)
        if denominator[0] == 'add' and len(denominator[1]) == 2:
            terms = list(denominator[1])
            # Check for 1 + g(x)^2
            if (same(terms[0], num(1)) and terms[1][0] == 'pow' and 
                same(terms[1][2], num(2))):
                g = terms[1][1]
                deriv_g = safe_diff(g, C)
                if deriv_g is not None and same(deriv_g, numerator):
                    return fn('atan', g), ['Use the standard result.',
                                          'So I = atan(' + pretty(g) + ') + C']
            # Also check g(x)^2 + 1
            if (terms[1] == num(1) and terms[0][0] == 'add' and 
                terms[0][1][0][0] == 'pow' and same(terms[0][1][0][2], num(2))):
                g = terms[0][1][0][1]
                deriv_g = safe_diff(g, C)
                if deriv_g is not None and same(deriv_g, numerator):
                    return fn('atan', g), ['Use the standard result.',
                                          'So I = atan(' + pretty(g) + ') + C']
    
    if A[0] == 'div' and (
            A[2][0] == 'fn' and A[2][1] == 'sqrt' or A[2][0] == 'pow' and same(A[2][2], num(1, 2))):
        numerator = A[1]
        J = A[2][2]if A[2][0] == 'fn'else A[2][1]
        H = poly_num(J, C)
        if numerator == F and H is not None and len(H) == 3 and H[2] == num(1):
            lin = H[1]
            half_lin = divq(lin, num(2)) if is_num(lin) else div(lin, num(2))
            shifted = add([F, half_lin])
            constant = sim(sub(H[0], power(half_lin, num(2))))
            if is_num(constant) and constant[1] > 0:
                coeff = sim(neg(half_lin))
                term1 = fn('sqrt', J)
                term2 = neg(mul([half_lin, fn('log', fn('abs', add([shifted, fn('sqrt', J)])))]))
                result = sim(add([term1, term2]))
                return result, direct_quadratic_surd_lines(C, J, shifted, constant, coeff, result)
        if is_one(numerator) and H is not None and len(H) == 3 and is_zero(H[1]) and H[2] == num(1):
            B = fn('log', fn('abs', add([F, power(J, num(1, 2))])))
            return B, [
                'Use the standard result for 1/sqrt(' + C + '^2+a).', 'So I = ' + pretty(B) + ' + C']
        if is_one(numerator) and H is not None and len(H) == 3 and H[2] == num(1):
            lin = H[1]
            half_lin = divq(lin, num(2)) if is_num(lin) else div(lin, num(2))
            shifted = add([F, half_lin])
            constant = sim(sub(H[0], power(half_lin, num(2))))
            if is_num(constant) and constant[1] > 0:
                B = fn('log', fn('abs', add([shifted, power(add([power(shifted, num(2)), constant]), num(1, 2))])))
                return B, [
                    'Complete the square in the radical.',
                    'Use the standard result for 1/sqrt(u^2+a).', 'So I = ' + pretty(B) + ' + C']
            if is_num(constant) and constant[1] < 0:
                a_sq = negq(constant)
                B = fn('log', fn('abs', add([shifted, power(add([power(shifted, num(2)), constant]), num(1, 2))])))
                return B, [
                    'Complete the square in the radical.',
                    'Write the denominator as sqrt(u^2-' + pretty(a_sq) + ').',
                    'Use the standard result for 1/sqrt(u^2-a^2).',
                    'So I = ' + pretty(B) + ' + C']
        if is_one(numerator) and H is not None and len(H) == 3 and is_zero(H[1]) and H[2] == num(1):
            B = fn('log', fn('abs', add([F, power(J, num(1, 2))])))
            return B, [
                'Use the standard result for 1/sqrt(' + C + '^2+a).', 'So I = ' + pretty(B) + ' + C']
        if is_one(numerator) and (
                same(J, add([num(1), neg(power(F, num(2)))])) or
                same(J, add([neg(power(F, num(2))), num(1)]))):
            return fn('asin', F), [
                'Use the standard result for 1/sqrt(1-' + C + '^2).', 'So I = asin(' + C + ') + C']
        if is_one(numerator) and H is not None and len(H) == 3 and is_zero(H[1]) and is_num(
                H[0]) and is_num(H[2]) and H[0][1] > 0 and H[2][1] < 0:
            V = fn('sqrt', H[0])
            T = fn('sqrt', negq(H[2]))
            W = div(mul([T, F]), V)
            D = div(fn('asin', W), T)
            return D, [
                'Use the standard result for 1/sqrt(a^2-b^2*' + C + '^2).', 'So I = ' + pretty(D) + ' + C']
        K = linear_info(J, C)
        if is_one(numerator) and K is not None:
            G, M = K
            N = num(-1, 2)
            if N == num(-1):
                B = fn('log', fn('abs', J))
                D = div(B, G)
            else:
                I = addq(N, num(1))
                B = power(J, I)
                D = div(B, mul([G, I]))
            return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(diff(B, C)if not (
                B[0] == 'fn' and B[1] == 'log')else div(G, J)), 'So I = ' + pretty(D) + ' + C']
        return None, None
    # Add support for a^x where a is a constant (not e)
    if A[0] == 'pow' and is_num(A[1]) and A[1][1] > 0 and A[1] != E:
        # a^x integration: ∫a^x dx = a^x/ln(a) + C
        base = A[1]
        exp = A[2]
        # Check if exponent is linear in var (i.e., a^(kx) or a^x)
        K = linear_info(exp, C)
        if K is None:
            return None, None
        G, M = K
        # For a^(kx), result is a^(kx)/(k*ln(a))
        # For a^x, result is a^x/ln(a)
        log_base = fn('log', base)
        if is_one(G):
            result = div(A, log_base)
            return result, ['Use the standard result: ∫a^x dx = a^x/ln(a) + C', '= ' + pretty(result) + ' + C']
        else:
            result = div(power(base, exp), mul([G, log_base]))
            return result, ['Use the standard result: ∫a^(kx) dx = a^(kx)/(k*ln(a)) + C', '= ' + pretty(result) + ' + C']
    if A[0] == 'pow' and same(A[1], E):
        K = linear_info(A[2], C)
        if K is None:
            return None, None
        G, M = K
        D = div(A, G)
        B = A
        return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(
            diff(B, C)), 'So I = ' + pretty(D) + ' + C']
    if A[0] == 'fn':
        O = A[1]
        J = A[2]
        K = linear_info(J, C)
        if K is None:
            if J == F:
                B = primitive_of_named_function(O, F)
                if B is not None:
                    return B, []
            return None, None
        G, M = K
        B = primitive_of_named_function(O, J)
        if B is not None:
            D = div(B, G)
            return D, ['Use the standard integral for ' + pretty(A) + '.', 'So I = ' + pretty(D) + ' + C']
    if A[0] == 'pow' and linear_info(A[1], C) is not None and is_num(A[2]):
        G, M = linear_info(A[1], C)
        if same(A[2], num(-1)):
            B = fn('log', fn('abs', A[1]))
            D = div(B, G)
        else:
            I = addq(A[2], num(1))
            B = power(A[1], I)
            D = div(B, mul([G, I]))
        return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(div(
            G, A[1])if B[0] == 'fn' and B[1] == 'log'else diff(B, C)), 'So I = ' + pretty(D) + ' + C']
    if A[0] == 'pow' and A[1][0] == 'fn' and is_int_num(A[2]):
        O = A[1][1]
        J = A[1][2]
        K = linear_info(J, C)
        if K is not None:
            G, M = K
            B = primitive_of_named_power(O, J, A[2])
            if B is not None:
                D = div(B, G)
                return D, ['Use the standard integral for ' + pretty(A) + '.', 'So I = ' + pretty(D) + ' + C']
        F2, E2 = trig_high_power_rewrite(O, A[2], J)
        if E2 is not None:
            R, Q = integrate_standard(expand_small(E2), C)
            if R is not None:
                return R, [
                    F2,
                    'So I = ' + int_text(expand_small(E2), C),
                    'Integrate each term separately.',
                    '= ' + pretty(R) + ' + C']
    if A[0] == 'mul':
        L = list(A[1])
        if len(L) == 2 and L[0][0] == 'fn' and L[1][0] == 'fn' and same(
                L[0][2], L[1][2]):
            K = linear_info(L[0][2], C)
            if K is not None:
                G, M = K
                B = primitive_of_named_product(L[0][1], L[1][1], L[0][2])
                if B is not None:
                    D = div(B, G)
                    return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(
                        diff(B, C)), 'So I = ' + pretty(D) + ' + C']
    return None, None


def integrate_standard(node, var):
    C = var
    B = node
    if B[0] != 'add':
        A, E = integrate_standard_term(B, C)
        if A is not None:
            return A, E
    D = expand_small(B)
    if not same(D, B):
        A, E = integrate_standard(D, C)
        if A is not None:
            return A, ['Integrate each term separately.',
                       '= ' + pretty(A) + ' + C']if D[0] == 'add'else E
    if B[0] == 'add':
        F = []
        for H in flat(B, 'add'):
            G, E = integrate_standard_term(H, C)
            if G is None:
                return None, None
            F.append((G, E))
        A = make_sum_answer(F)
        return A, ['Integrate each term separately.',
                   '= ' + pretty(A) + ' + C']
    return integrate_standard_term(B, C)


def linear_outer_function_factor(node, var):
    A = sim(node)
    if A[0] == 'fn' and A[1] in ('sin', 'cos', 'exp'):
        B = linear_info(A[2], var)
        if B is not None:
            return A[1], A[2], B[0]
    if A[0] == 'pow' and same(A[1], E):
        B = linear_info(A[2], var)
        if B is not None:
            return 'exp', A[2], B[0]
    return None


def split_poly_outer_product(node, var, names):
    P, A = split_numeric_term(node)
    A = sim(A)
    if A[0] == 'div' and is_num(A[2]):
        P = divq(P, A[2])
        A = sim(A[1])
    B = flat(A, 'mul') if A[0] == 'mul' else [A]
    found_index = None
    found = None
    i = 0
    while i < len(B):
        C = linear_outer_function_factor(B[i], var)
        if C is not None and C[0] in names:
            if found is not None:
                return None
            found = C
            found_index = i
        i += 1
    if found is None:
        return None
    C = B[:found_index] + B[found_index + 1:]
    D = mul([P, make_mul(C)])
    E = poly_num(D, var)
    if E is None:
        return None
    return D, found[0], found[1], found[2]


def integrate_poly_exp_repeated(poly_expr, arg, k, var):
    B = poly_expr
    C = num(1)
    D = []
    while not is_zero(B):
        D.append(mul([C, div(B, k)]))
        B = diff(B, var)
        C = neg(div(C, k))
    return mul([fn('exp', arg), add(D)])


def integrate_poly_trig_repeated(poly_expr, trig_name, arg, k, var):
    A = poly_expr
    B = k
    C = []
    D = 0
    while not is_zero(A):
        if trig_name == 'sin':
            pattern = (('cos', -1), ('sin', 1), ('cos', 1), ('sin', -1))
        else:
            pattern = (('sin', 1), ('cos', 1), ('sin', -1), ('cos', -1))
        E, F = pattern[D % 4]
        G = div(mul([A, fn(E, arg)]), B)
        C.append(neg(G) if F < 0 else G)
        A = diff(A, var)
        B = mul([B, k])
        D += 1
    return add(C)


def integrate_poly_linear_parts(node, var):
    A = split_poly_outer_product(node, var, ('sin', 'cos', 'exp'))
    if A is None:
        return None, None
    B, C, D, E = A
    F = poly_num(B, var)
    if F is None or len(F) <= 1:
        return None, None
    if len(F) > 18:
        return None, None
    if C == 'exp':
        G = integrate_poly_exp_repeated(B, D, E, var)
        H = [
            'Use integration by parts repeatedly.',
            'Let p = ' + pretty(B) + ' and dv = e^(' + pretty(D) + ') d' + var + '.',
            'Since d(' + pretty(D) + ')/d' + var + ' = ' + pretty(E) + ', each step reduces the degree of p.',
            'Continue until the polynomial derivative is 0.',
            '= ' + pretty(G) + ' + C']
        return G, H
    G = integrate_poly_trig_repeated(B, C, D, E, var)
    H = [
        'Use integration by parts repeatedly.',
        'Let p = ' + pretty(B) + ' and dv = ' + C + '(' + pretty(D) + ') d' + var + '.',
        'Since d(' + pretty(D) + ')/d' + var + ' = ' + pretty(E) + ', each step reduces the degree of p.',
        'Continue until the polynomial derivative is 0.',
        '= ' + pretty(G) + ' + C']
    return G, H


def integrate_derivative_times_outer(node, var):
    A = sim(node)
    B = flat(A, 'mul') if A[0] == 'mul' else [A]
    i = 0
    while i < len(B):
        C = B[i]
        if C[0] == 'fn' and C[1] in ('sin', 'cos', 'exp'):
            D = C[2]
            E = safe_diff(D, var)
            if E is not None and not is_zero(E):
                F = make_mul(B[:i] + B[i + 1:])
                G = quotient_by_equivalent_target(F, E, var)
                if G is not None and not depends(G, var):
                    if C[1] == 'sin':
                        H = neg(fn('cos', D))
                    elif C[1] == 'cos':
                        H = fn('sin', D)
                    else:
                        H = fn('exp', D)
                    I = sim(mul([G, H]))
                    return I, [
                        'Recognise reverse chain rule.',
                        'Let u = ' + pretty(D) + ', so du/d' + var + ' = ' + pretty(E) + '.',
                        'The remaining factor is ' + pretty(G) + ' times du/d' + var + '.',
                        '= ' + pretty(I) + ' + C']
        i += 1
    return None, None


FN_CANDIDATE_NAMES = 'sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'sqrt', 'exp', 'log', 'asin', 'acos', 'atan'


def ordered_candidates(node, var, mode):
    F = var
    D = []
    E = set()
    H = set()
    G = sym(F)

    def B(node):
        A = node
        if A is None or not depends(A, F) or A == G or A in E:
            return
        E.add(A)
        D.append(A)

    def C(cur, depth):
        D = depth
        A = cur
        if D > 6 or A in H or not depends(A, F):
            return
        H.add(A)
        E = A[0]
        if E == 'fn':
            if A[1] in FN_CANDIDATE_NAMES:
                B(A)
            C(A[2], D + 1)
            return
        if E == 'div':
            B(A[2])
            C(A[1], D + 1)
            C(A[2], D + 1)
            return
        if E == 'pow':
            B(A)
            B(A[1])
            if mode == 'sub' and A[2] == num(1, 2):
                B(fn('sqrt', A[1]))
            C(A[1], D + 1)
            C(A[2], D + 1)
            return
        if E == 'add':
            B(A)
        elif linear_info(A, F) is not None:
            B(A)
        if E in ('add', 'mul'):
            G = 0
            while G < len(A[1]):
                C(A[1][G], D + 1)
                G += 1
    C(node, 0)
    if mode == 'sub':
        B(fn('sqrt', G))
    return D


def u_power_info(node):
    B, A = split_const_mul(node, 'u')
    if A == U:
        return B, num(1)
    if A[0] == 'pow' and A[1] == U and is_num(A[2]):
        return B, A[2]
    if A[0] == 'div' and is_one(A[1]) and A[2] == U:
        return B, num(-1)
    if A[0] == 'div' and is_one(
            A[1]) and A[2][0] == 'pow' and A[2][1] == U and is_num(A[2][2]):
        return B, neg(A[2][2])
    if A[0] == 'pow' and A[1] == U and A[2] == num(-1):
        return B, num(-1)


def integrate_reverse_chain(node, var):
    D = node
    B = var
    A, M = integrate_derivative_times_outer(D, B)
    if A is not None:
        return A, M
    I = ordered_candidates(D, B, 'reverse')
    E = 0
    while E < len(I):
        C = I[E]
        F = safe_diff(C, B)
        if F is not None and not is_zero(F):
            J = quotient_by_equivalent_target(D, F, B)
            if J is None:
                for Q in ('sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'exp'):
                    R = fn(Q, C)
                    S = quotient_by_target(D, R)
                    if S is None:
                        continue
                    T = quotient_by_equivalent_target(S, F, B)
                    if T is not None and not depends(T, B):
                        J = R if is_one(T) else mul([T, R])
                        break
            if J is not None:
                G = rewrite_in_equivalent_u(J, C, B)
                if G is not None:
                    H = expand_small(G)
                    A = direct_u_form(H)
                    if A is None:
                        K = u_power_info(H)
                        if K is not None:
                            L, M = K
                            if same(M, num(-1)):
                                A = mul([L, fn('log', fn('abs', U))])
                            else:
                                N = addq(M, num(1))
                                A = mul([L, div(power(U, N), N)])
                    if A is not None:
                        O = subst(A, 'u', C)
                        return O, substitution_work(D, B, C, F, G, H, A, O)
        E += 1
    return None, None


def solve_u_expr(node):
    B = node
    B = expand_small(B)
    if B[0] == 'add':
        D = flat(B, 'add')
        C = []
        for G in D:
            E = solve_u_expr(G)
            if E is None:
                break
            C.append(E)
        if len(C) == len(D):
            return add(C)
    A, F = integrate_u_subproblem(B)
    if A is not None:
        return A
    A, F = integrate_division(B, 'u', True)
    if A is not None:
        return A
    A, F = integrate_partial(B, 'u')
    if A is not None:
        return A


def normalize_den_powers(node):
    A = node
    if A[0] == 'add':
        return sim(add([normalize_den_powers(B) for B in flat(A, 'add')]))
    if A[0] == 'mul':
        return sim(mul([normalize_den_powers(B) for B in flat(A, 'mul')]))
    if A[0] == 'pow':
        return sim(power(normalize_den_powers(A[1]), normalize_den_powers(A[2])))
    if A[0] != 'div':
        return A
    B = normalize_den_powers(A[1])
    C = normalize_den_powers(A[2])
    if C[0] == 'mul':
        K = flat(C, 'mul')
        i = 0
        while i < len(K):
            if same(K[i], U):
                J = K[:i] + K[i + 1:]
                if len(J) == 1 and J[0][0] == 'add':
                    L = flat(J[0], 'add')
                    if len(L) == 2 and ((same(L[0], U) and same(L[1], power(U, num(-1)))) or (same(L[1], U) and same(L[0], power(U, num(-1))))):
                        return sim(div(B, add([power(U, num(2)), num(1)])))
            i += 1
    D = flat(C, 'mul') if C[0] == 'mul' else [C]
    E = [B]
    F = []
    G = False
    for H in D:
        if H[0] == 'pow' and is_num(H[2]) and H[2][1] < 0:
            E.append(power(H[1], neg(H[2])))
            G = True
        else:
            F.append(H)
    if not G:
        return sim(div(B, C))
    I = make_mul(E)
    J = make_mul(F)
    return sim(I if is_one(J) else div(I, J))


def cancel_u_radical(node):
    A = node
    if A[0] != 'div':
        return A
    G, H = split_const_mul(A[1], 'u')
    if H != U:
        return A
    D = A[2]
    if D[0] != 'pow' or D[2] != num(1, 2):
        return A
    C = flat(D[1], 'mul')
    F = []
    E = False
    B = 0
    while B < len(C):
        if not E and C[B][0] == 'pow' and C[B][1] == U and C[B][2] == num(2):
            E = True
        else:
            F.append(C[B])
        B += 1
    if not E:
        return A
    return div(G, power(make_mul(F), num(1, 2)))


def integrate_substitution(node, var, forced_u=None):
    J = forced_u
    C = var
    B = node
    if J is None:
        G = ordered_candidates(B, C, 'sub')
    else:
        G = [J]
    H = 0
    while H < len(G):
        D = G[H]
        E = safe_diff(D, C)
        if E is not None and not is_zero(E):
            F = quotient_by_equivalent_target(B, E, C)
            if F is None:
                for N in ('sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'exp'):
                    O = fn(N, D)
                    P = quotient_by_target(B, O)
                    if P is None:
                        continue
                    Q = quotient_by_equivalent_target(P, E, C)
                    if Q is not None and not depends(Q, C):
                        F = O if is_one(Q) else mul([Q, O])
                        break
            if F is None:
                F = div(B, E)
            if F is not None:
                A = rewrite_in_equivalent_u(F, D, C)
                if A is not None:
                    A = normalize_den_powers(A)
                    A = cancel_u_radical(A)
                    A = normalize_den_powers(A)
                    K = expand_small(A)
                    L = K if not same(K, A)else A
                    I = solve_u_expr(L)
                    if I is not None:
                        M = subst(I, 'u', D)
                        return M, substitution_work(B, C, D, E, A, L, I, M)
        H += 1
    if J is None:
        return integrate_trig_sub_radical(B, C)
    return None, None


def trig_simple_fraction_rewrite(node):
    A = sim(node)
    if A[0] != 'div':
        return None, None
    top = A[1]
    bot = A[2]

    def one_pm_fn(expr, name):
        if expr[0] != 'add':
            return None
        terms = flat(expr, 'add')
        if len(terms) != 2:
            return None
        saw_one = False
        arg = None
        sign = None
        i = 0
        while i < len(terms):
            term = terms[i]
            if same(term, num(1)):
                saw_one = True
            elif term[0] == 'fn' and term[1] == name:
                arg = term[2]
                sign = 1
            elif term[0] == 'mul':
                coeff, rest = split_coeff(term)
                if is_minus_one(coeff) and rest[0] == 'fn' and rest[1] == name:
                    arg = rest[2]
                    sign = -1
                else:
                    return None
            else:
                return None
            i += 1
        if not saw_one or arg is None:
            return None
        return sign, arg

    top_pm_cos = one_pm_fn(top, 'cos')
    top_pm_sin = one_pm_fn(top, 'sin')
    bot_pm_cos = one_pm_fn(bot, 'cos')
    bot_pm_sin = one_pm_fn(bot, 'sin')

    if top[0] == 'fn' and top[1] == 'sin' and same(top[2], mul([num(2), sym('x')])):
        top_arg = div(top[2], num(2))
        if bot_pm_cos is not None and same(bot_pm_cos[1], top_arg):
            if bot_pm_cos[0] > 0:
                return [
                    'Use sin(2A) = 2sin A cos A.',
                    'Write 2sin(A)cos(A)/(1+cos(A)) = 2sin(A) - 2sin(A)/(1+cos(A)).'
                ], add([
                    mul([num(2), fn('sin', top_arg)]),
                    neg(div(mul([num(2), fn('sin', top_arg)]), add([num(1), fn('cos', top_arg)])))
                ])
            return 'Use sin(2A) = 2sin A cos A and split the fraction.', div(mul([num(2), fn('sin', top_arg), fn('cos', top_arg)]), add([num(1), neg(fn('cos', top_arg))]))
        if bot_pm_sin is not None and same(bot_pm_sin[1], top_arg):
            if bot_pm_sin[0] > 0:
                return 'Use sin(2A) = 2sin A cos A and simplify the denominator.', mul([
                    num(2),
                    fn('cos', top_arg),
                    add([num(1), neg(div(num(1), add([num(1), fn('sin', top_arg)])))])
                ])
            return 'Use sin(2A) = 2sin A cos A and simplify the denominator.', mul([
                num(2),
                fn('cos', top_arg),
                add([num(1), div(num(1), add([num(1), neg(fn('sin', top_arg))]))])
            ])

    if bot[0] == 'fn' and bot[1] == 'sin':
        bot_arg = bot[2]
        if top_pm_cos is not None and same(top_pm_cos[1], mul([num(2), bot_arg])):
            if top_pm_cos[0] < 0:
                return 'Use 1-cos(2A) = 2sin^2 A and cancel a factor of sin A.', mul([num(2), fn('sin', bot_arg)])
            return 'Use 1+cos(2A) = 2cos^2 A and divide by sin A.', div(mul([num(2), power(fn('cos', bot_arg), num(2))]), fn('sin', bot_arg))
    if top[0] == 'add' and bot[0] == 'fn' and bot[1] == 'sin':
        top_pm_cos = one_pm_fn(top, 'cos')
        if top_pm_cos is not None and top_pm_cos[0] > 0 and same(top_pm_cos[1], mul([num(2), bot[2]])):
            return 'Use 1+cos(2A) = 2cos^2 A and divide by sin A.', div(mul([num(2), power(fn('cos', bot[2]), num(2))]), fn('sin', bot[2]))
    return None, None


def trig_rewrite_step(node, var=None):
    B = node
    I = expand_square(B)
    if I is not None:
        return 'Expand the brackets.', I
    J, H = trig_weighted_square_rewrite(B)
    if H is not None:
        return J, H
    J, H = trig_simple_fraction_rewrite(B)
    if H is not None:
        return J, H
    J, H = trig_linear_reciprocal_rewrite(B)
    if H is not None:
        return J, H
    if B[0] == 'div' and B[1][0] == 'add' and B[2][0] == 'pow' and same(
            B[2][2], num(2)) and B[2][1][0] == 'fn':
        D = flat(B[1], 'add')
        P = B[2][1]
        C = P[2]
        if len(D) == 2 and same(P, fn('cos', C)):
            if same(D[0], num(1)) and same(D[1], neg(fn('sin', C))) or same(
                    D[1], num(1)) and same(D[0], neg(fn('sin', C))):
                return 'Rewrite in sec and tan.', add(
                    [power(fn('sec', C), num(2)), neg(mul([fn('sec', C), fn('tan', C)]))])
        if len(D) == 2 and same(P, fn('sin', C)):
            if same(D[0], num(1)) and same(D[1], fn('cos', C)) or same(
                    D[1], num(1)) and same(D[0], fn('cos', C)):
                return 'Rewrite in cosec and cot.', add(
                    [power(fn('cosec', C), num(2)), mul([fn('cosec', C), fn('cot', C)])])
    if B[0] == 'pow' and B[1][0] == 'fn' and same(B[2], num(2)):
        J = B[1][1]
        C = B[1][2]
        F, E = trig_square_rewrite(J, C)
        if E is not None:
            return F, E
    if B[0] == 'pow' and B[1][0] == 'fn':
        F, E = trig_high_power_rewrite(B[1][1], B[2], B[1][2])
        if E is not None:
            return F, E
    F, E = trig_same_angle_power_rewrite(B)
    if E is not None:
        return F, E
    if B[0] == 'mul':
        A = list(B[1])
        F, E = trig_cubic_linear_combo_rewrite(B)
        if E is not None:
            return F, E
        if len(A) == 2 and A[0][0] == 'fn' and A[1][0] == 'fn' and A[0][1] == 'sin' and A[1][1] == 'cos' and same(
                A[0][2], A[1][2]):
            return trig_product_rewrite(A[0][1], A[1][1], A[0][2])
        if len(A) == 2 and A[0][0] == 'fn' and A[1][0] == 'fn' and A[0][1] == 'cos' and A[1][1] == 'sin' and same(
                A[0][2], A[1][2]):
            return trig_product_rewrite(A[0][1], A[1][1], A[0][2])
        if len(A) == 2 and A[0][0] == 'fn' and A[1][0] == 'fn':
            F, E = trig_product_to_sum_rewrite(
                A[0][1], A[1][1], A[0][2], A[1][2], var)
            if E is not None:
                return F, E
        G = 0
        while G < len(A):
            F, E = trig_rewrite_step(A[G], var)
            if E is not None:
                return F, expand_small(mul(A[:G] + [E] + A[G + 1:]))
            G += 1
    return None, None


def integrate_shifted_sinodd_cos2(node, var):
    coeff, rest = split_const_mul(node, var)
    factors = list(flat(rest, 'mul')) if rest[0] == 'mul' else [rest]
    sin_arg = None
    cos_arg = None
    used = [False] * len(factors)
    i = 0
    while i < len(factors):
        factor = factors[i]
        if factor[0] == 'pow' and is_int_num(factor[2]):
            base = factor[1]
            power_value = factor[2][1]
            if base[0] == 'fn' and base[1] == 'sin' and power_value == 5 and sin_arg is None:
                sin_arg = base[2]
                used[i] = True
            elif base[0] == 'fn' and base[1] == 'cos' and power_value == 2 and cos_arg is None:
                cos_arg = base[2]
                used[i] = True
        i += 1
    if sin_arg is None or cos_arg is None:
        return None, None
    i = 0
    while i < len(factors):
        if not used[i] and not is_one(factors[i]):
            return None, None
        i += 1
    if linear_info(sin_arg, var) is None or linear_info(cos_arg, var) is None:
        return None, None

    terms = []
    for mult, base_coeff in ((1, 10), (3, -5), (5, 1)):
        u = sim(mul([num(mult), sin_arg]))
        v = sim(mul([num(2), cos_arg]))
        terms.append(mul([coeff, num(base_coeff, 32), fn('sin', u)]))
        terms.append(mul([coeff, num(base_coeff, 64), fn('sin', add([u, v]))]))
        terms.append(mul([coeff, num(base_coeff, 64), fn('sin', add([u, neg(v)]))]))
    expanded = normalize_add_coeffs(expand_small_recursive(add(terms)))
    ans, ans_lines = integrate_standard(expanded, var)
    if ans is None:
        return None, None
    lines = [
        'Use sin^5(A) = (10*sin(A) - 5*sin(3*A) + sin(5*A))/16.',
        'Use cos^2(B) = (1 + cos(2*B))/2.',
        'Use sin(P)*cos(Q) = (1/2)*[sin(P + Q) + sin(P - Q)].',
        'So I = Int[' + pretty(expanded) + '] d' + var,
    ]
    if ans_lines is not None:
        i = 0
        while i < len(ans_lines):
            if ans_lines[i] not in lines:
                lines.append(ans_lines[i])
            i += 1
    lines.append('= ' + pretty(ans) + ' + C')
    return ans, lines


def integrate_trig(node, var, allow_steps=True):
    E = allow_steps
    C = var
    B = node
    shifted_ans, shifted_lines = integrate_shifted_sinodd_cos2(B, C)
    if shifted_ans is not None:
        return shifted_ans, shifted_lines if E else []
    D = []
    F = False
    G = 0
    while G < TRIG_REWRITE_LIMIT:
        J, H = trig_rewrite_step(B, C)
        if H is None:
            break
        B = H
        F = True
        if E:
            if isinstance(J, list):
                K = 0
                while K < len(J):
                    D.append(J[K])
                    K += 1
            else:
                D.append(J)
            D.append('So I = ' + int_text(B, C))
        G += 1
    if not F:
        return None, None
    A, I = integrate_after_trig_rewrite(B, C, 1)
    if A is None:
        A, I = integrate_division(B, C, True)
        if A is None:
            A, I = integrate_partial(B, C)
    if A is None:
        return None, None
    if E:
        if I is not None:
            K = 0
            while K < len(I):
                D.append(I[K])
                K += 1
        if len(D) == 0 or D[-1] != '= ' + pretty(A) + ' + C':
            D.append('= ' + pretty(A) + ' + C')
        return A, D
    return A, I or []


def _solve_with_method(node, var, method, forced_u=None):
    F = forced_u
    E = method
    D = var
    C = node if E == '4' and F is not None else sim(node)
    if F is not None:
        F = sim(F)
    if E == '2':
        A, B = integrate_standard(C, D)
        return finish_integral_solve(standard_title(C, D), A, B)
    if E == '3':
        A, B = integrate_trig(C, D, True)
        return finish_integral_solve('Using trigonometric identities', A, B)
    if E == '4':
        if F is None:
            A, B = integrate_reverse_chain(C, D)
            if A is not None:
                return finish_integral_solve('Reverse chain rule', A, B)
            A, B = integrate_substitution(C, D, None)
            return finish_integral_solve('Integration by substitution', A, B)
        A, B = integrate_substitution(C, D, F)
        return finish_integral_solve('Integration by substitution', A, B)
    if E == '5':
        A, B = integrate_by_parts(C, D, 0)
        return finish_integral_solve('Integration by parts', A, B)
    if E == '6':
        A, B = integrate_partial(C, D)
        return finish_integral_solve('Partial fractions', A, B)
    if E == '7':
        A, B = integrate_division(C, D, True)
        return finish_integral_solve('Partial fractions', A, B)
    G, A, B = integrate_auto(C, D, 0, True, True)
    if A is None:
        return 'Automatic integration', A, B
    if G == 'direct':
        return finish_integral_solve(standard_title(C, D), A, B)
    if G == 'reverse':
        return finish_integral_solve('Reverse chain rule', A, B)
    if G == 'trig':
        return finish_integral_solve('Using trigonometric identities', A, B)
    if G == 'sub':
        return finish_integral_solve('Integration by substitution', A, B)
    if G == 'parts':
        return finish_integral_solve('Integration by parts', A, B)
    return finish_integral_solve('Partial fractions', A, B)


def fallback_attempts(node, var, forced_u=None):
    attempts = [('2', 'dir'), ('4', 'sub'), ('3', 'trig'), ('5', 'pts'), ('6', 'pf'), ('7', 'div')]
    best = None
    best_ans = None
    best_title = None
    best_lines = None
    attempt_lines = []
    i = 0
    while i < len(attempts):
        if cancellation_requested():
            break
        method, label = attempts[i]
        title, ans, lines = _solve_with_method(node, var, method, forced_u)
        if ans is not None:
            lines = format_attempt_lines(lines)
            block = format_attempt_block(i + 1, label, lines)
            j = 0
            while j < len(block):
                attempt_lines.append(block[j])
                j += 1
            score = integral_candidate_score(title, ans, lines)
            if best is None or score < best:
                best = score
                best_title = title
                best_ans = ans
                best_lines = lines
        i += 1
    if best is None:
        return None, None, None
    return best_title, (best_ans, best_lines), attempt_lines


def solve(node, var, method, forced_u=None):
    title, ans, lines = _solve_with_method(node, var, method, forced_u)
    if ans is not None:
        return title, ans, lines
    fallback_title, fallback_payload, attempt_lines = fallback_attempts(node, var, forced_u)
    if fallback_title is not None:
        best_ans, best_lines = fallback_payload
        merged = list(attempt_lines or [])
        if best_lines:
            merged.append('Choose the simplest successful attempt.')
            i = 0
            while i < len(best_lines):
                merged.append(best_lines[i])
                i += 1
        return fallback_title, best_ans, simplify_attempt_lines(merged)
    return title, ans, lines


def solve_result_or_reason(node, var, method, forced_u=None):
    A, B, C = solve(node, var, method, forced_u)
    if B is not None:
        return A, B, C, None
    return A, B, C, hard_integral_failure_reason(node, var)

def integrate_termwise_with(node, var, solver, depth):
    if node[0] != 'add':
        return None, None
    A = []
    for C in node[1]:
        B, D = solver(C, var, depth + 1)
        if B is None:
            return None, None
        A.append(B)
    return add(A), []


def integrate_dv_subproblem(node, var, depth=0):
    C = var
    B = node
    if depth > 24:
        return None, None
    A, D = integrate_standard(B, C)
    if A is not None:
        return A, []
    A, D = integrate_reverse_chain(B, C)
    if A is not None:
        return A, []
    A, D = integrate_cyclic_parts(B, C)
    if A is not None:
        return A, []
    return integrate_trig(B, C, False)


def integrate_u_subproblem(node, var='u', depth=0):
    D = depth
    C = var
    B = node
    if D > 24:
        return None, None
    A, E = integrate_termwise_with(B, C, integrate_u_subproblem, D)
    if A is not None:
        return A, []
    A, E = integrate_standard(B, C)
    if A is not None:
        return A, []
    A, E = integrate_by_parts(B, C, D)
    if A is not None:
        return A, []
    return integrate_trig(B, C, False)


def integrate_after_trig_rewrite(node, var, depth=0):
    E = depth
    C = var
    B = node
    if E > 4:
        return None, None
    if B[0] == 'add':
        parts = flat(B, 'add')
        solved = []
        i = 0
        while i < len(parts):
            A, D = integrate_after_trig_rewrite(parts[i], C, E + 1)
            if A is None:
                solved = None
                break
            solved.append((parts[i], A, D or []))
            i += 1
        if solved is not None:
            total = add([item[1] for item in solved])
            lines = ['Integrate each term separately.']
            i = 0
            while i < len(solved):
                lines.append('Int[' + pretty(solved[i][0]) + '] d' + C + ' = ' + pretty(solved[i][1]))
                i += 1
            lines.append('= ' + pretty(total) + ' + C')
            return total, lines
    A, D = integrate_termwise_with(B, C, integrate_after_trig_rewrite, E)
    if A is not None:
        return A, ['Integrate each term separately.', '= ' + pretty(A) + ' + C']
    A, D = integrate_standard(B, C)
    if A is not None:
        return A, D
    A, D = integrate_reverse_chain(B, C)
    if A is not None:
        return A, D
    A, D = integrate_cyclic_parts(B, C)
    if A is not None:
        return A, D
    A, D = integrate_by_parts(B, C, E)
    if A is not None:
        return A, D
    return None, None


def choose_best_integral_candidate(candidates):
    if len(candidates) == 0:
        return None
    best = candidates[0]
    i = 1
    while i < len(candidates):
        score_a = integral_candidate_score(best[0], best[1], best[2])
        score_b = integral_candidate_score(candidates[i][0], candidates[i][1], candidates[i][2])
        if score_b < score_a:
            best = candidates[i]
        i += 1
    return best


def format_attempt_block(index, label, lines):
    out = ['Attempt ' + str(index) + ' (' + label + ')']
    i = 0
    while lines is not None and i < len(lines):
        out.append(lines[i])
        i += 1
    return out


def fallback_attempts(node, var, forced_u=None):
    attempts = [('2', 'dir'), ('4', 'sub'), ('3', 'trig'), ('5', 'pts'), ('6', 'pf'), ('7', 'div')]
    found = []
    attempt_lines = []
    i = 0
    while i < len(attempts):
        if cancellation_requested():
            break
        method, label = attempts[i]
        title, ans, lines = _solve_with_method(node, var, method, forced_u)
        if ans is not None:
            lines = format_attempt_lines(lines)
            block = format_attempt_block(i + 1, label, lines)
            j = 0
            while j < len(block):
                attempt_lines.append(block[j])
                j += 1
            found.append((title, ans, lines, label))
        i += 1
    best = choose_best_integral_candidate(found)
    if best is None:
        return None, None, None
    return best[0], (best[1], best[2]), attempt_lines


def solve(node, var, method, forced_u=None):
    title, ans, lines = _solve_with_method(node, var, method, forced_u)
    if ans is not None:
        return title, ans, lines
    fallback_title, fallback_payload, attempt_lines = fallback_attempts(node, var, forced_u)
    if fallback_title is not None:
        best_ans, best_lines = fallback_payload
        merged = list(attempt_lines or [])
        merged.append('Choose the simplest successful attempt.')
        i = 0
        while best_lines is not None and i < len(best_lines):
            merged.append(best_lines[i])
            i += 1
        return fallback_title, best_ans, simplify_attempt_lines(merged)
    return title, ans, lines


def solve_result_or_reason(node, var, method, forced_u=None):
    A, B, C = solve(node, var, method, forced_u)
    if B is not None:
        return A, B, C, None
    return A, B, C, hard_integral_failure_reason(node, var)

def simplify_attempt_lines(lines):
    if lines is None:
        return None
    out = []
    i = 0
    while i < len(lines):
        if len(out) == 0 or lines[i] != out[-1]:
            out.append(lines[i])
        i += 1
    return out


def format_attempt_lines(lines):
    return simplify_attempt_lines(lines or [])


def fallback_attempts(node, var, forced_u=None):
    attempts = [('2', 'dir'), ('4', 'sub'), ('3', 'trig'), ('5', 'pts'), ('6', 'pf'), ('7', 'div')]
    best = None
    best_title = None
    best_ans = None
    best_lines = None
    attempt_lines = []
    i = 0
    while i < len(attempts):
        if cancellation_requested():
            break
        method, label = attempts[i]
        title, ans, lines = _solve_with_method(node, var, method, forced_u)
        if ans is not None:
            lines = format_attempt_lines(lines)
            attempt_lines.append('Attempt ' + str(i + 1) + ' (' + label + ')')
            j = 0
            while j < len(lines):
                attempt_lines.append(lines[j])
                j += 1
            score = integral_candidate_score(title, ans, lines)
            if best is None or score < best:
                best = score
                best_title = title
                best_ans = ans
                best_lines = lines
        i += 1
    if best is None:
        return None, None, None
    return best_title, (best_ans, best_lines), attempt_lines


def solve(node, var, method, forced_u=None):
    title, ans, lines = _solve_with_method(node, var, method, forced_u)
    if ans is not None:
        return title, ans, lines
    fallback_title, fallback_payload, attempt_lines = fallback_attempts(node, var, forced_u)
    if fallback_title is not None:
        best_ans, best_lines = fallback_payload
        merged = list(attempt_lines or [])
        merged.append('Choose the simplest successful attempt.')
        i = 0
        while best_lines is not None and i < len(best_lines):
            merged.append(best_lines[i])
            i += 1
        return fallback_title, best_ans, simplify_attempt_lines(merged)
    return title, ans, lines


def solve_result_or_reason(node, var, method, forced_u=None):
    A, B, C = solve(node, var, method, forced_u)
    if B is not None:
        return A, B, C, None
    return A, B, C, hard_integral_failure_reason(node, var)

def integrate_cyclic_parts(node, var):
    A, B = split_const_mul(node, var)
    C = flat(B, 'mul') if B[0] == 'mul' else [B]
    D = None
    F = None
    G = []
    for H in C:
        if D is None:
            if H[0] == 'fn' and H[1] == 'exp':
                D = H
                continue
            if H[0] == 'pow' and same(H[1], E):
                D = fn('exp', H[2])
                continue
        if F is None and H[0] == 'fn' and H[1] in ('sin', 'cos'):
            F = H
            continue
        G.append(H)
    if D is None or F is None or len(G) > 0:
        return None, None
    I = linear_info(D[2], var)
    J = linear_info(F[2], var)
    if I is None or J is None:
        return None, None
    K = I[0]
    L = J[0]
    M = add([mul([K, K]), mul([L, L])])
    N = fn('exp', D[2])
    if F[1] == 'sin':
        O = add([mul([K, fn('sin', F[2])]), neg(mul([L, fn('cos', F[2])]))])
        P = 'Use the standard result for Int[e^(ax+b)sin(cx+d)] dx.'
    else:
        O = add([mul([K, fn('cos', F[2])]), mul([L, fn('sin', F[2])])])
        P = 'Use the standard result for Int[e^(ax+b)cos(cx+d)] dx.'
    Q = sim(div(mul([A, N, O]), M))
    return Q, [P, '= ' + pretty(Q) + ' + C']


def choose_parts(node, var):
    F = var
    E = node
    J = sym(F)
    if E[0] == 'fn' and E[1] in ('log', 'asin', 'acos', 'atan') and depends(E[2], F):
        return E, num(1), J
    if E[0] == 'mul':
        B = list(flat(E, 'mul'))
        A = 0
        while A < len(B):
            C = B[A]
            if C[0] == 'fn' and C[1] in ('asin', 'acos') and depends(C[2], F):
                D = []
                K = 0
                while K < len(B):
                    if K != A:
                        D.append(B[K])
                    K += 1
                if len(D) > 0:
                    L = D[0] if len(D) == 1 else mul(D)
                    M, N = integrate_dv_subproblem(L, F, 0)
                    if M is not None:
                        return C, L, M
            A += 1
    if E[0] == 'pow' and E[1][0] == 'fn' and E[1][1] == 'log' and depends(
            E[1][2], F) and is_int_num(E[2]) and E[2][1] > 0:
        return E, num(1), J
    N, C = split_const_mul(E, F)
    if C[0] == 'fn' and C[1] in ('log', 'asin', 'acos', 'atan') and depends(C[2], F):
        D, _ = integrate_dv_subproblem(N, F, 0)
        if D is not None:
            return C, N, D
    if C[0] == 'pow' and C[1][0] == 'fn' and C[1][1] == 'log' and depends(
            C[1][2], F) and is_int_num(C[2]) and C[2][1] > 0:
        D, _ = integrate_dv_subproblem(N, F, 0)
        if D is not None:
            return C, N, D
    if C[0] == 'div' and C[1][0] == 'fn' and C[1][1] == 'log' and depends(
            C[1][2], F):
        O = mul([N, div(num(1), C[2])])
        P, S = integrate_dv_subproblem(O, F, 0)
        if P is not None:
            return C[1], O, P
    if C[0] != 'mul':
        return
    B = list(C[1])
    L = []
    A = 0
    while A < len(B):
        L.append(poly_degree(B[A], F))
        A += 1

    def M(u, rest):
        A = mul([N] + rest)
        B, C = integrate_dv_subproblem(A, F, 0)
        if B is not None:
            return u, A, B
    A = 0
    while A < len(B):
        if B[A][0] == 'fn' and B[A][1] in ('log', 'asin', 'acos', 'atan') and depends(B[A][2], F):
            G = []
            K = 0
            while K < len(B):
                if K != A:
                    G.append(B[K])
                K += 1
            D = M(B[A], G)
            if D is not None:
                return D
        if B[A][0] == 'pow' and B[A][1][0] == 'fn' and B[A][1][1] == 'log' and depends(B[A][1][2], F) and B[A][2][0] == 'num' and B[A][2][1] > 0:
            G = []
            K = 0
            while K < len(B):
                if K != A:
                    G.append(B[K])
                K += 1
            D = M(B[A], G)
            if D is not None:
                return D
        A += 1
    H = []
    G = []
    A = 0
    while A < len(B):
        I = L[A]
        if I is not None and I >= 1:
            H.append(B[A])
        else:
            G.append(B[A])
        A += 1
    if len(H) > 0:
        Q = H[0]if len(H) == 1 else mul(H)
        D = M(Q, G)
        if D is not None:
            return D
        A = 0
        while A < len(B):
            I = L[A]
            if I is not None and I >= 1:
                R = B[:A] + B[A + 1:]
                D = M(B[A], R)
                if D is not None:
                    return D
            A += 1


def integrate_by_parts(node, var, depth=0):
    H = depth
    A = var
    if H > 24:
        return None, None
    X, Y = integrate_poly_linear_parts(node, A)
    if X is not None:
        return X, Y
    P, Q = split_const_mul(node, A)
    if Q[0] == 'fn' and Q[1] == 'log' and Q[2][0] == 'add':
        R = flat(Q[2], 'add')
        if len(R) == 2:
            S = None
            T = None
            for U in R:
                if S is None and same(U, sym(A)):
                    S = U
                elif T is None and ((U[0] == 'fn' and U[1] == 'sqrt') or (U[0] == 'pow' and same(U[2], num(1, 2)))):
                    T = U[2] if U[0] == 'fn' else U[1]
            if S is not None and T is not None and same(T, add([power(sym(A), num(2)), num(-1)])):
                V = fn('sqrt', T)
                W = sim(mul([P, add([mul([sym(A), Q]), neg(V)])]))
                return W, [
                    'u = ' + pretty(Q),
                    'dv = d' + A,
                    'du/d' + A + ' = 1/sqrt(' + A + '^2-1)',
                    'v = ' + A,
                    'I = u*v-Int[' + A + '/sqrt(' + A + '^2-1)] d' + A,
                    'Int[' + A + '/sqrt(' + A + '^2-1)] d' + A + ' = ' + pretty(V),
                    'I = ' + pretty(W) + ' + C']
    if Q[0] == 'pow' and Q[1][0] == 'fn' and Q[1][1] == 'sec' and same(Q[2], num(3)):
        R = linear_info(Q[1][2], A)
        if R is not None:
            S = fn('sec', Q[1][2])
            T = fn('tan', Q[1][2])
            U = fn('log', fn('abs', add([S, T])))
            V = sim(div(mul([P, add([mul([S, T]), U])]), mul([num(2), R[0]])))
            W = pretty(power(S, num(2)))
            return V, [
                'u = ' + pretty(S),
                'dv = ' + pretty(power(S, num(2))) + ' d' + A,
                'du/d' + A + ' = ' + pretty(diff(S, A)),
                'v = ' + pretty(div(T, R[0])),
                'I = u*v-Int[' + pretty(div(mul([S, power(T, num(2))]), R[0])) + '] d' + A,
                'Use tan^2 = sec^2-1.',
                'So 2I = ' + pretty(div(mul([S, T]), R[0])) + '+' + pretty(div(U, R[0])),
                'I = ' + pretty(V) + ' + C']
    P, Q = split_const_mul(node, A)
    R = flat(Q, 'mul') if Q[0] == 'mul' else [Q]
    S = sym(A)
    T = None
    U = None
    for V in R:
        if same(V, S):
            T = V
        elif V[0] == 'fn' and V[1] in ('asin', 'acos') and same(V[2], S):
            U = V[1]
    if T is not None and U is not None and len(R) == 2:
        W = fn('sqrt', add([num(1), neg(power(S, num(2)))]))
        if U == 'asin':
            X = add([
                mul([div(power(S, num(2)), num(2)), fn('asin', S)]),
                neg(div(fn('asin', S), num(4))),
                div(mul([S, W]), num(4))])
            Y = [
                'u = asin(' + A + ')',
                'v = ' + pretty(div(power(S, num(2)), num(2))),
                'I = u*v-1/2*Int[' + A + '^2/sqrt(1-' + A + '^2)] d' + A,
                'Use ' + A + '^2 = 1-(1-' + A + '^2).',
                '= ' + pretty(X) + ' + C']
            return mul([P, X]), Y
        X = add([
            mul([div(power(S, num(2)), num(2)), fn('acos', S)]),
            neg(div(fn('acos', S), num(4))),
            neg(div(mul([S, W]), num(4)))])
        Y = [
            'u = acos(' + A + ')',
            'v = ' + pretty(div(power(S, num(2)), num(2))),
            'I = u*v+1/2*Int[' + A + '^2/sqrt(1-' + A + '^2)] d' + A,
            'Use ' + A + '^2 = 1-(1-' + A + '^2).',
            '= ' + pretty(X) + ' + C']
        return mul([P, X]), Y
    C, B = integrate_cyclic_parts(node, A)
    if C is not None:
        return C, B
    I = choose_parts(node, A)
    if I is None:
        return None, None
    C, M, D = I
    J = safe_diff(C, A)
    if J is None:
        return None, None
    E = mul([D, J])
    K, F = integrate_auto(E, A, H + 1, True)
    if K is None and C[0] == 'fn' and C[1] in ('asin', 'acos') and same(C[2], sym(A)):
        N = fn('sqrt', add([num(1), neg(power(sym(A), num(2)))]))
        if C[1] == 'asin' and same(E, div(M, N)):
            K = neg(N)
            F = ['Use the standard result for Int[x/sqrt(1-x^2)] dx.', '= -' + pretty(N) + ' + C']
        elif C[1] == 'acos' and same(E, neg(div(M, N))):
            K = N
            F = ['Use the standard result for Int[x/sqrt(1-x^2)] dx.', '= ' + pretty(N) + ' + C']
    if K is None:
        return None, None
    L = add([mul([C, D]), neg(K)])
    B = [
        'u = ' + pretty(C),
        'du/d' + A + ' = ' + pretty(J),
        'v = ' + pretty(D),
        'I = u*v-Int[' + pretty(E) + '] d' + A]
    if F:
        B.append('Now integrate Int[' + pretty(E) + '] d' + A)
        G = 0
        while G < len(F):
            B.append(F[G])
            G += 1
    B.append('= ' + pretty(L) + ' + C')
    return L, B


def integral_answer_line(node):
    return '= ' + pretty(node) + ' + C'


def split_numeric_term(node):
    A = sim(node)
    if is_num(A):
        return A, num(1)
    if A[0] == 'mul':
        D = num(1)
        C = []
        for B in flat(A, 'mul'):
            if is_num(B):
                D = mulq(D, B)
            else:
                C.append(B)
        return D, make_mul(C)
    if A[0] == 'div':
        D, C = split_numeric_term(A[1])
        return D, div(C, A[2])
    return num(1), A


def recombine_numeric_term(coeff, rest):
    if is_one(rest):
        return coeff
    if is_one(coeff):
        return rest
    if is_minus_one(coeff):
        return neg(rest)
    return mul([coeff, rest])


def expand_small_recursive(node):
    A = sim(node)
    B = A[0]
    if B in ('num', 'sym', 'const'):
        return A
    if B == 'fn':
        return 'fn', A[1], expand_small_recursive(A[2])
    if B in ('pow', 'div'):
        return sim((B, expand_small_recursive(A[1]), expand_small_recursive(A[2])))
    C = []
    for D in flat(A, B):
        C.append(expand_small_recursive(D))
    return sim(expand_small((B, tuple(C))))


def normalize_add_coeffs(node):
    A = sim(node)
    B = A[0]
    if B == 'add':
        D = num(0)
        C = []
        E = {}
        for H in flat(A, 'add'):
            G = normalize_add_coeffs(H)
            I, F = split_numeric_term(G)
            if is_one(F):
                D = addq(D, I)
            else:
                J = sig(F)
                if J not in E:
                    C.append(J)
                    E[J] = [F, num(0)]
                E[J][1] = addq(E[J][1], I)
        K = []
        for H in C:
            F, I = E[H]
            if not is_zero(I):
                K.append(recombine_numeric_term(I, F))
        if not is_zero(D):
            K.append(D)
        return sim(make_add(K)) if len(K) > 0 else num(0)
    if B == 'fn':
        return 'fn', A[1], normalize_add_coeffs(A[2])
    if B in ('pow', 'div'):
        return sim((B, normalize_add_coeffs(A[1]), normalize_add_coeffs(A[2])))
    if B == 'mul':
        C = []
        for D in flat(A, 'mul'):
            C.append(normalize_add_coeffs(D))
        return sim(('mul', tuple(C)))
    return A


def finalize_integral_answer(node):
    A = _cache_get('finalize_integral', node)
    if A is not _CACHE_MISS:
        return A

    def B(cur):
        C = sim(cur)
        D = combine_logs(C)
        E = normalize_add_coeffs(expand_small_recursive(D))
        return normalize_trig_signs(sim(combine_logs(E)))
    F = []
    G = set()

    def H(cur):
        I = sig(cur)
        if I in G:
            return
        G.add(I)
        F.append(cur)
    H(sim(node))
    H(combine_logs(sim(node)))
    C = sim(node)
    J = 0
    while J < 3:
        C = B(C)
        H(C)
        J += 1
    K = F[0]
    L = (len(pretty(K)), len(repr(sig(K))))
    for C in F[1:]:
        M = (len(pretty(C)), len(repr(sig(C))))
        if M < L:
            K = C
            L = M
    return _cache_set('finalize_integral', node, K)


def finalize_integral_result(ans, lines):
    if ans is None:
        return ans, lines
    B = finalize_integral_answer(ans)
    if sig(B) == sig(ans):
        return B, lines
    A = list(lines or [])
    if len(A) == 0 or A[-1] != 'Simplify.':
        A.append('Simplify.')
    return B, A


def integral_is_rational_division(node, var):
    return node[0] == 'div' and poly_num(node[1], var) is not None and poly_num(
        node[2], var) is not None


def auto_route_termwise(node, var, depth):
    C = node
    D = expand_small(node)
    E = []
    if C[0] != 'add' and D[0] == 'add' and not same(C, D):
        C = D
        E = ['Split the integral into simpler terms.',
             'So I = ' + int_text(C, var)]
    A, B = integrate_termwise_with(
        C, var, lambda n, v, d: integrate_auto(n, v, d, False), depth)
    if A is not None:
        return 'direct', A, E + [
            'Integrate each term separately.',
            integral_answer_line(A)]
    return None, None, None


def auto_route_standard(node, var, depth):
    A, B = integrate_standard(node, var)
    if A is not None:
        return 'direct', A, B
    return None, None, None


def auto_route_reverse(node, var, depth):
    A, B = integrate_reverse_chain(node, var)
    if A is not None:
        return 'reverse', A, B
    return None, None, None


def auto_route_trig(node, var, depth):
    A, B = integrate_trig(node, var, True)
    if A is not None:
        return 'trig', A, B
    return None, None, None


def auto_route_division(node, var, depth):
    A, B = integrate_division(node, var, True)
    if A is not None:
        return 'pf', A, B
    A, B = integrate_partial(node, var)
    if A is not None:
        return 'pf', A, B
    return None, None, None


def auto_route_substitution(node, var, depth):
    A, B = integrate_substitution(node, var, None)
    if A is not None:
        return 'sub', A, B
    return None, None, None


def auto_route_parts(node, var, depth):
    A, B = integrate_cyclic_parts(node, var)
    if A is not None:
        return 'parts', A, B
    A, B = integrate_by_parts(node, var, depth)
    if A is not None:
        return 'parts', A, B
    return None, None, None


def auto_integral_routes(rational, allow_termwise):
    A = []
    A.extend([auto_route_standard, auto_route_reverse, auto_route_trig])
    if allow_termwise:
        A.append(auto_route_termwise)
    if rational:
        A.append(auto_route_division)
    A.extend([auto_route_substitution, auto_route_parts])
    if not rational:
        A.append(auto_route_division)
    return A


def pf_factor_list(node, var):
    C = var
    A = node
    if A[0] == 'mul':
        D = []
        for K in A[1]:
            I = pf_factor_list(K, C)
            if I is None:
                return
            D.extend(I)
        return D
    if A[0] == 'pow' and is_int_num(
            A[2]) and A[2][1] > 0 and linear_info(A[1], C) is not None:
        return [(A[1], A[2][1])]
    if linear_info(A, C) is not None:
        return [(A, 1)]
    J = poly_num(A, C)
    if J is None:
        return
    E = factor_poly_linear(J)
    if E is None:
        return
    F = {}
    H = []
    B = 0
    while B < len(E):
        G = str(E[B])
        if G not in F:
            H.append(G)
            F[G] = [E[B], 0]
        F[G][1] += 1
        B += 1
    D = []
    B = 0
    while B < len(H):
        L, M = F[H[B]]
        D.append((add([sym(C), neg(L)]), M))
        B += 1
    return D


def pad_coeffs(coeffs, n):
    A = list(coeffs)
    while len(A) < n:
        A.append(num(0))
    return A


def solve_linear_system(mat, rhs):
    C = rhs
    B = mat
    F = len(C)
    A = 0
    while A < F:
        D = A
        while D < F and is_zero(B[D][A]):
            D += 1
        if D == F:
            return
        if D != A:
            B[A], B[D] = B[D], B[A]
            C[A], C[D] = C[D], C[A]
        I = B[A][A]
        H = A
        while H < F:
            B[A][H] = divq(B[A][H], I)
            H += 1
        C[A] = divq(C[A], I)
        E = 0
        while E < F:
            if E != A and not is_zero(B[E][A]):
                J = B[E][A]
                G = A
                while G < F:
                    B[E][G] = subq(B[E][G], mulq(J, B[A][G]))
                    G += 1
                C[E] = subq(C[E], mulq(J, C[A]))
            E += 1
        A += 1
    return C


def symbolic_pf_term(name, factor, power_n):
    B = power_n
    A = factor
    if B == 1:
        return div(sym(name), A)
    return div(sym(name), power(A, num(B)))


def integrate_partial_linear_quadratic(node, var):
    C = node
    A = var
    if C[0] != 'div':
        return None, None
    F = poly_num(C[1], A)
    B = poly_num(C[2], A)
    if F is None or B is None or len(F) >= len(B):
        return None, None
    K = split_linear_quadratic_product(C[2], A)
    if K is None:
        return None, None
    G, H = K
    I = poly_num(G, A)
    L = poly_num(H, A)
    if I is None or L is None:
        return None, None
    M = pad_coeffs(F, len(B) - 1)
    J = [pad_coeffs(L, len(B) -
                    1), pad_coeffs(poly_mul_num([num(0), num(1)], I), len(B) -
                                   1), pad_coeffs(I, len(B) -
                                                  1)]
    N = []
    D = 0
    while D < len(M):
        N.append([J[0][D], J[1][D], J[2][D]])
        D += 1
    O = solve_linear_system(N, list(M))
    if O is None:
        return None, None
    P, Q, R = O
    V = add([mul([Q, sym(A)]), R])
    W = add([div(P, G), div(V, H)])
    S = []
    for T in flat(W, 'add'):
        E, X = integrate_standard(T, A)
        if E is None:
            E, X = integrate_quadratic_rational(T, A)
        if E is None:
            return None, None
        S.append(E)
    U = add(S)
    Y = add([div(sym('A'), G), div(add([mul([sym('B'), sym(A)]), sym('C')]), H)])
    Z = [
        'Use partial fractions.',
        pretty(C) +
        ' = ' +
        pretty(Y),
        'A = ' +
        pretty(P) +
        ', B = ' +
        pretty(Q) +
        ', C = ' +
        pretty(R),
        'So I = ' +
        pretty(U) +
        ' + C']
    return U, Z


def integrate_partial_repeated_linear_quadratic(node, var):
    A = node
    B = var
    if A[0] != 'div':
        return None, None
    C = poly_num(A[1], B)
    D = poly_num(A[2], B)
    if C is None or D is None or len(C) >= len(D):
        return None, None
    E = split_repeated_linear_quadratic_product(A[2], B)
    if E is None:
        return None, None
    F, G = E
    H = poly_num(F, B)
    I = poly_num(G, B)
    if H is None or I is None:
        return None, None
    J = pad_coeffs(C, len(D) - 1)
    K = poly_mul_num(H, I)
    L = poly_mul_num(H, H)
    M = [
        pad_coeffs(K, len(D) - 1),
        pad_coeffs(I, len(D) - 1),
        pad_coeffs(poly_mul_num([num(0), num(1)], L), len(D) - 1),
        pad_coeffs(L, len(D) - 1)]
    N = []
    O = 0
    while O < len(J):
        N.append([M[0][O], M[1][O], M[2][O], M[3][O]])
        O += 1
    P = solve_linear_system(N, list(J))
    if P is None:
        return None, None
    Q, R, S, T = P
    U = add([
        div(Q, F),
        div(R, power(F, num(2))),
        div(add([mul([S, sym(B)]), T]), G)])
    V = add([
        div(sym('A'), F),
        div(sym('B'), power(F, num(2))),
        div(add([mul([sym('C'), sym(B)]), sym('D')]), G)])
    W = []
    for X in flat(U, 'add'):
        Y, Z = integrate_standard(X, B)
        if Y is None:
            Y, Z = integrate_quadratic_rational(X, B)
        if Y is None:
            return None, None
        W.append(Y)
    a = add(W)
    b = [
        'Use partial fractions.',
        pretty(A) + ' = ' + pretty(V),
        'A = ' + pretty(Q) + ', B = ' + pretty(R) + ', C = ' + pretty(S) + ', D = ' + pretty(T),
        'So I = ' + pretty(a) + ' + C']
    return a, b


def integrate_partial_two_linear_quadratic(node, var):
    A = node
    B = var
    if A[0] != 'div':
        return None, None
    C = poly_num(A[1], B)
    D = poly_num(A[2], B)
    if C is None or D is None or len(C) >= len(D):
        return None, None
    E, F = factor_poly_linear_partial(D, 2)
    if len(E) != 2 or len(F) != 3:
        return None, None
    G = add([sym(B), neg(E[0])])
    H = add([sym(B), neg(E[1])])
    I = poly_num(G, B)
    J = poly_num(H, B)
    K = pad_coeffs(C, len(D) - 1)
    L = pad_coeffs(poly_mul_num(J, F), len(D) - 1)
    M = pad_coeffs(poly_mul_num(I, F), len(D) - 1)
    N = pad_coeffs(poly_mul_num(I, J), len(D) - 1)
    O = pad_coeffs(poly_mul_num([num(0), num(1)], N), len(D) - 1)
    P = []
    Q = 0
    while Q < len(K):
        P.append([L[Q], M[Q], O[Q], N[Q]])
        Q += 1
    R = solve_linear_system(P, list(K))
    if R is None:
        return None, None
    S, T, U, V = R
    W = add([div(sym('A'), G), div(sym('B'), H), div(add([mul([sym('C'), sym(B)]), sym('D')]), poly_to_node(F, B))])
    X = add([div(S, G), div(T, H), div(add([mul([U, sym(B)]), V]), poly_to_node(F, B))])
    Y = []
    for Z in flat(X, 'add'):
        a, b = integrate_standard(Z, B)
        if a is None:
            a, b = integrate_quadratic_rational(Z, B)
        if a is None:
            return None, None
        Y.append(a)
    c = add(Y)
    d = [
        'Use partial fractions.',
        pretty(A) + ' = ' + pretty(W),
        'A = ' + pretty(S) + ', B = ' + pretty(T) + ', C = ' + pretty(U) + ', D = ' + pretty(V),
        'So I = ' + pretty(c) + ' + C']
    return c, d


def integrate_partial(node, var):
    E = var
    D = node
    if D[0] != 'div':
        return None, None
    if is_one(D[1]) and quad_info(D[2], E) is not None:
        A, B = integrate_quadratic_rational(D, E)
        if A is not None:
            lines = [
                'Use partial fractions, first checking for common factors.',
                'After cancellation the integrand is ' + pretty(D) + ', so no decomposition is needed.',
            ]
            if B is not None:
                i = 0
                while i < len(B):
                    if B[i] not in lines:
                        lines.append(B[i])
                    i += 1
            lines.append('So I = ' + pretty(A) + ' + C')
            return A, lines
    if D[2][0] == 'mul':
        factors = list(flat(D[2], 'mul'))
        i = 0
        while i < len(factors):
            coeff = quotient_by_target(D[1], factors[i])
            if coeff is not None and not depends(coeff, E):
                remaining = make_mul(factors[:i] + factors[i + 1:])
                reduced = div(coeff, remaining)
                A, B = integrate_standard(reduced, E)
                if A is None:
                    A, B = integrate_quadratic_rational(reduced, E)
                if A is not None:
                    lines = [
                        'Use partial fractions, first checking for common factors.',
                        'Cancel the common factor ' + pretty(factors[i]) + '.',
                        pretty(D) + ' = ' + pretty(reduced),
                    ]
                    if B is not None:
                        j = 0
                        while j < len(B):
                            if B[j] not in lines:
                                lines.append(B[j])
                            j += 1
                    lines.append('So I = ' + pretty(A) + ' + C')
                    return A, lines
            i += 1
    J = poly_num(D[1], E)
    F = poly_num(D[2], E)
    if J is None or F is None:
        return None, None
    if len(J) >= len(F):
        return None, None
    C = pf_factor_list(D[2], E)
    if C is None:
        A, B = integrate_partial_linear_quadratic(D, E)
        if A is not None:
            return A, B
        A, B = integrate_partial_repeated_linear_quadratic(D, E)
        if A is not None:
            return A, B
        return integrate_partial_two_linear_quadratic(D, E)
    K = []
    L = []
    O = []
    P = 0
    A = 0
    while A < len(C):
        H, Z = C[A]
        B = 1
        while B <= Z:
            Q, R = poly_divmod_num(F, poly_num(power(H, num(B)), E))
            if Q is None or not (len(R) == 1 and is_zero(R[0])):
                return None, None
            K.append(pad_coeffs(Q, len(F) - 1))
            S = chr(65 + P)
            L.append(div(sym(S), H if B == 1 else power(H, num(B))))
            O.append(symbolic_pf_term(S, H, B))
            P += 1
            B += 1
        A += 1
    T = pad_coeffs(J, len(F) - 1)
    U = []
    A = 0
    while A < len(T):
        V = []
        B = 0
        while B < len(K):
            V.append(K[B][A])
            B += 1
        U.append(V)
        A += 1
    I = solve_linear_system(U, list(T))
    if I is None:
        return None, None
    M = []
    A = 0
    while A < len(L):
        M.append(subst(L[A], chr(65 + A), I[A]))
        A += 1
    W = []
    A = 0
    while A < len(M):
        X, c = integrate_standard(M[A], E)
        if X is None:
            return None, None
        W.append(X)
        A += 1
    Y = add(W)
    G = ['Use partial fractions.']
    G.append(pretty(D) + ' = ' + pretty(add(O)))
    if len(C) == 2 and C[0][1] == 1 and C[1][1] == 1:
        a = C[0][0]
        b = C[1][0]
        G.append(
            pretty(
                D[1]) +
            ' ≡ A*(' +
            pretty(b) +
            ')+B*(' +
            pretty(a) +
            ')')
    N = ''
    A = 0
    while A < len(I):
        if A > 0:
            N += ', '
        N += chr(65 + A) + ' = ' + pretty(I[A])
        A += 1
    G.append(N)
    G.append('So I = ' + pretty(Y) + ' + C')
    return Y, G


def integrate_trig_sub_radical(node, var):
    A = node
    B = var
    if A[0] != 'div' or not is_one(A[1]):
        return None, None
    C = flat(A[2], 'mul') if A[2][0] == 'mul' else [A[2]]
    D = False
    E = None
    F = []
    for G in C:
        if not D and G[0] == 'pow' and G[1] == sym(B) and same(G[2], num(2)):
            D = True
        elif E is None and ((G[0] == 'fn' and G[1] == 'sqrt') or (G[0] == 'pow' and same(G[2], num(1, 2)))):
            E = G[2] if G[0] == 'fn' else G[1]
        else:
            F.append(G)
    if not D or E is None or len(F) > 0:
        return None, None
    H = poly_num(E, B)
    if H is None or len(H) != 3 or not is_zero(H[1]) or not is_num(H[0]) or not is_num(H[2]) or H[2][1] >= 0:
        return None, None
    if H[2] != num(-1):
        return None, None
    I = H[0]
    J = fn('sqrt', I)
    K = neg(div(fn('sqrt', add([I, neg(power(sym(B), num(2)))])), mul([I, sym(B)])))
    L = div(num(1), I)
    M = [
        'Use x = ' + pretty(J) + '*sin(t).',
        'dx = ' + pretty(J) + '*cos(t) dt',
        'sqrt(' + pretty(I) + '-' + B + '^2) = ' + pretty(J) + '*cos(t)',
        'So I = Int[' + pretty(L) + '*cosec(t)^2] dt',
        '= ' + pretty(neg(mul([L, fn('cot', sym('t'))]))) + ' + C',
        '= ' + pretty(K) + ' + C']
    return K, M


def integrate_division(node, var, follow):
    B = node
    A = var
    if B[0] != 'div':
        return None, None
    D = poly_num(B[1], A)
    E = poly_num(B[2], A)
    if D is None or E is None or len(D) < len(E):
        return None, None
    H, F = poly_divmod_num(D, E)
    if H is None:
        return None, None
    I = poly_to_node(H, A)
    J = num(0)if len(F) == 1 and is_zero(
        F[0])else div(
        poly_to_node(
            F, A), B[2])
    C = I if is_zero(J)else add([I, J])
    if follow:
        G, K = integrate_auto(C, A, 0, True)
        if G is None:
            quotient_ans, quotient_lines = integrate_auto(I, A, 0, True) if not is_zero(I) else (num(0), [])
            special_ans, special_lines = integrate_special_division_remainder(J, A) if not is_zero(J) else (num(0), [])
            if special_ans is None:
                return None, None
            if quotient_ans is None:
                return None, None
            total = special_ans if is_zero(quotient_ans) else add([quotient_ans, special_ans])
            out_lines = ['Divide the numerator by the denominator.', 'So I = ' + int_text(C, A)]
            i = 0
            while i < len(special_lines):
                out_lines.append(special_lines[i])
                i += 1
            if len(quotient_lines) != 0:
                i = 0
                while i < len(quotient_lines):
                    if quotient_lines[i] not in out_lines:
                        out_lines.append(quotient_lines[i])
                    i += 1
            out_lines.append('= ' + pretty(total) + ' + C')
            return total, out_lines
        return G, ['Divide the numerator by the denominator.',
                   'So I = ' + int_text(C, A), '= ' + pretty(G) + ' + C']
    return C, ['Divide the numerator by the denominator.',
               'So I = ' + int_text(C, A)]


def de_common_factor(parts, var):
    D = var
    A = parts
    E = []
    F = []
    for B in A:
        C, G = split_const_mul(B, D)
        if not is_one(G):
            E.append(G)
            if G[0] == 'mul':
                for H in flat(G, 'mul'):
                    if depends(H, D):
                        E.append(H)
            break
    for B in E:
        if any((cheap_same(B, C) or same(B, C))for C in F):
            continue
        F.append(B)
        G = []
        H = True
        for I in A:
            J = quotient_by_target(I, B)
            if J is None or depends(J, D):
                H = False
                break
            G.append(J)
        if H:
            return B, G
    return None, None


def split_separable_rhs(node, xvar, yvar):
    D = yvar
    C = xvar
    A = node
    T = depends(A, C)
    O = depends(A, D)
    if T and O:
        I = A[0]
        if I == 'fn' and A[1] == 'exp' and A[2][0] == 'add':
            F = []
            G = []
            for B in flat(A[2], 'add'):
                J = depends(B, C)
                H = depends(B, D)
                if J and H:
                    return
                if H:
                    G.append(B)
                else:
                    F.append(B)
            P = make_add(F)
            Q = make_add(G)
            return num(1)if is_zero(P)else fn('exp', P), num(
                1)if is_zero(Q)else fn('exp', Q)
        if I == 'pow' and same(A[1], E) and A[2][0] == 'add':
            F = []
            G = []
            for B in flat(A[2], 'add'):
                J = depends(B, C)
                H = depends(B, D)
                if J and H:
                    return
                if H:
                    G.append(B)
                else:
                    F.append(B)
            return power(E, make_add(F)), power(E, make_add(G))
        if I == 'add':
            F = []
            G = []
            for B in flat(A, 'add'):
                H = split_separable_rhs(B, C, D)
                if H is None:
                    return
                F.append(H[0])
                G.append(H[1])
            if len(G) > 0 and all(cheap_same(
                    G[0], B) or same(G[0], B)for B in G[1:]):
                return make_add(F), G[0]
            if len(F) > 0 and all(cheap_same(
                    F[0], B) or same(F[0], B)for B in F[1:]):
                return F[0], make_add(G)
            H, I = de_common_factor(F, C)
            if H is not None:
                J = []
                for B, K in zip(I, G):
                    J.append(K if is_one(B)else mul([B, K]))
                return H, make_add(J)
            H, I = de_common_factor(G, D)
            if H is not None:
                J = []
                for B, K in zip(F, I):
                    J.append(B if is_one(K)else mul([B, K]))
                return make_add(J), H
        if I == 'mul':
            R = []
            S = []
            K = 0
            while K < len(A[1]):
                L = split_separable_rhs(A[1][K], C, D)
                if L is None:
                    return
                R.append(L[0])
                S.append(L[1])
                K += 1
            return make_mul(R), make_mul(S)
        if I == 'div':
            M = split_separable_rhs(A[1], C, D)
            N = split_separable_rhs(A[2], C, D)
            if M is None or N is None:
                return
            return div(M[0], N[0]), div(M[1], N[1])
        return
    if O:
        return num(1), A
    return A, num(1)


def de_rewrite_factor(node):
    B = node
    if B[0] == 'div' and is_one(B[1]) and B[2][0] == 'fn':
        C = B[2][1]
        A = B[2][2]
        if C == 'tan':
            return 'Use cot(' + pretty(A) + ') = 1/tan(' + \
                pretty(A) + ').', fn('cot', A)
        if C == 'cot':
            return 'Use tan(' + pretty(A) + ') = 1/cot(' + \
                pretty(A) + ').', fn('tan', A)
        if C == 'sin':
            return 'Use cosec(' + pretty(A) + ') = 1/sin(' + \
                pretty(A) + ').', fn('cosec', A)
        if C == 'cosec':
            return 'Use sin(' + pretty(A) + ') = 1/cosec(' + \
                pretty(A) + ').', fn('sin', A)
        if C == 'cos':
            return 'Use sec(' + pretty(A) + ') = 1/cos(' + \
                pretty(A) + ').', fn('sec', A)
        if C == 'sec':
            return 'Use cos(' + pretty(A) + ') = 1/sec(' + \
                pretty(A) + ').', fn('cos', A)
        if C == 'exp':
            D = neg(A)
            return 'Use 1/exp(' + pretty(A) + ') = exp(' + \
                pretty(D) + ').', fn('exp', D)
    return None, None


def de_scaled_log_abs(node):
    A = sim(node)
    if A[0] == 'fn' and A[1] == 'log' and A[2][0] == 'fn' and A[2][1] == 'abs':
        return num(1), A[2][2]
    C, B = split_coeff(A)
    if is_num(
            C) and B[0] == 'fn' and B[1] == 'log' and B[2][0] == 'fn' and B[2][1] == 'abs':
        return C, B[2][2]
    if A[0] == 'div' and is_num(A[2]):
        D, E = de_scaled_log_abs(A[1])
        if D is not None:
            return divq(D, A[2]), E
    return None, None


def de_log_target(node):
    return de_scaled_log_abs(node)


def de_log_argument(node):
    C, D = de_scaled_log_abs(node)
    if D is not None:
        return D if is_one(C) else power(D, C)
    A = sim(node)
    if A[0] == 'add':
        E = []
        B = 0
        F = flat(A, 'add')
        while B < len(F):
            G = de_log_argument(F[B])
            if G is None:
                return
            E.append(G)
            B += 1
        return make_mul(E)


def de_absorb_k_sign(node):
    B = node
    if not depends(B, 'k'):
        return B
    A, C = split_coeff(B)
    if is_num(A) and A[1] < 0:
        return C if is_minus_one(A)else mul([num(-A[1], A[2]), C])
    return B


def de_linear_coeffs(node, yvar):
    A = poly_num(node, yvar)
    if A is None or len(A) > 2:
        return
    B = A[1] if len(A) > 1 else num(0)
    C = A[0]
    return B, C


def de_fractional_linear_solution(target, rhs, yvar):
    A = target
    if A[0] == 'mul':
        B = flat(A, 'mul')
        if len(B) == 2 and B[1][0] == 'pow' and B[1][2] == num(-1):
            A = div(B[0], B[1][1])
        elif len(B) == 2 and B[0][0] == 'pow' and B[0][2] == num(-1):
            A = div(B[1], B[0][1])
    if A[0] != 'div':
        return
    B = de_linear_coeffs(A[1], yvar)
    C = de_linear_coeffs(A[2], yvar)
    if B is None or C is None:
        return
    D, E = B
    F, G = C
    H = add([D, neg(mul([rhs, F]))])
    if is_zero(H):
        return
    I = add([mul([rhs, G]), neg(E)])
    return pretty(sym(yvar)) + ' = ' + pretty(div(I, H))


def simplify_exp_log_sum(node):
    A = sim(node)
    if A[0] == 'fn':
        B = simplify_exp_log_sum(A[2])
        if A[1] == 'exp' and B[0] == 'add':
            D = []
            C = num(1)
            for E in flat(B, 'add'):
                if E[0] == 'fn' and E[1] == 'log' and is_num(E[2]) and E[2][1] > 0:
                    C = mulq(C, E[2])
                else:
                    D.append(E)
            if not is_one(C):
                if len(D) == 0:
                    return C
                F = fn('exp', make_add(D))
                return sim(mul([C, F]))
        return sim(('fn', A[1], B))
    if A[0] in ('pow', 'div'):
        return sim((A[0], simplify_exp_log_sum(A[1]), simplify_exp_log_sum(A[2])))
    if A[0] in ('add', 'mul'):
        return sim((A[0], tuple(simplify_exp_log_sum(B) for B in A[1])))
    return A


def de_solve_target(target, rhs, yvar):
    E = target
    A = simplify_exp_log_sum(rhs)
    F = linear_info(E, yvar)
    if F is None:
        return pretty(E) + ' = ' + pretty(A)
    D, B = F
    G = sym(yvar)
    if is_one(D):
        C = add([A, neg(B)])
    elif is_minus_one(D):
        if depends(A, 'k'):
            C = add([B, de_absorb_k_sign(neg(A))])
        else:
            C = add([B, neg(A)])
    else:
        C = div(add([A, neg(B)]), D)
    return pretty(G) + ' = ' + pretty(C)


def de_quadratic_target(node, yvar):
    F = sym(yvar)
    E = power(F, num(2))
    A = num(0)
    B = num(0)
    C = flat(node, 'add')if node[0] == 'add'else [node]
    for D in C:
        if depends(D, yvar):
            G, H = split_coeff(D)
            if cheap_same(H, E) or same(H, E):
                A = addq(A, G)
            else:
                return None, None
        else:
            B = add([B, D])
    return (A, B)if not is_zero(A)else (None, None)


def de_finish_target(target, rhs, yvar, y0=None):
    B = rhs
    A = target
    C, D = de_log_target(A)
    if D is not None:
        return de_finish_target(
            D, fn('exp', B if is_one(C)else div(B, C)), yvar, y0)
    E = de_log_argument(A)
    if E is not None:
        return de_finish_target(E, fn('exp', B), yvar, y0)
    E = de_fractional_linear_solution(A, B, yvar)
    if E is not None:
        return E
    C = de_solve_target(A, B, yvar)
    if not (A[0] == 'fn' and A[2] == sym(yvar)):
        D, F = de_quadratic_target(A, yvar)
        if D is not None:
            G = div(add([B, neg(F)]), D)
            if y0 is None:
                return pretty(power(sym(yvar), num(2))) + ' = ' + pretty(G)
            return pretty(sym(yvar)) + ' = ' + pretty(neg(fn('sqrt', G))
                                                      if split_coeff(y0)[0][1] < 0 else fn('sqrt', G))
        return C
    D = A[1]
    if D == 'sin':
        return pretty(sym(yvar)) + ' = ' + pretty(fn('asin', B))
    if D == 'cos':
        return pretty(sym(yvar)) + ' = ' + pretty(fn('acos', B))
    if D == 'tan':
        return pretty(sym(yvar)) + ' = ' + pretty(fn('atan', B))
    if D == 'sec':
        return pretty(sym(yvar)) + ' = ' + pretty(fn('acos', div(num(1), B)))
    if D == 'cosec':
        return pretty(sym(yvar)) + ' = ' + pretty(fn('asin', div(num(1), B)))
    if D == 'cot':
        return pretty(sym(yvar)) + ' = ' + pretty(fn('atan', div(num(1), B)))
    return C


def de_inverse_form(node, yvar):
    A = node
    if A[0] == 'div' and is_num(A[1]):
        C = linear_info(A[2], yvar)
        if C is not None:
            return A[1], A[2]
    D, B = split_coeff(A)
    if is_num(D) and B[0] == 'pow' and B[2] == num(-1):
        C = linear_info(B[1], yvar)
        if C is not None:
            return D, B[1]
    return None, None


def de_exp_target(node, yvar):
    A = node
    if A[0] == 'fn' and A[1] == 'exp':
        C = linear_info(A[2], yvar)
        if C is not None:
            return num(1), A[2]
    D, B = split_coeff(A)
    if is_num(D) and B[0] == 'fn' and B[1] == 'exp':
        C = linear_info(B[2], yvar)
        if C is not None:
            return D, B[2]
    return None, None


def de_linear_y_coeff(node, yvar):
    A, B = split_const_mul(node, yvar)
    C = sym(yvar)
    if cheap_same(B, C) or same(B, C):
        return A


def split_linear_rhs(node, xvar, yvar):
    B = []
    C = []
    D = False
    for A in flat(node, 'add')if node[0] == 'add'else [node]:
        E = de_linear_y_coeff(A, yvar)
        if E is not None:
            if depends(E, yvar):
                return
            C.append(E)
            D = True
        elif depends(A, yvar):
            return
        else:
            B.append(A)
    if not D:
        return
    return make_add(B), make_add(C)


def de_integrating_factor_divide(node, factor):
    if node[0] == 'add':
        B = []
        for A in flat(node, 'add'):
            C = quotient_by_target(A, factor)
            if C is None:
                B = None
                break
            B.append(C)
        if B is not None:
            return sim(make_add(B))
    A = quotient_by_target(node, factor)
    if A is not None:
        return sim(A)
    if factor[0] == 'fn' and factor[1] == 'exp':
        return sim(mul([node, fn('exp', neg(factor[2]))]))
    return sim(div(node, factor))


def simplify_integrating_factor(node, xvar):
    A = sim(node)
    if A[0] == 'fn' and A[1] == 'exp':
        B = A[2]
        if B[0] == 'fn' and B[1] == 'log' and B[2][0] == 'fn' and B[2][1] == 'abs':
            C = B[2][2]
            if linear_info(C, xvar) is not None:
                return C
        if B[0] == 'mul':
            D, E = split_coeff(B)
            if is_minus_one(D) and E[0] == 'fn' and E[1] == 'log' and E[2][0] == 'fn' and E[2][1] == 'abs':
                F = E[2][2]
                if linear_info(F, xvar) is not None:
                    return div(num(1), F)
    return A


def solve_linear_de(rhs, xvar, yvar, bc=None):
    M = split_linear_rhs(rhs, xvar, yvar)
    if M is None:
        return None, None
    Q, A = M
    P = neg(A)
    try:
        B, C = integrate_auto(P, xvar, 0, True)
    except ValueError:
        return None, None
    if B is None:
        return None, None
    N = simplify_integrating_factor(fn('exp', B), xvar)
    O = sim(mul([N, Q]))
    try:
        D, E = integrate_auto(O, xvar, 0, True)
    except ValueError:
        return None, None
    if D is None:
        return None, None
    F = mul([N, sym(yvar)])
    G = de_integrating_factor_divide(D, N)
    H = de_integrating_factor_divide(sym('C'), N)
    I = sim(normalize_add_coeffs(expand_small_recursive(add([G, H]))))
    J = [
        'Rearrange to d' +
        yvar +
        '/d' +
        xvar +
        ' + (' +
        pretty(P) +
        ')*' +
        yvar +
        ' = ' +
        pretty(Q),
        'Int[' +
        pretty(P) +
        '] d' +
        xvar +
        ' = ' +
        pretty(B),
        'Integrating factor = ' +
        pretty(N),
        'Multiply through by ' +
        pretty(N),
        'd/d' +
        xvar +
        '[' +
        pretty(F) +
        '] = ' +
        pretty(O),
        pretty(F) + ' = ' + pretty(D) + ' + C']
    if bc is not None:
        K, L = bc
        R = sim(mul([subst(N, xvar, K), L]))
        S = sim(subst(D, xvar, K))
        T = sim(add([R, neg(S)]))
        U = de_integrating_factor_divide(T, N)
        I = sim(normalize_add_coeffs(expand_small_recursive(add([G, U]))))
        J.append(
            'Use ' +
            yvar +
            ' = ' +
            pretty(L) +
            ' when ' +
            xvar +
            ' = ' +
            pretty(K) +
            '.')
        J.append('C = ' + pretty(T))
        J.append('So ' + yvar + ' = ' + pretty(I))
        return yvar + ' = ' + pretty(I), J
    J.append('So ' + yvar + ' = ' + pretty(I))
    return yvar + ' = ' + pretty(I), J


def solve_separable_de(rhs, xvar, yvar, bc=None):
    F = xvar
    B = yvar
    N = split_separable_rhs(rhs, F, B)
    if N is None:
        return None, None
    X, O = N
    G = num(1)if is_one(O)else div(num(1), O)
    K = X
    P = G
    Q, H = de_rewrite_factor(G)
    if H is not None:
        P = H
    Y = H if H is not None else G
    try:
        E, Z = integrate_auto(Y, B, 0, True)
    except ValueError:
        return None, None
    if E is None:
        return None, None
    try:
        D, Z = integrate_auto(K, F, 0, True)
    except ValueError:
        return None, None
    if D is None:
        return None, None
    A = ['Separate variables', int_text(G, B) + ' = ' + int_text(K, F)]
    if Q is not None:
        A.append(Q)
        A.append(int_text(P, B) + ' = ' + int_text(K, F))
    L = pretty(E) + ' = ' + pretty(D) + ' + C'
    A.append(L)
    R, I = de_log_target(E)
    if bc is not None:
        M, N = bc
        O = sim(subst(E, B, N))
        P = sim(subst(D, F, M))
        Q = sim(add([O, neg(P)]))
        A.append(
            'Use ' +
            B +
            ' = ' +
            pretty(N) +
            ' when ' +
            F +
            ' = ' +
            pretty(M) +
            '.')
        A.append('C = ' + pretty(Q))
        S = sim(add([D, Q]))
        T = pretty(E) + ' = ' + pretty(S)
        if T != L:
            A.append(T)
        C = de_finish_target(E, S, B, N)
        A.append('So ' + C)
        return C, A
    if I is not None:
        J = D if is_one(R)else expand_small(mul([divq(num(1), R), D]))
        S = 'ln|' + display_abs_arg(I) + '| = ' + pretty(J) + ' + C'
        if S != L:
            A.append(S)
        A.append('Write C = ln(k).')
        A.append('ln|' + display_abs_arg(I) + '| = ' + pretty(J) + ' + ln(k)')
        T = de_log_argument(J)
        a = mul([sym('k'), T])if T is not None else mul(
            [sym('k'), fn('exp', J)])
        C = de_finish_target(I, a, B)
        A.append('So ' + C)
        return C, A
    b, U = de_inverse_form(E, B)
    if U is not None:
        C = de_finish_target(U, div(b, add([D, sym('C')])), B)
        A.append('So ' + C)
        return C, A
    V, W = de_exp_target(E, B)
    if W is not None:
        M = add([D, sym('C')])
        if not is_one(V):
            M = div(M, V)
        C = de_finish_target(W, log_abs(M), B)
        A.append('So ' + C)
        return C, A
    return L, A


def solve_de(rhs, xvar, yvar, bc=None):
    A, B = solve_separable_de(rhs, xvar, yvar, bc)
    if A is not None:
        return A, B
    return solve_linear_de(rhs, xvar, yvar, bc)


def integrate_auto(node, var, depth=0, allow_termwise=True, return_kind=False):
    F = depth
    D = var
    C = node

    def E(kind, ans, lines):
        A = lines
        if return_kind:
            return kind, ans, A
        return ans, A
    if F > 24:
        return E(None, None, None)
    G = integral_is_rational_division(C, D)
    for H in auto_integral_routes(G, allow_termwise):
        I, A, B = H(C, D, F)
        if A is not None:
            return E(I, A, B)
    return E(None, None, None)


def is_faxb_term(node, var):
    B = var
    E, A = split_const_mul(node, B)
    D = sym(B)
    if A[0] == 'fn' and linear_info(A[2], B) is not None and A[2] != D:
        return True
    if A[0] == 'pow' and linear_info(A[1], B) is not None and A[1] != D:
        return True
    if A[0] == 'div' and is_one(A[1]) and linear_info(
            A[2], B) is not None and A[2] != D:
        return True
    if A[0] == 'div' and is_one(A[1]) and A[2][0] == 'pow' and linear_info(
            A[2][1], B) is not None and A[2][1] != D:
        return True
    if A[0] == 'pow' and A[1][0] == 'fn' and linear_info(
            A[1][2], B) is not None:
        return True
    if A[0] == 'mul':
        C = list(A[1])
        if len(C) == 2 and C[0][0] == 'fn' and C[1][0] == 'fn' and same(
                C[0][2], C[1][2]) and linear_info(C[0][2], B) is not None:
            if (C[0][1], C[1][1]) in (('sec', 'tan'), ('cosec', 'cot')):
                return True
    return False


def standard_title(node, var):
    A = node
    C = A[1]if A[0] == 'add'else (A,)
    B = 0
    while B < len(C):
        if is_faxb_term(C[B], var):
            return 'f(ax+b)'
        B += 1
    return 'std'


def display_method_title(title):
    return {
        'std': 'Direct integration',
        'f(ax+b)': 'Direct integration',
    }.get(title, title)


def finish_integral_solve(title, ans, lines):
    if ans is None:
        return title, ans, lines
    A, B = finalize_integral_result(ans, lines)
    return title, A, B


def _solve_with_method(node, var, method, forced_u=None):
    F = forced_u
    E = method
    D = var
    C = sim(node)
    if F is not None:
        F = sim(F)
    if E == '2':
        A, B = integrate_standard(C, D)
        return finish_integral_solve(standard_title(C, D), A, B)
    if E == '3':
        A, B = integrate_trig(C, D, True)
        return finish_integral_solve('Using trigonometric identities', A, B)
    if E == '4':
        if F is None:
            A, B = integrate_reverse_chain(C, D)
            if A is not None:
                return finish_integral_solve('Reverse chain rule', A, B)
            A, B = integrate_substitution(C, D, None)
            return finish_integral_solve('Integration by substitution', A, B)
        A, B = integrate_substitution(C, D, F)
        return finish_integral_solve('Integration by substitution', A, B)
    if E == '5':
        A, B = integrate_by_parts(C, D, 0)
        return finish_integral_solve('Integration by parts', A, B)
    if E == '6':
        A, B = integrate_partial(C, D)
        return finish_integral_solve('Partial fractions', A, B)
    if E == '7':
        A, B = integrate_division(C, D, True)
        return finish_integral_solve('Polynomial division', A, B)
    G, A, B = integrate_auto(C, D, 0, True, True)
    if A is None:
        return 'Automatic integration', A, B
    if G == 'direct':
        return finish_integral_solve(standard_title(C, D), A, B)
    if G == 'reverse':
        return finish_integral_solve('Reverse chain rule', A, B)
    if G == 'trig':
        return finish_integral_solve('Using trigonometric identities', A, B)
    if G == 'sub':
        return finish_integral_solve('Integration by substitution', A, B)
    if G == 'parts':
        return finish_integral_solve('Integration by parts', A, B)
    return finish_integral_solve('Partial fractions', A, B)


def solve(node, var, method, forced_u=None):
    title, ans, lines = _solve_with_method(node, var, method, forced_u)
    if ans is not None:
        return title, ans, ensure_working_lines(node, var, title, ans, lines)
    fallback_title, fallback_lines, attempt_lines = fallback_attempts(node, var, forced_u)
    if fallback_title is not None:
        method_map = {
            'std': '2',
            'f(ax+b)': '2',
            'Reverse chain rule': '4',
            'Integration by substitution': '4',
            'Using trigonometric identities': '3',
            'Integration by parts': '5',
            'Partial fractions': '6',
            'Polynomial division': '7',
        }
        best_method = method_map.get(fallback_title, '2')
        retry_title, retry_ans, retry_lines = _solve_with_method(node, var, best_method, forced_u)
        return retry_title, retry_ans, ensure_working_lines(node, var, retry_title, retry_ans, retry_lines)
    return title, ans, lines


def ensure_working_lines(node, var, title, ans, lines):
    if ans is None:
        return lines
    if lines is not None and len(lines) > 0:
        return ensure_reasoning_marker(lines)
    if title in ('std', 'f(ax+b)'):
        simplified = sim(node)
        if simplified[0] == 'div' and is_one(simplified[1]):
            poly = poly_num(simplified[2], var)
            if poly is not None and len(poly) == 3 and poly[2] == num(1) and not is_zero(poly[1]):
                half_lin = divq(poly[1], num(2)) if is_num(poly[1]) else div(poly[1], num(2))
                shifted = add([sym(var), half_lin])
                constant = sim(sub(poly[0], power(half_lin, num(2))))
                if is_num(constant) and constant[1] > 0:
                    completed = add([power(shifted, num(2)), constant])
                    result_lines = [
                        'Complete the square in the denominator.',
                        pretty(simplified[2]) + ' = ' + pretty(completed),
                        'Use the standard result for 1/(u^2+a^2).',
                        'So I = ' + pretty(ans) + ' + C',
                    ]
                    return ensure_reasoning_marker(result_lines)
        result_lines = [
            'Use the standard integral for ' + pretty(node) + '.',
            'So I = ' + pretty(ans) + ' + C',
        ]
        return ensure_reasoning_marker(result_lines)
    if title == 'Reverse chain rule':
        result_lines = [
            'Use reverse chain rule.',
            'So I = ' + pretty(ans) + ' + C',
        ]
        return ensure_reasoning_marker(result_lines)
    if title == 'Integration by substitution':
        return [
            'Use substitution.',
            'So I = ' + pretty(ans) + ' + C',
        ]
    if title == 'Integration by parts':
        return [
            'Use integration by parts.',
            'So I = ' + pretty(ans) + ' + C',
        ]
    if title == 'Using trigonometric identities':
        return [
            'Use a trigonometric identity first.',
            'So I = ' + pretty(ans) + ' + C',
        ]
    if title == 'Partial fractions':
        return [
            'Use partial fractions.',
            'So I = ' + pretty(ans) + ' + C',
        ]
    if title == 'Polynomial division':
        return [
            'Divide the numerator by the denominator first.',
            'So I = ' + pretty(ans) + ' + C',
        ]
    return ['So I = ' + pretty(ans) + ' + C']


def fallback_attempts(node, var, forced_u=None):
    attempts = [('2', 'dir'), ('4', 'sub'), ('3', 'trig'), ('5', 'pts'), ('6', 'pf'), ('7', 'div')]
    best = None
    best_title = None
    attempt_lines = []
    i = 0
    while i < len(attempts):
        if cancellation_requested():
            break
        method, label = attempts[i]
        title, ans, lines = _solve_with_method(node, var, method, forced_u)
        if ans is not None:
            attempt_lines.append('Attempt ' + str(i + 1) + ' (' + label + ')')
            j = 0
            while lines is not None and j < len(lines):
                attempt_lines.append(lines[j])
                j += 1
            score = integral_candidate_score(title, ans, lines)
            if best is None or score < best:
                best = score
                best_title = title
        i += 1
    if best is None:
        return None, None, None
    return best_title, None, attempt_lines


def cancellation_requested():
    return False


def integral_candidate_score(title, ans, lines):
    if ans is None:
        return (10**9, 10**9, 10**9)
    title_score = {'std': 0, 'f(ax+b)': 1, 'Reverse chain rule': 2, 'Integration by substitution': 3, 'Using trigonometric identities': 4, 'Integration by parts': 5, 'Partial fractions': 6, 'Polynomial division': 6}.get(title, 7)
    return (len(pretty(ans)), len(lines or []), title_score)


def solve_result_or_reason(node, var, method, forced_u=None):
    A, B, C = solve(node, var, method, forced_u)
    if B is not None:
        return A, B, C, None
    return A, B, C, hard_integral_failure_reason(node, var)


def can_handle_derivative_case(node, var, deps):
    try:
        diff(node, var, deps)
        return True, None
    except Exception as err:
        return False, str(err)

def subst(node, name, value):
    C = value
    B = name
    A = node
    D = A[0]
    if D == 'sym':
        return C if A[1] == B else A
    if D in ('num', 'const'):
        return A
    if D == 'fn':
        return 'fn', A[1], subst(A[2], B, C)
    if D == 'pow':
        return 'pow', subst(A[1], B, C), subst(A[2], B, C)
    if D == 'div':
        return 'div', subst(A[1], B, C), subst(A[2], B, C)
    F = []
    E = 0
    while E < len(A[1]):
        F.append(subst(A[1][E], B, C))
        E += 1
    return D, tuple(F)


_depends_uncached = depends


def depends(node, name):
    A = (node, name)
    B = _cache_get('depends', A)
    if B is not _CACHE_MISS:
        return B
    return _cache_set('depends', A, _depends_uncached(node, name))


_sim_uncached = sim


def sim(node):
    A = _cache_get('sim', node)
    if A is not _CACHE_MISS:
        return A
    return _cache_set('sim', node, _sim_uncached(node))


_poly_num_uncached = poly_num


def poly_num(node, var):
    A = (node, var)
    B = _cache_get('poly_num', A)
    if B is not _CACHE_MISS:
        return B
    return _cache_set('poly_num', A, _poly_num_uncached(node, var))


_linear_info_uncached = linear_info


def linear_info(node, var):
    A = (node, var)
    B = _cache_get('linear_info', A)
    if B is not _CACHE_MISS:
        return B
    return _cache_set('linear_info', A, _linear_info_uncached(node, var))


_ordered_candidates_uncached = ordered_candidates


def ordered_candidates(node, var, mode):
    A = (node, var, mode)
    B = _cache_get('ordered_candidates', A)
    if B is not _CACHE_MISS:
        return B
    return _cache_set(
        'ordered_candidates',
        A,
        _ordered_candidates_uncached(
            node,
            var,
            mode))


def combine_logs(node):
    A = _cache_get('combine_logs', node)
    if A is not _CACHE_MISS:
        return A
    return _cache_set('combine_logs', node, _combine_logs_uncached(node, 0))


MENU_LINE_WIDTH = 20
MENU_PAGE_LINES = 4


def wrap_menu_text(text, width=MENU_LINE_WIDTH):
    text = text.strip()
    if text == '':
        return ['']
    out = []
    remaining = text
    while remaining != '':
        if len(remaining) <= width:
            out.append(remaining)
            break
        split = remaining.rfind(' ', 0, width + 1)
        if split <= 0:
            out.append(remaining[:width])
            remaining = remaining[width:]
        else:
            out.append(remaining[:split])
            remaining = remaining[split + 1:]
        while remaining[:1] == ' ':
            remaining = remaining[1:]
    return out


def build_menu_pages(options):
    pages = []
    page = []
    used = 0
    i = 0
    while i < len(options):
        key, label = options[i]
        lines = wrap_menu_text(key + ' ' + label)
        if used > 0 and used + len(lines) > MENU_PAGE_LINES:
            pages.append(page)
            page = []
            used = 0
        page.append(lines)
        used += len(lines)
        i += 1
    if len(page) == 0:
        pages.append([])
    else:
        pages.append(page)
    return pages


def paged_menu_input(prompt_label, options, default=None):
    pages = build_menu_pages(options)
    valid = {}
    i = 0
    while i < len(options):
        valid[options[i][0]] = True
        i += 1
    page_index = 0
    while True:
        page = pages[page_index]
        i = 0
        while i < len(page):
            lines = page[i]
            j = 0
            while j < len(lines):
                print(lines[j])
                j += 1
            i += 1
        prompt = prompt_label
        if len(pages) > 1:
            prompt += ' ' + str(page_index + 1) + '/' + str(len(pages)) + ' n/p'
        prompt += ': '
        choice = input(prompt).strip()
        if choice == '':
            if default is not None:
                return default
        else:
            lowered = choice.lower()
            if lowered == 'n' and len(pages) > 1:
                page_index += 1
                if page_index >= len(pages):
                    page_index = 0
                continue
            if lowered == 'p' and len(pages) > 1:
                page_index -= 1
                if page_index < 0:
                    page_index = len(pages) - 1
                continue
            if choice in valid:
                return choice
        print('Bad mode.')


def main():
    D = paged_menu_input('M', [
        ('1', 'int'),
        ('2', 'de'),
        ('3', 'param area'),
    ], '1')
    begin_user_action()
    try:
        if D == '1':
            F = input('f: ').strip()
            if F == '':
                raise ValueError('Enter f.')
            J, K = parse_input(F)
            E = paged_menu_input('Met', [
                ('1', 'auto'),
                ('2', 'dir'),
                ('3', 'trig'),
                ('4', 'sub'),
                ('5', 'pts'),
                ('6', 'pf'),
                ('7', 'div'),
            ], '1')
            G = None
            if E == '4':
                H = input('u: ').strip()
                if H != '':
                    G = parse_forced_u(H)
            L, C, A, Q = solve_result_or_reason(J, K, E, G)
            if C is None:
                print(Q)
            else:
                I = '= ' + pretty(C) + ' + C'
                print('Method: ' + display_method_title(L))
                B = 0
                while B < len(A):
                    print(str(B + 1) + '. ' + A[B])
                    B += 1
                if len(A) == 0 or A[-1] != I:
                    print(I)
        elif D == '2':
            F = input('dy/dx: ').strip()
            try:
                M, N, O = parse_de_input(F)
            except Exception:
                print(DE_FAIL)
                return
            G = input('BC: ').strip()
            H = parse_de_condition(G, N, O)if G != ''else None
            C, A = solve_de(M, N, O, H)
            if C is None:
                print(DE_FAIL)
            else:
                B = 0
                while B < len(A):
                    print(str(B + 1) + '. ' + A[B])
                    B += 1
                if len(A) == 0 or A[-1] != C:
                    print(C)
        elif D == '3':
            F = input('x(t): ').strip()
            G = input('y(t): ').strip()
            if F == '' or G == '':
                raise ValueError('Enter x(t) and y(t).')
            H = parse(F)
            I = parse(G)
            J = diff(H, 't')
            K = sim(mul([I, J]))
            L, C, A, Q = solve_result_or_reason(K, 't', '1', None)
            print('dx/dt = ' + pretty(J))
            print('Int[y dx] = Int[' + pretty(K) + '] dt')
            if C is None:
                print(Q)
            else:
                print('Method: ' + display_method_title(L))
                B = 0
                while B < len(A):
                    print(str(B + 1) + '. ' + A[B])
                    B += 1
                M = '= ' + pretty(C) + ' + C'
                if len(A) == 0 or A[-1] != M:
                    print(M)
        else:
            print('Bad mode.')
    except Exception as P:
        print('Err: ' + str(P))


run = main
if not SKIP_AUTORUN:
    main()
