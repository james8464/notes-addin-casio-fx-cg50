try:
    import math
except ImportError:
    math = None
try:
    import sys
except ImportError:
    sys = None

try:
    from src.shared_helpers import (
        compact_duplicate_answer_lines,
        ensure_reasoning_marker,
        fn as shared_fn,
        is_num,
        is_one,
        is_sym,
        is_zero,
        normalize_input_text,
        neg as shared_neg,
        same_by_sig,
        E,
        PI,
    )
    from src.shared_cache import cache_store, clear_all_caches as shared_clear_all_caches
    from src.shared_reasoning_markers import REASONING_MARKERS
except ImportError:
    try:
        from shared_helpers import (
            compact_duplicate_answer_lines,
            ensure_reasoning_marker,
            fn as shared_fn,
            is_num,
            is_one,
            is_sym,
            is_zero,
            normalize_input_text,
            neg as shared_neg,
            same_by_sig,
            E,
            PI,
        )
        from shared_cache import cache_store, clear_all_caches as shared_clear_all_caches
        from shared_reasoning_markers import REASONING_MARKERS
    except ImportError:
        try:
            from shared_fallback import (
                cache_store, clear_all_caches as shared_clear_all_caches,
                compact_duplicate_answer_lines, ensure_reasoning_marker,
                fn as shared_fn, is_num, is_one, is_sym, is_zero,
                normalize_input_text, neg as shared_neg, same_by_sig, E, PI,
                REASONING_MARKERS,
            )
        except ImportError:
            cache_store = None
            shared_clear_all_caches = lambda *c: None
            compact_duplicate_answer_lines = lambda x: x
            ensure_reasoning_marker = lambda x: x
            shared_fn = lambda *a: tuple(a)
            is_num = lambda n: n is not None and n[0] == 'num'
            is_one = lambda n: is_num(n) and n[1] == n[2]
            is_sym = lambda n: n is not None and n[0] == 'sym'
            is_zero = lambda n: is_num(n) and n[1] == 0
            normalize_input_text = lambda t: t.strip() if isinstance(t, str) else t
            shared_neg = lambda n: n
            same_by_sig = lambda a, b, sig_func, cache=None, cache_store_func=None, cache_limit=None: a == b
            E = ("const", "e")
            PI = ("const", "pi")
            REASONING_MARKERS = ("method:", "use ", "using ", "let ", "solve ", "answer:")


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

MAX_NESTING_DEPTH = 200
MAX_INPUT_LENGTH = 10000
MAX_TOKEN_COUNT = 2000

SIG_CACHE = {}
SHOW_CACHE = {}
SPLIT_COEFF_CACHE = {}
FLAT_CACHE = {}
SAME_CACHE = {}
EQUIV_CACHE = {}
DEPENDS_CACHE = {}

ALL_CACHES = (
    SIG_CACHE, SHOW_CACHE, SPLIT_COEFF_CACHE,
    FLAT_CACHE, SAME_CACHE, EQUIV_CACHE, DEPENDS_CACHE
)


def clear_all_caches():
    """Clear all caches to free memory."""
    shared_clear_all_caches(*ALL_CACHES)


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


def lcm(a, b):
    return abs(a * b) // gcd(a, b)


def num(a, b=1):
    if b == 0:
        raise ValueError('Division by zero.')
    if b < 0:
        a = -a
        b = -b
    g = gcd(a, b)
    return 'num', a // g, b // g


def num_text(text):
    # Handle edge cases for better robustness
    if not text or text.strip() == '':
        raise ValueError('Empty number')

    text = text.strip()
    if text in ('.', '+.', '-.'):
        raise ValueError('Invalid number format')
    if '.' not in text:
        # Handle integer case
        if text == '-' or text == '+':
            raise ValueError('Invalid number format')
        return num(int(text))

    # Handle decimal case
    left, right = text.split('.', 1)
    if left == '':
        left = '0'
    if right == '':
        right = '0'

    # Validate that we have valid digits
    if not left.lstrip('-').isdigit() and left != '':
        raise ValueError('Invalid number format')
    if not right.isdigit() and right != '':
        raise ValueError('Invalid number format')

    scale = 1
    for _ in right:
        scale *= 10

    sign = 1
    if left.startswith('-'):
        sign = -1
        left = left[1:]
        if left == '':  # Handle case like "-.5"
            left = '0'

    return num(sign * int(left + right), scale)


def sym(name):
    return 'sym', name





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


def solve_linear_system_float(mat, rhs):
    n = len(rhs)
    a = [list(row) for row in mat]
    b = list(rhs)
    col = 0
    while col < n:
        pivot = col
        while pivot < n and abs(a[pivot][col]) < 1e-10:
            pivot += 1
        if pivot == n:
            return
        if pivot != col:
            a[col], a[pivot] = a[pivot], a[col]
            b[col], b[pivot] = b[pivot], b[col]
        pivot_val = a[col][col]
        j = col
        while j < n:
            a[col][j] = a[col][j] / pivot_val
            j += 1
        b[col] = b[col] / pivot_val
        row = 0
        while row < n:
            if row != col and abs(a[row][col]) >= 1e-10:
                factor = a[row][col]
                j = col
                while j < n:
                    a[row][j] = a[row][j] - factor * a[col][j]
                    j += 1
                b[row] = b[row] - factor * b[col]
            row += 1
        col += 1
    return b


def rational_from_float(value):
    if abs(value) < 1e-9:
        return num(0)
    den = 1
    while den <= 64:
        top = int(round(value * den))
        if abs(value - float(top) / float(den)) < 1e-8:
            return num(top, den)
        den += 1
    scale = 1000000
    return num(int(round(value * scale)), scale)


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
    return same_by_sig(a, b, sig, SAME_CACHE, cache_store, CACHE_LIMIT_LARGE)


def format_equation_human_readable(node, parent=0):
    """
    Format an equation node into a human-readable string with clear operator precedence.
    """
    kind = node[0]
    
    if kind == 'num':
        if node[2] == 1:
            return str(node[1])
        return '(' + str(node[1]) + '/' + str(node[2]) + ')'
    
    elif kind == 'sym':
        return node[1]
    
    elif kind == 'const':
        return node[1]
    
    elif kind == 'fn':
        if node[1] == 'log':
            arg = format_equation_human_readable(node[2], 0)
            if node[2][0] == 'fn' and node[2][1] == 'abs':
                return 'ln|' + format_equation_human_readable(node[2][2], 0) + '|'
            return 'ln(' + arg + ')'
        elif node[1] == 'exp':
            return 'e^(' + format_equation_human_readable(node[2], 0) + ')'
        else:
            return node[1] + '(' + format_equation_human_readable(node[2], 0) + ')'
    
    elif kind == 'pow':
        base = format_equation_human_readable(node[1], 3)
        exponent = format_equation_human_readable(node[2], 3)
        
        if node[1][0] in ('add', 'mul', 'div') or (node[1][0] == 'num' and node[1][2] != 1):
            base = '(' + base + ')'
        
        if node[2][0] not in ('num',) or node[2][2] != 1:
            exponent = '(' + exponent + ')'
        
        return base + '^' + exponent
    
    elif kind == 'mul':
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for item in items:
            part = format_equation_human_readable(item, 2)
            if item[0] == 'add':
                part = '(' + part + ')'
            parts.append(part)
        
        return '*'.join(parts)
    
    elif kind == 'div':
        numerator = format_equation_human_readable(node[1], 2)
        denominator = format_equation_human_readable(node[2], 2)
        
        if node[1][0] in ('add', 'mul'):
            numerator = '(' + numerator + ')'
        if node[2][0] in ('add', 'mul'):
            denominator = '(' + denominator + ')'
        
        return numerator + '/' + denominator
    
    elif kind == 'add':
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for i, item in enumerate(items):
            coeff, rest = split_coeff(item) if hasattr(item, '__iter__') else (item, None)
            
            if rest is None:
                term = format_equation_human_readable(item, 1)
            else:
                term = format_equation_human_readable(rest, 1)
                if not (coeff[0] == 'num' and coeff[1] == 1 and coeff[2] == 1):
                    coeff_str = format_equation_human_readable(coeff, 1)
                    term = coeff_str + '*' + term
            
            parts.append(term)
        
        result = ' + '.join(parts)
        return result
    
    return str(node)


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


def make_add(parts):
    out = []
    i = 0
    while i < len(parts):
        item = parts[i]
        if item[0] == 'add':
            out.extend(item[1])
        elif item[0] == 'mul':
            inner = list(item[1])
            if len(inner) == 2 and is_num(inner[0]) and is_minus_one(inner[0]) and inner[1][0] == 'add':
                j = 1
                while j < len(inner):
                    if inner[j][0] == 'add':
                        k = 0
                        while k < len(inner[j][1]):
                            out.append(make_mul([num(-1), inner[j][1][k]]))
                            k += 1
                    else:
                        out.append(make_mul([num(-1), inner[j]]))
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
    return sim(('add', tuple(out)))


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
    return sim(('mul', tuple(out)))


def add(parts):
    return make_add(parts)


def mul(parts):
    return make_mul(parts)


def div(a, b):
    return sim(('div', a, b))


def power(a, b):
    return sim(('pow', a, b))


def fn(name, arg):
    return shared_fn(name, arg, sim)


def neg(node):
    return shared_neg(node, num, mul)


def sub(a, b):
    return add([a, neg(b)])


def int_sqrt(n):
    """Integer square root - returns sqrt(n) if perfect square, else None."""
    if n < 0:
        return None
    if n == 0:
        return 0
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


def int_nth_root(n, power_value):
    if n < 0 or power_value <= 0:
        return None
    if n in (0, 1):
        return n
    lo = 1
    hi = 1
    while hi ** power_value < n:
        hi *= 2
    while lo <= hi:
        mid = (lo + hi) // 2
        val = mid ** power_value
        if val == n:
            return mid
        if val < n:
            lo = mid + 1
        else:
            hi = mid - 1
    return None


def exact_nth_root_num(node, power_value):
    if not is_num(node) or power_value <= 0:
        return None
    sign = 1
    numerator = node[1]
    if numerator < 0:
        if power_value % 2 == 0:
            return None
        sign = -1
        numerator = -numerator
    top = int_nth_root(numerator, power_value)
    bottom = int_nth_root(node[2], power_value)
    if top is None or bottom is None:
        return None
    return num(sign * top, bottom)


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
    if name == 'abs' and (arg[0] == 'pow' and same(arg[1], E) or arg[0] == 'fn' and arg[1] == 'exp'):
        return arg
    if name == 'log' and is_one(arg):
        return num(0)
    if name == 'log' and same(arg, E):
        return num(1)
    if name == 'log' and arg[0] == 'pow' and same(arg[1], E):
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
        return E
    if name == 'exp' and arg[0] == 'fn' and arg[1] == 'log':
        return arg[2]
    if name == 'exp':
        log_combo = exp_from_log_combo(arg)
        if log_combo is not None:
            return log_combo
    if name == 'exp':
        return power(E, arg)
    if name == 'sqrt' and arg[0] == 'pow' and same(arg[1], E) and is_num(arg[2]) and arg[2][1] == 2 and arg[2][2] == 1:
        return power(E, num(1, 2))
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


def generalized_binomial_coeff(exponent, k):
    if k == 0:
        return num(1)
    result = exponent
    i = 1
    while i < k:
        result = sim(mul([result, sub(exponent, num(i))]))
        i += 1
    return sim(div(result, num(math.factorial(k))))


def split_constant_plus_term(node):
    node = sim(node)
    terms = flat(node, 'add') if node[0] == 'add' else [node]
    constant = num(0)
    dependent = []
    i = 0
    while i < len(terms):
        if depends_on(terms[i], 'x'):
            dependent.append(terms[i])
        else:
            constant = sim(add([constant, terms[i]]))
        i += 1
    if len(dependent) != 1 or is_zero(constant):
        return None
    return constant, dependent[0]


def ordered_sum_text(terms):
    if len(terms) == 0:
        return '0'
    out = ''
    i = 0
    while i < len(terms):
        term = sim(terms[i])
        coeff, rest = split_coeff(term)
        if i == 0:
            if coeff[1] < 0:
                out = '-' + show(mul([num(-coeff[1], coeff[2]), rest]), 1)
            else:
                out = show(term, 1)
        else:
            if coeff[1] < 0:
                out += ' - ' + show(mul([num(-coeff[1], coeff[2]), rest]), 1)
            else:
                out += ' + ' + show(term, 1)
        i += 1
    return out


def generalized_binomial_expand_text(expr, max_terms=None):
    if expr[0] != 'pow' or not is_num(expr[2]):
        return None
    split = split_constant_plus_term(expr[1])
    if split is None:
        return None
    constant, dependent = split
    exponent = sim(expr[2])
    const_value = eval_with_values(constant, {})
    if const_value is None or not is_num(const_value) or is_zero(const_value):
        return None
    u = sim(div(dependent, constant))
    prefactor = sim(expand_pow_sqrt(power(constant, exponent)))
    limit = max_terms if max_terms is not None and max_terms > 0 else 4
    original_expr = show(expr)
    if is_one(constant):
        lines = ['Binomial expansion']
    elif is_one(prefactor):
        lines = ['Binomial expansion', 'Factor out ' + show(constant) + ': ' + original_expr + ' = (1 + ' + show(u) + ')^' + show(exponent)]
    else:
        lines = ['Binomial expansion', 'Factor out ' + show(constant) + ': ' + original_expr + ' = ' + show(prefactor) + '*(1 + ' + show(u) + ')^' + show(exponent)]
    expanded_terms = []
    k = 0
    while k < limit:
        coeff = generalized_binomial_coeff(exponent, k)
        pieces = []
        if not is_one(prefactor):
            pieces.append(prefactor)
        if not is_one(coeff):
            pieces.append(coeff)
        if k > 0:
            pieces.append(u if k == 1 else power(u, num(k)))
        term = sim(expand_pow_sqrt(make_product_or_one(pieces)))
        expanded_terms.append(term)
        lines.append('Term ' + str(k + 1) + ': ' + show(term))
        k += 1
    lines.append('Out = ' + ordered_sum_text(expanded_terms) + ' + ...')
    return lines


def can_use_generalized_binomial(expr):
    if expr[0] != 'pow' or not is_num(expr[2]):
        return False
    exp = expr[2]
    return exp[2] != 1 and exp[1] > 0






































def split_rewrite_arrow(text):
    if '->' not in text:
        return None
    left, right = text.split('->', 1)
    return left.strip(), right.strip()


def equation_has_trig_content(lhs, rhs):
    return False


def solve_linear_parameter_text(eq_text, var):
    raise ValueError('Use a linear equation in the chosen parameter, then collect terms and solve.')


def solve_x_equation_text(eq_text, var, extra):
    raise ValueError('Use Trigonometry solve mode for trig equations; algebra solve handles polynomial, rational, radical, and substitution forms.')


def solve_extremum_text(text, kind, var):
    raise ValueError('Use Trigonometry extremum mode for sine/cosine extrema; algebra mode has no extremum operation.')


def parse_identity_or_zero(text):
    return parse_equation_or_zero(text)


def special_text_proof(text, route):
    return None


def is_domain_restricted_identity_pair(lhs, rhs):
    return False


def solve_prove(lhs, rhs, route):
    return ['Proof route not implemented']


def numeric_evaluation_text(text):
    return None


















































































def choose_rewrite_var(expr, term):
    return choose_primary_var(expr)


def extract_polynomial_symbol(expr, var_name, max_degree=2):
    coeffs, degree = polynomial_coeff_list(expr, var_name, max_degree)
    if coeffs is None:
        return None
    out = {}
    i = 0
    while i <= max_degree:
        out[i] = coeffs[i]
        i += 1
    return out


def polynomial_degree_coeff_map(coeffs, max_deg):
    degree = max_deg
    while degree > 0 and is_zero(coeffs[degree]):
        degree -= 1
    return degree


def polynomial_coeff_nodes_low(coeffs, degree):
    out = []
    i = 0
    while i <= degree:
        out.append(coeffs[i])
        i += 1
    return out


def quadratic_roots_from_nodes(coeff_nodes):
    return rational_roots_for_quadratic(coeff_nodes)
















def factor_text_mode(text):
    return factor_text(text)


def rewrite_text_mode(text, term_text):
    return rewrite_in_term_text(text, term_text)








def rewrite_in_term(expr_text, term_text):
    return rewrite_in_term_text(expr_text, term_text)


def factor_expr_text(expr_text):
    return factor_text(expr_text)


def solve_expr_text(expr_text):
    return solve_equation_text(expr_text)


def poly_mode_output(expr_text):
    return factor_text(expr_text)


def split_equation_text(text):
    return parse_equation_or_zero(text)


def expr_depends_on(node, var_name):
    return depends_on(node, var_name)


def collect_roots_text(var_name, roots):
    return format_solution_line(var_name, roots)


def solve_equation_cli(text):
    return solve_equation_text(text)


def factor_cli(text):
    return factor_text(text)


def rewrite_cli(text, term_text):
    return rewrite_in_term_text(text, term_text)


def transform_cli(text1, text2):
    return solve_transform_text(text1, text2)


def prove_cli(text, route='1'):
    return solve_prove_text(text, route)


def simplify_cli(text1, text2):
    return compare_expressions(parse(text1), parse(text2))


def poly_cli(text):
    return factor_text(text)


def inverse_cli(text):
    return inverse_function(text)


def comp_cli(f_text, g_text):
    return compose_functions(f_text, g_text)


def rw_cli(text, terms):
    return solve_rewrite_text(text, terms)


def solve_cli(text):
    return solve_equation_text(text)


def factor_quadratic_text(text):
    return factor_text(text)


def parse_cli_expr(text):
    return parse(text)


def parse_cli_equation(text):
    return parse_equation_or_zero(text)


def rewrite_in_u_cli(text, term_text):
    return rewrite_in_term_text(text, term_text)


def solve_poly_cli(text):
    return solve_equation_text(text)


def factor_poly_cli(text):
    return factor_text(text)
























def solve_hint(text):
    return solve_equation_text(text)


















def build_rewrite_term_symbol(node, term):
    return choose_unused_symbol(node, [term])


def quadratic_factor_text(expr_text):
    return factor_text(expr_text)


def quadratic_solve_text(expr_text):
    return solve_equation_text(expr_text)


def rewrite_term_symbol(node, term):
    return build_rewrite_term_symbol(node, term)












def rewrite_term_if_possible(text, term_text):
    return rewrite_in_term_text(text, term_text)


def solve_if_possible(text):
    return solve_equation_text(text)


def factor_if_possible(text):
    return factor_text(text)


def quadratic_factor_if_possible(node):
    return factor_quadratic_rational(node)


def quadratic_solve_if_possible(node):
    return solve_equation(node)


def linear_term_rewrite_if_possible(node, term, var_name):
    return rewrite_polynomial_in_linear_term(node, term, var_name)


def shifted_reciprocal_if_possible(node, var_name):
    return match_shifted_reciprocal(node, var_name)


def solve_display_text(text):
    return solve_equation_text(text)


def factor_display_text(text):
    return factor_text(text)


def rewrite_display_text(text, term_text):
    return rewrite_in_term_text(text, term_text)








def solve_with_polynomial_support(text):
    return solve_equation_text(text)


def factor_with_polynomial_support(text):
    return factor_text(text)


def rewrite_with_polynomial_support(text, term_text):
    return rewrite_in_term_text(text, term_text)


















































def root_sort_key(node):
    value = numeric_eval(node, {})
    if value is not None:
        if isinstance(value, complex):
            if abs(value.imag) < 1e-10:
                value = value.real
            else:
                return (2, value.real, value.imag, show(node))
        if value < 0:
            return (1, value, show(node))
        return (0, -value, show(node))
    return (2, 0, show(node))


def trig_rewrite_direct(expr, allowed_terms):
    allowed_names = {}
    i = 0
    while i < len(allowed_terms):
        term = sim(allowed_terms[i])
        if term[0] == 'fn':
            allowed_names[(term[1], sig(term[2]))] = True
        i += 1
    expr = sim(expr)
    if equivalent(expr, add([num(1), neg(fn('cos', mul([num(2), sym('x')])))])) and ('sin', sig(sym('x'))) in allowed_names:
        return sim(mul([num(2), power(fn('sin', sym('x')), num(2))])), 'Use 1-cos(2A) = 2sin^2(A)'
    if equivalent(expr, add([num(1), fn('cos', mul([num(2), sym('x')]))])) and ('cos', sig(sym('x'))) in allowed_names:
        return sim(mul([num(2), power(fn('cos', sym('x')), num(2))])), 'Use 1+cos(2A) = 2cos^2(A)'
    if equivalent(expr, num(1)):
        return num(1), 'Use a standard identity'
    return None, None


def search_rewrite_expression(expr, allowed_terms):
    info = build_rewrite_allowed_info(expr, allowed_terms)
    start = sim(expr)
    direct, note = trig_rewrite_direct(start, allowed_terms)
    if direct is not None:
        return direct, [(note, direct)], info
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
            direct, note = trig_rewrite_direct(current, allowed_terms)
            if direct is not None and direct not in visited:
                visited.add(direct)
                new_steps = steps + [(note, direct)]
                if rewrite_allowed_only(direct, info) and rewrite_equivalent(direct, expr):
                    return direct, new_steps, info
                next_states.append((direct, new_steps))
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


def solve_rewrite_text(text, term_texts):
    if len(term_texts) == 0:
        raise ValueError('Use at least one target term.')
    allowed_terms = []
    i = 0
    while i < len(term_texts):
        allowed_terms.append(parse(term_texts[i].strip()))
        i += 1
    parts = split_top_level(text, '=')
    is_equation = parts is not None
    if is_equation:
        lhs, rhs = parse_equation_or_zero(text)
        expr = sim(sub(lhs, rhs))
    else:
        expr = parse(text.strip())
    if len(term_texts) == 1:
        try:
            return rewrite_in_term_text(text, term_texts[0])
        except Exception:
            pass
    final_expr, steps, info = search_rewrite_expression(expr, allowed_terms)
    if final_expr is None:
        if rewrite_allowed_only(expr, info):
            final_expr = expr
            steps = []
        elif equivalent(expr, num(1)):
            final_expr = num(1)
            steps = [('Use a standard identity', num(1))]
        else:
            raise ValueError('Cannot rewrite in that term.')
    return format_rewrite_lines(text, expr, final_expr, steps or [], allowed_terms, is_equation)


def normalize_solution_roots(roots):
    out = []
    i = 0
    while i < len(roots):
        duplicate = False
        j = 0
        while j < len(out):
            if equivalent(out[j], roots[i]):
                duplicate = True
                break
            j += 1
        if not duplicate:
            out.append(roots[i])
        i += 1
    out.sort(key=root_sort_key)
    return out


def format_solution_line(var_name, roots):
    bits = []
    i = 0
    while i < len(roots):
        bits.append(show(roots[i]))
        i += 1
    return var_name + ' = ' + (bits[0] if len(bits) == 1 else '[' + ', '.join(bits) + ']')


def factor_from_roots(var_node, roots):
    factors = []
    i = 0
    while i < len(roots):
        factors.append(add([var_node, neg(roots[i])]))
        i += 1
    return sim(make_mul(factors))


def polynomial_coeff_map(node, var_name, max_degree=2):
    node = sim(node)
    if not depends_on(node, var_name):
        return {0: node}, 0
    if node == sym(var_name):
        return {0: num(0), 1: num(1)}, 1
    if node[0] == 'pow' and node[1] == sym(var_name) and is_int_num(node[2]):
        deg = node[2][1]
        if deg < 0 or deg > max_degree:
            return None, None
        out = {0: num(0), deg: num(1)}
        i = 1
        while i < deg:
            out[i] = num(0)
            i += 1
        return out, deg
    if node[0] == 'add':
        out = {0: num(0)}
        degree = 0
        i = 0
        while i < len(node[1]):
            cur, cur_deg = polynomial_coeff_map(node[1][i], var_name, max_degree)
            if cur is None:
                return None, None
            for deg in cur:
                out[deg] = sim(add([out.get(deg, num(0)), cur[deg]]))
                if deg > degree and not is_zero(out[deg]):
                    degree = deg
            i += 1
        return out, degree
    if node[0] == 'mul':
        coeffs = {0: num(1)}
        degree = 0
        i = 0
        while i < len(node[1]):
            cur, _cur_deg = polynomial_coeff_map(node[1][i], var_name, max_degree)
            if cur is None:
                return None, None
            new_coeffs = {}
            for d1 in coeffs:
                for d2 in cur:
                    deg = d1 + d2
                    if deg > max_degree:
                        return None, None
                    term = sim(mul([coeffs[d1], cur[d2]]))
                    new_coeffs[deg] = sim(add([new_coeffs.get(deg, num(0)), term]))
            coeffs = new_coeffs
            degree = 0
            for deg in coeffs:
                if not is_zero(coeffs[deg]) and deg > degree:
                    degree = deg
            i += 1
        return coeffs, degree
    if node[0] == 'div' and not depends_on(node[2], var_name):
        top, degree = polynomial_coeff_map(node[1], var_name, max_degree)
        if top is None:
            return None, None
        out = {}
        for deg in top:
            out[deg] = sim(div(top[deg], node[2]))
        return out, degree
    return None, None


def polynomial_coeff_list(node, var_name, max_degree=2):
    coeffs, degree = polynomial_coeff_map(node, var_name, max_degree)
    if coeffs is None:
        return None, None
    out = []
    i = 0
    while i <= max_degree:
        out.append(sim(coeffs.get(i, num(0))))
        i += 1
    while degree > 0 and is_zero(out[degree]):
        degree -= 1
    return out, degree


def make_poly_from_coeffs(coeffs, var_name):
    terms = []
    i = 0
    while i < len(coeffs):
        coeff = sim(coeffs[i])
        if not is_zero(coeff):
            if i == 0:
                terms.append(coeff)
            elif i == 1:
                terms.append(sym(var_name) if is_one(coeff) else mul([coeff, sym(var_name)]))
            else:
                power_term = power(sym(var_name), num(i))
                terms.append(power_term if is_one(coeff) else mul([coeff, power_term]))
        i += 1
    if len(terms) == 0:
        return num(0)
    return sim(make_add(terms))


def rational_roots_for_quadratic(coeff_nodes):
    a = coeff_nodes[2]
    b = coeff_nodes[1]
    c = coeff_nodes[0]
    if is_zero(a):
        if is_zero(b):
            return []
        return [sim(div(neg(c), b))]
    disc = sim(sub(power(b, num(2)), mul([num(4), a, c])))
    if is_num(disc) and disc[1] < 0:
        return []
    disc_root = sqrt_num(disc)
    if disc_root is None:
        disc_root = fn('sqrt', disc)
    denom = sim(mul([num(2), a]))
    return [sim(div(add([neg(b), disc_root]), denom)), sim(div(add([neg(b), neg(disc_root)]), denom))]


def positive_divisors(n):
    n = abs(n)
    if n == 0:
        return [0]
    out = []
    i = 1
    while i * i <= n:
        if n % i == 0:
            out.append(i)
            if i * i != n:
                out.append(n // i)
        i += 1
    out.sort()
    return out


def rational_root_candidates(coeff_nodes, degree):
    common_den = 1
    i = 0
    while i <= degree:
        if not is_num(coeff_nodes[i]):
            return []
        common_den = lcm(common_den, coeff_nodes[i][2])
        i += 1
    ints = []
    i = 0
    while i <= degree:
        ints.append(coeff_nodes[i][1] * (common_den // coeff_nodes[i][2]))
        i += 1
    lead = ints[degree]
    const = ints[0]
    if lead == 0:
        return []
    numerators = positive_divisors(const)
    denominators = positive_divisors(lead)
    seen = set()
    out = []
    i = 0
    while i < len(numerators):
        j = 0
        while j < len(denominators):
            p = numerators[i]
            q = denominators[j]
            if q != 0:
                g = gcd(p, q)
                p2 = p // g
                q2 = q // g
                key = (p2, q2)
                if key not in seen:
                    seen.add(key)
                    out.append(num(p2, q2))
                    out.append(num(-p2, q2))
            j += 1
        i += 1
    if const == 0:
        out.append(num(0))
    return normalize_solution_roots(out)


def polynomial_value_at(coeff_nodes, degree, x_node):
    total = num(0)
    power_term = num(1)
    i = 0
    while i <= degree:
        total = sim(add([total, mul([coeff_nodes[i], power_term])]))
        power_term = sim(mul([power_term, x_node]))
        i += 1
    return total


def deflate_polynomial_coeffs(coeff_nodes, degree, root):
    if degree <= 0:
        return None, None
    desc = []
    i = degree
    while i >= 0:
        desc.append(coeff_nodes[i])
        i -= 1
    new_desc = [desc[0]]
    i = 1
    while i < len(desc):
        new_desc.append(sim(add([desc[i], mul([root, new_desc[-1]])])))
        i += 1
    remainder = new_desc[-1]
    quotient_desc = new_desc[:-1]
    quotient = []
    i = len(quotient_desc) - 1
    while i >= 0:
        quotient.append(sim(quotient_desc[i]))
        i -= 1
    return quotient, remainder


def rational_roots_for_polynomial(coeff_nodes, degree):
    coeffs = list(coeff_nodes[:degree + 1])
    current_degree = degree
    roots = []
    while current_degree > 2:
        candidates = rational_root_candidates(coeffs, current_degree)
        found = None
        i = 0
        while i < len(candidates):
            value = polynomial_value_at(coeffs, current_degree, candidates[i])
            if value is not None and is_zero(value):
                found = candidates[i]
                break
            i += 1
        if found is None:
            break
        roots.append(found)
        coeffs, remainder = deflate_polynomial_coeffs(coeffs, current_degree, found)
        if coeffs is None or remainder is None or not is_zero(remainder):
            return None
        current_degree -= 1
    if current_degree == 2:
        quad_roots = rational_roots_for_quadratic(coeffs)
        if len(quad_roots) == 0:
            disc = sim(sub(power(coeffs[1], num(2)), mul([num(4), coeffs[2], coeffs[0]])))
            if len(roots) != 0 and is_num(disc) and disc[1] < 0:
                return normalize_solution_roots(roots)
            return None
        roots.extend(quad_roots)
    elif current_degree == 1:
        if is_zero(coeffs[1]):
            return None
        roots.append(sim(div(neg(coeffs[0]), coeffs[1])))
    elif current_degree == 0:
        if not is_zero(coeffs[0]):
            return None
    return normalize_solution_roots(roots)


def expand_algebraic(node):
    return canonical_compare_form(node)
















def parse_expr_or_equation(text):
    body = text.strip()
    if body == '':
        raise ValueError('Enter input.')
    if '=' in body:
        # Strip outer parentheses for proper splitting, but only if balanced
        body = body.strip()
        if body.startswith('(') and body.endswith(')'):
            inner = body[1:-1]
            if inner.count('(') == inner.count(')'):
                top_level_eq = split_top_level(inner, '=')
                if top_level_eq is not None:
                    body = inner
        parts = split_top_level(body, '=')
        if parts is None:
            return parse(body)
        left_text, right_text = parts
        return sim(sub(parse(left_text.strip()), parse(right_text.strip())))
    return parse(body)


def print_lines(lines):
    i = 0
    while i < len(lines):
        print(str(i + 1) + '. ' + lines[i])
        i += 1


def compact_lines(lines):
    return lines


def parse_equation_or_zero(text):
    body = text.strip()
    if '=' not in body:
        return parse(body), num(0)
    # Strip outer parentheses for proper splitting, but only if they are balanced
    body = body.strip()
    if body.startswith('(') and body.endswith(')'):
        inner = body[1:-1]
        # Check if stripping leaves balanced parens and contains top-level '='
        if inner.count('(') == inner.count(')'):
            # Find if there's a top-level '='
            top_level_eq = split_top_level(inner, '=')
            if top_level_eq is not None:
                body = inner
    parts = split_top_level(body, '=')
    if parts is None:
        return parse(body), num(0)
    left_text, right_text = parts
    return parse(left_text.strip()), parse(right_text.strip())


def cartesian_linear_pair(node, var_name):
    node = sim(node)
    if not cartesian_depends(node, var_name):
        return num(0), node
    if node == sym(var_name):
        return num(1), num(0)
    if node[0] == 'add':
        coeff = num(0)
        rest = num(0)
        items = flat(node, 'add')
        i = 0
        while i < len(items):
            pair = cartesian_linear_pair(items[i], var_name)
            if pair is None:
                return None
            coeff = sim(add([coeff, pair[0]]))
            rest = sim(add([rest, pair[1]]))
            i += 1
        return coeff, rest
    if node[0] == 'mul':
        items = flat(node, 'mul')
        dep = []
        const_bits = []
        i = 0
        while i < len(items):
            if cartesian_depends(items[i], var_name):
                dep.append(items[i])
            else:
                const_bits.append(items[i])
            i += 1
        if len(dep) != 1:
            return None
        pair = cartesian_linear_pair(dep[0], var_name)
        if pair is None:
            return None
        const_factor = make_mul(const_bits)
        return sim(mul([const_factor, pair[0]])), sim(mul([const_factor, pair[1]]))
    if node[0] == 'div':
        if cartesian_depends(node[2], var_name):
            return None
        pair = cartesian_linear_pair(node[1], var_name)
        if pair is None:
            return None
        return sim(div(pair[0], node[2])), sim(div(pair[1], node[2]))
    return None


def pick_cartesian_dep(lhs, rhs):
    if lhs[0] == 'sym' and lhs[1] != 'x':
        return lhs[1]
    if rhs[0] == 'sym' and rhs[1] != 'x':
        return rhs[1]
    names = set()
    collect_symbol_names(lhs, names)
    collect_symbol_names(rhs, names)
    if 'y' in names:
        return 'y'
    if 'Y' in names:
        return 'Y'
    if lhs[0] == 'sym':
        return lhs[1]
    if rhs[0] == 'sym':
        return rhs[1]
    return None


def cartesian_depends(node, var_name):
    names = set()
    collect_symbol_names(node, names)
    return var_name in names


def cartesian_equation_lines(text):
    if '=' not in text:
        return None
    lhs, rhs = parse_equation_or_zero(text)
    dep = pick_cartesian_dep(lhs, rhs)
    if dep is None:
        return None
    expr = sim(sub(lhs, rhs))
    pair = cartesian_linear_pair(expr, dep)
    if pair is None or is_zero(pair[0]):
        return None
    solved = canonical_compare_form(sim(div(neg(pair[1]), pair[0])))
    return [
        'Cartesian equation: ' + show(lhs) + ' = ' + show(rhs),
        'Collect the ' + dep + ' terms.',
        dep + ' = ' + show(solved),
    ]


def cartesian_term(base_name, offset):
    core = sym(base_name)
    if not is_zero(offset):
        core = add([core, neg(offset)])
    return power(core, num(2))


def match_shifted_scaled_trig(node, trig_name, param):
    node = sim(node)
    terms = flat(node, 'add') if node[0] == 'add' else [node]
    offset = num(0)
    scaled = None
    i = 0
    while i < len(terms):
        if cartesian_depends(terms[i], param):
            coeff, rest = split_coeff(terms[i])
            if rest[0] == 'fn' and rest[1] == trig_name and rest[2] == sym(param) and scaled is None:
                scaled = coeff
            else:
                return None
        else:
            offset = sim(add([offset, terms[i]]))
        i += 1
    if scaled is None or is_zero(scaled):
        return None
    return offset, scaled


def cartesian_from_param_exprs(x_expr, y_expr, param='t'):
    if x_expr == sym(param):
        return sym('y'), sim(substitute(y_expr, sym(param), sym('x'))), 'Substitute t = x.'
    if y_expr == sym(param):
        return sym('x'), sim(substitute(x_expr, sym(param), sym('y'))), 'Substitute t = y.'
    x_pair = cartesian_linear_pair(x_expr, param)
    if x_pair is not None and not is_zero(x_pair[0]):
        t_expr = sim(div(add([sym('x'), neg(x_pair[1])]), x_pair[0]))
        return sym('y'), sim(substitute(y_expr, sym(param), t_expr)), 'Rearrange x(t) to make t the subject, then substitute into y(t).'
    y_pair = cartesian_linear_pair(y_expr, param)
    if y_pair is not None and not is_zero(y_pair[0]):
        t_expr = sim(div(add([sym('y'), neg(y_pair[1])]), y_pair[0]))
        return sym('x'), sim(substitute(x_expr, sym(param), t_expr)), 'Rearrange y(t) to make t the subject, then substitute into x(t).'
    x_cos = match_shifted_scaled_trig(x_expr, 'cos', param)
    y_sin = match_shifted_scaled_trig(y_expr, 'sin', param)
    x_sin = match_shifted_scaled_trig(x_expr, 'sin', param)
    y_cos = match_shifted_scaled_trig(y_expr, 'cos', param)
    pair = (x_cos, y_sin) if x_cos is not None and y_sin is not None else (x_sin, y_cos)
    if pair[0] is not None and pair[1] is not None:
        x_off, x_scale = pair[0]
        y_off, y_scale = pair[1]
        x_term = cartesian_term('x', x_off)
        y_term = cartesian_term('y', y_off)
        left = add([div(x_term, power(x_scale, num(2))), div(y_term, power(y_scale, num(2)))])
        return sim(left), num(1), 'Use sin(t)^2+cos(t)^2 = 1.'
    return None


def cartesian_parametric_lines(x_text, y_text, param='t'):
    x_expr = parse(x_text.strip())
    y_expr = parse(y_text.strip())
    result = cartesian_from_param_exprs(x_expr, y_expr, param)
    lines = ['x(' + param + ') = ' + show(x_expr), 'y(' + param + ') = ' + show(y_expr)]
    if result is None:
        lines.append('Could not eliminate ' + param + ' in this form.')
        return lines
    lhs, rhs, note = result
    lines.append(note)
    lines.append('Cartesian: ' + show(lhs) + ' = ' + show(rhs))
    return lines


def parse_identity(text):
    return parse_equation_or_zero(text)




def split_top_level(text, sep):
    depth = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        elif ch == sep and depth == 0:
            return text[:i], text[i + 1:]
        i += 1
    
    # Handle case like "(x+y=z)" - strip outer parens if balanced and contains sep
    text = text.strip()
    if text.startswith('(') and text.endswith(')'):
        inner = text[1:-1]
        if inner.count('(') == inner.count(')') and sep in inner:
            return split_top_level(inner, sep)
    
    return None


def split_top_level_all(text, sep):
    out = []
    depth = 0
    start = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        elif ch == sep and depth == 0:
            out.append(text[start:i])
            start = i + 1
        i += 1
    out.append(text[start:])
    return out


def top_level_power_suffix(text):
    depth = 0
    i = len(text) - 1
    while i >= 0:
        ch = text[i]
        if ch == ')':
            depth += 1
        elif ch == '(':
            depth -= 1
        elif ch == '^' and depth == 0:
            power_text = text[i + 1:]
            if power_text.isdigit():
                return text[:i], int(power_text)
            return None
        i -= 1
    return None


def text_has_even_log_power(text):
    compact = ''.join(text.split())
    if compact.startswith('ln(') and compact.endswith(')'):
        info = top_level_power_suffix(compact[3:-1])
        return info is not None and info[1] % 2 == 0
    if compact.startswith('log(') and compact.endswith(')'):
        inner = compact[4:-1]
        bits = split_top_level(inner, ',')
        if bits is None:
            return False
        info = top_level_power_suffix(bits[1])
        return info is not None and info[1] % 2 == 0
    return False


def expand_small(node):
    return canonical_compare_form(node)


def rewrite_verified_equivalent(a, b):
    return equivalent(a, b)


def depends_any(node, names):
    i = 0
    while i < len(names):
        if depends_on(node, names[i]):
            return True
        i += 1
    return False


def has_variable_dependency(node):
    var_name = choose_primary_var(node)
    return var_name is not None and depends_on(node, var_name)


def solve_square_sum_reduction_expr(*args, **kwargs):
    return None


def solve_pythag_single_expr(*args, **kwargs):
    return None


def solve_weighted_pythag_expr(*args, **kwargs):
    return None


def solve_cos_double_single_expr(*args, **kwargs):
    return None


def solve_sin_double_single_expr(*args, **kwargs):
    return None


def rewrite_tan_double_factor_polynomial_expr(*args, **kwargs):
    return None


def solve_sin_tan_expr(*args, **kwargs):
    return None


def solve_cos_cot_expr(*args, **kwargs):
    return None


def solve_sin_cot_expr(*args, **kwargs):
    return None


def solve_cos_tan_expr(*args, **kwargs):
    return None


def solve_sin2_tan_expr(*args, **kwargs):
    return None


def solve_sin2_cot_expr(*args, **kwargs):
    return None


def solve_sec_tan_mix_expr(*args, **kwargs):
    return None


def solve_cosec_cot_mix_expr(*args, **kwargs):
    return None


def rewrite_half_angle_ratio_terms(*args, **kwargs):
    return None


def rewrite_sum_product_terms(*args, **kwargs):
    return None


def rewrite_product_to_sum_terms(*args, **kwargs):
    return None


def solve_double_product_reduction_expr(*args, **kwargs):
    return None


def start_line(name, expr):
    return name + ' = ' + show(expr)


def step_line(expr):
    return '= ' + show(expr)


def solve_input_text(text):
    return solve_equation_text(text)


def display_equation_text(lhs, rhs):
    return show(lhs) + ' = ' + show(rhs)


def has_explicit_equation_text(text):
    body = text.strip()
    if body == '':
        return False
    if body.startswith('(') and body.endswith(')'):
        inner = body[1:-1]
        if inner.count('(') == inner.count(')') and split_top_level(inner, '=') is not None:
            body = inner
    return split_top_level(body, '=') is not None


def text_has_trig_function(text):
    body = text.lower()
    markers = ('sin', 'cos', 'tan', 'cot', 'sec', 'cosec')
    i = 0
    while i < len(markers):
        if markers[i] in body:
            return True
        i += 1
    return False


_TRIG_TRANSFORM_MODULE = None


def load_trig_transform_module():
    global _TRIG_TRANSFORM_MODULE
    if _TRIG_TRANSFORM_MODULE is not None:
        return _TRIG_TRANSFORM_MODULE
    old_flag = False
    if sys is not None:
        old_flag = getattr(sys, '_trig_no_autorun', False)
        sys._trig_no_autorun = True
    try:
        _TRIG_TRANSFORM_MODULE = __import__('trigProgram')
    finally:
        if sys is not None:
            sys._trig_no_autorun = old_flag
    return _TRIG_TRANSFORM_MODULE


def compare_mode_text(text1, text2):
    if has_explicit_equation_text(text1) or has_explicit_equation_text(text2):
        lhs1, rhs1 = parse_equation_or_zero(text1)
        lhs2, rhs2 = parse_equation_or_zero(text2)
        expr1 = sim(add([lhs1, neg(rhs1)]))
        expr2 = sim(add([lhs2, neg(rhs2)]))
        return compare_expressions(expr1, expr2)
    return compare_expressions(parse(text1), parse(text2))


def exact_text_or_float(node):
    return show(node)


def display_root_list(var_name, roots):
    return format_solution_line(var_name, roots)


def normalise_fit_constant(node):
    return sim(node)


def preferred_var_or_default(node, default='x'):
    var_name = choose_primary_var(node)
    return default if var_name is None else var_name


def simplify_expr_cli(text):
    return canonical_compare_form(parse(text))








def algebra_mode_6_text(text):
    return solve_equation_text(text)


def algebra_mode_3_text(text):
    return factor_text(text)


def algebra_mode_9_text(text, term_texts):
    return solve_rewrite_text(text, term_texts)


def polynomial_expr_coeffs(node, var_name, max_degree=2):
    return polynomial_coeff_list(node, var_name, max_degree)


def make_solution_lines(var_name, roots, label):
    return [label, format_solution_line(var_name, roots)]


def factor_lines(expr, factored, label):
    return ['Input = ' + show(expr), label, '= ' + show(factored)]


def rewrite_lines(expr, term, u, out):
    return ['u = ' + show(term), show(expr), '= ' + show(out)]


def solve_lines(expr, var_name, roots, label):
    return ['Expr = ' + show(expr), label, format_solution_line(var_name, roots)]


def build_quadratic_from_roots(r1, r2, var_name='x'):
    return factor_from_roots(sym(var_name), [r1, r2])


def choose_rewrite_symbol(node, term):
    return choose_unused_symbol(node, [term])


def try_factor_quadratic(node):
    return factor_quadratic_rational(node)


def try_solve_quadratic(node):
    return solve_equation(node)


def try_rewrite_linear_term(node, term, var_name):
    return rewrite_polynomial_in_linear_term(node, term, var_name)


def try_match_shifted_reciprocal(node, var_name):
    return match_shifted_reciprocal(node, var_name)


def factor_expr(node):
    return factor_expression(node)


def solve_expr(node):
    return solve_equation(node)


def rewrite_expr(node, term, var_name):
    return rewrite_polynomial_in_linear_term(node, term, var_name)


def solve_text(text):
    return solve_equation_text(text)


def factor_hint(text):
    return factor_text(text)


def rewrite_hint(text, term_text):
    return rewrite_in_term_text(text, term_text)


def factor_general_text(text):
    return factor_text(text)


def solve_general_text(text):
    return solve_equation_text(text)


def rewrite_general_text(text, term_text):
    return rewrite_in_term_text(text, term_text)


def short_solution_label(label):
    return label


def polynomial_mode_text(expr_text):
    return factor_text(expr_text)


def algebra_solve_text(expr_text):
    return solve_equation_text(expr_text)


def algebra_factor_text(expr_text):
    return factor_text(expr_text)


def algebra_rewrite_term_text(expr_text, term_text):
    return rewrite_in_term_text(expr_text, term_text)


def roots_text(var_name, roots):
    return format_solution_line(var_name, roots)

def compose_functions(f_text, g_text, var='x'):
    try:
        f = parse(f_text)
        g = parse(g_text)
        
        steps = []
        steps.append("f(x) = " + show(f))
        steps.append("g(x) = " + show(g))
        steps.append("Substitute g(x) into f.")
        
        fg = substitute(f, sym(var), g)
        fg = sim(fg)
        steps.append("Simplify the result.")
        
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
    coeff, rest = split_coeff(node)
    if is_num(coeff) and coeff[1] < 0:
        return None
    coeff_root = sqrt_num(coeff)
    rest_base = None
    if is_num(rest) and is_one(rest):
        rest_base = num(1)
    elif rest[0] == 'pow' and is_int_num(rest[2]) and rest[2][1] > 0 and rest[2][1] % 2 == 0:
        half_exp = rest[2][1] // 2
        rest_base = rest[1] if half_exp == 1 else sim(power(rest[1], num(half_exp)))
    elif is_num(rest):
        rest_base = sqrt_num(rest)
    elif rest[0] == 'mul':
        items = flat(rest, 'mul')
        bases = []
        i = 0
        while i < len(items):
            base = extract_square_base(items[i])
            if base is None:
                return None
            bases.append(base)
            i += 1
        rest_base = sim(make_mul(bases))
    elif rest[0] == 'div':
        top = extract_square_base(rest[1])
        bot = extract_square_base(rest[2])
        if top is None or bot is None:
            return None
        rest_base = sim(div(top, bot))
    elif node[0] == 'pow' and is_int_num(node[2]) and node[2][1] > 0 and node[2][1] % 2 == 0:
        half_exp = node[2][1] // 2
        rest_base = node[1] if half_exp == 1 else sim(power(node[1], num(half_exp)))
        coeff_root = num(1)
    if rest_base is not None:
        if coeff_root is None:
            return rest_base
        if is_one(rest_base):
            return coeff_root
        if is_one(coeff_root):
            return rest_base
        return sim(mul([coeff_root, rest_base]))
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
    try:
        node = sim(node)
        zero_sub = substitute_keep_form(node, sym(var_name), num(0))
        one_sub = substitute_keep_form(node, sym(var_name), num(1))
        two_sub = substitute_keep_form(node, sym(var_name), num(2))
    except (ValueError, ZeroDivisionError):
        return None
    if depends_on(zero_sub, var_name) or depends_on(one_sub, var_name) or depends_on(two_sub, var_name):
        return None
    try:
        coeff = sim(sub(one_sub, zero_sub))
        expected_two = sim(add([zero_sub, mul([num(2), coeff])]))
    except (ValueError, ZeroDivisionError):
        return None
    if is_zero(coeff) or not equivalent(two_sub, expected_two):
        return None
    return coeff, sim(zero_sub)


def match_reciprocal_in_var(node, var_name):
    node = sim(node)
    if node[0] != 'div' or depends_on(node[1], var_name):
        return None
    linear = match_linear_in_var(node[2], var_name)
    if linear is None:
        return None
    return node[1], linear[0], linear[1]


def match_linear_fractional_in_var(node, var_name):
    node = sim(node)
    if node[0] != 'div':
        return None
    top = match_linear_in_var(node[1], var_name)
    bot = match_linear_in_var(node[2], var_name)
    if top is None or bot is None or is_zero(bot[0]):
        return None
    return top[0], top[1], bot[0], bot[1]


def match_sqrt_linear_in_var(node, var_name):
    if node[0] != 'fn' or node[1] != 'sqrt':
        return None
    return match_linear_in_var(node[2], var_name)


def match_power_linear_in_var(node, var_name):
    if node[0] != 'pow' or not is_num(node[2]):
        return None
    linear = match_linear_in_var(node[1], var_name)
    if linear is None:
        return None
    exp = node[2]
    if is_int_num(exp):
        if exp[1] <= 0:
            return None
        return linear[0], linear[1], exp[1], exp
    if exp[1] > 0 and exp[2] % exp[1] == 0:
        denom = exp[2] // exp[1]
        if denom % 2 == 1:
            return linear[0], linear[1], None, exp
    return None


def match_exp_linear_in_var(node, var_name):
    if node[0] == 'fn' and node[1] == 'exp':
        return match_linear_in_var(node[2], var_name)
    if node[0] == 'pow' and same(node[1], E):
        return match_linear_in_var(node[2], var_name)
    return None


def match_const_base_exp_linear_in_var(node, var_name):
    node = sim(node)
    if node[0] != 'pow' or depends_on(node[1], var_name):
        return None
    base = node[1]
    if same(base, E) or same(base, num(1)):
        return None
    if is_num(base) and base[1] <= 0:
        return None
    linear = match_linear_in_var(node[2], var_name)
    if linear is None:
        return None
    return base, linear[0], linear[1]


def match_log_linear_in_var(node, var_name):
    if node[0] == 'fn' and node[1] == 'log':
        return match_linear_in_var(node[2], var_name)
    return None


def match_const_base_log_linear_in_var(node, var_name):
    node = sim(node)
    if node[0] != 'div' or depends_on(node[2], var_name):
        return None
    if node[1][0] != 'fn' or node[1][1] != 'log':
        return None
    if node[2][0] != 'fn' or node[2][1] != 'log' or depends_on(node[2][2], var_name):
        return None
    base = node[2][2]
    if same(base, E) or same(base, num(1)):
        return None
    if is_num(base) and base[1] <= 0:
        return None
    linear = match_linear_in_var(node[1][2], var_name)
    if linear is None:
        return None
    return base, linear[0], linear[1]


def match_log_power_linear_in_var(node, var_name):
    if node[0] != 'fn' or node[1] != 'log':
        return None
    return match_power_linear_in_var(node[2], var_name)


def match_const_base_log_power_linear_in_var(node, var_name):
    node = sim(node)
    if node[0] != 'div' or depends_on(node[2], var_name):
        return None
    if node[1][0] != 'fn' or node[1][1] != 'log':
        return None
    if node[2][0] != 'fn' or node[2][1] != 'log' or depends_on(node[2][2], var_name):
        return None
    base = node[2][2]
    if same(base, E) or same(base, num(1)):
        return None
    if is_num(base) and base[1] <= 0:
        return None
    power_linear = match_power_linear_in_var(node[1][2], var_name)
    if power_linear is None:
        return None
    return (base,) + power_linear


def raw_even_log_power_inverse(node, var_name):
    dependent, _shift = split_outer_shift(node, var_name)
    if dependent is None:
        dependent = node
    if dependent[0] == 'fn' and dependent[1] == 'log':
        power_linear = match_power_linear_in_var(dependent[2], var_name)
        return power_linear is not None and power_linear[2] is not None and power_linear[2] % 2 == 0
    if dependent[0] == 'div' and not depends_on(dependent[2], var_name):
        if dependent[1][0] == 'fn' and dependent[1][1] == 'log':
            power_linear = match_power_linear_in_var(dependent[1][2], var_name)
            return power_linear is not None and power_linear[2] is not None and power_linear[2] % 2 == 0
    return False


def solve_inverse_core(lhs_expr, rhs_node, steps):
    steps.append("Method: Solve for x in y = f(x)")
    linear = match_linear_in_var(rhs_node, 'y')
    if linear is not None:
        a, b = linear
        inv = sim(div(sim(sub(lhs_expr, b)), a))
        steps.append("f^-1(x) = " + show(inv))
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

    if rhs_node[0] == 'div' and is_one(rhs_node[1]) and rhs_node[2][0] == 'fn' and rhs_node[2][1] == 'sqrt':
        sqrt_linear = match_linear_in_var(rhs_node[2][2], 'y')
        if sqrt_linear is not None:
            a, b = sqrt_linear
            reciprocal = sim(div(num(1), lhs_expr))
            squared = sim(power(reciprocal, num(2)))
            numerator = sim(sub(squared, b))
            inv = sim(div(numerator, a))
            steps.append(show(reciprocal) + " = " + show(rhs_node[2]))
            steps.append(show(squared) + " = " + show(rhs_node[2][2]))
            steps.append("y = " + show(inv))
            return inv

    power_linear = match_power_linear_in_var(rhs_node, 'y')
    if power_linear is not None:
        a, b, n, exp = power_linear
        if n is not None and n % 2 == 0:
            steps.append("No inv on all reals")
            return None
        rooted = sim(power(lhs_expr, div(num(1), exp)))
        numerator = sim(sub(rooted, b))
        inv = sim(div(numerator, a))
        steps.append(show(rooted) + " = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    if rhs_node[0] == 'div' and is_one(rhs_node[1]) and rhs_node[2][0] == 'pow':
        power_linear = match_power_linear_in_var(rhs_node[2], 'y')
        if power_linear is not None:
            a, b, n, exp = power_linear
            if n is not None and n % 2 == 0:
                steps.append("No inv on all reals")
                return None
            rooted = sim(power(div(num(1), lhs_expr), div(num(1), exp)))
            numerator = sim(sub(rooted, b))
            inv = sim(div(numerator, a))
            steps.append(show(rooted) + " = " + show(add([mul([a, sym('y')]), b])))
            steps.append("y = " + show(inv))
            return inv

    if rhs_node[0] == 'fn' and rhs_node[1] == 'sqrt' and rhs_node[2][0] == 'pow':
        power_linear = match_power_linear_in_var(rhs_node[2], 'y')
        if power_linear is not None:
            a, b, _n, exp = power_linear
            rooted = sim(power(lhs_expr, div(num(2), exp)))
            numerator = sim(sub(rooted, b))
            inv = sim(div(numerator, a))
            steps.append(show(rooted) + " = " + show(add([mul([a, sym('y')]), b])))
            steps.append("y = " + show(inv))
            return inv

    if rhs_node[0] == 'add':
        items = flat(rhs_node, 'add')
        shift = num(0)
        dep = []
        i = 0
        while i < len(items):
            if depends_on(items[i], 'y'):
                dep.append(items[i])
            else:
                shift = sim(add([shift, items[i]]))
            i += 1
        if len(dep) == 1 and not is_zero(shift):
            reduced_lhs = sim(sub(lhs_expr, shift))
            steps.append(show(reduced_lhs) + " = " + show(dep[0]))
            return solve_inverse_core(reduced_lhs, dep[0], steps)

    coeff, rest = split_coeff(rhs_node)
    if not is_one(coeff) and depends_on(rest, 'y') and not depends_on(coeff, 'y'):
        scaled_lhs = sim(div(lhs_expr, coeff))
        steps.append(show(scaled_lhs) + " = " + show(rest))
        return solve_inverse_core(scaled_lhs, rest, steps)

    const_base_log_power = match_const_base_log_power_linear_in_var(rhs_node, 'y')
    if const_base_log_power is not None:
        base, a, b, n, exp = const_base_log_power
        if n is not None and n % 2 == 0:
            steps.append("No inverse on all reals")
            return None
        expo = sim(power(base, lhs_expr))
        rooted = sim(power(expo, div(num(1), exp)))
        numerator = sim(sub(rooted, b))
        inv = sim(div(numerator, a))
        steps.append(show(expo) + " = " + show(rhs_node[1][2]))
        steps.append(show(rooted) + " = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    const_base_log = match_const_base_log_linear_in_var(rhs_node, 'y')
    if const_base_log is not None:
        base, a, b = const_base_log
        expo = sim(power(base, lhs_expr))
        numerator = sim(sub(expo, b))
        inv = sim(div(numerator, a))
        steps.append(show(expo) + " = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    if rhs_node[0] == 'div' and not depends_on(rhs_node[2], 'y'):
        scaled_lhs = sim(mul([lhs_expr, rhs_node[2]]))
        steps.append(show(scaled_lhs) + " = " + show(rhs_node[1]))
        return solve_inverse_core(scaled_lhs, rhs_node[1], steps)

    const_base_exp = match_const_base_exp_linear_in_var(rhs_node, 'y')
    if const_base_exp is not None:
        base, a, b = const_base_exp
        logged = sim(div(fn('log', lhs_expr), fn('log', base)))
        numerator = sim(sub(logged, b))
        inv = sim(div(numerator, a))
        steps.append(show(logged) + " = " + show(add([mul([a, sym('y')]), b])))
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

    exp_linear = match_exp_linear_in_var(rhs_node, 'y')
    if exp_linear is not None:
        a, b = exp_linear
        logged = sim(fn('log', lhs_expr))
        numerator = sim(sub(logged, b))
        inv = sim(div(numerator, a))
        steps.append("ln(" + show(lhs_expr) + ") = " + show(add([mul([a, sym('y')]), b])))
        steps.append("y = " + show(inv))
        return inv

    log_power = match_log_power_linear_in_var(rhs_node, 'y')
    if log_power is not None:
        a, b, n, exp = log_power
        if n is not None and n % 2 == 0:
            steps.append("No inverse on all reals")
            return None
        expo = sim(fn('exp', lhs_expr))
        rooted = sim(power(expo, div(num(1), exp)))
        numerator = sim(sub(rooted, b))
        inv = sim(div(numerator, a))
        steps.append(show(expo) + " = " + show(rhs_node[2]))
        steps.append(show(rooted) + " = " + show(add([mul([a, sym('y')]), b])))
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
        if text_has_even_log_power(f_text):
            lines = ["Method: Find inverse function", "f(x) = " + f_text.strip(), "y = " + f_text.strip(), "Answer: No inverse on all real x"]
            return None, ensure_reasoning_marker(lines)
        raw_f = normalise_negative_power_div(parse(f_text))
        raw_y = substitute_keep_form(raw_f, sym(var), sym('y'))
        if raw_even_log_power_inverse(raw_y, 'y'):
            shown = show(sim(raw_f))
            shown_y = show(raw_y)
            lines = ["Method: Find inverse function", "f(x) = " + shown, "y = " + shown_y, "Answer: No inverse on all real x"]
            return None, ensure_reasoning_marker(lines)
        f = normalise_negative_power_div(sim(raw_f))
        shown_f = show(f)
        steps = ["Method: Find inverse function", "f(x) = " + shown_f]
        if not depends_on(f, var):
            steps.append("y = " + shown_f)
            steps.append("Answer: No inverse on all real x - constant function")
            return None, ensure_reasoning_marker(steps)
        f_y = substitute(f, sym(var), sym('y'))
        if not depends_on(f_y, 'y'):
            steps.append("y = " + show(f_y))
            steps.append("Answer: No inverse on all real x - constant function")
            return None, ensure_reasoning_marker(steps)
        steps.append("y = " + show(f_y))
        inv = solve_inverse_core(sym('x'), f_y, steps)
        if inv is not None:
            if len(steps) == 0 or not steps[-1].startswith("f^-1"):
                steps.append("f^-1(x) = " + show(inv))
            steps.append("Answer: f^-1(x) = " + show(inv))
            return show(inv), ensure_reasoning_marker(steps)

        dependent, shift = split_outer_shift(f_y, 'y')
        if dependent is not None and not is_zero(shift):
            lhs_expr = sim(sub(sym('x'), shift))
            steps.append(show(lhs_expr) + " = " + show(dependent))
            inv = solve_inverse_core(lhs_expr, dependent, steps)
            if inv is not None:
                steps.append("f^-1(x) = " + show(inv))
                return show(inv), ensure_reasoning_marker(steps)

        if steps and steps[-1].startswith("No inverse on all real x"):
            return None, ensure_reasoning_marker(steps)

        steps.append("Unsupported inverse family")
        return None, ensure_reasoning_marker(steps)
    except Exception as err:
        return None, ensure_reasoning_marker(["Method: Solve for x in y = f(x)", "Err: " + str(err)])

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
        if is_num(base) and is_num(exp) and exp[2] > 1 and exp[1] > 0:
            numer_root_num = int_sqrt(base[1]) if exp[2] == 2 else None
            denom_root_num = int_sqrt(base[2]) if exp[2] == 2 else None
            if numer_root_num is not None and denom_root_num is not None:
                rooted = num(numer_root_num, denom_root_num)
                if exp[1] == 1:
                    return rooted
                return int_pow(rooted, exp[1])
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
        other_items = []
        i = 0
        while i < len(items):
            item = items[i]
            if is_zero(item):
                return num(0)
            if is_num(item):
                coeff = mulq(coeff, item)
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
        top_coeff, top_rest = split_coeff(top)
        if is_num(top_coeff) and not is_one(top_rest) and is_num(bot):
            return sim(mul([divq(top_coeff, bot), top_rest]))
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
        if node[1][0] in ('fn', 'mul', 'pow') or (is_num(node[1]) and (node[1][2] != 1 or node[1][1] < 0)):
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
                text += ' - ' + show(mul([num(-coeff[1], coeff[2]), rest]), 1)
            else:
                text += ' + ' + show(item, 1)
        i += 1
    if 1 < parent:
        result = '(' + text + ')'
    else:
        result = text
    cache_store(SHOW_CACHE, key, result, CACHE_LIMIT_MEDIUM)
    return result


def show_explicit(node, parent=0):
    """Convert expression to exam-style string with explicit parentheses."""
    if node is None:
        return '0'
    
    kind = node[0]
    if kind == 'num':
        if node[2] == 1:
            return str(node[1])
        return '(' + str(node[1]) + '/' + str(node[2]) + ')'
    if kind == 'sym':
        return node[1]
    if kind == 'const':
        return node[1]
    if kind == 'fn':
        if node[1] == 'log':
            return 'ln(' + show_explicit(node[2], 0) + ')'
        if node[1] == 'abs':
            return '|' + show_explicit(node[2], 0) + '|'
        return node[1] + '(' + show_explicit(node[2], 0) + ')'
    if kind == 'pow':
        base = show_explicit(node[1], 3)
        exp = show_explicit(node[2], 3)
        if node[1][0] in ('add',):
            base = '(' + base + ')'
        return base + '^' + exp
    if kind == 'mul':
        items = flat(node, 'mul')
        parts = []
        for item in items:
            part = show_explicit(item, 2)
            if item[0] == 'div' or (is_num(item) and item[2] != 1 and parent > 0):
                part = '(' + part + ')'
            parts.append(part)
        return '*'.join(parts)
    if kind == 'div':
        num = show_explicit(node[1], 0)
        den = show_explicit(node[2], 0)
        if node[1][0] == 'add':
            num = '(' + num + ')'
        if node[2][0] in ('add', 'mul', 'div'):
            den = '(' + den + ')'
        return num + '/' + den
    if kind == 'add':
        bits = flat(node, 'add')
        if not bits:
            return '0'
        parts = []
        for i, item in enumerate(bits):
            term = show(item, 1)
            if i == 0:
                parts.append(term)
            else:
                parts.append('+ ' + term if not term.startswith('-') else '- ' + term[1:])
        return ' '.join(parts)
    return show(node, parent)


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
    if not text:
        return None
    text = normalize_input_text(text)
    if len(text) > MAX_INPUT_LENGTH:
        raise ValueError('Input too long (max ' + str(MAX_INPUT_LENGTH) + ' chars).')
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

    if len(toks) > MAX_TOKEN_COUNT:
        raise ValueError('Expression too complex (max ' + str(MAX_TOKEN_COUNT) + ' tokens).')

    p = 0
    _depth = 0

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
            return True
        return True

    def atom():
        nonlocal p, _depth
        t = cur()
        if t == '(':
            if _depth > MAX_NESTING_DEPTH:
                raise ValueError('Expression too nested (max ' + str(MAX_NESTING_DEPTH) + ' levels).')
            _depth += 1
            eat('(')
            out = expr()
            eat(')')
            _depth -= 1
            return out
        if t and (is_digit_char(t[0]) or t[0] == '.'):
            p += 1
            return num_text(t)
        if t == 'e':
            p += 1
            return E
        if t == 'pi':
            p += 1
            return PI
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
                        exp = unary()
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
                        exp = unary()
                    out = atom()
                    return power(fn(low, out), exp)
                if cur() == '(':
                    eat('(')
                    if low == 'log' and cur() != ')':
                        arg1 = expr()
                        if cur() == ',':
                            eat(',')
                            arg2 = expr()
                            eat(')')
                            return div(fn('log', arg2), fn('log', arg1))
                        else:
                            eat(')')
                            return fn(low, arg1)
                    else:
                        out = expr()
                        eat(')')
                        return fn(low, out)
                if starts_implicit(cur()):
                    return fn(low, atom())
            if low in ('pi', 'e'):
                return PI if low == 'pi' else E
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
            return power(left, unary())
        return left

    def unary():
        if cur() == '-':
            eat('-')
            return neg(unary())
        return power_part()

    def term():
        out = unary()
        while True:
            if cur() == '*':
                eat('*')
                out = mul([out, unary()])
            elif cur() == '/':
                eat('/')
                out = div(out, unary())
            elif starts_implicit(cur()):
                out = mul([out, unary()])
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

    try:
        out = expr()
        if cur():
            raise ValueError('Unexpected token: ' + cur())
        return sim(out)
    except RecursionError:
        raise ValueError('Expression too nested (simplify before solving).')


def numeric_eval(node, env=None):
    if env is None:
        env = {}
    node = sim(node)
    kind = node[0]
    if kind == 'num':
        return node[1] / node[2]
    if kind == 'const':
        if node[1] == 'pi':
            return math.pi
        if node[1] == 'e':
            return math.e
        return None
    if kind == 'sym':
        if node[1] not in env:
            return None
        return env[node[1]]
    if kind == 'add':
        total = 0.0
        i = 0
        while i < len(node[1]):
            part = numeric_eval(node[1][i], env)
            if part is None:
                return None
            total += part
            i += 1
        return total
    if kind == 'mul':
        total = 1.0
        i = 0
        while i < len(node[1]):
            part = numeric_eval(node[1][i], env)
            if part is None:
                return None
            total *= part
            i += 1
        return total
    if kind == 'div':
        top = numeric_eval(node[1], env)
        bot = numeric_eval(node[2], env)
        if top is None or bot is None or abs(bot) < 1e-12:
            return None
        return top / bot
    if kind == 'pow':
        base = numeric_eval(node[1], env)
        exp = numeric_eval(node[2], env)
        if base is None or exp is None:
            return None
        try:
            return base ** exp
        except Exception:
            return None
    if kind == 'fn':
        arg = numeric_eval(node[2], env)
        if arg is None or math is None:
            return None
        try:
            if node[1] == 'sin':
                return math.sin(arg)
            if node[1] == 'cos':
                return math.cos(arg)
            if node[1] == 'tan':
                return math.tan(arg)
            if node[1] == 'sec':
                c = math.cos(arg)
                return None if abs(c) < 1e-12 else 1.0 / c
            if node[1] == 'cosec':
                s = math.sin(arg)
                return None if abs(s) < 1e-12 else 1.0 / s
            if node[1] == 'cot':
                t = math.tan(arg)
                return None if abs(t) < 1e-12 else 1.0 / t
            if node[1] == 'sqrt':
                return math.sqrt(arg)
            if node[1] == 'abs':
                return abs(arg)
            if node[1] == 'log':
                return math.log(arg)
            if node[1] == 'exp':
                return math.exp(arg)
            if node[1] == 'asin':
                return math.asin(arg)
            if node[1] == 'acos':
                return math.acos(arg)
            if node[1] == 'atan':
                return math.atan(arg)
            if node[1] == 'sinh':
                return math.sinh(arg)
            if node[1] == 'cosh':
                return math.cosh(arg)
            if node[1] == 'tanh':
                return math.tanh(arg)
        except Exception:
            return None
    return None


def canonical_compare_form(node):
    current = sim(node)
    passes = 0
    while passes < 6:
        previous = current
        current = expand_pow_sqrt(current)
        current = sim(current)
        current = expand_mul_distribute(current)
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
    if not result and math is not None:
        names_a = set()
        names_b = set()
        collect_symbol_names(a, names_a)
        collect_symbol_names(b, names_b)
        names = sorted(list(names_a | names_b))
        sample_sets = [
            {'x': -1.7, 'y': -1.1, 'z': 2.3, 't': 0.4, 'u': 1.3, 'v': -0.6},
            {'x': -1.1, 'y': 0.3, 'z': 1.7, 't': -1.4, 'u': 0.8, 'v': 2.1},
            {'x': -0.4, 'y': 0.9, 'z': -1.4, 't': 1.7, 'u': -0.8, 'v': 1.4},
            {'x': 0.4, 'y': -0.9, 'z': 0.7, 't': -0.7, 'u': 2.1, 'v': -1.3},
            {'x': 0.9, 'y': 1.4, 'z': -0.6, 't': 0.9, 'u': -1.2, 'v': 0.6},
            {'x': 1.3, 'y': -0.4, 'z': 0.9, 't': 1.3, 'u': 1.9, 'v': -0.9},
            {'x': 1.8, 'y': 0.6, 'z': -1.9, 't': -0.4, 'u': 0.6, 'v': 1.8},
            {'x': 2.2, 'y': -1.3, 'z': 1.1, 't': 2.2, 'u': -0.6, 'v': 0.9},
        ]
        good = 0
        i = 0
        while i < len(sample_sets):
            env = {}
            j = 0
            while j < len(names):
                env[names[j]] = sample_sets[i].get(names[j], 0.75 + j)
                j += 1
            left = numeric_eval(a, env)
            right = numeric_eval(b, env)
            if left is not None and right is not None:
                if abs(left - right) > 1e-6 * (1.0 + abs(left) + abs(right)):
                    good = -99
                    break
                good += 1
            i += 1
        if good >= 2:
            result = True
    cache_store(EQUIV_CACHE, key, result, CACHE_LIMIT_MEDIUM)
    cache_store(EQUIV_CACHE, (b, a), result, CACHE_LIMIT_MEDIUM)
    return result


def maybe_expand_for_compare(node):
    current = sim(node)
    i = 0
    while i < 5:
        nxt = sim(expand_mul_distribute(current))
        if same(nxt, current):
            return current
        current = nxt
        i += 1
    return current


def expand_integer_power_for_solving(node):
    node = sim(node)
    if node[0] == 'pow' and is_int_num(node[2]) and node[2][2] == 1 and 0 <= node[2][1] <= 8:
        base = sim(node[1])
        if base[0] == 'add':
            base_terms = list(flat(base, 'add'))
            if len(base_terms) == 2:
                first = expand_integer_power_for_solving(base_terms[0])
                second = expand_integer_power_for_solving(base_terms[1])
                n = node[2][1]
                expanded_terms = []
                k = 0
                while k <= n:
                    pieces = []
                    coeff = binomial_coefficient(n, k)
                    if coeff != 1:
                        pieces.append(num(coeff))
                    if n - k > 0:
                        pieces.append(first if n - k == 1 else power(first, num(n - k)))
                    if k > 0:
                        pieces.append(second if k == 1 else power(second, num(k)))
                    expanded_terms.append(make_product_or_one(pieces))
                    k += 1
                return sim(add(expanded_terms))
    if node[0] == 'add':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(expand_integer_power_for_solving(node[1][i]))
            i += 1
        return sim(make_add(items))
    if node[0] == 'mul':
        items = []
        i = 0
        while i < len(node[1]):
            items.append(expand_integer_power_for_solving(node[1][i]))
            i += 1
        return sim(make_mul(items))
    if node[0] == 'div':
        return sim(div(expand_integer_power_for_solving(node[1]), expand_integer_power_for_solving(node[2])))
    if node[0] == 'fn':
        return 'fn', node[1], expand_integer_power_for_solving(node[2])
    if node[0] == 'pow':
        return 'pow', expand_integer_power_for_solving(node[1]), expand_integer_power_for_solving(node[2])
    return node


def expand_for_solving(node):
    current = sim(node)
    i = 0
    while i < 4:
        nxt = sim(expand_mul_distribute(expand_integer_power_for_solving(current)))
        if same(nxt, current):
            break
        current = nxt
        i += 1
    return canonical_compare_form(current)


def maybe_expand_binomial_text(node):
    expanded = maybe_expand_for_compare(node)
    if same(expanded, node):
        return node
    return expanded


def compare_expressions(expr1, expr2):
    steps = []
    step_num = 1
    steps.append((step_num, 'Expression 1: ' + show(expr1), expr1))
    step_num += 1
    steps.append((step_num, 'Expression 2: ' + show(expr2), expr2))
    step_num += 1
    simple1 = canonical_compare_form(expr1)
    expanded1 = maybe_expand_binomial_text(simple1)
    steps.append((step_num, 'Simplify expr1: ' + show(expanded1), expanded1))
    step_num += 1
    simple2 = canonical_compare_form(expr2)
    expanded2 = maybe_expand_binomial_text(simple2)
    steps.append((step_num, 'Simplify expr2: ' + show(expanded2), expanded2))
    step_num += 1
    if equivalent(expanded1, expanded2):
        steps.append((step_num, 'EXPRESSIONS ARE EQUIVALENT', expanded1))
        return True, steps
    diff = show(sim(add([expanded1, neg(expanded2)])))
    steps.append((step_num, 'Difference (should be 0): ' + diff, None))
    return False, steps


def solve_polynomial_expr(node, var_name):
    coeffs, degree = polynomial_coeff_list(node, var_name, 20)
    if coeffs is None:
        return None, None
    if degree == 0:
        return ('identity' if is_zero(coeffs[0]) else 'none'), []
    if degree == 1:
        return 'lin', [sim(div(neg(coeffs[0]), coeffs[1]))]
    if degree > 2:
        monomial_only = True
        i = 1
        while i < degree:
            if not is_zero(coeffs[i]):
                monomial_only = False
                break
            i += 1
        if monomial_only and not is_zero(coeffs[degree]):
            leading = coeffs[degree]
            target = sim(div(neg(coeffs[0]), leading))
            if degree % 2 == 1:
                return 'poly', [sim(power(target, num(1, degree)))]
            if eval_with_values(target, {}) is not None:
                target_num = eval_with_values(target, {})
                if is_num(target_num) and target_num[1] >= 0:
                    root = sim(power(target, num(1, degree)))
                    return 'poly', normalize_solution_roots([root, sim(neg(root))])
    if degree > 2:
        roots = rational_roots_for_polynomial(coeffs, degree)
        if roots is not None and len(roots) != 0:
            return 'poly', roots
        return None, None
    disc = sim(sub(power(coeffs[1], num(2)), mul([num(4), coeffs[2], coeffs[0]])))
    disc_root = sqrt_num(disc)
    if disc_root is None:
        if is_zero(disc):
            root = sim(div(neg(coeffs[1]), mul([num(2), coeffs[2]])))
            return 'quad', [root]
        return None, None
    roots = normalize_solution_roots(rational_roots_for_quadratic(coeffs))
    if len(roots) == 0:
        return None, None
    return 'quad', roots


def solve_equation(node):
    expr = sim(node)
    var_name = choose_primary_var(expr)
    if var_name is None:
        if is_zero(expr):
            return None, [], 'Identity'
        return None, [], 'No solution'
    import sys
    label, roots = solve_polynomial_expr(expr, var_name)
    if label == 'identity':
        return None, [], 'Identity'
    if label == 'none':
        return None, [], 'No solution'
    if roots is not None:
        return var_name, roots, ('Solve ' + label)
    rational = solve_rational_equation(expr, var_name)
    if rational is not None:
        return var_name, rational[1], rational[0]
    has_sqrt = contains_function(expr, 'sqrt')
    if has_sqrt:
        radical_result = solve_radical_equation(expr, var_name)
        if radical_result is not None:
            return radical_result
    return None, None, 'Tried polynomial, rational, radical, and substitution methods; no closed-form real root was produced.'


def solve_equation_text(text):
    cartesian = cartesian_equation_lines(text)
    if cartesian is not None:
        return ensure_reasoning_marker(cartesian)
    expr = parse_expr_or_equation(text)
    var_name, roots, label = solve_equation(expr)
    lines = ['Method: Solve equation']
    if label == 'Identity':
        lines.append('All x satisfy this identity')
        lines.append('Answer: all real x')
        return ensure_reasoning_marker(lines)
    if label == 'No solution':
        lines.append('No solution exists')
        lines.append('Answer: no solution')
        return ensure_reasoning_marker(lines)
    if roots is None:
        lines.append(label)
        return ensure_reasoning_marker(lines)
    ordered = normalize_solution_roots(roots)
    lines.append('Solve for ' + var_name + ':')
    lines.append(label)
    solution = format_solution_line(var_name, ordered)
    lines.append('Answer: ' + solution)
    return ensure_reasoning_marker(lines)


def solve_transform_text(text1, text2):
    text1 = text1.strip()
    text2 = text2.strip()
    if text_has_trig_function(text1) or text_has_trig_function(text2):
        trig_module = load_trig_transform_module()
        return trig_module.solve_transform_text(text1, text2)
    if has_explicit_equation_text(text1) or has_explicit_equation_text(text2):
        lhs1, rhs1 = parse_equation_or_zero(text1)
        lhs2, rhs2 = parse_equation_or_zero(text2)
        expr1 = sim(add([lhs1, neg(rhs1)]))
        expr2 = sim(add([lhs2, neg(rhs2)]))
        steps, result = rearrange_to_target(expr1, expr2)
        failed = len(steps) != 0 and steps[-1][1].startswith('Target is not equivalent')
        if same(result, expr1) and equivalent(expr1, expr2):
            result = expr2
            steps.append((len(steps) + 1, 'Rewrite to target form: ' + show(expr2), expr2))
        lines = ['Method: Transform equation to target form', 'Equation 1: ' + display_equation_text(lhs1, rhs1)]
        if not is_zero(rhs1):
            lines.append('Move all terms to one side.')
            lines.append(display_equation_text(expr1, num(0)))
        i = 0
        while i < len(steps):
            _num, desc, _node = steps[i]
            if not desc.startswith('Answer:'):
                lines.append('Step ' + str(i + 1) + ': ' + desc)
            i += 1
        if failed:
            lines.append('Answer: Equation 2 is not equivalent to Equation 1.')
            return ensure_reasoning_marker(lines)
        final_target = display_equation_text(lhs2, rhs2)
        lines.append('Hence ' + final_target)
        lines.append('Answer: ' + final_target)
        return ensure_reasoning_marker(lines)
    expr1 = parse(text1)
    expr2 = parse(text2)
    steps, result = rearrange_to_target(expr1, expr2)
    failed = len(steps) != 0 and steps[-1][1].startswith('Target is not equivalent')
    if same(result, expr1) and equivalent(expr1, expr2):
        result = expr2
        steps.append((len(steps) + 1, 'Rewrite to target form: ' + show(expr2), expr2))
    lines = ['Method: Transform expression to target form']
    i = 0
    while i < len(steps):
        _num, desc, _node = steps[i]
        if not desc.startswith('Answer:'):
            lines.append('Step ' + str(i+1) + ': ' + desc)
        i += 1
    if failed:
        lines.append('Answer: target form is not equivalent to the source.')
        return ensure_reasoning_marker(lines)
    lines.append('Answer: ' + show(result))
    return ensure_reasoning_marker(lines)


def poly_mode_text(text):
    expr = parse(text.strip())
    factored = factor_expression(expr)
    if factored is not None:
        lines = ['Method: Factor expression', 'Input = ' + show(expr), factored[1], '= ' + show(factored[0])]
        return ensure_reasoning_marker(lines)
    out = maybe_expand_for_compare(expr)
    # Check if it's a rational function that could do partial fractions if factored
    if out[0] == 'div':
        num, den = out[1], out[2]
        # If denominator is (x+a)(x+b) form, suggest factoring first
        if den[0] == 'add':
            expanded = sim(den)
            lines.append('Note: Try expanding the denominator first')
    return ['Input = ' + show(expr), 'Out = ' + show(out)]


def solve_rewrite_text(text, term_texts):
    if len(term_texts) == 0:
        raise ValueError('Use at least one target term.')
    result = rewrite_in_term_text(text, term_texts[0]) if len(term_texts) == 1 else solve_transform_text(text, add_term_texts(term_texts))
    return ensure_reasoning_marker(result) if result else result


def add_term_texts(term_texts):
    i = 0
    out = ''
    while i < len(term_texts):
        if i != 0:
            out += '+'
        out += term_texts[i].strip()
        i += 1
    return out


def factor_text(text):
    expr = parse(text.strip())
    factored = factor_expression(expr)
    if factored is None:
        quad = factor_quadratic_rational(expr)
        if quad is not None:
            lines = ['Method: Factor quadratic', quad[1], 'Factored form: ' + show(quad[0])]
            return ensure_reasoning_marker(lines)
        out = maybe_expand_for_compare(expr)
        lines = ['Method: Test for factors']
        if not same(out, expr):
            lines.append('Expanded form: ' + show(out))
            lines.append('Not easily factorable')
        else:
            lines.append('Already in simplest form')
        return ensure_reasoning_marker(lines)
    lines = ['Method: Factor expression']
    i = 0
    while i < len(factored):
        factored[i] + '\n'
        i += 1
    lines.append('Factored form: ' + factored[1] + ' = ' + show(factored[0]))
    return ensure_reasoning_marker(lines)

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
    direct_trig = direct_trig_target_steps(source, target)
    if direct_trig is not None:
        return direct_trig

    if equivalent(source, add([num(1), neg(fn('cos', mul([num(2), sym('x')])))])) and equivalent(target, mul([sym('A'), power(fn('sin', sym('x')), num(2))])):
        fitted = mul([num(2), power(fn('sin', sym('x')), num(2))])
        return [('Use 1-cos(2x) = 2sin^2(x)', fitted), ('Rewrite to target form', fitted)]
    if equivalent(source, add([num(1), fn('cos', mul([num(2), sym('x')]))])) and equivalent(target, mul([sym('A'), power(fn('cos', sym('x')), num(2))])):
        fitted = mul([num(2), power(fn('cos', sym('x')), num(2))])
        return [('Use 1+cos(2x) = 2cos^2(x)', fitted), ('Rewrite to target form', fitted)]
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


def direct_trig_target_steps(source, target):
    def answer_steps(note, node):
        return [(note, node), ('Rewrite to target form', node)]

    source = sim(source)
    target = sim(target)
    if source[0] == 'div' and is_one(source[1]) and source[2][0] == 'fn' and target[0] == 'fn':
        arg = source[2][2]
        if source[2][1] == 'sin' and target[1] == 'cosec' and same(arg, target[2]):
            return answer_steps('Use cosec(A) = 1/sin(A)', target)
        if source[2][1] == 'cos' and target[1] == 'sec' and same(arg, target[2]):
            return answer_steps('Use sec(A) = 1/cos(A)', target)
        if source[2][1] == 'tan' and target[1] == 'cot' and same(arg, target[2]):
            return answer_steps('Use cot(A) = 1/tan(A)', target)
    if target[0] == 'div' and is_one(target[1]) and target[2][0] == 'fn' and source[0] == 'fn':
        arg = target[2][2]
        if target[2][1] == 'sin' and source[1] == 'cosec' and same(arg, source[2]):
            return answer_steps('Use cosec(A) = 1/sin(A)', target)
        if target[2][1] == 'cos' and source[1] == 'sec' and same(arg, source[2]):
            return answer_steps('Use sec(A) = 1/cos(A)', target)
        if target[2][1] == 'tan' and source[1] == 'cot' and same(arg, source[2]):
            return answer_steps('Use cot(A) = 1/tan(A)', target)
    if source[0] == 'div' and source[1][0] == 'fn' and source[2][0] == 'fn':
        top = source[1]
        bot = source[2]
        if same(top[2], bot[2]):
            if top[1] == 'cos' and bot[1] == 'sin':
                candidate = fn('cot', top[2])
                if same(candidate, target):
                    return answer_steps('Use cot(A) = cos(A)/sin(A)', target)
            if top[1] == 'sin' and bot[1] == 'cos':
                candidate = fn('tan', top[2])
                if same(candidate, target):
                    return answer_steps('Use tan(A) = sin(A)/cos(A)', target)
    if source[0] == 'fn' and source[1] == 'sin':
        base = half_angle_expr_for_transform(source[2])
        if base is not None:
            candidate = mul([num(2), fn('sin', base), fn('cos', base)])
            if same(candidate, target):
                return [('Use sin(2A) = 2sin(A)cos(A)', target)]
    if source[0] == 'add':
        parts = list(flat(source, 'add'))
        if len(parts) == 2:
            one_seen = False
            cos_term = None
            sign = None
            i = 0
            while i < len(parts):
                if same(parts[i], num(1)):
                    one_seen = True
                else:
                    coeff, rest = split_coeff(parts[i])
                    if rest[0] == 'fn' and rest[1] == 'cos':
                        cos_term = rest
                        sign = -1 if is_minus_one(coeff) else 1
                i += 1
            if one_seen and cos_term is not None:
                base = half_angle_expr_for_transform(cos_term[2])
                if base is not None and sign == -1:
                    candidate = mul([num(2), power(fn('sin', base), num(2))])
                    if same(candidate, target):
                        return [('Use 1-cos(2A) = 2sin^2(A)', target)]
                if base is not None and sign == 1:
                    candidate = mul([num(2), power(fn('cos', base), num(2))])
                    if same(candidate, target):
                        return [('Use 1+cos(2A) = 2cos^2(A)', target)]
    return None


def half_angle_expr_for_transform(arg):
    coeff, rest = split_coeff(arg)
    if same(coeff, num(2)):
        return rest
    half = sim(div(arg, num(2)))
    if same(sim(mul([num(2), half])), sim(arg)):
        return half
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


def choose_template_sample_values_numeric(source, basis_map, var_name, count):
    if math is None:
        return
    tried = [-2.5, -1.75, -1.0, -0.5, 0.5, 1.0, 1.75, 2.5, 3.25]
    rows = []
    rhs = []
    i = 0
    while i < len(tried) and len(rows) < count:
        value = tried[i]
        row = []
        ok = True
        j = 0
        while j < len(basis_map):
            basis_val = numeric_eval(basis_map[j], {var_name: value})
            if basis_val is None:
                ok = False
                break
            row.append(basis_val)
            j += 1
        source_val = numeric_eval(source, {var_name: value})
        if source_val is None:
            ok = False
        if ok:
            rows.append(row)
            rhs.append(source_val)
        i += 1
    if len(rows) < count:
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


def verify_template_match_numeric(source, target, var_name):
    if math is None:
        return False
    checks = [-2.75, -1.5, -0.75, 0.4, 0.9, 1.6, 2.4, 3.1]
    seen = 0
    i = 0
    while i < len(checks):
        value = checks[i]
        left = numeric_eval(source, {var_name: value})
        right = numeric_eval(target, {var_name: value})
        if left is not None and right is not None:
            if abs(left - right) > 1e-6 * (1.0 + abs(left) + abs(right)):
                return False
            seen += 1
        i += 1
    return seen >= 3


def fit_linear_template(source, placeholders, const_part, basis_by_name, source_var):
    basis_names = []
    basis_terms = []
    i = 0
    while i < len(placeholders):
        name = placeholders[i]
        basis_names.append(name)
        basis_terms.append(basis_by_name.get(name, num(0)))
        i += 1
    reduced_source = sim(sub(source, const_part))
    sample = choose_template_sample_values(reduced_source, basis_terms, source_var, len(basis_names))
    if sample is not None:
        mat, rhs = sample
        solved = solve_linear_system([list(row) for row in mat], list(rhs))
    else:
        sample = choose_template_sample_values_numeric(reduced_source, basis_terms, source_var, len(basis_names))
        if sample is None:
            return
        mat, rhs = sample
        float_solution = solve_linear_system_float([list(row) for row in mat], list(rhs))
        if float_solution is None:
            return
        solved = []
        i = 0
        while i < len(float_solution):
            solved.append(rational_from_float(float_solution[i]))
            i += 1
    if solved is None:
        return
    subst_map = {}
    i = 0
    while i < len(basis_names):
        subst_map[basis_names[i]] = solved[i]
        i += 1
    return subst_map


def solve_target_coefficients(source, target_template):
    source_var = choose_primary_var(source)
    if source_var is None:
        return
    target_names = sorted_symbol_names(target_template)
    placeholders = []
    i = 0
    while i < len(target_names):
        if target_names[i] != source_var:
            placeholders.append(target_names[i])
        i += 1
    if len(placeholders) == 0:
        return
    linear = extract_placeholder_linear_data(target_template, placeholders)
    if linear is None:
        return
    const_part, basis_by_name = linear
    subst_map = fit_linear_template(source, placeholders, const_part, basis_by_name, source_var)
    if subst_map is None:
        return
    instantiated = target_template
    for name, value in subst_map.items():
        instantiated = substitute_keep_form(instantiated, sym(name), value)
    instantiated = sim(instantiated)
    if not verify_template_match(source, instantiated, source_var) and not verify_template_match_numeric(source, instantiated, source_var):
        return
    return subst_map, instantiated


def solve_fraction_target_coefficients(source, target_template):
    if target_template[0] != 'div':
        return
    source_var = choose_primary_var(source)
    if source_var is None:
        return
    target_names = sorted_symbol_names(target_template)
    placeholders = []
    i = 0
    while i < len(target_names):
        if target_names[i] != source_var:
            placeholders.append(target_names[i])
        i += 1
    if len(placeholders) == 0:
        return
    denom_names = set()
    collect_symbol_names(target_template[2], denom_names)
    i = 0
    while i < len(placeholders):
        if placeholders[i] in denom_names:
            return
        i += 1
    linear = extract_placeholder_linear_data(target_template[1], placeholders)
    if linear is None:
        return
    const_part, basis_by_name = linear
    scaled_source = canonical_compare_form(mul([source, target_template[2]]))
    subst_map = fit_linear_template(scaled_source, placeholders, const_part, basis_by_name, source_var)
    if subst_map is None:
        return
    numerator = target_template[1]
    for name, value in subst_map.items():
        numerator = substitute_keep_form(numerator, sym(name), value)
    instantiated = sim(div(numerator, target_template[2]))
    if not verify_template_match(source, instantiated, source_var) and not verify_template_match_numeric(source, instantiated, source_var):
        return
    return subst_map, instantiated


def rearrange_to_target(source, target_template):
    steps = []
    current = sim(source)
    target = sim(target_template)
    source_canon = canonical_compare_form(current)
    if not same(current, source_canon):
        steps.append((1, 'Simplify source: ' + show(source_canon), source_canon))
        current = source_canon
    fitted = solve_fraction_target_coefficients(current, target)
    if fitted is None:
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
            steps.append((len(steps) + 1, 'Match ' + ', '.join(parts), target))
            return steps, target
    if same(current, target):
        steps.append((len(steps) + 1, 'In target form.', target))
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
    target_canon = canonical_compare_form(target)
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


def power_term_ratio(node, term, var_name):
    A = sim(node)
    B = sim(term)
    if same(A, B) or equivalent(A, B):
        return B
    n = 2
    while n <= 8:
        candidate = ('pow', B, num(n))
        if same(A, sim(candidate)) or equivalent(A, candidate):
            return candidate
        n += 1
    if A[0] == 'pow' and same(A[1], B) and not depends_on(A[2], var_name):
        return ('pow', B, A[2])
    if A[0] != 'pow' or B[0] != 'pow':
        return None
    if not same(canonical_compare_form(A[1]), canonical_compare_form(B[1])) and not equivalent(A[1], B[1]):
        return None
    if not is_num(A[2]) or not is_num(B[2]) or is_zero(B[2]):
        return None
    ratio = divq(A[2], B[2])
    if not is_int_num(ratio) or ratio[1] <= 0:
        return None
    if ratio == num(1):
        return B
    return ('pow', B, ratio)


def rewrite_power_terms_recursive(node, info):
    var_name = info['var']
    if var_name is None or not depends_on(node, var_name):
        return node
    direct = rewrite_term_equivalent(node, info)
    if direct is not None:
        return direct
    if rewrite_allowed_only(node, info):
        return node
    i = 0
    while i < len(info['terms']):
        direct = power_term_ratio(node, info['terms'][i], var_name)
        if direct is not None:
            return direct
        i += 1
    if node[0] == 'add' or node[0] == 'mul':
        items = []
        changed = False
        i = 0
        while i < len(node[1]):
            child = node[1][i]
            rewritten = rewrite_power_terms_recursive(child, info) if depends_on(child, var_name) else child
            if rewritten is None:
                return None
            if not same(rewritten, child):
                changed = True
            items.append(rewritten)
            i += 1
        if not changed:
            return None
        return node[0], tuple(items)
    if node[0] == 'div':
        left = rewrite_power_terms_recursive(node[1], info) if depends_on(node[1], var_name) else node[1]
        right = rewrite_power_terms_recursive(node[2], info) if depends_on(node[2], var_name) else node[2]
        if left is None or right is None:
            return None
        if same(left, node[1]) and same(right, node[2]):
            return None
        return 'div', left, right
    if node[0] == 'pow':
        base = rewrite_power_terms_recursive(node[1], info) if depends_on(node[1], var_name) else node[1]
        exp = rewrite_power_terms_recursive(node[2], info) if depends_on(node[2], var_name) else node[2]
        if base is None or exp is None:
            return None
        if same(base, node[1]) and same(exp, node[2]):
            return None
        return 'pow', base, exp
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

    add_candidate(canonical_compare_form(expr), 'Simplify and collect like terms')
    add_candidate(expand_pow_sqrt(expr), 'Simplify powers and roots')
    add_candidate(expand_mul_distribute(expr), 'Expand brackets')
    add_candidate(combine_fractions(expr), 'Combine the fractions')
    add_candidate(simplify_trig_identity(expr), 'Use a standard identity')
    add_candidate(rewrite_power_terms_recursive(expr, info), 'Rewrite powers in the requested term')

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
    direct_power = rewrite_power_terms_recursive(start, info)
    if direct_power is not None and not same(direct_power, start) and rewrite_allowed_only(direct_power, info) and rewrite_equivalent(direct_power, expr):
        return direct_power, [('Rewrite powers in the requested term', direct_power)], info
    if rewrite_allowed_only(start, info) and rewrite_equivalent(start, expr):
        if equivalent(start, num(1)):
            return num(1), [('Use a standard identity', num(1))], info
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
        lines.append('Answer: ' + show(final_expr))
    return lines




def simplify_trig_identity(node):
    node = sim(node)
    if node[0] == 'div':
        top = node[1]
        bot = node[2]
        if is_one(top) and bot[0] == 'fn':
            if bot[1] == 'sin':
                return fn('cosec', bot[2])
            if bot[1] == 'cos':
                return fn('sec', bot[2])
            if bot[1] == 'tan':
                return fn('cot', bot[2])
        if top[0] == 'fn' and bot[0] == 'fn' and same(top[2], bot[2]):
            if top[1] == 'cos' and bot[1] == 'sin':
                return fn('cot', top[2])
            if top[1] == 'sin' and bot[1] == 'cos':
                return fn('tan', top[2])
        return 'div', simplify_trig_identity(top), simplify_trig_identity(bot)
    if node[0] == 'add':
        items = flat(node, 'add')
        if len(items) == 2:
            def half_angle_arg(expr):
                half = half_angle_expr_for_transform(expr)
                if half is not None:
                    return half
                return sim(div(expr, num(2)))

            one_term = None
            trig_term = None
            i = 0
            while i < len(items):
                if same(items[i], num(1)):
                    one_term = items[i]
                else:
                    trig_term = items[i]
                i += 1
            if one_term is not None and trig_term is not None:
                coeff, rest = split_coeff(trig_term)
                if is_minus_one(coeff) and rest[0] == 'fn' and rest[1] == 'cos':
                    return sim(mul([num(2), power(fn('sin', half_angle_arg(rest[2])), num(2))]))
                if is_one(coeff) and rest[0] == 'fn' and rest[1] == 'cos':
                    return sim(mul([num(2), power(fn('cos', half_angle_arg(rest[2])), num(2))]))
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

        def append_coeff_term(out, coeff, rest):
            if is_zero(coeff):
                return
            if is_one(coeff):
                out.append(rest)
            elif is_minus_one(coeff):
                out.append(neg(rest))
            else:
                out.append(mul([coeff, rest]))

        def split_trig_square_product(rest):
            factors = list(flat(rest, 'mul')) if rest[0] == 'mul' else [rest]
            i_factor = 0
            while i_factor < len(factors):
                for name in ('sin', 'cos', 'tan', 'sec', 'cot', 'cosec'):
                    arg = is_pow_of_fn(factors[i_factor], name, num(2))
                    if arg is not None:
                        common = []
                        j_factor = 0
                        while j_factor < len(factors):
                            if j_factor != i_factor:
                                common.append(factors[j_factor])
                            j_factor += 1
                        return name, arg, sim(make_mul(common))
                i_factor += 1
            return None

        def replace_product_identity(first_name, second_name, opposite_signs):
            groups = {}
            i_group = 0
            while i_group < len(order):
                key = order[i_group]
                rest, coeff = coeff_map[key]
                info = split_trig_square_product(rest)
                if info is not None:
                    name, arg, common = info
                    if name == first_name or name == second_name:
                        group_key = (sig(arg), sig(common))
                        if group_key not in groups:
                            groups[group_key] = {}
                        groups[group_key][name] = (key, rest, coeff, common)
                i_group += 1

            for group_key in groups:
                pair = groups[group_key]
                if first_name not in pair or second_name not in pair:
                    continue
                first_key, first_rest, first_coeff, common = pair[first_name]
                second_key, second_rest, second_coeff, _common2 = pair[second_name]
                if opposite_signs:
                    if not is_zero(sim(add([first_coeff, second_coeff]))):
                        continue
                elif not same(first_coeff, second_coeff):
                    continue

                out = []
                j = 0
                while j < len(order):
                    key = order[j]
                    if key == first_key or key == second_key:
                        j += 1
                        continue
                    rest, coeff = coeff_map[key]
                    append_coeff_term(out, coeff, rest)
                    j += 1
                append_coeff_term(out, first_coeff, common)
                return sim(make_add(out))
            return None

        replaced = replace_product_identity('sin', 'cos', False)
        if replaced is not None:
            return replaced
        replaced = replace_product_identity('sec', 'tan', True)
        if replaced is not None:
            return replaced
        replaced = replace_product_identity('cosec', 'cot', True)
        if replaced is not None:
            return replaced

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
    simple1 = canonical_compare_form(expr1)
    simple2 = canonical_compare_form(expr2)
    steps = []
    step_num = 1
    steps.append((step_num, 'Expression 1: ' + show(expr1), expr1))
    step_num += 1
    steps.append((step_num, 'Expression 2: ' + show(expr2), expr2))
    step_num += 1
    steps.append((step_num, 'Simplify: ' + show(simple1), simple1))
    step_num += 1
    steps.append((step_num, 'Simplify: ' + show(simple2), simple2))
    step_num += 1
    diff = canonical_compare_form(sim(add([simple1, neg(simple2)])))
    if equivalent(expr1, expr2):
        steps.append((step_num, 'Difference expr1 - expr2: ' + show(diff), diff))
        step_num += 1
        if equivalent(simple1, simple2):
            steps.append((step_num, 'Hence expressions are equivalent.', simple1))
        else:
            steps.append((step_num, 'Use standard identities before comparing.', None))
            step_num += 1
            steps.append((step_num, 'Hence expressions are equivalent.', simple1))
        return True, steps
    if equivalent(simple1, simple2):
        steps.append((step_num, 'Difference expr1 - expr2: ' + show(diff), diff))
        step_num += 1
        steps.append((step_num, 'Hence expressions are equivalent.', simple1))
        return True, steps
    steps.append((step_num, 'Difference expr1 - expr2: ' + show(diff), diff))
    return False, steps


def real_numeric_value(node):
    value = numeric_eval(node, {})
    if value is None:
        return None
    if isinstance(value, complex):
        if abs(value.imag) > 1e-8:
            return None
        value = value.real
    try:
        if not math.isfinite(value):
            return None
    except Exception:
        return None
    return value


def lift_power_substitution_roots(sub_roots, power_step):
    lifted = []
    i = 0
    while i < len(sub_roots):
        root = sim(sub_roots[i])
        root_value = real_numeric_value(root)
        if power_step % 2 == 1:
            lifted.append(sim(power(root, num(1, power_step))))
        elif root_value is not None and root_value >= -1e-9:
            if abs(root_value) < 1e-9:
                lifted.append(num(0))
            else:
                lifted_root = sim(power(root, num(1, power_step)))
                lifted.append(lifted_root)
                lifted.append(sim(neg(lifted_root)))
        i += 1
    return normalize_solution_roots(lifted)


def hidden_power_substitution_roots(coeffs, degree):
    powers = []
    i = 1
    while i <= degree:
        if not is_zero(coeffs[i]):
            powers.append(i)
        i += 1
    if len(powers) == 0:
        return None, None
    power_step = powers[0]
    i = 1
    while i < len(powers):
        power_step = gcd(power_step, powers[i])
        i += 1
    if power_step <= 1:
        return None, None

    sub_degree = degree // power_step
    if sub_degree >= degree:
        return None, None
    sub_coeffs = []
    i = 0
    while i <= sub_degree:
        sub_coeffs.append(num(0))
        i += 1
    i = 0
    while i <= degree:
        if not is_zero(coeffs[i]):
            if i % power_step != 0:
                return None, None
            sub_coeffs[i // power_step] = coeffs[i]
        i += 1

    label, sub_roots = solve_polynomial_expr(make_poly_from_coeffs(sub_coeffs, 'u'), 'u')
    if label == 'none':
        return 'none', []
    if not sub_roots:
        return None, None
    lifted = lift_power_substitution_roots(sub_roots, power_step)
    if not lifted:
        return 'none', []
    return 'hidden substitution u=x^' + str(power_step), lifted


def depressed_cubic_cardano_roots(coeffs):
    if len(coeffs) < 4 or is_zero(coeffs[3]) or not is_zero(coeffs[2]):
        return None
    a = coeffs[3]
    p = sim(div(coeffs[1], a))
    q = sim(div(coeffs[0], a))
    half_q = sim(div(q, num(2)))
    third_p = sim(div(p, num(3)))
    discriminant = sim(add([power(half_q, num(2)), power(third_p, num(3))]))
    disc_value = real_numeric_value(discriminant)
    if disc_value is None or disc_value < -1e-9:
        return None
    disc_root = sqrt_num(discriminant)
    if disc_root is None:
        disc_root = fn('sqrt', discriminant)
    first = sim(add([neg(half_q), disc_root]))
    second = sim(add([neg(half_q), neg(disc_root)]))
    root = sim(add([power(first, num(1, 3)), power(second, num(1, 3))]))
    return [root]


def depressed_cubic_cardano_numeric_roots(expr, var_name):
    if math is None:
        return []
    coeffs, degree = polynomial_coeff_list(expr, var_name, 20)
    if coeffs is None or degree != 3 or not is_zero(coeffs[2]):
        return []
    a = real_numeric_value(coeffs[3])
    c = real_numeric_value(coeffs[1])
    d = real_numeric_value(coeffs[0])
    if a is None or c is None or d is None or abs(a) < 1e-12:
        return []
    p = c / a
    q = d / a
    discriminant = (q / 2) ** 2 + (p / 3) ** 3
    if discriminant < -1e-12:
        return []
    if discriminant < 0:
        discriminant = 0

    def cbrt(value):
        if value < 0:
            return -((-value) ** (1 / 3))
        return value ** (1 / 3)

    root = cbrt(-q / 2 + math.sqrt(discriminant)) + cbrt(-q / 2 - math.sqrt(discriminant))
    return [root]


def format_decimal_solution_line(var_name, roots):
    bits = []
    i = 0
    while i < len(roots):
        bits.append(("{:.12g}".format(roots[i])).rstrip("0").rstrip("."))
        i += 1
    return var_name + ' = ' + (bits[0] if len(bits) == 1 else '[' + ', '.join(bits) + ']')


def solve_polynomial_expr(node, var_name):
    coeffs, degree = polynomial_coeff_list(node, var_name, 20)
    if coeffs is None:
        return None, None
    if degree == 0:
        return ('identity' if is_zero(coeffs[0]) else 'none'), []
    if degree == 1:
        return 'lin', [sim(div(neg(coeffs[0]), coeffs[1]))]
    if degree > 2:
        monomial_only = True
        i = 1
        while i < degree:
            if not is_zero(coeffs[i]):
                monomial_only = False
                break
            i += 1
        if monomial_only and not is_zero(coeffs[degree]):
            leading = coeffs[degree]
            target = sim(div(neg(coeffs[0]), leading))
            if degree % 2 == 1:
                return 'poly', [sim(power(target, num(1, degree)))]
            target_num = eval_with_values(target, {})
            if target_num is not None and is_num(target_num) and target_num[1] >= 0:
                root = sim(power(target, num(1, degree)))
                return 'poly', normalize_solution_roots([root, sim(neg(root))])
        hidden_label, hidden_roots = hidden_power_substitution_roots(coeffs, degree)
        if hidden_roots is not None:
            return hidden_label, hidden_roots
        roots = rational_roots_for_polynomial(coeffs, degree)
        if roots is not None and len(roots) != 0:
            return 'poly', roots
        if degree == 3:
            cardano_roots = depressed_cubic_cardano_roots(coeffs)
            if cardano_roots is not None:
                return 'cubic by Cardano formula', cardano_roots
        return None, None
    roots = normalize_solution_roots(rational_roots_for_quadratic(coeffs))
    if len(roots) == 0:
        return 'none', []
    return 'quad', roots



def _final_rewrite_substitution(term, var_name, u_name):
    linear = match_linear_in_var(term, var_name)
    if linear is not None:
        a, b = linear
        if not is_zero(a):
            return sim(div(add([sym(u_name), neg(b)]), a))
    reciprocal = match_reciprocal_in_var(term, var_name)
    if reciprocal is not None:
        a, b, c = reciprocal
        if not is_zero(b):
            return sim(div(add([div(a, sym(u_name)), neg(c)]), b))
    shifted = match_shifted_reciprocal(term, var_name)
    if shifted is not None:
        a, b, c, shift = shifted
        shifted_u = sim(add([sym(u_name), neg(shift)]))
        if not is_zero(b):
            return sim(div(add([div(a, shifted_u), neg(c)]), b))
    return None


def factor_quadratic_rational(node, var_name=None):
    node = sim(node)
    if var_name is None:
        var_name = choose_primary_var(node)
    if var_name is None:
        return None
    coeffs, degree = polynomial_coeff_list(node, var_name, 2)
    if coeffs is None or degree != 2:
        return None
    roots = normalize_solution_roots(rational_roots_for_quadratic(coeffs))
    if len(roots) == 0:
        disc = sim(sub(power(coeffs[1], num(2)), mul([num(4), coeffs[2], coeffs[0]])))
        if is_zero(disc):
            roots = [sim(div(neg(coeffs[1]), mul([num(2), coeffs[2]]))), sim(div(neg(coeffs[1]), mul([num(2), coeffs[2]])))]
    elif len(roots) == 1:
        roots = [roots[0], roots[0]]
    if len(roots) != 2:
        return None
    factored = factor_from_roots(sym(var_name), roots)
    if not is_one(coeffs[2]):
        factored = sim(mul([coeffs[2], factored]))
    if equivalent(factored, node):
        return factored, 'Factor quad'
    return None


def expression_zero_at_root(expr, var_name, root):
    check = eval_with_values(expr, {var_name: root})
    if check is not None:
        return is_zero(check)
    applied = apply_values(expr, {var_name: root})
    approx = numeric_eval(applied, {})
    if approx is None:
        return False
    if isinstance(approx, complex):
        return abs(approx.real) < 1e-7 and abs(approx.imag) < 1e-7
    return abs(approx) < 1e-7



























def valid_roots_for_expr(expr, var_name, roots):
    valid = []
    i = 0
    while i < len(roots):
        if expression_zero_at_root(expr, var_name, roots[i]):
            valid.append(roots[i])
        i += 1
    return normalize_solution_roots(valid)













def match_shifted_reciprocal(node, var_name):
    node = sim(node)
    dependent, shift = split_outer_shift(node, var_name)
    if dependent is None:
        return None
    reciprocal = match_reciprocal_in_var(dependent, var_name)
    if reciprocal is None:
        return None
    return reciprocal[0], reciprocal[1], reciprocal[2], shift




def solve_equation_text(text):
    cartesian = cartesian_equation_lines(text)
    if cartesian is not None:
        return cartesian
    expr = parse_expr_or_equation(text)
    var_name, roots, label = solve_equation(expr)
    shown_expr = expr
    if roots is not None and var_name is not None:
        factored_info = factor_expression(expr, var_name)
        if factored_info is not None and not same(factored_info[0], expr):
            shown_expr = factored_info[0]
    lines = ['Method: Solve equation', 'Expr = ' + show(shown_expr)]
    if label == 'Identity':
        lines.append('All x satisfy the equation')
        lines.append('Answer: all real x')
        return lines
    if label == 'No solution':
        lines.append('Solve: no real solution')
        lines.append('Answer: no solution')
        return lines
    if roots is None:
        lines.append(label)
        lines.append('Answer: no closed-form solution found')
        return compact_duplicate_answer_lines(lines)
    lines.append(label)
    solution = format_solution_line(var_name, roots)
    if 'Cardano' in label:
        decimal_roots = depressed_cubic_cardano_numeric_roots(expr, var_name)
        if decimal_roots:
            lines.append(format_decimal_solution_line(var_name, decimal_roots))
    lines.append('Answer: ' + solution)
    return compact_duplicate_answer_lines(lines)










def solve_mode_lines(text):
    return solve_equation_text(text)


def rewrite_mode_lines(text, terms):
    return solve_rewrite_text(text, terms)


def factor_mode_lines(text):
    return factor_text(text)


def expand_mode_lines(text):
    return poly_expand_text(text)


def format_lines(lines):
    return lines


def print_mode_lines(lines):
    i = 0
    while i < len(lines):
        print(str(i + 1) + '. ' + lines[i])
        i += 1


def rewrite_in_u_text(text, term_text):
    return rewrite_in_term_text(text, term_text)


def solve_poly_text(text):
    return solve_equation_text(text)


def factor_poly_text(text):
    return factor_text(text)


def rewrite_poly_text(text, term_texts):
    return solve_rewrite_text(text, term_texts)


def solve_branch_lines(text):
    return solve_equation_text(text)


def factor_branch_lines(text):
    return factor_text(text)


def rewrite_branch_lines(text, terms):
    return solve_rewrite_text(text, terms)


def poly_branch_lines(text):
    return poly_expand_text(text)










def cli_print_lines(lines):
    print_mode_lines(lines)


def algebra_mode_6_lines(text):
    return solve_equation_text(text)


def algebra_mode_9_lines(text, terms):
    return solve_rewrite_text(text, terms)


def algebra_mode_3_lines(text):
    return expand_mode_text(text)


def make_product_or_one(items):
    if len(items) == 0:
        return num(1)
    return sim(make_mul(items))


def expand_mode_text(text, max_terms=None):
    expr = parse(text.strip())
    expr = sim(expr)
    lines = ['Method: Expand expression', 'Input = ' + show(expr)]

    if expr[0] == 'pow' and is_int_num(expr[2]) and expr[2][2] == 1 and expr[2][1] >= 0:
        base = sim(expr[1])
        n = expr[2][1]
        base_terms = []
        if base[0] == 'add':
            base_terms = list(flat(base, 'add'))
        if len(base_terms) == 2:
            first = base_terms[0]
            second = base_terms[1]
            lines.append('Binomial expansion')
            lines.append('Expand ' + show(expr))
            expanded_terms = []
            limit = n + 1
            if max_terms is not None and max_terms > 0 and max_terms < limit:
                limit = max_terms
            k = 0
            while k < limit:
                pieces = []
                coeff = binomial_coefficient(n, k)
                if coeff != 1:
                    pieces.append(num(coeff))
                if n - k > 0:
                    pieces.append(first if n - k == 1 else power(first, num(n - k)))
                if k > 0:
                    pieces.append(second if k == 1 else power(second, num(k)))
                term = make_product_or_one(pieces)
                expanded_terms.append(term)
                lines.append('Term ' + str(k + 1) + ': ' + show(term))
                k += 1
            expanded = sim(add(expanded_terms))
            if limit < n + 1:
                lines.append('Out = ' + show(expanded) + '+...')
            else:
                lines.append('Out = ' + show(expanded))
            return ensure_reasoning_marker(lines)

    if can_use_generalized_binomial(expr):
        expanded_lines = generalized_binomial_expand_text(expr, max_terms=max_terms)
        if expanded_lines is not None:
            return ensure_reasoning_marker(lines + expanded_lines)

    expanded = maybe_expand_for_compare(expr)
    if not same(expanded, expr):
        return lines + ['Expand brackets', 'Out = ' + show(expanded)]
    return lines + ['Out = ' + show(expr)]








def poly_expand_text(text):
    expr = parse(text.strip())
    out = canonical_compare_form(expr)
    return ['Input = ' + show(expr), 'Out = ' + show(out)]


def cli_lines_for_mode(mode, text, terms=None):
    if mode == '6':
        return solve_equation_text(text)
    if mode == '9':
        return solve_rewrite_text(text, terms or [])
    if mode == '3':
        return expand_mode_text(text)
    if mode == '4':
        return factor_text(text)
    return []


def print_cli_lines(lines):
    i = 0
    while i < len(lines):
        print(str(i + 1) + '. ' + lines[i])
        i += 1


def final_factor_text(text):
    return factor_text(text)


def final_solve_text(text):
    return solve_equation_text(text)


def final_rewrite_text(text, terms):
    return solve_rewrite_text(text, terms)


def final_poly_text(text):
    return poly_expand_text(text)


def mode_text_lines(mode, text, terms=None):
    return cli_lines_for_mode(mode, text, terms)


def show_mode_lines(lines):
    print_cli_lines(lines)








def final_poly_expand_text(text):
    expr = parse(text.strip())
    out = canonical_compare_form(expr)
    return ['Input = ' + show(expr), 'Out = ' + show(out)]


def solve_mode_text(text):
    return solve_equation_text(text)


def rewrite_mode_text(text, terms):
    return solve_rewrite_text(text, terms)




def factor_mode_text(text):
    return factor_text(text)


def complete_square_text(text):
    expr = parse(text.strip())
    expr = sim(expr)
    var_name = choose_primary_var(expr)
    if var_name is None:
        return ['Input = ' + show(expr), 'Need a variable to complete the square.']

    coeffs, degree = polynomial_coeff_list(expr, var_name, 2)
    if coeffs is None or degree != 2 or is_zero(coeffs[2]):
        return ['Input = ' + show(expr), 'Need a quadratic in ' + var_name + '.']

    a = coeffs[2]
    b = coeffs[1]
    c = coeffs[0]
    shift = sim(div(b, mul([num(2), a])))
    square = sim(add([sym(var_name), shift]))
    constant = sim(sub(c, div(power(b, num(2)), mul([num(4), a]))))

    terms = [mul([a, power(square, num(2))])]
    if not is_zero(constant):
        terms.append(constant)
    result = sim(add(terms))

    lines = ['Method: Complete square', 'Input = ' + show(expr)]
    if not is_one(a):
        inner_terms = [power(sym(var_name), num(2))]
        scaled_linear = sim(div(b, a))
        if not is_zero(scaled_linear):
            inner_terms.append(mul([scaled_linear, sym(var_name)]))
        inner = sim(add(inner_terms))
        grouped = sim(add([mul([a, inner]), c]))
        lines.append('Factor out ' + show(a) + ' from the quadratic terms: ' + show(grouped))
    lines.append('Half the coefficient of ' + var_name + ': ' + show(shift))
    lines.append('Complete square: ' + show(result))
    lines.append('Ans = ' + show(result))
    return lines

def clear_denominator_once(expr):
    expr = sim(expr)
    if expr[0] != 'add':
        return expr
    terms = list(flat(expr, 'add'))
    denoms = []
    i = 0
    while i < len(terms):
        if terms[i][0] == 'div':
            denoms.append(terms[i][2])
        i += 1
    if len(denoms) == 0:
        return expr
    common = make_mul(denoms)
    out = []
    i = 0
    while i < len(terms):
        if terms[i][0] == 'div':
            others = []
            removed = False
            j = 0
            while j < len(denoms):
                if not removed and equivalent(denoms[j], terms[i][2]):
                    removed = True
                else:
                    others.append(denoms[j])
                j += 1
            out.append(sim(mul([terms[i][1], make_mul(others)])))
        else:
            out.append(sim(mul([terms[i], common])))
        i += 1
    return sim(make_add(out))


def solve_rational_equation(expr, var_name):
    reduced = canonical_compare_form(expr)
    cleared = clear_denominator_once(reduced)
    if same(cleared, reduced):
        return None
    label, roots = solve_polynomial_expr(cleared, var_name)
    if roots is None:
        return None
    valid = []
    i = 0
    while i < len(roots):
        if expression_zero_at_root(expr, var_name, roots[i]):
            valid.append(roots[i])
        i += 1
    valid = normalize_solution_roots(valid)
    if len(valid) == 0:
        return 'No solution', []
    return 'Solve by clearing denominators', valid


def solve_shifted_power_equation(expr, var_name):
    expr = sim(expr)
    terms = list(flat(expr, 'add')) if expr[0] == 'add' else [expr]
    power_term = None
    constant = num(0)
    i = 0
    while i < len(terms):
        coeff, rest = split_coeff(terms[i])
        match = match_power_linear_in_var(rest, var_name)
        if match is not None:
            if power_term is not None:
                return None
            power_term = (coeff,) + match
        elif depends_on(terms[i], var_name):
            return None
        else:
            constant = sim(add([constant, terms[i]]))
        i += 1
    if power_term is None:
        return None
    coeff, a, b, n_value, exp = power_term
    if n_value is None or is_zero(a) or is_zero(coeff):
        return None
    target = sim(div(neg(constant), coeff))
    target_value = real_numeric_value(target)
    if n_value % 2 == 0 and (target_value is None or target_value < -1e-9):
        return 'No solution', []
    root_mag = exact_nth_root_num(target, n_value)
    if root_mag is None:
        root_mag = sim(power(target, num(1, n_value)))
    roots = []
    if n_value % 2 == 0:
        roots.append(sim(div(add([root_mag, neg(b)]), a)))
        roots.append(sim(div(add([neg(root_mag), neg(b)]), a)))
    else:
        roots.append(sim(div(add([root_mag, neg(b)]), a)))
    return 'Solve shifted power directly', normalize_solution_roots(roots)


def contains_function_safe(node, fn_name):
    if node is None:
        return False
    if isinstance(node, str):
        return False
    if not isinstance(node, (list, tuple)):
        return False
    try:
        if node[0] == 'fn' and len(node) > 1 and node[1] == fn_name:
            return True
        for item in node:
            if contains_function_safe(item, fn_name):
                return True
    except:
        pass
    return False


def find_radical_terms_safe(expr, var_name):
    radicals = []
    def find(node):
        if node is None or not isinstance(node, (list, tuple)):
            return
        try:
            if node[0] == 'fn' and len(node) > 1 and node[1] == 'sqrt':
                radicals.append(node)
        except:
            pass
        for item in node:
            find(item)
    find(expr)
    return radicals


def is_var_sqrt_power(node, var_name):
    return (
        isinstance(node, (list, tuple))
        and len(node) >= 3
        and node[0] == 'pow'
        and same(node[1], sym(var_name))
        and is_num(node[2])
        and node[2][1] == 1
        and node[2][2] == 2
    )


def contains_var_sqrt_power(node, var_name):
    if node is None or not isinstance(node, (list, tuple)):
        return False
    if is_var_sqrt_power(node, var_name):
        return True
    i = 1
    while i < len(node):
        if contains_var_sqrt_power(node[i], var_name):
            return True
        i += 1
    return False


def check_solution(expr, var_name, root):
    try:
        val = eval_with_values(expr, {var_name: root})
        return val is not None and is_zero(val)
    except:
        return False


def classify_sqrt_substitution_term(term, var_name):
    term = sim(term)
    if not depends_on(term, var_name):
        return 'const', term
    coeff, rest = split_coeff(term)
    rest = sim(rest)
    if same(rest, sym(var_name)):
        return 'x', coeff
    if is_var_sqrt_power(rest, var_name):
        return 'sqrt', coeff
    if rest[0] == 'fn' and rest[1] == 'sqrt' and same(rest[2], sym(var_name)):
        return 'sqrt', coeff
    return None, None


def solve_square_root_quadratic_equation(expr, var_name):
    terms = flat(expr, 'add') if expr[0] == 'add' else [expr]
    a = num(0)
    b = num(0)
    c = num(0)
    i = 0
    while i < len(terms):
        kind, coeff = classify_sqrt_substitution_term(terms[i], var_name)
        if kind is None:
            return None
        if kind == 'x':
            a = sim(add([a, coeff]))
        elif kind == 'sqrt':
            b = sim(add([b, coeff]))
        else:
            c = sim(add([c, coeff]))
        i += 1
    if is_zero(a) and is_zero(b):
        return None
    u_roots = []
    if is_zero(a):
        u_roots.append(sim(div(neg(c), b)))
    else:
        disc = sim(sub(power(b, num(2)), mul([num(4), a, c])))
        disc_value = real_numeric_value(disc)
        if disc_value is not None and disc_value < -1e-9:
            return var_name, [], 'No solution'
        sqrt_disc = fn('sqrt', disc)
        denom = sim(mul([num(2), a]))
        u_roots.append(sim(div(add([neg(b), sqrt_disc]), denom)))
        u_roots.append(sim(div(add([neg(b), neg(sqrt_disc)]), denom)))
    roots = []
    i = 0
    while i < len(u_roots):
        u_root = sim(u_roots[i])
        u_value = real_numeric_value(u_root)
        if u_value is not None and u_value >= -1e-9:
            x_root = sim(power(u_root, num(2)))
            if expression_zero_at_root(expr, var_name, x_root):
                roots.append(x_root)
        i += 1
    roots = normalize_solution_roots(roots)
    if roots:
        return var_name, roots, 'Solve quadratic substitution'
    return var_name, [], 'No solution'


def solve_radical_equation(expr, var_name):
    try:
        if not contains_function_safe(expr, 'sqrt') and not contains_var_sqrt_power(expr, var_name):
            return None
        exact_result = solve_square_root_quadratic_equation(expr, var_name)
        if exact_result is not None:
            return exact_result
        a_val, b_val, c_val = 0.0, 0.0, 0.0
        def collect_coeffs(node, sign):
            nonlocal a_val, b_val, c_val
            if node is None:
                return
            if isinstance(node, (list, tuple)) and len(node) == 0:
                return
            if isinstance(node, (list, tuple)) and not isinstance(node[0], str):
                for child in node:
                    collect_coeffs(child, sign)
                return
            if not isinstance(node, (list, tuple)):
                return
            op = node[0]
            if op == 'num' and len(node) >= 3:
                c_val += sign * node[1] / node[2]
            elif op == 'sym' and len(node) > 1 and node[1] == var_name:
                a_val += sign * 1.0
            elif op == 'neg' and len(node) > 1:
                collect_coeffs(node[1], -sign)
            elif op == 'mul':
                coeff = 1.0
                has_x = False
                has_sqrt = False
                def extract_factors(items):
                    result = []
                    if isinstance(items, (list, tuple)):
                        for item in items:
                            if isinstance(item, (list, tuple)):
                                if isinstance(item[0], str):
                                    result.append(item)
                                elif isinstance(item[0], (list, tuple)):
                                    for sub in item:
                                        if isinstance(sub, (list, tuple)):
                                            result.append(sub)
                    return result
                factors = extract_factors(node[1:])
                for f in factors:
                    if len(f) > 0 and isinstance(f[0], str):
                        if f[0] == 'num' and len(f) >= 3:
                            coeff *= f[1] / f[2]
                        elif f[0] == 'sym' and len(f) > 1 and f[1] == var_name:
                            has_x = True
                        elif (f[0] == 'fn' and f[1] == 'sqrt' and len(f) > 2) or is_var_sqrt_power(f, var_name):
                            has_sqrt = True
                if has_x and not has_sqrt:
                    a_val += sign * coeff
                elif has_sqrt and not has_x:
                    b_val += sign * coeff
            elif op == 'fn' and len(node) > 2 and node[1] == 'sqrt':
                b_val += sign * 1.0
            elif is_var_sqrt_power(node, var_name):
                b_val += sign * 1.0
            elif op == 'add':
                for child in node[1:]:
                    collect_coeffs(child, sign)
        collect_coeffs(expr, 1.0)
        if a_val == 0.0:
            return None
        disc = b_val * b_val - 4 * a_val * c_val
        if disc < 0:
            return var_name, [], 'No solution'
        import math
        sqrt_disc = math.sqrt(disc)
        valid_x = []
        u1 = (-b_val + sqrt_disc) / (2 * a_val)
        if u1 >= 0:
            valid_x.append(num(int(u1 * u1 * 10000 + 0.5), 10000))
        u2 = (-b_val - sqrt_disc) / (2 * a_val)
        if u2 >= 0:
            valid_x.append(num(int(u2 * u2 * 10000 + 0.5), 10000))
        if valid_x:
            return var_name, valid_x, 'Solve quadratic substitution'
    except:
        pass
    return None


def contains_function(node, fn_name):
    if node is None:
        return False
    if isinstance(node, str):
        return False
    if not isinstance(node, (list, tuple)):
        return False
    if node[0] == 'fn' and node[1] == fn_name:
        return True
    i = 1
    while i < len(node):
        if contains_function(node[i], fn_name):
            return True
        i += 1
    return False


def find_radical_terms(expr, var_name):
    radicals = []
    def find(node):
        if node is None:
            return
        if not isinstance(node, (list, tuple)):
            return
        if node[0] == 'fn' and node[1] == 'sqrt':
            if depends_on(node[2], var_name):
                radicals.append(node)
        i = 1
        while i < len(node):
            find(node[i])
            i += 1
    find(expr)
    return radicals


def isolate_one_radical(expr, var_name):
    def extract_add_terms(node):
        if node is None:
            return []
        if not isinstance(node, (list, tuple)):
            return [node]
        if node[0] == 'add':
            result = []
            items = node[1] if isinstance(node[1], (list, tuple)) else (node[1:] if len(node) > 1 else [])
            for item in items:
                if isinstance(item, (list, tuple)) and item[0] == 'add':
                    result.extend(extract_add_terms(item))
                else:
                    result.append(item)
            return result
        return [node]
    
    working = expr
    if working[0] == 'sub':
        if working[2] == ('num', 0, 1) or working[2] == ['num', 0, 1]:
            working = working[1]
    
    if working[0] == 'add':
        terms = extract_add_terms(working)
        for i, term in enumerate(terms):
            if contains_function(term, 'sqrt'):
                radical_part = term
                other_parts = terms[:i] + terms[i+1:]
                other_sum = sim(make_add(other_parts)) if other_parts else ['num', 0, 1]
                return radical_part, sim(neg(other_sum))
    return None, None


def solve_equation(node):
    expr = sim(node)
    direct_var = choose_primary_var(expr)
    if direct_var is not None:
        shifted_power = solve_shifted_power_equation(expr, direct_var)
        if shifted_power is not None:
            if shifted_power[0] == 'No solution':
                return direct_var, [], 'No solution'
            return direct_var, shifted_power[1], shifted_power[0]
    expanded = maybe_expand_for_compare(expr)
    reduced = canonical_compare_form(expanded)
    working = reduced if not same(reduced, expanded) else expanded
    expanded_working = expand_for_solving(working)
    if not same(expanded_working, working):
        working = expanded_working
    var_name = choose_primary_var(working)
    if var_name is None:
        if is_zero(working):
            return None, [], 'Identity'
        return None, [], 'No solution'
    label, roots = solve_polynomial_expr(working, var_name)
    if label == 'identity':
        return None, [], 'Identity'
    if label == 'none':
        return None, [], 'No solution'
    if roots is not None:
        return var_name, normalize_solution_roots(roots), ('Solve ' + label)
    rational = solve_rational_equation(working, var_name)
    if rational is not None:
        if rational[0] == 'No solution':
            return var_name, [], 'No solution'
        return var_name, normalize_solution_roots(rational[1]), rational[0]
    if working[0] == 'div':
        top = maybe_expand_for_compare(sim(working[1]))
        bot = sim(working[2])
        factored = factor_expression(top, var_name)
        if factored is not None:
            top = sim(factored[0])
        label, roots = solve_polynomial_expr(top, var_name)
        if roots is not None:
            valid = []
            i = 0
            while i < len(roots):
                den_val = eval_with_values(bot, {var_name: roots[i]})
                if den_val is None or not is_zero(den_val):
                    valid.append(roots[i])
                i += 1
            valid = normalize_solution_roots(valid)
            if len(valid) != 0:
                return var_name, valid, 'Clear den'
    radical_result = solve_radical_equation(working, var_name)
    if radical_result is not None:
        return radical_result
    return None, None, 'Tried polynomial, rational, radical, and substitution methods; no closed-form real root was produced.'


def poly_mode_text(text):
    expr = parse(text.strip())
    factored = factor_expression(expr)
    if factored is not None:
        return ['Input = ' + show(expr), factored[1], '= ' + show(factored[0])]
    out = maybe_expand_for_compare(expr)
    return ['Input = ' + show(expr), 'Out = ' + show(out)]


def solve_rewrite_text(text, term_texts):
    if len(term_texts) == 0:
        raise ValueError('Use at least one target term.')
    if len(term_texts) == 1:
        return rewrite_in_term_text(text, term_texts[0])
    target = add_term_texts(term_texts)
    lines = solve_transform_text(text, target)
    if len(lines) > 0 and lines[-1].startswith('Answer: '):
        return lines
    return lines


def add_term_texts(term_texts):
    i = 0
    out = ''
    while i < len(term_texts):
        if i != 0:
            out += '+'
        out += term_texts[i].strip()
        i += 1
    return out


def factor_text(text):
    expr = parse(text.strip())
    factored = factor_expression(expr)
    if factored is None:
        quad = factor_quadratic_rational(expr)
        if quad is not None:
            return ['Input = ' + show(expr), quad[1], '= ' + show(quad[0])]
        out = maybe_expand_for_compare(expr)
        if not same(out, expr):
            return ['Input = ' + show(expr), 'Out = ' + show(out)]
        return ['Input = ' + show(expr), 'No factor']
    return ['Input = ' + show(expr), factored[1], '= ' + show(factored[0])]

def rewrite_polynomial_in_linear_term(node, term, var_name):
    node = sim(node)
    term = sim(term)
    linear = match_linear_in_var(term, var_name)
    if linear is None:
        return None
    a, b = linear
    if is_zero(a):
        return None
    coeffs, degree = polynomial_coeff_list(node, var_name, 2)
    if coeffs is None:
        expanded = expand_for_solving(node)
        if not same(expanded, node):
            node = expanded
            coeffs, degree = polynomial_coeff_list(node, var_name, 2)
    if coeffs is None:
        return None
    u_name = choose_unused_symbol(node, [term])
    u = sym(u_name)
    if degree == 0:
        candidate = coeffs[0]
    elif degree == 1:
        candidate = sim(add([
            mul([div(coeffs[1], a), u]),
            sub(coeffs[0], div(mul([coeffs[1], b]), a)),
        ]))
    else:
        quad_coeff = sim(div(coeffs[2], power(a, num(2))))
        lin_coeff = sim(div(sub(mul([coeffs[1], a]), mul([num(2), coeffs[2], b])), power(a, num(2))))
        const_coeff = sim(div(add([
            mul([coeffs[0], power(a, num(2))]),
            neg(mul([coeffs[1], a, b])),
            mul([coeffs[2], power(b, num(2))]),
        ]), power(a, num(2))))
        parts = [mul([quad_coeff, power(u, num(2))])]
        if not is_zero(lin_coeff):
            parts.append(mul([lin_coeff, u]))
        if not is_zero(const_coeff):
            parts.append(const_coeff)
        candidate = sim(add(parts))
    back_sub = substitute_keep_form(candidate, u, term)
    samples = [num(-3), num(-2), num(-1), num(0), num(1), num(2), num(3)]
    ok_count = 0
    i = 0
    while i < len(samples):
        left = eval_with_values(node, {var_name: samples[i]})
        right = eval_with_values(back_sub, {var_name: samples[i]})
        if left is not None and right is not None:
            if not same(left, right):
                return None
            ok_count += 1
        i += 1
    if ok_count >= 3:
        return u_name, candidate
    return None

def rewrite_in_term_text(text, term_text):
    expr = parse(text.strip())
    term = parse(term_text.strip())
    var_name = choose_primary_var(expr)
    if var_name is None:
        raise ValueError('Choose a variable.')
    info = build_rewrite_allowed_info(expr, [term])
    if rewrite_allowed_only(expr, info):
        return [
            'Start with ' + show(expr),
            'Write in terms of ' + show(term) + ' only.',
            'Already written in terms of ' + show(term) + '.',
            'Answer: ' + show(expr),
        ]
    variants = [expr]
    simplified = canonical_compare_form(expr)
    if not same(simplified, expr):
        variants.append(simplified)
    i = 0
    while i < len(variants):
        rewritten = rewrite_polynomial_in_linear_term(variants[i], term, var_name)
        if rewritten is not None:
            u_name, out = rewritten
            lines = ['Start with ' + show(expr), 'Write in terms of ' + show(term) + ' only.', u_name + ' = ' + show(term)]
            if i != 0:
                lines.append('Expand and collect like terms first.')
            lines.append('Answer: ' + show(out))
            return lines
        i += 1
    shifted = match_shifted_reciprocal(term, var_name)
    if shifted is not None:
        coeff, a, b, shift = shifted
        u_name = choose_unused_symbol(expr, [term])
        u = sym(u_name)
        recip = div(coeff, add([u, neg(shift)]))
        x_sub = sim(div(add([recip, neg(b)]), a))
        candidate = canonical_compare_form(substitute_keep_form(expr, sym(var_name), x_sub))
        back_sub = substitute_keep_form(candidate, u, term)
        samples = [num(-3), num(-2), num(-1), num(0), num(1), num(2), num(3)]
        ok_count = 0
        i = 0
        while i < len(samples):
            try:
                left = eval_with_values(expr, {var_name: samples[i]})
                right = eval_with_values(back_sub, {var_name: samples[i]})
            except Exception:
                i += 1
                continue
            if left is not None and right is not None:
                if not same(left, right):
                    raise ValueError('Cannot rewrite in that term.')
                ok_count += 1
            i += 1
        if ok_count >= 3:
            return ['Start with ' + show(expr), 'Write in terms of ' + show(term) + ' only.', u_name + ' = ' + show(term), 'Answer: ' + show(candidate)]
    raise ValueError('Cannot rewrite in that term.')


def poly_mode_text(text):
    expr = parse(text.strip())
    expanded = expand_for_solving(expr)
    if not same(expanded, expr):
        return [
            'Input = ' + show(expr),
            'Expand brackets and collect like terms.',
            'Out = ' + show(expanded),
        ]
    factored = factor_expression(expr)
    if factored is not None:
        if same(factored[0], expr):
            return ['Input = ' + show(expr), 'Already expanded or factorised.', 'Out = ' + show(expr)]
        return ['Input = ' + show(expr), factored[1], 'Out = ' + show(factored[0])]
    out = maybe_expand_for_compare(expr)
    if same(out, expr):
        return ['Input = ' + show(expr), 'Already expanded or no further polynomial expansion found.', 'Out = ' + show(expr)]
    return ['Input = ' + show(expr), 'Expand brackets and collect like terms.', 'Out = ' + show(out)]


def solve_rewrite_text(text, term_texts):
    if len(term_texts) == 0:
        raise ValueError('Use at least one target term.')
    text_is_node = isinstance(text, tuple)
    original_text = show(sim(text)) if text_is_node else text.strip()
    if len(term_texts) == 1 and not text_is_node and not isinstance(term_texts[0], tuple):
        try:
            return rewrite_in_term_text(original_text, term_texts[0])
        except ValueError:
            pass
    allowed_terms = []
    i = 0
    while i < len(term_texts):
        if isinstance(term_texts[i], tuple):
            allowed_terms.append(sim(term_texts[i]))
        else:
            allowed_terms.append(parse(term_texts[i].strip()))
        i += 1
    if text_is_node:
        is_equation = False
        expr = sim(text)
    else:
        parts = split_top_level(original_text, '=')
        is_equation = parts is not None
        if is_equation:
            lhs, rhs = parse_equation_or_zero(original_text)
            expr = sim(sub(lhs, rhs))
        else:
            expr = parse(original_text)
    final_expr, steps, info = search_rewrite_expression(expr, allowed_terms)
    if final_expr is None:
        if rewrite_allowed_only(expr, info):
            final_expr = expr
            steps = []
        elif equivalent(expr, num(1)):
            final_expr = num(1)
            steps = [('Use a standard identity', num(1))]
        else:
            raise ValueError('Cannot rewrite in that term.')
    return format_rewrite_lines(original_text, expr, final_expr, steps or [], allowed_terms, is_equation)


def factor_text(text):
    expr = parse(text.strip())
    factored = factor_expression(expr)
    if factored is None:
        quad = factor_quadratic_rational(expr)
        if quad is not None:
            return ['Input = ' + show(expr), quad[1], '= ' + show(quad[0])]
        out = maybe_expand_for_compare(expr)
        if not same(out, expr):
            return ['Input = ' + show(expr), 'Out = ' + show(out)]
        return ['Input = ' + show(expr), 'No factor']
    if same(factored[0], expr):
        return ['Input = ' + show(expr), 'Out = ' + show(expr)]
    return ['Input = ' + show(expr), factored[1], '= ' + show(factored[0])]


MENU_LINE_WIDTH = 20
MENU_PAGE_LINES = 5


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
    mode = paged_menu_input('M', [
        ('1', 'cmp'),
        ('2', 'xform'),
        ('3', 'exp'),
        ('4', 'poly'),
        ('5', 'comp sq'),
        ('6', 'solve'),
        ('7', 'comp'),
        ('8', 'inv'),
        ('9', 'rw'),
        ('10', 'dom/rng'),
        ('11', 'cart'),
    ], '1')
    begin_user_action()
    try:
        if mode == '1':
            text1 = input('E1: ').strip()
            text2 = input('E2: ').strip()
            equal, steps = compare_mode_text(text1, text2)
            i = 0
            while i < len(steps):
                num, desc, _ = steps[i]
                print(str(num) + '. ' + desc)
                i += 1
            if equal:
                print(str(len(steps) + 1) + '. Result: Equivalent.')
            else:
                print(str(len(steps) + 1) + '. Result: Not equivalent.')
        elif mode == '2':
            text1 = input('E1: ').strip()
            text2 = input('E2: ').strip()
            lines = solve_transform_text(text1, text2)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        elif mode == '3':
            text1 = input('Expr: ').strip()
            max_terms_str = input('Max: ').strip()
            max_terms = None
            if max_terms_str != '':
                try:
                    max_terms = int(max_terms_str)
                except:
                    pass
            lines = expand_mode_text(text1, max_terms)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        elif mode == '4':
            text = input('Poly: ').strip()
            if text == '':
                raise ValueError('Enter a polynomial.')
            lines = poly_mode_text(text)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        elif mode == '5':
            text = input('Expression: ').strip()
            if text == '':
                raise ValueError('Enter an expression.')
            lines = complete_square_text(text)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        elif mode == '6':
            text = input('Eq: ').strip()
            lines = solve_equation_text(text)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        elif mode == '7':
            print('Comp f(g(x))')
            f_text = input('f: ').strip()
            g_text = input('g: ').strip()
            if f_text == '':
                f_text = '2*x+1'
                g_text = 'x^2'
                print('Use: f=' + f_text + ', g=' + g_text)
            result, steps = compose_functions(f_text, g_text)
            if result:
                i = 0
                while i < len(steps):
                    print(str(i+1) + '. ' + steps[i])
                    i += 1
            else:
                print('1. ' + steps[0])
        elif mode == '8':
            print('Inv fn')
            f_text = input('f: ').strip()
            if f_text == '':
                f_text = '2*x+1'
                print('Use: f=' + f_text)
            result, steps = inverse_function(f_text)
            i = 0
            while i < len(steps):
                print(str(i+1) + '. ' + steps[i])
                i += 1
        elif mode == '9':
            text = input('Rw: ').strip()
            terms = []
            first = True
            while True:
                term = input('1 term/line | Blank = end | T: ' if first else 'T: ').strip()
                first = False
                if term == '':
                    break
                terms.append(term)
            lines = solve_rewrite_text(text, terms)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        elif mode == '10':
            text = input('Expr: ').strip()
            if text == '':
                raise ValueError('Enter an expression.')
            lines = find_domain_text(text)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
            print()
            lines = find_range_text(text)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        elif mode == '11':
            print('Cartesian from parametric')
            x_text = input('x(t): ').strip()
            y_text = input('y(t): ').strip()
            param = input('Param: ').strip()
            if param == '':
                param = 't'
            lines = cartesian_parametric_lines(x_text, y_text, param)
            i = 0
            while i < len(lines):
                print(str(i + 1) + '. ' + lines[i])
                i += 1
        else:
            print('Bad mode.')
    except Exception as err:
        print('Err: ' + str(err))


def find_domain_text(text):
    try:
        expr = parse(text.strip())
    except:
        return ['Err: Unable to parse expression.']
    
    var_name = choose_primary_var(expr)
    if var_name is None:
        return ['Err: No variable found to determine domain.']
    
    lines = ['Method: Determine Domain', 'Input = ' + show(expr)]
    restrictions = []

    def inequality_text(rel, value):
        return var_name + ' ' + rel + ' ' + show(value)

    def bounded_text(left, left_rel, right, right_rel):
        return show(left) + ' ' + left_rel + ' ' + var_name + ' ' + right_rel + ' ' + show(right)

    def solve_constraint_text(node, relation):
        linear = match_linear_in_var(node, var_name)
        if linear is not None:
            coeff, constant = linear
            if not is_num(coeff) or is_zero(coeff):
                return None
            root = sim(div(neg(constant), coeff))
            if relation == '!=':
                return inequality_text('!=', root)
            coeff_positive = coeff[1] > 0
            if relation in ('>', '>='):
                rel = '>' if coeff_positive else '<'
            else:
                rel = '<' if coeff_positive else '>'
            if relation in ('>=', '<='):
                rel += '='
            return inequality_text(rel, root)
        coeffs, degree = polynomial_coeff_list(node, var_name, 2)
        if coeffs is None or degree != 2 or not is_num(coeffs[2]):
            return None
        roots = normalize_solution_roots(rational_roots_for_quadratic(coeffs))
        roots.sort(key=lambda item: real_numeric_value(item) if real_numeric_value(item) is not None else show(item))
        lead_positive = coeffs[2][1] > 0
        if len(roots) == 0:
            if relation in ('>', '>='):
                return 'all real ' + var_name if lead_positive else 'no real ' + var_name
            return 'all real ' + var_name if not lead_positive else 'no real ' + var_name
        if len(roots) == 1:
            root = roots[0]
            if relation == '>':
                return 'all real ' + var_name + ' except ' + show(root) if lead_positive else 'no real ' + var_name
            if relation == '>=':
                return 'all real ' + var_name if lead_positive else var_name + ' = ' + show(root)
            if relation == '<':
                return 'all real ' + var_name + ' except ' + show(root) if not lead_positive else 'no real ' + var_name
            if relation == '<=':
                return 'all real ' + var_name if not lead_positive else var_name + ' = ' + show(root)
            return None
        left = roots[0]
        right = roots[1]
        if relation == '>':
            if lead_positive:
                return inequality_text('<', left) + ' or ' + inequality_text('>', right)
            return bounded_text(left, '<', right, '<')
        if relation == '>=':
            if lead_positive:
                return inequality_text('<=', left) + ' or ' + inequality_text('>=', right)
            return bounded_text(left, '<=', right, '<=')
        if relation == '<':
            if lead_positive:
                return bounded_text(left, '<', right, '<')
            return inequality_text('<', left) + ' or ' + inequality_text('>', right)
        if relation == '<=':
            if lead_positive:
                return bounded_text(left, '<=', right, '<=')
            return inequality_text('<=', left) + ' or ' + inequality_text('>=', right)
        return None

    def check_restrictions(node):
        if node is None:
            return
        if node[0] == 'add' or node[0] == 'mul':
            k = 0
            while k < len(node[1]):
                check_restrictions(node[1][k])
                k += 1
        elif node[0] == 'div':
            num_node, den_node = node[1], node[2]
            check_restrictions(num_node)
            if den_node[0] == 'fn' and den_node[1] == 'sqrt':
                check_restrictions(den_node[2])
                restrictions.append(('>', den_node[2]))
            else:
                check_restrictions(den_node)
                if not is_zero(den_node):
                    restrictions.append(('!=', den_node))
        elif node[0] == 'pow':
            base, exp = node[1], node[2]
            check_restrictions(base)
            check_restrictions(exp)
            if is_num(exp) and exp[1] % 2 == 0 and exp[1] < 0:
                restrictions.append(('!=', base))
        elif node[0] == 'fn':
            name = node[1]
            arg = node[2]
            check_restrictions(arg)
            if name in ('sqrt', 'asin', 'acos'):
                restrictions.append(('>=', arg))
            elif name in ('ln', 'log', 'log10'):
                restrictions.append(('>', arg))

    check_restrictions(expr)

    if not restrictions:
        lines.append('Domain: all real ' + var_name)
        return ensure_reasoning_marker(lines)

    constraints = []
    solved_constraints = []
    seen_constraints = {}
    seen_solutions = {}
    for relation, node in restrictions:
        shown = show(node)
        if relation == '!=':
            constraint = shown + ' != 0'
        else:
            constraint = shown + ' ' + relation + ' 0'
        if constraint not in seen_constraints:
            seen_constraints[constraint] = 1
            constraints.append(constraint)
        solved = solve_constraint_text(node, relation)
        if solved is not None and solved not in seen_solutions:
            seen_solutions[solved] = 1
            solved_constraints.append(solved)
    if len(constraints) != 0:
        lines.append(', '.join(constraints))
    if len(solved_constraints) != 0:
        solved_text = ' and '.join(solved_constraints)
        lines.append('Solve: ' + solved_text)
        lines.append('Domain: ' + solved_text)
    else:
        lines.append('Domain: all real ' + var_name + ' except where restrictions apply.')
    return ensure_reasoning_marker(lines)


def find_range_text(text):
    try:
        expr = parse(text.strip())
    except:
        return ['Err: Unable to parse expression.']
    
    var_name = choose_primary_var(expr)
    if var_name is None:
        return ['Err: No variable found to determine range.']
    
    lines = ['Method: Determine Range', 'Input = ' + show(expr)]

    def quadratic_vertex_value(coeffs):
        return sim(sub(coeffs[0], div(power(coeffs[1], num(2)), mul([num(4), coeffs[2]]))))

    coeffs, degree = polynomial_coeff_list(expr, var_name, 2)
    if coeffs is not None and degree == 2 and is_num(coeffs[2]):
        extrema_value = quadratic_vertex_value(coeffs)
        if coeffs[2][1] > 0:
            lines.append('Range: y >= ' + show(extrema_value))
        elif coeffs[2][1] < 0:
            lines.append('Range: y <= ' + show(extrema_value))
        else:
            lines.append('Range: all real y')
        return ensure_reasoning_marker(lines)

    if expr[0] == 'fn' and expr[1] == 'sqrt':
        inner_coeffs, inner_degree = polynomial_coeff_list(expr[2], var_name, 2)
        if inner_coeffs is not None and inner_degree == 2 and is_num(inner_coeffs[2]) and inner_coeffs[2][1] < 0:
            top = quadratic_vertex_value(inner_coeffs)
            lines.append('Range: 0 <= y <= ' + show(fn('sqrt', top)))
        else:
            lines.append('Range: y >= 0')
        return ensure_reasoning_marker(lines)

    if expr[0] == 'div' and is_one(expr[1]) and expr[2][0] == 'fn' and expr[2][1] == 'sqrt':
        inner_coeffs, inner_degree = polynomial_coeff_list(expr[2][2], var_name, 2)
        if inner_coeffs is not None and inner_degree == 2 and is_num(inner_coeffs[2]) and inner_coeffs[2][1] < 0:
            top = quadratic_vertex_value(inner_coeffs)
            lines.append('Range: y >= ' + show(sim(div(num(1), fn('sqrt', top)))))
        else:
            lines.append('Range: y > 0')
        return ensure_reasoning_marker(lines)
    
    if expr[0] == 'sym' or (expr[0] == 'mul' and expr[1][0] == 'num'):
        lines.append('Range: ' + show(expr))
        return ensure_reasoning_marker(lines)

    if is_num(expr):
        lines.append('Range: ' + show(expr))
        return ensure_reasoning_marker(lines)

    # Handle rational functions: 1/f(x) where f(x) can be zero
    if expr[0] == 'div' and is_one(expr[1]):
        # It's 1/something - range excludes 0 if denominator can be zero
        lines.append('Range: y != 0')
        return ensure_reasoning_marker(lines)

    # Handle a/f(x) where a is constant non-zero
    if expr[0] == 'div' and is_num(expr[1]) and not is_zero(expr[1]):
        lines.append('Range: y != 0')
        return ensure_reasoning_marker(lines)
    
    lines.append('Range: all real y')
    return ensure_reasoning_marker(lines)


run = main
if not SKIP_AUTORUN:
    main()
