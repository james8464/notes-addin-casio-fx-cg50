try:
    import math
except ImportError:
    math = None

try:
    import sys
except ImportError:
    sys = None

# ============================================================================
# Core Constants and Configuration
# ============================================================================

FAST_GCD = math.gcd if math is not None and hasattr(math, 'gcd') else None
FAST_ISQRT = math.isqrt if math is not None and hasattr(math, 'isqrt') else None
SKIP_AUTORUN = sys is not None and getattr(sys, '_algebra_no_autorun', False)
MICROPYTHON_RUNTIME = sys is not None and getattr(getattr(sys, 'implementation', None), 'name', '') == 'micropython'

# Cache size limits - adjust based on runtime
DESKTOP_CACHE_LIMIT_SMALL = 1024
DESKTOP_CACHE_LIMIT_MEDIUM = 2048
DESKTOP_CACHE_LIMIT_LARGE = 4096
LOWMEM_CACHE_LIMIT_SMALL = 128
LOWMEM_CACHE_LIMIT_MEDIUM = 256
LOWMEM_CACHE_LIMIT_LARGE = 512

CACHE_LIMIT_SMALL = DESKTOP_CACHE_LIMIT_SMALL
CACHE_LIMIT_MEDIUM = DESKTOP_CACHE_LIMIT_MEDIUM
CACHE_LIMIT_LARGE = DESKTOP_CACHE_LIMIT_LARGE

LOW_MEMORY_RUNTIME = False

# ============================================================================
# Cache Dictionaries for Performance
# ============================================================================
SIG_CACHE = {}
SIM_CACHE = {}
SHOW_CACHE = {}
SPLIT_COEFF_CACHE = {}
FLAT_CACHE = {}
SAME_CACHE = {}
EQUIV_CACHE = {}

ALL_CACHES = (
    SIG_CACHE, SIM_CACHE, SHOW_CACHE, SPLIT_COEFF_CACHE,
    FLAT_CACHE, SAME_CACHE, EQUIV_CACHE
)


def clear_all_caches():
    """Clear all caches to free memory."""
    i = 0
    while i < len(ALL_CACHES):
        ALL_CACHES[i].clear()
        i += 1


def cache_store(cache, key, value, limit):
    """Store value in cache, clearing if limit reached."""
    if len(cache) >= limit:
        cache.clear()
    cache[key] = value
    return value


def apply_runtime_profile(low_memory=None):
    global LOW_MEMORY_RUNTIME, CACHE_LIMIT_SMALL, CACHE_LIMIT_MEDIUM, CACHE_LIMIT_LARGE
    if low_memory is None:
        low_memory = MICROPYTHON_RUNTIME
    LOW_MEMORY_RUNTIME = bool(low_memory)
    if LOW_MEMORY_RUNTIME:
        CACHE_LIMIT_SMALL = LOWMEM_CACHE_LIMIT_SMALL
        CACHE_LIMIT_MEDIUM = LOWMEM_CACHE_LIMIT_MEDIUM
        CACHE_LIMIT_LARGE = LOWMEM_CACHE_LIMIT_LARGE
    else:
        CACHE_LIMIT_SMALL = DESKTOP_CACHE_LIMIT_SMALL
        CACHE_LIMIT_MEDIUM = DESKTOP_CACHE_LIMIT_MEDIUM
        CACHE_LIMIT_LARGE = DESKTOP_CACHE_LIMIT_LARGE
    clear_all_caches()


def begin_user_action():
    """Called at start of user action - clear caches on low memory."""
    if LOW_MEMORY_RUNTIME:
        clear_all_caches()


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
    g = gcd(a, b)
    return 'num', a // g, b // g


def num_text(text):
    if '.' not in text:
        return num(int(text))
    left, right = text.split('.', 1)
    if left == '':
        left = '0'
    if right == '':
        right = '0'
    scale = 1
    i = 0
    while i < len(right):
        scale *= 10
        i += 1
    sign = 1
    if left[:1] == '-':
        sign = -1
        left = left[1:]
    return num(sign * int(left + right), scale)


def sym(name):
    return 'sym', name


def E():
    return 'const', 'e'


def PI():
    return 'const', 'pi'


def is_num(node):
    return node[0] == 'num'


def is_sym(node):
    return node[0] == 'sym'


def is_zero(node):
    return is_num(node) and node[1] == 0


def is_one(node):
    return is_num(node) and node[1] == node[2]


def is_minus_one(node):
    return is_num(node) and node[1] == -node[2]


def is_int_num(node):
    return is_num(node) and node[2] == 1


def addq(a, b):
    return num(a[1] * b[2] + b[1] * a[2], a[2] * b[2])


def mulq(a, b):
    return num(a[1] * b[1], a[2] * b[2])


def divq(a, b):
    return num(a[1] * b[2], a[2] * b[1])


def negq(a):
    return num(-a[1], a[2])


def subq(a, b):
    return addq(a, negq(b))


def int_pow(a, n):
    if n >= 0:
        return num(a[1] ** n, a[2] ** n)
    return num(a[2] ** (-n), a[1] ** (-n))


def flat(node, kind):
    """Flatten nested add/mul nodes. Cached for performance."""
    key = (id(node), kind)
    cached = FLAT_CACHE.get(key)
    if cached is not None:
        return cached
    
    if node[0] != kind:
        return [node]
    items = node[1]
    first = items[0] if items else None
    if first is None or first[0] != kind:
        result = list(items)
        cache_store(FLAT_CACHE, key, result, CACHE_LIMIT_MEDIUM)
        return result
    out = []
    i = 0
    while i < len(items):
        if items[i][0] == kind:
            out.extend(flat(items[i], kind))
        else:
            out.append(items[i])
        i += 1
    cache_store(FLAT_CACHE, key, out, CACHE_LIMIT_MEDIUM)
    return out


def sig(node):
    """Get canonical signature of expression tree for comparison. Cached."""
    # Primitives are their own signature
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return node
    
    # Check cache using node id (expressions are immutable once created)
    key = id(node)
    cached = SIG_CACHE.get(key)
    if cached is not None:
        return cached
    
    # Compute signature
    if kind == 'fn':
        result = ('fn', node[1], sig(node[2]))
    elif kind == 'pow':
        result = ('pow', sig(node[1]), sig(node[2]))
    elif kind == 'div':
        result = ('div', sig(node[1]), sig(node[2]))
    elif kind == 'add':
        parts = []
        i = 0
        while i < len(node[1]):
            parts.append(sig(node[1][i]))
            i += 1
        parts.sort()
        result = ('add', tuple(parts))
    elif kind == 'mul':
        parts = []
        i = 0
        while i < len(node[1]):
            parts.append(sig(node[1][i]))
            i += 1
        parts.sort()
        result = ('mul', tuple(parts))
    else:
        result = node
    
    cache_store(SIG_CACHE, key, result, CACHE_LIMIT_LARGE)
    return result


def same(a, b):
    """Check if two expression trees are equivalent. Cached."""
    # Quick identity check
    if a is b or a == b:
        return True
    
    # Check cache using pair of ids
    key = (id(a), id(b))
    cached = SAME_CACHE.get(key)
    if cached is not None:
        return cached
    
    # Compare signatures
    result = sig(a) == sig(b)
    cache_store(SAME_CACHE, key, result, CACHE_LIMIT_LARGE)
    return result


def split_coeff(node):
    """Split coefficient and base from a term. Cached for performance."""
    # Check cache
    key = id(node)
    cached = SPLIT_COEFF_CACHE.get(key)
    if cached is not None:
        return cached
    
    if is_num(node):
        result = (node, num(1))
    elif node[0] == 'pow':
        if is_int_num(node[2]) and node[2][1] == 1:
            result = (num(1), node[1])
        else:
            result = (num(1), node)
    elif node[0] == 'mul':
        items = flat(node, 'mul')
        if items and is_num(items[0]):
            if len(items) == 2:
                result = (items[0], items[1])
            else:
                result = (items[0], ('mul', tuple(items[1:])))
        else:
            result = (num(1), node)
    else:
        result = (num(1), node)
    
    cache_store(SPLIT_COEFF_CACHE, key, result, CACHE_LIMIT_MEDIUM)
    return result


def make_add(parts):
    out = []
    i = 0
    while i < len(parts):
        item = parts[i]
        if item[0] == 'add':
            out.extend(item[1])
        elif item[0] == 'mul':
            inner = list(item[1])
            if len(inner) >= 2 and is_num(inner[0]) and is_minus_one(inner[0]) and inner[1][0] == 'add':
                j = 1
                while j < len(inner):
                    if inner[j][0] == 'add':
                        k = 0
                        while k < len(inner[j][1]):
                            out.append(mul([num(-1), inner[j][1][k]]))
                            k += 1
                    else:
                        out.append(mul([num(-1), inner[j]]))
                    j += 1
            else:
                out.append(item)
        else:
            out.append(item)
        i += 1
    if len(out) == 0:
        return num(0)
    if len(out) == 1:
        return out[0]
    result = 'add', tuple(out)
    return sim(result)


def make_mul(parts):
    out = []
    i = 0
    while i < len(parts):
        item = parts[i]
        if item[0] == 'mul':
            out.extend(item[1])
        else:
            out.append(item)
        i += 1
    if len(out) == 0:
        return num(1)
    if len(out) == 1:
        return out[0]
    result = 'mul', tuple(out)
    return sim(result)


def add(parts):
    return sim(make_add(parts))


def mul(parts):
    return sim(make_mul(parts))


def div(a, b):
    return sim(('div', a, b))


def power(a, b):
    return sim(('pow', a, b))


def fn(name, arg):
    if name == 'ln':
        name = 'log'
    return sim(('fn', name, arg))


def neg(node):
    return mul([num(-1), node])


def sub(a, b):
    return add([a, neg(b)])


def int_sqrt(n):
    """Integer square root - returns sqrt(n) if perfect square, else None."""
    if n <= 0:
        return None
    # Use fast integer square root if available (Python 3.8+)
    if FAST_ISQRT is not None:
        A = FAST_ISQRT(n)
        if A * A == n:
            return A
        return None
    # Fallback: Newton's method approximation
    A = int(n ** 0.5)
    while A * A > n:
        A -= 1
    while (A + 1) * (A + 1) <= n:
        A += 1
    if A * A != n:
        return None
    return A


def sqrt_num(node):
    if not is_num(node) or node[1] < 0:
        return None
    A = int_sqrt(node[1])
    B = int_sqrt(node[2])
    if A is None or B is None:
        return None
    return num(A, B)


def fn_simplify(name, arg):
    if name == 'sqrt' and is_num(arg):
        result = sqrt_num(arg)
        if result is not None:
            return result
    if name == 'abs' and is_num(arg):
        return num(abs(arg[1]), arg[2])
    if name == 'abs' and (arg[0] == 'pow' and same(arg[1], E()) or arg[0] == 'fn' and arg[1] == 'exp'):
        return arg
    if name == 'log' and is_one(arg):
        return num(0)
    if name == 'log' and same(arg, E()):
        return num(1)
    if name == 'log' and arg[0] == 'pow' and same(arg[1], E()):
        return arg[2]
    if name == 'log' and arg[0] == 'fn' and arg[1] == 'exp':
        return arg[2]
    if name == 'log' and arg[0] == 'mul':
        parts = flat(arg, 'mul')
        result = None
        i = 0
        while i < len(parts):
            log_part = fn_simplify('log', parts[i])
            if log_part is None:
                log_part = ('fn', 'log', parts[i])
            if result is None:
                result = log_part
            else:
                result = add([result, log_part])
            i += 1
        return result
    if name == 'log' and arg[0] == 'div':
        num_part = fn_simplify('log', arg[1])
        den_part = fn_simplify('log', arg[2])
        if num_part is None:
            num_part = ('fn', 'log', arg[1])
        if den_part is None:
            den_part = ('fn', 'log', arg[2])
        return sub(num_part, den_part)
    if name == 'log' and arg[0] == 'pow':
        coeff = arg[2]
        if is_num(coeff):
            inner_log = fn_simplify('log', arg[1])
            if inner_log is None:
                inner_log = ('fn', 'log', arg[1])
            return mul([coeff, inner_log])
    if name == 'exp' and is_zero(arg):
        return num(1)
    if name == 'exp' and is_one(arg):
        return E()
    if name == 'exp':
        return power(E(), arg)
    if name == 'sqrt' and arg[0] == 'pow' and same(arg[1], E()) and is_num(arg[2]) and arg[2][1] == 2 and arg[2][2] == 1:
        return power(E(), num(1, 2))
    if name == 'sqrt' and is_int_num(arg) and arg[1] > 0:
        sq = int_sqrt(arg[1])
        if sq is not None:
            return num(sq)
    return None


def binomial_coefficient(n, k):
    if k < 0 or k > n:
        return 0
    if k == 0 or k == n:
        return 1
    if k > n - k:
        k = n - k
    result = 1
    i = 0
    while i < k:
        result = result * (n - i) // (i + 1)
        i += 1
    return result


def expand_binomial(base_expr, power_expr, max_terms=None):
    """Expand (a+b)^n using binomial theorem. Returns (result, steps)."""
    if not is_num(power_expr):
        return None, None
    n = power_expr[1]
    d = power_expr[2]
    
    # Positive integer power: (a+b)^n
    if d == 1 and n >= 0 and n <= 100:
        if base_expr[0] != 'add':
            return None, None
        terms_list = list(base_expr[1])
        if len(terms_list) != 2:
            return None, None
        a, b = terms_list[0], terms_list[1]
        result_terms = []
        steps = ['Expand (' + show(add([a, b])) + ')^' + str(n)]
        i = 0
        while i <= n:
            if max_terms is not None and i > max_terms:
                break
            coeff = binomial_coefficient(n, i)
            term = mul([num(coeff), power(a, num(n - i)), power(b, num(i))])
            term = sim(expand_binomial_pow(term))
            result_terms.append(term)
            steps.append('Term ' + str(i+1) + ': C(' + str(n) + ',' + str(i) + ') * ' + show(a) + '^' + str(n-i) + ' * ' + show(b) + '^' + str(i) + ' = ' + show(term))
            i += 1
        return sim(make_add(result_terms)), steps
    
    # Negative integer power: (a+b)^(-n) as series
    elif d == 1 and n < 0:
        if base_expr[0] != 'add':
            return None, None
        terms_list = list(base_expr[1])
        if len(terms_list) != 2:
            return None, None
        a, b = terms_list[0], terms_list[1]
        coeff_a, rest_a = split_coeff(a)
        if not is_one(coeff_a):
            a, b = rest_a, mul([coeff_a, b])
            base_expr = add([a, b])
        steps = [
            'Expand (' + show(base_expr) + ')^' + str(n),
            'Factor out ' + show(a) + ': (' + show(a) + ')*(1 + ' + show(div(b, a)) + ')^' + str(n)
        ]
        result_terms = []
        i = 0
        num_terms = min(10, max_terms) if max_terms else 10
        x = div(b, a)
        while i < num_terms:
            coeff = generalized_binomial_coeff(-abs(n), 1, i)
            term = sim(mul([coeff, power(x, num(i))]))
            result_terms.append(term)
            steps.append('Term ' + str(i+1) + ': ' + show(coeff) + ' * (' + show(x) + ')^' + str(i) + ' = ' + show(term))
            i += 1
        if result_terms:
            result = sim(add([power(a, power_expr), make_add(result_terms)]))
            return result, steps
        return None, None
    
    elif d > 1:
        return None, None
    return None, None


def generalized_binomial_coeff(n_num, n_den, k):
    if k == 0:
        return num(1)
    if k < 0:
        return num(0)
    result = num(1)
    i = 0
    while i < k:
        numerator = n_num * (n_den - i)
        denominator = n_den * (i + 1)
        g = gcd(numerator, denominator)
        result = mulq(result, num(numerator // g, denominator // g))
        result = divq(result, num(i + 1))
        i += 1
    return result


def expand_binomial_pow(node):
    if node[0] == 'pow' and node[1][0] == 'add':
        power_expr = node[2]
        if is_int_num(power_expr) and power_expr[1] >= 0:
            result, steps = expand_binomial(node[1], power_expr)
            if result:
                return result
        elif is_num(power_expr) and power_expr[1] < 0:
            result, steps = expand_binomial(node[1], power_expr)
            if result:
                return result
        elif is_num(power_expr) and power_expr[2] > 1:
            result, steps = expand_binomial(node[1], power_expr)
            if result:
                return result
    if node[0] == 'pow' and node[1][0] == 'mul' and is_int_num(node[2]):
        base_items = list(node[1][1])
        new_items = []
        i = 0
        while i < len(base_items):
            if node[2][1] > 1:
                new_items.append(('pow', base_items[i], node[2]))
            else:
                new_items.append(base_items[i])
            i += 1
        return expand_binomial_pow(('mul', tuple(new_items)))
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            item = node[1][i]
            if item[0] == 'pow' and item[1][0] == 'add':
                power_expr = item[2]
                if is_int_num(power_expr) and power_expr[1] >= 0:
                    expanded, steps = expand_binomial(item[1], power_expr)
                elif is_num(power_expr) and power_expr[1] < 0:
                    expanded, steps = expand_binomial(item[1], power_expr)
                elif is_num(power_expr) and power_expr[2] > 1:
                    expanded, steps = expand_binomial(item[1], power_expr)
                else:
                    expanded = None
                if expanded:
                    if expanded[0] == 'mul':
                        items.extend(expanded[1])
                    else:
                        items.append(expanded)
            else:
                items.append(expand_binomial_pow(item))
            i += 1
        if len(items) == 0:
            return num(1)
        if len(items) == 1:
            return items[0]
        return ('mul', tuple(items))
    if node[0] == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(expand_binomial_pow(node[1][i]))
            i += 1
        if len(items) == 0:
            return num(0)
        if len(items) == 1:
            return items[0]
        return ('add', tuple(items))
    if node[0] == 'div':
        return ('div', expand_binomial_pow(node[1]), expand_binomial_pow(node[2]))
    return node


def expand_algebraic(node):
    if node[0] == 'pow' and node[1][0] == 'add':
        power_expr = node[2]
        if is_int_num(power_expr):
            result, steps = expand_binomial(node[1], power_expr)
            if result:
                return result
        elif is_num(power_expr) and power_expr[2] > 1:
            result, steps = expand_binomial(node[1], power_expr)
            if result:
                return result
        elif is_num(power_expr) and power_expr[1] < 0:
            result, steps = expand_binomial(node[1], power_expr)
            if result:
                return result
    return node


def complete_the_square(node):
    node = sim(node)
    if node[0] != 'add':
        return None, None
    items = flat(node, 'add')
    has_x2 = False
    const_term = num(0)
    coeff_x2 = num(0)
    coeff_x = num(0)
    i = 0
    while i < len(items):
        c, rest = split_coeff(items[i])
        if rest[0] == 'pow' and is_sym(rest[1]) and is_int_num(rest[2]) and rest[2][1] == 2:
            coeff_x2 = addq(coeff_x2, c)
            has_x2 = True
        elif same(rest, sym('x')):
            coeff_x = addq(coeff_x, c)
        elif is_one(rest):
            const_term = addq(const_term, c)
        elif rest[0] == 'mul':
            inner = list(flat(rest, 'mul'))
            has_x_term = False
            mult = num(1)
            j = 0
            while j < len(inner):
                if same(inner[j], sym('x')):
                    has_x_term = True
                elif is_num(inner[j]):
                    mult = mulq(mult, inner[j])
                j += 1
            if has_x_term:
                coeff_x = addq(coeff_x, mulq(c, mult))
            else:
                const_term = addq(const_term, mulq(c, rest))
        else:
            const_term = addq(const_term, mulq(c, rest))
        i += 1
    if not has_x2:
        return None, None
    if is_zero(coeff_x) and is_zero(const_term):
        return mul([coeff_x2, power(sym('x'), num(2))]), "No x or constant term"
    if is_zero(coeff_x):
        return add([mul([coeff_x2, power(sym('x'), num(2))]), const_term]), "No x term"
    a = coeff_x2
    b = coeff_x
    c_val = const_term
    if not is_one(a) and not is_minus_one(a):
        b = divq(b, a)
        c_val = divq(c_val, a)
        a = num(1)
    half_b = divq(b, num(2))
    half_b_sq = power(half_b, num(2))
    new_c = subq(c_val, half_b_sq)
    x_sq = power(sym('x'), num(2))
    bx = mul([b, sym('x')])
    sq_form = add([x_sq, bx, half_b_sq])
    if not is_one(a):
        sq_form = mul([a, sq_form])
    if is_zero(new_c):
        result = sq_form
        note = "Perfect square: (x + " + show(half_b) + ")^2"
    elif is_num(new_c) and new_c[1] > 0:
        result = add([sq_form, new_c])
        note = "Complete square: (x + " + show(half_b) + ")^2 + " + show(new_c)
    else:
        result = add([sq_form, new_c])
        note = "Complete square: (x + " + show(half_b) + ")^2 " + show(new_c)
    result = sim(result)
    return result, note


def solve_equation(node):
    node = sim(node)
    if node[0] != 'add':
        return None, None, None
    items = flat(node, 'add')
    lhs_items = []
    rhs_items = []
    i = 0
    while i < len(items):
        c, rest = split_coeff(items[i])
        if c[1] < 0:
            rhs_items.append(mul([num(-c[1], c[2]), rest]))
        else:
            lhs_items.append(items[i])
        i += 1
    lhs = make_add(lhs_items) if lhs_items else num(0)
    rhs = make_add(rhs_items) if rhs_items else num(0)
    lhs = sim(lhs)
    rhs = sim(rhs)
    diff = sub(lhs, rhs)
    diff = sim(diff)
    if diff[0] != 'add':
        return None, None, None
    poly_items = flat(diff, 'add')
    a = num(0)
    b = num(0)
    c = num(0)
    i = 0
    while i < len(poly_items):
        coeff, rest = split_coeff(poly_items[i])
        if rest[0] == 'pow' and is_sym(rest[1]) and is_int_num(rest[2]) and rest[2][1] == 2:
            a = addq(a, coeff)
        elif same(rest, sym('x')):
            b = addq(b, coeff)
        elif is_one(rest):
            c = addq(c, coeff)
        else:
            c = addq(c, mul([coeff, rest]))
        i += 1
    if is_zero(a) and is_zero(b):
        if is_zero(c):
            return None, "All real x", "Identity (infinite solutions)"
        else:
            return None, None, "No solution (contradiction)"
    if is_zero(a):
        if is_zero(b):
            return None, None, "No solution"
        x = neg(divq(c, b))
        x = sim(x)
        return "x = " + show(x), None, None
    discriminant = sub(power(b, num(2)), mul([num(4), a, c]))
    discriminant = sim(discriminant)
    if is_num(discriminant) and discriminant[1] < 0:
        return None, None, "No real solutions"
    sqrt_disc = None
    if is_num(discriminant):
        disc_val = discriminant[1] / discriminant[2]
        if disc_val >= 0:
            sqrt_val = int(disc_val ** 0.5)
            if sqrt_val * sqrt_val == disc_val:
                sqrt_disc = num(sqrt_val)
    if sqrt_disc is None:
        return None, "x = (-b ± sqrt(b^2-4ac)) / 2a", "Quadratic formula"
    neg_b = negq(b)
    two_a = mulq(num(2), a)
    root1 = divq(addq(neg_b, sqrt_disc), two_a)
    root2 = divq(subq(neg_b, sqrt_disc), two_a)
    root1 = sim(root1)
    root2 = sim(root2)
    return "x = " + show(root1) + " or x = " + show(root2), None, None


def solve_simultaneous(eqn1_text, eqn2_text, var1='x', var2='y'):
    try:
        eqn1_parts = eqn1_text.split('=')
        eqn2_parts = eqn2_text.split('=')
        
        if len(eqn1_parts) != 2 or len(eqn2_parts) != 2:
            return None, "Equations must have = sign"
        
        eqn1_left = parse(eqn1_parts[0].strip())
        eqn1_right = parse(eqn1_parts[1].strip())
        eqn1 = sub(eqn1_left, eqn1_right)
        
        eqn2_left = parse(eqn2_parts[0].strip())
        eqn2_right = parse(eqn2_parts[1].strip())
        eqn2 = sub(eqn2_left, eqn2_right)
        
        eqn1 = sim(eqn1)
        eqn2 = sim(eqn2)
        
        steps = []
        steps.append("Equation 1: " + eqn1_text + " -> " + show(eqn1) + " = 0")
        steps.append("Equation 2: " + eqn2_text + " -> " + show(eqn2) + " = 0")
        
        try:
            items1 = list(eqn1[1]) if eqn1[0] == 'add' else [eqn1]
        except:
            items1 = [eqn1]
        
        try:
            items2 = list(eqn2[1]) if eqn2[0] == 'add' else [eqn2]
        except:
            items2 = [eqn2]
        
        coeff_x1, coeff_y1, const1 = extract_linear_coeffs(eqn1, var1, var2)
        coeff_x2, coeff_y2, const2 = extract_linear_coeffs(eqn2, var1, var2)
        
        steps.append("From eqn1: " + str(coeff_x1) + "*x + " + str(coeff_y1) + "*y = " + str(const1))
        steps.append("From eqn2: " + str(coeff_x2) + "*x + " + str(coeff_y2) + "*y = " + str(const2))
        
        det = coeff_x1 * coeff_y2 - coeff_x2 * coeff_y1
        if det == 0:
            return None, "No unique solution (parallel or coincident lines)"
        
        x = (const1 * coeff_y2 - const2 * coeff_y1) / det
        y = (coeff_x1 * const2 - coeff_x2 * const1) / det
        
        steps.append("Using Cramer's rule:")
        steps.append("Det = " + str(det))
        steps.append("x = " + str(x) + ", y = " + str(y))
        
        return (str(x), str(y)), steps
        
    except Exception as err:
        return None, "Error: " + str(err)


def extract_linear_coeffs(eqn, var1, var2):
    eqn = sim(eqn)
    if eqn[0] != 'add':
        items = [eqn]
    else:
        items = flat(eqn, 'add')
    
    coeff_x = 0
    coeff_y = 0
    const = 0
    
    for item in items:
        c, rest = split_coeff(item)
        if same(rest, sym(var1)):
            coeff_x += c[1] / c[2]
        elif same(rest, sym(var2)):
            coeff_y += c[1] / c[2]
        elif is_one(rest):
            const -= c[1] / c[2]
        else:
            const -= c[1] / c[2] * sim_as_number(rest)
    
    return coeff_x, coeff_y, const


def sim_as_number(node):
    if is_num(node):
        return node[1] / node[2]
    return 1


def compose_functions(f_text, g_text, var='x'):
    try:
        f = parse(f_text)
        g = parse(g_text)
        
        steps = []
        steps.append("f(x) = " + f_text)
        steps.append("g(x) = " + g_text)
        
        fg = substitute(f, sym(var), g)
        fg = sim(fg)
        
        fg = expand_algebraic(fg)
        fg = sim(fg)
        
        steps.append("f(g(x)) = " + show(fg))
        
        return show(fg), steps
        
    except Exception as err:
        return None, ["Error: " + str(err)]


def substitute(node, old, new):
    if same(node, old):
        return new
    if is_num(node) or node[0] == 'const':
        return node
    if node[0] == 'sym':
        return new if same(node, old) else node
    if node[0] == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(substitute(node[1][i], old, new))
            i += 1
        return make_add(items)
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(substitute(node[1][i], old, new))
            i += 1
        return make_mul(items)
    if node[0] == 'pow':
        return ('pow', substitute(node[1], old, new), node[2])
    if node[0] == 'fn':
        return ('fn', node[1], substitute(node[2], old, new))
    if node[0] == 'div':
        return ('div', substitute(node[1], old, new), substitute(node[2], old, new))
    return node


def inverse_function(f_text, var='x'):
    try:
        f = parse(f_text)
        
        steps = []
        steps.append("f(x) = " + f_text)
        
        f_y = substitute(f, sym(var), sym('y'))
        steps.append("Replace f(x) with y: y = " + show(f_y))
        steps.append("Swap x and y: x = " + show(f_y))
        
        if f[0] == 'sym' and same(f, sym(var)):
            inv = sym(var)
            steps.append("Solve for y: y = " + show(inv))
            steps.append("f^-1(x) = " + show(inv))
            return show(inv), steps
        
        if f[0] == 'mul' and len(f[1]) == 2:
            inner = list(f[1])
            coeff = None
            var_part = None
            for item in inner:
                if is_num(item):
                    coeff = item
                elif same(item, sym(var)):
                    var_part = item
            if coeff and var_part and is_one(coeff):
                inv = sym(var)
                steps.append("Solve for y: y = " + show(inv))
                steps.append("f^-1(x) = " + show(inv))
                return show(inv), steps
        
        if f[0] == 'add':
            items = []
            try:
                items = list(f[1])
            except:
                items = [f]
            
            coeff_x = num(0)
            const = num(0)
            
            for item in items:
                c, rest = split_coeff(item)
                if same(rest, sym(var)):
                    coeff_x = addq(coeff_x, c)
                elif is_one(rest):
                    const = addq(const, c)
            
            if not is_zero(coeff_x):
                const_neg = mul([num(-1), const])
                inv = div(const_neg, coeff_x)
                inv = sim(inv)
                inv_x = mul([inv, sym(var)])
                inv_x = sim(inv_x)
                if is_zero(const):
                    inv_x = inv
                steps.append("Solve for y: y = " + show(inv_x))
                steps.append("f^-1(x) = " + show(inv_x))
                return show(inv_x), steps
        
        if f[0] == 'mul':
            inner = list(f[1])
            coeff_x = num(1)
            has_x = False
            
            for item in inner:
                if is_num(item):
                    coeff_x = mulq(coeff_x, item)
                elif same(item, sym(var)):
                    has_x = True
            
            if has_x:
                inv = div(num(1), coeff_x)
                inv = sim(inv)
                inv_str = show(inv) + "*x"
                steps.append("Solve for y: y = " + show(inv) + "*x")
                steps.append("f^-1(x) = " + inv_str)
                return inv_str, steps
        
        steps.append("Cannot find inverse for this function type")
        return None, steps
        
    except Exception as err:
        return None, ["Error: " + str(err)]


def solve_for_y(eqn, var):
    eqn = sim(eqn)
    if eqn[0] != 'add':
        if eqn[0] == 'pow' and same(eqn[1], sym(var)):
            return num(1)
        return eqn
    
    items = flat(eqn, 'add')
    
    y_terms = []
    other_terms = []
    
    for item in items:
        c, rest = split_coeff(item)
        if same(rest, sym(var)):
            y_terms.append(mul([num(-c[1], c[2]), sym(var)]))
        elif rest[0] == 'pow' and same(rest[1], sym(var)):
            if is_int_num(rest[2]) and rest[2][1] == 2:
                return sym(var)
            y_terms.append(mul([num(-c[1], c[2]), rest]))
        else:
            other_terms.append(mul([num(-c[1], c[2]), rest]))
    
    if y_terms:
        y_sum = make_add(y_terms) if len(y_terms) > 1 else y_terms[0]
        if other_terms:
            other_sum = make_add(other_terms)
            return div(other_sum, y_sum) if y_sum != sym(var) else sym(var)
        return sym(var) if is_one(y_sum) else y_sum
    
    return sym(var)


def extract_power(node):
    if node[0] == 'pow':
        return node[1], node[2]
    if node[0] == 'fn' and node[1] == 'sqrt':
        return node[2], num(1, 2)
    return None, None


def poly_gcd(a, b):
    while not is_zero(b):
        remainder = poly_div(a, b)
        if remainder is None:
            return b
        a, b = b, remainder
    if is_num(a):
        if a[1] < 0:
            return num(-1)
        return num(1)
    return a


def poly_div(a, b):
    return None


def poly_gcd(a, b):
    while not is_zero(b):
        remainder = poly_div(a, b)
        if remainder is None:
            return b
        a, b = b, remainder
    if is_num(a):
        if a[1] < 0:
            return num(-1)
        return num(1)
    return a


def poly_div(a, b):
    return None


def sim(node):
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return node
    if kind == 'fn':
        arg = sim(node[2])
        simplified = fn_simplify(node[1], arg)
        if simplified is not None:
            return simplified
        return 'fn', node[1], arg
    if kind == 'pow':
        base = sim(node[1])
        exp = sim(node[2])
        if is_zero(exp):
            return num(1)
        if is_one(exp):
            return base
        if is_zero(base):
            if is_num(exp) and exp[1] > 0:
                return num(0)
        if is_one(base):
            return num(1)
        if is_num(exp) and is_int_num(exp):
            if is_num(base):
                return int_pow(base, exp[1])
            if base[0] == 'pow' and is_num(base[2]):
                return power(base[1], mulq(base[2], exp))
            if base[0] == 'div' and is_int_num(exp):
                if exp[1] > 0:
                    return div(power(base[1], exp), power(base[2], exp))
                return div(power(base[2], num(-exp[1])), power(base[1], num(-exp[1])))
        return 'pow', base, exp
    if kind == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            item = sim(node[1][i])
            if item[0] == 'add':
                items.extend(item[1])
            else:
                items.append(item)
            i += 1
        total = num(0)
        order = []
        data = {}
        i = 0
        while i < len(items):
            c, rest = split_coeff(items[i])
            if is_one(rest):
                total = addq(total, c)
            else:
                key = sig(rest)
                if key not in data:
                    order.append(key)
                    data[key] = [rest, num(0)]
                data[key][1] = addq(data[key][1], c)
            i += 1
        out = []
        i = 0
        while i < len(order):
            rest, c = data[order[i]]
            if is_zero(c):
                i += 1
                continue
            if is_one(c):
                out.append(rest)
            elif is_minus_one(c):
                out.append(mul([num(-1), rest]))
            else:
                out.append(mul([c, rest]))
            i += 1
        if not is_zero(total) or len(out) == 0:
            out.append(total)
        if len(out) == 0:
            return num(0)
        if len(out) == 1:
            return out[0]
        return 'add', tuple(out)
    if kind == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            item = sim(node[1][i])
            if item[0] == 'mul':
                items.extend(item[1])
            else:
                items.append(item)
            i += 1
        has_div = False
        i = 0
        while i < len(items):
            if items[i][0] == 'div':
                has_div = True
                break
            i += 1
        if has_div:
            top_bits = []
            bot_bits = []
            i = 0
            while i < len(items):
                if items[i][0] == 'div':
                    top_bits.append(items[i][1])
                    bot_bits.append(items[i][2])
                else:
                    top_bits.append(items[i])
                i += 1
            return sim(('div', make_mul(top_bits), make_mul(bot_bits)))
        coeff = num(1)
        order = []
        data = {}
        counted_sym_keys = set()  # Track keys that had all occurrences counted
        has_neg_add = False
        neg_add_items = None
        other_items = []
        i = 0
        while i < len(items):
            item = items[i]
            if is_zero(item):
                return num(0)
            if is_num(item):
                coeff = mulq(coeff, item)
            elif item[0] == 'mul':
                inner = list(item[1])
                if len(inner) >= 2 and is_num(inner[0]) and is_minus_one(inner[0]) and inner[1][0] == 'add':
                    has_neg_add = True
                    neg_add_items = list(inner[1][1])
                elif len(inner) == 2 and same(inner[0], inner[1]):
                    key = sig(inner[0])
                    if key not in data:
                        order.append(key)
                        data[key] = [inner[0], num(0)]
                    data[key][1] = add([data[key][1], num(2)])
                else:
                    other_items.append(item)
            elif item[0] == 'sym':
                key = sig(item)
                if key in counted_sym_keys:
                    # Already counted all occurrences of this symbol, skip
                    i += 1
                    continue
                if key in data:
                    # Key was added by pow item, add 1 to exponent
                    data[key][1] = add([data[key][1], num(1)])
                    i += 1
                    continue
                # Count all occurrences of this symbol
                j = i + 1
                count = 1
                while j < len(items):
                    if same(items[j], item):
                        count += 1
                    j += 1
                order.append(key)
                data[key] = [item, num(0)]
                data[key][1] = add([data[key][1], num(count)])
                counted_sym_keys.add(key)
            elif item[0] == 'pow':
                if item[1][0] == 'mul':
                    inner = list(item[1][1])
                    if len(inner) >= 2 and is_num(inner[0]) and is_minus_one(inner[0]) and inner[1][0] == 'add':
                        has_neg_add = True
                        neg_add_items = list(inner[1][1])
                    else:
                        key = sig(item[1])
                        if key not in data:
                            order.append(key)
                            data[key] = [item[1], num(0)]
                        data[key][1] = add([data[key][1], item[2]])
                else:
                    key = sig(item[1])
                    if key not in data:
                        order.append(key)
                        data[key] = [item[1], num(0)]
                    data[key][1] = add([data[key][1], item[2]])
            else:
                other_items.append(item)
            i += 1
        temp_out = []
        if is_zero(coeff):
            return num(0)
        if not is_one(coeff):
            temp_out.append(coeff)
        i = 0
        while i < len(order):
            base, exp = data[order[i]]
            if is_zero(exp):
                i += 1
                continue
            if is_one(exp):
                temp_out.append(base)
            else:
                temp_out.append(('pow', base, exp))
            i += 1
        i = 0
        while i < len(other_items):
            temp_out.append(other_items[i])
            i += 1
        if has_neg_add and neg_add_items is not None:
            new_items = []
            j = 0
            while j < len(neg_add_items):
                new_items.append(mul([num(-1), neg_add_items[j]]))
                j += 1
            return make_add(temp_out + new_items)
        if len(temp_out) == 0:
            return num(1)
        if len(temp_out) == 1:
            return temp_out[0]
        return 'mul', tuple(temp_out)
    if kind == 'div':
        top = sim(node[1])
        bot = sim(node[2])
        if is_zero(top):
            return num(0)
        if is_one(bot):
            return top
        if same(top, bot):
            return num(1)
        if is_num(top) and is_num(bot):
            return divq(top, bot)
        if bot[0] == 'mul':
            bot_items = list(bot[1])
            has_neg_add = False
            neg_add_items = None
            new_bot_items = []
            i = 0
            while i < len(bot_items):
                item = bot_items[i]
                if item[0] == 'mul':
                    inner = list(item[1])
                    if len(inner) >= 2 and is_num(inner[0]) and is_minus_one(inner[0]) and inner[1][0] == 'add':
                        has_neg_add = True
                        neg_add_items = list(inner[1][1])
                    else:
                        new_bot_items.append(item)
                else:
                    new_bot_items.append(item)
                i += 1
            if has_neg_add and neg_add_items is not None:
                new_items = []
                j = 0
                while j < len(neg_add_items):
                    new_items.append(mul([num(-1), neg_add_items[j]]))
                    j += 1
                new_top = add([top] + new_items)
                if len(new_bot_items) == 0:
                    return new_top
                if len(new_bot_items) == 1:
                    return div(new_top, new_bot_items[0])
                return div(new_top, make_mul(new_bot_items))
        if top[0] == 'pow' and same(top[1], bot):
            if is_int_num(top[2]) and top[2][1] > 1:
                return power(bot, num(top[2][1] - 1))
            if is_int_num(top[2]) and top[2][1] == 1:
                return bot
        if top[0] == 'pow' and bot[0] == 'pow' and same(top[1], bot[1]):
            if is_int_num(top[2]) and is_int_num(bot[2]):
                new_exp = num(top[2][1] - bot[2][1])
                if is_zero(new_exp):
                    return num(1)
                if new_exp[1] > 0:
                    return power(top[1], new_exp)
        if top[0] == 'mul':
            top_items = list(top[1])
            matching_idx = -1
            matching_exp = None
            i = 0
            while i < len(top_items):
                if same(top_items[i], bot):
                    matching_idx = i
                    if top_items[i][0] == 'pow':
                        matching_exp = top_items[i][2]
                    break
                if top_items[i][0] == 'pow' and same(top_items[i][1], bot):
                    matching_idx = i
                    matching_exp = top_items[i][2]
                    break
                i += 1
            if matching_idx >= 0:
                remaining_items = []
                j = 0
                while j < len(top_items):
                    if j != matching_idx:
                        remaining_items.append(top_items[j])
                    j += 1
                if matching_exp is not None and is_int_num(matching_exp):
                    exp_val = matching_exp[1]
                    if exp_val == 1:
                        if len(remaining_items) == 0:
                            return bot
                        if len(remaining_items) == 1:
                            return mul([remaining_items[0], bot])
                        return mul(remaining_items + [bot])
                    elif exp_val == 2 and len(remaining_items) == 1 and is_num(remaining_items[0]):
                        return remaining_items[0]
                    else:
                        return div(make_mul(remaining_items), power(bot, num(exp_val - 1)))
                elif len(remaining_items) == 1 and is_num(remaining_items[0]):
                    return remaining_items[0]
                elif len(remaining_items) == 0:
                    return num(1)
        return 'div', top, bot
    return node


def show(node, parent=0):
    """Convert expression tree to string representation. Cached."""
    # Check cache using node id and parent level
    key = (id(node), parent)
    cached = SHOW_CACHE.get(key)
    if cached is not None:
        return cached
    
    kind = node[0]
    if kind == 'num':
        if node[2] == 1:
            return str(node[1])
        result = str(node[1]) + '/' + str(node[2])
        cache_store(SHOW_CACHE, key, result, CACHE_LIMIT_MEDIUM)
        return result
    if kind == 'sym':
        cache_store(SHOW_CACHE, key, node[1], CACHE_LIMIT_MEDIUM)
        return node[1]
    if kind == 'const':
        cache_store(SHOW_CACHE, key, node[1], CACHE_LIMIT_MEDIUM)
        return node[1]
    if kind == 'fn':
        if node[1] == 'log':
            result = 'ln(' + show(node[2], 0) + ')'
        else:
            result = node[1] + '(' + show(node[2], 0) + ')'
        cache_store(SHOW_CACHE, key, result, CACHE_LIMIT_MEDIUM)
        return result
    if kind == 'pow':
        left = show(node[1], 3)
        right = show(node[2], 3)
        if node[1][0] == 'fn' or (is_num(node[1]) and (node[1][2] != 1 or node[1][1] < 0)) or node[1][0] in ('sym', 'mul'):
            left = '(' + left + ')'
        if is_num(node[2]) and node[2][2] != 1:
            right = '(' + right + ')'
        result = left + '^' + right
        cache_store(SHOW_CACHE, key, result, CACHE_LIMIT_MEDIUM)
        return result
    if kind == 'mul':
        items = flat(node, 'mul')
        parts = []
        i = 0
        while i < len(items):
            part = show(items[i], 2)
            if items[i][0] == 'div' or (is_num(items[i]) and items[i][2] != 1):
                part = '(' + part + ')'
            parts.append(part)
            i += 1
        result = '*'.join(parts)
        cache_store(SHOW_CACHE, key, result, CACHE_LIMIT_MEDIUM)
        return result
    if kind == 'div':
        left = show(node[1], 2)
        right = show(node[2], 2)
        if node[1][0] in ('add', 'mul') or (is_num(node[1]) and node[1][2] != 1):
            left = '(' + left + ')'
        # Special case: avoid double parentheses for simple binomial denominators
        if node[2][0] == 'add' and len(node[2][1]) == 2:
            terms = list(node[2][1])
            simple_terms = 0
            for term in terms:
                coeff, rest = split_coeff(term)
                if is_sym(rest) and (is_one(coeff) or is_minus_one(coeff)):
                    simple_terms += 1
                elif is_one(rest):
                    simple_terms += 1
            if simple_terms == 2:
                right = show(node[2], 2)
            else:
                if node[2][0] in ('add', 'mul', 'div') or (is_num(node[2]) and node[2][2] != 1):
                    right = '(' + right + ')'
        elif node[2][0] in ('add', 'mul', 'div') or (is_num(node[2]) and node[2][2] != 1):
            right = '(' + right + ')'
        result = left + '/' + right
        cache_store(SHOW_CACHE, key, result, CACHE_LIMIT_MEDIUM)
        return result
    # Add expression
    bits = flat(node, 'add')
    text = ''
    i = 0
    while i < len(bits):
        item = bits[i]
        coeff, rest = split_coeff(item)
        if i == 0:
            if coeff[1] < 0:
                text = '-' + show(mul([num(-coeff[1], coeff[2]), rest]), 1)
            else:
                text = show(item, 1)
        else:
            if coeff[1] < 0:
                text += '-' + show(mul([num(-coeff[1], coeff[2]), rest]), 1)
            else:
                text += '+' + show(item, 1)
        i += 1
    if 1 < parent:
        result = '(' + text + ')'
    else:
        result = text
    cache_store(SHOW_CACHE, key, result, CACHE_LIMIT_MEDIUM)
    return result


def is_name_start(ch):
    return ('A' <= ch <= 'Z') or ('a' <= ch <= 'z') or ch == '_'


def is_name_char(ch):
    return is_name_start(ch) or ('0' <= ch <= '9')


def is_digit_char(ch):
    return '0' <= ch <= '9'


def is_num_token_start(text, i):
    ch = text[i]
    return is_digit_char(ch) or (ch == '.' and i + 1 < len(text) and is_digit_char(text[i + 1]))


FUNC_NAMES = ('sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'exp', 'log', 'sqrt', 'abs', 'ln', 'atan', 'asin', 'acos', 'asin', 'sinh', 'cosh', 'tanh')
FUNC_ALIASES = {'csc': 'cosec', 'arcsin': 'asin', 'arccos': 'acos', 'arctan': 'atan'}


def parse(text):
    toks = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in ' \t\r\n':
            i += 1
        elif text[i:i + 2] == '**':
            toks.append('**')
            i += 2
        elif ch in '()+-*/^=,':
            toks.append(ch)
            i += 1
        elif is_num_token_start(text, i):
            j = i
            seen_dot = 0
            while j < len(text) and (is_digit_char(text[j]) or text[j] == '.'):
                if text[j] == '.':
                    seen_dot += 1
                j += 1
            if seen_dot > 1:
                raise ValueError('Bad number.')
            toks.append(text[i:j])
            i = j
        elif is_name_start(ch):
            j = i + 1
            while j < len(text) and is_name_char(text[j]):
                j += 1
            word = text[i:j]
            toks.append(word)
            i = j
        else:
            raise ValueError('Unexpected character: ' + ch)

    p = 0

    def cur():
        if p >= len(toks):
            return ''
        return toks[p]

    def eat(x):
        nonlocal p
        if cur() != x:
            raise ValueError('Expected ' + repr(x) + ' but found ' + repr(cur()) + '.')
        p += 1

    def is_atom_start(t):
        if t == '' or t in (')', '+', '-', '*', '/', '^', '**', '=', ','):
            return False
        if t == '(':
            return True
        if is_digit_char(t[0]) or t[0] == '.':
            return True
        if is_name_start(t[0]):
            return True
        return False

    def starts_implicit(t):
        if not is_atom_start(t):
            return False
        if t in FUNC_NAMES:
            return False
        return True

    def atom():
        nonlocal p
        t = cur()
        if t == '(':
            eat('(')
            out = expr()
            eat(')')
            return out
        if t == '-':
            eat('-')
            return neg(atom())
        if t and (is_digit_char(t[0]) or t[0] == '.'):
            p += 1
            return num_text(t)
        if t == 'e':
            p += 1
            return E()
        if t == 'pi':
            p += 1
            return PI()
        if t:
            p += 1
            low = t.lower()
            if low == 'ln':
                return fn('log', atom())
            if low in FUNC_ALIASES:
                actual_name = FUNC_ALIASES[low]
                if cur() == '(' and actual_name in FUNC_NAMES:
                    eat('(')
                    out = expr()
                    eat(')')
                    return fn(actual_name, out)
            if cur() == '(' and low in FUNC_NAMES:
                eat('(')
                out = expr()
                eat(')')
                return fn(low, out)
            return sym(t)
        raise ValueError('Unexpected end.')

    def power_part():
        left = atom()
        if cur() == '^' or cur() == '**':
            eat(cur())
            return power(left, power_exp())
        return left

    def power_exp():
        out = power_part()
        while starts_implicit(cur()):
            out = mul([out, power_part()])
        return out

    def term():
        out = power_exp()
        while True:
            if cur() == '*':
                eat('*')
                out = mul([out, power_exp()])
            elif cur() == '/':
                eat('/')
                out = div(out, power_exp())
            elif starts_implicit(cur()):
                out = mul([out, power_exp()])
            else:
                break
        return out

    def expr():
        out = term()
        while cur() in ('+', '-'):
            if cur() == '+':
                eat('+')
                out = add([out, term()])
            else:
                eat('-')
                out = add([out, neg(term())])
        return out

    out = expr()
    if cur():
        raise ValueError('Unexpected token: ' + cur())
    return sim(out)


def equivalent(a, b):
    """Check if two expressions are mathematically equivalent. Cached."""
    # Quick identity check
    if a is b or a == b:
        return True
    
    # Check cache
    key = (id(a), id(b))
    cached = EQUIV_CACHE.get(key)
    if cached is not None:
        return cached
    
    # Try signature comparison first (fast path)
    if sig(a) == sig(b):
        cache_store(EQUIV_CACHE, key, True, CACHE_LIMIT_MEDIUM)
        return True
    
    # Compute a - b and check if zero
    diff = sim(add([a, neg(b)]))
    if is_zero(diff):
        cache_store(EQUIV_CACHE, key, True, CACHE_LIMIT_MEDIUM)
        return True
    
    # Try expanding powers and square roots
    diff = expand_all_pow_sqrt(diff)
    diff = sim(diff)
    if is_zero(diff):
        cache_store(EQUIV_CACHE, key, True, CACHE_LIMIT_MEDIUM)
        return True
    
    # Try the reverse: b - a
    diff2 = sim(add([b, neg(a)]))
    if is_zero(diff2):
        cache_store(EQUIV_CACHE, key, True, CACHE_LIMIT_MEDIUM)
        return True
    diff2 = expand_all_pow_sqrt(diff2)
    diff2 = sim(diff2)
    if is_zero(diff2):
        cache_store(EQUIV_CACHE, key, True, CACHE_LIMIT_MEDIUM)
        return True
    
    cache_store(EQUIV_CACHE, key, False, CACHE_LIMIT_MEDIUM)
    return False


def show_equation(left, right):
    return show(left) + ' = ' + show(right)


def simplify_step(node, step_num):
    simplified = sim(node)
    return step_num, 'Simplify: ' + show(simplified), simplified


def expand_pow_sqrt(node):
    if node[0] == 'pow' and node[1][0] == 'fn' and node[1][1] == 'sqrt':
        base = node[1][2]
        exp = node[2]
        if is_num(exp) and exp[1] == 2 and exp[2] == 1:
            return base
        if is_num(exp) and exp[1] == 3 and exp[2] == 1:
            return power(base, num(3, 2))
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(expand_pow_sqrt(node[1][i]))
            i += 1
        return make_mul(items)
    if node[0] == 'div':
        return 'div', expand_pow_sqrt(node[1]), expand_pow_sqrt(node[2])
    if node[0] == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(expand_pow_sqrt(node[1][i]))
            i += 1
        return make_add(items)
    if node[0] == 'pow':
        return 'pow', expand_pow_sqrt(node[1]), expand_pow_sqrt(node[2])
    return node


def normalize_sqrt_pow(node):
    node = sim(node)
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(normalize_sqrt_pow(node[1][i]))
            i += 1
        return make_mul(items)
    if node[0] == 'pow' and node[1][0] == 'fn' and node[1][1] == 'sqrt':
        base = node[1][2]
        exp = node[2]
        return power(base, exp)
    return node


def convert_pow_to_sqrt_form(node):
    return node


def convert_sqrt_to_pow_form(node):
    if node[0] == 'pow' and is_num(node[2]):
        base = node[1]
        exp = node[2]
        if exp[2] == 2 and is_int_num(num(exp[1], 2)):
            half_exp = num(exp[1] // 2)
            return power(base, half_exp)
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(convert_sqrt_to_pow_form(node[1][i]))
            i += 1
        return make_mul(items)
    if node[0] == 'div':
        return 'div', convert_sqrt_to_pow_form(node[1]), convert_sqrt_to_pow_form(node[2])
    if node[0] == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(convert_sqrt_to_pow_form(node[1][i]))
            i += 1
        return make_add(items)
    return node


def expand_all_pow_sqrt(node):
    node = expand_pow_sqrt(node)
    node = sim(node)
    node = convert_pow_to_sqrt_form(node)
    node = sim(node)
    node = expand_mul_distribute(node)
    node = sim(node)
    node = combine_fractions(node)
    node = simplify_trig_identity(node)
    node = sim(node)
    return node


def combine_fractions(node):
    if node[0] == 'add':
        items = flat(node, 'add')
        numerator_terms = {}
        denominators = {}
        i = 0
        while i < len(items):
            item = items[i]
            if item[0] == 'div':
                top = item[1]
                bot = item[2]
                key = sig(bot)
                if key not in denominators:
                    denominators[key] = bot
                    numerator_terms[key] = []
                numerator_terms[key].append(top)
            else:
                key = 'none'
                if key not in denominators:
                    denominators[key] = num(1)
                    numerator_terms[key] = []
                numerator_terms[key].append(item)
            i += 1
        if len(denominators) == 1:
            for k in denominators:
                if k != 'none' or len(numerator_terms) > 1:
                    combined = make_add(numerator_terms[k])
                    return div(combined, denominators[k])
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(combine_fractions(node[1][i]))
            i += 1
        return make_mul(items)
    if node[0] == 'div':
        return 'div', combine_fractions(node[1]), combine_fractions(node[2])
    return node


def expand_mul_distribute(node):
    if node[0] == 'mul':
        items = flat(node, 'mul')
        has_add = False
        i = 0
        while i < len(items):
            if items[i][0] == 'add':
                has_add = True
                break
            i += 1
        if has_add:
            result = None
            i = 0
            while i < len(items):
                if items[i][0] == 'add':
                    inner_items = list(items[i][1])
                    j = 0
                    while j < len(inner_items):
                        rest = items[:i] + [inner_items[j]] + items[i + 1:]
                        term = make_mul(rest)
                        if result is None:
                            result = term
                        else:
                            result = add([result, term])
                        j += 1
                    return expand_mul_distribute(sim(result))
                i += 1
    if node[0] == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(expand_mul_distribute(node[1][i]))
            i += 1
        return make_add(items)
    if node[0] == 'div':
        return 'div', expand_mul_distribute(node[1]), expand_mul_distribute(node[2])
    return node


def extract_linear_expr(node, var):
    if node[0] == 'add':
        terms = flat(node, 'add')
        coeff_a = num(0)
        coeff_x = num(0)
        const = num(0)
        i = 0
        while i < len(terms):
            term = terms[i]
            c, rest = split_coeff(term)
            if same(rest, sym(var)):
                coeff_x = addq(coeff_x, c)
            elif rest[0] == 'mul':
                items = flat(rest, 'mul')
                has_var = False
                mult = num(1)
                j = 0
                while j < len(items):
                    if same(items[j], sym(var)):
                        has_var = True
                    elif is_num(items[j]):
                        mult = mulq(mult, items[j])
                    else:
                        mult = mulq(mult, items[j])
                    j += 1
                if has_var:
                    coeff_x = addq(coeff_x, mulq(c, mult))
                else:
                    const = addq(const, mulq(c, rest))
            else:
                const = addq(const, mulq(c, rest))
            i += 1
        return coeff_x, const
    return num(0), node


def solve_for_unknown_constants(derived_expr, target_with_A_B, unknown_names):
    steps = []
    step_num = 1
    steps.append((step_num, 'Given expression: ' + show(derived_expr), derived_expr))
    step_num += 1
    steps.append((step_num, 'Target form with unknowns: ' + show(target_with_A_B), target_with_A_B))
    step_num += 1
    expanded_derived = expand_all_pow_sqrt(derived_expr)
    steps.append((step_num, 'Expand sqrt/powers: ' + show(expanded_derived), expanded_derived))
    step_num += 1
    expanded_target = expand_all_pow_sqrt(target_with_A_B)
    steps.append((step_num, 'Expand target form: ' + show(expanded_target), expanded_target))
    step_num += 1
    if not equivalent(expanded_derived, expanded_target):
        diff = sim(add([expanded_derived, neg(expanded_target)]))
        steps.append((step_num, 'Expressions not equivalent. Difference: ' + show(diff), None))
        return None, steps
    normalized_derived = convert_pow_to_sqrt_form(expanded_derived)
    normalized_derived = sim(normalized_derived)
    normalized_target = convert_pow_to_sqrt_form(expanded_target)
    normalized_target = sim(normalized_target)
    steps.append((step_num, 'Normalized: ' + show(normalized_derived) + ' vs ' + show(normalized_target), None))
    step_num += 1
    steps.append((step_num, 'Expressions ARE equivalent. Unknowns can be determined.', None))
    return True, steps


def rearrange_to_target(source, target_template):
    steps = []
    current = sim(source)
    steps.append((1, 'Original: ' + show(current), current))
    current = sim(current)
    steps.append((2, 'Simplify: ' + show(current), current))
    return steps, current


def simplify_trig_identity(node):
    if node[0] == 'add':
        items = flat(node, 'add')
        if len(items) == 2:
            t1, t2 = items[0], items[1]
            s1 = is_pow_of_fn(t1, 'sin', num(2))
            s2 = is_pow_of_fn(t2, 'cos', num(2))
            if s1 and s2 and same(s1, s2):
                return num(1)
            c1 = is_pow_of_fn(t1, 'cos', num(2))
            c2 = is_pow_of_fn(t2, 'sin', num(2))
            if c1 and c2 and same(c1, c2):
                return num(1)
            t1_check = is_pow_of_fn(t1, 'tan', num(2))
            t2_check = is_pow_of_fn(t2, 'sec', num(2))
            if t1_check and t2_check and same(t1_check, t2_check):
                return num(1)
            sec1 = is_pow_of_fn(t1, 'sec', num(2))
            sec2 = is_pow_of_fn(t2, 'sec', num(2))
            if sec1 and sec2:
                if same(sec1, sec2):
                    tan_arg = same(t1_check, sec2) if t1_check else None
                    if tan_arg:
                        return num(1)
            cot1 = is_pow_of_fn(t1, 'cot', num(2))
            cosec1 = is_pow_of_fn(t1, 'cosec', num(2))
            if cot1 and cosec1 and same(cot1, cosec1):
                return num(1)
        if len(items) == 3:
            pos_terms = []
            neg_terms = []
            i = 0
            while i < len(items):
                c, rest = split_coeff(items[i])
                if c[1] < 0:
                    neg_terms.append(mul([num(-c[1], c[2]), rest]))
                else:
                    pos_terms.append(items[i])
                i += 1
            if len(pos_terms) == 2 and len(neg_terms) == 1:
                t1, t2 = pos_terms[0], pos_terms[1]
                s1 = is_pow_of_fn(t1, 'sin', num(2))
                s2 = is_pow_of_fn(t2, 'cos', num(2))
                if s1 and s2 and same(s1, s2):
                    return neg(neg_terms[0])
                c1 = is_pow_of_fn(t1, 'cos', num(2))
                c2 = is_pow_of_fn(t2, 'sin', num(2))
                if c1 and c2 and same(c1, c2):
                    return neg(neg_terms[0])
                t1_check = is_pow_of_fn(t1, 'tan', num(2))
                t2_check = is_pow_of_fn(t2, 'sec', num(2))
                if t1_check and t2_check and same(t1_check, t2_check):
                    return neg(neg_terms[0])
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(simplify_trig_identity(node[1][i]))
            i += 1
        return make_mul(items)
    if node[0] == 'div':
        return 'div', simplify_trig_identity(node[1]), simplify_trig_identity(node[2])
    if node[0] == 'pow':
        return 'pow', simplify_trig_identity(node[1]), simplify_trig_identity(node[2])
    if node[0] == 'fn':
        return 'fn', node[1], simplify_trig_identity(node[2])
    return node


def is_pow_of_fn(node, fn_name, exp):
    if node[0] == 'pow':
        if same(node[2], exp) and node[1][0] == 'fn' and node[1][1] == fn_name:
            return node[1][2]
    return None


def compare_expressions(expr1, expr2):
    steps = []
    step_num = 1
    steps.append((step_num, 'Expression 1: ' + show(expr1), expr1))
    step_num += 1
    steps.append((step_num, 'Expression 2: ' + show(expr2), expr2))
    step_num += 1
    simple1 = sim(expr1)
    simple1 = simplify_trig_identity(simple1)
    steps.append((step_num, 'Simplify expr1: ' + show(simple1), simple1))
    step_num += 1
    simple2 = sim(expr2)
    simple2 = simplify_trig_identity(simple2)
    steps.append((step_num, 'Simplify expr2: ' + show(simple2), simple2))
    step_num += 1
    if equivalent(simple1, simple2):
        steps.append((step_num, 'EXPRESSIONS ARE EQUIVALENT', simple1))
        return True, steps
    diff = show(sim(add([simple1, neg(simple2)])))
    steps.append((step_num, 'Difference (should be 0): ' + diff, None))
    return False, steps


def main():
    print('1 compare')
    print('2 transform')
    print('3 expand')
    print('4 poly ops')
    print('5 complete sq')
    print('6 solve')
    print('7 simult eqns')
    print('8 composite')
    print('9 inverse')
    mode = input('Mode: ').strip()
    if mode == '':
        mode = '1'
    begin_user_action()
    try:
        if mode == '1':
            text1 = input('Expr1: ').strip()
            text2 = input('Expr2: ').strip()
            expr1 = parse(text1)
            expr2 = parse(text2)
            print('1. expr1 = ' + show(expr1))
            print('2. expr2 = ' + show(expr2))
            print('3. Simplify both...')
            s1 = sim(expr1)
            s2 = sim(expr2)
            print('4. expr1 = ' + show(s1))
            print('5. expr2 = ' + show(s2))
            equal, steps = compare_expressions(expr1, expr2)
            i = 0
            while i < len(steps):
                num, desc, _ = steps[i]
                print(str(num + 5) + '. ' + desc)
                i += 1
            if equal:
                print(str(len(steps) + 6) + '. RESULT: Equivalent')
            else:
                print(str(len(steps) + 6) + '. RESULT: Not equivalent')
        elif mode == '2':
            text1 = input('Source: ').strip()
            expr1 = parse(text1)
            text2 = input('Target form (with A,B): ').strip()
            expr2 = parse(text2)
            print('1. Source = ' + show(expr1))
            print('2. Target = ' + show(expr2))
            steps, result = rearrange_to_target(expr1, expr2)
            i = 0
            while i < len(steps):
                num, desc, _ = steps[i]
                print(str(num + 2) + '. ' + desc)
                i += 1
            print(str(len(steps) + 3) + '. Final = ' + show(result))
        elif mode == '3':
            text1 = input('Expression: ').strip()
            expr1 = parse(text1)
            max_terms_str = input('Max terms (Enter for all): ').strip()
            max_terms = None
            if max_terms_str != '':
                try:
                    max_terms = int(max_terms_str)
                except:
                    pass
            print('1. Input = ' + show(expr1))
            if expr1[0] == 'pow' and expr1[1][0] == 'add':
                power_expr = expr1[2]
                if is_int_num(power_expr) and power_expr[1] >= 0:
                    expanded, steps = expand_binomial(expr1[1], power_expr, max_terms)
                    if expanded:
                        print('2. Binomial expansion')
                        i = 0
                        while i < len(steps):
                            print(str(i+3) + '. ' + steps[i])
                            i += 1
                        print(str(len(steps) + 3) + '. Output = ' + show(expanded))
                    else:
                        print('2. Cannot expand (not binomial)')
                        print('3. Output = ' + show(expr1))
                elif is_num(power_expr) and power_expr[1] < 0:
                    expanded, steps = expand_binomial(expr1[1], power_expr, max_terms)
                    if expanded:
                        print('2. Binomial expansion (negative power)')
                        i = 0
                        while i < len(steps):
                            print(str(i+3) + '. ' + steps[i])
                            i += 1
                        print(str(len(steps) + 3) + '. Output = ' + show(expanded))
                    else:
                        print('2. Cannot expand')
                        print('3. Output = ' + show(expr1))
                else:
                    print('2. Cannot expand')
                    print('3. Output = ' + show(expr1))
            else:
                print('2. Cannot expand (not binomial)')
                print('3. Output = ' + show(expr1))
        elif mode == '4':
            print('1 add polys')
            print('2 subtract polys')
            print('3 multiply polys')
            submode = input('Operation: ').strip()
            if submode == '1' or submode == '2' or submode == '3':
                text1 = input('Poly1: ').strip()
                text2 = input('Poly2: ').strip()
                p1 = parse(text1)
                p2 = parse(text2)
                print('1. p1 = ' + show(p1))
                print('2. p2 = ' + show(p2))
                if submode == '1':
                    print('3. Add: p1 + p2')
                    result = add([p1, p2])
                    print('4. Sum = ' + show(sim(result)))
                elif submode == '2':
                    print('3. Subtract: p1 - p2')
                    result = sub(p1, p2)
                    print('4. Difference = ' + show(sim(result)))
                else:
                    print('3. Multiply: p1 * p2')
                    result = mul([p1, p2])
                    print('4. Product = ' + show(sim(result)))
        elif mode == '5':
            text1 = input('Expression: ').strip()
            expr1 = parse(text1)
            print('1. Input = ' + show(expr1))
            result, note = complete_the_square(expr1)
            if result is None:
                print('2. Cannot complete square')
                print('3. Output = ' + show(expr1))
            else:
                print('2. ' + (note if note else 'Result'))
                print('3. Result = ' + show(result))
        elif mode == '6':
            text1 = input('Equation (set = 0): ').strip()
            if '=' not in text1:
                print('Input should be in form f(x)=0 or expression=0')
            else:
                parts = text1.split('=')
                left = parse(parts[0].strip())
                if len(parts) > 1:
                    right = parse(parts[1].strip())
                    expr = sub(left, right)
                    expr = sim(expr)
                else:
                    expr = sim(left)
                print('1. Equation = ' + show(expr) + ' = 0')
                solution, formula, note = solve_equation(expr)
                if solution is not None:
                    print('2. ' + solution)
                    if note:
                        print('3. ' + note)
                elif formula is not None:
                    print('2. ' + formula)
                    if note:
                        print('3. ' + note)
                else:
                    print('2. ' + (note if note else 'No solution'))
        elif mode == '7':
            print('1 linear+linear')
            submode = input('Type: ').strip()
            if submode == '1':
                text1 = input('Eqn1 (e.g. 2*x+3*y=5): ').strip()
                text2 = input('Eqn2 (e.g. x-y=1): ').strip()
                if text1 == '':
                    text1 = '2*x+3*y=5'
                    text2 = 'x-y=1'
                    print('Using: ' + text1 + ', ' + text2)
                result, steps = solve_simultaneous(text1, text2)
                print('1. ' + steps[0])
                print('2. ' + steps[1])
                if result:
                    i = 2
                    while i < len(steps):
                        print(str(i+1) + '. ' + steps[i])
                        i += 1
                    print('3. Solution: x=' + result[0] + ', y=' + result[1])
                else:
                    print('2. ' + str(result))
        elif mode == '8':
            print('Composite functions f(g(x))')
            f_text = input('f(x): ').strip()
            g_text = input('g(x): ').strip()
            if f_text == '':
                f_text = '2*x+1'
                g_text = 'x^2'
                print('Using: f(x)=' + f_text + ', g(x)=' + g_text)
            result, steps = compose_functions(f_text, g_text)
            print('1. f(x) = ' + f_text)
            print('2. g(x) = ' + g_text)
            if result:
                i = 0
                while i < len(steps):
                    print(str(i+3) + '. ' + steps[i])
                    i += 1
            else:
                print('3. ' + steps[0])
        elif mode == '9':
            print('Inverse functions')
            f_text = input('f(x): ').strip()
            if f_text == '':
                f_text = '2*x+1'
                print('Using: f(x)=' + f_text)
            result, steps = inverse_function(f_text)
            print('1. f(x) = ' + f_text)
            if result:
                i = 0
                while i < len(steps):
                    print(str(i+2) + '. ' + steps[i])
                    i += 1
            else:
                print('2. ' + steps[0])
        else:
            print('Mode must be 1-9.')
    except Exception as err:
        print('Input error: ' + str(err))


run = main
if not SKIP_AUTORUN:
    main()