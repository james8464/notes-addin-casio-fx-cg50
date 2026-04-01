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
FAIL = 'This integral is outside the supported A-level method set.'
SKIP_AUTORUN = sys is not None and getattr(sys, '_int_no_autorun', False)
MICROPYTHON_RUNTIME = sys is not None and getattr(
    getattr(sys, 'implementation', None), 'name', '') == 'micropython'
LOW_MEMORY_RUNTIME = False
EXPAND_PASS_LIMIT = 4
TRIG_REWRITE_LIMIT = 4
E = 'const', 'e'
PI = 'const', 'pi'
U = 'sym', 'u'
FUNC_NAMES = 'sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'exp', 'log', 'log10', 'sqrt', 'abs', 'ln', 'atan'


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
    return None


def clear_engine_caches():
    return None


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
                                    'sqrt_power',)}
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
                                            'inv_one_plus_square',)}
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
    ('sec',
     4): (
        'Use sec^2 x = 1+tan^2 x.',
        'sec4'),
    ('cosec',
     4): (
        'Use cosec^2 x = 1+cot^2 x.',
        'cosec4')}
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
    elif D == 'sec4':
        B = mul([power(fn('sec', A), num(2)), add(
            [num(1), power(fn('tan', A), num(2))])])
    else:
        B = mul([power(fn('cosec', A), num(2)), add(
            [num(1), power(fn('cot', A), num(2))])])
    return C[0], B


def trig_product_rewrite(left_name, right_name, arg):
    A = TRIG_PRODUCT_IDENTITIES.get((left_name, right_name))
    if A is None:
        return None, None
    return A, div(fn('sin', mul([num(2), arg])), num(2))


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
        if S == 'log' and N[0] == 'fn' and N[1] == 'abs' and (
                N[2][0] == 'pow' and same(N[2][1], E) or N[2][0] == 'fn' and N[2][1] == 'exp'):
            return N[2][2]
        A = special_trig_value(S, N)
        if A is not None:
            return A
        if S == 'exp':
            return 'fn', 'exp', N
        return 'fn', S, N
    if R == 'pow':
        D = sim(L[1])
        B = sim(L[2])
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
                B += '-' + _show(display_abs(F[C]), pr(A))
            else:
                B += '+' + _show(F[C], pr(A))
            C += 1
    if pr(A) < P:
        return '(' + B + ')'
    return B


def show(node, parent=0): return _show(display_rearrange(sim(node)), parent)
def pretty(node): return _show(display_rearrange(combine_logs(node)), 0)
def int_text(node, var): return 'Int[' + pretty(node) + '] d' + var


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
        'Let I = ' +
        int_text(
            node,
            B),
        'Let u = ' +
        pretty(C),
        'du/d' +
        B +
        ' = ' +
        pretty(d),
        du_equals_text(
            d,
            B)]
    E = solve_for_u_back(C, B)
    if E is not None:
        A.append(E)
    A.append('So I = ' + int_text(D, 'u'))
    if not same(work, D):
        A.append('= ' + int_text(work, 'u'))
    A.append('= ' + pretty(uans) + ' + C')
    A.append('= ' + pretty(ans) + ' + C')
    return A


def combine_logs(node):
    A = node
    A = sim(A)
    B = A[0]
    if B in ('num', 'sym', 'const'):
        return A
    if B == 'fn':
        return 'fn', A[1], combine_logs(A[2])
    if B in ('pow', 'div'):
        return sim((B, combine_logs(A[1]), combine_logs(A[2])))
    if B == 'mul':
        G = []
        for F in flat(A, 'mul'):
            G.append(combine_logs(F))
        return sim(('mul', tuple(G)))
    C = []
    for F in flat(A, 'add'):
        C.append(combine_logs(F))
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
        S = diff(N, B)
        T = A[1]
        O = derivative_of_named_function(T, N, S)
        if O is not None:
            return O
    raise ValueError(FAIL)


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
            if B == 'pi':
                return PI
            if B == 'e':
                return E
            if F() == '(' and B in FUNC_NAMES:
                D('(')
                C = L()
                D(')')
                return fn(B, C)
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


def split_input_var(text):
    A = text
    A = A.strip()
    if not (A[:1] == '(' and A[-1:] == ')'):
        return
    E = 0
    B = -1
    C = 0
    while C < len(A):
        F = A[C]
        if F == '(':
            E += 1
        elif F == ')':
            E -= 1
        elif F == ',' and E == 1:
            B = C
        C += 1
    if B == -1:
        return
    H = A[1:B].strip()
    D = A[B + 1:-1].strip()
    if H == '' or D == '':
        return
    G = 0
    while G < len(D):
        if not is_name_char(D[G]):
            return
        G += 1
    return H, D


def parse_input(text):
    A = split_input_var(text)
    if A is None:
        return parse(text), 'x'
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


def parse_de_input(text):
    A = text
    A = A.strip()
    if '=' in A:
        return parse_de_equation(A)
    return parse(A), 'x', 'y'


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
    E = fn('sqrt', B)
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
    G = quad_scale(A[2], D)
    if G is None:
        return None, None
    J, K, M = F
    H = B[1]if len(B) > 1 else num(0)
    L = B[0]
    E = divq(H, mulq(num(2), J))if not is_zero(H)else num(0)
    I = subq(L, mulq(E, K))
    C = []
    if not is_zero(E):
        C.append(mul([E, fn('log', fn('abs', A[2]))]))
    if not is_zero(I):
        C.append(mul([I, G]))
    if len(C) == 0:
        return num(0), []
    return add(C), []


def split_linear_quadratic_product(node, var):
    B = var
    A = flat(node, 'mul')
    if len(A) != 2:
        return
    C = linear_info(A[0], B) is not None
    D = linear_info(A[1], B) is not None
    E = quad_scale(A[0], B) is not None
    F = quad_scale(A[1], B) is not None
    if C and F:
        return A[0], A[1]
    if D and E:
        return A[1], A[0]


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


def _rewrite_in_u(node, inner, var):
    C = var
    B = inner
    A = node
    if cheap_same(A, B) or same(A, B):
        return U
    if B[0] == 'add':
        D = flat(B, 'add')
        if len(D) == 2:
            if not depends(D[0], C) and cheap_same(A, D[1]):
                return add([U, neg(D[0])])
            if not depends(D[1], C) and cheap_same(A, D[0]):
                return add([U, neg(D[1])])
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
    S, U = integrate_quadratic_rational(A, C)
    if S is not None:
        return S, U
    
    # Add pattern for 1/(x^2 + a^2) = (1/a) * arctan(x/a) + C  
    if A[0] == 'div' and is_one(A[1]) and A[2][0] == 'add':
        terms = flat(A[2], 'add')
        if len(terms) == 2:
            # Look for x^2 and positive constant terms
            x_squared_term = None
            constant_term = None
            for term in terms:
                if term[0] == 'pow' and term[1] == sym(C) and same(term[2], num(2)):
                    x_squared_term = term
                elif is_num(term) and term[1] > 0:
                    constant_term = term
            
            # If we found the pattern, apply arctan formula
            if x_squared_term is not None and constant_term is not None:
                a = fn('sqrt', constant_term)
                result = div(fn('atan', div(sym(C), a)), a)
                return result, ['Use the standard result: 1/(' + C + '^2 + ' + pretty(constant_term) + ') -> (1/' + pretty(a) + ')*arctan(' + C + '/' + pretty(a) + ')',
                               'So I = ' + pretty(result) + ' + C']
    
    # Add pattern for f'(x)/f(x) = ln|f(x)| + C (general logarithmic integration)
    if A[0] == 'div':
        numerator = A[1]
        denominator = A[2]
        # Check if numerator is derivative of denominator
        deriv_denom = diff(denominator, C)
        if same(deriv_denom, numerator) or (is_num(numerator) and is_num(deriv_denom) and 
                                            divq(numerator, deriv_denom) and is_one(divq(numerator, deriv_denom))):
            result = fn('log', fn('abs', denominator))
            return result, ['Use the standard result for f\'(x)/f(x) -> ln|f(x)|.',
                           'So I = ' + pretty(result) + ' + C']
        # Special case: k*f'(x)/f(x) = k*ln|f(x)| + C  
        coeff, base_numer = split_coeff(numerator)
        if not is_one(coeff):
            deriv_check = diff(denominator, C)
            if same(deriv_check, base_numer) or (is_num(base_numer) and is_num(deriv_check) and 
                                                divq(base_numer, deriv_check) and is_one(divq(base_numer, deriv_check))):
                result = mul([coeff, fn('log', fn('abs', denominator))])
                return result, ['Use the standard result for k*f\'(x)/f(x) -> k*ln|f(x)|.',
                               'So I = ' + pretty(result) + ' + C']
    
    # Add pattern for 1/sqrt(a^2 - x^2) = arcsin(x/a) + C
    if A[0] == 'div' and is_one(A[1]) and (
            A[2][0] == 'fn' and A[2][1] == 'sqrt' or A[2][0] == 'pow' and same(A[2][2], num(1, 2))):
        J = A[2][2]if A[2][0] == 'fn'else A[2][1]
        H = poly_num(J, C)
        if H is not None and len(H) == 3 and is_zero(H[1]) and H[2] == num(1):
            B = fn('log', fn('abs', add([F, power(J, num(1, 2))])))
            return B, [
                'Use the standard result for 1/sqrt(' + C + '^2+a).', 'So I = ' + pretty(B) + ' + C']
        if same(J, add([num(1), neg(power(F, num(2)))])) or same(
                J, add([neg(power(F, num(2))), num(1)])):
            return fn('asin', F), [
                'Use the standard result for 1/sqrt(1-' + C + '^2).', 'So I = asin(' + C + ') + C']
        if H is not None and len(H) == 3 and is_zero(H[1]) and is_num(
                H[0]) and is_num(H[2]) and H[0][1] > 0 and H[2][1] < 0:
            V = fn('sqrt', H[0])
            T = fn('sqrt', negq(H[2]))
            W = div(mul([T, F]), V)
            D = div(fn('asin', W), T)
            return D, [
                'Use the standard result for 1/sqrt(a^2-b^2*' + C + '^2).', 'So I = ' + pretty(D) + ' + C']
        # Additional check for a^2 - x^2 form
        if H is not None and len(H) == 3 and is_zero(H[1]) and is_num(H[2]) and is_num(H[0]):
            # Check if we have form a^2 - x^2
            if H[2][1] < 0 and H[0][1] > 0:  # a^2 - x^2 where a^2 > 0
                a_squared = H[0]
                a = fn('sqrt', a_squared)
                result = fn('asin', div(sym(C), a))
                return result, ['Use the standard result for 1/sqrt(' + pretty(a_squared) + '-' + C + '^2).',
                               'So I = ' + pretty(result) + ' + C']
        G, M = linear_info(A[2][1], C)
        N = neg(A[2][2])
        if N == num(-1):
            B = fn('log', fn('abs', A[2][1]))
            D = div(B, G)
        else:
            I = addq(N, num(1))
            B = power(A[2][1], I)
            D = div(B, mul([G, I]))
        return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(diff(B, C)if not (
            B[0] == 'fn' and B[1] == 'log')else div(G, A[2][1])), 'So I = ' + pretty(D) + ' + C']
    if A[0] == 'div' and is_one(A[1]) and (
            A[2][0] == 'fn' and A[2][1] == 'sqrt' or A[2][0] == 'pow' and same(A[2][2], num(1, 2))):
        J = A[2][2]if A[2][0] == 'fn'else A[2][1]
        H = poly_num(J, C)
        if H is not None and len(H) == 3 and is_zero(H[1]) and H[2] == num(1):
            B = fn('log', fn('abs', add([F, power(J, num(1, 2))])))
            return B, [
                'Use the standard result for 1/sqrt(' + C + '^2+a).', 'So I = ' + pretty(B) + ' + C']
        if same(J, add([num(1), neg(power(F, num(2)))])) or same(
                J, add([neg(power(F, num(2))), num(1)])):
            return fn('asin', F), [
                'Use the standard result for 1/sqrt(1-' + C + '^2).', 'So I = asin(' + C + ') + C']
        if H is not None and len(H) == 3 and is_zero(H[1]) and is_num(
                H[0]) and is_num(H[2]) and H[0][1] > 0 and H[2][1] < 0:
            V = fn('sqrt', H[0])
            T = fn('sqrt', negq(H[2]))
            W = div(mul([T, F]), V)
            D = div(fn('asin', W), T)
            return D, [
                'Use the standard result for 1/sqrt(a^2-b^2*' + C + '^2).', 'So I = ' + pretty(D) + ' + C']
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
            if O in ('tan', 'cot', 'sec', 'cosec'):
                return D, []
            return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(
                diff(B, C)), 'So I = ' + pretty(D) + ' + C']
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
    if A[0] == 'pow' and A[1][0] == 'fn' and same(A[2], num(2)):
        O = A[1][1]
        J = A[1][2]
        K = linear_info(J, C)
        if K is not None:
            G, M = K
            B = primitive_of_named_power(O, J, A[2])
            if B is not None:
                D = div(B, G)
                return D, ['Consider y = ' + pretty(B), 'dy/d' + C + ' = ' + pretty(
                    diff(B, C)), 'So I = ' + pretty(D) + ' + C']
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


FN_CANDIDATE_NAMES = 'sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'sqrt', 'exp'


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
    I = ordered_candidates(D, B, 'reverse')
    E = 0
    while E < len(I):
        C = I[E]
        F = diff(C, B)
        if not is_zero(F):
            J = quotient_by_target(D, F)
            if J is not None:
                G = rewrite_in_u(J, C, B)
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
        E = diff(D, C)
        if not is_zero(E):
            F = quotient_by_target(B, E)
            if F is None:
                F = div(B, E)
            if F is not None:
                A = rewrite_in_u(F, D, C)
                if A is not None:
                    A = cancel_u_radical(A)
                    K = expand_small(A)
                    L = K if not same(K, A)else A
                    I = solve_u_expr(L)
                    if I is not None:
                        M = subst(I, 'u', D)
                        return M, substitution_work(B, C, D, E, A, L, I, M)
        H += 1
    return None, None


def trig_rewrite_step(node):
    B = node
    I = expand_square(B)
    if I is not None:
        return 'Expand the brackets.', I
    if B[0] == 'div' and B[1][0] == 'add' and B[2][0] == 'pow' and same(
            B[2][2], num(2)) and B[2][1][0] == 'fn':
        D = flat(B[1], 'add')
        H = B[2][1]
        C = H[2]
        if len(D) == 2 and same(H, fn('cos', C)):
            if same(D[0], num(1)) and same(D[1], neg(fn('sin', C))) or same(
                    D[1], num(1)) and same(D[0], neg(fn('sin', C))):
                return 'Rewrite in sec and tan.', add(
                    [power(fn('sec', C), num(2)), neg(mul([fn('sec', C), fn('tan', C)]))])
        if len(D) == 2 and same(H, fn('sin', C)):
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
    if B[0] == 'mul':
        A = list(B[1])
        if len(A) == 2 and A[0][0] == 'fn' and A[1][0] == 'fn' and A[0][1] == 'sin' and A[1][1] == 'cos' and same(
                A[0][2], A[1][2]):
            return trig_product_rewrite(A[0][1], A[1][1], A[0][2])
        if len(A) == 2 and A[0][0] == 'fn' and A[1][0] == 'fn' and A[0][1] == 'cos' and A[1][1] == 'sin' and same(
                A[0][2], A[1][2]):
            return trig_product_rewrite(A[0][1], A[1][1], A[0][2])
        G = 0
        while G < len(A):
            F, E = trig_rewrite_step(A[G])
            if E is not None:
                return F, expand_small(mul(A[:G] + [E] + A[G + 1:]))
            G += 1
    return None, None


def integrate_trig(node, var, allow_steps=True):
    E = allow_steps
    C = var
    B = node
    D = []
    F = False
    G = 0
    while G < TRIG_REWRITE_LIMIT:
        J, H = trig_rewrite_step(B)
        if H is None:
            break
        B = H
        F = True
        if E:
            D.append(J)
            D.append('So I = ' + int_text(B, C))
        G += 1
    if not F:
        return None, None
    A, I = integrate_after_trig_rewrite(B, C, 0)
    if A is None:
        A, I = integrate_division(B, C, True)
        if A is None:
            A, I = integrate_partial(B, C)
    if A is None:
        return None, None
    if E:
        D.append('= ' + pretty(A) + ' + C')
        return A, D
    return A, []


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
    if depth > 4:
        return None, None
    A, D = integrate_standard(B, C)
    if A is not None:
        return A, []
    A, D = integrate_reverse_chain(B, C)
    if A is not None:
        return A, []
    return integrate_trig(B, C, False)


def integrate_u_subproblem(node, var='u', depth=0):
    D = depth
    C = var
    B = node
    if D > 4:
        return None, None
    A, E = integrate_termwise_with(B, C, integrate_u_subproblem, D)
    if A is not None:
        return A, []
    A, E = integrate_standard(B, C)
    if A is not None:
        return A, []
    return integrate_trig(B, C, False)


def integrate_after_trig_rewrite(node, var, depth=0):
    E = depth
    C = var
    B = node
    if E > 4:
        return None, None
    A, D = integrate_termwise_with(B, C, integrate_after_trig_rewrite, E)
    if A is not None:
        return A, []
    A, D = integrate_standard(B, C)
    if A is not None:
        return A, []
    A, D = integrate_reverse_chain(B, C)
    if A is not None:
        return A, []
    A, D = integrate_trig(B, C, False)
    if A is not None:
        return A, []
    A, D = integrate_substitution(B, C, None)
    if A is not None:
        return A, []
    return None, None


def choose_parts(node, var):
    F = var
    E = node
    J = sym(F)
    if E[0] == 'fn' and E[1] == 'log' and same(E[2], J):
        return E, num(1), J
    N, C = split_const_mul(E, F)
    if C[0] == 'div' and C[1][0] == 'fn' and C[1][1] == 'log' and same(
            C[1][2], J):
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
        if B[A][0] == 'fn' and B[A][1] == 'log' and same(B[A][2], J):
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
    if H > 4:
        return None, None
    I = choose_parts(node, A)
    if I is None:
        return None, None
    C, M, D = I
    J = diff(C, A)
    E = mul([D, J])
    K, F = integrate_auto(E, A, H + 1, True)
    if K is None:
        return None, None
    L = add([mul([C, D]), neg(K)])
    B = [
        'Let u = ' + pretty(C),
        'du/d' + A + ' = ' + pretty(J),
        'dv/d' + A + ' = ' + pretty(M),
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


def integrate_partial(node, var):
    E = var
    D = node
    if D[0] != 'div':
        return None, None
    J = poly_num(D[1], E)
    F = poly_num(D[2], E)
    if J is None or F is None:
        return None, None
    if len(J) >= len(F):
        return None, None
    C = pf_factor_list(D[2], E)
    if C is None:
        return integrate_partial_linear_quadratic(D, E)
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
            return None, None
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


def de_log_target(node):
    A = node
    if A[0] == 'fn' and A[1] == 'log' and A[2][0] == 'fn' and A[2][1] == 'abs':
        return num(1), A[2][2]
    C, B = split_coeff(A)
    if is_num(
            C) and B[0] == 'fn' and B[1] == 'log' and B[2][0] == 'fn' and B[2][1] == 'abs':
        return C, B[2][2]
    return None, None


def de_log_argument(node):
    A = node
    if A[0] == 'fn' and A[1] == 'log' and A[2][0] == 'fn' and A[2][1] == 'abs':
        return A[2][2]
    D, B = split_coeff(A)
    if is_num(
            D) and B[0] == 'fn' and B[1] == 'log' and B[2][0] == 'fn' and B[2][1] == 'abs':
        return power(B[2][2], D)
    if A[0] == 'add':
        E = []
        C = 0
        F = flat(A, 'add')
        while C < len(F):
            G = de_log_argument(F[C])
            if G is None:
                return
            E.append(G)
            C += 1
        return make_mul(E)


def de_absorb_k_sign(node):
    B = node
    if not depends(B, 'k'):
        return B
    A, C = split_coeff(B)
    if is_num(A) and A[1] < 0:
        return C if is_minus_one(A)else mul([num(-A[1], A[2]), C])
    return B


def de_solve_target(target, rhs, yvar):
    E = target
    A = rhs
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
    C = de_solve_target(A, B, yvar)
    if not (A[0] == 'fn' and A[2] == sym(yvar)):
        D, E = de_quadratic_target(A, yvar)
        if D is not None:
            F = div(add([B, neg(E)]), D)
            if y0 is None:
                return pretty(power(sym(yvar), num(2))) + ' = ' + pretty(F)
            return pretty(sym(yvar)) + ' = ' + pretty(neg(fn('sqrt', F))
                                                      if split_coeff(y0)[0][1] < 0 else fn('sqrt', F))
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


def solve_de(rhs, xvar, yvar, bc=None):
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
    E, Z = integrate_auto(Y, B, 0, True)
    if E is None:
        return None, None
    D, Z = integrate_auto(K, F, 0, True)
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


def integrate_auto(node, var, depth=0, allow_termwise=True, return_kind=False):
    F = depth
    D = var
    C = node

    def E(kind, ans, lines):
        A = lines
        if return_kind:
            return kind, ans, A
        return ans, A
    if F > 4:
        return E(None, None, None)
    if allow_termwise:
        A, H = integrate_termwise_with(
            C, D, lambda n, v, d: integrate_auto(
                n, v, d, False), F)
        if A is not None:
            return E('direct', A, [
                     'Integrate each term separately.', '= ' + pretty(A) + ' + C'])
    A, B = integrate_standard(C, D)
    if A is not None:
        return E('direct', A, B)
    A, B = integrate_reverse_chain(C, D)
    if A is not None:
        return E('reverse', A, B)
    A, B = integrate_trig(C, D, True)
    if A is not None:
        return E('trig', A, B)
    G = C[0] == 'div' and poly_num(
        C[1], D) is not None and poly_num(
        C[2], D) is not None
    if G:
        A, B = integrate_division(C, D, True)
        if A is not None:
            return E('pf', A, B)
        A, B = integrate_partial(C, D)
        if A is not None:
            return E('pf', A, B)
    A, B = integrate_substitution(C, D, None)
    if A is not None:
        return E('sub', A, B)
    A, B = integrate_by_parts(C, D, F)
    if A is not None:
        return E('parts', A, B)
    if not G:
        A, B = integrate_division(C, D, True)
        if A is not None:
            return E('pf', A, B)
        A, B = integrate_partial(C, D)
        return E('pf', A, B)
    return E('pf', None, None)


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
            return 'Integrating f(ax+b)'
        B += 1
    return 'Integrating standard functions'


def solve(node, var, method, forced_u=None):
    F = forced_u
    E = method
    D = var
    C = node
    C = sim(C)
    if F is not None:
        F = sim(F)
    if E == '2':
        A, B = integrate_standard(C, D)
        return standard_title(C, D), A, B
    if E == '3':
        A, B = integrate_trig(C, D, True)
        return 'Using trigonometric identities', A, B
    if E == '4':
        if F is None:
            A, B = integrate_reverse_chain(C, D)
            if A is not None:
                return 'Reverse chain rule', A, B
            A, B = integrate_substitution(C, D, None)
            return 'Integration by substitution', A, B
        A, B = integrate_substitution(C, D, F)
        return 'Integration by substitution', A, B
    if E == '5':
        A, B = integrate_by_parts(C, D, 0)
        return 'Integration by parts', A, B
    if E == '6':
        A, B = integrate_partial(C, D)
        return 'Partial fractions', A, B
    if E == '7':
        A, B = integrate_division(C, D, True)
        return 'Partial fractions', A, B
    G, A, B = integrate_auto(C, D, 0, True, True)
    if A is None:
        return 'Partial fractions', A, B
    if G == 'direct':
        return standard_title(C, D), A, B
    if G == 'reverse':
        return 'Reverse chain rule', A, B
    if G == 'trig':
        return 'Using trigonometric identities', A, B
    if G == 'sub':
        return 'Integration by substitution', A, B
    if G == 'parts':
        return 'Integration by parts', A, B
    return 'Partial fractions', A, B


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


def main():
    print('1 integral')
    print('2 de')
    D = input('Mode: ').strip()
    if D == '':
        D = '1'
    begin_user_action()
    try:
        if D == '1':
            F = input('f = ').strip()
            J, K = parse_input(F)
            print('1 auto')
            print('2 direct')
            print('3 trig')
            print('4 sub')
            print('5 parts')
            print('6 pf')
            print('7 divide')
            E = input('Method: ').strip()
            if E == '':
                E = '1'
            G = None
            if E == '4':
                H = input('u = ').strip()
                if H != '':
                    G = parse_forced_u(H)
            L, C, A = solve(J, K, E, G)
            if C is None:
                print(FAIL)
            else:
                I = '= ' + pretty(C) + ' + C'
                print('Method: ' + L)
                B = 0
                while B < len(A):
                    print(str(B + 1) + '. ' + A[B])
                    B += 1
                if len(A) == 0 or A[-1] != I:
                    print(I)
        elif D == '2':
            F = input('dy/dx = ').strip()
            M, N, O = parse_de_input(F)
            G = input('BC: ').strip()
            H = parse_de_condition(G, N, O)if G != ''else None
            C, A = solve_de(M, N, O, H)
            if C is None:
                print(FAIL)
            else:
                B = 0
                while B < len(A):
                    print(str(B + 1) + '. ' + A[B])
                    B += 1
                if len(A) == 0 or A[-1] != C:
                    print(C)
        else:
            print('Mode must be 1 or 2.')
    except Exception as P:
        print('Input error: ' + str(P))


run = main
if not SKIP_AUTORUN:
    main()
