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
SHOW_CACHE = {}
SPLIT_COEFF_CACHE = {}
FLAT_CACHE = {}
SAME_CACHE = {}
EQUIV_CACHE = {}
DEPENDS_CACHE = {}
POLY_CACHE = {}

ALL_CACHES = (
    SIG_CACHE, SHOW_CACHE, SPLIT_COEFF_CACHE,
    FLAT_CACHE, SAME_CACHE, EQUIV_CACHE, DEPENDS_CACHE,
    POLY_CACHE
)


def clear_all_caches():
    """Clear all caches to free memory."""
    i = 0
    while i < len(ALL_CACHES):
        ALL_CACHES[i].clear()
        i += 1


def cache_store(cache, key, value, limit):
    """Store value in cache with bounded incremental eviction."""
    if len(cache) >= limit:
        drop = 1
        if limit >= 32:
            drop = limit // 8
        i = 0
        while i < drop and cache:
            try:
                first_key = next(iter(cache))
            except StopIteration:
                break
            try:
                del cache[first_key]
            except KeyError:
                break
            i += 1
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
    """Called at start of user action - reset caches for a fresh run."""
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


def solve_linear_system(mat, rhs):
    n = len(rhs)
    a = [list(row) for row in mat]
    b = list(rhs)
    col = 0
    while col < n:
        pivot = col
        while pivot < n and is_zero(a[pivot][col]):
            pivot += 1
        if pivot == n:
            return
        if pivot != col:
            a[col], a[pivot] = a[pivot], a[col]
            b[col], b[pivot] = b[pivot], b[col]
        pivot_val = a[col][col]
        j = col
        while j < n:
            a[col][j] = divq(a[col][j], pivot_val)
            j += 1
        b[col] = divq(b[col], pivot_val)
        row = 0
        while row < n:
            if row != col and not is_zero(a[row][col]):
                factor = a[row][col]
                j = col
                while j < n:
                    a[row][j] = subq(a[row][j], mulq(factor, a[col][j]))
                    j += 1
                b[row] = subq(b[row], mulq(factor, b[col]))
            row += 1
        col += 1
    return b


def int_pow(a, n):
    if n >= 0:
        return num(a[1] ** n, a[2] ** n)
    return num(a[2] ** (-n), a[1] ** (-n))


def flat(node, kind):
    """Flatten nested add/mul nodes. Cached for performance."""
    key = (node, kind)
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
    key = node
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
    key = (a, b)
    cached = SAME_CACHE.get(key)
    if cached is not None:
        return cached
    
    # Compare signatures
    result = sig(a) == sig(b)
    cache_store(SAME_CACHE, key, result, CACHE_LIMIT_LARGE)
    cache_store(SAME_CACHE, (b, a), result, CACHE_LIMIT_LARGE)
    return result


def split_coeff(node):
    """Split coefficient and base from a term. Cached for performance."""
    # Check cache
    key = node
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


def depends_on(node, var_name):
    key = (node, var_name)
    cached = DEPENDS_CACHE.get(key)
    if cached is not None:
        return cached
    kind = node[0]
    if kind == 'sym':
        result = node[1] == var_name
    elif kind in ('num', 'const'):
        result = False
    elif kind == 'fn':
        result = depends_on(node[2], var_name)
    elif kind == 'pow':
        result = depends_on(node[1], var_name) or depends_on(node[2], var_name)
    elif kind == 'div':
        result = depends_on(node[1], var_name) or depends_on(node[2], var_name)
    elif kind in ('add', 'mul'):
        result = False
        i = 0
        while i < len(node[1]):
            if depends_on(node[1][i], var_name):
                result = True
                break
            i += 1
    else:
        result = False
    cache_store(DEPENDS_CACHE, key, result, CACHE_LIMIT_MEDIUM)
    return result


def collect_symbol_names(node, names):
    kind = node[0]
    if kind == 'sym':
        names.add(node[1])
        return
    if kind == 'fn':
        collect_symbol_names(node[2], names)
        return
    if kind == 'pow':
        collect_symbol_names(node[1], names)
        collect_symbol_names(node[2], names)
        return
    if kind == 'div':
        collect_symbol_names(node[1], names)
        collect_symbol_names(node[2], names)
        return
    if kind in ('add', 'mul'):
        i = 0
        while i < len(node[1]):
            collect_symbol_names(node[1][i], names)
            i += 1


def choose_primary_var(node, preferred='x'):
    names = set()
    collect_symbol_names(node, names)
    if preferred in names:
        return preferred
    preferred_order = ('x', 'y', 'z', 't', 'u', 'v')
    i = 0
    while i < len(preferred_order):
        if preferred_order[i] in names:
            return preferred_order[i]
        i += 1
    lowercase = []
    uppercase = []
    for name in names:
        if len(name) == 1 and 'a' <= name <= 'z':
            lowercase.append(name)
        elif len(name) == 1 and 'A' <= name <= 'Z':
            uppercase.append(name)
    if len(lowercase) == 1:
        return lowercase[0]
    if lowercase:
        lowercase.sort()
        return lowercase[0]
    if len(uppercase) == 1:
        return uppercase[0]
    return None


def merge_poly_coeffs(left, right):
    out = {}
    for degree in left:
        out[degree] = left[degree]
    for degree in right:
        if degree in out:
            out[degree] = sim(add([out[degree], right[degree]]))
        else:
            out[degree] = right[degree]
    dead = []
    for degree in out:
        if is_zero(out[degree]):
            dead.append(degree)
    i = 0
    while i < len(dead):
        del out[dead[i]]
        i += 1
    if not out:
        out[0] = num(0)
    return out


def mul_poly_coeffs(left, right):
    out = {}
    for degree_left in left:
        for degree_right in right:
            degree = degree_left + degree_right
            term = sim(mul([left[degree_left], right[degree_right]]))
            if degree in out:
                out[degree] = sim(add([out[degree], term]))
            else:
                out[degree] = term
    dead = []
    for degree in out:
        if is_zero(out[degree]):
            dead.append(degree)
    i = 0
    while i < len(dead):
        del out[dead[i]]
        i += 1
    if not out:
        out[0] = num(0)
    return out


def poly_coeffs(node, var_name):
    key = (node, var_name)
    cached = POLY_CACHE.get(key)
    if cached is not None:
        return cached
    var_node = sym(var_name)
    kind = node[0]
    if kind in ('num', 'const'):
        result = {0: node}
    elif kind == 'sym':
        if same(node, var_node):
            result = {1: num(1)}
        else:
            result = {0: node}
    elif kind == 'fn':
        if depends_on(node, var_name):
            result = None
        else:
            result = {0: node}
    elif kind == 'add':
        result = {}
        i = 0
        while i < len(node[1]):
            part = poly_coeffs(node[1][i], var_name)
            if part is None:
                result = None
                break
            result = merge_poly_coeffs(result, part) if result else part
            i += 1
    elif kind == 'mul':
        result = {0: num(1)}
        items = flat(node, 'mul')
        i = 0
        while i < len(items):
            part = poly_coeffs(items[i], var_name)
            if part is None:
                result = None
                break
            result = mul_poly_coeffs(result, part)
            i += 1
    elif kind == 'div':
        if depends_on(node[2], var_name):
            result = None
        else:
            top = poly_coeffs(node[1], var_name)
            if top is None:
                result = None
            else:
                result = {}
                for degree in top:
                    result[degree] = sim(div(top[degree], node[2]))
    elif kind == 'pow':
        if depends_on(node[2], var_name):
            result = None
        elif is_int_num(node[2]) and node[2][1] >= 0:
            base_poly = poly_coeffs(node[1], var_name)
            if base_poly is None:
                result = None
            else:
                result = {0: num(1)}
                n = node[2][1]
                i = 0
                while i < n:
                    result = mul_poly_coeffs(result, base_poly)
                    i += 1
        elif depends_on(node[1], var_name):
            result = None
        else:
            result = {0: node}
    else:
        result = None
    cache_store(POLY_CACHE, key, result, CACHE_LIMIT_MEDIUM)
    return result


def make_poly_term(coeff, var_name, degree):
    if is_zero(coeff):
        return num(0)
    if degree == 0:
        return coeff
    base = sym(var_name)
    if degree > 1:
        base = power(base, num(degree))
    if is_one(coeff):
        return base
    if is_minus_one(coeff):
        return neg(base)
    return sim(mul([coeff, base]))


def rebuild_polynomial(coeffs, var_name, ascending=False):
    degrees = list(coeffs.keys())
    if not degrees:
        return num(0)
    degrees.sort(reverse=not ascending)
    parts = []
    i = 0
    while i < len(degrees):
        degree = degrees[i]
        coeff = coeffs[degree]
        if not is_zero(coeff):
            parts.append(make_poly_term(coeff, var_name, degree))
        i += 1
    if not parts:
        return num(0)
    if len(parts) == 1:
        return parts[0]
    return 'add', tuple(parts)


def polynomial_degree(coeffs):
    if not coeffs:
        return -1
    degrees = list(coeffs.keys())
    degrees.sort()
    return degrees[-1]


def polynomial_coeff(coeffs, degree):
    if degree in coeffs:
        return coeffs[degree]
    return num(0)


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


def exp_from_log_combo(arg):
    arg = sim(arg)
    if arg[0] == 'fn' and arg[1] == 'log':
        return arg[2]
    if arg[0] == 'mul':
        coeff, rest = split_coeff(arg)
        if rest[0] == 'fn' and rest[1] == 'log' and is_num(coeff):
            return power(rest[2], coeff)
        return None
    if arg[0] == 'add':
        items = flat(arg, 'add')
        factors = []
        i = 0
        while i < len(items):
            part = exp_from_log_combo(items[i])
            if part is None:
                return None
            factors.append(part)
            i += 1
        return sim(make_mul(factors))
    return None


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
    if name == 'exp' and arg[0] == 'fn' and arg[1] == 'log':
        return arg[2]
    if name == 'exp':
        log_combo = exp_from_log_combo(arg)
        if log_combo is not None:
            return log_combo
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


def reorder_expanded_terms_ascending(node, var_name=None):
    """For polynomial sums, reorder terms into ascending powers of the chosen variable."""
    node = sim(node)
    if var_name is None:
        var_name = choose_primary_var(node)
    if var_name is None:
        return node
    coeffs = poly_coeffs(node, var_name)
    if coeffs is None:
        return node
    return rebuild_polynomial(coeffs, var_name, ascending=True)


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
        expanded = sim(make_add(result_terms))
        expanded = reorder_expanded_terms_ascending(expanded)
        return expanded, steps
    
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
    var_name = choose_primary_var(node)
    if var_name is None:
        return None, None
    coeffs = poly_coeffs(node, var_name)
    if coeffs is None or polynomial_degree(coeffs) != 2:
        return None, None
    a = polynomial_coeff(coeffs, 2)
    b = polynomial_coeff(coeffs, 1)
    c_val = polynomial_coeff(coeffs, 0)
    if is_zero(a):
        return None, None
    var_node = sym(var_name)
    shift = sim(div(b, mul([num(2), a])))
    square_part = power(add([var_node, shift]), num(2))
    if not is_one(a):
        square_part = sim(mul([a, square_part]))
    adjustment = sim(sub(c_val, div(power(b, num(2)), mul([num(4), a]))))
    if is_zero(adjustment):
        result = square_part
        note = "Perfect square: " + show(square_part)
    else:
        result = sim(add([square_part, adjustment]))
        note = "Complete square: " + show(result)
    return result, note


def solve_equation(node):
    node = sim(node)
    var_name = choose_primary_var(node)
    if var_name is None:
        if is_zero(node):
            return None, "All real values", "Identity (infinite solutions)"
        return None, None, None
    coeffs = poly_coeffs(node, var_name)
    if coeffs is None:
        return None, None, None
    degree = polynomial_degree(coeffs)
    a = polynomial_coeff(coeffs, 2)
    b = polynomial_coeff(coeffs, 1)
    c = polynomial_coeff(coeffs, 0)
    label = var_name + " = "
    if degree == 0:
        if is_zero(c):
            return None, "All real " + var_name, "Identity (infinite solutions)"
        return None, None, "No solution (contradiction)"
    if is_zero(a) and is_zero(b):
        return None, None, "No solution (contradiction)"
    if is_zero(a):
        if is_zero(b):
            return None, None, "No solution"
        root = sim(neg(div(c, b)))
        return label + show(root), None, None
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
        return None, label + "(-b ± sqrt(b^2-4ac)) / 2a", "Quadratic formula"
    neg_b = sim(neg(b))
    two_a = sim(mul([num(2), a]))
    root1 = sim(div(add([neg_b, sqrt_disc]), two_a))
    root2 = sim(div(sub(neg_b, sqrt_disc), two_a))
    if same(root1, root2):
        return label + show(root1), None, None
    return label + show(root1) + " or " + var_name + " = " + show(root2), None, None

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
        return ('pow', substitute(node[1], old, new), substitute(node[2], old, new))
    if node[0] == 'fn':
        return ('fn', node[1], substitute(node[2], old, new))
    if node[0] == 'div':
        return ('div', substitute(node[1], old, new), substitute(node[2], old, new))
    return node


def make_add_raw(parts):
    if len(parts) == 0:
        return num(0)
    if len(parts) == 1:
        return parts[0]
    return ('add', tuple(parts))


def make_mul_raw(parts):
    if len(parts) == 0:
        return num(1)
    if len(parts) == 1:
        return parts[0]
    return ('mul', tuple(parts))


def substitute_keep_form(node, old, new):
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
            items.append(substitute_keep_form(node[1][i], old, new))
            i += 1
        return make_add_raw(items)
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(substitute_keep_form(node[1][i], old, new))
            i += 1
        return make_mul_raw(items)
    if node[0] == 'pow':
        return ('pow', substitute_keep_form(node[1], old, new), substitute_keep_form(node[2], old, new))
    if node[0] == 'fn':
        return ('fn', node[1], substitute_keep_form(node[2], old, new))
    if node[0] == 'div':
        return ('div', substitute_keep_form(node[1], old, new), substitute_keep_form(node[2], old, new))
    return node


def normalise_negative_power_div(node):
    node = sim(node)
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return node
    if kind == 'fn':
        return fn(node[1], normalise_negative_power_div(node[2]))
    if kind == 'pow':
        base = normalise_negative_power_div(node[1])
        exp = normalise_negative_power_div(node[2])
        if is_int_num(exp) and exp[1] < 0:
            return div(num(1), power(base, num(-exp[1])))
        return power(base, exp)
    if kind == 'div':
        return div(normalise_negative_power_div(node[1]), normalise_negative_power_div(node[2]))
    if kind == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(normalise_negative_power_div(node[1][i]))
            i += 1
        return make_add(items)
    if kind == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(normalise_negative_power_div(node[1][i]))
            i += 1
        top = []
        bot = []
        i = 0
        while i < len(items):
            item = items[i]
            if item[0] == 'div':
                top.append(item[1])
                bot.append(item[2])
            else:
                top.append(item)
            i += 1
        top_node = make_mul(top) if top else num(1)
        bot_node = make_mul(bot) if bot else num(1)
        if is_one(bot_node):
            return top_node
        return div(top_node, bot_node)
    return node


def equation_text(lhs, rhs):
    return show(lhs) + ' = ' + show(rhs)


def tree_size(node):
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return 1
    if kind == 'fn':
        return 1 + tree_size(node[2])
    if kind in ('pow', 'div'):
        return 1 + tree_size(node[1]) + tree_size(node[2])
    if kind in ('add', 'mul'):
        total = 1
        i = 0
        while i < len(node[1]):
            total += tree_size(node[1][i])
            i += 1
        return total
    return 1


def extract_square_base(node):
    node = sim(node)
    if is_num(node):
        return sqrt_num(node)
    if node[0] == 'pow' and same(node[2], num(2)):
        return node[1]
    if node[0] == 'mul':
        items = flat(node, 'mul')
        bases = []
        i = 0
        while i < len(items):
            base = extract_square_base(items[i])
            if base is None:
                return None
            bases.append(base)
            i += 1
        return sim(make_mul(bases))
    if node[0] == 'div':
        top = extract_square_base(node[1])
        bot = extract_square_base(node[2])
        if top is None or bot is None:
            return None
        return sim(div(top, bot))
    return None


def factor_difference_of_squares(node):
    node = sim(node)
    items = flat(node, 'add') if node[0] == 'add' else [node]
    if len(items) != 2:
        return None
    positive = None
    negative = None
    i = 0
    while i < len(items):
        coeff, rest = split_coeff(items[i])
        magnitude = rest
        if not is_one(coeff) and not is_minus_one(coeff):
            magnitude = sim(mul([num(abs(coeff[1]), coeff[2]), rest]))
        if coeff[1] < 0:
            negative = magnitude
        else:
            positive = magnitude
        i += 1
    if positive is None or negative is None:
        return None
    left = extract_square_base(positive)
    right = extract_square_base(negative)
    if left is None or right is None:
        return None
    result = sim(mul([add([left, neg(right)]), add([left, right])]))
    if equivalent(result, node):
        return result, 'Factor as a difference of squares'
    return None


def factor_quadratic_rational(node, var_name=None):
    node = sim(node)
    if var_name is None:
        var_name = choose_primary_var(node)
    if var_name is None:
        return None
    coeffs = poly_coeffs(node, var_name)
    if coeffs is None or polynomial_degree(coeffs) != 2:
        return None
    a = polynomial_coeff(coeffs, 2)
    b = polynomial_coeff(coeffs, 1)
    c = polynomial_coeff(coeffs, 0)
    if not is_num(a) or not is_num(b) or not is_num(c) or is_zero(a):
        return None
    disc = sim(sub(power(b, num(2)), mul([num(4), a, c])))
    sqrt_disc = sqrt_num(disc)
    if sqrt_disc is None:
        return None
    two_a = sim(mul([num(2), a]))
    neg_b = sim(neg(b))
    root1 = sim(div(add([neg_b, sqrt_disc]), two_a))
    root2 = sim(div(sub(neg_b, sqrt_disc), two_a))
    factor1 = add([sym(var_name), neg(root1)])
    factor2 = add([sym(var_name), neg(root2)])
    if same(root1, root2):
        core = power(factor1, num(2))
    else:
        core = mul([factor1, factor2])
    if not is_one(a):
        core = mul([a, core])
    core = sim(core)
    if equivalent(core, node):
        return core, 'Factor the quadratic'
    return None


def factor_expression(node, preferred_var=None):
    diff_sq = factor_difference_of_squares(node)
    if diff_sq is not None:
        return diff_sq
    quad = factor_quadratic_rational(node, preferred_var)
    if quad is not None:
        return quad
    return None


def factor_list(node, preferred_var=None):
    factored = factor_expression(node, preferred_var)
    if factored is not None:
        node = factored[0]
    if node[0] == 'mul':
        return list(flat(node, 'mul'))
    return [node]


def cancel_fraction_factor(node, preferred_var=None):
    node = sim(node)
    if node[0] != 'div':
        return None
    top_factors = factor_list(node[1], preferred_var)
    bot_factors = factor_list(node[2], preferred_var)
    used_bot = [False] * len(bot_factors)
    top_keep = []
    cancelled = False
    i = 0
    while i < len(top_factors):
        matched = False
        j = 0
        while j < len(bot_factors):
            if not used_bot[j] and equivalent(top_factors[i], bot_factors[j]):
                used_bot[j] = True
                matched = True
                cancelled = True
                break
            j += 1
        if not matched:
            top_keep.append(top_factors[i])
        i += 1
    if not cancelled:
        return None
    bot_keep = []
    i = 0
    while i < len(bot_factors):
        if not used_bot[i]:
            bot_keep.append(bot_factors[i])
        i += 1
    top_node = make_mul(top_keep) if top_keep else num(1)
    bot_node = make_mul(bot_keep) if bot_keep else num(1)
    result = top_node if is_one(bot_node) else sim(div(top_node, bot_node))
    if equivalent(mul([result, node[2]]), node[1]):
        return result, 'Cancel the common factor'
    return None


def choose_unused_symbol(node, extra_nodes=None):
    names = set()
    collect_symbol_names(node, names)
    if extra_nodes is not None:
        i = 0
        while i < len(extra_nodes):
            collect_symbol_names(extra_nodes[i], names)
            i += 1
    for candidate in ('T', 'U', 'V', 'W', 'Q', 'R', 'S'):
        if candidate not in names:
            return candidate
    index = 1
    while True:
        candidate = '_T' + str(index)
        if candidate not in names:
            return candidate
        index += 1


def rewrite_polynomial_in_linear_term(node, term, var_name):
    node = sim(node)
    term = sim(term)
    if var_name is None:
        return None
    coeffs = poly_coeffs(node, var_name)
    if coeffs is None:
        return None
    linear = match_linear_in_var(term, var_name)
    if linear is None:
        return None
    a, b = linear
    if is_zero(a):
        return None
    temp_name = choose_unused_symbol(node, [term])
    temp = sym(temp_name)
    inv = sim(div(sub(temp, b), a))
    rewritten = substitute(node, sym(var_name), inv)
    rewritten = canonical_compare_form(rewritten)
    final_expr = substitute_keep_form(rewritten, temp, term)
    if equivalent(final_expr, node) and not same(final_expr, node):
        return final_expr, 'Write the expression in powers of ' + show(term)
    return None


def match_shifted_reciprocal(node, var_name):
    shift = num(0)
    reciprocal = None
    items = flat(node, 'add') if node[0] == 'add' else [node]
    i = 0
    while i < len(items):
        item = items[i]
        numerator = None
        denominator = None
        if item[0] == 'div':
            numerator = item[1]
            denominator = item[2]
        else:
            coeff, rest = split_coeff(item)
            if rest[0] == 'div' and is_one(rest[1]):
                numerator = coeff
                denominator = rest[2]
        if numerator is not None and denominator is not None and not depends_on(numerator, var_name):
            coeffs = poly_coeffs(denominator, var_name)
            if coeffs is not None and polynomial_degree(coeffs) == 1:
                if reciprocal is not None:
                    return None
                reciprocal = (
                    numerator,
                    polynomial_coeff(coeffs, 1),
                    polynomial_coeff(coeffs, 0)
                )
                i += 1
                continue
        if depends_on(item, var_name):
            return None
        shift = sim(add([shift, item]))
        i += 1
    if reciprocal is None:
        return None
    return reciprocal[0], reciprocal[1], reciprocal[2], shift


def split_outer_shift(node, var_name):
    items = flat(node, 'add') if node[0] == 'add' else [node]
    dependent = []
    shift = num(0)
    i = 0
    while i < len(items):
        if depends_on(items[i], var_name):
            dependent.append(items[i])
        else:
            shift = sim(add([shift, items[i]]))
        i += 1
    if not dependent:
        return None, shift
    if len(dependent) == 1:
        return dependent[0], shift
    return sim(make_add(dependent)), shift


def match_linear_in_var(node, var_name):
    coeffs = poly_coeffs(node, var_name)
    if coeffs is None or polynomial_degree(coeffs) != 1:
        return None
    a = polynomial_coeff(coeffs, 1)
    b = polynomial_coeff(coeffs, 0)
    if is_zero(a):
        return None
    return a, b


def match_reciprocal_in_var(node, var_name):
    if node[0] != 'div' or depends_on(node[1], var_name):
        return None
    linear = match_linear_in_var(node[2], var_name)
    if linear is None:
        return None
    return node[1], linear[0], linear[1]


def match_linear_fractional_in_var(node, var_name):
    if node[0] != 'div':
        return None
    top = poly_coeffs(node[1], var_name)
    bot = poly_coeffs(node[2], var_name)
    if top is None or bot is None:
        return None
    if polynomial_degree(top) > 1 or polynomial_degree(bot) != 1:
        return None
    a = polynomial_coeff(top, 1)
    b = polynomial_coeff(top, 0)
    c = polynomial_coeff(bot, 1)
    d = polynomial_coeff(bot, 0)
    if is_zero(c) and is_zero(d):
        return None
    return a, b, c, d


def match_sqrt_linear_in_var(node, var_name):
    if node[0] != 'fn' or node[1] != 'sqrt':
        return None
    return match_linear_in_var(node[2], var_name)


def match_power_linear_in_var(node, var_name):
    if node[0] != 'pow' or not is_int_num(node[2]) or node[2][1] <= 0:
        return None
    linear = match_linear_in_var(node[1], var_name)
    if linear is None:
        return None
    return linear[0], linear[1], node[2][1]


def match_exp_linear_in_var(node, var_name):
    if node[0] == 'fn' and node[1] == 'exp':
        return match_linear_in_var(node[2], var_name)
    if node[0] == 'pow' and same(node[1], E()):
        return match_linear_in_var(node[2], var_name)
    return None


def match_log_linear_in_var(node, var_name):
    if node[0] == 'fn' and node[1] == 'log':
        return match_linear_in_var(node[2], var_name)
    return None


def solve_inverse_core(lhs_expr, rhs_node, steps):
    linear = match_linear_in_var(rhs_node, 'y')
    if linear is not None:
        a, b = linear
        numerator = sim(sub(lhs_expr, b))
        inv = sim(div(numerator, a))
        if not is_zero(b):
            steps.append(show(numerator) + " = " + show(mul([a, sym('y')])))
        steps.append("y = " + show(inv))
        return inv

    reciprocal = match_reciprocal_in_var(rhs_node, 'y')
    if reciprocal is not None:
        a, b, c = reciprocal
        rhs = sim(div(a, lhs_expr))
        numerator = sim(sub(rhs, c))
        inv = sim(div(numerator, b))
        steps.append(show(add([mul([b, sym('y')]), c])) + " = " + show(rhs))
        steps.append("y = " + show(inv))
        return inv

    frac = match_linear_fractional_in_var(rhs_node, 'y')
    if frac is not None:
        a, b, c, d = frac
        left_coeff = sim(sub(mul([lhs_expr, c]), a))
        right_side = sim(sub(b, mul([lhs_expr, d])))
        inv = sim(div(right_side, left_coeff))
        steps.append(show(mul([lhs_expr, add([mul([c, sym('y')]), d])])) + " = " + show(add([mul([a, sym('y')]), b])))
        steps.append(show(mul([left_coeff, sym('y')])) + " = " + show(right_side))
        steps.append("y = " + show(inv))
        return inv

    sqrt_linear = match_sqrt_linear_in_var(rhs_node, 'y')
    if sqrt_linear is not None:
        a, b = sqrt_linear
        squared = sim(power(lhs_expr, num(2)))
        numerator = sim(sub(squared, b))
        inv = sim(div(numerator, a))
        steps.append(show(squared) + " = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    power_linear = match_power_linear_in_var(rhs_node, 'y')
    if power_linear is not None:
        a, b, n = power_linear
        if n % 2 == 0:
            steps.append("No inverse on all real x for an even power")
            return None
        rooted = sim(power(lhs_expr, num(1, n)))
        numerator = sim(sub(rooted, b))
        inv = sim(div(numerator, a))
        steps.append(show(rooted) + " = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    exp_linear = match_exp_linear_in_var(rhs_node, 'y')
    if exp_linear is not None:
        a, b = exp_linear
        logged = sim(fn('log', lhs_expr))
        numerator = sim(sub(logged, b))
        inv = sim(div(numerator, a))
        steps.append("ln(" + show(lhs_expr) + ") = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    log_linear = match_log_linear_in_var(rhs_node, 'y')
    if log_linear is not None:
        a, b = log_linear
        expo = sim(fn('exp', lhs_expr))
        numerator = sim(sub(expo, b))
        inv = sim(div(numerator, a))
        steps.append(show(expo) + " = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    return None


def inverse_function(f_text, var='x'):
    try:
        f = normalise_negative_power_div(sim(parse(f_text)))
        steps = []
        steps.append("f(x) = " + f_text)
        f_y = substitute(f, sym(var), sym('y'))
        steps.append("Let y = " + show(f_y))
        steps.append("Swap: x = " + show(f_y))
        inv = solve_inverse_core(sym('x'), f_y, steps)
        if inv is not None:
            steps.append("f^-1(x) = " + show(inv))
            return show(inv), steps

        dependent, shift = split_outer_shift(f_y, 'y')
        if dependent is not None and not is_zero(shift):
            lhs_expr = sim(sub(sym('x'), shift))
            steps.append(show(lhs_expr) + " = " + show(dependent))
            inv = solve_inverse_core(lhs_expr, dependent, steps)
            if inv is not None:
                steps.append("f^-1(x) = " + show(inv))
                return show(inv), steps

        if steps and steps[-1].startswith("No inverse on all real x"):
            return None, steps

        steps.append("No inverse method")
        return None, steps
    except Exception as err:
        return None, ["Err: " + str(err)]

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
            if base[0] == 'mul' and exp[1] >= 0:
                base_items = flat(base, 'mul')
                pieces = []
                i = 0
                while i < len(base_items):
                    pieces.append(power(base_items[i], exp))
                    i += 1
                return sim(make_mul(pieces))
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
    key = (node, parent)
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
        left = show(node[1], 0)
        right = show(node[2], 0)
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
            low = word.lower()
            if low in FUNC_ALIASES or low in FUNC_NAMES or low in ('pi', 'e'):
                toks.append(word)
            elif word.isalpha() and len(word) > 1:
                k = 0
                while k < len(word):
                    toks.append(word[k])
                    k += 1
            else:
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
                if cur() == '^' or cur() == '**':
                    eat(cur())
                    if cur() == '(':
                        eat('(')
                        exp = expr()
                        eat(')')
                    else:
                        exp = atom()
                    out = atom()
                    return power(fn(actual_name, out), exp)
                if cur() == '(' and actual_name in FUNC_NAMES:
                    eat('(')
                    out = expr()
                    eat(')')
                    return fn(actual_name, out)
                if starts_implicit(cur()) and actual_name in FUNC_NAMES:
                    return fn(actual_name, atom())
            if low in FUNC_NAMES:
                if cur() == '^' or cur() == '**':
                    eat(cur())
                    if cur() == '(':
                        eat('(')
                        exp = expr()
                        eat(')')
                    else:
                        exp = atom()
                    out = atom()
                    return power(fn(low, out), exp)
                if cur() == '(':
                    eat('(')
                    out = expr()
                    eat(')')
                    return fn(low, out)
                if starts_implicit(cur()):
                    return fn(low, atom())
            if low in ('pi', 'e'):
                return PI() if low == 'pi' else E()
            if len(t) == 1:
                return sym(t)
            if t.isalpha():
                parts = []
                i = 0
                while i < len(t):
                    parts.append(sym(t[i]))
                    i += 1
                return make_mul(parts)
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
    if a is b or a == b:
        return True
    key = (a, b)
    cached = EQUIV_CACHE.get(key)
    if cached is not None:
        return cached
    if sig(a) == sig(b):
        cache_store(EQUIV_CACHE, key, True, CACHE_LIMIT_MEDIUM)
        cache_store(EQUIV_CACHE, (b, a), True, CACHE_LIMIT_MEDIUM)
        return True
    diff = sim(add([a, neg(b)]))
    if is_zero(diff):
        cache_store(EQUIV_CACHE, key, True, CACHE_LIMIT_MEDIUM)
        cache_store(EQUIV_CACHE, (b, a), True, CACHE_LIMIT_MEDIUM)
        return True
    diff = canonical_compare_form(diff)
    result = is_zero(diff)
    cache_store(EQUIV_CACHE, key, result, CACHE_LIMIT_MEDIUM)
    cache_store(EQUIV_CACHE, (b, a), result, CACHE_LIMIT_MEDIUM)
    return result


def canonical_compare_form(node):
    current = sim(node)
    passes = 0
    while passes < 4:
        previous = current
        current = expand_binomial_pow(current)
        current = sim(current)
        current = expand_pow_sqrt(current)
        current = sim(current)
        current = expand_mul_distribute(current)
        current = sim(current)
        var_name = choose_primary_var(current)
        if var_name is not None:
            coeffs = poly_coeffs(current, var_name)
            if coeffs is not None:
                current = rebuild_polynomial(coeffs, var_name, ascending=True)
                current = sim(current)
        current = combine_fractions(current)
        current = sim(current)
        cancelled = cancel_fraction_factor(current)
        if cancelled is not None:
            current = sim(cancelled[0])
        current = simplify_trig_identity(current)
        current = sim(current)
        if current == previous:
            break
        passes += 1
    return current


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

def expand_all_pow_sqrt(node):
    return canonical_compare_form(node)


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


def trig_identity_description(source, target):
    source = sim(source)
    target = sim(target)

    if same(source, add([num(1), neg(fn('cos', mul([num(2), sym('x')])))])):
        if same(target, mul([num(2), power(fn('sin', sym('x')), num(2))])):
            return 'Use 1-cos(2A) = 2sin^2(A)'
    if same(source, add([num(1), fn('cos', mul([num(2), sym('x')]))])):
        if same(target, mul([num(2), power(fn('cos', sym('x')), num(2))])):
            return 'Use 1+cos(2A) = 2cos^2(A)'

    items = flat(source, 'add') if source[0] == 'add' else [source]
    coeff_map = {}
    i = 0
    while i < len(items):
        coeff, rest = split_coeff(items[i])
        coeff_map[sig(rest)] = coeff
        i += 1

    def coeff_of(rest):
        coeff = coeff_map.get(sig(rest))
        if coeff is None:
            return num(0)
        return coeff

    i = 0
    while i < len(items):
        sin_arg = is_pow_of_fn(items[i], 'sin', num(2))
        if sin_arg is not None:
            cos_sq = power(fn('cos', sin_arg), num(2))
            if is_one(coeff_of(items[i])) and is_one(coeff_of(cos_sq)) and is_one(target):
                return "Use sin^2(A) + cos^2(A) = 1"
        cos_arg = is_pow_of_fn(items[i], 'cos', num(2))
        if cos_arg is not None:
            sin_sq = power(fn('sin', cos_arg), num(2))
            if is_one(coeff_of(items[i])) and is_one(coeff_of(sin_sq)) and is_one(target):
                return "Use sin^2(A) + cos^2(A) = 1"
        tan_arg = is_pow_of_fn(items[i], 'tan', num(2))
        if tan_arg is not None:
            sec_sq = power(fn('sec', tan_arg), num(2))
            if is_one(coeff_of(items[i])) and is_one(coeff_of(num(1))) and same(target, sec_sq):
                return "Use 1 + tan^2(A) = sec^2(A)"
            if is_minus_one(coeff_of(items[i])) and is_one(coeff_of(sec_sq)) and is_one(target):
                return "Use sec^2(A) - tan^2(A) = 1"
        cot_arg = is_pow_of_fn(items[i], 'cot', num(2))
        if cot_arg is not None:
            cosec_sq = power(fn('cosec', cot_arg), num(2))
            if is_one(coeff_of(items[i])) and is_one(coeff_of(num(1))) and same(target, cosec_sq):
                return "Use 1 + cot^2(A) = cosec^2(A)"
            if is_minus_one(coeff_of(items[i])) and is_one(coeff_of(cosec_sq)) and is_one(target):
                return "Use cosec^2(A) - cot^2(A) = 1"
        sec_arg = is_pow_of_fn(items[i], 'sec', num(2))
        if sec_arg is not None:
            tan_sq = power(fn('tan', sec_arg), num(2))
            if is_one(coeff_of(items[i])) and is_minus_one(coeff_of(tan_sq)) and is_one(target):
                return "Use sec^2(A) - tan^2(A) = 1"
        cosec_arg = is_pow_of_fn(items[i], 'cosec', num(2))
        if cosec_arg is not None:
            cot_sq = power(fn('cot', cosec_arg), num(2))
            if is_one(coeff_of(items[i])) and is_minus_one(coeff_of(cot_sq)) and is_one(target):
                return "Use cosec^2(A) - cot^2(A) = 1"
        i += 1
    return None


def factor_target_description(source, target):
    if target[0] != 'mul' or same(source, target):
        return None
    var_name = choose_primary_var(source)
    target_items = flat(target, 'mul')
    constants = []
    dependents = []
    i = 0
    while i < len(target_items):
        if var_name is not None and depends_on(target_items[i], var_name):
            dependents.append(target_items[i])
        else:
            constants.append(target_items[i])
        i += 1
    if not constants or not dependents:
        return None
    constant_part = constants[0] if len(constants) == 1 else make_mul(constants)
    return "Factor out " + show(constant_part)


def reverse_binomial_description(target):
    if target[0] == 'pow' and target[1][0] == 'add' and is_int_num(target[2]) and target[2][1] >= 2:
        return "Recognize a binomial pattern"
    return None


def target_route_steps(source, target):
    cancelled = cancel_fraction_factor(source)
    if cancelled is not None and equivalent(cancelled[0], target):
        desc = cancelled[1]
        if same(cancelled[0], target):
            return [(desc, target)]
        return [(desc, cancelled[0]), ("Rewrite to target form", target)]

    factored = factor_expression(source)
    if factored is not None and equivalent(factored[0], target):
        desc = factored[1]
        if same(factored[0], target):
            return [(desc, target)]
        return [(desc, factored[0]), ("Rewrite to target form", target)]

    candidate, note = complete_the_square(source)
    if candidate is not None and equivalent(candidate, target):
        desc = note if note else "Complete the square"
        if same(candidate, target):
            return [(desc, target)]
        return [(desc, candidate), ("Rewrite to target form", target)]

    fitted = solve_target_coefficients(source, target)
    if fitted is not None:
        coeffs, fitted_target = fitted
        names = sorted(coeffs.keys())
        parts = []
        i = 0
        while i < len(names):
            parts.append(names[i] + ' = ' + show(coeffs[names[i]]))
            i += 1
        if len(parts) > 0:
            if same(fitted_target, target):
                return [("Match coefficients: " + ', '.join(parts), target)]
            return [("Match coefficients: " + ', '.join(parts), fitted_target), ("Rewrite to target form", target)]

    trig_desc = trig_identity_description(source, target)
    if trig_desc is not None:
        return [(trig_desc, target)]

    factor_desc = factor_target_description(source, target)
    if factor_desc is not None and equivalent(source, target):
        return [(factor_desc, target)]

    binomial_desc = reverse_binomial_description(target)
    if binomial_desc is not None and equivalent(source, target):
        return [(binomial_desc, target)]

    return None


def sorted_symbol_names(node):
    names = set()
    collect_symbol_names(node, names)
    return sorted(names)


def eval_with_values(node, values):
    out = node
    for name, value in values.items():
        out = substitute(out, sym(name), value)
    out = sim(out)
    return out if is_num(out) else None


def apply_values(node, values):
    out = node
    for name, value in values.items():
        out = substitute(out, sym(name), value)
    return sim(out)


def zero_placeholder_values(names):
    values = {}
    i = 0
    while i < len(names):
        values[names[i]] = num(0)
        i += 1
    return values


def extract_placeholder_linear_data(target, placeholders):
    data = {}
    const_part = num(0)
    zero_values = zero_placeholder_values(placeholders)
    terms = flat(target, 'add') if target[0] == 'add' else [target]
    i = 0
    while i < len(terms):
        term = sim(terms[i])
        names = set()
        collect_symbol_names(term, names)
        used = []
        j = 0
        while j < len(placeholders):
            if placeholders[j] in names:
                used.append(placeholders[j])
            j += 1
        if len(used) == 0:
            const_part = add([const_part, term])
            i += 1
            continue
        if len(used) != 1:
            return
        name = used[0]
        values0 = dict(zero_values)
        values1 = dict(zero_values)
        values2 = dict(zero_values)
        values1[name] = num(1)
        values2[name] = num(2)
        term0 = apply_values(term, values0)
        term1 = apply_values(term, values1)
        term2 = apply_values(term, values2)
        if not is_zero(term0) or not same(term2, mul([num(2), term1])):
            return
        if name in data:
            data[name] = add([data[name], term1])
        else:
            data[name] = term1
        i += 1
    return const_part, data


def choose_template_sample_values(source, basis_map, var_name, count):
    tried = [num(-3), num(-2), num(-1), num(1), num(2), num(3), num(4), num(1, 2), num(3, 2)]
    chosen = []
    rows = []
    rhs = []
    i = 0
    while i < len(tried) and len(chosen) < count:
        value = tried[i]
        row = []
        ok = True
        j = 0
        while j < len(basis_map):
            basis_val = eval_with_values(basis_map[j], {var_name: value})
            if basis_val is None:
                ok = False
                break
            row.append(basis_val)
            j += 1
        source_val = eval_with_values(source, {var_name: value})
        if source_val is None:
            ok = False
        if ok:
            chosen.append(value)
            rows.append(row)
            rhs.append(source_val)
        i += 1
    if len(chosen) < count:
        return
    return rows, rhs


def verify_template_match(source, target, var_name):
    checks = [num(-4), num(-3), num(-2), num(-1), num(1), num(2), num(3), num(4), num(1, 2), num(3, 2), num(5, 2)]
    seen = 0
    i = 0
    while i < len(checks):
        value = checks[i]
        left = eval_with_values(source, {var_name: value})
        right = eval_with_values(target, {var_name: value})
        if left is not None and right is not None:
            if not same(left, right):
                return False
            seen += 1
        i += 1
    return seen >= 4


def solve_target_coefficients(source, target_template):
    source_names = sorted_symbol_names(source)
    target_names = sorted_symbol_names(target_template)
    placeholders = []
    i = 0
    while i < len(target_names):
        if target_names[i] not in source_names:
            placeholders.append(target_names[i])
        i += 1
    if len(placeholders) == 0:
        return
    if len(source_names) != 1:
        return
    linear = extract_placeholder_linear_data(target_template, placeholders)
    if linear is None:
        return
    const_part, basis_by_name = linear
    basis_names = []
    basis_terms = []
    i = 0
    while i < len(placeholders):
        name = placeholders[i]
        basis_names.append(name)
        basis_terms.append(basis_by_name.get(name, num(0)))
        i += 1
    reduced_source = sim(sub(source, const_part))
    sample = choose_template_sample_values(reduced_source, basis_terms, source_names[0], len(basis_names))
    if sample is None:
        return
    mat, rhs = sample
    solved = solve_linear_system([list(row) for row in mat], list(rhs))
    if solved is None:
        return
    subst_map = {}
    i = 0
    while i < len(basis_names):
        subst_map[basis_names[i]] = solved[i]
        i += 1
    instantiated = target_template
    for name, value in subst_map.items():
        instantiated = substitute_keep_form(instantiated, sym(name), value)
    instantiated = sim(instantiated)
    if not verify_template_match(source, instantiated, source_names[0]):
        return
    return subst_map, instantiated


def rearrange_to_target(source, target_template):
    steps = []
    current = sim(source)
    steps.append((1, 'Original: ' + show(current), current))
    target = sim(target_template)
    fitted = solve_target_coefficients(current, target)
    if fitted is not None:
        coeffs, target = fitted
        names = sorted(coeffs.keys())
        parts = []
        i = 0
        while i < len(names):
            parts.append(names[i] + ' = ' + show(coeffs[names[i]]))
            i += 1
        if len(parts) > 0:
            steps.append((2, 'Match coefficients: ' + ', '.join(parts), target))
            steps.append((3, 'Rewrite to target form: ' + show(target), target))
            return steps, target
    if same(current, target):
        steps.append((len(steps) + 1, 'Already in target form: ' + show(target), target))
        return steps, target
    route_steps = target_route_steps(current, target)
    if route_steps is not None:
        i = 0
        while i < len(route_steps):
            desc, node = route_steps[i]
            text = desc
            if not desc.endswith(': ' + show(node)):
                text = desc + ': ' + show(node)
            steps.append((len(steps) + 1, text, node))
            i += 1
        return steps, target
    source_canon = canonical_compare_form(current)
    target_canon = canonical_compare_form(target)
    if not same(current, source_canon):
        steps.append((2, 'Simplify: ' + show(source_canon), source_canon))
    if same(source_canon, target):
        steps.append((len(steps) + 1, 'Match target form: ' + show(target), target))
        return steps, target
    if same(source_canon, target_canon):
        steps.append((len(steps) + 1, 'Rewrite to target form: ' + show(target), target))
        return steps, target
    if equivalent(current, target):
        steps.append((len(steps) + 1, 'Rewrite to target form: ' + show(target), target))
        return steps, target
    diff = canonical_compare_form(sim(sub(current, target)))
    steps.append((len(steps) + 1, 'Target is not equivalent to the source: difference = ' + show(diff), current))
    return steps, current


ALGEBRA_REWRITE_SEARCH_DEPTH = 6
ALGEBRA_REWRITE_BEAM_WIDTH = 8


def build_rewrite_allowed_info(expr, term_nodes):
    terms = []
    canons = []
    i = 0
    while i < len(term_nodes):
        term = sim(term_nodes[i])
        terms.append(term)
        canons.append(canonical_compare_form(term))
        i += 1
    names = set()
    collect_symbol_names(expr, names)
    i = 0
    while i < len(terms):
        collect_symbol_names(terms[i], names)
        i += 1
    var_name = choose_primary_var(expr)
    return {
        'terms': terms,
        'canons': canons,
        'var': var_name,
        'names': names,
    }


def rewrite_term_exact(node, info):
    node = sim(node)
    i = 0
    while i < len(info['terms']):
        if same(node, info['terms'][i]):
            return info['terms'][i]
        i += 1
    return None


def rewrite_term_equivalent(node, info):
    node = sim(node)
    canon = None
    i = 0
    while i < len(info['terms']):
        if canon is None:
            canon = canonical_compare_form(node)
        if same(canon, info['canons'][i]) or equivalent(node, info['terms'][i]):
            return info['terms'][i]
        i += 1
    return None


def rewrite_allowed_only(node, info):
    var_name = info['var']
    if var_name is None or not depends_on(node, var_name):
        return True
    if rewrite_term_exact(node, info) is not None:
        return True
    if node[0] == 'add' or node[0] == 'mul':
        i = 0
        while i < len(node[1]):
            if not rewrite_allowed_only(node[1][i], info):
                return False
            i += 1
        return True
    if node[0] == 'div':
        return rewrite_allowed_only(node[1], info) and rewrite_allowed_only(node[2], info)
    if node[0] == 'pow':
        if not rewrite_allowed_only(node[1], info):
            return False
        if depends_on(node[2], var_name):
            return rewrite_allowed_only(node[2], info)
        return True
    return False


def rewrite_burden(node, info):
    var_name = info['var']
    if var_name is None or not depends_on(node, var_name):
        return 0
    if rewrite_term_exact(node, info) is not None:
        return 0
    if node[0] in ('sym', 'fn'):
        return 6
    if node[0] == 'div' or node[0] == 'pow':
        return 1 + rewrite_burden(node[1], info) + rewrite_burden(node[2], info)
    if node[0] == 'add' or node[0] == 'mul':
        total = 1
        i = 0
        while i < len(node[1]):
            total += rewrite_burden(node[1][i], info)
            i += 1
        return total
    return 4


def rewrite_candidate_steps(expr, info):
    out = []
    burden = rewrite_burden(expr, info)

    def add_candidate(candidate, note):
        if candidate is None:
            return
        if same(candidate, expr):
            return
        candidate_burden = rewrite_burden(candidate, info)
        if candidate_burden > burden and not rewrite_allowed_only(candidate, info):
            return
        out.append((candidate, note))

    cancelled = cancel_fraction_factor(expr, info['var'])
    if cancelled is not None:
        add_candidate(cancelled[0], cancelled[1])

    factored = factor_expression(expr, info['var'])
    if factored is not None:
        add_candidate(factored[0], factored[1])

    squared, note = complete_the_square(expr)
    if squared is not None:
        add_candidate(squared, note if note else 'Complete the square')

    i = 0
    while i < len(info['terms']):
        rewritten = rewrite_polynomial_in_linear_term(expr, info['terms'][i], info['var'])
        if rewritten is not None:
            add_candidate(rewritten[0], rewritten[1])
        i += 1

    add_candidate(canonical_compare_form(expr), 'Simplify and collect like terms')
    add_candidate(expand_binomial_pow(expr), 'Expand the binomial')
    add_candidate(expand_pow_sqrt(expr), 'Simplify powers and roots')
    add_candidate(expand_mul_distribute(expr), 'Expand brackets')
    add_candidate(combine_fractions(expr), 'Combine the fractions')
    add_candidate(simplify_trig_identity(expr), 'Use a standard identity')

    exact = rewrite_term_equivalent(expr, info)
    if exact is not None and not same(exact, expr):
        add_candidate(exact, 'Rewrite to the requested term')
    return out


def rewrite_candidate_score(expr, info, step_count):
    return rewrite_burden(expr, info) * 1000 + tree_size(expr) + step_count * 5


def rewrite_equivalent(a, b):
    return same(canonical_compare_form(a), canonical_compare_form(b)) or equivalent(a, b)


def search_rewrite_expression(expr, allowed_terms):
    info = build_rewrite_allowed_info(expr, allowed_terms)
    start = sim(expr)
    if rewrite_allowed_only(start, info) and rewrite_equivalent(start, expr):
        return start, [], info
    states = [(start, [])]
    visited = {start}
    depth = 0
    while depth < ALGEBRA_REWRITE_SEARCH_DEPTH:
        next_states = []
        i = 0
        while i < len(states):
            current, steps = states[i]
            candidates = rewrite_candidate_steps(current, info)
            j = 0
            while j < len(candidates):
                candidate, note = candidates[j]
                if candidate not in visited:
                    visited.add(candidate)
                    new_steps = steps + [(note, candidate)]
                    if rewrite_allowed_only(candidate, info) and rewrite_equivalent(candidate, expr):
                        return candidate, new_steps, info
                    next_states.append((candidate, new_steps))
                j += 1
            i += 1
        next_states.sort(key=lambda item: rewrite_candidate_score(item[0], info, len(item[1])))
        states = next_states[:ALGEBRA_REWRITE_BEAM_WIDTH]
        depth += 1
    return None, None, info


def format_rewrite_lines(original_text, expr, final_expr, steps, allowed_terms, is_equation):
    bits = []
    i = 0
    while i < len(allowed_terms):
        bits.append(show(allowed_terms[i]))
        i += 1
    lines = []
    if is_equation:
        lines.append('Start with ' + original_text.strip())
        lines.append('Write in terms of ' + ', '.join(bits) + ' only.')
        lines.append('Move all terms to one side.')
        lines.append(equation_text(expr, num(0)))
        prev = expr
        i = 0
        while i < len(steps):
            lines.append(steps[i][0])
            if not same(prev, steps[i][1]):
                lines.append(equation_text(steps[i][1], num(0)))
                prev = steps[i][1]
            i += 1
        lines.append('Hence ' + equation_text(final_expr, num(0)))
    else:
        lines.append('Start with ' + show(expr))
        lines.append('Write in terms of ' + ', '.join(bits) + ' only.')
        prev = expr
        i = 0
        while i < len(steps):
            lines.append(steps[i][0])
            if not same(prev, steps[i][1]):
                lines.append('= ' + show(steps[i][1]))
                prev = steps[i][1]
            i += 1
        lines.append('Final = ' + show(final_expr))
    return lines


def solve_rewrite_text(text, term_texts):
    begin_user_action()
    if len(term_texts) == 0:
        raise ValueError('Enter at least one allowed term.')
    allowed_terms = []
    i = 0
    while i < len(term_texts):
        allowed_terms.append(parse(term_texts[i].strip()))
        i += 1
    if '=' in text:
        parts = text.split('=', 1)
        lhs = parse(parts[0].strip())
        rhs = parse(parts[1].strip())
        expr = sim(sub(lhs, rhs))
        is_equation = True
    else:
        expr = parse(text.strip())
        is_equation = False
    final_expr, steps, info = search_rewrite_expression(expr, allowed_terms)
    if final_expr is None:
        if rewrite_allowed_only(expr, info):
            final_expr = expr
            steps = []
        else:
            raise ValueError('This expression cannot be written using only the requested terms.')
    return format_rewrite_lines(text, expr, final_expr, steps or [], allowed_terms, is_equation)


def simplify_trig_identity(node):
    node = sim(node)
    if node[0] == 'add':
        items = flat(node, 'add')
        coeff_map = {}
        order = []
        i = 0
        while i < len(items):
            coeff, rest = split_coeff(items[i])
            key = sig(rest)
            if key not in coeff_map:
                coeff_map[key] = [rest, coeff]
                order.append(key)
            else:
                coeff_map[key][1] = addq(coeff_map[key][1], coeff)
            i += 1

        def coeff_for(rest):
            item = coeff_map.get(sig(rest))
            if item is None:
                return num(0)
            return item[1]

        def replace_pair(first, second, replacement):
            key_first = sig(first)
            key_second = sig(second)
            if key_first not in coeff_map or key_second not in coeff_map:
                return None
            c_first = coeff_map[key_first][1]
            c_second = coeff_map[key_second][1]
            if not is_one(c_first) or not is_one(c_second):
                return None
            out = []
            j = 0
            while j < len(order):
                key = order[j]
                if key == key_first or key == key_second:
                    j += 1
                    continue
                rest, coeff = coeff_map[key]
                if is_zero(coeff):
                    j += 1
                    continue
                if is_one(coeff):
                    out.append(rest)
                elif is_minus_one(coeff):
                    out.append(neg(rest))
                else:
                    out.append(mul([coeff, rest]))
                j += 1
            out.append(replacement)
            return sim(make_add(out))

        def replace_signed(first, first_negative, second, second_negative, replacement):
            key_first = sig(first)
            key_second = sig(second)
            if key_first not in coeff_map or key_second not in coeff_map:
                return None
            c_first = coeff_map[key_first][1]
            c_second = coeff_map[key_second][1]
            if first_negative:
                if not is_minus_one(c_first):
                    return None
            elif not is_one(c_first):
                return None
            if second_negative:
                if not is_minus_one(c_second):
                    return None
            elif not is_one(c_second):
                return None
            out = []
            j = 0
            while j < len(order):
                key = order[j]
                if key == key_first or key == key_second:
                    j += 1
                    continue
                rest, coeff = coeff_map[key]
                if is_zero(coeff):
                    j += 1
                    continue
                if is_one(coeff):
                    out.append(rest)
                elif is_minus_one(coeff):
                    out.append(neg(rest))
                else:
                    out.append(mul([coeff, rest]))
                j += 1
            out.append(replacement)
            return sim(make_add(out))

        i = 0
        while i < len(items):
            sin_arg = is_pow_of_fn(items[i], 'sin', num(2))
            if sin_arg is not None:
                cos_term = power(fn('cos', sin_arg), num(2))
                replaced = replace_pair(items[i], cos_term, num(1))
                if replaced is not None:
                    return replaced
            tan_arg = is_pow_of_fn(items[i], 'tan', num(2))
            if tan_arg is not None:
                sec_term = power(fn('sec', tan_arg), num(2))
                if is_one(coeff_for(sec_term)) and is_minus_one(coeff_for(items[i])):
                    return sim(make_add([num(1)] + [item for item in items if not same(item, sec_term) and not same(item, items[i])]))
                one_plus = replace_pair(items[i], num(1), sec_term)
                if one_plus is not None:
                    return one_plus
            cot_arg = is_pow_of_fn(items[i], 'cot', num(2))
            if cot_arg is not None:
                cosec_term = power(fn('cosec', cot_arg), num(2))
                replaced = replace_signed(cosec_term, False, items[i], True, num(1))
                if replaced is not None:
                    return replaced
                one_plus = replace_pair(items[i], num(1), cosec_term)
                if one_plus is not None:
                    return one_plus
            sec_arg = is_pow_of_fn(items[i], 'sec', num(2))
            if sec_arg is not None:
                tan_term = power(fn('tan', sec_arg), num(2))
                replaced = replace_signed(items[i], False, tan_term, True, num(1))
                if replaced is not None:
                    return replaced
            cosec_arg = is_pow_of_fn(items[i], 'cosec', num(2))
            if cosec_arg is not None:
                cot_term = power(fn('cot', cosec_arg), num(2))
                replaced = replace_signed(items[i], False, cot_term, True, num(1))
                if replaced is not None:
                    return replaced
            i += 1
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
    simple1 = canonical_compare_form(expr1)
    steps.append((step_num, 'Simplify expr1: ' + show(simple1), simple1))
    step_num += 1
    simple2 = canonical_compare_form(expr2)
    steps.append((step_num, 'Simplify expr2: ' + show(simple2), simple2))
    step_num += 1
    if equivalent(simple1, simple2):
        steps.append((step_num, 'EXPRESSIONS ARE EQUIVALENT', simple1))
        return True, steps
    diff = show(sim(add([simple1, neg(simple2)])))
    steps.append((step_num, 'Difference (should be 0): ' + diff, None))
    return False, steps


def main():
    print('1 cmp')
    print('2 xform')
    print('3 exp')
    print('4 poly')
    print('5 comp sq')
    print('6 solve')
    print('7 comp')
    print('8 inv')
    print('9 rw')
    mode = input('M: ').strip()
    if mode == '':
        mode = '1'
    begin_user_action()
    try:
        if mode == '1':
            text1 = input('E1: ').strip()
            text2 = input('E2: ').strip()
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
            text1 = input('Src: ').strip()
            expr1 = parse(text1)
            text2 = input('Tgt: ').strip()
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
            text1 = input('Expr: ').strip()
            expr1 = parse(text1)
            max_terms_str = input('Max: ').strip()
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
            print('1 add')
            print('2 sub')
            print('3 mul')
            submode = input('Op: ').strip()
            if submode == '1' or submode == '2' or submode == '3':
                text1 = input('P1: ').strip()
                text2 = input('P2: ').strip()
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
                print('2. No comp sq')
                print('3. Out = ' + show(expr1))
            else:
                print('2. ' + (note if note else 'Ans'))
                print('3. Ans = ' + show(result))
        elif mode == '6':
            text1 = input('Eq: ').strip()
            if '=' not in text1:
                print('Use f(x)=0')
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
            print('Comp f(g(x))')
            f_text = input('f: ').strip()
            g_text = input('g: ').strip()
            if f_text == '':
                f_text = '2*x+1'
                g_text = 'x^2'
                print('Use: f=' + f_text + ', g=' + g_text)
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
        elif mode == '8':
            print('Inv fn')
            f_text = input('f: ').strip()
            if f_text == '':
                f_text = '2*x+1'
                print('Use: f=' + f_text)
            result, steps = inverse_function(f_text)
            print('1. f(x) = ' + f_text)
            if result:
                i = 0
                while i < len(steps):
                    print(str(i+2) + '. ' + steps[i])
                    i += 1
            else:
                print('2. ' + steps[0])
        elif mode == '9':
            text = input('Rw: ').strip()
            print('1 term/line')
            print('Blank = end')
            terms = []
            while True:
                term = input('T: ').strip()
                if term == '':
                    break
                terms.append(term)
            lines = solve_rewrite_text(text, terms)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        else:
            print('Bad mode.')
    except Exception as err:
        print('Err: ' + str(err))


run = main
if not SKIP_AUTORUN:
    main()
