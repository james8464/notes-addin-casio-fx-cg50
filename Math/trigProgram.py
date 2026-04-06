try:
    import math
except ImportError:
    math = None
try:
    import sys
except ImportError:
    sys = None


# ---------------------------------------------------------------------------
# Core constants and runtime profile
# ---------------------------------------------------------------------------
E = ("const", "e")
PI = ("const", "pi")
FUNC_NAMES = (
    "sin",
    "cos",
    "tan",
    "sec",
    "cosec",
    "cot",
    "exp",
    "sqrt",
    "abs",
    "ln",
    "log",
)
FUNC_ALIASES = {
    "csc": "cosec",
}
SKIP_AUTORUN = sys is not None and getattr(sys, "_trig_no_autorun", False)

# Formula-booklet identities
FORMULA_SIN_ADD = "Use sin(A+B) = sin A cos B + cos A sin B."
FORMULA_SIN_SUB = "Use sin(A-B) = sin A cos B - cos A sin B."
FORMULA_COS_ADD = "Use cos(A+B) = cos A cos B - sin A sin B."
FORMULA_COS_SUB = "Use cos(A-B) = cos A cos B + sin A sin B."
FORMULA_TAN_ADD = "Use tan(A+B) = (tan A + tan B)/(1 - tan A tan B)."
FORMULA_TAN_SUB = "Use tan(A-B) = (tan A - tan B)/(1 + tan A tan B)."
FORMULA_COT_ADD_TAN = "Use cot(A+B) = (1 - tan A tan B)/(tan A + tan B)."
FORMULA_COT_SUB_TAN = "Use cot(A-B) = (1 + tan A tan B)/(tan A - tan B)."
FORMULA_SIN_PLUS_SIN = "Use sin A + sin B = 2sin((A+B)/2)cos((A-B)/2)."
FORMULA_SIN_MINUS_SIN = "Use sin A - sin B = 2cos((A+B)/2)sin((A-B)/2)."
FORMULA_COS_PLUS_COS = "Use cos A + cos B = 2cos((A+B)/2)cos((A-B)/2)."
FORMULA_COS_MINUS_COS = "Use cos A - cos B = -2sin((A+B)/2)sin((A-B)/2)."
FORMULA_LOG_BASE_CHANGE = "Use log_a x = log_b x / log_b a."
FORMULA_EXP_BASE_CHANGE = "Use e^(x ln a) = a^x."

FAST_GCD = math.gcd if math is not None and hasattr(math, "gcd") else None
FAST_ISQRT = math.isqrt if math is not None and hasattr(math, "isqrt") else None
FAST_ISFINITE = math.isfinite if math is not None and hasattr(math, "isfinite") else None
PI_FLOAT = math.pi if math is not None and hasattr(math, "pi") else 3.141592653589793
E_FLOAT = math.e if math is not None and hasattr(math, "e") else 2.718281828459045

MICROPYTHON_RUNTIME = (
    sys is not None
    and getattr(getattr(sys, "implementation", None), "name", "") == "micropython"
)

DESKTOP_CACHE_LIMIT_SMALL = 2048
DESKTOP_CACHE_LIMIT_MEDIUM = 4096
DESKTOP_CACHE_LIMIT_LARGE = 8192
LOWMEM_CACHE_LIMIT_SMALL = 128
LOWMEM_CACHE_LIMIT_MEDIUM = 256
LOWMEM_CACHE_LIMIT_LARGE = 512

CACHE_LIMIT_SMALL = DESKTOP_CACHE_LIMIT_SMALL
CACHE_LIMIT_MEDIUM = DESKTOP_CACHE_LIMIT_MEDIUM
CACHE_LIMIT_LARGE = DESKTOP_CACHE_LIMIT_LARGE
SOLVE_PASS_LIMIT = 8
FACTOR_REWRITE_PASS_LIMIT = 3
FACTOR_REWRITE_DEPTH_LIMIT = 2
SOLVE_LOOKAHEAD_ENABLED = True
LOW_MEMORY_RUNTIME = False

# The engine reuses immutable tuple-AST nodes heavily, so these caches
# keep repeated simplify/show/match passes cheap without pulling in a CAS.
SIG_CACHE = {}
SIM_CACHE = {}
SHOW_CACHE = {}
SORT_KEY_CACHE = {}
FULL_SIMPLIFY_CACHE = {}
EQUIVALENT_CACHE = {}
FLAT_ADD_CACHE = {}
FLAT_MUL_CACHE = {}
SYMBOLS_CACHE = {}
SPLIT_COEFF_CACHE = {}
SPLIT_NUM_FACTOR_CACHE = {}
MATCH_FN_POWER_CACHE = {}
MATCH_DOUBLE_FN_CACHE = {}
FUNCTIONS_CACHE = {}
KINDS_CACHE = {}
TREE_SIZE_CACHE = {}
RECIPROCAL_BURDEN_CACHE = {}
TRIG_ARG_KEYS_CACHE = {}
SOLVE_FEATURE_CACHE = {}
FINAL_ANGLE_TEXT_CACHE = {}
SOLUTION_LIST_CACHE = {}


def cache_store(cache, key, value, limit):
    if limit <= 0:
        return value
    if key not in cache and len(cache) >= limit:
        trim = limit // 8
        if trim < 1:
            trim = 1
        i = 0
        while i < trim and len(cache) >= limit:
            oldest = next(iter(cache))
            del cache[oldest]
            i += 1
    cache[key] = value
    return value


ALL_ENGINE_CACHES = (
    SIG_CACHE,
    SIM_CACHE,
    SHOW_CACHE,
    SORT_KEY_CACHE,
    FULL_SIMPLIFY_CACHE,
    EQUIVALENT_CACHE,
    FLAT_ADD_CACHE,
    FLAT_MUL_CACHE,
    SYMBOLS_CACHE,
    SPLIT_COEFF_CACHE,
    SPLIT_NUM_FACTOR_CACHE,
    MATCH_FN_POWER_CACHE,
    MATCH_DOUBLE_FN_CACHE,
    FUNCTIONS_CACHE,
    KINDS_CACHE,
    TREE_SIZE_CACHE,
    RECIPROCAL_BURDEN_CACHE,
    TRIG_ARG_KEYS_CACHE,
    SOLVE_FEATURE_CACHE,
    FINAL_ANGLE_TEXT_CACHE,
    SOLUTION_LIST_CACHE,
)


def clear_engine_caches():
    # Low-memory calculators benefit more from clearing a handful of hot caches
    # between user actions than from holding onto lots of old subtrees.
    i = 0
    while i < len(ALL_ENGINE_CACHES):
        ALL_ENGINE_CACHES[i].clear()
        i += 1


def apply_runtime_profile(low_memory=None):
    # The desktop build can afford deeper rewrite passes and larger caches.
    # The calculator profile keeps the same maths logic but trims runtime state.
    global CACHE_LIMIT_SMALL, CACHE_LIMIT_MEDIUM, CACHE_LIMIT_LARGE
    global SOLVE_PASS_LIMIT, FACTOR_REWRITE_PASS_LIMIT, FACTOR_REWRITE_DEPTH_LIMIT
    global SOLVE_LOOKAHEAD_ENABLED, LOW_MEMORY_RUNTIME
    if low_memory is None:
        low_memory = MICROPYTHON_RUNTIME
    LOW_MEMORY_RUNTIME = bool(low_memory)
    if LOW_MEMORY_RUNTIME:
        CACHE_LIMIT_SMALL = LOWMEM_CACHE_LIMIT_SMALL
        CACHE_LIMIT_MEDIUM = LOWMEM_CACHE_LIMIT_MEDIUM
        CACHE_LIMIT_LARGE = LOWMEM_CACHE_LIMIT_LARGE
        SOLVE_PASS_LIMIT = 6
        FACTOR_REWRITE_PASS_LIMIT = 2
        FACTOR_REWRITE_DEPTH_LIMIT = 2
        SOLVE_LOOKAHEAD_ENABLED = False
    else:
        CACHE_LIMIT_SMALL = DESKTOP_CACHE_LIMIT_SMALL
        CACHE_LIMIT_MEDIUM = DESKTOP_CACHE_LIMIT_MEDIUM
        CACHE_LIMIT_LARGE = DESKTOP_CACHE_LIMIT_LARGE
        SOLVE_PASS_LIMIT = 8
        FACTOR_REWRITE_PASS_LIMIT = 3
        FACTOR_REWRITE_DEPTH_LIMIT = 2
        SOLVE_LOOKAHEAD_ENABLED = False
    clear_engine_caches()


def begin_user_action():
    # Calculator runs are short and interactive, so clearing caches here keeps
    # memory usage predictable without changing user-facing behaviour.
    if LOW_MEMORY_RUNTIME:
        clear_engine_caches()


def _force_low_memory_runtime(flag):
    apply_runtime_profile(flag)


apply_runtime_profile()


def to_radians(value):
    return value * PI_FLOAT / 180.0


def to_degrees(value):
    return value * 180.0 / PI_FLOAT


def is_finite_value(value):
    if FAST_ISFINITE is not None:
        return FAST_ISFINITE(value)
    inf = float("inf")
    return value == value and value != inf and value != -inf


# ---------------------------------------------------------------------------
# Basic AST constructors and helpers
# ---------------------------------------------------------------------------
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
        raise ValueError("Division by zero.")
    if b < 0:
        a = -a
        b = -b
    g = gcd(a, b)
    return ("num", a // g, b // g)


def num_text(text):
    if "." not in text:
        return num(int(text))
    left, right = text.split(".", 1)
    if left == "":
        left = "0"
    if right == "":
        right = "0"
    scale = 1
    i = 0
    while i < len(right):
        scale *= 10
        i += 1
    sign = 1
    if left[:1] == "-":
        sign = -1
        left = left[1:]
    whole = left + right
    if whole == "":
        whole = "0"
    return num(sign * int(whole), scale)


def sym(name):
    return ("sym", name)


def is_num(node):
    return node[0] == "num"


def is_sym(node):
    return node[0] == "sym"


def is_const(node):
    return node[0] == "const"


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


def int_pow(a, n):
    if n >= 0:
        return num(a[1] ** n, a[2] ** n)
    n = -n
    return num(a[2] ** n, a[1] ** n)


def make_add(parts):
    out = []
    i = 0
    while i < len(parts):
        item = parts[i]
        if item[0] == "add":
            out.extend(item[1])
        else:
            out.append(item)
        i += 1
    if len(out) == 0:
        return num(0)
    if len(out) == 1:
        return out[0]
    return ("add", tuple(out))


def make_mul(parts):
    out = []
    i = 0
    while i < len(parts):
        item = parts[i]
        if item[0] == "mul":
            out.extend(item[1])
        else:
            out.append(item)
        i += 1
    if len(out) == 0:
        return num(1)
    if len(out) == 1:
        return out[0]
    return ("mul", tuple(out))


def make_display_mul(parts):
    # Pretty-print helper: keep the product structure for proof/solve display,
    # but drop harmless 1 factors so the working lines stay readable.
    out = []
    i = 0
    while i < len(parts):
        item = parts[i]
        if item[0] == "mul":
            inner = list(flat(item, "mul"))
            j = 0
            while j < len(inner):
                if not is_one(inner[j]):
                    out.append(inner[j])
                j += 1
        elif not is_one(item):
            out.append(item)
        i += 1
    if len(out) == 0:
        return num(1)
    if len(out) == 1:
        return out[0]
    return ("mul", tuple(out))


def add(parts):
    return sim(make_add(parts))


def mul(parts):
    return sim(make_mul(parts))


def div(a, b):
    return sim(("div", a, b))


def power(a, b):
    return sim(("pow", a, b))


def fn(name, arg):
    if name == "ln":
        name = "log"
    if name == "csc":
        name = "cosec"
    return sim(("fn", name, arg))


def neg(node):
    return mul([num(-1), node])


def flat(node, kind):
    if node[0] != kind:
        return (node,)
    items = node[1]
    i = 0
    while i < len(items):
        if items[i][0] == kind:
            break
        i += 1
    if i == len(items):
        return items
    cache = None
    if kind == "add":
        cache = FLAT_ADD_CACHE
    elif kind == "mul":
        cache = FLAT_MUL_CACHE
    if cache is not None:
        cached = cache.get(node)
        if cached is not None:
            return cached
    out = []
    i = 0
    while i < len(items):
        item = items[i]
        if item[0] == kind:
            out.extend(flat(item, kind))
        else:
            out.append(item)
        i += 1
    out = tuple(out)
    if cache is not None:
        return cache_store(cache, node, out, CACHE_LIMIT_MEDIUM)
    return out


def _sig_uncached(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return ("fn", node[1], sig(node[2]))
    if kind == "pow":
        return ("pow", sig(node[1]), sig(node[2]))
    if kind == "div":
        return ("div", sig(node[1]), sig(node[2]))
    items = []
    for item in flat(node, kind):
        items.append(sig(item))
    items.sort()
    return (kind, tuple(items))


def sig(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    cached = SIG_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(SIG_CACHE, node, _sig_uncached(node), CACHE_LIMIT_LARGE)


def same(a, b):
    return a == b or sig(a) == sig(b)


def cheap_same(a, b):
    if a is b or a == b:
        return True
    if a[0] != b[0]:
        return False
    if a[0] == "fn" and a[1] != b[1]:
        return False
    return sig(a) == sig(b)


def _symbols_uncached(node):
    kind = node[0]
    if kind == "sym":
        return (node[1],)
    if kind in ("num", "const"):
        return ()
    if kind == "fn":
        return symbols_of(node[2])
    if kind in ("pow", "div"):
        left = symbols_of(node[1])
        right = symbols_of(node[2])
        if len(left) == 0:
            return right
        if len(right) == 0:
            return left
        seen = {}
        out = []
        i = 0
        while i < len(left):
            if left[i] not in seen:
                seen[left[i]] = 1
                out.append(left[i])
            i += 1
        i = 0
        while i < len(right):
            if right[i] not in seen:
                seen[right[i]] = 1
                out.append(right[i])
            i += 1
        return tuple(out)
    seen = {}
    out = []
    i = 0
    while i < len(node[1]):
        part = symbols_of(node[1][i])
        j = 0
        while j < len(part):
            if part[j] not in seen:
                seen[part[j]] = 1
                out.append(part[j])
            j += 1
        i += 1
    return tuple(out)


def symbols_of(node):
    kind = node[0]
    if kind == "sym":
        return (node[1],)
    if kind in ("num", "const"):
        return ()
    cached = SYMBOLS_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(SYMBOLS_CACHE, node, _symbols_uncached(node), CACHE_LIMIT_MEDIUM)


def _function_names_uncached(node):
    kind = node[0]
    if kind == "fn":
        names = _function_names_uncached(node[2])
        if node[1] in names:
            return names
        return (node[1],) + names
    if kind in ("num", "sym", "const"):
        return ()
    if kind in ("pow", "div"):
        left = function_names_of(node[1])
        right = function_names_of(node[2])
    else:
        left = ()
        right = ()
        i = 0
        while i < len(node[1]):
            part = function_names_of(node[1][i])
            if len(left) == 0:
                left = part
            elif len(part) != 0:
                seen = {}
                out = []
                j = 0
                while j < len(left):
                    if left[j] not in seen:
                        seen[left[j]] = 1
                        out.append(left[j])
                    j += 1
                j = 0
                while j < len(part):
                    if part[j] not in seen:
                        seen[part[j]] = 1
                        out.append(part[j])
                    j += 1
                left = tuple(out)
            i += 1
        return left
    if len(left) == 0:
        return right
    if len(right) == 0:
        return left
    seen = {}
    out = []
    i = 0
    while i < len(left):
        if left[i] not in seen:
            seen[left[i]] = 1
            out.append(left[i])
        i += 1
    i = 0
    while i < len(right):
        if right[i] not in seen:
            seen[right[i]] = 1
            out.append(right[i])
        i += 1
    return tuple(out)


def function_names_of(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return ()
    cached = FUNCTIONS_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(FUNCTIONS_CACHE, node, _function_names_uncached(node), CACHE_LIMIT_MEDIUM)


def _kind_names_uncached(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return (kind,)
    seen = {kind: 1}
    out = [kind]
    if kind == "fn":
        part = kind_names_of(node[2])
        i = 0
        while i < len(part):
            if part[i] not in seen:
                seen[part[i]] = 1
                out.append(part[i])
            i += 1
        return tuple(out)
    if kind in ("pow", "div"):
        items = (node[1], node[2])
    else:
        items = node[1]
    i = 0
    while i < len(items):
        part = kind_names_of(items[i])
        j = 0
        while j < len(part):
            if part[j] not in seen:
                seen[part[j]] = 1
                out.append(part[j])
            j += 1
        i += 1
    return tuple(out)


def kind_names_of(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return (kind,)
    cached = KINDS_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(KINDS_CACHE, node, _kind_names_uncached(node), CACHE_LIMIT_MEDIUM)


def _tree_size_uncached(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return 1
    if kind == "fn":
        return 1 + tree_size(node[2])
    if kind in ("pow", "div"):
        return 1 + tree_size(node[1]) + tree_size(node[2])
    total = 1
    i = 0
    while i < len(node[1]):
        total += tree_size(node[1][i])
        i += 1
    return total


def tree_size(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return 1
    cached = TREE_SIZE_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(TREE_SIZE_CACHE, node, _tree_size_uncached(node), CACHE_LIMIT_MEDIUM)


def _reciprocal_burden_uncached(node):
    kind = node[0]
    score = 0
    if kind == "div":
        score += 1
        return score + reciprocal_burden(node[1]) + reciprocal_burden(node[2])
    if kind == "fn":
        if node[1] in ("sec", "cosec", "cot"):
            score += 1
        return score + reciprocal_burden(node[2])
    if kind in ("pow",):
        return reciprocal_burden(node[1]) + reciprocal_burden(node[2])
    if kind in ("num", "sym", "const"):
        return 0
    i = 0
    while i < len(node[1]):
        score += reciprocal_burden(node[1][i])
        i += 1
    return score


def reciprocal_burden(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return 0
    cached = RECIPROCAL_BURDEN_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(RECIPROCAL_BURDEN_CACHE, node, _reciprocal_burden_uncached(node), CACHE_LIMIT_MEDIUM)


def _trig_arg_keys_uncached(node):
    kind = node[0]
    if kind == "fn":
        keys = _trig_arg_keys_uncached(node[2])
        if node[1] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            cur = sig(node[2])
            if cur in keys:
                return keys
            return (cur,) + keys
        return keys
    if kind in ("num", "sym", "const"):
        return ()
    if kind in ("pow", "div"):
        left = trig_arg_keys_of(node[1])
        right = trig_arg_keys_of(node[2])
    else:
        left = ()
        right = ()
        i = 0
        while i < len(node[1]):
            part = trig_arg_keys_of(node[1][i])
            if len(left) == 0:
                left = part
            elif len(part) != 0:
                seen = {}
                out = []
                j = 0
                while j < len(left):
                    if left[j] not in seen:
                        seen[left[j]] = 1
                        out.append(left[j])
                    j += 1
                j = 0
                while j < len(part):
                    if part[j] not in seen:
                        seen[part[j]] = 1
                        out.append(part[j])
                    j += 1
                left = tuple(out)
            i += 1
        return left
    if len(left) == 0:
        return right
    if len(right) == 0:
        return left
    seen = {}
    out = []
    i = 0
    while i < len(left):
        if left[i] not in seen:
            seen[left[i]] = 1
            out.append(left[i])
        i += 1
    i = 0
    while i < len(right):
        if right[i] not in seen:
            seen[right[i]] = 1
            out.append(right[i])
        i += 1
    return tuple(out)


def trig_arg_keys_of(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return ()
    cached = TRIG_ARG_KEYS_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(TRIG_ARG_KEYS_CACHE, node, _trig_arg_keys_uncached(node), CACHE_LIMIT_MEDIUM)


def depends(node, name):
    return name in symbols_of(node)


def collect_symbols(node, out):
    names = symbols_of(node)
    i = 0
    while i < len(names):
        out.add(names[i])
        i += 1


def split_coeff(node):
    if is_num(node):
        return node, num(1)
    cached = SPLIT_COEFF_CACHE.get(node)
    if cached is not None:
        return cached
    if node[0] == "mul":
        items = node[1]
        if items and items[0][0] != "mul" and is_num(items[0]):
            if len(items) == 2:
                return cache_store(SPLIT_COEFF_CACHE, node, (items[0], items[1]), CACHE_LIMIT_MEDIUM)
            return cache_store(SPLIT_COEFF_CACHE, node, (items[0], ("mul", tuple(items[1:]))), CACHE_LIMIT_MEDIUM)
        items = flat(node, "mul")
        if items and is_num(items[0]):
            if len(items) == 2:
                return cache_store(SPLIT_COEFF_CACHE, node, (items[0], items[1]), CACHE_LIMIT_MEDIUM)
            return cache_store(SPLIT_COEFF_CACHE, node, (items[0], ("mul", tuple(items[1:]))), CACHE_LIMIT_MEDIUM)
    return cache_store(SPLIT_COEFF_CACHE, node, (num(1), node), CACHE_LIMIT_MEDIUM)


def split_num_factor(node):
    node = sim(node)
    if is_num(node):
        return node, num(1)
    cached = SPLIT_NUM_FACTOR_CACHE.get(node)
    if cached is not None:
        return cached
    if node[0] == "mul":
        items = node[1]
        if items and is_num(items[0]):
            if len(items) == 2:
                return cache_store(SPLIT_NUM_FACTOR_CACHE, node, (items[0], items[1]), CACHE_LIMIT_SMALL)
            return cache_store(SPLIT_NUM_FACTOR_CACHE, node, (items[0], make_mul(items[1:])), CACHE_LIMIT_SMALL)
    return cache_store(SPLIT_NUM_FACTOR_CACHE, node, (num(1), node), CACHE_LIMIT_SMALL)


def split_const_factor(node, var):
    node = sim(node)
    if is_num(node):
        return node, num(1)
    if node[0] == "div" and not depends(node[2], var):
        coeff, rest = split_const_factor(node[1], var)
        return sim(div(coeff, node[2])), rest
    if node[0] == "mul":
        items = node[1]
        coeff = num(1)
        rest_bits = []
        i = 0
        while i < len(items):
            if not depends(items[i], var):
                coeff = sim(mul([coeff, items[i]]))
            else:
                rest_bits.append(items[i])
            i += 1
        if len(rest_bits) == 0:
            return coeff, num(1)
        return coeff, make_mul(rest_bits)
    return num(1), node


def int_sqrt(n):
    if n < 0:
        return None
    if FAST_ISQRT is not None:
        root = FAST_ISQRT(n)
        if root * root == n:
            return root
        return None
    x = 0
    while x * x < n:
        x += 1
    if x * x == n:
        return x
    return None


def sqrt_num(node):
    if not is_num(node) or node[1] < 0:
        return None
    a = int_sqrt(node[1])
    b = int_sqrt(node[2])
    if a is not None and b is not None:
        return num(a, b)
    if a is not None and a != 1:
        return div(num(a), fn("sqrt", num(node[2])))
    if a == 1 and node[2] != 1:
        return div(num(1), fn("sqrt", num(node[2])))
    if b is not None and b != 1:
        return mul([num(1, b), fn("sqrt", num(node[1]))])
    return None


def norm_pow_base(node):
    if node[0] != "pow" or not is_num(node[1]):
        return node
    base = node[1]
    exp = node[2]
    if base[1] <= 0 or base[2] != 1:
        return node
    n = base[1]
    p = 2
    while p * p <= n:
        if n % p == 0:
            k = 0
            while n % p == 0:
                n //= p
                k += 1
            if n == 1 and k > 1:
                return ("pow", num(p), add([num(k), exp, num(0)]))
            return node
        p += 1
    return node


def factor_map(node):
    if node[0] == "mul":
        items = node[1]
    else:
        items = (node,)
    coeff = num(1)
    order = []
    data = {}
    i = 0
    while i < len(items):
        item = norm_pow_base(items[i])
        if is_num(item):
            coeff = mulq(coeff, item)
        elif item[0] == "fn" and item[1] == "sqrt":
            key = sig(item[2])
            if key not in data:
                order.append(key)
                data[key] = [item[2], num(0)]
            data[key][1] = addq(data[key][1], num(1, 2))
        elif item[0] == "pow":
            key = sig(item[1])
            if key not in data:
                order.append(key)
                data[key] = [item[1], num(0)]
            exp = data[key][1]
            if is_num(exp) and is_num(item[2]):
                data[key][1] = addq(exp, item[2])
            else:
                data[key][1] = add([exp, item[2]])
        else:
            key = sig(item)
            if key not in data:
                order.append(key)
                data[key] = [item, num(0)]
            exp = data[key][1]
            if is_num(exp):
                data[key][1] = addq(exp, num(1))
            else:
                data[key][1] = add([exp, num(1)])
        i += 1
    return coeff, order, data


def display_neg(node):
    coeff, _ = split_coeff(node)
    if coeff[1] < 0:
        return True
    if node[0] == "div":
        top_coeff, _ = split_coeff(node[1])
        if top_coeff[1] < 0:
            return True
    return False


def display_abs(node):
    if node[0] == "div":
        top_coeff, top_rest = split_coeff(node[1])
        if top_coeff[1] < 0:
            top = top_rest if is_minus_one(top_coeff) else mul([num(-top_coeff[1], top_coeff[2]), top_rest])
            return div(top, node[2])
    coeff, rest = split_coeff(node)
    if coeff[1] < 0:
        return rest if is_minus_one(coeff) else mul([num(-coeff[1], coeff[2]), rest])
    return node


def _sim_uncached(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        name = node[1]
        arg = sim(node[2])
        if name == "exp":
            return power(E, arg)
        if name == "sqrt":
            if arg[0] == "pow" and same(arg[1], E):
                coeff, rest = split_num_factor(arg[2])
                if same(coeff, num(2)):
                    return power(E, rest)
            simple = sqrt_num(arg)
            if simple is not None:
                return simple
        if name == "abs" and is_num(arg):
            return num(abs(arg[1]), arg[2])
        if name == "ln":
            name = "log"
        if name == "log":
            if same(arg, E):
                return num(1)
            if arg[0] == "pow" and same(arg[1], E):
                return arg[2]
        return ("fn", name, arg)
    if kind == "pow":
        base = sim(node[1])
        exp = sim(node[2])
        if same(exp, num(1, 2)):
            return fn("sqrt", base)
        if same(exp, num(-1, 2)):
            return div(num(1), fn("sqrt", base))
        if same(base, E) and exp[0] == "fn" and exp[1] == "log":
            return exp[2]
        if is_num(exp):
            if is_zero(exp):
                return num(1)
            if is_one(exp):
                return base
            if is_num(base) and is_int_num(exp):
                return int_pow(base, exp[1])
            if base[0] == "fn" and base[1] == "sqrt" and is_int_num(exp) and exp[1] % 2 == 0:
                return power(base[2], num(exp[1] // 2))
            if base[0] == "pow" and is_num(base[2]):
                return power(base[1], mul([base[2], exp]))
        if is_one(base):
            return num(1)
        return ("pow", base, exp)
    if kind == "add":
        items = []
        raw = list(node[1])
        i = 0
        while i < len(raw):
            cur = sim(raw[i])
            if cur[0] == "add":
                items.extend(cur[1])
            else:
                items.append(cur)
            i += 1
        total = num(0)
        order = []
        data = {}
        i = 0
        while i < len(items):
            coeff, rest = split_coeff(items[i])
            if is_one(rest):
                total = addq(total, coeff)
            else:
                key = sig(rest)
                if key not in data:
                    order.append(key)
                    data[key] = [rest, num(0)]
                data[key][1] = addq(data[key][1], coeff)
            i += 1
        out = []
        i = 0
        while i < len(order):
            rest, coeff = data[order[i]]
            if not is_zero(coeff):
                if is_one(coeff):
                    out.append(rest)
                elif is_minus_one(coeff):
                    out.append(mul([num(-1), rest]))
                else:
                    out.append(mul([coeff, rest]))
            i += 1
        if not is_zero(total) or len(out) == 0:
            out.append(total)
        if len(out) == 1:
            return out[0]
        out.sort(key=sort_key)
        return ("add", tuple(out))
    if kind == "mul":
        items = []
        raw = list(node[1])
        i = 0
        while i < len(raw):
            cur = sim(raw[i])
            if cur[0] == "mul":
                items.extend(cur[1])
            else:
                items.append(cur)
            i += 1
        i = 0
        while i < len(items):
            if items[i][0] == "div":
                top = []
                bot = []
                j = 0
                while j < len(items):
                    if items[j][0] == "div":
                        top.append(items[j][1])
                        bot.append(items[j][2])
                    else:
                        top.append(items[j])
                    j += 1
                return div(make_mul(top), make_mul(bot))
            i += 1
        coeff = num(1)
        order = []
        data = {}
        i = 0
        while i < len(items):
            item = items[i]
            if is_zero(item):
                return num(0)
            if is_num(item):
                coeff = mulq(coeff, item)
            elif item[0] == "fn" and item[1] == "sqrt":
                key = sig(item[2])
                if key not in data:
                    order.append(key)
                    data[key] = [item[2], num(0)]
                data[key][1] = addq(data[key][1], num(1, 2))
            else:
                item = norm_pow_base(item)
                if item[0] == "pow":
                    base = item[1]
                    exp = item[2]
                else:
                    base = item
                    exp = num(1)
                key = sig(base)
                if key not in data:
                    order.append(key)
                    data[key] = [base, num(0)]
                prev = data[key][1]
                if is_num(prev) and is_num(exp):
                    data[key][1] = addq(prev, exp)
                else:
                    data[key][1] = add([prev, exp])
            i += 1
        out = []
        if not is_one(coeff):
            out.append(coeff)
        i = 0
        while i < len(order):
            base, exp = data[order[i]]
            if not is_zero(exp):
                if is_one(exp):
                    out.append(base)
                else:
                    out.append(power(base, exp))
            i += 1
        if len(out) == 0:
            return num(1)
        if len(out) == 1:
            return out[0]
        return ("mul", tuple(out))
    if kind == "div":
        top = sim(node[1])
        bot = sim(node[2])
        if is_zero(top):
            return num(0)
        if is_one(bot):
            return top
        if same(top, bot):
            if has_variable_dependency(top):
                return ("div", top, bot)
            return num(1)
        if top[0] == "div":
            return div(top[1], mul([top[2], bot]))
        if bot[0] == "div":
            return div(mul([top, bot[2]]), bot[1])
        if is_num(top) and is_num(bot):
            return divq(top, bot)
        top_coeff, top_order, top_data = factor_map(top)
        bot_coeff, bot_order, bot_data = factor_map(bot)
        coeff = divq(top_coeff, bot_coeff)
        i = 0
        while i < len(top_order):
            key = top_order[i]
            if key in bot_data:
                top_exp = top_data[key][1]
                bot_exp = bot_data[key][1]
                top_data[key][1] = add([top_exp, neg(bot_exp)])
                bot_data[key][1] = num(0)
            i += 1
        top_out = []
        bot_out = []
        if not is_one(coeff):
            if coeff[2] == 1:
                top_out.append(num(coeff[1]))
            else:
                if coeff[1] == -1:
                    top_out.append(num(-1, 1))
                elif coeff[1] != 1:
                    top_out.append(num(coeff[1], 1))
                bot_out.append(num(coeff[2], 1))
        i = 0
        while i < len(top_order):
            base, exp = top_data[top_order[i]]
            exp = sim(exp)
            if not is_zero(exp):
                top_out.append(base if is_one(exp) else power(base, exp))
            i += 1
        i = 0
        while i < len(bot_order):
            base, exp = bot_data[bot_order[i]]
            exp = sim(exp)
            if not is_zero(exp):
                bot_out.append(base if is_one(exp) else power(base, exp))
            i += 1
        top_expr = make_mul(top_out)
        bot_expr = make_mul(bot_out)
        if is_one(bot_expr):
            return top_expr
        return ("div", top_expr, bot_expr)
    return node


def sim(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    cached = SIM_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(SIM_CACHE, node, _sim_uncached(node), CACHE_LIMIT_LARGE)


def pr(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return 5
    if kind == "fn":
        return 4
    if kind == "pow":
        return 3
    if kind in ("mul", "div"):
        return 2
    return 1


def needs_div_brackets(node):
    if node[0] == "add":
        return True
    if node[0] == "div":
        return True
    if node[0] == "mul" and len(node[1]) > 1:
        return True
    return pr(node) < pr(("div", num(1), num(1)))


def _sort_key_uncached(node):
    order = {
        "num": 0,
        "sym": 1,
        "const": 2,
        "fn": 3,
        "pow": 4,
        "div": 5,
        "mul": 6,
        "add": 7,
    }
    return str(order.get(node[0], 9)) + ":" + show(node)


def sort_key(node):
    cached = SORT_KEY_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(SORT_KEY_CACHE, node, _sort_key_uncached(node), CACHE_LIMIT_MEDIUM)


def _show_uncached(node, parent=0):
    kind = node[0]
    if kind == "num":
        if node[2] == 1:
            text = str(node[1])
        else:
            text = str(node[1]) + "/" + str(node[2])
    elif kind == "sym":
        text = node[1]
    elif kind == "const":
        text = node[1]
    elif kind == "fn":
        if node[1] == "log":
            text = "ln(" + _show(node[2], 0) + ")"
        else:
            text = node[1] + "(" + _show(node[2], 0) + ")"
    elif kind == "pow":
        if same(node[2], num(1, 2)):
            text = "sqrt(" + _show(node[1], 0) + ")"
        elif same(node[2], num(-1, 2)):
            text = "1/sqrt(" + _show(node[1], 0) + ")"
        else:
            left = _show(node[1], pr(node))
            right = _show(node[2], pr(node))
            if pr(node[1]) < pr(node) or node[1][0] in ("add", "div"):
                left = "(" + _show(node[1], 0) + ")"
            if pr(node[2]) < pr(node) or (is_num(node[2]) and node[2][2] != 1):
                right = "(" + _show(node[2], 0) + ")"
            text = left + "^" + right
    elif kind == "mul":
        items = list(node[1])
        if items and is_minus_one(items[0]):
            rest = make_mul(items[1:])
            inner = _show(rest, pr(node))
            if pr(rest) < pr(node):
                inner = "(" + _show(rest, 0) + ")"
            text = "-" + inner
        elif items and is_num(items[0]) and items[0][2] != 1 and len(items) > 1:
            # Show fractional coefficients as a single numerator over the
            # denominator, e.g. (3*sin(x))/2 instead of 3/2*sin(x).
            coeff = items[0]
            top_items = []
            if abs(coeff[1]) != 1:
                top_items.append(num(abs(coeff[1]), 1))
            i = 1
            while i < len(items):
                top_items.append(items[i])
                i += 1
            top = make_mul(top_items)
            top_text = _show(top, 0)
            if coeff[1] < 0:
                text = "-(" + top_text + ")/" + str(coeff[2])
            else:
                text = "(" + top_text + ")/" + str(coeff[2])
        else:
            out = []
            i = 0
            while i < len(items):
                item = items[i]
                chunk = _show(item, pr(node))
                if pr(item) < pr(node) or item[0] == "add":
                    chunk = "(" + _show(item, 0) + ")"
                out.append(chunk)
                i += 1
            text = "*".join(out)
    elif kind == "div":
        left = _show(node[1], pr(node))
        right = _show(node[2], pr(node))
        if needs_div_brackets(node[1]):
            left = "(" + _show(node[1], 0) + ")"
        if needs_div_brackets(node[2]) or pr(node[2]) <= pr(node):
            right = "(" + _show(node[2], 0) + ")"
        text = left + "/" + right
    else:
        bits = list(flat(node, "add"))
        text = ""
        i = 0
        while i < len(bits):
            item = bits[i]
            if i == 0:
                head = display_abs(item) if display_neg(item) else item
                text = _show(head, pr(node))
                if display_neg(item):
                    text = "-" + text
            else:
                if display_neg(item):
                    chunk = _show(display_abs(item), pr(node))
                    if display_abs(item)[0] == "add":
                        chunk = "(" + _show(display_abs(item), 0) + ")"
                    text += "-" + chunk
                else:
                    text += "+" + _show(item, pr(node))
            i += 1
    if pr(node) < parent:
        return "(" + text + ")"
    return text


def _show(node, parent=0):
    key = (node, parent)
    cached = SHOW_CACHE.get(key)
    if cached is not None:
        return cached
    return cache_store(SHOW_CACHE, key, _show_uncached(node, parent), CACHE_LIMIT_LARGE)


def show(node, parent=0):
    return _show(sim(node), parent)


def is_digit_char(ch):
    return "0" <= ch <= "9"


def is_alpha_char(ch):
    return ("A" <= ch <= "Z") or ("a" <= ch <= "z")


def is_name_start(ch):
    return is_alpha_char(ch) or ch == "_"


def is_name_char(ch):
    return is_name_start(ch) or is_digit_char(ch)


def is_num_token_start(text, i):
    ch = text[i]
    return is_digit_char(ch) or (ch == "." and i + 1 < len(text) and is_digit_char(text[i + 1]))


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------
def parse(text):
    # Keep tokenization deliberately small and calculator-friendly:
    # no imports, no eval, and accept both ** and ^ for powers.
    toks = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in " \t\r\n":
            i += 1
        elif text[i : i + 2] == "**":
            toks.append("**")
            i += 2
        elif ch == "^":
            toks.append("**")
            i += 1
        elif ch in "()+-*/=,":
            toks.append(ch)
            i += 1
        elif is_num_token_start(text, i):
            j = i
            seen_dot = 0
            while j < len(text) and (is_digit_char(text[j]) or text[j] == "."):
                if text[j] == ".":
                    seen_dot += 1
                j += 1
            if seen_dot > 1:
                raise ValueError("Bad number.")
            toks.append(text[i:j])
            i = j
        elif is_name_start(ch):
            j = i + 1
            while j < len(text) and is_name_char(text[j]):
                j += 1
            toks.append(text[i:j])
            i = j
        else:
            raise ValueError("Unexpected character: " + ch)

    p = 0

    def cur():
        if p >= len(toks):
            return None
        return toks[p]

    def eat(tok=None):
        nonlocal p
        here = cur()
        if tok is not None and here != tok:
            raise ValueError("Expected " + repr(tok) + " but found " + repr(here) + ".")
        p += 1
        return here

    def starts_implicit(tok):
        if tok is None:
            return False
        if tok == "(":
            return True
        if tok in ("+", "-", "*", "/", "**", ")", ",", "="):
            return False
        return True

    def atom():
        tok = cur()
        if tok == "(":
            eat("(")
            out = expr()
            eat(")")
            return out
        if tok is None:
            raise ValueError("Unexpected end.")
        if tok not in ("+", "*", "/", "**", ")", ",", "="):
            eat()
            if tok[0].isdigit() or tok[0] == ".":
                return num_text(tok)
            low = tok.lower()
            if low == "ln":
                low = "log"
            if low == "pi":
                return PI
            if low == "e":
                return E
            if low in FUNC_ALIASES:
                low = FUNC_ALIASES[low]
            if low in FUNC_NAMES:
                if cur() == "**":
                    eat("**")
                    exp = power_part()
                    if cur() == "(":
                        eat("(")
                        out = expr()
                        eat(")")
                    elif starts_implicit(cur()):
                        out = atom()
                    else:
                        raise ValueError("Bad function power form.")
                    return power(fn(low, out), exp)
                if cur() == "(":
                    eat("(")
                    if low == "log" and cur() != ")":
                        arg1 = expr()
                        if cur() == ",":
                            eat(",")
                            arg2 = expr()
                            eat(")")
                            return div(fn("log", arg2), fn("log", arg1))
                        else:
                            eat(")")
                            return fn(low, arg1)
                    else:
                        out = expr()
                        eat(")")
                        return fn(low, out)
                if starts_implicit(cur()):
                    return fn(low, atom())
            return sym(tok)
        raise ValueError("Bad token: " + repr(tok))

    def power_part():
        # Power is right-associative, so parse a**b**c as a**(b**c).
        left = atom()
        if cur() == "**":
            eat("**")
            return power(left, power_part())
        return left

    def unary():
        if cur() == "-":
            eat("-")
            return neg(unary())
        return power_part()

    def term():
        out = unary()
        while True:
            tok = cur()
            if tok == "*":
                eat("*")
                out = mul([out, unary()])
            elif tok == "/":
                eat("/")
                out = div(out, unary())
            elif starts_implicit(tok):
                out = mul([out, unary()])
            else:
                return out

    def expr():
        out = term()
        while cur() in ("+", "-"):
            if cur() == "+":
                eat("+")
                out = add([out, term()])
            else:
                eat("-")
                out = add([out, neg(term())])
        return out

    out = expr()
    if p != len(toks):
        raise ValueError("Unexpected token: " + repr(cur()))
    return sim(out)


def split_top_level(text, ch):
    depth = 0
    i = 0
    while i < len(text):
        cur = text[i]
        if cur == "(":
            depth += 1
        elif cur == ")":
            depth -= 1
        elif cur == ch and depth == 0:
            return text[:i], text[i + 1 :]
        i += 1
    return None


def split_top_level_all(text, ch):
    out = []
    depth = 0
    start = 0
    i = 0
    while i < len(text):
        cur = text[i]
        if cur == "(":
            depth += 1
        elif cur == ")":
            depth -= 1
        elif cur == ch and depth == 0:
            out.append(text[start:i].strip())
            start = i + 1
        i += 1
    out.append(text[start:].strip())
    return out


def parse_identity(text):
    parts = split_top_level(text, "=")
    if parts is None:
        raise ValueError("Identity input must contain '='.")
    left = parse(parts[0].strip())
    right = parse(parts[1].strip())
    return left, right


def split_rewrite_arrow(text):
    depth = 0
    i = 0
    while i < len(text) - 1:
        cur = text[i]
        if cur == "(":
            depth += 1
        elif cur == ")":
            depth -= 1
        elif depth == 0 and text[i : i + 2] in ("->", "=>"):
            return text[:i].strip(), text[i + 2 :].strip()
        i += 1
    return None


def exact_num_value(node):
    if is_num(node):
        return node
    return None


def maybe_scalar_times_var(node, var):
    node = sim(node)
    if node == sym(var):
        return num(1)
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        if len(items) == 2 and is_num(items[0]) and items[1] == sym(var):
            return items[0]
    if node[0] == "div" and node[1] == sym(var) and is_num(node[2]):
        return divq(num(1), node[2])
    return None


def split_sum(node):
    node = sim(node)
    if node[0] != "add":
        return None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None
    second = bits[1]
    coeff, rest = split_coeff(second)
    if coeff[1] < 0:
        return bits[0], display_abs(second), -1
    return bits[0], bits[1], 1


def divide_terms_by_two_for_display(node):
    node = sim(node)
    if node[0] != "add":
        return expand_negative_add_terms(sim(div(node, num(2))))
    terms = list(flat(node, "add"))
    halved = []
    i = 0
    while i < len(terms):
        coeff, rest = split_coeff(terms[i])
        half_coeff = sim(mul([coeff, num(1, 2)]))
        halved.append(mul([half_coeff, rest]))
        i += 1
    return expand_negative_add_terms(sim(add(halved)))


def half_sum_diff_args(arg1, arg2):
    left = split_sum(arg1)
    right = split_sum(arg2)
    if left is not None and right is not None:
        pair = match_plus_minus_pair(arg1, arg2)
        if pair is not None:
            if left[2] > 0 and right[2] < 0:
                return pair[0], pair[1]
            if left[2] < 0 and right[2] > 0:
                return pair[0], neg(pair[1])
    half_sum = divide_terms_by_two_for_display(add([arg1, arg2]))
    half_diff = divide_terms_by_two_for_display(add([arg1, neg(arg2)]))
    return half_sum, half_diff


def signed_unit_term(node):
    coeff, rest = split_coeff(node)
    if same(coeff, num(1)) or same(coeff, num(-1)):
        return coeff, rest
    return None


def extract_binomial(node):
    node = sim(node)
    if node[0] != "add":
        return None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_minus_one(coeff1) and is_one(coeff2):
        coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
    if not is_one(coeff1):
        return None
    if not (is_one(coeff2) or is_minus_one(coeff2)):
        return None
    return rest1, rest2, 1 if is_one(coeff2) else -1


def trig_pow_arg(node, name, exp):
    node = sim(node)
    if node[0] == "pow" and same(node[2], num(exp)) and node[1][0] == "fn" and node[1][1] == name:
        return node[1][2]
    return None


def sec_cosec_pair_value(left, right):
    left = sim(left)
    right = sim(right)
    if left[0] != "fn" or right[0] != "fn":
        return None, None
    if not same(left[2], right[2]):
        return None, None
    pair = (left[1], right[1])
    if pair == ("sec", "tan"):
        return num(1), "Use sec^2 A - tan^2 A = 1."
    if pair == ("cosec", "cot"):
        return num(1), "Use cosec^2 A - cot^2 A = 1."
    return None, None


def trig_name(node):
    if node[0] == "fn" and node[1] in ("sin", "cos", "tan", "sec", "cosec", "cot"):
        return node[1]
    return None


def reciprocal_trig(node):
    node = sim(node)
    if node[0] == "fn":
        name = node[1]
        arg = reciprocal_trig(node[2])
        if name == "sec":
            return div(num(1), fn("cos", arg))
        if name == "cosec":
            return div(num(1), fn("sin", arg))
        if name == "cot":
            return div(fn("cos", arg), fn("sin", arg))
        if name == "tan":
            return div(fn("sin", arg), fn("cos", arg))
        return fn(name, arg)
    if node[0] == "pow":
        return power(reciprocal_trig(node[1]), reciprocal_trig(node[2]))
    if node[0] == "div":
        return div(reciprocal_trig(node[1]), reciprocal_trig(node[2]))
    if node[0] in ("add", "mul"):
        out = []
        i = 0
        while i < len(node[1]):
            out.append(reciprocal_trig(node[1][i]))
            i += 1
        return sim((node[0], tuple(out)))
    return node


def named_reciprocal_trig(node):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return fn(node[1], named_reciprocal_trig(node[2]))
    if kind == "pow":
        return power(named_reciprocal_trig(node[1]), named_reciprocal_trig(node[2]))
    if kind == "div":
        top = named_reciprocal_trig(node[1])
        bot = named_reciprocal_trig(node[2])
        top_coeff, top_rest = split_coeff(top)
        bot_coeff, bot_rest = split_coeff(bot)
        coeff = div(top_coeff, bot_coeff)
        out = None
        if is_one(top_rest):
            if bot_rest[0] == "fn":
                if bot_rest[1] == "cos":
                    out = fn("sec", bot_rest[2])
                elif bot_rest[1] == "sin":
                    out = fn("cosec", bot_rest[2])
                elif bot_rest[1] == "tan":
                    out = fn("cot", bot_rest[2])
            elif bot_rest[0] == "pow" and is_int_num(bot_rest[2]) and bot_rest[2][1] > 0 and bot_rest[1][0] == "fn":
                if bot_rest[1][1] == "cos":
                    out = power(fn("sec", bot_rest[1][2]), bot_rest[2])
                elif bot_rest[1][1] == "sin":
                    out = power(fn("cosec", bot_rest[1][2]), bot_rest[2])
                elif bot_rest[1][1] == "tan":
                    out = power(fn("cot", bot_rest[1][2]), bot_rest[2])
        if out is None and top_rest[0] == "fn" and bot_rest[0] == "fn" and same(top_rest[2], bot_rest[2]):
            if top_rest[1] == "sin" and bot_rest[1] == "cos":
                out = fn("tan", top_rest[2])
            elif top_rest[1] == "cos" and bot_rest[1] == "sin":
                out = fn("cot", top_rest[2])
        if out is None and top_rest[0] == "pow" and bot_rest[0] == "pow" and is_int_num(top_rest[2]) and is_int_num(bot_rest[2]) and same(top_rest[2], bot_rest[2]):
            if top_rest[1][0] == "fn" and bot_rest[1][0] == "fn" and same(top_rest[1][2], bot_rest[1][2]):
                if top_rest[1][1] == "sin" and bot_rest[1][1] == "cos":
                    out = power(fn("tan", top_rest[1][2]), top_rest[2])
                elif top_rest[1][1] == "cos" and bot_rest[1][1] == "sin":
                    out = power(fn("cot", top_rest[1][2]), top_rest[2])
        if out is not None:
            if is_one(coeff):
                return out
            return mul([coeff, out])
        return div(top, bot)
    out = []
    i = 0
    while i < len(node[1]):
        out.append(named_reciprocal_trig(node[1][i]))
        i += 1
    return sim((kind, tuple(out)))


def trig_formula_expand(node):
    node = sim(node)
    if node[0] != "fn":
        return None, None
    name = node[1]
    arg = node[2]
    pair = split_sum(arg)
    if pair is not None:
        left, right, sign = pair
        if name == "sin":
            if sign > 0:
                return add([mul([fn("sin", left), fn("cos", right)]), mul([fn("cos", left), fn("sin", right)])]), FORMULA_SIN_ADD
            return add([mul([fn("sin", left), fn("cos", right)]), neg(mul([fn("cos", left), fn("sin", right)]))]), FORMULA_SIN_SUB
        if name == "cos":
            if sign > 0:
                return add([mul([fn("cos", left), fn("cos", right)]), neg(mul([fn("sin", left), fn("sin", right)]))]), FORMULA_COS_ADD
            return add([mul([fn("cos", left), fn("cos", right)]), mul([fn("sin", left), fn("sin", right)])]), FORMULA_COS_SUB
        if name == "tan":
            if sign > 0:
                return div(add([fn("tan", left), fn("tan", right)]), add([num(1), neg(mul([fn("tan", left), fn("tan", right)]))])), FORMULA_TAN_ADD
            return div(add([fn("tan", left), neg(fn("tan", right))]), add([num(1), mul([fn("tan", left), fn("tan", right)])])), FORMULA_TAN_SUB
    coeff = None
    base = None
    if arg[0] == "mul":
        items = list(flat(arg, "mul"))
        if len(items) == 2 and is_num(items[0]):
            coeff = items[0]
            base = items[1]
    elif arg[0] == "div" and is_num(arg[2]):
        if arg[1][0] == "mul":
            pass
    if coeff is not None and is_int_num(coeff):
        if coeff[1] == 2:
            if name == "sin":
                return mul([num(2), fn("sin", base), fn("cos", base)]), "Use sin(2A) = 2sin A cos A."
            if name == "cos":
                return add([power(fn("cos", base), num(2)), neg(power(fn("sin", base), num(2)))]), "Use cos(2A) = cos^2 A - sin^2 A."
            if name == "tan":
                return div(mul([num(2), fn("tan", base)]), add([num(1), neg(power(fn("tan", base), num(2)))])), "Use tan(2A) = 2tan A/(1 - tan^2 A)."
        if coeff[1] == 3:
            if name == "sin":
                return add([mul([num(3), fn("sin", base)]), neg(mul([num(4), power(fn("sin", base), num(3))]))]), "Use sin(3A) = 3sin A - 4sin^3 A."
            if name == "cos":
                return add([mul([num(4), power(fn("cos", base), num(3))]), neg(mul([num(3), fn("cos", base)]))]), "Use cos(3A) = 4cos^3 A - 3cos A."
            if name == "tan":
                return div(add([mul([num(3), fn("tan", base)]), neg(power(fn("tan", base), num(3)))]), add([num(1), neg(mul([num(3), power(fn("tan", base), num(2))]))])), "Use tan(3A) = (3tan A - tan^3 A)/(1 - 3tan^2 A)."
        if coeff[1] == 4:
            if name == "sin":
                return mul([num(2), fn("sin", mul([num(2), base])), fn("cos", mul([num(2), base]))]), "Use sin(4A) = 2sin(2A)cos(2A)."
            if name == "cos":
                return add([power(fn("cos", mul([num(2), base])), num(2)), neg(power(fn("sin", mul([num(2), base])), num(2)))]), "Use cos(4A) = cos^2(2A) - sin^2(2A)."
            if name == "tan":
                tan2 = fn("tan", mul([num(2), base]))
                return div(mul([num(2), tan2]), add([num(1), neg(power(tan2, num(2)))])), "Use tan(4A) = 2tan(2A)/(1 - tan^2(2A))."
    return None, None


def sum_product_expand(node):
    node = sim(node)
    match = split_sum(node)
    if match is not None:
        left, right, sign = match
        if left[0] == "fn" and right[0] == "fn" and left[1] == right[1] and left[1] in ("sin", "cos"):
            half_sum, half_diff = half_sum_diff_args(left[2], right[2])
            if left[1] == "sin" and sign > 0:
                return mul([num(2), fn("sin", half_sum), fn("cos", half_diff)]), FORMULA_SIN_PLUS_SIN
            if left[1] == "sin" and sign < 0:
                return mul([num(2), fn("cos", half_sum), fn("sin", half_diff)]), FORMULA_SIN_MINUS_SIN
            if left[1] == "cos" and sign > 0:
                return mul([num(2), fn("cos", half_sum), fn("cos", half_diff)]), FORMULA_COS_PLUS_COS
            if left[1] == "cos" and sign < 0:
                return neg(mul([num(2), fn("sin", half_sum), fn("sin", half_diff)])), FORMULA_COS_MINUS_COS
    if node[0] != "mul":
        return None, None
    items = list(flat(node, "mul"))
    coeff = num(1)
    trig_bits = []
    i = 0
    while i < len(items):
        if is_num(items[i]):
            coeff = mulq(coeff, items[i])
        elif items[i][0] == "fn" and items[i][1] in ("sin", "cos"):
            trig_bits.append(items[i])
        else:
            return None, None
        i += 1
    if len(trig_bits) != 2 or not (same(coeff, num(2)) or same(coeff, num(-2))):
        return None, None
    a = trig_bits[0]
    b = trig_bits[1]
    out = None
    note = None
    names = (a[1], b[1])
    if names == ("sin", "cos") or names == ("cos", "sin"):
        sin_arg = a[2] if a[1] == "sin" else b[2]
        cos_arg = a[2] if a[1] == "cos" else b[2]
        out = add([fn("sin", add([sin_arg, cos_arg])), fn("sin", add([sin_arg, neg(cos_arg)]))])
        note = "Use 2sin A cos B = sin(A+B) + sin(A-B)."
    elif names == ("cos", "cos"):
        out = add([fn("cos", add([a[2], b[2]])), fn("cos", add([a[2], neg(b[2])]))])
        note = "Use 2cos A cos B = cos(A+B) + cos(A-B)."
    elif names == ("sin", "sin"):
        out = add([fn("cos", add([a[2], neg(b[2])])), neg(fn("cos", add([a[2], b[2]])))])
        note = "Use 2sin A sin B = cos(A-B) - cos(A+B)."
    if out is None:
        return None, None
    if same(coeff, num(-2)):
        out = neg(out)
    return sim(out), note


def half_angle_expand(node):
    node = sim(node)
    if node[0] != "pow" or not same(node[2], num(2)):
        return None, None
    if node[1][0] != "fn":
        return None, None
    name = node[1][1]
    arg = node[1][2]
    if name not in ("sin", "cos"):
        return None, None
    if arg[0] == "div" and is_num(arg[2]) and is_int_num(arg[2]) and arg[2][1] == 2:
        doubled = arg[1]
        if name == "sin":
            return div(add([num(1), neg(fn("cos", doubled))]), num(2)), "Use sin^2(A/2) = (1-cos A)/2."
        return div(add([num(1), fn("cos", doubled)]), num(2)), "Use cos^2(A/2) = (1+cos A)/2."
    return None, None


def power_reduction_once(node):
    node = sim(node)
    if node[0] == "pow" and same(node[2], num(2)) and node[1][0] == "fn":
        name = node[1][1]
        arg = node[1][2]
        if name == "sin":
            return div(add([num(1), neg(fn("cos", mul([num(2), arg])))]), num(2)), "Use sin^2 A = (1-cos 2A)/2."
        if name == "cos":
            return div(add([num(1), fn("cos", mul([num(2), arg]))]), num(2)), "Use cos^2 A = (1+cos 2A)/2."
        if name == "tan":
            return add([power(fn("sec", arg), num(2)), num(-1)]), "Use tan^2 A = sec^2 A - 1."
        if name == "cot":
            return add([power(fn("cosec", arg), num(2)), num(-1)]), "Use cot^2 A = cosec^2 A - 1."
    if node[0] == "pow" and same(node[2], num(4)) and node[1][0] == "fn":
        name = node[1][1]
        arg = node[1][2]
        if name in ("sin", "cos"):
            inner = power_reduction_once(power(fn(name, arg), num(2)))[0]
            if inner is not None:
                return power(inner, num(2)), "Rewrite " + name + "^4 A as (" + name + "^2 A)^2."
    return None, None


def special_trig_identity_once(node):
    node = sim(node)
    if node[0] != "add":
        return None, None
    items = list(flat(node, "add"))
    if len(items) != 2:
        return None, None
    first = signed_unit_term(items[0])
    second = signed_unit_term(items[1])
    if first is None or second is None:
        return None, None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_minus_one(coeff1) and is_one(coeff2):
        coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
    if is_one(rest1) and is_one(coeff1):
        if trig_pow_arg(rest2, "tan", 2) is not None and is_one(coeff2):
            return power(fn("sec", rest2[1][2]), num(2)), "Use 1 + tan^2 A = sec^2 A."
        if trig_pow_arg(rest2, "cot", 2) is not None and is_one(coeff2):
            return power(fn("cosec", rest2[1][2]), num(2)), "Use 1 + cot^2 A = cosec^2 A."
    if is_one(rest2) and is_one(coeff2):
        if trig_pow_arg(rest1, "tan", 2) is not None and is_one(coeff1):
            return power(fn("sec", rest1[1][2]), num(2)), "Use 1 + tan^2 A = sec^2 A."
        if trig_pow_arg(rest1, "cot", 2) is not None and is_one(coeff1):
            return power(fn("cosec", rest1[1][2]), num(2)), "Use 1 + cot^2 A = cosec^2 A."
    if is_one(coeff1) and is_minus_one(coeff2):
        arg1 = trig_pow_arg(rest1, "sec", 2)
        arg2 = trig_pow_arg(rest2, "tan", 2)
        if arg1 is not None and arg2 is not None and same(arg1, arg2):
            return num(1), "Use sec^2 A - tan^2 A = 1."
        arg1 = trig_pow_arg(rest1, "cosec", 2)
        arg2 = trig_pow_arg(rest2, "cot", 2)
        if arg1 is not None and arg2 is not None and same(arg1, arg2):
            return num(1), "Use cosec^2 A - cot^2 A = 1."
        arg1 = trig_pow_arg(rest1, "sec", 2)
        if arg1 is not None and is_one(rest2):
            return power(fn("tan", arg1), num(2)), "Use sec^2 A - 1 = tan^2 A."
        arg1 = trig_pow_arg(rest1, "cosec", 2)
        if arg1 is not None and is_one(rest2):
            return power(fn("cot", arg1), num(2)), "Use cosec^2 A - 1 = cot^2 A."
    return None, None


def factor_difference_once(node):
    node = sim(node)
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        if len(items) == 2:
            left = extract_binomial(items[0])
            right = extract_binomial(items[1])
            if left is not None and right is not None and same(left[0], right[0]) and same(left[1], right[1]) and left[2] == -right[2]:
                return add([power(left[0], num(2)), neg(power(left[1], num(2)))]), "Use (A-B)(A+B) = A^2 - B^2."
    if node[0] != "add":
        return None, None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None, None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None, None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_minus_one(coeff1) and is_one(coeff2):
        coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
    if not (is_one(coeff1) and is_minus_one(coeff2)):
        return None, None
    if rest1[0] == "pow" and rest2[0] == "pow" and same(rest1[2], num(4)) and same(rest2[2], num(4)):
        return mul([add([power(rest1[1], num(2)), neg(power(rest2[1], num(2)))]), add([power(rest1[1], num(2)), power(rest2[1], num(2))])]), "Use A^4 - B^4 = (A^2-B^2)(A^2+B^2)."
    return None, None


def raw_step_line(node):
    return "= " + _show(node, 0)


def proof_identity_rewrite_once(node):
    node = sim(node)
    for rewriter in (direct_double_angle_rewrite, trig_formula_expand, half_angle_expand):
        rewritten, note = rewriter(node)
        if rewritten is not None:
            return sim(rewritten), note
    if node[0] == "fn":
        arg = node[2]
        if node[1] == "sec":
            return div(num(1), fn("cos", arg)), "Use sec A = 1/cos A."
        if node[1] == "cosec":
            return div(num(1), fn("sin", arg)), "Use cosec A = 1/sin A."
        if node[1] == "tan":
            return div(fn("sin", arg), fn("cos", arg)), "Use tan A = sin A / cos A."
        if node[1] == "cot":
            return div(fn("cos", arg), fn("sin", arg)), "Use cot A = cos A / sin A."
    if node[0] == "pow" and node[1][0] == "fn" and same(node[2], num(2)):
        name = node[1][1]
        arg = node[1][2]
        if name == "sec":
            return div(num(1), power(fn("cos", arg), num(2))), "Use sec^2 A = 1/cos^2 A."
        if name == "cosec":
            return div(num(1), power(fn("sin", arg), num(2))), "Use cosec^2 A = 1/sin^2 A."
        if name == "tan":
            return div(power(fn("sin", arg), num(2)), power(fn("cos", arg), num(2))), "Use tan^2 A = sin^2 A / cos^2 A."
        if name == "cot":
            return div(power(fn("cos", arg), num(2)), power(fn("sin", arg), num(2))), "Use cot^2 A = cos^2 A / sin^2 A."
    rewritten, note = power_reduction_once(node)
    if rewritten is not None:
        return sim(rewritten), note
    return None, None


def rewrite_one_term_in_sum_for_display(node):
    node = sim(node)
    if node[0] != "add":
        return None, None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None, None
    i = 0
    while i < 2:
        term = bits[i]
        coeff, rest = split_coeff(term)
        if same(coeff, num(1)) or same(coeff, num(-1)):
            rewritten, note = proof_identity_rewrite_once(rest)
            if rewritten is not None:
                new_term = rewritten if same(coeff, num(1)) else neg(rewritten)
                out = [bits[0], bits[1]]
                out[i] = new_term
                return ("add", tuple(out)), note
        i += 1
    return None, None


def proof_forward_identity_rewrite_once(node):
    node = sim(node)
    if node[0] == "fn":
        rewritten, note = trig_formula_expand(node)
        if rewritten is not None:
            return sim(rewritten), note
        return None, None
    if node[0] != "add":
        return None, None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None, None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None, None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_minus_one(coeff1) and is_one(coeff2):
        coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
    if is_one(coeff1) and is_one(rest1) and rest2[0] == "fn" and rest2[1] == "cos" and match_numeric_multiple_arg(rest2[2], 2) is not None:
        rewritten, note = direct_double_angle_rewrite(node)
        if rewritten is not None:
            return sim(rewritten), note
    return None, None


def rewrite_sum_terms_for_display(node):
    node = sim(node)
    if node[0] != "add":
        rewritten, note = proof_forward_identity_rewrite_once(node)
        return rewritten, note
    terms = list(flat(node, "add"))
    notes = []
    changed = False
    progress = True
    while progress:
        progress = False
        i = 0
        while i < len(terms):
            j = i + 1
            while j < len(terms):
                pair = sim(("add", (terms[i], terms[j])))
                rewritten, note = proof_forward_identity_rewrite_once(pair)
                if rewritten is not None:
                    terms = terms[:i] + [rewritten] + terms[i + 1 : j] + terms[j + 1 :]
                    changed = True
                    progress = True
                    if note is not None and note not in notes:
                        notes.append(note)
                    break
                j += 1
            if progress:
                break
            i += 1
        if progress:
            continue
        i = 0
        while i < len(terms):
            coeff, rest = split_coeff(terms[i])
            rewritten, note = proof_forward_identity_rewrite_once(rest)
            if rewritten is not None:
                terms[i] = sim(mul([coeff, rewritten]))
                changed = True
                progress = True
                if note is not None and note not in notes:
                    notes.append(note)
                break
            i += 1
    if not changed:
        return None, None
    if len(terms) == 1:
        rebuilt = sim(terms[0])
    else:
        rebuilt = sim(("add", tuple(terms)))
    if len(notes) == 1:
        return rebuilt, notes[0]
    cleaned = []
    i = 0
    while i < len(notes):
        note = notes[i]
        if note.startswith("Use "):
            note = note[4:]
        if note.endswith("."):
            note = note[:-1]
        cleaned.append(note)
        i += 1
    return rebuilt, "Use " + "; ".join(cleaned) + "."


def factor_difference_of_squares_for_display(node):
    node = sim(node)
    if node[0] != "add":
        return None, None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None, None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None, None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_minus_one(coeff1) and is_one(coeff2):
        coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
    if not (is_one(coeff1) and is_minus_one(coeff2)):
        return None, None
    if rest1[0] == "pow" and rest2[0] == "pow" and same(rest1[2], num(2)) and same(rest2[2], num(2)):
        a = rest1[1]
        b = rest2[1]
        return ("mul", (("add", (a, b)), ("add", (a, neg(b))))), "Use A^2 - B^2 = (A+B)(A-B)."
    return None, None


def split_fraction_term(node):
    node = sim(node)
    if node[0] == "div":
        return node[1], node[2]
    return node, num(1)


def common_denominator_step(node):
    pair = split_sum(node)
    if pair is None:
        return None
    left, right, sign = pair
    left_num, left_den = split_fraction_term(left)
    right_num, right_den = split_fraction_term(right)
    if is_one(left_den) and is_one(right_den):
        return None
    if cheap_same(left_den, right_den) and not is_one(left_den):
        if sign > 0:
            raw_num = ("add", (left_num, right_num))
        else:
            raw_num = ("add", (left_num, neg(right_num)))
        raw = ("div", raw_num, left_den)
        return sim(raw), raw, "Factor out the common denominator."
    left_piece = make_display_mul([left_num, right_den])
    right_piece = make_display_mul([right_num, left_den])
    if sign > 0:
        raw_num = ("add", (left_piece, right_piece))
    else:
        raw_num = ("add", (left_piece, neg(right_piece)))
    raw_den = make_display_mul([left_den, right_den])
    raw = ("div", raw_num, raw_den)
    return sim(raw), raw, "Use a common denominator."


def match_trig_product_pair(node):
    node = sim(node)
    coeff, rest = split_coeff(node)
    if not is_one(coeff):
        return None
    items = list(flat(rest, "mul")) if rest[0] == "mul" else [rest]
    if len(items) != 2:
        return None
    if items[0][0] != "fn" or items[1][0] != "fn":
        return None
    if items[0][1] not in ("sin", "cos") or items[1][1] not in ("sin", "cos"):
        return None
    return items


def trig_formula_contract_once(node):
    node = sim(node)
    if node[0] != "add":
        return None, None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None, None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None, None
    sign1, term1 = first
    sign2, term2 = second
    left_bits = match_trig_product_pair(term1)
    right_bits = match_trig_product_pair(term2)
    if left_bits is None or right_bits is None:
        return None, None

    left_map = {left_bits[0][1]: left_bits[0][2], left_bits[1][1]: left_bits[1][2]}
    right_map = {right_bits[0][1]: right_bits[0][2], right_bits[1][1]: right_bits[1][2]}

    if is_minus_one(sign1) and is_one(sign2):
        sign1, sign2 = sign2, sign1
        left_map, right_map = right_map, left_map

    if is_one(sign1) and "sin" in left_map and "cos" in left_map and "sin" in right_map and "cos" in right_map:
        if cheap_same(left_map["sin"], right_map["cos"]) and cheap_same(left_map["cos"], right_map["sin"]):
            if is_one(sign2):
                return fn("sin", add([left_map["sin"], left_map["cos"]])), FORMULA_SIN_ADD
            return fn("sin", add([left_map["sin"], neg(left_map["cos"])])), FORMULA_SIN_SUB

    if left_bits[0][1] == "cos" and left_bits[1][1] == "cos" and is_one(sign1):
        left_a = left_bits[0][2]
        left_b = left_bits[1][2]
        if right_bits[0][1] == "sin" and right_bits[1][1] == "sin":
            right_a = right_bits[0][2]
            right_b = right_bits[1][2]
            if (cheap_same(left_a, right_a) and cheap_same(left_b, right_b)) or (cheap_same(left_a, right_b) and cheap_same(left_b, right_a)):
                if is_one(sign2):
                    return fn("cos", add([left_a, neg(left_b)])), FORMULA_COS_SUB
                return fn("cos", add([left_a, left_b])), FORMULA_COS_ADD
    return None, None


def cancel_fraction_common_factor_for_display(node):
    if node[0] != "div":
        return None
    reduced = sim(node)
    if cheap_same(reduced, node):
        return None
    return reduced


def rewrite_fraction_denominator_double_angle(node):
    node = sim(node)
    if node[0] != "div":
        return None, None
    product = match_same_angle_sin_cos_product(node[2])
    if product is None or not same(product[0], num(1)):
        return None, None
    raw = ("div", (("mul", (num(2), node[1]))), fn("sin", mul([num(2), product[1]])))
    return sim(raw), raw


def proof_named_target_rewrite(node, target):
    node = sim(node)
    target = sim(target)
    named = named_reciprocal_trig(node)
    if not cheap_same(named, node) and equivalent(named, target):
        if target[0] == "fn" and target[1] == "tan":
            return named, "Rewrite sin(A)/cos(A) as tan(A)."
        if target[0] == "fn" and target[1] == "cot":
            return named, "Rewrite cos(A)/sin(A) as cot(A)."
    if node[0] == "div" and node[2][0] == "fn":
        arg = node[2][2]
        if node[2][1] == "sin":
            candidate = sim(mul([node[1], fn("cosec", arg)]))
            if equivalent(candidate, target):
                return candidate, "Rewrite 1/sin(A) as cosec(A)."
        if node[2][1] == "cos":
            candidate = sim(mul([node[1], fn("sec", arg)]))
            if equivalent(candidate, target):
                return candidate, "Rewrite 1/cos(A) as sec(A)."
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        coeff_bits = []
        sin_arg = None
        cos_arg = None
        sec_arg = None
        cosec_arg = None
        i = 0
        while i < len(items):
            item = items[i]
            if item[0] == "fn" and item[1] == "sin":
                if sin_arg is not None:
                    sin_arg = False
                    break
                sin_arg = item[2]
            elif item[0] == "fn" and item[1] == "cos":
                if cos_arg is not None:
                    cos_arg = False
                    break
                cos_arg = item[2]
            elif item[0] == "fn" and item[1] == "sec":
                if sec_arg is not None:
                    sec_arg = False
                    break
                sec_arg = item[2]
            elif item[0] == "fn" and item[1] == "cosec":
                if cosec_arg is not None:
                    cosec_arg = False
                    break
                cosec_arg = item[2]
            else:
                coeff_bits.append(item)
            i += 1
        coeff = make_mul(coeff_bits)
        if sin_arg not in (None, False) and sec_arg not in (None, False) and same(sin_arg, sec_arg):
            candidate = sim(mul([coeff, fn("tan", sin_arg)]))
            if equivalent(candidate, target):
                return candidate, "Rewrite sin(A)sec(A) as tan(A)."
        if cos_arg not in (None, False) and cosec_arg not in (None, False) and same(cos_arg, cosec_arg):
            candidate = sim(mul([coeff, fn("cot", cos_arg)]))
            if equivalent(candidate, target):
                return candidate, "Rewrite cos(A)cosec(A) as cot(A)."
    if not cheap_same(named, node) and equivalent(named, target):
        if node[0] == "div" and is_one(node[1]) and node[2][0] == "fn":
            if node[2][1] == "cos":
                return named, "Rewrite 1/cos(A) as sec(A)."
            if node[2][1] == "sin":
                return named, "Rewrite 1/sin(A) as cosec(A)."
        if node[0] == "div" and node[2][0] == "fn":
            if node[2][1] == "cos":
                return named, "Rewrite in terms of sec."
            if node[2][1] == "sin":
                return named, "Rewrite in terms of cosec."
        return named, "Rewrite in named trig form."
    return None, None


def proof_target_layout_steps(node, target):
    node = sim(node)
    target = sim(target)
    if node[0] != "div":
        return None
    numer = node[1]
    denom = node[2]
    if numer[0] == "fn" and numer[1] == "sin":
        den_items = list(flat(denom, "mul")) if denom[0] == "mul" else [denom]
        if len(den_items) == 2:
            left = match_simple_trig_term_norm(den_items[0], "sin")
            right = match_simple_trig_term_norm(den_items[1], "cos")
            if left is None or right is None:
                left = match_simple_trig_term_norm(den_items[0], "cos")
                right = match_simple_trig_term_norm(den_items[1], "sin")
                if left is not None and right is not None:
                    left, right = right, left
            if left is not None and right is not None and cheap_same(left[1], right[1]):
                base = left[1]
                target_info = match_sin_cos_fraction_target(target)
                if target_info is not None and cheap_same(target_info[2], base) and cheap_same(target_info[0], target_info[1]):
                    arg_pair = split_sum(numer[2])
                    if arg_pair is not None and cheap_same(arg_pair[0], target_info[0]) and cheap_same(arg_pair[1], base):
                        if target_info[3] > 0 and arg_pair[2] > 0:
                            raw = ("div", add([mul([fn("sin", target_info[0]), fn("cos", base)]), mul([fn("cos", target_info[0]), fn("sin", base)])]), denom)
                            return [FORMULA_SIN_ADD, raw_step_line(raw), step_line(target)]
                        if target_info[3] < 0 and arg_pair[2] < 0:
                            raw = ("div", add([mul([fn("sin", target_info[0]), fn("cos", base)]), neg(mul([fn("cos", target_info[0]), fn("sin", base)]))]), denom)
                            return [FORMULA_SIN_SUB, raw_step_line(raw), step_line(target)]
    return None


def match_sin_cos_fraction_target(node):
    node = sim(node)
    if node[0] != "add":
        return None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None
    sin_arg = None
    cos_arg = None
    base = None
    cos_sign = None
    i = 0
    while i < len(bits):
        nume, den = split_fraction_term(bits[i])
        coeff, rest = split_coeff(nume)
        if not (is_one(coeff) or is_minus_one(coeff)):
            return None
        if rest[0] != "fn" or den[0] != "fn":
            return None
        if rest[1] == "sin" and den[1] == "sin":
            if not is_one(coeff):
                return None
            sin_arg = rest[2]
            base = den[2] if base is None else base
            if not cheap_same(base, den[2]):
                return None
        elif rest[1] == "cos" and den[1] == "cos":
            cos_arg = rest[2]
            cos_sign = 1 if is_one(coeff) else -1
            base = den[2] if base is None else base
            if not cheap_same(base, den[2]):
                return None
        else:
            return None
        i += 1
    if sin_arg is None or cos_arg is None or base is None or cos_sign is None:
        return None
    return sin_arg, cos_arg, base, cos_sign


def remove_factor_from_term(node, factor):
    coeff, rest = split_coeff(node)
    items = list(flat(rest, "mul")) if rest[0] == "mul" else [rest]
    i = 0
    while i < len(items):
        if cheap_same(items[i], factor):
            remain = make_mul(items[:i] + items[i + 1 :])
            if is_one(remain):
                return coeff
            return sim(mul([coeff, remain]))
        if items[i][0] == "pow" and cheap_same(items[i][1], factor) and is_int_num(items[i][2]) and items[i][2][1] > 1:
            exp_value = items[i][2][1]
            remain_bits = list(items)
            if exp_value == 2:
                remain_bits[i] = factor
            else:
                remain_bits[i] = power(factor, num(exp_value - 1))
            remain = make_mul(remain_bits)
            if is_one(remain):
                return coeff
            return sim(mul([coeff, remain]))
        i += 1
    return None


def factor_common_term_once(node, var):
    if node[0] != "add":
        return None, None
    terms = list(flat(node, "add"))
    if len(terms) < 2:
        return None, None
    candidates = {}
    i = 0
    while i < len(terms):
        coeff, rest = split_coeff(terms[i])
        _ = coeff
        if depends(rest, var):
            items = list(flat(rest, "mul")) if rest[0] == "mul" else [rest]
            seen = {}
            j = 0
            while j < len(items):
                item = items[j]
                if depends(item, var):
                    trial_items = [item]
                    if item[0] == "pow" and is_int_num(item[2]) and item[2][1] > 1 and depends(item[1], var):
                        trial_items.append(item[1])
                    k = 0
                    while k < len(trial_items):
                        trial = trial_items[k]
                        key = sig(trial)
                        if key not in seen:
                            seen[key] = 1
                            if key not in candidates:
                                candidates[key] = [trial, []]
                            candidates[key][1].append(i)
                        k += 1
                j += 1
        i += 1
    best = None
    for key in candidates:
        item, picks = candidates[key]
        if len(picks) < 2:
            continue
        score = (-len(picks), -tree_size(item), reciprocal_burden(item))
        if best is None or score < best[0]:
            best = (score, item, picks)
    if best is None:
        return None, None
    factor = best[1]
    picks = best[2]
    inner_terms = []
    outer_terms = []
    i = 0
    while i < len(terms):
        if i in picks:
            reduced = remove_factor_from_term(terms[i], factor)
            if reduced is None:
                return None, None
            inner_terms.append(reduced)
        else:
            outer_terms.append(terms[i])
        i += 1
    if len(inner_terms) < 2:
        return None, None
    factored = sim(mul([factor, add(inner_terms)]))
    new_expr = factored if len(outer_terms) == 0 else sim(add([factored] + outer_terms))
    if cheap_same(new_expr, node):
        return None, None
    return new_expr, "Factorise."


def factor_common_term_for_proof(node):
    node = sim(node)
    pieces = [node]
    if node[0] == "mul":
        pieces = list(flat(node, "mul"))
    names = set()
    collect_symbols(node, names)
    name_list = list(names)
    name_list.sort()
    best = None

    def consider(expr):
        nonlocal best
        i = 0
        while i < len(name_list):
            factored, note = factor_common_term_once(expr, name_list[i])
            if factored is not None:
                score = (tree_size(factored), reciprocal_burden(factored))
                if best is None or score < best[0]:
                    best = (score, factored, note)
            i += 1

    consider(node)
    if node[0] == "mul":
        i = 0
        while i < len(pieces):
            if pieces[i][0] == "add":
                j = 0
                while j < len(name_list):
                    factored, note = factor_common_term_once(pieces[i], name_list[j])
                    if factored is not None:
                        rebuilt = sim(make_mul(pieces[:i] + [factored] + pieces[i + 1 :]))
                        score = (tree_size(rebuilt), reciprocal_burden(rebuilt))
                        if best is None or score < best[0]:
                            best = (score, rebuilt, note)
                    j += 1
            i += 1
    if best is None:
        return None, None
    return best[1], best[2]


def factor_fraction_terms_for_display(node):
    node = sim(node)
    if node[0] != "div":
        return None, None
    numer = node[1]
    denom = node[2]
    changed_numer = False
    changed_denom = False
    numer_factored, _note = factor_common_term_for_proof(numer)
    if numer_factored is not None:
        numer = numer_factored
        changed_numer = True
    denom_factored, _note = factor_common_term_for_proof(denom)
    if denom_factored is not None:
        denom = denom_factored
        changed_denom = True
    if not changed_numer and not changed_denom:
        return None, None
    if changed_numer and changed_denom:
        note = "Factorise numerator and denominator."
    elif changed_numer:
        note = "Factorise the numerator."
    else:
        note = "Factorise the denominator."
    return ("div", numer, denom), note


def named_power_product_info(node):
    coeff, rest = split_coeff(node)
    items = list(flat(rest, "mul")) if rest[0] == "mul" else [rest]
    powers = {}
    arg = None
    i = 0
    while i < len(items):
        item = items[i]
        exp = None
        fn_node = None
        if item[0] == "fn" and item[1] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            fn_node = item
            exp = 1
        elif item[0] == "pow" and item[1][0] == "fn" and is_int_num(item[2]) and item[2][1] > 0:
            if item[1][1] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
                fn_node = item[1]
                exp = item[2][1]
        else:
            return None
        if arg is None:
            arg = fn_node[2]
        elif not equivalent(arg, fn_node[2]):
            return None
        name = fn_node[1]
        if name in powers:
            powers[name] += exp
        else:
            powers[name] = exp
        i += 1
    return coeff, arg, powers


def build_named_power_term(name, arg, exp):
    if exp <= 0:
        return num(1)
    if exp == 1:
        return fn(name, arg)
    return power(fn(name, arg), num(exp))


def build_named_power_product(arg, powers):
    order = ("sin", "cos", "tan", "cot", "sec", "cosec")
    parts = []
    i = 0
    while i < len(order):
        exp = powers.get(order[i], 0)
        if exp > 0:
            parts.append(build_named_power_term(order[i], arg, exp))
        i += 1
    return make_mul(parts)


def factor_trig_power_pair_for_proof(node):
    node = sim(node)
    outer_coeff, rest = split_coeff(node)
    if rest[0] != "add":
        return None, None
    bits = list(flat(rest, "add"))
    if len(bits) != 2:
        return None, None
    info1 = named_power_product_info(bits[0])
    info2 = named_power_product_info(bits[1])
    if info1 is None or info2 is None or not equivalent(info1[1], info2[1]):
        return None, None
    if not equivalent(display_abs(info1[0]), display_abs(info2[0])):
        return None, None
    arg = info1[1]
    names = {key: 1 for key in info1[2]}
    for key in info2[2]:
        names[key] = 1
    if len(names) != 2:
        return None, None
    name_list = list(names)
    name_list.sort()
    common = {}
    rem1 = {}
    rem2 = {}
    i = 0
    while i < len(name_list):
        name = name_list[i]
        exp1 = info1[2].get(name, 0)
        exp2 = info2[2].get(name, 0)
        common[name] = exp1 if exp1 < exp2 else exp2
        rem1[name] = exp1 - common[name]
        rem2[name] = exp2 - common[name]
        i += 1
    common_factor = build_named_power_product(arg, common)
    if is_one(common_factor):
        return None, None
    coeff_mag = display_abs(info1[0])
    coeff_choices = [coeff_mag, neg(coeff_mag)]
    i = 0
    while i < len(coeff_choices):
        coeff_choice = coeff_choices[i]
        if not is_num(coeff_choice):
            i += 1
            continue
        rel1 = divq(info1[0], coeff_choice)
        rel2 = divq(info2[0], coeff_choice)
        if not ((is_one(rel1) or is_minus_one(rel1)) and (is_one(rel2) or is_minus_one(rel2))):
            i += 1
            continue
        rem1_term = build_named_power_product(arg, rem1)
        rem2_term = build_named_power_product(arg, rem2)
        inner = add([
            rem1_term if is_one(rel1) else neg(rem1_term),
            rem2_term if is_one(rel2) else neg(rem2_term),
        ])
        reduced, _note = pythagorean_identity_rewrite_once(inner)
        if reduced is not None:
            total_factor = sim(mul([outer_coeff, coeff_choice, common_factor]))
            return sim(mul([total_factor, inner])), "Factorise."
        i += 1
    return None, None


def pythagorean_identity_rewrite_once(node):
    node = sim(node)
    bits = list(flat(node, "add")) if node[0] == "add" else [node]
    if len(bits) != 2:
        return None, None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None, None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_one(coeff1) and is_one(coeff2):
        if (
            rest1[0] == "pow"
            and rest2[0] == "pow"
            and cheap_same(rest1[2], num(2))
            and cheap_same(rest2[2], num(2))
            and rest1[1][0] == "fn"
            and rest2[1][0] == "fn"
            and cheap_same(rest1[1][2], rest2[1][2])
        ):
            names = {rest1[1][1], rest2[1][1]}
            if names == {"sin", "cos"}:
                return num(1), "Use sin^2 A + cos^2 A = 1."
    if is_one(coeff1) and is_minus_one(coeff2):
        if (
            rest1[0] == "pow"
            and rest2[0] == "pow"
            and cheap_same(rest1[2], num(2))
            and cheap_same(rest2[2], num(2))
            and rest1[1][0] == "fn"
            and rest2[1][0] == "fn"
            and cheap_same(rest1[1][2], rest2[1][2])
        ):
            if rest1[1][1] == "sec" and rest2[1][1] == "tan":
                return num(1), "Use sec^2 A - tan^2 A = 1."
            if rest1[1][1] == "cosec" and rest2[1][1] == "cot":
                return num(1), "Use cosec^2 A - cot^2 A = 1."
    return None, None


def expand_square(node):
    node = sim(node)
    if node[0] == "pow" and same(node[2], num(2)) and node[1][0] == "add":
        bits = list(flat(node[1], "add"))
        if len(bits) == 2:
            a = bits[0]
            b = bits[1]
            return add([power(a, num(2)), mul([num(2), a, b]), power(b, num(2))])
    return None


def expand_powered_monomial(node):
    node = sim(node)
    if node[0] != "pow" or not same(node[2], num(2)) or node[1][0] != "mul":
        return None
    items = list(flat(node[1], "mul"))
    coeff = num(1)
    others = []
    i = 0
    while i < len(items):
        if is_num(items[i]):
            coeff = mulq(coeff, items[i])
        else:
            others.append(items[i])
        i += 1
    if same(coeff, num(1)) or len(others) == 0:
        return None
    other = make_mul(others)
    return sim(mul([power(coeff, num(2)), power(other, num(2))]))


def expand_product(node):
    node = sim(node)
    if node[0] != "mul":
        return None
    items = list(flat(node, "mul"))
    i = 0
    while i < len(items):
        if items[i][0] == "add":
            terms = list(flat(items[i], "add"))
            out = []
            j = 0
            while j < len(terms):
                out.append(mul(items[:i] + [terms[j]] + items[i + 1 :]))
                j += 1
            return add(out)
        i += 1
    return None


def expand_fraction(node):
    node = sim(node)
    if node[0] != "div":
        return None
    top = node[1]
    bot = node[2]
    if top[0] == "add":
        out = []
        terms = list(flat(top, "add"))
        i = 0
        while i < len(terms):
            out.append(div(terms[i], bot))
            i += 1
        return add(out)
    return None


def expand_small(node):
    cur = sim(node)
    i = 0
    while i < 5:
        nxt = expand_fraction(cur)
        if nxt is None:
            nxt = expand_square(cur)
        if nxt is None:
            nxt = expand_powered_monomial(cur)
        if nxt is None:
            nxt = expand_product(cur)
        if nxt is None or same(nxt, cur):
            return cur
        cur = sim(nxt)
        i += 1
    return cur


def expand_embedded_small(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        cur = fn(node[1], expand_embedded_small(node[2]))
    elif kind == "pow":
        cur = power(expand_embedded_small(node[1]), expand_embedded_small(node[2]))
    elif kind == "div":
        cur = div(expand_embedded_small(node[1]), expand_embedded_small(node[2]))
    else:
        out = []
        i = 0
        while i < len(node[1]):
            out.append(expand_embedded_small(node[1][i]))
            i += 1
        cur = sim((kind, tuple(out)))
    nxt = expand_small(cur)
    if same(nxt, cur):
        return cur
    return expand_embedded_small(nxt)


def rewrite_children(node, fn_rewrite):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return sim(("fn", node[1], fn_rewrite(node[2])))
    if kind == "pow":
        return sim(("pow", fn_rewrite(node[1]), fn_rewrite(node[2])))
    if kind == "div":
        return sim(("div", fn_rewrite(node[1]), fn_rewrite(node[2])))
    out = []
    i = 0
    while i < len(node[1]):
        out.append(fn_rewrite(node[1][i]))
        i += 1
    return sim((kind, tuple(out)))


def expand_trig_tree(node):
    node = sim(node)

    def inner(cur):
        cur = rewrite_children(cur, inner)
        out, _ = trig_formula_expand(cur)
        if out is not None:
            return sim(out)
        out, _ = half_angle_expand(cur)
        if out is not None:
            return sim(out)
        out, _ = power_reduction_once(cur)
        if out is not None:
            return sim(out)
        return sim(cur)

    return inner(node)


def expand_safe_trig_tree(node):
    node = sim(node)

    def inner(cur):
        cur = rewrite_children(cur, inner)
        out, _ = trig_formula_expand(cur)
        if out is not None:
            return sim(out)
        return sim(cur)

    return inner(node)


def is_square_of(name, node, arg):
    return node[0] == "pow" and same(node[2], num(2)) and node[1][0] == "fn" and node[1][1] == name and same(node[1][2], arg)


def reduce_special_add(node):
    node = sim(node)
    if node[0] != "add":
        return node
    items = list(flat(node, "add"))
    changed = True
    while changed:
        changed = False
        i = 0
        while i < len(items):
            coeff_i, rest_i = split_coeff(items[i])
            j = i + 1
            while j < len(items):
                coeff_j, rest_j = split_coeff(items[j])
                if coeff_i == coeff_j:
                    if rest_i[0] == "pow" and rest_j[0] == "pow" and same(rest_i[2], num(2)) and same(rest_j[2], num(2)):
                        if rest_i[1][0] == "fn" and rest_j[1][0] == "fn" and same(rest_i[1][2], rest_j[1][2]):
                            if (rest_i[1][1], rest_j[1][1]) in (("sin", "cos"), ("cos", "sin")):
                                items.pop(j)
                                items.pop(i)
                                items.append(coeff_i)
                                changed = True
                                break
                j += 1
            if changed:
                break
            i += 1
        if changed:
            continue
        i = 0
        while i < len(items):
            if is_one(items[i]):
                j = 0
                while j < len(items):
                    coeff_j, rest_j = split_coeff(items[j])
                    if is_minus_one(coeff_j) and rest_j[0] == "pow" and same(rest_j[2], num(2)) and rest_j[1][0] == "fn":
                        name = rest_j[1][1]
                        arg = rest_j[1][2]
                        if name == "sin":
                            items.pop(j)
                            items.pop(i if i < j else i - 1)
                            items.append(power(fn("cos", arg), num(2)))
                            changed = True
                            break
                        if name == "cos":
                            items.pop(j)
                            items.pop(i if i < j else i - 1)
                            items.append(power(fn("sin", arg), num(2)))
                            changed = True
                            break
                    j += 1
                if changed:
                    break
            i += 1
    return sim(("add", tuple(items)))


def reduce_identities(node):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return sim(("fn", node[1], reduce_identities(node[2])))
    if kind == "pow":
        return sim(("pow", reduce_identities(node[1]), reduce_identities(node[2])))
    if kind == "div":
        return sim(("div", reduce_identities(node[1]), reduce_identities(node[2])))
    out = []
    i = 0
    while i < len(node[1]):
        out.append(reduce_identities(node[1][i]))
        i += 1
    if kind == "add":
        return reduce_special_add(("add", tuple(out)))
    return sim((kind, tuple(out)))


def has_variable_dependency(node):
    names = set()
    collect_symbols(node, names)
    return len(names) != 0


def simplify_fraction(node):
    node = sim(node)
    if node[0] != "div":
        return node
    numer, denom = node[1], node[2]
    if same(numer, denom) and has_variable_dependency(denom):
        return node

    def cancel_factor(expr, factor):
        if same(expr, factor):
            if has_variable_dependency(factor):
                return expr
            return num(1)
        if expr[0] == "mul":
            items = list(flat(expr, "mul"))
            i = 0
            while i < len(items):
                if same(items[i], factor):
                    if has_variable_dependency(factor):
                        return expr
                    reduced = items[:i] + items[i + 1 :]
                    return make_mul(reduced)
                i += 1
            i = 0
            while i < len(items):
                if items[i][0] == "pow" and same(items[i][1], factor) and items[i][2][0] == "num":
                    if has_variable_dependency(factor):
                        return expr
                    e = items[i][2][1]
                    if e > 1:
                        reduced = items[:i] + [power(factor, num(e - 1))] + items[i + 1 :]
                        return make_mul(reduced)
                    if e == 1:
                        reduced = items[:i] + items[i + 1 :]
                        return make_mul(reduced)
                i += 1
        if expr[0] == "pow" and same(expr[1], factor) and expr[2][0] == "num":
            if has_variable_dependency(factor):
                return expr
            e = expr[2][1]
            if e > 1:
                return power(factor, num(e - 1))
            if e == 1:
                return num(1)
        return expr

    new_numer = cancel_factor(numer, denom)
    if not same(new_numer, numer):
        return new_numer
    return node


def _full_simplify_uncached(node):
    cur = sim(node)
    i = 0
    while i < 8:
        prev = cur
        cur = reciprocal_trig(cur)
        cur = expand_safe_trig_tree(cur)
        cur = expand_small(cur)
        cur = sim(cur)
        cur = reduce_identities(cur)
        cur = sim(cur)
        if same(cur, prev):
            break
        i += 1
    return sim(cur)


def full_simplify(node):
    cached = FULL_SIMPLIFY_CACHE.get(node)
    if cached is not None:
        return cached
    return cache_store(FULL_SIMPLIFY_CACHE, node, _full_simplify_uncached(node), CACHE_LIMIT_MEDIUM)


def eval_numeric_mode(node, env, deg_mode):
    kind = node[0]
    if kind == "num":
        return node[1] * 1.0 / node[2]
    if kind == "sym":
        if node[1] not in env:
            raise ValueError("Missing value for " + node[1])
        return env[node[1]]
    if kind == "const":
        if node[1] == "pi":
            return math.pi
        if node[1] == "e":
            return math.e
        raise ValueError("Unknown constant.")
    if kind == "fn":
        name = node[1]
        arg = eval_numeric_mode(node[2], env, deg_mode)
        trig_arg = to_radians(arg) if deg_mode and name in ("sin", "cos", "tan", "sec", "cosec", "cot") else arg
        if name == "sin":
            return math.sin(trig_arg)
        if name == "cos":
            return math.cos(trig_arg)
        if name == "tan":
            c = math.cos(trig_arg)
            if abs(c) < 1e-12:
                raise ValueError("tan undefined")
            return math.sin(trig_arg) / c
        if name == "sec":
            c = math.cos(trig_arg)
            if abs(c) < 1e-12:
                raise ValueError("sec undefined")
            return 1.0 / c
        if name == "cosec":
            s = math.sin(trig_arg)
            if abs(s) < 1e-12:
                raise ValueError("cosec undefined")
            return 1.0 / s
        if name == "cot":
            s = math.sin(trig_arg)
            if abs(s) < 1e-12:
                raise ValueError("cot undefined")
            return math.cos(trig_arg) / s
        if name == "sqrt":
            return math.sqrt(arg)
        if name == "abs":
            return abs(arg)
        if name == "log":
            return math.log(arg)
        raise ValueError("Unknown function: " + name)
    if kind == "pow":
        return eval_numeric_mode(node[1], env, deg_mode) ** eval_numeric_mode(node[2], env, deg_mode)
    if kind == "div":
        return eval_numeric_mode(node[1], env, deg_mode) / eval_numeric_mode(node[2], env, deg_mode)
    if kind == "mul":
        out = 1.0
        i = 0
        while i < len(node[1]):
            out *= eval_numeric_mode(node[1][i], env, deg_mode)
            i += 1
        return out
    out = 0.0
    i = 0
    while i < len(node[1]):
        out += eval_numeric_mode(node[1][i], env, deg_mode)
        i += 1
    return out


def _equivalent_uncached(a, b):
    if same(a, b) and has_variable_dependency(a):
        return True
    diff = full_simplify(add([a, neg(b)]))
    diff_size = tree_size(diff)
    if diff_size > 80:
        return False
    if is_zero(diff):
        if a[0] == "div" and same(a[1], a[2]) and has_variable_dependency(a[1]):
            return False
        if b[0] == "div" and same(b[1], b[2]) and has_variable_dependency(b[1]):
            return False
        return True
    if (a[0] == "div" and same(a[1], a[2]) and has_variable_dependency(a[1]) and same(b, num(1))) or (b[0] == "div" and same(b[1], b[2]) and has_variable_dependency(b[1]) and same(a, num(1))):
        return False
    if math is None:
        return False
    names = set()
    collect_symbols(diff, names)
    names = sorted(list(names))
    samples = [0.23, 0.61, 1.11, 1.63]
    good = 0
    if len(names) == 0:
        try:
            value = eval_numeric(diff, {})
            return abs(value) < 1e-8
        except (ValueError, ZeroDivisionError):
            return False
    if len(names) == 1:
        i = 0
        while i < len(samples):
            env = {names[0]: samples[i]}
            try:
                value = eval_numeric(diff, env)
            except (ValueError, ZeroDivisionError):
                i += 1
                continue
            if is_finite_value(value):
                if abs(value) > 1e-6:
                    return False
                good += 1
            i += 1
        return good >= 2
    if len(names) == 2:
        pts = [(0.23, 0.41), (0.61, 0.9), (1.11, 0.37), (1.63, 0.74)]
        i = 0
        while i < len(pts):
            env = {names[0]: pts[i][0], names[1]: pts[i][1]}
            try:
                value = eval_numeric(diff, env)
            except (ValueError, ZeroDivisionError):
                i += 1
                continue
            if is_finite_value(value):
                if abs(value) > 1e-6:
                    return False
                good += 1
            i += 1
        return good >= 2
    return False


def equivalent(a, b):
    if a == b:
        return True
    sig_a = sig(a)
    sig_b = sig(b)
    if sig_a == sig_b:
        return True
    if sig_a <= sig_b:
        key = (sig_a, sig_b)
    else:
        key = (sig_b, sig_a)
    cached = EQUIVALENT_CACHE.get(key)
    if cached is not None:
        return cached
    return cache_store(EQUIVALENT_CACHE, key, _equivalent_uncached(a, b), CACHE_LIMIT_SMALL)


def start_line(side_name, expr):
    if side_name in ("LHS", "RHS"):
        return show(expr)
    return side_name + " = " + show(expr)


def step_line(expr):
    return "= " + show(expr)


def equation_line(left, right):
    return show(left) + " = " + show(right)


def ordered_sum_text(nodes):
    out = ""
    i = 0
    while i < len(nodes):
        node = sim(nodes[i])
        if not is_zero(node):
            head = display_abs(node) if display_neg(node) else node
            chunk = show(head)
            if out == "":
                out = "-" + chunk if display_neg(node) else chunk
            else:
                out += "-" + chunk if display_neg(node) else "+" + chunk
        i += 1
    if out == "":
        return "0"
    return out


def quadratic_standard_text(a, var_node, b, c):
    return ordered_sum_text([mul([a, power(var_node, num(2))]), mul([b, var_node]), c])


def exact_pi_multiple(node):
    node = sim(node)
    if same(node, PI):
        return num(1)
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        coeff = num(1)
        found = False
        i = 0
        while i < len(items):
            if same(items[i], PI):
                if found:
                    return None
                found = True
            elif is_num(items[i]):
                coeff = mulq(coeff, items[i])
            else:
                inner = exact_pi_multiple(items[i])
                if inner is None:
                    return None
                if found:
                    return None
                found = True
                coeff = mulq(coeff, inner)
            i += 1
        if found:
            return coeff
    if node[0] == "div" and same(node[1], PI) and is_num(node[2]):
        return divq(num(1), node[2])
    if node[0] == "div" and node[1][0] == "mul":
        items = list(flat(node[1], "mul"))
        coeff = num(1)
        found = False
        i = 0
        while i < len(items):
            if same(items[i], PI):
                if found:
                    return None
                found = True
            elif is_num(items[i]):
                coeff = mulq(coeff, items[i])
            else:
                inner = exact_pi_multiple(items[i])
                if inner is None:
                    return None
                if found:
                    return None
                found = True
                coeff = mulq(coeff, inner)
            i += 1
        if found and is_num(node[2]):
            return divq(coeff, node[2])
    if node[0] == "div":
        top = exact_pi_multiple(node[1])
        if top is not None and is_num(node[2]):
            return divq(top, node[2])
    return None


def contains_pi(node):
    kind = node[0]
    if kind == "const":
        return node[1] == "pi"
    if kind in ("num", "sym"):
        return False
    if kind == "fn":
        return contains_pi(node[2])
    if kind in ("pow", "div"):
        return contains_pi(node[1]) or contains_pi(node[2])
    i = 0
    while i < len(node[1]):
        if contains_pi(node[1][i]):
            return True
        i += 1
    return False


def collect_angle_units(node, out):
    kind = node[0]
    if kind == "fn" and node[1] in ("sin", "cos", "tan", "sec", "cosec", "cot"):
        if contains_pi(node[2]):
            out.append("rad")
        elif angle_to_degree(node[2]) is not None:
            out.append("deg")
        else:
            raise ValueError("Exact value mode needs constant angles.")
        collect_angle_units(node[2], out)
        return
    if kind in ("num", "sym", "const"):
        return
    if kind == "fn":
        collect_angle_units(node[2], out)
        return
    if kind in ("pow", "div"):
        collect_angle_units(node[1], out)
        collect_angle_units(node[2], out)
        return
    i = 0
    while i < len(node[1]):
        collect_angle_units(node[1][i], out)
        i += 1


def angle_to_degree(node):
    node = sim(node)
    pi_mult = exact_pi_multiple(node)
    if pi_mult is not None:
        return mulq(pi_mult, num(180))
    if is_num(node):
        return node
    return None


def degree_mod_360(deg):
    deg = sim(deg)
    if not is_num(deg):
        return None
    mod = 360 * deg[2]
    n = deg[1] % mod
    if n < 0:
        n += mod
    return num(n, deg[2])


def degree_int(node):
    node = degree_mod_360(node)
    if node is None or node[2] != 1:
        return None
    return node[1]


def reference_degree(deg):
    deg = degree_int(deg)
    if deg is None:
        return None
    if deg > 180:
        deg = 360 - deg
    if deg > 90:
        deg = 180 - deg
    return deg


def quadrant_of_degree(deg):
    deg = degree_int(deg)
    if deg is None:
        return None
    if deg == 0 or deg == 90 or deg == 180 or deg == 270:
        return 0
    if deg < 90:
        return 1
    if deg < 180:
        return 2
    if deg < 270:
        return 3
    return 4


def exact_first_quadrant(name, deg):
    r2 = fn("sqrt", num(2))
    r3 = fn("sqrt", num(3))
    r6 = fn("sqrt", num(6))
    if deg == 0:
        if name in ("sin", "tan"):
            return num(0)
        if name in ("cos", "sec"):
            return num(1)
        if name == "cosec":
            return None
        if name == "cot":
            return None
    if deg == 30:
        table = {
            "sin": num(1, 2),
            "cos": div(r3, num(2)),
            "tan": div(num(1), r3),
            "sec": div(num(2), r3),
            "cosec": num(2),
            "cot": r3,
        }
        return table.get(name)
    if deg == 45:
        table = {
            "sin": div(r2, num(2)),
            "cos": div(r2, num(2)),
            "tan": num(1),
            "sec": r2,
            "cosec": r2,
            "cot": num(1),
        }
        return table.get(name)
    if deg == 60:
        table = {
            "sin": div(r3, num(2)),
            "cos": num(1, 2),
            "tan": r3,
            "sec": num(2),
            "cosec": div(num(2), r3),
            "cot": div(num(1), r3),
        }
        return table.get(name)
    if deg == 90:
        table = {
            "sin": num(1),
            "cos": num(0),
            "tan": None,
            "sec": None,
            "cosec": num(1),
            "cot": num(0),
        }
        return table.get(name)
    if deg == 15:
        table = {
            "sin": div(add([r6, neg(r2)]), num(4)),
            "cos": div(add([r6, r2]), num(4)),
            "tan": add([num(2), neg(r3)]),
            "sec": div(num(1), div(add([r6, r2]), num(4))),
            "cosec": div(num(1), div(add([r6, neg(r2)]), num(4))),
            "cot": add([num(2), r3]),
        }
        return sim(table.get(name))
    if deg == 75:
        table = {
            "sin": div(add([r6, r2]), num(4)),
            "cos": div(add([r6, neg(r2)]), num(4)),
            "tan": add([num(2), r3]),
            "sec": div(num(1), div(add([r6, neg(r2)]), num(4))),
            "cosec": div(num(1), div(add([r6, r2]), num(4))),
            "cot": add([num(2), neg(r3)]),
        }
        return sim(table.get(name))
    return None


def exact_trig_value(name, arg):
    deg = angle_to_degree(arg)
    if deg is None:
        raise ValueError("Exact value mode needs constant angles.")
    deg = degree_mod_360(deg)
    if deg is None:
        raise ValueError("Unsupported exact angle.")
    ref = reference_degree(deg)
    if ref is None or ref % 15 != 0:
        raise ValueError("Only standard exact angles are supported.")
    base = exact_first_quadrant(name, ref)
    if base is None:
        raise ValueError("This exact value is undefined.")
    quad = quadrant_of_degree(deg)
    if name in ("sin", "cosec"):
        if quad in (3, 4):
            return neg(base)
        return base
    if name in ("cos", "sec"):
        if quad in (2, 3):
            return neg(base)
        return base
    if name in ("tan", "cot"):
        if quad in (2, 4):
            return neg(base)
        return base
    raise ValueError("Bad trig function.")


def exact_trig_lines(name, arg):
    deg = angle_to_degree(arg)
    if deg is None:
        raise ValueError("Exact value mode needs constant angles.")
    deg = degree_mod_360(deg)
    if deg is None:
        raise ValueError("Unsupported exact angle.")
    d = degree_int(deg)
    ref = reference_degree(deg)
    lines = []
    if d != ref and d not in (0, 90, 180, 270):
        if quadrant_of_degree(deg) == 2:
            if name in ("sin", "cosec"):
                lines.append(name + "(" + show(arg) + ") = " + name + "(" + str(ref) + ")")
            else:
                lines.append(name + "(" + show(arg) + ") = -" + name + "(" + str(ref) + ")")
        elif quadrant_of_degree(deg) == 3:
            if name in ("tan", "cot"):
                lines.append(name + "(" + show(arg) + ") = " + name + "(" + str(ref) + ")")
            else:
                lines.append(name + "(" + show(arg) + ") = -" + name + "(" + str(ref) + ")")
        elif quadrant_of_degree(deg) == 4:
            if name in ("cos", "sec"):
                lines.append(name + "(" + show(arg) + ") = " + name + "(" + str(ref) + ")")
            else:
                lines.append(name + "(" + show(arg) + ") = -" + name + "(" + str(ref) + ")")
    if ref in (15, 75):
        sign = "+" if ref == 75 else "-"
        lines.append(str(ref) + " = 45 " + sign + " 30")
        if name == "sin":
            lines.append("sin(" + str(ref) + ") = sin(45" + sign + "30)")
            lines.append("= sin(45)cos(30) " + sign + " cos(45)sin(30)")
        elif name == "cos":
            lines.append("cos(" + str(ref) + ") = cos(45" + sign + "30)")
            if ref == 75:
                lines.append("= cos(45)cos(30) - sin(45)sin(30)")
            else:
                lines.append("= cos(45)cos(30) + sin(45)sin(30)")
        elif name == "tan":
            lines.append("tan(" + str(ref) + ") = tan(45" + sign + "30)")
            if ref == 75:
                lines.append("= (tan(45)+tan(30))/(1-tan(45)tan(30))")
            else:
                lines.append("= (tan(45)-tan(30))/(1+tan(45)tan(30))")
        elif name in ("cot", "sec", "cosec"):
            base_name = {"cot": "tan", "sec": "cos", "cosec": "sin"}[name]
            lines.extend(exact_trig_lines(base_name, num(ref)))
            lines.append(name + "(" + str(ref) + ") = 1/" + base_name + "(" + str(ref) + ")")
    return lines


def replace_exact_trig(node, lines):
    node = sim(node)
    if node[0] in ("num", "sym", "const"):
        return node
    if node[0] == "fn" and node[1] in ("sin", "cos", "tan", "sec", "cosec", "cot"):
        if angle_to_degree(node[2]) is None:
            return node
        try:
            value = exact_trig_value(node[1], node[2])
            sub_lines = exact_trig_lines(node[1], node[2])
        except Exception:
            return node
        i = 0
        while i < len(sub_lines):
            lines.append(sub_lines[i])
            i += 1
        lines.append(show(node) + " = " + show(value))
        return value
    if node[0] == "fn":
        return sim(("fn", node[1], replace_exact_trig(node[2], lines)))
    if node[0] == "pow":
        return sim(("pow", replace_exact_trig(node[1], lines), replace_exact_trig(node[2], lines)))
    if node[0] == "div":
        return sim(("div", replace_exact_trig(node[1], lines), replace_exact_trig(node[2], lines)))
    if node[0] in ("add", "mul"):
        out = []
        i = 0
        while i < len(node[1]):
            out.append(replace_exact_trig(node[1][i], lines))
            i += 1
        return sim((node[0], tuple(out)))
    return node


def replace_exact_trig_quiet(node):
    return full_simplify(replace_exact_trig(node, []))


def match_double_angle_arg(node):
    node = sim(node)
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        if len(items) == 2 and is_num(items[0]) and same(items[0], num(2)) and items[1][0] == "sym":
            return items[1]
    return None


def match_numeric_multiple_arg(node, value):
    node = sim(node)
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        if len(items) == 2 and is_num(items[0]) and same(items[0], num(value)):
            return items[1]
    return None


def match_same_angle_sin_cos_product(node):
    node = sim(node)
    if node[0] != "mul":
        return None
    items = list(flat(node, "mul"))
    coeff = num(1)
    sin_arg = None
    cos_arg = None
    i = 0
    while i < len(items):
        item = items[i]
        if is_num(item):
            coeff = mulq(coeff, item)
        elif item[0] == "fn" and item[1] == "sin":
            if sin_arg is not None:
                return None
            sin_arg = item[2]
        elif item[0] == "fn" and item[1] == "cos":
            if cos_arg is not None:
                return None
            cos_arg = item[2]
        else:
            return None
        i += 1
    if sin_arg is None or cos_arg is None or not same(sin_arg, cos_arg):
        return None
    return coeff, sin_arg


def match_cot_term(node):
    node = sim(node)
    if node[0] == "fn" and node[1] == "cot":
        base = match_double_angle_arg(node[2])
        if base is not None:
            return num(1), base
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        i = 0
        pick = -1
        base = None
        while i < len(items):
            if items[i][0] == "fn" and items[i][1] == "cot":
                base = match_double_angle_arg(items[i][2])
                if base is not None:
                    pick = i
                    break
            i += 1
        if pick != -1:
            coeff = make_mul(items[:pick] + items[pick + 1 :])
            if not depends(coeff, base[1]):
                return coeff, base
    return None


def match_sin_sq_cot_term(node):
    node = sim(node)
    items = [node]
    if node[0] == "mul":
        items = list(flat(node, "mul"))
    base = None
    saw_sin_sq = False
    saw_cot = False
    coeff_bits = []
    i = 0
    while i < len(items):
        item = items[i]
        if not saw_sin_sq and item[0] == "pow" and same(item[2], num(2)) and item[1][0] == "fn" and item[1][1] == "sin" and item[1][2][0] == "sym":
            saw_sin_sq = True
            base = item[1][2]
        elif not saw_cot:
            cot_match = match_cot_term(item)
            if cot_match is not None:
                saw_cot = True
                if base is None:
                    base = cot_match[1]
                elif not same(base, cot_match[1]):
                    return None
                coeff_bits.append(cot_match[0])
            else:
                coeff_bits.append(item)
        else:
            coeff_bits.append(item)
        i += 1
    if not (saw_sin_sq and saw_cot and base is not None):
        return None
    coeff = make_mul(coeff_bits)
    if depends(coeff, base[1]):
        return None
    return coeff, base


def match_sincos_term(node):
    node = sim(node)
    items = [node]
    if node[0] == "mul":
        items = list(flat(node, "mul"))
    base = None
    saw_sin = False
    saw_cos = False
    coeff_bits = []
    i = 0
    while i < len(items):
        item = items[i]
        if not saw_sin and item[0] == "fn" and item[1] == "sin" and item[2][0] == "sym":
            saw_sin = True
            base = item[2]
        elif not saw_cos and item[0] == "fn" and item[1] == "cos" and item[2][0] == "sym":
            saw_cos = True
            if base is None:
                base = item[2]
            elif not same(base, item[2]):
                return None
        else:
            coeff_bits.append(item)
        i += 1
    if not (saw_sin and saw_cos and base is not None):
        return None
    coeff = make_mul(coeff_bits)
    if depends(coeff, base[1]):
        return None
    return coeff, base


def match_partb_rhs(node):
    node = sim(node)
    terms = [node]
    if node[0] == "add":
        terms = list(flat(node, "add"))
    if len(terms) != 2:
        return None
    first = match_sin_sq_cot_term(terms[0])
    second = match_sincos_term(terms[1])
    if first is None or second is None:
        first = match_sincos_term(terms[0])
        second = match_sin_sq_cot_term(terms[1])
        if first is None or second is None:
            return None
        n_coeff, base1 = first
        m_coeff, base2 = second
    else:
        m_coeff, base2 = first
        n_coeff, base1 = second
    if not same(base1, base2):
        return None
    return base1, m_coeff, n_coeff


def match_linear_cot_const(node):
    node = sim(node)
    terms = [node]
    if node[0] == "add":
        terms = list(flat(node, "add"))
    if len(terms) < 2:
        return None
    cot_coeff = None
    base = None
    const_terms = []
    i = 0
    while i < len(terms):
        cot_match = match_cot_term(terms[i])
        if cot_match is not None and cot_coeff is None:
            cot_coeff = cot_match[0]
            base = cot_match[1]
        else:
            const_terms.append(terms[i])
        i += 1
    if cot_coeff is None or base is None:
        return None
    const_expr = make_add(const_terms)
    if depends(const_expr, base[1]):
        return None
    return base, cot_coeff, const_expr


def nonzero_equation_expr(lhs, rhs):
    if is_zero(rhs):
        return lhs
    if is_zero(lhs):
        return neg(rhs)
    return add([lhs, neg(rhs)])


def derive_cot_quadratic_expr(source_lhs, source_rhs):
    options = [
        (sim(source_lhs), sim(source_rhs)),
        (sim(source_rhs), sim(source_lhs)),
    ]
    i = 0
    while i < len(options):
        left = options[i][0]
        right = options[i][1]
        left_match = match_linear_cot_const(left)
        right_match = match_partb_rhs(right)
        if left_match is not None and right_match is not None:
            base1, cot_coeff, const_term = left_match
            base2, m_coeff, n_coeff = right_match
            if same(base1, base2):
                half_m = full_simplify(div(m_coeff, num(2)))
                half_n = full_simplify(div(n_coeff, num(2)))
                if equivalent(cot_coeff, half_m):
                    dbl = mul([num(2), base1])
                    sin_dbl = fn("sin", dbl)
                    sin_coeff = full_simplify(add([half_m, half_n]))
                    linear_coeff = sim(neg(const_term))
                    const_coeff = sim(neg(half_m))
                    final_expr = sim(add([mul([sin_coeff, power(sin_dbl, num(2))]), mul([linear_coeff, sin_dbl]), const_coeff]))
                    return base1, sin_coeff, linear_coeff, const_coeff, final_expr
        i += 1
    return None


def prove_cot_quadratic_equation(source_lhs, source_rhs, target_lhs=None, target_rhs=None, target_text=None):
    options = [
        (source_lhs, source_rhs),
        (source_rhs, source_lhs),
    ]
    i = 0
    while i < len(options):
        left = sim(options[i][0])
        right = sim(options[i][1])
        left_match = match_linear_cot_const(left)
        right_match = match_partb_rhs(right)
        if left_match is not None and right_match is not None:
            base1, cot_coeff, const_term = left_match
            base2, m_coeff, n_coeff = right_match
            if same(base1, base2):
                half_m = full_simplify(div(m_coeff, num(2)))
                half_n = full_simplify(div(n_coeff, num(2)))
                if equivalent(cot_coeff, half_m):
                    base = base1
                    dbl = mul([num(2), base])
                    cot_dbl = fn("cot", dbl)
                    sin_dbl = fn("sin", dbl)
                    cos_dbl = fn("cos", dbl)
                    cosec_dbl = fn("cosec", dbl)
                    rhs1 = add([mul([half_m, add([num(1), neg(cos_dbl)]), cot_dbl]), mul([half_n, sin_dbl])])
                    rhs2 = add([mul([half_m, cot_dbl]), neg(mul([half_m, cos_dbl, cot_dbl])), mul([half_n, sin_dbl])])
                    sin_coeff = full_simplify(add([half_m, half_n]))
                    rhs4 = add([mul([half_m, cot_dbl]), neg(mul([half_m, cosec_dbl])), mul([sin_coeff, sin_dbl])])
                    after_cancel = add([neg(mul([half_m, cosec_dbl])), mul([sin_coeff, sin_dbl])])
                    mult_left = sim(mul([const_term, sin_dbl]))
                    mult_right = sim(add([neg(half_m), mul([sin_coeff, power(sin_dbl, num(2))])]))
                    linear_coeff = sim(neg(const_term))
                    const_coeff = sim(neg(half_m))
                    final_expr = sim(add([mul([sin_coeff, power(sin_dbl, num(2))]), mul([linear_coeff, sin_dbl]), const_coeff]))
                    final_text = quadratic_standard_text(sin_coeff, sin_dbl, linear_coeff, const_coeff)
                    target_ok = True
                    if target_lhs is not None and target_rhs is not None:
                        target_expr = nonzero_equation_expr(sim(target_lhs), sim(target_rhs))
                        if not (equivalent(final_expr, target_expr) or equivalent(final_expr, neg(target_expr))):
                            target_ok = False
                    if not target_ok:
                        i += 1
                        continue
                    lines = [
                        "Start with " + equation_line(source_lhs, source_rhs),
                        "Use sin^2(A) = (1-cos(2A))/2 and 2sin(A)cos(A) = sin(2A) on RHS",
                        equation_line(left, rhs1),
                        equation_line(left, rhs2),
                        "Use cos(2A)cot(2A) = cosec(2A)-sin(2A).",
                        equation_line(left, rhs4),
                        "Subtract " + show(mul([half_m, cot_dbl])) + " from both sides.",
                        equation_line(const_term, after_cancel),
                        show(cot_dbl) + " is defined, so " + show(sin_dbl) + " != 0. Multiply through by " + show(sin_dbl) + ".",
                        equation_line(mult_left, mult_right),
                        "Rearrange into standard form.",
                        final_text + " = 0",
                    ]
                    if target_lhs is not None and target_rhs is not None:
                        if target_text is None:
                            lines.append("Hence " + final_text + " = 0")
                        else:
                            lines.append("Hence " + target_text.strip())
                    else:
                        lines.append("Hence the equation can be written as " + final_text + " = 0")
                    return compact_lines(lines)
        i += 1
    return None


def match_shift_side(node):
    coeff = num(1)
    inner = node
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        i = 0
        pick = -1
        while i < len(items):
            if items[i][0] == "fn" and items[i][1] in ("sin", "cos"):
                if split_sum(items[i][2]) is not None:
                    pick = i
                    break
            i += 1
        if pick != -1:
            inner = items[pick]
            coeff_bits = items[:pick] + items[pick + 1 :]
            coeff = make_mul(coeff_bits)
    if inner[0] != "fn" or inner[1] not in ("sin", "cos"):
        return None
    pair = split_sum(inner[2])
    if pair is None:
        return None
    left, right, sign = pair
    left_deg = angle_to_degree(left)
    right_deg = angle_to_degree(right)
    if left_deg is None and right_deg is None:
        return None
    if left_deg is None and right_deg is not None:
        base = left
        shift = right
    elif left_deg is not None and right_deg is None:
        base = right
        if sign > 0:
            if left_deg[1] < 0:
                shift = neg(left)
                sign = -1
            else:
                shift = left
                sign = 1
        else:
            return None
    else:
        return None
    return coeff, inner[1], base, shift, sign


def independent_of_names(node, names):
    found = set()
    collect_symbols(node, found)
    i = 0
    name_list = list(found)
    while i < len(name_list):
        if name_list[i] in names:
            return False
        i += 1
    return True


def match_generic_shift_side(node):
    coeff = num(1)
    inner = node
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        i = 0
        pick = -1
        while i < len(items):
            if items[i][0] == "fn" and items[i][1] in ("sin", "cos"):
                if split_sum(items[i][2]) is not None:
                    pick = i
                    break
            i += 1
        if pick != -1:
            inner = items[pick]
            coeff = make_mul(items[:pick] + items[pick + 1 :])
    if inner[0] != "fn" or inner[1] not in ("sin", "cos"):
        return None
    pair = split_sum(inner[2])
    if pair is None:
        return None
    return coeff, inner[1], pair[0], pair[1], pair[2]


def match_shift_target(node, base):
    raw = match_generic_shift_side(node)
    if raw is None:
        return None
    coeff, name, left, right, sign = raw
    names = set()
    collect_symbols(base, names)
    if same(left, base) and independent_of_names(right, names):
        return coeff, name, right, sign
    if sign > 0 and same(right, base) and independent_of_names(left, names):
        return coeff, name, left, sign
    return None


def split_trig_term(node):
    node = sim(node)
    if node[0] == "fn" and node[1] in ("sin", "cos"):
        return num(1), node[1], node[2]
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        pick = -1
        i = 0
        while i < len(items):
            if items[i][0] == "fn" and items[i][1] in ("sin", "cos"):
                if pick != -1:
                    return None
                pick = i
            i += 1
        if pick == -1:
            return None
        coeff_bits = items[:pick] + items[pick + 1 :]
        coeff = make_mul(coeff_bits)
        return coeff, items[pick][1], items[pick][2]
    return None


def match_linear_combo(node):
    node = sim(node)
    terms = [node]
    if node[0] == "add":
        terms = list(flat(node, "add"))
    base = None
    cos_coeff = num(0)
    sin_coeff = num(0)
    seen = False
    i = 0
    while i < len(terms):
        part = split_trig_term(terms[i])
        if part is None:
            return None
        coeff, name, arg = part
        if base is None:
            base = arg
        elif not same(base, arg):
            return None
        if name == "cos":
            cos_coeff = full_simplify(add([cos_coeff, coeff]))
        else:
            sin_coeff = full_simplify(add([sin_coeff, coeff]))
        seen = True
        i += 1
    if not seen:
        return None
    return base, cos_coeff, sin_coeff


def shift_placeholder(name, base, sign):
    base_text = show(base)
    return "R*" + name + "(" + base_text + ("+" if sign > 0 else "-") + "a)"


def show_shift_form(coeff, name, base, shift, sign):
    inner = name + "(" + show(base) + ("+" if sign > 0 else "-") + show(shift) + ")"
    coeff = sim(coeff)
    if is_one(coeff):
        return inner
    if is_minus_one(coeff):
        return "-" + inner
    return show(coeff) + "*" + inner


def bridge_to_target(lines, cur, target, target_name):
    """Append intermediate simplification steps from cur to target before '= RHS'.
    When cur is equivalent to target but looks different, this fills the gap."""
    if same(cur, target):
        lines.append("= " + target_name)
        return
    named, note = proof_named_target_rewrite(cur, target)
    if named is not None and not same(named, cur):
        if note:
            lines.append(note)
        lines.append(step_line(named))
        if same(named, target):
            lines.append("= " + target_name)
            return
        cur = named
    layout = proof_target_layout_steps(cur, target)
    if layout is not None:
        lines.extend(layout)
        lines.append("= " + target_name)
        return
    working = cur
    staged = []
    for transform, note in [
        (lambda e: full_simplify(reduce_identities(e)), "Use Pythagorean identities."),
        (simplify_fraction, "Simplify the fraction."),
        (lambda e: full_simplify(expand_small(e)), "Expand and simplify."),
        (full_simplify, None),
    ]:
        nxt = transform(working)
        if not same(nxt, working):
            if note:
                staged.append(note)
            staged.append(step_line(nxt))
            working = nxt
        if same(working, target):
            lines.extend(staged)
            lines.append("= " + target_name)
            return
    if equivalent(working, target):
        lines.append(step_line(target))
    lines.append("= " + target_name)


def prove_shift_compare(source, target, source_name, target_name):
    combo = match_linear_combo(source)
    shifted = match_shift_side(target)
    if combo is None or shifted is None:
        return None
    base, cos_coeff, sin_coeff = combo
    coeff, name, target_base, shift, sign = shifted
    if not same(base, target_base):
        return None
    if angle_to_degree(shift) is None:
        return None
    try:
        tan_shift = exact_trig_value("tan", shift)
    except Exception:
        return None
    if name == "cos":
        tan_value = full_simplify(div(neg(sin_coeff), cos_coeff)) if sign > 0 else full_simplify(div(sin_coeff, cos_coeff))
    else:
        tan_value = full_simplify(div(cos_coeff, sin_coeff)) if sign > 0 else full_simplify(div(neg(cos_coeff), sin_coeff))
    cos_sq = full_simplify(mul([cos_coeff, cos_coeff]))
    sin_sq = full_simplify(mul([sin_coeff, sin_coeff]))
    r_sq = full_simplify(add([cos_sq, sin_sq]))
    r_val = full_simplify(fn("sqrt", r_sq))
    if not equivalent(tan_value, tan_shift):
        return None
    if not equivalent(r_val, coeff):
        return None

    base_text = show(base)
    shift_text = show(shift)
    target_text = show_shift_form(coeff, name, target_base, shift, sign)
    lines = [start_line(source_name, source), "Let " + source_name + " = " + shift_placeholder(name, base, sign) + "."]

    if name == "cos":
        if sign > 0:
            lines.append("Use cos(A+B) = cos A cos B - sin A sin B.")
            lines.append("= R*cos(" + base_text + ")*cos(a)-R*sin(" + base_text + ")*sin(a)")
            lines.append("Compare cos(" + base_text + "): R*cos(a) = " + show(cos_coeff))
            lines.append("Compare sin(" + base_text + "): -R*sin(a) = " + show(sin_coeff))
        else:
            lines.append("Use cos(A-B) = cos A cos B + sin A sin B.")
            lines.append("= R*cos(" + base_text + ")*cos(a)+R*sin(" + base_text + ")*sin(a)")
            lines.append("Compare cos(" + base_text + "): R*cos(a) = " + show(cos_coeff))
            lines.append("Compare sin(" + base_text + "): R*sin(a) = " + show(sin_coeff))
    else:
        if sign > 0:
            lines.append("Use sin(A+B) = sin A cos B + cos A sin B.")
            lines.append("= R*sin(" + base_text + ")*cos(a)+R*cos(" + base_text + ")*sin(a)")
            lines.append("Compare sin(" + base_text + "): R*cos(a) = " + show(sin_coeff))
            lines.append("Compare cos(" + base_text + "): R*sin(a) = " + show(cos_coeff))
        else:
            lines.append("Use sin(A-B) = sin A cos B - cos A sin B.")
            lines.append("= R*sin(" + base_text + ")*cos(a)-R*cos(" + base_text + ")*sin(a)")
            lines.append("Compare sin(" + base_text + "): R*cos(a) = " + show(sin_coeff))
            lines.append("Compare cos(" + base_text + "): -R*sin(a) = " + show(cos_coeff))

    lines.append("Divide the second equation by the first.")
    lines.append("tan(a) = " + show(tan_value))
    lines.append("a = " + shift_text)
    lines.append("Square and add the two equations.")
    r_sq_text = show(cos_sq) + "+" + show(sin_sq)
    lines.append("R^2 = " + r_sq_text)
    if r_sq_text != show(r_sq):
        lines.append("= " + show(r_sq))
    lines.append("R = " + show(r_val))
    lines.append("Therefore " + source_name + " = " + target_text)
    lines.append("= " + target_name)
    return lines


def prove_shift_identity(source, target, source_name, target_name):
    match = match_shift_side(source)
    if match is None:
        return None
    coeff, name, base, shift, sign = match
    lines = [start_line(source_name, source)]
    arg = add([base, shift]) if sign > 0 else add([base, neg(shift)])
    inner = fn(name, arg)
    expanded, note = trig_formula_expand(inner)
    if expanded is not None and not is_one(coeff):
        expanded = mul([coeff, expanded])
    if expanded is None:
        return None
    lines.append(note)
    expanded = full_simplify(expanded)
    lines.append(step_line(expanded))
    exact_lines = []
    substituted = replace_exact_trig(expanded, exact_lines)
    i = 0
    while i < len(exact_lines):
        lines.append(exact_lines[i])
        i += 1
    substituted = full_simplify(substituted)
    if not same(substituted, expanded):
        lines.append(step_line(substituted))
    if equivalent(substituted, target):
        bridge_to_target(lines, substituted, target, target_name)
        return lines
    # Try one extra double-angle pass for cases like 4cos(2x-pi/6)
    doubled = full_simplify(expand_trig_tree(substituted))
    doubled = full_simplify(doubled)
    if not same(doubled, substituted):
        lines.append("Use double-angle identities.")
        lines.append(step_line(doubled))
    if equivalent(doubled, target):
        bridge_to_target(lines, doubled, target, target_name)
        return lines
    return None


def prove_tan_multiple(source, target, source_name, target_name):
    source = sim(source)
    if source[0] != "fn" or source[1] != "tan":
        return None
    arg = source[2]
    coeff = maybe_scalar_times_var(arg, "x")
    _ = coeff
    expanded, note = trig_formula_expand(source)
    if expanded is None:
        return None
    lines = [start_line(source_name, source)]
    expanded = full_simplify(expanded)
    expanded = detail_trig_expansion(lines, expanded, target, target_name, note)
    cur = expanded
    cur2 = reciprocal_trig(cur)
    cur2 = full_simplify(cur2)
    if not same(cur2, cur):
        lines.append("Write tan and cot in reciprocal form where needed.")
        lines.append(step_line(cur2))
        cur = cur2
    # Special divide-through step for tan(2x)
    if source[1] == "tan":
        base = None
        if arg[0] == "mul":
            items = list(flat(arg, "mul"))
            if len(items) == 2 and is_num(items[0]) and items[0] == num(2):
                base = items[1]
        if base is not None:
            cur = div(num(2), add([div(num(1), fn("tan", base)), neg(fn("tan", base))]))
            lines.append("Divide numerator and denominator by tan(" + show(base) + ").")
            lines.append(step_line(cur))
            cur = div(num(2), add([fn("cot", base), neg(fn("tan", base))]))
            lines.append("Use cot(A) = 1/tan(A).")
            lines.append(step_line(cur))
            if equivalent(cur, target):
                bridge_to_target(lines, cur, target, target_name)
                return lines
    if equivalent(cur, target):
        bridge_to_target(lines, cur, target, target_name)
        return lines
    return None


def product_to_sum_once(node):
    node = sim(node)
    coeff, rest = split_coeff(node)
    items = list(flat(rest, "mul")) if rest[0] == "mul" else [rest]
    if len(items) != 2 or items[0][0] != "fn" or items[1][0] != "fn":
        return None, None
    left = items[0]
    right = items[1]
    scale = mul([coeff, num(1, 2)])
    if left[1] == "cos" and right[1] == "cos":
        sum_arg = expand_negative_add_terms(sim(add([left[2], right[2]])))
        diff_arg = expand_negative_add_terms(sim(add([left[2], neg(right[2])])))
        out = mul([scale, add([fn("cos", sum_arg), fn("cos", diff_arg)])])
        return out, "Use 2cos A cos B = cos(A+B) + cos(A-B)."
    if left[1] == "sin" and right[1] == "sin":
        diff_arg = expand_negative_add_terms(sim(add([left[2], neg(right[2])])))
        sum_arg = expand_negative_add_terms(sim(add([left[2], right[2]])))
        out = mul([scale, add([fn("cos", diff_arg), neg(fn("cos", sum_arg))])])
        return out, "Use 2sin A sin B = cos(A-B) - cos(A+B)."
    if (left[1], right[1]) == ("sin", "cos") or (left[1], right[1]) == ("cos", "sin"):
        sin_arg = left[2] if left[1] == "sin" else right[2]
        cos_arg = left[2] if left[1] == "cos" else right[2]
        sum_arg = expand_negative_add_terms(sim(add([sin_arg, cos_arg])))
        diff_arg = expand_negative_add_terms(sim(add([sin_arg, neg(cos_arg)])))
        out = mul([scale, add([fn("sin", sum_arg), fn("sin", diff_arg)])])
        return out, "Use 2sin A cos B = sin(A+B) + sin(A-B)."
    return None, None


def match_plus_minus_pair(arg1, arg2):
    left = split_sum(sim(arg1))
    right = split_sum(sim(arg2))
    if left is None or right is None or left[2] == right[2]:
        return None
    pairs = (
        (left[0], left[1], right[0], right[1]),
        (left[0], left[1], right[1], right[0]),
    )
    i = 0
    while i < len(pairs):
        a1, b1, a2, b2 = pairs[i]
        if equivalent(a1, a2) and equivalent(b1, b2):
            if left[2] > 0:
                return a1, b1
            return a2, b2
        i += 1
    return None


def match_cos_sq_minus_sin_sq(node):
    node = sim(node)
    if node[0] != "add":
        return None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_minus_one(coeff1) and is_one(coeff2):
        coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
    if not (is_one(coeff1) and is_minus_one(coeff2)):
        return None
    if rest1[0] == "pow" and rest2[0] == "pow" and cheap_same(rest1[2], num(2)) and cheap_same(rest2[2], num(2)):
        if rest1[1][0] == "fn" and rest2[1][0] == "fn" and rest1[1][1] == "cos" and rest2[1][1] == "sin":
            return rest1[1][2], rest2[1][2]
    return None


def match_sin_sq_minus_sin_sq(node):
    node = sim(node)
    if node[0] != "add":
        return None
    bits = list(flat(node, "add"))
    if len(bits) != 2:
        return None
    first = signed_unit_term(bits[0])
    second = signed_unit_term(bits[1])
    if first is None or second is None:
        return None
    coeff1, rest1 = first
    coeff2, rest2 = second
    if is_minus_one(coeff1) and is_one(coeff2):
        coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
    if not (is_one(coeff1) and is_minus_one(coeff2)):
        return None
    if rest1[0] == "pow" and rest2[0] == "pow" and cheap_same(rest1[2], num(2)) and cheap_same(rest2[2], num(2)):
        if rest1[1][0] == "fn" and rest2[1][0] == "fn" and rest1[1][1] == "sin" and rest2[1][1] == "sin":
            return rest1[1][2], rest2[1][2]
    return None


def prove_product_pair_identity(source, target, source_name, target_name):
    expanded, note = product_to_sum_once(source)
    if expanded is None:
        return None
    lines = [start_line(source_name, source), note, step_line(expanded)]
    pair = None
    if source[0] == "mul":
        items = list(flat(source, "mul"))
        if len(items) == 2 and items[0][0] == "fn" and items[1][0] == "fn":
            pair = match_plus_minus_pair(items[0][2], items[1][2])
    target_pair = match_cos_sq_minus_sin_sq(target)
    if pair is not None and target_pair is not None:
        first_base = None
        second_base = None
        if equivalent(target_pair[0], pair[0]) and equivalent(target_pair[1], pair[1]):
            first_base = pair[0]
            second_base = pair[1]
        elif equivalent(target_pair[0], pair[1]) and equivalent(target_pair[1], pair[0]):
            first_base = pair[1]
            second_base = pair[0]
        if first_base is not None:
            step2 = div(add([add([mul([num(2), power(fn("cos", first_base), num(2))]), num(-1)]), fn("cos", mul([num(2), second_base]))]), num(2))
            step3 = div(add([add([mul([num(2), power(fn("cos", first_base), num(2))]), num(-1)]), add([num(1), mul([num(-2), power(fn("sin", second_base), num(2))])])]), num(2))
            step4 = add([power(fn("cos", first_base), num(2)), neg(power(fn("sin", second_base), num(2)))])
            lines.extend([
                "Use cos(2A) = 2cos^2 A - 1.",
                step_line(step2),
                "Use cos(2A) = 1 - 2sin^2 A.",
                step_line(step3),
                "Simplify.",
                step_line(step4),
            ])
            if equivalent(step4, target):
                bridge_to_target(lines, step4, target, target_name)
                return lines
    target_pair = match_sin_sq_minus_sin_sq(target)
    if pair is not None and target_pair is not None:
        first_base = None
        second_base = None
        if equivalent(target_pair[0], pair[0]) and equivalent(target_pair[1], pair[1]):
            first_base = pair[0]
            second_base = pair[1]
        elif equivalent(target_pair[0], pair[1]) and equivalent(target_pair[1], pair[0]):
            first_base = pair[1]
            second_base = pair[0]
        if first_base is not None:
            step2 = div(add([fn("cos", mul([num(2), second_base])), neg(add([num(1), mul([num(-2), power(fn("sin", first_base), num(2))])]))]), num(2))
            step3 = div(add([add([num(1), mul([num(-2), power(fn("sin", second_base), num(2))])]), neg(add([num(1), mul([num(-2), power(fn("sin", first_base), num(2))])]))]), num(2))
            step4 = add([power(fn("sin", first_base), num(2)), neg(power(fn("sin", second_base), num(2)))])
            lines.extend([
                "Use cos(2A) = 1 - 2sin^2 A.",
                step_line(step2),
                "Use cos(2A) = 1 - 2sin^2 A.",
                step_line(step3),
                "Simplify.",
                step_line(step4),
            ])
            if equivalent(step4, target):
                bridge_to_target(lines, step4, target, target_name)
                return lines
    if equivalent(expanded, target):
        bridge_to_target(lines, expanded, target, target_name)
        return lines
    return None


def prove_sum_product_identity(source, target, source_name, target_name):
    direct, note = sum_product_expand(source)
    if direct is None:
        return None
    lines = [start_line(source_name, source)]
    direct = full_simplify(direct)
    direct = detail_trig_expansion(lines, direct, target, target_name, note)
    if equivalent(direct, target):
        bridge_to_target(lines, direct, target, target_name)
        return lines
    return None


def normalize_reciprocal_pair_sum_binomial(data):
    if data is None:
        return None
    first, second, sign = data
    if first[0] == "fn" and first[1] in ("cosec", "sec") and is_one(second):
        return first, sign
    if is_one(first) and second[0] == "fn" and second[1] in ("cosec", "sec") and sign > 0:
        return second, 1
    return None


def reciprocal_pair_sum_working(source):
    pair = split_sum(source)
    if pair is None or pair[2] < 0:
        return None
    left = pair[0]
    right = pair[1]
    if left[0] != "div" or right[0] != "div" or not is_one(left[1]) or not is_one(right[1]):
        return None
    left_bin = normalize_reciprocal_pair_sum_binomial(extract_binomial(left[2]))
    right_bin = normalize_reciprocal_pair_sum_binomial(extract_binomial(right[2]))
    if left_bin is None or right_bin is None:
        return None
    base = None
    if cheap_same(left_bin[0], right_bin[0]) and left_bin[1] == -1 and right_bin[1] == 1:
        base = left_bin[0]
    elif cheap_same(left_bin[0], right_bin[0]) and left_bin[1] == 1 and right_bin[1] == -1:
        left, right = right, left
        base = right_bin[0]
    if base is None or base[0] != "fn" or base[1] not in ("cosec", "sec"):
        return None
    arg = base[2]
    common = div(add([right[2], left[2]]), mul([left[2], right[2]]))
    reduced = div(mul([num(2), base]), add([power(base, num(2)), num(-1)]))
    if base[1] == "cosec":
        step3 = div(mul([num(2), base]), power(fn("cot", arg), num(2)))
        step4 = div(mul([num(2), base, power(fn("sin", arg), num(2))]), power(fn("cos", arg), num(2)))
        step5 = div(mul([num(2), fn("sin", arg)]), power(fn("cos", arg), num(2)))
        final = mul([num(2), fn("tan", arg), fn("sec", arg)])
        id_note = "Use cosec^2 A - 1 = cot^2 A."
        ratio_note = "Use cot(A) = cos(A)/sin(A)."
        reciprocal_note = "Use cosec(A) = 1/sin(A)."
        finish_note = "Rewrite in terms of tan and sec."
    else:
        step3 = div(mul([num(2), base]), power(fn("tan", arg), num(2)))
        step4 = div(mul([num(2), base, power(fn("cos", arg), num(2))]), power(fn("sin", arg), num(2)))
        step5 = div(mul([num(2), fn("cos", arg)]), power(fn("sin", arg), num(2)))
        final = mul([num(2), fn("cot", arg), fn("cosec", arg)])
        id_note = "Use sec^2 A - 1 = tan^2 A."
        ratio_note = "Use tan(A) = sin(A)/cos(A)."
        reciprocal_note = "Use sec(A) = 1/cos(A)."
        finish_note = "Rewrite in terms of cot and cosec."
    lines = [
        "Use a common denominator.",
        step_line(common),
        "Simplify the numerator and denominator.",
        step_line(reduced),
        id_note,
        step_line(step3),
        ratio_note,
        step_line(step4),
        reciprocal_note,
        step_line(step5),
        finish_note,
        step_line(final),
    ]
    return final, compact_lines(lines)


def prove_reciprocal_pair_sum(source, target, source_name, target_name):
    rewritten = reciprocal_pair_sum_working(source)
    if rewritten is None:
        return None
    final, body = rewritten
    if equivalent(final, target):
        lines_out = [start_line(source_name, source)] + body
        bridge_to_target(lines_out, final, target, target_name)
        return lines_out
    return None


def signed_reciprocal_piece_text(sign_value, node):
    text = show(node)
    if sign_value < 0:
        return "-(" + text + ")"
    return text


def match_signed_unit_fraction(node):
    if node[0] == "div" and is_num(node[1]) and (cheap_same(node[1], num(1)) or cheap_same(node[1], num(-1))):
        return 1 if is_one(node[1]) else -1, node[2]
    coeff, rest = split_coeff(node)
    if (cheap_same(coeff, num(1)) or cheap_same(coeff, num(-1))) and rest[0] == "div" and is_one(rest[1]):
        return 1 if is_one(coeff) else -1, rest[2]
    return None


def classify_reciprocal_conjugate_binomial(data):
    if data is None:
        return None
    first, second, sign = data
    if is_one(first) and second[0] == "fn":
        if second[1] == "sin":
            return "one_sin", second[2], sign
        if second[1] == "cos":
            return "one_cos", second[2], sign
        if second[1] == "sec":
            return "sec_one", second[2], sign
        if second[1] == "cosec":
            return "cosec_one", second[2], sign
    if first[0] == "fn" and is_one(second):
        if first[1] == "sin" and sign > 0:
            return "one_sin", first[2], 1
        if first[1] == "cos" and sign > 0:
            return "one_cos", first[2], 1
        if first[1] == "sec":
            return "sec_one", first[2], sign
        if first[1] == "cosec":
            return "cosec_one", first[2], sign
    if first[0] == "fn" and second[0] == "fn" and cheap_same(first[2], second[2]):
        if first[1] == "sec" and second[1] == "tan":
            return "sec_tan", first[2], sign
        if first[1] == "tan" and second[1] == "sec" and sign > 0:
            return "sec_tan", first[2], 1
        if first[1] == "cosec" and second[1] == "cot":
            return "cosec_cot", first[2], sign
        if first[1] == "cot" and second[1] == "cosec" and sign > 0:
            return "cosec_cot", first[2], 1
    return None


def reciprocal_conjugate_pair_working(source):
    source = sim(source)
    terms = list(flat(source, "add")) if source[0] == "add" else [source]
    if len(terms) != 2:
        return None
    left = match_signed_unit_fraction(terms[0])
    right = match_signed_unit_fraction(terms[1])
    if left is None or right is None:
        return None
    left_sign, left_den = left
    right_sign, right_den = right
    left_kind = classify_reciprocal_conjugate_binomial(extract_binomial(left_den))
    right_kind = classify_reciprocal_conjugate_binomial(extract_binomial(right_den))
    if left_kind is None or right_kind is None:
        return None
    if left_kind[0] != right_kind[0] or not cheap_same(left_kind[1], right_kind[1]):
        return None
    if set([left_kind[2], right_kind[2]]) != set([-1, 1]):
        return None
    family = left_kind[0]
    arg = left_kind[1]
    raw_common_num = add([mul([num(left_sign), right_den]), mul([num(right_sign), left_den])])
    common_num = full_simplify(raw_common_num)
    common_num_text = signed_reciprocal_piece_text(left_sign, right_den)
    right_piece_text = signed_reciprocal_piece_text(right_sign, left_den)
    if right_piece_text.startswith("-"):
        common_num_text += right_piece_text
    else:
        common_num_text += "+" + right_piece_text
    common_den_text = "(" + show(left_den) + ")*(" + show(right_den) + ")"
    lines = [
        "Use a common denominator.",
        "= (" + common_num_text + ")/(" + common_den_text + ")",
    ]
    sum_like = False
    diff_like = False
    if family == "one_sin":
        sum_like = equivalent(common_num, num(2))
        diff_like = equivalent(common_num, mul([num(2), fn("sin", arg)]))
    elif family == "one_cos":
        sum_like = equivalent(common_num, num(2))
        diff_like = equivalent(common_num, mul([num(2), fn("cos", arg)]))
    elif family == "sec_tan":
        sum_like = equivalent(common_num, mul([num(2), fn("sec", arg)]))
        diff_like = equivalent(common_num, mul([num(2), fn("tan", arg)]))
    elif family == "cosec_cot":
        sum_like = equivalent(common_num, mul([num(2), fn("cosec", arg)]))
        diff_like = equivalent(common_num, mul([num(2), fn("cot", arg)]))
    else:
        return None
    if family == "one_sin":
        if sum_like:
            step2 = div(num(2), add([num(1), neg(power(fn("sin", arg), num(2)))]))
            step3 = div(num(2), power(fn("cos", arg), num(2)))
            final = mul([num(2), power(fn("sec", arg), num(2))])
        elif diff_like:
            step2 = div(mul([num(2), fn("sin", arg)]), add([num(1), neg(power(fn("sin", arg), num(2)))]))
            step3 = div(mul([num(2), fn("sin", arg)]), power(fn("cos", arg), num(2)))
            final = mul([num(2), fn("tan", arg), fn("sec", arg)])
        else:
            return None
        lines.extend([
            "Use (A+B)(A-B) = A^2 - B^2.",
            step_line(step2),
            "Use 1-sin^2 A = cos^2 A.",
            step_line(step3),
        ])
        if diff_like:
            lines.extend([
                "Rewrite in terms of tan and sec.",
                step_line(final),
            ])
        else:
            lines.append("Rewrite 1/cos^2 A as sec^2 A.")
            lines.append(step_line(final))
    elif family == "one_cos":
        if sum_like:
            step2 = div(num(2), add([num(1), neg(power(fn("cos", arg), num(2)))]))
            step3 = div(num(2), power(fn("sin", arg), num(2)))
            final = mul([num(2), power(fn("cosec", arg), num(2))])
        elif diff_like:
            step2 = div(mul([num(2), fn("cos", arg)]), add([num(1), neg(power(fn("cos", arg), num(2)))]))
            step3 = div(mul([num(2), fn("cos", arg)]), power(fn("sin", arg), num(2)))
            final = mul([num(2), fn("cot", arg), fn("cosec", arg)])
        else:
            return None
        lines.extend([
            "Use (A+B)(A-B) = A^2 - B^2.",
            step_line(step2),
            "Use 1-cos^2 A = sin^2 A.",
            step_line(step3),
        ])
        if diff_like:
            lines.extend([
                "Rewrite in terms of cot and cosec.",
                step_line(final),
            ])
        else:
            lines.append("Rewrite 1/sin^2 A as cosec^2 A.")
            lines.append(step_line(final))
    elif family == "sec_tan":
        if sum_like:
            numer = mul([num(2), fn("sec", arg)])
        elif diff_like:
            numer = mul([num(2), fn("tan", arg)])
        else:
            return None
        step2 = div(numer, add([power(fn("sec", arg), num(2)), neg(power(fn("tan", arg), num(2)))]))
        final = numer
        lines.extend([
            "Use (A+B)(A-B) = A^2 - B^2.",
            step_line(step2),
            "Use sec^2 A - tan^2 A = 1.",
            step_line(final),
        ])
    elif family == "cosec_cot":
        if sum_like:
            numer = mul([num(2), fn("cosec", arg)])
        elif diff_like:
            numer = mul([num(2), fn("cot", arg)])
        else:
            return None
        step2 = div(numer, add([power(fn("cosec", arg), num(2)), neg(power(fn("cot", arg), num(2)))]))
        final = numer
        lines.extend([
            "Use (A+B)(A-B) = A^2 - B^2.",
            step_line(step2),
            "Use cosec^2 A - cot^2 A = 1.",
            step_line(final),
        ])
    else:
        return None
    return final, compact_lines(lines)


def prove_reciprocal_conjugate_pair(source, target, source_name, target_name):
    rewritten = reciprocal_conjugate_pair_working(source)
    if rewritten is None:
        return None
    final, body = rewritten
    if equivalent(final, target):
        lines_out = [start_line(source_name, source)] + body
        bridge_to_target(lines_out, final, target, target_name)
        return lines_out
    return None


def prove_conjugate_ratio_identity(source, target, source_name, target_name):
    source = sim(source)
    target = sim(target)
    if source[0] != "div":
        return None
    top = source[1]
    bot = source[2]
    top_sin = match_simple_trig_term_norm(top, "sin")
    top_cos = match_simple_trig_term_norm(top, "cos")
    top_cos_pm = match_one_pm_trig_norm(top, "cos")
    bot_cos_pm = match_one_pm_trig_norm(bot, "cos")
    bot_sin_pm = match_one_pm_trig_norm(bot, "sin")
    arg = None
    lines = [start_line(source_name, source)]

    if top_sin is not None and is_one(top_sin[0]) and bot_cos_pm is not None and cheap_same(top_sin[1], bot_cos_pm[1]) and bot_cos_pm[0] > 0:
        arg = top_sin[1]
        conj = add([num(1), neg(fn("cos", arg))])
        step1_text = "= (" + show(fn("sin", arg)) + "*(" + show(conj) + "))/((1+cos(" + show(arg) + "))*(1-cos(" + show(arg) + ")))"
        step2_raw = ("div", make_display_mul([fn("sin", arg), conj]), power(fn("sin", arg), num(2)))
        step3_raw = ("div", conj, fn("sin", arg))
        step3 = div(conj, fn("sin", arg))
        lines.extend([
            "Multiply top and bottom by 1-cos(A).",
            step1_text,
            "Use 1-cos^2 A = sin^2 A.",
            raw_step_line(step2_raw),
            raw_step_line(step3_raw),
        ])
        if equivalent(step3, target):
            bridge_to_target(lines, step3, target, target_name)
            return lines
        return None

    if top_cos_pm is not None and top_cos_pm[0] < 0 and top_sin is None and bot[0] == "fn" and bot[1] == "sin" and cheap_same(top_cos_pm[1], bot[2]):
        arg = bot[2]
        conj = add([num(1), fn("cos", arg)])
        step1_text = "= ((" + show(top) + ")*(" + show(conj) + "))/(" + show(fn("sin", arg)) + "*(" + show(conj) + "))"
        step2_raw = ("div", add([num(1), neg(power(fn("cos", arg), num(2)))]), make_display_mul([fn("sin", arg), conj]))
        step3_raw = ("div", power(fn("sin", arg), num(2)), make_display_mul([fn("sin", arg), conj]))
        step4_raw = ("div", fn("sin", arg), conj)
        step4 = div(fn("sin", arg), conj)
        lines.extend([
            "Multiply top and bottom by 1+cos(A).",
            step1_text,
            "Use 1-cos^2 A = sin^2 A.",
            raw_step_line(step2_raw),
            raw_step_line(step4_raw) if cheap_same(step3_raw, step4_raw) else raw_step_line(step3_raw),
        ])
        if not cheap_same(step3_raw, step4_raw):
            lines.append(raw_step_line(step4_raw))
        if equivalent(step4, target):
            bridge_to_target(lines, step4, target, target_name)
            return lines
        return None

    if top_cos is not None and is_one(top_cos[0]) and bot_sin_pm is not None and cheap_same(top_cos[1], bot_sin_pm[1]) and bot_sin_pm[0] < 0:
        arg = top_cos[1]
        conj = add([num(1), fn("sin", arg)])
        step1_text = "= (" + show(fn("cos", arg)) + "*(" + show(conj) + "))/((1-sin(" + show(arg) + "))*(1+sin(" + show(arg) + ")))"
        step2_raw = ("div", make_display_mul([fn("cos", arg), conj]), power(fn("cos", arg), num(2)))
        step3_raw = ("div", conj, fn("cos", arg))
        step3 = div(conj, fn("cos", arg))
        step4 = add([fn("sec", arg), fn("tan", arg)])
        lines.extend([
            "Multiply top and bottom by 1+sin(A).",
            step1_text,
            "Use 1-sin^2 A = cos^2 A.",
            raw_step_line(step2_raw),
            raw_step_line(step3_raw),
            "Rewrite in terms of sec and tan.",
            step_line(step4),
        ])
        if equivalent(step4, target):
            bridge_to_target(lines, step4, target, target_name)
            return lines
        if equivalent(step3, target):
            bridge_to_target(lines, step3, target, target_name)
            return lines
        return None
    return None


def prove_reciprocal_identity(source, target, source_name, target_name):
    source = sim(source)
    target = sim(target)
    if source[0] == "fn" and source[1] == "sec" and target[0] == "div":
        if is_one(target[1]) and target[2][0] == "fn" and target[2][1] == "cos" and same(source[2], target[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use sec A = 1 / cos A.", step_line(target), "= " + target_name]
    if target[0] == "fn" and target[1] == "sec" and source[0] == "div":
        if is_one(source[1]) and source[2][0] == "fn" and source[2][1] == "cos" and same(target[2], source[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use 1 / cos A = sec A.", step_line(target), "= " + target_name]
    if source[0] == "fn" and source[1] == "cosec" and target[0] == "div":
        if is_one(target[1]) and target[2][0] == "fn" and target[2][1] == "sin" and same(source[2], target[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use cosec A = 1 / sin A.", step_line(target), "= " + target_name]
    if target[0] == "fn" and target[1] == "cosec" and source[0] == "div":
        if is_one(source[1]) and source[2][0] == "fn" and source[2][1] == "sin" and same(target[2], source[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use 1 / sin A = cosec A.", step_line(target), "= " + target_name]
    if source[0] == "fn" and source[1] == "cot" and target[0] == "div":
        if target[1][0] == "fn" and target[1][1] == "cos" and target[2][0] == "fn" and target[2][1] == "sin" and same(source[2], target[1][2]) and same(source[2], target[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use cot A = cos A / sin A.", step_line(target), "= " + target_name]
    if target[0] == "fn" and target[1] == "cot" and source[0] == "div":
        if source[1][0] == "fn" and source[1][1] == "cos" and source[2][0] == "fn" and source[2][1] == "sin" and same(target[2], source[1][2]) and same(target[2], source[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use cos A / sin A = cot A.", step_line(target), "= " + target_name]
    if source[0] == "fn" and source[1] == "tan" and target[0] == "div":
        if target[1][0] == "fn" and target[1][1] == "sin" and target[2][0] == "fn" and target[2][1] == "cos" and same(source[2], target[1][2]) and same(source[2], target[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use tan A = sin A / cos A.", step_line(target), "= " + target_name]
    if target[0] == "fn" and target[1] == "tan" and source[0] == "div":
        if source[1][0] == "fn" and source[1][1] == "sin" and source[2][0] == "fn" and source[2][1] == "cos" and same(target[2], source[1][2]) and same(target[2], source[2][2]) and equivalent(source, target):
            return [start_line(source_name, source), "Use sin A / cos A = tan A.", step_line(target), "= " + target_name]
    if source[0] == "fn" and source[1] == "cos" and target[0] == "fn" and target[1] == "cos":
        if same(target[2], neg(source[2])) and equivalent(source, target):
            return [start_line(source_name, source), "Use cos(-A) = cos A.", step_line(target), "= " + target_name]
    if source[0] == "fn" and source[1] == "sin" and target[0] == "fn" and target[1] == "sin":
        if same(target[2], neg(source[2])) and equivalent(source, target):
            return [start_line(source_name, source), "Use sin(-A) = -sin A.", step_line(neg(target)), "= " + target_name]
    if source[0] == "div" and target[0] == "fn" and target[1] == "tan":
        top_pm = match_one_pm_cos(source[1])
        bot_sin = match_simple_trig_term_norm(source[2], "sin")
        if top_pm is not None and top_pm[0] < 0 and bot_sin is not None and is_one(bot_sin[0]) and same_double_angle_norm(top_pm[1], target[2]) and same_double_angle_norm(bot_sin[1], target[2]):
            return None
    lines = prove_conjugate_ratio_identity(source, target, source_name, target_name)
    if lines is not None:
        return lines
    direct, note = special_trig_identity_once(source)
    if direct is not None and note is not None:
        lines = [start_line(source_name, source), note, step_line(direct)]
        if equivalent(direct, target):
            bridge_to_target(lines, direct, target, target_name)
            return lines
    factored, note = factor_difference_once(source)
    if factored is not None:
        lines = [start_line(source_name, source), note, step_line(factored)]
        reduced, note2 = special_trig_identity_once(factored)
        if reduced is not None and note2 is not None:
            lines.append(note2)
            lines.append(step_line(reduced))
            final = full_simplify(reduced)
            if not same(final, reduced):
                lines.append("Simplify.")
                lines.append(step_line(final))
            if equivalent(final, target):
                bridge_to_target(lines, final, target, target_name)
                return lines
        if factored[0] == "mul":
            items = list(flat(factored, "mul"))
            if len(items) == 2:
                left_reduced, note_left = special_trig_identity_once(items[0])
                if left_reduced is not None and note_left is not None:
                    cur = mul([left_reduced, items[1]])
                    lines.append(note_left)
                    lines.append(step_line(cur))
                    final = full_simplify(cur)
                    if not same(final, cur):
                        lines.append("Simplify.")
                        lines.append(step_line(final))
                    if equivalent(final, target):
                        bridge_to_target(lines, final, target, target_name)
                        return lines
                right_reduced, note_right = special_trig_identity_once(items[1])
                if right_reduced is not None and note_right is not None:
                    cur = mul([items[0], right_reduced])
                    lines.append(note_right)
                    lines.append(step_line(cur))
                    final = full_simplify(cur)
                    if not same(final, cur):
                        lines.append("Simplify.")
                        lines.append(step_line(final))
                    if equivalent(final, target):
                        bridge_to_target(lines, final, target, target_name)
                        return lines
    pair = split_sum(source)
    if pair is not None:
        pair_lines = prove_reciprocal_pair_sum(source, target, source_name, target_name)
        if pair_lines is not None:
            return pair_lines
        pair_lines = prove_reciprocal_conjugate_pair(source, target, source_name, target_name)
        if pair_lines is not None:
            return pair_lines
        left, right, sign = pair
        if sign > 0 and left[0] == "fn" and right[0] == "fn" and same(left[2], right[2]):
            if set([left[1], right[1]]) == set(["tan", "cot"]):
                arg = left[2]
                s = fn("sin", arg)
                c = fn("cos", arg)
                step1 = add([div(s, c), div(c, s)])
                step2 = div(add([power(s, num(2)), power(c, num(2))]), mul([s, c]))
                step3 = div(num(1), mul([s, c]))
                step4 = mul([fn("sec", arg), fn("cosec", arg)])
                lines = [
                    start_line(source_name, source),
                    "Write tan and cot in terms of sin and cos.",
                    step_line(step1),
                    "Use a common denominator.",
                    step_line(step2),
                    "Use sin^2 A + cos^2 A = 1.",
                    step_line(step3),
                    "Rewrite 1/(sin A cos A) as cosec A sec A.",
                    step_line(step4),
                ]
                if equivalent(step4, target):
                    bridge_to_target(lines, step4, target, target_name)
                    return lines
    if source[0] == "div" and is_one(source[1]):
        inner = extract_binomial(source[2])
        if inner is not None and inner[2] < 0:
            left, right, _ = inner
            const_value, note = sec_cosec_pair_value(left, right)
            if const_value is not None:
                conj = add([left, right])
                step1_text = "= (" + show(conj) + ")/((" + show(add([left, neg(right)])) + ")*(" + show(conj) + "))"
                step2 = div(conj, add([power(left, num(2)), neg(power(right, num(2)))]))
                step3 = div(conj, const_value)
                step4 = full_simplify(step3)
                lines = [
                    start_line(source_name, source),
                    "Multiply top and bottom by the conjugate.",
                    step1_text,
                    "Use (A-B)(A+B) = A^2 - B^2.",
                    step_line(step2),
                    note,
                    step_line(step3),
                ]
                if equivalent(step3, target):
                    bridge_to_target(lines, step3, target, target_name)
                    return lines
                if not same(step4, step3):
                    lines.append("Simplify.")
                    lines.append(step_line(step4))
                if equivalent(step4, target):
                    bridge_to_target(lines, step4, target, target_name)
                    return lines
    return None


def match_reciprocal_minus_base_proof(node, reciprocal_name, base_name):
    node = sim(node)
    terms = list(flat(node, "add")) if node[0] == "add" else [node]
    reciprocal_term = None
    base_term = None
    i = 0
    while i < len(terms):
        part = match_simple_trig_term_norm(terms[i], reciprocal_name)
        if part is not None:
            if reciprocal_term is not None:
                return None
            reciprocal_term = part
            i += 1
            continue
        part = match_simple_trig_term_norm(terms[i], base_name)
        if part is not None:
            if base_term is not None:
                return None
            base_term = part
            i += 1
            continue
        if not is_zero(terms[i]):
            return None
        i += 1
    if reciprocal_term is None or base_term is None:
        return None
    if not cheap_same(reciprocal_term[1], base_term[1]):
        return None
    if not equivalent(reciprocal_term[0], neg(base_term[0])):
        return None
    return reciprocal_term[0], reciprocal_term[1]


def match_cosec_cot_double_proof(node):
    node = sim(node)
    terms = list(flat(node, "add")) if node[0] == "add" else [node]
    cosec_term = None
    cos_cot_term = None
    i = 0
    while i < len(terms):
        part = match_simple_trig_term_norm(terms[i], "cosec")
        if part is not None:
            if cosec_term is not None:
                return None
            cosec_term = part
            i += 1
            continue
        part = match_cos_times_cot(terms[i])
        if part is not None:
            if cos_cot_term is not None:
                return None
            cos_cot_term = part
            i += 1
            continue
        if not is_zero(terms[i]):
            return None
        i += 1
    if cosec_term is None or cos_cot_term is None:
        return None
    coeff, base = cosec_term
    cot_coeff, cos_arg, cot_arg = cos_cot_term
    if not cheap_same(base, cos_arg):
        return None
    if not same_double_angle_norm(cot_arg, base):
        return None
    if not equivalent(cot_coeff, neg(mul([num(2), coeff]))):
        return None
    return coeff, base, cot_coeff, cot_arg


def prove_reciprocal_reduction_identity(source, target, source_name, target_name):
    source = sim(source)
    target = sim(target)

    match = match_reciprocal_minus_base_proof(source, "cosec", "sin")
    if match is not None:
        coeff, arg = match
        product = sim(mul([coeff, fn("cos", arg), fn("cot", arg)]))
        if equivalent(product, target):
            step1 = add([div(coeff, fn("sin", arg)), neg(mul([coeff, fn("sin", arg)]))])
            step2 = div(add([coeff, neg(mul([coeff, power(fn("sin", arg), num(2))]))]), fn("sin", arg))
            step3 = div(mul([coeff, power(fn("cos", arg), num(2))]), fn("sin", arg))
            lines = [
                start_line(source_name, source),
                "Use cosec(A) = 1/sin(A).",
                raw_step_line(step1),
                "Use a common denominator.",
                raw_step_line(step2),
                "Use 1-sin^2 A = cos^2 A.",
                raw_step_line(step3),
                "Rewrite cos^2(A)/sin(A) as cos(A)cot(A).",
                step_line(product),
                "= " + target_name,
            ]
            return lines

    match = match_reciprocal_minus_base_proof(source, "sec", "cos")
    if match is not None:
        coeff, arg = match
        product = sim(mul([coeff, fn("sin", arg), fn("tan", arg)]))
        if equivalent(product, target):
            step1 = add([div(coeff, fn("cos", arg)), neg(mul([coeff, fn("cos", arg)]))])
            step2 = div(add([coeff, neg(mul([coeff, power(fn("cos", arg), num(2))]))]), fn("cos", arg))
            step3 = div(mul([coeff, power(fn("sin", arg), num(2))]), fn("cos", arg))
            lines = [
                start_line(source_name, source),
                "Use sec(A) = 1/cos(A).",
                raw_step_line(step1),
                "Use a common denominator.",
                raw_step_line(step2),
                "Use 1-cos^2 A = sin^2 A.",
                raw_step_line(step3),
                "Rewrite sin^2(A)/cos(A) as sin(A)tan(A).",
                step_line(product),
                "= " + target_name,
            ]
            return lines

    match = match_cosec_cot_double_proof(source)
    if match is not None:
        coeff, arg, cot_coeff, cot_arg = match
        final = sim(mul([num(2), coeff, fn("sin", arg)]))
        if equivalent(final, target):
            left_frac = show(div(coeff, fn("sin", arg)))
            raw_num = make_display_mul([cot_coeff, fn("cos", arg), fn("cos", cot_arg)])
            raw_den = make_display_mul([num(2), fn("sin", arg), fn("cos", arg)])
            raw_num_text = show(display_abs(raw_num))
            raw_den_text = show(raw_den)
            if raw_num[0] in ("add", "mul", "div"):
                raw_num_text = "(" + raw_num_text + ")"
            if raw_den[0] in ("add", "mul", "div"):
                raw_den_text = "(" + raw_den_text + ")"
            step2_text = "= " + left_frac + ("-" if display_neg(raw_num) else "+") + raw_num_text + "/" + raw_den_text
            top = mul([num(2), coeff, power(fn("sin", arg), num(2))])
            top_text = show(top)
            den_text = show(fn("sin", arg))
            if top[0] in ("add", "mul", "div"):
                top_text = "(" + top_text + ")"
            step1 = add([
                div(coeff, fn("sin", arg)),
                div(make_display_mul([cot_coeff, fn("cos", arg), fn("cos", cot_arg)]), fn("sin", cot_arg)),
            ])
            step2 = add([
                div(coeff, fn("sin", arg)),
                div(
                    make_display_mul([cot_coeff, fn("cos", arg), fn("cos", cot_arg)]),
                    make_display_mul([num(2), fn("sin", arg), fn("cos", arg)]),
                ),
            ])
            step3 = add([
                div(coeff, fn("sin", arg)),
                div(neg(mul([coeff, fn("cos", cot_arg)])), fn("sin", arg)),
            ])
            step4 = div(add([coeff, neg(mul([coeff, fn("cos", cot_arg)]))]), fn("sin", arg))
            lines = [
                start_line(source_name, source),
                "Use cosec(A) = 1/sin(A) and cot(A) = cos(A)/sin(A).",
                raw_step_line(step1),
                "Use sin(2A) = 2sin A cos A.",
                step2_text,
                raw_step_line(step3),
                "Use a common denominator.",
                raw_step_line(step4),
                "Use 1-cos(2A) = 2sin^2 A.",
                "= " + top_text + "/" + den_text,
                step_line(final),
                "= " + target_name,
            ]
            return lines

    return None


def prove_log_exp_identity(source, target, source_name, target_name):
    source = sim(source)
    if source[0] == "fn" and source[1] == "exp":
        step1 = power(E, source[2])
        lines = [start_line(source_name, source), "Use exp(A) = e^A.", step_line(step1)]
        if equivalent(step1, target):
            bridge_to_target(lines, step1, target, target_name)
            return lines
    if source[0] == "fn" and source[1] == "log":
        inner = source[2]
        if inner[0] == "fn" and inner[1] == "exp":
            step1 = fn("log", power(E, inner[2]))
            step2 = inner[2]
            lines = [
                start_line(source_name, source),
                "Use exp(A) = e^A.",
                step_line(step1),
                "Use ln(e^A) = A.",
                step_line(step2),
            ]
            if equivalent(step2, target):
                bridge_to_target(lines, step2, target, target_name)
                return lines
        if inner[0] == "pow" and same(inner[1], E):
            step1 = inner[2]
            lines = [start_line(source_name, source), "Use ln(e^A) = A.", step_line(step1)]
            if equivalent(step1, target):
                bridge_to_target(lines, step1, target, target_name)
                return lines
        if inner[0] == "mul":
            items = list(flat(inner, "mul"))
            if len(items) == 2:
                step1 = add([fn("log", items[0]), fn("log", items[1])])
                lines = [start_line(source_name, source), "Use ln(AB) = ln A + ln B.", step_line(step1)]
                if equivalent(step1, target):
                    bridge_to_target(lines, step1, target, target_name)
                    return lines
        if inner[0] == "div":
            step1 = add([fn("log", inner[1]), neg(fn("log", inner[2]))])
            lines = [start_line(source_name, source), "Use ln(A/B) = ln A - ln B.", step_line(step1)]
            if equivalent(step1, target):
                bridge_to_target(lines, step1, target, target_name)
                return lines
        if inner[0] == "pow":
            step1 = mul([inner[2], fn("log", inner[1])])
            lines = [start_line(source_name, source), "Use ln(A^n) = n ln A.", step_line(step1)]
            if equivalent(step1, target):
                bridge_to_target(lines, step1, target, target_name)
                return lines
    if source[0] == "pow" and same(source[1], E):
        exp_arg = source[2]
        if exp_arg[0] == "mul":
            items = list(flat(exp_arg, "mul"))
            log_pick = -1
            i = 0
            while i < len(items):
                if items[i][0] == "fn" and items[i][1] == "log":
                    log_pick = i
                    break
                i += 1
            if log_pick != -1:
                base = items[log_pick][2]
                exp_bits = items[:log_pick] + items[log_pick + 1 :]
                if len(exp_bits) != 0:
                    exp_val = make_mul(exp_bits)
                    step1 = power(base, exp_val)
                    lines = [start_line(source_name, source), FORMULA_EXP_BASE_CHANGE, step_line(step1)]
                    if equivalent(step1, target):
                        bridge_to_target(lines, step1, target, target_name)
                        return lines
        if exp_arg[0] == "fn" and exp_arg[1] == "log":
            step1 = exp_arg[2]
            lines = [start_line(source_name, source), "Use e^(ln A) = A.", step_line(step1)]
            if equivalent(step1, target):
                bridge_to_target(lines, step1, target, target_name)
                return lines
        pair = split_sum(exp_arg)
        if pair is not None and pair[2] > 0:
            left = pair[0]
            right = pair[1]
            step1_text = "= e^(" + show(left) + ")*e^(" + show(right) + ")"
            step2 = full_simplify(("mul", (("pow", E, left), ("pow", E, right))))
            lines = [start_line(source_name, source), "Use e^(A+B) = e^A * e^B.", step1_text]
            if clean_expr_text(step1_text[2:]) != clean_expr_text(show(step2)):
                lines.append("Use e^(ln A) = A where possible.")
                lines.append(step_line(step2))
            if equivalent(step2, target):
                bridge_to_target(lines, step2, target, target_name)
                return lines
    return None


def prove_power_identity(source, target, source_name, target_name):
    source = sim(source)
    if source[0] == "add":
        bits = list(flat(source, "add"))
        if len(bits) == 2:
            first = signed_unit_term(bits[0])
            second = signed_unit_term(bits[1])
            if first is not None and second is not None and is_one(first[0]) and is_one(second[0]):
                left = first[1]
                right = second[1]
                if left[0] == "pow" and right[0] == "pow" and left[1][0] == "fn" and right[1][0] == "fn" and same(left[1][2], right[1][2]):
                    names = set([left[1][1], right[1][1]])
                    if names == set(["sin", "cos"]) and same(left[2], right[2]):
                        arg = left[1][2]
                        exp = left[2]
                        s2 = power(fn("sin", arg), num(2))
                        c2 = power(fn("cos", arg), num(2))
                        u = mul([s2, c2])
                        if same(exp, num(4)):
                            step1 = add([power(add([s2, c2]), num(2)), neg(mul([num(2), u]))])
                            step2 = add([num(1), neg(mul([num(2), u]))])
                            step3 = add([num(1), neg(mul([num(1, 2), power(fn("sin", mul([num(2), arg])), num(2))]))])
                            lines = [
                                start_line(source_name, source),
                                "Use a^2 + b^2 = (a+b)^2 - 2ab with a = sin^2 x and b = cos^2 x.",
                                step_line(step1),
                                "Use sin^2 x + cos^2 x = 1.",
                                step_line(step2),
                                "Use sin(2x) = 2sin(x)cos(x).",
                                step_line(step3),
                            ]
                            if equivalent(step3, target):
                                bridge_to_target(lines, step3, target, target_name)
                                return lines
                        if same(exp, num(6)):
                            step1 = add([power(add([s2, c2]), num(3)), neg(mul([num(3), u, add([s2, c2])]))])
                            step2 = add([num(1), neg(mul([num(3), u]))])
                            step3 = add([num(1), neg(mul([num(3, 4), power(fn("sin", mul([num(2), arg])), num(2))]))])
                            lines = [
                                start_line(source_name, source),
                                "Use a^3 + b^3 = (a+b)^3 - 3ab(a+b) with a = sin^2 x and b = cos^2 x.",
                                step_line(step1),
                                "Use sin^2 x + cos^2 x = 1.",
                                step_line(step2),
                                "Use sin(2x) = 2sin(x)cos(x).",
                                step_line(step3),
                            ]
                            if equivalent(step3, target):
                                bridge_to_target(lines, step3, target, target_name)
                                return lines
                        if same(exp, num(8)):
                            s4 = power(fn("sin", arg), num(4))
                            c4 = power(fn("cos", arg), num(4))
                            step1 = add([power(add([s4, c4]), num(2)), neg(mul([num(2), power(u, num(2))]))])
                            step2 = add([power(add([num(1), neg(mul([num(2), u]))]), num(2)), neg(mul([num(2), power(u, num(2))]))])
                            step3 = add([num(1), neg(mul([num(4), u])), mul([num(2), power(u, num(2))])])
                            step4 = add([num(1), neg(power(fn("sin", mul([num(2), arg])), num(2))), mul([num(1, 8), power(fn("sin", mul([num(2), arg])), num(4))])])
                            lines = [
                                start_line(source_name, source),
                                "Use a^2 + b^2 = (a+b)^2 - 2ab with a = sin^4 x and b = cos^4 x.",
                                step_line(step1),
                                "Use sin^4 x + cos^4 x = 1 - 2sin^2 x cos^2 x.",
                                step_line(step2),
                                "Expand and simplify.",
                                step_line(step3),
                                "Use sin(2x) = 2sin(x)cos(x).",
                                step_line(step4),
                            ]
                            if equivalent(step4, target):
                                bridge_to_target(lines, step4, target, target_name)
                                return lines
    if source[0] == "mul":
        items = list(flat(source, "mul"))
        if len(items) == 2 and is_num(items[0]) and items[0] == num(2) and items[1][0] == "pow" and same(items[1][2], num(2)) and items[1][1][0] == "fn":
            trig_fn = items[1][1][1]
            arg = items[1][1][2]
            if trig_fn == "cos":
                first = add([num(1), fn("cos", mul([num(2), arg]))])
                lines = [start_line(source_name, source), "Use 2cos^2 A = 1 + cos 2A.", step_line(first)]
                if equivalent(first, target):
                    bridge_to_target(lines, first, target, target_name)
                    return lines
            if trig_fn == "sin":
                first = add([num(1), neg(fn("cos", mul([num(2), arg])))])
                lines = [start_line(source_name, source), "Use 2sin^2 A = 1 - cos 2A.", step_line(first)]
                if equivalent(first, target):
                    bridge_to_target(lines, first, target, target_name)
                    return lines
    if source[0] == "pow" and source[1][0] == "fn" and source[1][1] == "sin" and same(source[2], num(4)):
        arg = source[1][2]
        step1 = power(power(fn("sin", arg), num(2)), num(2))
        step2 = power(div(add([num(1), neg(fn("cos", mul([num(2), arg])))]), num(2)), num(2))
        cos2 = fn("cos", mul([num(2), arg]))
        step3 = div(add([num(1), neg(mul([num(2), cos2])), power(cos2, num(2))]), num(4))
        step4 = div(add([num(1), neg(mul([num(2), cos2])), div(add([num(1), fn("cos", mul([num(4), arg]))]), num(2))]), num(4))
        cos4 = fn("cos", mul([num(4), arg]))
        step5 = sim(div(add([num(3), neg(mul([num(4), cos2])), cos4]), num(8)))
        lines = [
            start_line(source_name, source),
            "Rewrite sin^4 A as (sin^2 A)^2.",
            step_line(step1),
            "Use sin^2 A = (1-cos 2A)/2.",
            step_line(step2),
            "Expand the square.",
            step_line(step3),
            "Use cos^2(2A) = (1+cos 4A)/2.",
            step_line(step4),
        ]
        if not same(step5, step4):
            lines.append("Simplify.")
            lines.append(step_line(step5))
        if equivalent(step5, target):
            bridge_to_target(lines, step5, target, target_name)
            return lines
    if source[0] == "pow" and source[1][0] == "fn" and source[1][1] == "cos" and same(source[2], num(4)):
        arg = source[1][2]
        step1 = power(power(fn("cos", arg), num(2)), num(2))
        step2 = power(div(add([num(1), fn("cos", mul([num(2), arg]))]), num(2)), num(2))
        cos2 = fn("cos", mul([num(2), arg]))
        step3 = div(add([num(1), mul([num(2), cos2]), power(cos2, num(2))]), num(4))
        step4 = div(add([num(1), mul([num(2), cos2]), div(add([num(1), fn("cos", mul([num(4), arg]))]), num(2))]), num(4))
        cos4 = fn("cos", mul([num(4), arg]))
        step5 = sim(div(add([num(3), mul([num(4), cos2]), cos4]), num(8)))
        lines = [
            start_line(source_name, source),
            "Rewrite cos^4 A as (cos^2 A)^2.",
            step_line(step1),
            "Use cos^2 A = (1+cos 2A)/2.",
            step_line(step2),
            "Expand the square.",
            step_line(step3),
            "Use cos^2 A = (1+cos 2A)/2 again on cos^2(2A).",
            step_line(step4),
        ]
        if not same(step5, step4):
            lines.append("Simplify.")
            lines.append(step_line(step5))
        if equivalent(step5, target):
            bridge_to_target(lines, step5, target, target_name)
            return lines
    first, note = half_angle_expand(source)
    if first is None:
        first, note = power_reduction_once(source)
    if first is None:
        return None
    lines = [start_line(source_name, source), note, step_line(first)]
    cur = first
    i = 0
    while i < 8 and not equivalent(cur, target):
        nxt = expand_small(cur)
        nxt = full_simplify(nxt)
        if same(nxt, cur):
            extra, note2 = power_reduction_once(cur)
            if extra is None:
                break
            lines.append(note2)
            cur = full_simplify(extra)
            lines.append(step_line(cur))
        else:
            cur = nxt
            lines.append("Expand and simplify.")
            lines.append(step_line(cur))
        i += 1
    if equivalent(cur, target):
        bridge_to_target(lines, cur, target, target_name)
        return lines
    return None


def prove_half_angle_ratio_identity(source, target, source_name, target_name):
    def match_half_angle_target(node):
        node = sim(node)
        if node[0] != "fn" or node[1] not in ("tan", "cot"):
            return None
        arg = node[2]
        if arg[0] == "div" and is_int_num(arg[2]) and arg[2][1] == 2:
            return node[1], arg[1]
        if arg[0] == "mul":
            items = list(flat(arg, "mul"))
            if len(items) == 2:
                if same(items[0], num(1, 2)):
                    return node[1], items[1]
                if same(items[1], num(1, 2)):
                    return node[1], items[0]
        return None

    target_info = match_half_angle_target(target)
    if target_info is None:
        return None
    raw_source = source
    name, base = target_info
    half = div(base, num(2))
    half_text = show(half)
    if name == "tan":
        form1 = div(add([num(1), neg(fn("cos", base))]), fn("sin", base))
        form2 = div(fn("sin", base), add([num(1), fn("cos", base)]))
        if same(raw_source, form1):
            step1_text = "= (2*sin(" + half_text + ")^2)/(2*sin(" + half_text + ")*cos(" + half_text + "))"
            step2 = div(fn("sin", half), fn("cos", half))
        elif same(raw_source, form2):
            step1_text = "= (2*sin(" + half_text + ")*cos(" + half_text + "))/(2*cos(" + half_text + ")^2)"
            step2 = div(fn("sin", half), fn("cos", half))
        elif equivalent(source, form1):
            step1_text = "= (2*sin(" + half_text + ")^2)/(2*sin(" + half_text + ")*cos(" + half_text + "))"
            step2 = div(fn("sin", half), fn("cos", half))
        elif equivalent(source, form2):
            step1_text = "= (2*sin(" + half_text + ")*cos(" + half_text + "))/(2*cos(" + half_text + ")^2)"
            step2 = div(fn("sin", half), fn("cos", half))
        else:
            return None
        lines = [
            start_line(source_name, source),
            "Use half-angle identities.",
            step1_text,
            step_line(step2),
            "= " + show(target),
            "= " + target_name,
        ]
        return lines
    form1 = div(add([num(1), fn("cos", base)]), fn("sin", base))
    form2 = div(fn("sin", base), add([num(1), neg(fn("cos", base))]))
    if same(raw_source, form1):
        step1_text = "= (2*cos(" + half_text + ")^2)/(2*sin(" + half_text + ")*cos(" + half_text + "))"
        step2 = div(fn("cos", half), fn("sin", half))
    elif same(raw_source, form2):
        step1_text = "= (2*sin(" + half_text + ")*cos(" + half_text + "))/(2*sin(" + half_text + ")^2)"
        step2 = div(fn("cos", half), fn("sin", half))
    elif equivalent(source, form1):
        step1_text = "= (2*cos(" + half_text + ")^2)/(2*sin(" + half_text + ")*cos(" + half_text + "))"
        step2 = div(fn("cos", half), fn("sin", half))
    elif equivalent(source, form2):
        step1_text = "= (2*sin(" + half_text + ")*cos(" + half_text + "))/(2*sin(" + half_text + ")^2)"
        step2 = div(fn("cos", half), fn("sin", half))
    else:
        return None
    lines = [
        start_line(source_name, source),
        "Use half-angle identities.",
        step1_text,
        step_line(step2),
        "= " + show(target),
        "= " + target_name,
    ]
    return lines


def direct_double_angle_rewrite(node):
    node = sim(node)
    if node[0] == "fn":
        base = half_angle_expr(node[2])
        if base is None:
            base = match_numeric_multiple_arg(node[2], 2)
        if base is not None:
            if node[1] == "sin":
                return mul([num(2), fn("sin", base), fn("cos", base)]), "Use sin(2A) = 2sin A cos A."
            if node[1] == "cos":
                return add([power(fn("cos", base), num(2)), neg(power(fn("sin", base), num(2)))]), "Use cos(2A) = cos^2 A - sin^2 A."
            if node[1] == "tan":
                return div(mul([num(2), fn("tan", base)]), add([num(1), neg(power(fn("tan", base), num(2)))])), "Use tan(2A) = 2tan A/(1 - tan^2 A)."
    product = match_same_angle_sin_cos_product(node)
    if product is not None and same(product[0], num(2)):
        return fn("sin", mul([num(2), product[1]])), "Use sin(2A) = 2sin A cos A."
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        if len(items) == 2 and same(items[0], num(2)) and items[1][0] == "pow" and same(items[1][2], num(2)) and items[1][1][0] == "fn":
            trig_fn = items[1][1][1]
            arg = items[1][1][2]
            if trig_fn == "cos":
                return add([num(1), fn("cos", mul([num(2), arg]))]), "Use 2cos^2 A = 1 + cos 2A."
            if trig_fn == "sin":
                return add([num(1), neg(fn("cos", mul([num(2), arg])))]), "Use 2sin^2 A = 1 - cos 2A."
    if node[0] == "add":
        bits = list(flat(node, "add"))
        if len(bits) == 2:
            first = signed_unit_term(bits[0])
            second = signed_unit_term(bits[1])
            if first is not None and second is not None:
                coeff1, rest1 = first
                coeff2, rest2 = second
                if is_minus_one(coeff1) and is_one(coeff2):
                    coeff1, rest1, coeff2, rest2 = coeff2, rest2, coeff1, rest1
                if is_one(coeff1) and is_minus_one(coeff2):
                    if rest1[0] == "pow" and rest2[0] == "pow" and same(rest1[2], num(2)) and same(rest2[2], num(2)):
                        if rest1[1][0] == "fn" and rest2[1][0] == "fn" and rest1[1][1] == "cos" and rest2[1][1] == "sin" and same(rest1[1][2], rest2[1][2]):
                            return fn("cos", mul([num(2), rest1[1][2]])), "Use cos(2A) = cos^2 A - sin^2 A."
                if is_one(rest1) and is_one(coeff1) and coeff2[1] < 0 and rest2[0] == "fn" and rest2[1] == "cos":
                    base = match_numeric_multiple_arg(rest2[2], 2)
                    if base is not None:
                        return mul([num(2), power(fn("sin", base), num(2))]), "Use 1 - cos(2A) = 2sin^2 A."
                if is_one(rest1) and is_one(coeff1) and is_one(coeff2) and rest2[0] == "fn" and rest2[1] == "cos":
                    base = match_numeric_multiple_arg(rest2[2], 2)
                    if base is not None:
                        return mul([num(2), power(fn("cos", base), num(2))]), "Use 1 + cos(2A) = 2cos^2 A."
    return None, None


def prove_double_angle_identity(source, target, source_name, target_name):
    rewritten, note = direct_double_angle_rewrite(source)
    if rewritten is None:
        return None
    if not equivalent(rewritten, target):
        return None
    lines = [start_line(source_name, source)]
    rewritten = detail_trig_expansion(lines, rewritten, target, target_name, note)
    if equivalent(rewritten, target):
        bridge_to_target(lines, rewritten, target, target_name)
        return lines
    return None


def prove_factor_pythagorean_identity(source, target, source_name, target_name):
    factored, note = factor_trig_power_pair_for_proof(source)
    if factored is None:
        factored, note = factor_common_term_for_proof(source)
    if factored is None:
        return None
    lines = [start_line(source_name, source), note, step_line(factored)]
    cur = factored

    changed = True
    while changed:
        changed = False
        pieces = list(flat(cur, "mul")) if cur[0] == "mul" else [cur]
        i = 0
        while i < len(pieces):
            rewritten, pyth_note = pythagorean_identity_rewrite_once(pieces[i])
            if rewritten is not None:
                cur = sim(make_mul(pieces[:i] + [rewritten] + pieces[i + 1 :]))
                lines.append(pyth_note)
                lines.append(step_line(cur))
                changed = True
                break
            i += 1

    if same(cur, target):
        lines.append("= " + target_name)
        return lines

    rewritten, rewrite_note = direct_double_angle_rewrite(cur)
    if rewritten is not None:
        lines.append(rewrite_note)
        lines.append(step_line(rewritten))
        if equivalent(rewritten, target):
            bridge_to_target(lines, rewritten, target, target_name)
            return lines

    named, named_note = proof_named_target_rewrite(cur, target)
    if named is not None and not same(named, cur):
        lines.append(named_note)
        lines.append(step_line(named))
        if equivalent(named, target):
            bridge_to_target(lines, named, target, target_name)
            return lines
    if equivalent(cur, target):
        bridge_to_target(lines, cur, target, target_name)
        return lines
    return None


def finish_verbose_proof(lines, cur, target, target_name):
    cur = sim(cur)
    named, note = proof_named_target_rewrite(cur, target)
    if named is not None and not same(named, cur):
        lines.append(note)
        lines.append(step_line(named))
        bridge_to_target(lines, named, target, target_name)
        return lines
    layout = proof_target_layout_steps(cur, target)
    if layout is not None:
        lines.extend(layout)
        lines.append("= " + target_name)
        return lines
    if equivalent(cur, target):
        bridge_to_target(lines, cur, target, target_name)
        return lines
    return None


def finish_verbose_proof_structured(lines, cur, target, target_name):
    cur = sim(cur)
    layout = proof_target_layout_steps(cur, target)
    if layout is not None:
        lines.extend(layout)
        lines.append("= " + target_name)
        return lines
    return None


def detail_trig_expansion(lines, expr, target, target_name, note=None):
    """Append a sequence of expansion/simplification steps for `expr` to `lines`.
    Returns the simplified expression after applying expansions.
    This is a non-destructive helper to produce more textbook-style working.
    """
    expr = full_simplify(expr)
    if note is not None:
        lines.append(note)
    lines.append(step_line(expr))

    exact_lines = []
    substituted = replace_exact_trig(expr, exact_lines)
    i = 0
    while i < len(exact_lines):
        lines.append(exact_lines[i])
        i += 1
    substituted = full_simplify(substituted)
    if not same(substituted, expr):
        lines.append(step_line(substituted))
        expr = substituted

    nxt = expand_small(expr)
    nxt = full_simplify(nxt)
    if not same(nxt, expr):
        lines.append("Expand and simplify.")
        lines.append(step_line(nxt))
        expr = nxt

    nxt = reduce_identities(expr)
    nxt = full_simplify(nxt)
    if not same(nxt, expr):
        lines.append("Use Pythagorean identities.")
        lines.append(step_line(nxt))
        expr = nxt

    nxt = simplify_fraction(expr)
    if not same(nxt, expr):
        lines.append("Simplify the fraction.")
        lines.append(step_line(nxt))
        expr = nxt

    return expr


def prove_verbose_division_identity(source, target, source_name, target_name):
    source = sim(source)
    target = sim(target)
    if source[0] != "div":
        return None
    lines = [start_line(source_name, source)]
    cur = source
    changed = False

    def maybe_finish():
        return finish_verbose_proof_structured(lines, cur, target, target_name)

    top_rewrite, top_note = proof_identity_rewrite_once(cur[1])
    if top_rewrite is not None:
        raw = ("div", top_rewrite, cur[2])
        lines.append(top_note)
        lines.append(raw_step_line(raw))
        cur = raw
        changed = True
        done = maybe_finish()
        if done is not None:
            return done

    if cur[0] == "div":
        top_rewrite, top_note = rewrite_sum_terms_for_display(cur[1])
        if top_rewrite is not None:
            raw = ("div", top_rewrite, cur[2])
            lines.append(top_note)
            lines.append(raw_step_line(raw))
            cur = raw
            changed = True
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        factored, factor_note = factor_difference_of_squares_for_display(cur[1])
        if factored is not None:
            raw = ("div", factored, cur[2])
            lines.append(factor_note)
            lines.append(raw_step_line(raw))
            cancelled = cancel_fraction_common_factor_for_display(raw)
            if cancelled is not None:
                lines.append(step_line(cancelled))
                cur = cancelled
            else:
                cur = raw
            changed = True
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        bot_rewrite, bot_note = proof_identity_rewrite_once(cur[2])
        if bot_rewrite is not None:
            raw = ("div", cur[1], bot_rewrite)
            lines.append(bot_note)
            lines.append(raw_step_line(raw))
            cancelled = cancel_fraction_common_factor_for_display(raw)
            if cancelled is not None:
                lines.append(step_line(cancelled))
                cur = cancelled
            else:
                cur = raw
            changed = True
        done = maybe_finish()
        if done is not None:
            return done

    if cur[0] == "div":
        bot_rewrite, bot_note = rewrite_sum_terms_for_display(cur[2])
        if bot_rewrite is not None:
            raw = ("div", cur[1], bot_rewrite)
            lines.append(bot_note)
            lines.append(raw_step_line(raw))
            cancelled = cancel_fraction_common_factor_for_display(raw)
            if cancelled is not None:
                lines.append(step_line(cancelled))
                cur = cancelled
            else:
                cur = raw
            changed = True
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        bot_rewrite, bot_note = rewrite_one_term_in_sum_for_display(cur[2])
        if bot_rewrite is not None:
            raw = ("div", cur[1], bot_rewrite)
            lines.append(bot_note)
            lines.append(raw_step_line(raw))
            cur = raw
            changed = True
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        den_step = common_denominator_step(cur[2])
        if den_step is not None:
            _den_expr, den_raw, den_note = den_step
            raw = ("div", cur[1], den_raw)
            lines.append(den_note)
            lines.append(raw_step_line(raw))
            cancelled = cancel_fraction_common_factor_for_display(raw)
            if cancelled is not None:
                lines.append(step_line(cancelled))
                cur = cancelled
            else:
                cur = raw
            changed = True
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        frac_double, frac_raw = rewrite_fraction_denominator_double_angle(cur)
        if frac_double is not None:
            lines.append("Use sin(2A) = 2sin(A)cos(A).")
            lines.append(raw_step_line(frac_raw))
            cur = frac_double
            changed = True
        done = maybe_finish()
        if done is not None:
            return done

    if cur[0] == "div":
        factored_raw, factor_note = factor_fraction_terms_for_display(cur)
        if factored_raw is not None:
            lines.append(factor_note)
            lines.append(raw_step_line(factored_raw))
            cancelled = cancel_fraction_common_factor_for_display(factored_raw)
            if cancelled is not None:
                lines.append(step_line(cancelled))
                cur = cancelled
            else:
                cur = factored_raw
            changed = True
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        bot_rewrite, bot_note = proof_identity_rewrite_once(cur[2])
        if bot_rewrite is not None:
            raw = ("div", cur[1], bot_rewrite)
            lines.append(bot_note)
            lines.append(raw_step_line(raw))
            cur = raw
            changed = True
            done = maybe_finish()
            if done is not None:
                return done

    if not changed:
        return None
    return finish_verbose_proof(lines, cur, target, target_name)


def prove_verbose_common_denominator_identity(source, target, source_name, target_name):
    source = sim(source)
    target = sim(target)
    step = common_denominator_step(source)
    if step is None:
        return None
    _cur, raw, note = step
    cur = raw
    lines = [start_line(source_name, source), note, raw_step_line(raw)]

    def maybe_finish():
        return finish_verbose_proof_structured(lines, cur, target, target_name)

    if cur[0] == "div":
        num_expand, num_note = sum_product_expand(cur[1])
        if num_expand is not None:
            raw = ("div", num_expand, cur[2])
            lines.append(num_note)
            lines.append(raw_step_line(raw))
            cancelled = cancel_fraction_common_factor_for_display(raw)
            if cancelled is not None:
                lines.append(step_line(cancelled))
                cur = cancelled
            else:
                cur = raw
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        contract, contract_note = trig_formula_contract_once(cur[1])
        if contract is not None:
            raw = ("div", contract, cur[2])
            lines.append(contract_note)
            lines.append(raw_step_line(raw))
            cur = raw
            done = maybe_finish()
            if done is not None:
                return done

    if cur[0] == "div":
        frac_double, frac_raw = rewrite_fraction_denominator_double_angle(cur)
        if frac_double is not None:
            lines.append("Use sin(2A) = 2sin(A)cos(A).")
            lines.append(raw_step_line(frac_raw))
            cur = frac_double
            done = maybe_finish()
            if done is not None:
                return done
    return finish_verbose_proof(lines, cur, target, target_name)


def prove_formula_identity(source, target, source_name, target_name):
    lines = [start_line(source_name, source)]
    direct, note = trig_formula_expand(source)
    if direct is None:
        direct, note = half_angle_expand(source)
    if direct is None:
        direct, note = power_reduction_once(source)
    if direct is not None:
        direct = full_simplify(direct)
        lines.append(note)
        lines.append(step_line(direct))
        exact_lines = []
        direct_exact = replace_exact_trig(direct, exact_lines)
        i = 0
        while i < len(exact_lines):
            lines.append(exact_lines[i])
            i += 1
        direct_exact = full_simplify(direct_exact)
        if not same(direct_exact, direct):
            lines.append(step_line(direct_exact))
        if equivalent(direct_exact, target):
            bridge_to_target(lines, direct_exact, target, target_name)
            return lines
    tree = expand_trig_tree(source)
    tree = full_simplify(tree)
    if not same(tree, source):
        if direct is None:
            tree = full_simplify(tree)
            tree = detail_trig_expansion(lines, tree, target, target_name, "Use the standard trig identity.")
        elif not same(tree, direct):
            lines.append("Simplify the result.")
            tree = full_simplify(tree)
            tree = detail_trig_expansion(lines, tree, target, target_name, None)
        if equivalent(tree, target):
            bridge_to_target(lines, tree, target, target_name)
            return lines
    return None


def generic_side_path(expr, side_name):
    lines = [start_line(side_name, expr)]
    cur = expr
    nxt = reciprocal_trig(cur)
    if not same(nxt, cur):
        lines.append("Write tan, cot, sec and cosec using sin and cos.")
        cur = full_simplify(nxt)
        lines.append(step_line(cur))
    nxt = expand_trig_tree(cur)
    if not same(nxt, cur):
        lines.append("Expand the trig identities.")
        cur = full_simplify(nxt)
        lines.append(step_line(cur))
    nxt = expand_small(cur)
    nxt = full_simplify(nxt)
    if not same(nxt, cur):
        lines.append("Expand and simplify.")
        cur = nxt
        lines.append(step_line(cur))
    nxt = reduce_identities(cur)
    nxt = full_simplify(nxt)
    if not same(nxt, cur):
        lines.append("Use Pythagorean identities.")
        cur = nxt
        lines.append(step_line(cur))
    cur = full_simplify(cur)
    if len(lines) == 1:
        lines.append(step_line(cur))
    nxt = simplify_fraction(cur)
    if not same(nxt, cur):
        lines.append("Simplify the fraction.")
        cur = nxt
        lines.append(step_line(cur))
    return cur, lines


def prove_general(lhs, rhs):
    left_common, left_lines = generic_side_path(lhs, "LHS")
    right_common, right_lines = generic_side_path(rhs, "RHS")
    if equivalent(left_common, right_common):
        return left_lines + right_lines
    return None


def proof_complexity_score(expr):
    fn_names = function_names_of(expr)
    kind_names = kind_names_of(expr)
    trig_count = 0
    i = 0
    while i < len(fn_names):
        if fn_names[i] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            trig_count += 1
        i += 1
    top_add_terms = len(flat(expr, "add")) if expr[0] == "add" else 1
    return (
        1 if expr[0] == "div" else 0,
        top_add_terms,
        reciprocal_burden(expr),
        1 if "div" in kind_names else 0,
        len(trig_arg_keys_of(expr)),
        trig_count,
        tree_size(expr),
    )


def proof_direction_candidates(source, target, source_name, target_name):
    # Keep proof mode deterministic, but rank the more school-method routes
    # ahead of generic expansion so auto mode prefers sensible working.
    out = []
    for priority, fn_prover in (
        (0, prove_reciprocal_identity),
        (0, prove_reciprocal_reduction_identity),
        (0, prove_half_angle_ratio_identity),
        (0, prove_double_angle_identity),
        (1, prove_factor_pythagorean_identity),
        (1, prove_product_pair_identity),
        (2, prove_verbose_division_identity),
        (2, prove_verbose_common_denominator_identity),
        (3, prove_sum_product_identity),
        (3, prove_tan_multiple),
        (3, prove_shift_identity),
        (3, prove_shift_compare),
        (4, prove_power_identity),
        (4, prove_log_exp_identity),
        (6, prove_formula_identity),
    ):
        lines = fn_prover(source, target, source_name, target_name)
        if lines is not None:
            out.append(((priority, len(lines)), lines))
    return out


def best_proof_direction(source, target, source_name, target_name):
    candidates = proof_direction_candidates(source, target, source_name, target_name)
    if len(candidates) == 0:
        return None
    best = candidates[0]
    i = 1
    while i < len(candidates):
        if candidates[i][0] < best[0]:
            best = candidates[i]
        i += 1
    return best


def normalize_route(route):
    route = route.strip().lower()
    if route == "" or route == "auto":
        return "1"
    if route == "lhs":
        return "2"
    if route == "rhs":
        return "3"
    if route == "both":
        return "4"
    return route


def split_identity_text_raw(text):
    depth = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "=" and depth == 0:
            return text[:i].strip(), text[i + 1 :].strip()
        i += 1
    raise ValueError("Identity must contain '='.")


def split_equation_text_raw_or_zero(text):
    try:
        return split_identity_text_raw(text)
    except Exception:
        return text.strip(), "0"


def has_outer_parens_text(text):
    if len(text) < 2 or text[0] != "(" or text[-1] != ")":
        return False
    depth = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0 and i != len(text) - 1:
                return False
        i += 1
    return depth == 0


def clean_expr_text(text):
    out = text.replace(" ", "")
    while has_outer_parens_text(out):
        out = out[1:-1]
    return out


def special_text_proof(text, route):
    route = normalize_route(route)
    left, right = split_identity_text_raw(text)
    left_clean = clean_expr_text(left)
    right_clean = clean_expr_text(right)

    def try_one(source_text, target_text, source_name, target_name):
        if source_text.startswith("ln(exp(") and source_text.endswith("))"):
            inner = source_text[7:-2]
            if target_text == inner:
                return [
                    source_name + " = " + source_text,
                    "Use exp(A) = e^A.",
                    "= ln(e^(" + inner + "))",
                    "Use ln(e^A) = A.",
                    "= " + inner,
                    "= " + target_name,
                ]
        if source_text.startswith("ln(e^(") and source_text.endswith("))"):
            inner = source_text[6:-2]
            if target_text == inner:
                return [
                    source_name + " = " + source_text,
                    "Use ln(e^A) = A.",
                    "= " + inner,
                    "= " + target_name,
                ]
        if source_text.startswith("exp(ln(") and source_text.endswith("))"):
            inner = source_text[7:-2]
            if target_text == inner:
                return [
                    source_name + " = " + source_text,
                    "Use exp(A) = e^A.",
                    "= e^(ln(" + inner + "))",
                    "Use e^(ln A) = A.",
                    "= " + inner,
                    "= " + target_name,
                ]
        if source_text.startswith("exp(") and source_text.endswith(")"):
            inner = source_text[4:-1]
            if target_text == "e^(" + inner + ")" or target_text == "e^" + inner:
                return [
                    source_name + " = " + source_text,
                    "Use exp(A) = e^A.",
                    "= e^(" + inner + ")",
                    "= " + target_name,
                ]
        if source_text.startswith("e^(ln(") and source_text.endswith("))"):
            inner = source_text[6:-2]
            if target_text == inner:
                return [
                    source_name + " = " + source_text,
                    "Use e^(ln A) = A.",
                    "= " + inner,
                    "= " + target_name,
                ]
        return None

    if route in ("1", "2", "4"):
        lines = try_one(left_clean, right_clean, "LHS", "RHS")
        if lines is not None:
            return lines
    if route in ("1", "3", "4"):
        lines = try_one(right_clean, left_clean, "RHS", "LHS")
        if lines is not None:
            return lines
    return None


def prove_direct(lhs, rhs, route):
    route = normalize_route(route)
    if route == "1":
        left_first = proof_complexity_score(lhs) >= proof_complexity_score(rhs)
        first = best_proof_direction(lhs, rhs, "LHS", "RHS") if left_first else best_proof_direction(rhs, lhs, "RHS", "LHS")
        second = best_proof_direction(rhs, lhs, "RHS", "LHS") if left_first else best_proof_direction(lhs, rhs, "LHS", "RHS")
        if first is not None and second is not None:
            if second[0] < first[0]:
                return second[1]
            return first[1]
        if first is not None:
            return first[1]
        if second is not None:
            return second[1]
        both = prove_general(lhs, rhs)
        if both is not None:
            return both
        return None
    if route == "2":
        best = best_proof_direction(lhs, rhs, "LHS", "RHS")
        if best is not None:
            return best[1]
        return prove_general(lhs, rhs)
    if route == "3":
        best = best_proof_direction(rhs, lhs, "RHS", "LHS")
        if best is not None:
            return best[1]
        return prove_general(lhs, rhs)
    both = prove_general(lhs, rhs)
    if both is not None:
        return both
    left = best_proof_direction(lhs, rhs, "LHS", "RHS")
    right = best_proof_direction(rhs, lhs, "RHS", "LHS")
    if left is None:
        return None if right is None else right[1]
    if right is None or left[0] <= right[0]:
        return left[1]
    return right[1]


def finalize_proof_lines(lines):
    return lines


def prove_by_difference_zero(lhs, rhs):
    lhs = sim(lhs)
    rhs = sim(rhs)
    diff = sim(add([lhs, neg(rhs)]))
    if not equivalent(diff, num(0)):
        return None
    lines = ["LHS - RHS = " + show(diff)]
    if same(diff, num(0)):
        lines.append("0 = 0")
        lines.append("So LHS = RHS")
        return compact_lines(lines)
    var = detect_transform_var(lhs, rhs)
    cur = diff
    visited = {cur}
    pass_count = 0
    while pass_count < SOLVE_PASS_LIMIT and not same(cur, num(0)):
        best = best_solve_rewrite(cur, var, visited)
        if best is None:
            break
        cur = best["expr"]
        visited.add(cur)
        i = 0
        while i < len(best["lines"]):
            lines.append(best["lines"][i])
            i += 1
        pass_count += 1
    if not same(cur, num(0)):
        nxt = full_simplify(expand_embedded_small(expand_trig_tree(cur)))
        if not same(nxt, cur):
            lines.append("Expand the trig identities.")
            lines.append(equation_line(nxt, num(0)))
            cur = nxt
        nxt = full_simplify(reduce_identities(cur))
        if not same(nxt, cur):
            lines.append("Use Pythagorean identities.")
            lines.append(equation_line(nxt, num(0)))
            cur = nxt
        nxt = full_simplify(simplify_fraction(cur))
        if not same(nxt, cur):
            lines.append("Simplify the fraction.")
            lines.append(equation_line(nxt, num(0)))
            cur = nxt
    if not same(cur, num(0)):
        cur = full_simplify(cur)
    if not same(cur, num(0)):
        return None
    lines.append("0 = 0")
    lines.append("So LHS = RHS")
    return compact_lines(lines)


# ---------------------------------------------------------------------------
# Proof mode entrypoint
# ---------------------------------------------------------------------------
def solve_prove(lhs, rhs, route):
    # Proof mode stays deliberately narrower than solve mode so the output
    # remains a tidy textbook proof rather than a long rewrite trace.
    route = normalize_route(route)
    lhs = sim(lhs)
    rhs = sim(rhs)
    if same(lhs, rhs):
        return ["LHS = RHS"]
    lines = prove_direct(lhs, rhs, route)
    if lines is not None:
        return compact_lines(finalize_proof_lines(lines))
    if equivalent(lhs, rhs):
        lines = prove_by_difference_zero(lhs, rhs)
        if lines is not None:
            return lines
        return ["LHS = RHS"]
    raise ValueError("This is not an identity, so prove/show mode cannot be used.")


def extract_linear_combo_equation(lhs, rhs):
    combo = match_linear_combo(lhs)
    if combo is not None:
        names = set()
        collect_symbols(combo[0], names)
        if independent_of_names(rhs, names):
            return lhs, rhs, combo
    combo = match_linear_combo(rhs)
    if combo is not None:
        names = set()
        collect_symbols(combo[0], names)
        if independent_of_names(lhs, names):
            return rhs, lhs, combo
    return None


def extract_shift_equation(lhs, rhs, base):
    match = match_shift_target(lhs, base)
    if match is not None:
        names = set()
        collect_symbols(base, names)
        if independent_of_names(rhs, names):
            return lhs, rhs, match
    match = match_shift_target(rhs, base)
    if match is not None:
        names = set()
        collect_symbols(base, names)
        if independent_of_names(lhs, names):
            return rhs, lhs, match
    return None


def shift_angle_from_combo(name, sign, cos_coeff, sin_coeff, deg_mode):
    cos_val = constant_numeric(cos_coeff, deg_mode)
    sin_val = constant_numeric(sin_coeff, deg_mode)
    if cos_val is None or sin_val is None:
        return None
    if name == "sin":
        if sign > 0:
            return math.atan2(cos_val, sin_val) if not deg_mode else to_degrees(math.atan2(cos_val, sin_val))
        return math.atan2(-cos_val, sin_val) if not deg_mode else to_degrees(math.atan2(-cos_val, sin_val))
    if sign > 0:
        return math.atan2(-sin_val, cos_val) if not deg_mode else to_degrees(math.atan2(-sin_val, cos_val))
    return math.atan2(sin_val, cos_val) if not deg_mode else to_degrees(math.atan2(sin_val, cos_val))


def transform_linear_combo_shift(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text):
    source = extract_linear_combo_equation(eq1_lhs, eq1_rhs)
    if source is None:
        return None
    source_side, const_side, combo = source
    base, cos_coeff, sin_coeff = combo
    target = extract_shift_equation(eq2_lhs, eq2_rhs, base)
    if target is None:
        return None
    target_side, target_const, shift = target
    if not equivalent(const_side, target_const):
        return None
    coeff, name, shift_node, sign = shift
    r_expr = full_simplify(fn("sqrt", add([power(cos_coeff, num(2)), power(sin_coeff, num(2))])))
    coeff_names = set()
    collect_symbols(coeff, coeff_names)
    if len(coeff_names) == 0 and not equivalent(r_expr, coeff):
        return None
    phase_placeholder = shift_node[1] if shift_node[0] == "sym" else None
    coeff_placeholder = coeff[1] if coeff[0] == "sym" else None
    use_template_placeholders = phase_placeholder is not None or coeff_placeholder is not None
    phase_label = phase_placeholder or "a"
    coeff_label = coeff_placeholder or "R"
    deg_mode = False if shift_node[0] == "sym" else not contains_pi(shift_node)
    alpha = shift_angle_from_combo(name, sign, cos_coeff, sin_coeff, deg_mode)
    if alpha is None:
        return None
    if shift_node[0] != "sym":
        target_alpha = eval_numeric_mode(shift_node, {}, deg_mode)
        if abs(alpha - target_alpha) > 1e-6:
            return None
    if name == "sin":
        tan_expr = full_simplify(div(cos_coeff if sign > 0 else neg(cos_coeff), sin_coeff))
        first_left = "sin"
        second_left = "cos"
        first_eq = coeff_label + "*cos(" + phase_label + ") = " + show(sin_coeff)
        second_eq = (coeff_label + "*sin(" + phase_label + ") = " if sign > 0 else "-" + coeff_label + "*sin(" + phase_label + ") = ") + show(cos_coeff)
        formula = FORMULA_SIN_ADD if sign > 0 else FORMULA_SIN_SUB
    else:
        tan_expr = full_simplify(div(neg(sin_coeff) if sign > 0 else sin_coeff, cos_coeff))
        first_left = "cos"
        second_left = "sin"
        first_eq = coeff_label + "*cos(" + phase_label + ") = " + show(cos_coeff)
        second_eq = ("-" + coeff_label + "*sin(" + phase_label + ") = " if sign > 0 else coeff_label + "*sin(" + phase_label + ") = ") + show(sin_coeff)
        formula = FORMULA_COS_ADD if sign > 0 else FORMULA_COS_SUB
    r_cos_text = show(display_abs(cos_coeff))
    r_sin_text = show(display_abs(sin_coeff))
    raw_left, raw_right = split_equation_text_raw_or_zero(eq2_text)
    target_side_text = raw_left if same(target_side, eq2_lhs) else raw_right
    lines = [
        "Equation 1: " + equation_line(eq1_lhs, eq1_rhs),
        "Write " + show(source_side) + " as " + coeff_label + "*" + name + "(" + show(base) + ("+" if sign > 0 else "-") + phase_label + ").",
        formula,
        "Compare " + first_left + "(" + show(base) + "): " + first_eq,
        "Compare " + second_left + "(" + show(base) + "): " + second_eq,
        "Square and add the two equations.",
        coeff_label + " = sqrt(" + r_cos_text + "^2+" + r_sin_text + "^2)",
        "= " + show(r_expr),
        "Divide the second equation by the first.",
        "tan(" + phase_label + ") = " + show(tan_expr),
    ]
    angle_expr = shift_node if shift_node[0] != "sym" else parse(angle_text(alpha, deg_mode))
    if shift_node[0] == "sym":
        lines.append(phase_label + " = " + angle_text(alpha, deg_mode))
    else:
        lines.append(phase_label + " = " + show(shift_node))
    lines.append("So " + show(source_side) + " = " + target_side_text)
    if use_template_placeholders:
        values = {}
        if coeff_placeholder is not None:
            values[coeff_placeholder] = r_expr
        if phase_placeholder is not None:
            values[phase_placeholder] = angle_expr
        if raw_right == "0":
            lines.append("Hence " + show(substitute_symbols(eq2_lhs, values)))
        else:
            lines.append("Hence " + equation_line(substitute_symbols(eq2_lhs, values), substitute_symbols(eq2_rhs, values)))
    else:
        lines.append("Hence " + eq2_text.strip())
    return compact_lines(lines)


def factor_linear_term_from_root(atom, root):
    if not is_num(root):
        return None
    if root[2] == 1:
        return add([atom, num(-root[1])])
    return add([mul([num(root[2]), atom]), num(-root[1])])


def transform_factor_quadratic(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text):
    expr1 = sim(add([eq1_lhs, neg(eq1_rhs)]))
    expr2 = sim(add([eq2_lhs, neg(eq2_rhs)]))
    names = set()
    collect_symbols(expr1, names)
    if len(names) != 1:
        return None
    var = list(names)[0]
    quad = extract_quadratic_trig(expr1, var)
    if quad is None:
        return None
    name, arg, mult, offset, a, b, c = quad
    _ = mult
    _ = offset
    if not (is_num(a) and is_num(b) and is_num(c)):
        return None
    roots = quadratic_roots(a, b, c, False)
    if len(roots) != 2:
        return None
    root1 = infer_rational_node(roots[0], 48)
    root2 = infer_rational_node(roots[1], 48)
    if root1 is None or root2 is None:
        return None
    atom = fn(name, arg)
    factor1 = factor_linear_term_from_root(atom, root1)
    factor2 = factor_linear_term_from_root(atom, root2)
    if factor1 is None or factor2 is None:
        return None
    lead = num(root1[2] * root2[2])
    scale = full_simplify(div(a, lead))
    product_raw = factor1 if same(factor2, num(1)) else make_mul([factor1, factor2])
    if not is_one(scale):
        product_raw = make_mul([scale, product_raw])
    product = full_simplify(product_raw)
    if not (equivalent(product, expr1) and equivalent(product_raw, expr2)):
        return None
    lines = [
        "Equation 1: " + equation_line(eq1_lhs, eq1_rhs),
        "Let u = " + show(atom) + ".",
        quadratic_standard_text(a, sym("u"), b, c) + " = 0",
        "Factorise.",
        show(product_raw).replace(show(atom), "u") + " = 0",
        "Substitute back.",
        equation_line(product_raw, num(0)),
        "Hence " + eq2_text.strip(),
    ]
    return compact_lines(lines)


def single_symbol_name(node):
    names = set()
    collect_symbols(node, names)
    if len(names) != 1:
        return None
    return list(names)[0]


def transform_zero_form_rewrite(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text, rewriter):
    expr1 = sim(add([eq1_lhs, neg(eq1_rhs)]))
    expr2 = sim(add([eq2_lhs, neg(eq2_rhs)]))
    var = single_symbol_name(expr1)
    if var is None:
        return None
    result = rewriter(expr1, var)
    if result is None:
        return None
    new_expr, extra = result
    if not equivalent(new_expr, expr2):
        return None
    lines = [
        equation_line(eq1_lhs, eq1_rhs),
        equation_line(expr1, num(0)),
    ]
    i = 0
    while i < len(extra):
        lines.append(extra[i])
        i += 1
    lines.append("Hence " + eq2_text.strip())
    return compact_lines(lines)


def transform_same_angle_ratio(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text):
    expr1 = sim(add([eq1_lhs, neg(eq1_rhs)]))
    var = single_symbol_name(expr1)
    if var is None:
        return None
    data = collect_same_arg_terms(expr1, var, [("sin", "sin", 1, False), ("cos", "cos", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sin"]) or is_zero(coeffs["cos"]) or not is_zero(const):
        return None

    tan_expr = sim(add([fn("tan", arg), div(coeffs["cos"], coeffs["sin"])]))
    cot_expr = sim(add([fn("cot", arg), div(coeffs["sin"], coeffs["cos"])]))
    tan_equation = equation_line(fn("tan", arg), full_simplify(neg(div(coeffs["cos"], coeffs["sin"]))))
    cot_equation = equation_line(fn("cot", arg), full_simplify(neg(div(coeffs["sin"], coeffs["cos"]))))
    expr2 = sim(add([eq2_lhs, neg(eq2_rhs)]))

    if equivalent(tan_expr, expr2):
        lines = [
            "Equation 1: " + equation_line(eq1_lhs, eq1_rhs),
            "Move all terms to one side.",
            equation_line(expr1, num(0)),
            "cos(" + show(arg) + ") = 0 does not work, so divide by cos(" + show(arg) + ").",
            equation_line(add([mul([coeffs["sin"], fn("tan", arg)]), coeffs["cos"]]), num(0)),
            "Rearrange.",
            tan_equation,
            "Hence " + eq2_text.strip(),
        ]
        return compact_lines(lines)

    if equivalent(cot_expr, expr2):
        lines = [
            "Equation 1: " + equation_line(eq1_lhs, eq1_rhs),
            "Move all terms to one side.",
            equation_line(expr1, num(0)),
            "sin(" + show(arg) + ") = 0 does not work, so divide by sin(" + show(arg) + ").",
            equation_line(add([coeffs["sin"], mul([coeffs["cos"], fn("cot", arg)])]), num(0)),
            "Rearrange.",
            cot_equation,
            "Hence " + eq2_text.strip(),
        ]
        return compact_lines(lines)
    return None


def transform_half_angle_ratio(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs):
    if not (is_zero(eq1_rhs) and is_zero(eq2_rhs)):
        return None
    rewritten = half_angle_ratio_rewrite(eq1_lhs)
    if rewritten is None:
        return None
    new_expr, note = rewritten
    if not equivalent(new_expr, eq2_lhs):
        return None
    return [show(eq1_lhs), note, "= " + show(eq2_lhs)]


def detect_transform_var(*nodes):
    i = 0
    fallback = None
    while i < len(nodes):
        names = symbols_of(nodes[i])
        j = 0
        while j < len(names):
            name = names[j]
            if name == "x":
                return "x"
            if name[:1].islower() and fallback is None:
                fallback = name
            j += 1
        i += 1
    return fallback or "x"


def depends_any(node, names):
    syms = symbols_of(node)
    i = 0
    while i < len(names):
        if names[i] in syms:
            return True
        i += 1
    return False


# ---------------------------------------------------------------------------
# Transform mode helpers
# ---------------------------------------------------------------------------
FIT_TEMPLATE_SAMPLE_ANGLES = (
    num(0),
    div(PI, num(8)),
    div(PI, num(6)),
    div(PI, num(4)),
    div(PI, num(3)),
    mul([num(3), div(PI, num(8))]),
    div(PI, num(2)),
    mul([num(5), div(PI, num(8))]),
    mul([num(2), div(PI, num(3))]),
    mul([num(3), div(PI, num(4))]),
    mul([num(5), div(PI, num(6))]),
    PI,
)


def collect_symbol_order(node, out, seen):
    kind = node[0]
    if kind == "sym":
        if node[1] not in seen:
            seen[node[1]] = 1
            out.append(node[1])
        return
    if kind in ("num", "const"):
        return
    if kind == "fn":
        collect_symbol_order(node[2], out, seen)
        return
    if kind in ("pow", "div"):
        collect_symbol_order(node[1], out, seen)
        collect_symbol_order(node[2], out, seen)
        return
    i = 0
    while i < len(node[1]):
        collect_symbol_order(node[1][i], out, seen)
        i += 1


def detect_template_params(source_expr, template_expr, var):
    source_names = symbols_of(source_expr)
    target_order = []
    collect_symbol_order(template_expr, target_order, {})
    params = []
    i = 0
    while i < len(target_order):
        name = target_order[i]
        if name != var and name not in source_names:
            params.append(name)
        i += 1
    return params


def collect_trig_argument_lower_symbols(node, out):
    kind = node[0]
    if kind in ("num", "const"):
        return
    if kind == "sym":
        if node[1][:1].islower():
            out.add(node[1])
        return
    if kind == "fn":
        if node[1] in TRIG_REWRITE_FN_NAMES:
            names = symbols_of(node[2])
            i = 0
            while i < len(names):
                if names[i][:1].islower():
                    out.add(names[i])
                i += 1
        collect_trig_argument_lower_symbols(node[2], out)
        return
    if kind in ("pow", "div"):
        collect_trig_argument_lower_symbols(node[1], out)
        collect_trig_argument_lower_symbols(node[2], out)
        return
    i = 0
    while i < len(node[1]):
        collect_trig_argument_lower_symbols(node[1][i], out)
        i += 1


def zero_param_coeff_map(params):
    out = {}
    i = 0
    while i < len(params):
        out[params[i]] = num(0)
        i += 1
    return out


def add_param_coeff_maps(left, right, params):
    out = {}
    i = 0
    while i < len(params):
        out[params[i]] = full_simplify(add([left[params[i]], right[params[i]]]))
        i += 1
    return out


def scale_param_coeff_map(coeffs, scalar, params):
    out = {}
    i = 0
    while i < len(params):
        out[params[i]] = full_simplify(mul([coeffs[params[i]], scalar]))
        i += 1
    return out


def extract_linear_param_expr(node, params):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "const"):
        return zero_param_coeff_map(params), node
    if kind == "sym":
        if node[1] in params:
            out = zero_param_coeff_map(params)
            out[node[1]] = num(1)
            return out, num(0)
        return zero_param_coeff_map(params), node
    if kind == "fn":
        if depends_any(node[2], params):
            return None
        return zero_param_coeff_map(params), node
    if kind == "pow":
        if depends_any(node[1], params) or depends_any(node[2], params):
            if same(node[2], num(1)):
                return extract_linear_param_expr(node[1], params)
            return None
        return zero_param_coeff_map(params), node
    if kind == "div":
        if depends_any(node[2], params):
            return None
        left = extract_linear_param_expr(node[1], params)
        if left is None:
            return None
        scale = div(num(1), node[2])
        return scale_param_coeff_map(left[0], scale, params), full_simplify(mul([left[1], scale]))
    if kind == "add":
        coeffs = zero_param_coeff_map(params)
        const = num(0)
        i = 0
        while i < len(node[1]):
            cur = extract_linear_param_expr(node[1][i], params)
            if cur is None:
                return None
            coeffs = add_param_coeff_maps(coeffs, cur[0], params)
            const = full_simplify(add([const, cur[1]]))
            i += 1
        return coeffs, const
    if kind == "mul":
        dependent = []
        constant = []
        i = 0
        while i < len(node[1]):
            if depends_any(node[1][i], params):
                dependent.append(node[1][i])
            else:
                constant.append(node[1][i])
            i += 1
        scale = make_mul(constant)
        if len(dependent) == 0:
            return zero_param_coeff_map(params), node
        if len(dependent) > 1:
            return None
        inner = extract_linear_param_expr(dependent[0], params)
        if inner is None:
            return None
        return scale_param_coeff_map(inner[0], scale, params), full_simplify(mul([scale, inner[1]]))
    return None


def normalise_fit_constant(node):
    node = full_simplify(node)
    node = normalise_constant_trig_args(replace_exact_trig_quiet(node))
    return full_simplify(node)


def extract_template_basis_terms(node, params, out, seen):
    node = sim(node)
    if not depends_any(node, params):
        if sig(node) not in seen:
            seen[sig(node)] = 1
            out.append(node)
        return
    linear = extract_linear_param_expr(node, params)
    if linear is not None:
        if not is_zero(linear[1]):
            extract_template_basis_terms(linear[1], params, out, seen)
        i = 0
        while i < len(params):
            coeff = linear[0][params[i]]
            if not is_zero(coeff):
                extract_template_basis_terms(coeff, params, out, seen)
            i += 1
        return
    kind = node[0]
    if kind == "fn":
        extract_template_basis_terms(node[2], params, out, seen)
        return
    if kind in ("pow", "div"):
        extract_template_basis_terms(node[1], params, out, seen)
        extract_template_basis_terms(node[2], params, out, seen)
        return
    if kind in ("add", "mul"):
        i = 0
        while i < len(node[1]):
            extract_template_basis_terms(node[1][i], params, out, seen)
            i += 1


def rewrite_double_angles_to_allowed_basis(node, info):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        arg = rewrite_double_angles_to_allowed_basis(node[2], info)
        if node[1] == "cos":
            base = match_numeric_multiple_arg(arg, 2)
            if base is not None:
                sin_allowed = rewrite_allowed_has_fn(info, "sin", base)
                cos_allowed = rewrite_allowed_has_fn(info, "cos", base)
                if sin_allowed and (not cos_allowed or rewrite_burden(add([num(1), neg(mul([num(2), power(fn("sin", base), num(2))]))]), info) <= rewrite_burden(add([mul([num(2), power(fn("cos", base), num(2))]), num(-1)]), info)):
                    return full_simplify(add([num(1), neg(mul([num(2), power(fn("sin", base), num(2))]))]))
                if cos_allowed:
                    return full_simplify(add([mul([num(2), power(fn("cos", base), num(2))]), num(-1)]))
        if node[1] == "sin":
            base = match_numeric_multiple_arg(arg, 2)
            if base is not None and rewrite_allowed_has_fn(info, "sin", base) and rewrite_allowed_has_fn(info, "cos", base):
                return full_simplify(mul([num(2), fn("sin", base), fn("cos", base)]))
        return fn(node[1], arg)
    if kind == "pow":
        return power(rewrite_double_angles_to_allowed_basis(node[1], info), rewrite_double_angles_to_allowed_basis(node[2], info))
    if kind == "div":
        return div(rewrite_double_angles_to_allowed_basis(node[1], info), rewrite_double_angles_to_allowed_basis(node[2], info))
    items = []
    i = 0
    while i < len(node[1]):
        items.append(rewrite_double_angles_to_allowed_basis(node[1][i], info))
        i += 1
    return sim((kind, tuple(items)))


def rewrite_ratio_square_to_allowed_basis(node, info):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return fn(node[1], rewrite_ratio_square_to_allowed_basis(node[2], info))
    if kind == "pow":
        return power(rewrite_ratio_square_to_allowed_basis(node[1], info), rewrite_ratio_square_to_allowed_basis(node[2], info))
    if kind == "div":
        top = rewrite_ratio_square_to_allowed_basis(node[1], info)
        bot = rewrite_ratio_square_to_allowed_basis(node[2], info)
        top_pm = match_one_pm_cos_norm(top)
        bot_pm = match_one_pm_cos_norm(bot)
        if top_pm is not None and bot_pm is not None and cheap_same(top_pm[1], bot_pm[1]):
            half = half_angle_expr(top_pm[1])
            if half is not None:
                if top_pm[0] < 0 and bot_pm[0] > 0 and rewrite_allowed_has_fn(info, "tan", half):
                    return full_simplify(power(fn("tan", half), num(2)))
                if top_pm[0] > 0 and bot_pm[0] < 0 and rewrite_allowed_has_fn(info, "cot", half):
                    return full_simplify(power(fn("cot", half), num(2)))
        return div(top, bot)
    items = []
    i = 0
    while i < len(node[1]):
        items.append(rewrite_ratio_square_to_allowed_basis(node[1][i], info))
        i += 1
    return sim((kind, tuple(items)))


def rewrite_single_square_basis_to_allowed_terms(node, info):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return fn(node[1], rewrite_single_square_basis_to_allowed_terms(node[2], info))
    if kind == "pow":
        base = rewrite_single_square_basis_to_allowed_terms(node[1], info)
        exp = rewrite_single_square_basis_to_allowed_terms(node[2], info)
        if same(exp, num(2)) and base[0] == "fn" and base[1] in ("sin", "cos"):
            arg = base[2]
            if base[1] == "cos" and rewrite_allowed_has_fn(info, "sin", arg) and not rewrite_allowed_has_fn(info, "cos", arg):
                return full_simplify(add([num(1), neg(power(fn("sin", arg), num(2)))]))
            if base[1] == "sin" and rewrite_allowed_has_fn(info, "cos", arg) and not rewrite_allowed_has_fn(info, "sin", arg):
                return full_simplify(add([num(1), neg(power(fn("cos", arg), num(2)))]))
        if same(exp, num(2)):
            product = match_same_angle_sin_cos_product(base)
            if product is not None:
                coeff, arg = product
                if rewrite_allowed_has_fn(info, "sin", arg) and not rewrite_allowed_has_fn(info, "cos", arg):
                    return full_simplify(mul([power(coeff, num(2)), power(fn("sin", arg), num(2)), add([num(1), neg(power(fn("sin", arg), num(2)))])]))
                if rewrite_allowed_has_fn(info, "cos", arg) and not rewrite_allowed_has_fn(info, "sin", arg):
                    return full_simplify(mul([power(coeff, num(2)), power(fn("cos", arg), num(2)), add([num(1), neg(power(fn("cos", arg), num(2)))])]))
        return power(base, exp)
    if kind == "div":
        return div(rewrite_single_square_basis_to_allowed_terms(node[1], info), rewrite_single_square_basis_to_allowed_terms(node[2], info))
    items = []
    i = 0
    while i < len(node[1]):
        items.append(rewrite_single_square_basis_to_allowed_terms(node[1][i], info))
        i += 1
    return sim((kind, tuple(items)))


def extract_template_allowed_terms_raw(node, params, out, seen):
    node = normalise_rewrite_trig_signs(node)
    if not depends_any(node, params):
        key = sig(node)
        if key not in seen:
            seen[key] = 1
            out.append(node)
        return
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return
    if kind == "fn":
        if not depends_any(node[2], params):
            key = sig(node)
            if key not in seen:
                seen[key] = 1
                out.append(node)
        extract_template_allowed_terms_raw(node[2], params, out, seen)
        return
    if kind in ("pow", "div"):
        if not depends_any(node[1], params) and not depends_any(node[2], params):
            key = sig(node)
            if key not in seen:
                seen[key] = 1
                out.append(node)
        extract_template_allowed_terms_raw(node[1], params, out, seen)
        extract_template_allowed_terms_raw(node[2], params, out, seen)
        return
    i = 0
    while i < len(node[1]):
        extract_template_allowed_terms_raw(node[1][i], params, out, seen)
        i += 1


def add_transform_constant_candidate(out, seen, expr, steps):
    expr = force_single_fraction(normalise_rewrite_trig_signs(sim(expr)))
    key = sig(expr)
    if key in seen:
        return
    seen[key] = 1
    out.append((expr, steps))


def generate_constant_fit_candidates(source_expr, template_expr, params, var, allowed_terms=None):
    candidates = []
    seen = {}
    if allowed_terms is None:
        allowed_terms = []
        extract_template_basis_terms(template_expr, params, allowed_terms, {})
    info = build_rewrite_allowed_info(allowed_terms)
    if len(allowed_terms) != 0:
        rewritten, steps, _info = search_rewrite_expression(source_expr, allowed_terms)
        if rewritten is not None:
            add_transform_constant_candidate(candidates, seen, rewritten, steps)
        basis_double = rewrite_double_angles_to_allowed_basis(source_expr, info)
        basis_double = rewrite_ratio_square_to_allowed_basis(basis_double, info)
        basis_double = normalise_constant_fit_fraction(basis_double)
        if not same(basis_double, source_expr):
            add_transform_constant_candidate(candidates, seen, basis_double, [("Rewrite double angles using the target basis terms.", basis_double)])
    add_transform_constant_candidate(candidates, seen, source_expr, [])

    simple = full_simplify(source_expr)
    if not same(simple, source_expr):
        add_transform_constant_candidate(candidates, seen, simple, [("Simplify.", simple)])

    trig_expanded = normalise_rewrite_trig_signs(sim(expand_embedded_small(expand_trig_tree(source_expr))))
    if not same(trig_expanded, source_expr):
        add_transform_constant_candidate(candidates, seen, trig_expanded, [("Use standard trig identities.", trig_expanded)])

    expanded = normalise_rewrite_trig_signs(full_simplify(replace_exact_trig_quiet(sim(expand_embedded_small(expand_trig_tree(source_expr))))))
    if not same(expanded, source_expr):
        add_transform_constant_candidate(candidates, seen, expanded, [("Use standard trig identities.", expanded)])

    if source_expr[0] == "div":
        simplified_fraction = simplify_fraction(source_expr)
        if not same(simplified_fraction, source_expr):
            add_transform_constant_candidate(candidates, seen, simplified_fraction, [("Simplify the fraction.", simplified_fraction)])
        factored_raw, factor_note = factor_fraction_terms_for_display(source_expr)
        if factored_raw is not None:
            factored = sim(factored_raw)
            steps = [(factor_note, factored)]
            add_transform_constant_candidate(candidates, seen, factored, steps)
            cancelled = cancel_fraction_common_factor_for_display(factored_raw)
            if cancelled is not None and not same(cancelled, factored):
                add_transform_constant_candidate(candidates, seen, cancelled, steps + [("Cancel a common factor.", cancelled)])
    return candidates


def split_fit_basis_scalar(node, var):
    node = sim(node)
    if not depends(node, var):
        return node, num(1)
    if node[0] == "mul":
        scalar_bits = []
        basis_bits = []
        i = 0
        while i < len(node[1]):
            if depends(node[1][i], var):
                basis_bits.append(node[1][i])
            else:
                scalar_bits.append(node[1][i])
            i += 1
        return full_simplify(make_mul(scalar_bits)), make_mul(basis_bits)
    if node[0] == "div":
        top_scalar, top_basis = split_fit_basis_scalar(node[1], var)
        bot_scalar, bot_basis = split_fit_basis_scalar(node[2], var)
        scalar = full_simplify(div(top_scalar, bot_scalar))
        basis = full_simplify(div(top_basis, bot_basis))
        return scalar, basis
    if node[0] == "pow" and not depends(node[2], var):
        base_scalar, base_basis = split_fit_basis_scalar(node[1], var)
        if not is_one(base_scalar):
            return full_simplify(power(base_scalar, node[2])), full_simplify(power(base_basis, node[2]))
    return num(1), node


def fit_constant_basis_equations(expr, params, var, info=None):
    if info is not None and constant_fit_preserve_named_trig(info):
        expr = sim(expand_embedded_small(expr))
    else:
        expr = full_simplify(expand_embedded_small(expr))
    if info is not None:
        expr = rewrite_double_angles_to_allowed_basis(expr, info)
        expr = rewrite_ratio_square_to_allowed_basis(expr, info)
        expr = rewrite_single_square_basis_to_allowed_terms(expr, info)
        if constant_fit_preserve_named_trig(info):
            expr = force_single_fraction(normalise_rewrite_trig_signs(sim(expr)))
        else:
            expr = normalise_constant_fit_fraction(expr)
    if expr[0] == "div" and not depends_any(expr[2], params):
        expr = full_simplify(expr[1])
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    groups = {}
    i = 0
    while i < len(terms):
        term = normalise_constant_fit_fraction(terms[i])
        scalar, basis = split_fit_basis_scalar(term, var)
        if info is not None:
            basis = rewrite_double_angles_to_allowed_basis(basis, info)
            basis = rewrite_ratio_square_to_allowed_basis(basis, info)
            basis = rewrite_single_square_basis_to_allowed_terms(basis, info)
            basis = normalise_constant_fit_fraction(basis)
            scalar = normalise_constant_fit_fraction(scalar)
        if depends(basis, var):
            pass
        else:
            basis = num(1)
            scalar = term
        if depends(scalar, var):
            return None
        linear = extract_linear_param_expr(scalar, params)
        if linear is None:
            return None
        key = sig(basis)
        if key not in groups:
            groups[key] = {"basis": basis, "coeffs": zero_param_coeff_map(params), "const": num(0)}
        groups[key]["coeffs"] = add_param_coeff_maps(groups[key]["coeffs"], linear[0], params)
        groups[key]["const"] = full_simplify(add([groups[key]["const"], linear[1]]))
        i += 1
    out = []
    for key in groups:
        coeffs = groups[key]["coeffs"]
        const = groups[key]["const"]
        eq_terms = []
        i = 0
        while i < len(params):
            coeff = coeffs[params[i]]
            if not is_zero(coeff):
                eq_terms.append(mul([coeff, sym(params[i])]))
            i += 1
        if not is_zero(const):
            eq_terms.append(const)
        eq_expr = full_simplify(make_add(eq_terms))
        if not is_zero(eq_expr):
            out.append((groups[key]["basis"], eq_expr))
    out.sort(key=lambda item: (0 if is_one(item[0]) else 1, tree_size(item[0]), show(item[0])))
    return out


def solve_linear_param_system(equations, params):
    if len(equations) == 0:
        return None
    rows = []
    i = 0
    while i < len(equations):
        linear = extract_linear_param_expr(equations[i], params)
        if linear is None:
            return None
        row = []
        j = 0
        while j < len(params):
            row.append(normalise_fit_constant(linear[0][params[j]]))
            j += 1
        row.append(normalise_fit_constant(neg(linear[1])))
        rows.append(row)
        i += 1
    cols = len(params)
    pivot_row = 0
    pivot_cols = []
    col = 0
    while col < cols and pivot_row < len(rows):
        pick = -1
        r = pivot_row
        while r < len(rows):
            if not equivalent(rows[r][col], num(0)):
                pick = r
                break
            r += 1
        if pick == -1:
            col += 1
            continue
        if pick != pivot_row:
            rows[pivot_row], rows[pick] = rows[pick], rows[pivot_row]
        pivot = rows[pivot_row][col]
        c = col
        while c <= cols:
            rows[pivot_row][c] = normalise_fit_constant(div(rows[pivot_row][c], pivot))
            c += 1
        r = 0
        while r < len(rows):
            if r != pivot_row and not equivalent(rows[r][col], num(0)):
                factor = rows[r][col]
                c = col
                while c <= cols:
                    rows[r][c] = normalise_fit_constant(add([rows[r][c], neg(mul([factor, rows[pivot_row][c]]))]))
                    c += 1
            r += 1
        pivot_cols.append(col)
        pivot_row += 1
        col += 1
    r = 0
    while r < len(rows):
        all_zero = True
        c = 0
        while c < cols:
            if not equivalent(rows[r][c], num(0)):
                all_zero = False
                break
            c += 1
        if all_zero and not equivalent(rows[r][cols], num(0)):
            return None
        r += 1
    if len(pivot_cols) != cols:
        return None
    values = {}
    i = 0
    while i < len(pivot_cols):
        values[params[pivot_cols[i]]] = normalise_fit_constant(rows[i][cols])
        i += 1
    return values


def simplify_rational_vector(nodes):
    ints = []
    i = 0
    while i < len(nodes):
        if not is_num(nodes[i]):
            return nodes
        ints.append(nodes[i])
        i += 1
    lcm = 1
    i = 0
    while i < len(ints):
        lcm = abs(lcm * ints[i][2]) // gcd(lcm, ints[i][2])
        i += 1
    scaled = []
    i = 0
    while i < len(ints):
        scaled.append(ints[i][1] * (lcm // ints[i][2]))
        i += 1
    g = 0
    i = 0
    while i < len(scaled):
        g = gcd(g, abs(scaled[i]))
        i += 1
    if g == 0:
        return nodes
    out = []
    i = 0
    while i < len(scaled):
        out.append(num(scaled[i] // g))
        i += 1
    return out


def solve_homogeneous_param_ratio_system(equations, params):
    if len(equations) == 0:
        return None
    rows = []
    i = 0
    while i < len(equations):
        linear = extract_linear_param_expr(equations[i], params)
        if linear is None or not is_zero(linear[1]):
            return None
        row = []
        j = 0
        while j < len(params):
            row.append(normalise_fit_constant(linear[0][params[j]]))
            j += 1
        rows.append(row)
        i += 1
    cols = len(params)
    pivot_row = 0
    pivot_cols = []
    col = 0
    while col < cols and pivot_row < len(rows):
        pick = -1
        r = pivot_row
        while r < len(rows):
            if not equivalent(rows[r][col], num(0)):
                pick = r
                break
            r += 1
        if pick == -1:
            col += 1
            continue
        if pick != pivot_row:
            rows[pivot_row], rows[pick] = rows[pick], rows[pivot_row]
        pivot = rows[pivot_row][col]
        c = col
        while c < cols:
            rows[pivot_row][c] = normalise_fit_constant(div(rows[pivot_row][c], pivot))
            c += 1
        r = 0
        while r < len(rows):
            if r != pivot_row and not equivalent(rows[r][col], num(0)):
                factor = rows[r][col]
                c = col
                while c < cols:
                    rows[r][c] = normalise_fit_constant(add([rows[r][c], neg(mul([factor, rows[pivot_row][c]]))]))
                    c += 1
            r += 1
        pivot_cols.append(col)
        pivot_row += 1
        col += 1
    free_cols = []
    c = 0
    while c < cols:
        if c not in pivot_cols:
            free_cols.append(c)
        c += 1
    if len(free_cols) != 1:
        return None
    free_col = free_cols[0]
    vector = []
    c = 0
    while c < cols:
        vector.append(num(0))
        c += 1
    vector[free_col] = num(1)
    r = len(pivot_cols) - 1
    while r >= 0:
        pivot_col = pivot_cols[r]
        total = num(0)
        c = pivot_col + 1
        while c < cols:
            if not equivalent(rows[r][c], num(0)) and not is_zero(vector[c]):
                total = normalise_fit_constant(add([total, mul([rows[r][c], vector[c]])]))
            c += 1
        vector[pivot_col] = normalise_fit_constant(neg(total))
        r -= 1
    vector = simplify_rational_vector(vector)
    out = {}
    i = 0
    while i < len(params):
        out[params[i]] = vector[i]
        i += 1
    return out


def solve_linear_param_system_by_sampling(equations, params, var):
    chosen = []
    chosen_text = []
    values = None
    i = 0
    while i < len(FIT_TEMPLATE_SAMPLE_ANGLES):
        env = {var: FIT_TEMPLATE_SAMPLE_ANGLES[i]}
        sample_eqs = []
        j = 0
        while j < len(equations):
            sampled = normalise_fit_constant(substitute_symbols(equations[j], env))
            if depends_any(sampled, (var,)):
                return None, None
            if not is_zero(sampled):
                sample_eqs.append(sampled)
            j += 1
        if len(sample_eqs) != 0:
            k = 0
            while k < len(sample_eqs):
                chosen.append(sample_eqs[k])
                chosen_text.append(show(FIT_TEMPLATE_SAMPLE_ANGLES[i]))
                k += 1
            values = solve_linear_param_system(chosen, params)
            if values is not None:
                return values, chosen_text
        i += 1
    return None, None


def render_constant_fit_equations(basis_equations):
    out = []
    i = 0
    while i < len(basis_equations):
        basis, eq_expr = basis_equations[i]
        if is_one(basis):
            out.append("Constant term: " + equation_line(eq_expr, num(0)))
        else:
            out.append("coeff of " + show(basis) + ": " + equation_line(eq_expr, num(0)))
        i += 1
    return out


def preferred_positive_ratio_params(template_expr, params):
    expr = sim(template_expr)
    if expr[0] == "div":
        linear = extract_linear_param_expr(expr[2], params)
        if linear is not None:
            out = []
            i = 0
            while i < len(params):
                if equivalent(linear[0][params[i]], num(1)):
                    out.append(params[i])
                i += 1
            if len(out) != 0:
                return out
    if len(params) == 0:
        return []
    return [params[0]]


def combine_fraction_sum_once(node):
    node = sim(node)
    if node[0] != "add":
        return None
    terms = list(flat(node, "add"))
    if len(terms) < 2:
        return None
    denoms = []
    seen = {}
    i = 0
    while i < len(terms):
        part = match_scaled_div_norm(terms[i])
        denom = num(1) if part is None else part[2]
        key = sig(denom)
        if key not in seen and not is_one(denom):
            seen[key] = 1
            denoms.append(denom)
        i += 1
    if len(denoms) == 0:
        return None
    common = make_mul(denoms)
    numer_terms = []
    i = 0
    while i < len(terms):
        part = match_scaled_div_norm(terms[i])
        if part is None:
            coeff = num(1)
            top = terms[i]
            denom = num(1)
        else:
            coeff, top, denom = part
        factors = []
        j = 0
        while j < len(denoms):
            if sig(denoms[j]) != sig(denom):
                factors.append(denoms[j])
            j += 1
        numer_terms.append(full_simplify(mul([coeff, top, make_mul(factors)])))
        i += 1
    return ("div", make_add(numer_terms), common)


def force_single_fraction(node):
    node = sim(node)
    if node[0] != "add":
        return node
    terms = list(flat(node, "add"))
    denoms = []
    parts = []
    seen = {}
    any_frac = False
    i = 0
    while i < len(terms):
        part = match_scaled_div_norm(terms[i])
        if part is None:
            coeff = num(1)
            top = terms[i]
            denom = num(1)
        else:
            coeff, top, denom = part
            any_frac = True
        parts.append((coeff, top, denom))
        key = sig(denom)
        if not is_one(denom) and key not in seen:
            seen[key] = 1
            denoms.append(denom)
        i += 1
    if not any_frac:
        return node
    common = make_mul(denoms)
    numer_terms = []
    i = 0
    while i < len(parts):
        coeff, top, denom = parts[i]
        scale = num(1) if is_one(denom) else full_simplify(div(common, denom))
        numer_terms.append(full_simplify(mul([coeff, top, scale])))
        i += 1
    return sim(div(make_add(numer_terms), common))


def normalise_constant_fit_fraction(node, params=None):
    node = sim(node)
    if params is not None and node[0] == "div" and depends_any(node[2], params):
        node = ("div", full_simplify(node[1]), full_simplify(node[2]))
    else:
        node = full_simplify(node)
    i = 0
    while i < 6:
        combined = force_single_fraction(node)
        if combined is None or same(combined, node):
            combined = combine_fraction_sum_once(node)
        if combined is None or same(combined, node):
            break
        node = combined
        i += 1
    if node[0] == "div" and not (params is not None and depends_any(node[2], params)):
        factored_raw, _note = factor_fraction_terms_for_display(node)
        if factored_raw is not None:
            cancelled = cancel_fraction_common_factor_for_display(factored_raw)
            if cancelled is not None:
                node = full_simplify(cancelled)
            else:
                node = full_simplify(sim(factored_raw))
        simplified = simplify_fraction(node)
        if simplified[0] == "div":
            node = simplified
        else:
            node = force_single_fraction(full_simplify(simplified))
    if params is not None and node[0] == "div" and depends_any(node[2], params):
        return normalise_rewrite_trig_signs(node)
    node = force_single_fraction(full_simplify(node))
    return normalise_rewrite_trig_signs(node)


def generate_constant_fit_template_variants(template_expr, params, var):
    variants = [(template_expr, {}, params[:])]
    expr = sim(template_expr)
    if expr[0] != "mul":
        return variants
    items = list(flat(expr, "mul"))
    i = 0
    while i < len(items):
        item = items[i]
        if item[0] == "sym" and item[1] in params:
            rest = make_mul(items[:i] + items[i + 1 :])
            remaining = []
            j = 0
            while j < len(params):
                if params[j] != item[1]:
                    remaining.append(params[j])
                j += 1
            if len(remaining) != 0:
                linear = extract_linear_param_expr(rest, remaining)
                if linear is not None:
                    variants.append((rest, {item[1]: num(1)}, remaining))
        i += 1
    return variants


def transform_fit_constant_template(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq1_text, eq2_text):
    source_expr = sim(add([eq1_lhs, neg(eq1_rhs)]))
    template_expr = sim(add([eq2_lhs, neg(eq2_rhs)]))
    template_raw_expr = add([eq2_lhs, neg(eq2_rhs)])
    var = detect_transform_var(eq1_lhs, eq1_rhs)
    params = detect_template_params(source_expr, template_raw_expr, var)
    if len(params) == 0:
        return None

    template_variants = generate_constant_fit_template_variants(template_raw_expr, params, var)
    variant_index = 0
    while variant_index < len(template_variants):
        template_variant, fixed_values, fit_params = template_variants[variant_index]
        allowed_terms = [num(1)]
        extract_template_allowed_terms_raw(template_variant, fit_params, allowed_terms, {sig(num(1)): 1})
        info = build_rewrite_allowed_info(allowed_terms)
        candidates = generate_constant_fit_candidates(source_expr, template_variant, fit_params, var, allowed_terms)
        i = 0
        while i < len(candidates):
            candidate_expr, candidate_steps = candidates[i]
            candidate_expr = rewrite_double_angles_to_allowed_basis(candidate_expr, info)
            candidate_expr = rewrite_ratio_square_to_allowed_basis(candidate_expr, info)
            candidate_expr = rewrite_single_square_basis_to_allowed_terms(candidate_expr, info)
            if constant_fit_preserve_named_trig(info):
                candidate_expr = force_single_fraction(normalise_rewrite_trig_signs(sim(candidate_expr)))
            else:
                candidate_expr = normalise_constant_fit_fraction(candidate_expr)
            template_now = force_single_fraction(normalise_rewrite_trig_signs(sim(template_variant)))
            left_num, left_den = split_fraction_term(candidate_expr)
            right_num, right_den = split_fraction_term(template_now)
            cross_left = full_simplify(mul([left_num, right_den]))
            cross_right = full_simplify(mul([right_num, left_den]))
            fit_expr = full_simplify(add([cross_left, neg(cross_right)]))
            fit_expr = full_simplify(expand_embedded_small(fit_expr))
            fit_expr = rewrite_double_angles_to_allowed_basis(fit_expr, info)
            fit_expr = rewrite_ratio_square_to_allowed_basis(fit_expr, info)
            fit_expr = rewrite_single_square_basis_to_allowed_terms(fit_expr, info)
            if constant_fit_preserve_named_trig(info):
                fit_expr = force_single_fraction(normalise_rewrite_trig_signs(sim(fit_expr)))
            else:
                fit_expr = normalise_constant_fit_fraction(fit_expr)
            basis_equations = fit_constant_basis_equations(fit_expr, fit_params, var, info)
            if basis_equations is None:
                i += 1
                continue
            equation_exprs = []
            j = 0
            while j < len(basis_equations):
                equation_exprs.append(basis_equations[j][1])
                j += 1
            values = solve_linear_param_system(equation_exprs, fit_params)
            sampled_angles = None
            if values is None:
                ratio_values = solve_homogeneous_param_ratio_system(equation_exprs, fit_params)
                if ratio_values is not None:
                    ratio_options = [ratio_values]
                    neg_values = {}
                    j = 0
                    while j < len(fit_params):
                        neg_values[fit_params[j]] = normalise_fit_constant(neg(ratio_values[fit_params[j]]))
                        j += 1
                    ratio_options.append(neg_values)
                    best_values = None
                    best_score = None
                    preferred_positive = preferred_positive_ratio_params(template_now, fit_params)
                    j = 0
                    while j < len(ratio_options):
                        candidate_values = ratio_options[j]
                        if constant_fit_preserve_named_trig(info):
                            candidate_fitted = force_single_fraction(normalise_rewrite_trig_signs(sim(substitute_symbols(template_now, candidate_values))))
                        else:
                            candidate_fitted = normalise_constant_fit_fraction(substitute_symbols(template_now, candidate_values))
                        if equivalent(candidate_expr, candidate_fitted) or rewrite_verified_equivalent(candidate_fitted, candidate_expr):
                            preferred_bad = 1
                            k = 0
                            while k < len(preferred_positive):
                                try:
                                    value = eval_numeric(candidate_values[preferred_positive[k]], {})
                                except Exception:
                                    value = None
                                if value is not None and value > 0:
                                    preferred_bad = 0
                                    break
                                k += 1
                            score = (preferred_bad, len(show(candidate_fitted)), tree_size(candidate_fitted))
                            if best_score is None or score < best_score:
                                best_score = score
                                best_values = candidate_values
                        j += 1
                    values = best_values
            if values is None:
                values, sampled_angles = solve_linear_param_system_by_sampling(equation_exprs, fit_params, var)
            if values is None:
                i += 1
                continue
            all_values = {}
            j = 0
            while j < len(params):
                if params[j] in fixed_values:
                    all_values[params[j]] = fixed_values[params[j]]
                elif params[j] in values:
                    all_values[params[j]] = values[params[j]]
                j += 1
            if constant_fit_preserve_named_trig(info):
                fitted = force_single_fraction(normalise_rewrite_trig_signs(sim(substitute_symbols(template_expr, all_values))))
            else:
                fitted = normalise_constant_fit_fraction(substitute_symbols(template_expr, all_values))
            if not (equivalent(candidate_expr, fitted) or rewrite_verified_equivalent(fitted, candidate_expr)):
                i += 1
                continue
            if not (equivalent(source_expr, fitted) or rewrite_verified_equivalent(fitted, source_expr)):
                i += 1
                continue

            raw_left, raw_right = split_equation_text_raw_or_zero(eq1_text)
            target_left, target_right = split_equation_text_raw_or_zero(eq2_text)
            lines = []
            if raw_right == "0":
                lines.append("Start with " + raw_left + ".")
            else:
                lines.append("Equation 1: " + equation_line(eq1_lhs, eq1_rhs))
            if len(candidate_steps) != 0:
                j = 0
                cur = source_expr
                while j < len(candidate_steps):
                    note, nxt = candidate_steps[j]
                    if note not in ("", None):
                        lines.append(note)
                    if not same(nxt, cur):
                        lines.append(step_line(nxt))
                        cur = nxt
                    j += 1
            elif not same(candidate_expr, source_expr):
                lines.append("Simplify into the target terms.")
                lines.append(step_line(candidate_expr))
            lines.append("Compare with " + eq2_text.strip() + ".")
            if not is_one(left_den) or not is_one(right_den):
                lines.append("Clear the denominator.")
                lines.append(equation_line(cross_left, cross_right))
            if not is_zero(fit_expr):
                lines.append("Expand and simplify.")
                lines.append(equation_line(fit_expr, num(0)))
            basis_lines = render_constant_fit_equations(basis_equations)
            j = 0
            while j < len(basis_lines):
                lines.append(basis_lines[j])
                j += 1
            if sampled_angles is not None:
                lines.append("Use exact angle values to solve the constant equations.")
            bits = []
            j = 0
            while j < len(params):
                bits.append(params[j] + " = " + show(all_values[params[j]]))
                j += 1
            lines.append(", ".join(bits))
            if target_right == "0":
                lines.append("Hence " + show(substitute_symbols(eq2_lhs, all_values)))
            else:
                lines.append("Hence " + equation_line(substitute_symbols(eq2_lhs, all_values), substitute_symbols(eq2_rhs, all_values)))
            return compact_lines(lines)
            i += 1
        variant_index += 1
    return None


def match_constant_factor_template(expr, var):
    expr = sim(expr)
    names = symbols_of(expr)
    params = []
    i = 0
    while i < len(names):
        if names[i] != var and not names[i][:1].islower():
            params.append(names[i])
        i += 1
    if len(params) == 0:
        return None
    items = list(flat(expr, "mul")) if expr[0] == "mul" else [expr]
    inner = None
    outer_bits = []
    i = 0
    while i < len(items):
        if depends_any(items[i], params):
            if inner is not None:
                return None
            inner = items[i]
        else:
            outer_bits.append(items[i])
        i += 1
    if inner is None:
        return None
    terms = list(flat(inner, "add")) if inner[0] == "add" else [inner]
    basis = {}
    order = []
    i = 0
    while i < len(terms):
        coeff, rest = split_coeff(terms[i])
        bits = list(flat(rest, "mul")) if rest[0] == "mul" else [rest]
        pick = None
        remain = []
        j = 0
        while j < len(bits):
            if bits[j][0] == "sym" and bits[j][1] in params:
                if pick is not None:
                    return None
                pick = bits[j][1]
            else:
                remain.append(bits[j])
            j += 1
        if pick is None:
            return None
        basis_expr = make_mul(remain)
        if not is_one(coeff):
            basis_expr = full_simplify(mul([coeff, basis_expr]))
        basis[pick] = basis_expr
        order.append(pick)
        i += 1
    basis_name = None
    basis_arg = None
    i = 0
    while i < len(order):
        cur = basis[order[i]]
        if not is_one(cur):
            base_node = cur
            if base_node[0] == "pow" and same(base_node[2], num(2)):
                base_node = base_node[1]
            if base_node[0] == "fn" and base_node[1] in ("sin", "cos"):
                basis_name = base_node[1]
                basis_arg = base_node[2]
                break
        i += 1
    if basis_name is None or basis_arg is None:
        return None
    order = sorted(order)
    return {
        "outer": make_mul(outer_bits),
        "basis": basis,
        "params": order,
        "basis_name": basis_name,
        "basis_arg": basis_arg,
        "template_expr": expr,
    }


def match_inner_to_template_constants(inner, basis_map, param_order):
    terms = list(flat(inner, "add")) if inner[0] == "add" else [inner]
    values = {}
    seen = {}
    i = 0
    while i < len(terms):
        coeff, rest = split_coeff(terms[i])
        match_name = None
        j = 0
        while j < len(param_order):
            basis = basis_map[param_order[j]]
            if (is_one(basis) and is_one(rest)) or equivalent(rest, basis):
                match_name = param_order[j]
                break
            j += 1
        if match_name is None or match_name in seen:
            return None
        seen[match_name] = 1
        values[match_name] = full_simplify(coeff)
        i += 1
    i = 0
    while i < len(param_order):
        if param_order[i] not in seen:
            values[param_order[i]] = num(0)
        i += 1
    return values


def substitute_symbols(node, values):
    if node[0] == "sym" and node[1] in values:
        return values[node[1]]
    if node[0] in ("num", "const", "sym"):
        return node
    if node[0] == "fn":
        return fn(node[1], substitute_symbols(node[2], values))
    if node[0] == "pow":
        return power(substitute_symbols(node[1], values), substitute_symbols(node[2], values))
    if node[0] == "div":
        return div(substitute_symbols(node[1], values), substitute_symbols(node[2], values))
    out = []
    i = 0
    while i < len(node[1]):
        out.append(substitute_symbols(node[1][i], values))
        i += 1
    return sim((node[0], tuple(out)))


def transform_constant_factor_template(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text):
    # This handles exam questions that ask for an equation to be rewritten as
    # sin(...)*(A*cos(...)^2 + B*cos(...) + C) = 0 and then asks for A, B, C.
    var = detect_transform_var(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs)
    expr1 = sim(add([eq1_lhs, neg(eq1_rhs)]))
    expr2 = sim(add([eq2_lhs, neg(eq2_rhs)]))
    template = match_constant_factor_template(expr2, var)
    if template is None or template["outer"][0] != "fn" or template["outer"][1] != "sin":
        return None
    rewritten = rewrite_tan_double_factor_polynomial_expr(expr1, var, template["basis_name"])
    if rewritten is None:
        return None
    new_expr, extra = rewritten
    items = list(flat(new_expr, "mul")) if new_expr[0] == "mul" else [new_expr]
    inner = None
    outer_bits = []
    i = 0
    while i < len(items):
        if items[i][0] == "add":
            if inner is not None:
                return None
            inner = items[i]
        else:
            outer_bits.append(items[i])
        i += 1
    if inner is None:
        return None
    outer_now = make_mul(outer_bits)
    if not equivalent(outer_now, template["outer"]):
        return None
    values = match_inner_to_template_constants(inner, template["basis"], template["params"])
    if values is None:
        return None
    final_expr = full_simplify(substitute_symbols(expr2, values))
    if not equivalent(new_expr, final_expr):
        return None
    lines = [
        "Equation 1: " + equation_line(eq1_lhs, eq1_rhs),
        "Move all terms to one side.",
        equation_line(expr1, num(0)),
    ]
    i = 0
    while i < len(extra):
        lines.append(extra[i])
        i += 1
    lines.append("Compare with " + eq2_text.strip() + ".")
    bits = []
    i = 0
    while i < len(template["params"]):
        bits.append(template["params"][i] + " = " + show(values[template["params"][i]]))
        i += 1
    lines.append(", ".join(bits))
    lines.append("Hence " + equation_line(substitute_symbols(eq2_lhs, values), substitute_symbols(eq2_rhs, values)))
    return compact_lines(lines)


def solve_transform_text(eq1_text, eq2_text):
    begin_user_action()
    eq1_lhs, eq1_rhs = parse_equation_or_zero(eq1_text)
    eq2_lhs, eq2_rhs = parse_equation_or_zero(eq2_text)
    expression_only = is_zero(eq1_rhs) and is_zero(eq2_rhs)
    if same(eq1_lhs, eq2_lhs) and expression_only:
        return [show(eq1_lhs), '= ' + show(eq2_lhs)]
    if same(eq1_lhs, eq2_lhs) and same(eq1_rhs, eq2_rhs):
        return [equation_line(eq1_lhs, eq1_rhs)]
    if equivalent(eq1_lhs, eq2_lhs) and not same(eq1_lhs, eq2_lhs) and same(eq2_lhs, num(1)) and eq1_lhs[0] == 'div' and same(eq1_lhs[1], eq1_lhs[2]) and has_variable_dependency(eq1_lhs[1]):
        raise ValueError('Equation 2 does not match Equation 1 under domain restrictions.')
    if equivalent(eq1_lhs, eq2_lhs) and equivalent(eq1_rhs, eq2_rhs) and not expression_only:
        return [equation_line(eq1_lhs, eq1_rhs)]



















































































    if expression_only:
        if eq1_lhs[0] == 'div' and eq1_lhs[1][0] == 'fn' and eq1_lhs[1][1] == 'sin' and eq1_lhs[2][0] == 'fn' and eq1_lhs[2][1] == 'cos' and eq2_lhs[0] == 'fn' and eq2_lhs[1] == 'tan' and same(eq1_lhs[1][2], eq2_lhs[2]) and same(eq1_lhs[2][2], eq2_lhs[2]):
            return [show(eq1_lhs), 'Use sin A / cos A = tan A.', '= ' + show(eq2_lhs)]
        if eq1_lhs[0] == 'div' and is_one(eq1_lhs[1]) and eq1_lhs[2][0] == 'fn' and eq1_lhs[2][1] == 'cos' and eq2_lhs[0] == 'fn' and eq2_lhs[1] == 'sec' and same(eq1_lhs[2][2], eq2_lhs[2]):
            return [show(eq1_lhs), 'Use 1 / cos A = sec A.', '= ' + show(eq2_lhs)]
        if eq1_lhs[0] == 'div' and is_one(eq1_lhs[1]) and eq1_lhs[2][0] == 'fn' and eq1_lhs[2][1] == 'sin' and eq2_lhs[0] == 'fn' and eq2_lhs[1] == 'cosec' and same(eq1_lhs[2][2], eq2_lhs[2]):
            return [show(eq1_lhs), 'Use 1 / sin A = cosec A.', '= ' + show(eq2_lhs)]
        if eq1_lhs[0] == 'div' and eq1_lhs[1][0] == 'fn' and eq1_lhs[1][1] == 'cos' and eq1_lhs[2][0] == 'fn' and eq1_lhs[2][1] == 'sin' and eq2_lhs[0] == 'fn' and eq2_lhs[1] == 'cot' and same(eq1_lhs[1][2], eq2_lhs[2]) and same(eq1_lhs[2][2], eq2_lhs[2]):
            return [show(eq1_lhs), 'Use cos A / sin A = cot A.', '= ' + show(eq2_lhs)]
        lines = transform_half_angle_ratio(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs)
        if lines is not None:
            return compact_lines(lines)
        if eq1_lhs[0] == 'add' and eq2_lhs[0] == 'fn' and eq2_lhs[1] == 'cos':
            if equivalent(eq1_lhs, eq2_lhs):
                return [show(eq1_lhs), 'Use cos(2A) = cos^2 A - sin^2 A.', '= ' + show(eq2_lhs)]
        if eq2_lhs[0] == 'mul' and eq2_lhs[1][0] == 'num' and eq2_lhs[1][0] == 'num':
            pass
    source_arg_symbols = set()
    target_arg_symbols = set()
    collect_trig_argument_lower_symbols(eq1_lhs, source_arg_symbols)
    collect_trig_argument_lower_symbols(eq1_rhs, source_arg_symbols)
    collect_trig_argument_lower_symbols(eq2_lhs, target_arg_symbols)
    collect_trig_argument_lower_symbols(eq2_rhs, target_arg_symbols)
    if len(source_arg_symbols) != 0 and len(target_arg_symbols) != 0 and source_arg_symbols.isdisjoint(target_arg_symbols):
        raise ValueError("Equation 2 uses a different trig variable.")
    lines = prove_cot_quadratic_equation(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    lines = transform_constant_factor_template(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    lines = transform_linear_combo_shift(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    lines = transform_factor_quadratic(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    lines = transform_same_angle_ratio(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    # Try R sin(x+alpha) or R cos(x+alpha) form
    lines = transform_r_sin_cos_form(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    # Try a sin(2x) + b cos(2x) + c form
    lines = transform_sin_cos_2x_form(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    for rewriter in (
        solve_square_sum_reduction_expr,
        solve_pythag_single_expr,
        solve_weighted_pythag_expr,
        solve_cos_double_single_expr,
        solve_sin_double_single_expr,
        rewrite_tan_double_factor_polynomial_expr,
        solve_sin_tan_expr,
        solve_cos_cot_expr,
        solve_sin_cot_expr,
        solve_cos_tan_expr,
        solve_sin2_tan_expr,
        solve_sin2_cot_expr,
        solve_sec_tan_mix_expr,
        solve_cosec_cot_mix_expr,
        rewrite_half_angle_ratio_terms,
        rewrite_sum_product_terms,
        rewrite_product_to_sum_terms,
        solve_double_product_reduction_expr,
    ):
        lines = transform_zero_form_rewrite(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq2_text, rewriter)
        if lines is not None:
            return compact_lines(lines)
    lines = transform_fit_constant_template(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, eq1_text, eq2_text)
    if lines is not None:
        return compact_lines(lines)
    expr1 = sim(add([eq1_lhs, neg(eq1_rhs)]))
    expr2 = sim(add([eq2_lhs, neg(eq2_rhs)]))
    cot_quad = derive_cot_quadratic_expr(eq1_lhs, eq1_rhs)
    if cot_quad is not None:
        final_expr = cot_quad[4]
        if not (equivalent(final_expr, expr2) or equivalent(final_expr, neg(expr2))):
            raise ValueError(
                "Equation 2 does not match Equation 1. The supported rewrite gives "
                + equation_line(final_expr, num(0))
            )
    if not equivalent(expr1, expr2):
        raise ValueError("Equation 2 does not match Equation 1.")
    proof = solve_prove(expr1, expr2, "1")
    lines = [
        equation_line(eq1_lhs, eq1_rhs),
        equation_line(expr1, num(0)),
        equation_line(expr2, num(0)),
    ]
    i = 0
    while i < len(proof):
        lines.append(proof[i])
        i += 1
    lines.append("Hence " + eq2_text.strip())
    return compact_lines(lines)


def parse_equation_or_zero(text):
    parts = split_top_level(text, "=")
    if parts is None:
        return parse(text.strip()), num(0)
    return parse(parts[0].strip()), parse(parts[1].strip())


# ---------------------------------------------------------------------------
# Rewrite mode
# ---------------------------------------------------------------------------
TRIG_REWRITE_FN_NAMES = ("sin", "cos", "tan", "sec", "cosec", "cot")
REWRITE_SEARCH_DEPTH = 7
REWRITE_BEAM_WIDTH = 20


def is_lowercase_symbol_name(name):
    return name != "" and name[:1].islower()


def build_rewrite_allowed_info(term_nodes):
    terms = []
    term_sigs = {}
    trig_terms = {}

    def register_nested_trig(node):
        node = sim(node)
        kind = node[0]
        if kind in ("num", "sym", "const"):
            return
        if kind == "fn":
            if node[1] in TRIG_REWRITE_FN_NAMES:
                arg_key = sig(sim(node[2]))
                if arg_key not in trig_terms:
                    trig_terms[arg_key] = {"arg": sim(node[2]), "names": {}}
                trig_terms[arg_key]["names"][node[1]] = node
            register_nested_trig(node[2])
            return
        if kind in ("pow", "div"):
            register_nested_trig(node[1])
            register_nested_trig(node[2])
            return
        i = 0
        while i < len(node[1]):
            register_nested_trig(node[1][i])
            i += 1

    i = 0
    while i < len(term_nodes):
        term = sim(term_nodes[i])
        key = sig(term)
        if key not in term_sigs:
            term_sigs[key] = term
            terms.append(term)
            register_nested_trig(term)
        i += 1
    return {"terms": terms, "term_sigs": term_sigs, "trig_terms": trig_terms}


def rewrite_term_exact(node, info):
    return sig(sim(node)) in info["term_sigs"]


def normalise_rewrite_trig_signs(node):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        arg = normalise_rewrite_trig_signs(node[2])
        if node[1] in ("sin", "cos", "tan", "sec", "cosec", "cot"):
            coeff, rest = split_coeff(arg)
            if is_num(coeff) and coeff[1] < 0:
                pos_coeff = num(-coeff[1], coeff[2])
                pos_arg = rest if is_one(pos_coeff) else sim(mul([pos_coeff, rest]))
                if node[1] in ("cos", "sec"):
                    return fn(node[1], pos_arg)
                return sim(neg(fn(node[1], pos_arg)))
        return fn(node[1], arg)
    if kind == "pow":
        return power(normalise_rewrite_trig_signs(node[1]), normalise_rewrite_trig_signs(node[2]))
    if kind == "div":
        return div(normalise_rewrite_trig_signs(node[1]), normalise_rewrite_trig_signs(node[2]))
    items = []
    i = 0
    while i < len(node[1]):
        items.append(normalise_rewrite_trig_signs(node[1][i]))
        i += 1
    return sim((kind, tuple(items)))


def rewrite_allowed_has_fn(info, name, arg):
    arg = sim(arg)
    key = sig(arg)
    group = info["trig_terms"].get(key)
    if group is not None and name in group["names"]:
        return True
    for existing in info["trig_terms"].values():
        if name in existing["names"] and same(existing["arg"], arg):
            return True
    return False


def rewrite_allowed_has_power(info, name, arg, exponent):
    arg = sim(arg)
    exponent = sim(exponent)
    i = 0
    while i < len(info["terms"]):
        term = info["terms"][i]
        if term[0] == "pow" and same(term[2], exponent):
            base = sim(term[1])
            if base[0] == "fn" and base[1] == name and same(base[2], arg):
                return True
        i += 1
    return False


def constant_fit_preserve_named_trig(info):
    for group in info["trig_terms"].values():
        if "tan" in group["names"] or "cot" in group["names"]:
            return True
    return False


def rewrite_allowed_only(node, info):
    node = sim(node)
    if rewrite_term_exact(node, info):
        return True
    kind = node[0]
    if kind in ("num", "const"):
        return True
    if kind == "sym":
        return not is_lowercase_symbol_name(node[1])
    if kind == "fn":
        if node[1] in TRIG_REWRITE_FN_NAMES:
            return False
        return rewrite_allowed_only(node[2], info)
    if kind in ("pow", "div"):
        return rewrite_allowed_only(node[1], info) and rewrite_allowed_only(node[2], info)
    i = 0
    while i < len(node[1]):
        if not rewrite_allowed_only(node[1][i], info):
            return False
        i += 1
    return True


def rewrite_burden(node, info):
    node = sim(node)
    if rewrite_term_exact(node, info):
        return 0
    kind = node[0]
    if kind in ("num", "const"):
        return 0
    if kind == "sym":
        return 8 if is_lowercase_symbol_name(node[1]) else 0
    if kind == "fn":
        if node[1] in TRIG_REWRITE_FN_NAMES:
            return 20 + rewrite_burden(node[2], info)
        return rewrite_burden(node[2], info)
    if kind in ("pow", "div"):
        return rewrite_burden(node[1], info) + rewrite_burden(node[2], info)
    total = 0
    i = 0
    while i < len(node[1]):
        total += rewrite_burden(node[1][i], info)
        i += 1
    return total


def rewrite_local_to_allowed_terms(node, info):
    node = sim(node)
    if rewrite_term_exact(node, info):
        return None, None

    if node[0] == "fn":
        name = node[1]
        arg = node[2]
        if name in ("tan", "cot", "sec", "cosec"):
            if rewrite_allowed_has_fn(info, "sin", arg) or rewrite_allowed_has_fn(info, "cos", arg):
                return reciprocal_trig(node), "Write " + name + "(A) in terms of sin(A) and cos(A)."
            if name == "tan" and rewrite_allowed_has_fn(info, "cot", arg):
                return div(num(1), fn("cot", arg)), "Rewrite tan(A) as 1/cot(A)."
            if name == "cot" and rewrite_allowed_has_fn(info, "tan", arg):
                return div(num(1), fn("tan", arg)), "Rewrite cot(A) as 1/tan(A)."
        if name == "sin":
            double_base = match_numeric_multiple_arg(arg, 2)
            if double_base is not None:
                if rewrite_allowed_has_fn(info, "tan", double_base):
                    tan_base = fn("tan", double_base)
                    return div(mul([num(2), tan_base]), add([num(1), power(tan_base, num(2))])), "Use sin(2A) = 2tan(A)/(1+tan^2(A))."
                if rewrite_allowed_has_fn(info, "cot", double_base):
                    cot_base = fn("cot", double_base)
                    return div(mul([num(2), cot_base]), add([power(cot_base, num(2)), num(1)])), "Use sin(2A) = 2cot(A)/(1+cot^2(A))."
            if rewrite_allowed_has_fn(info, "tan", arg) and rewrite_allowed_has_fn(info, "sec", arg):
                return div(fn("tan", arg), fn("sec", arg)), "Rewrite sin(A) as tan(A)/sec(A)."
            if rewrite_allowed_has_fn(info, "cosec", arg):
                return div(num(1), fn("cosec", arg)), "Rewrite sin(A) as 1/cosec(A)."
        if name == "cos":
            if rewrite_allowed_has_fn(info, "sec", arg):
                return div(num(1), fn("sec", arg)), "Rewrite cos(A) as 1/sec(A)."
            if rewrite_allowed_has_fn(info, "cot", arg) and rewrite_allowed_has_fn(info, "cosec", arg):
                return div(fn("cot", arg), fn("cosec", arg)), "Rewrite cos(A) as cot(A)/cosec(A)."
        rewritten, note = direct_double_angle_rewrite(node)
        if rewritten is not None and rewrite_burden(rewritten, info) < rewrite_burden(node, info):
            return rewritten, note
        return None, None
    if node[0] == "pow" and is_int_num(node[2]) and node[1][0] == "fn":
        name = node[1][1]
        arg = node[1][2]
        exp_val = node[2][1]
        double_arg = sim(mul([num(2), arg]))
        if exp_val == 2:
            if name in ("sin", "cos") and rewrite_allowed_has_fn(info, "cos", double_arg):
                rewritten, note = power_reduction_once(node)
                if rewritten is not None:
                    return rewritten, note
            if name == "tan" and rewrite_allowed_has_fn(info, "sec", arg):
                rewritten, note = power_reduction_once(node)
                if rewritten is not None:
                    return rewritten, note
            if name == "cot" and rewrite_allowed_has_fn(info, "cosec", arg):
                rewritten, note = power_reduction_once(node)
                if rewritten is not None:
                    return rewritten, note
            if name == "sin" and rewrite_allowed_has_fn(info, "cos", arg) and not rewrite_allowed_has_fn(info, "sin", arg):
                return add([num(1), neg(power(fn("cos", arg), num(2)))]), "Use sin^2(A) = 1-cos^2(A)."
            if name == "cos" and rewrite_allowed_has_fn(info, "sin", arg) and not rewrite_allowed_has_fn(info, "cos", arg):
                return add([num(1), neg(power(fn("sin", arg), num(2)))]), "Use cos^2(A) = 1-sin^2(A)."
        if exp_val > 2 and exp_val % 2 == 0 and name in ("sin", "cos"):
            square = power(node[1], num(2))
            rewritten_square, note = rewrite_local_to_allowed_terms(square, info)
            if rewritten_square is not None and rewrite_allowed_only(rewritten_square, info):
                return power(rewritten_square, num(exp_val // 2)), note
    if node[0] == "pow" and is_int_num(node[2]) and node[2][1] > 0:
        exp_val = node[2][1]
        if node[1][0] == "div":
            tan2_info = match_tan_squared_fraction(node[1])
            if tan2_info is not None and rewrite_allowed_has_fn(info, "tan", tan2_info[1]):
                coeff, base = tan2_info
                rewritten = power(fn("tan", base), num(2 * exp_val))
                if not is_one(coeff):
                    rewritten = sim(mul([power(coeff, num(exp_val)), rewritten]))
                return rewritten, "Use (1-cos(2A))/(1+cos(2A)) = tan^2(A)."
            cot2_info = match_cot_squared_fraction(node[1])
            if cot2_info is not None and rewrite_allowed_has_fn(info, "cot", cot2_info[1]):
                coeff, base = cot2_info
                rewritten = power(fn("cot", base), num(2 * exp_val))
                if not is_one(coeff):
                    rewritten = sim(mul([power(coeff, num(exp_val)), rewritten]))
                return rewritten, "Use (1+cos(2A))/(1-cos(2A)) = cot^2(A)."
        reduced_base = full_simplify(reduce_sec_cosec_squares(reduce_identities(node[1])))
        if not same(reduced_base, node[1]):
            return power(reduced_base, node[2]), "Use identities inside the power."

    if node[0] == "mul":
        product = match_same_angle_sin_cos_product(node)
        if product is not None:
            coeff, base = product
            double_arg = sim(mul([num(2), base]))
            if rewrite_allowed_has_fn(info, "sin", double_arg):
                return sim(mul([coeff, div(fn("sin", double_arg), num(2))])), "Use 2sin(A)cos(A) = sin(2A)."
        items = list(flat(node, "mul"))
        i = 0
        while i < len(items):
            item_i = items[i]
            if item_i[0] == "pow" and is_int_num(item_i[2]) and item_i[2][1] > 0 and item_i[1][0] == "fn" and item_i[1][1] == "sin":
                j = i + 1
                while j < len(items):
                    item_j = items[j]
                    if item_j[0] == "pow" and is_int_num(item_j[2]) and same(item_j[2], item_i[2]) and item_j[1][0] == "fn" and item_j[1][1] == "cos" and same(item_j[1][2], item_i[1][2]):
                        double_arg = sim(mul([num(2), item_i[1][2]]))
                        if rewrite_allowed_has_fn(info, "sin", double_arg):
                            coeff_items = []
                            k = 0
                            while k < len(items):
                                if k != i and k != j:
                                    coeff_items.append(items[k])
                                k += 1
                            base = power(div(fn("sin", double_arg), num(2)), item_i[2])
                            rewritten = base if len(coeff_items) == 0 else sim(mul(coeff_items + [base]))
                            return rewritten, "Use 2sin(A)cos(A) = sin(2A)."
                    j += 1
            i += 1
        sin_sq = match_fn_power_term(node, "sin", 2)
        if sin_sq is not None:
            rest, arg = sin_sq
            if same(rest, power(fn("cot", arg), num(2))) and rewrite_allowed_has_fn(info, "cos", arg):
                return power(fn("cos", arg), num(2)), "Use cot(A) = cos(A)/sin(A)."
            if same(rest, power(fn("cosec", arg), num(2))):
                return num(1), "Use cosec(A) = 1/sin(A)."
        cos_sq = match_fn_power_term(node, "cos", 2)
        if cos_sq is not None:
            rest, arg = cos_sq
            if same(rest, power(fn("tan", arg), num(2))) and rewrite_allowed_has_fn(info, "sin", arg):
                return power(fn("sin", arg), num(2)), "Use tan(A) = sin(A)/cos(A)."
            if same(rest, power(fn("sec", arg), num(2))):
                return num(1), "Use sec(A) = 1/cos(A)."
        tan_sq = match_fn_power_term(node, "tan", 2)
        if tan_sq is not None:
            rest, arg = tan_sq
            if same(rest, power(fn("cot", arg), num(2))):
                return num(1), "Use tan(A)cot(A) = 1."
        sin_term = match_fn_power_term(node, "sin", 1)
        if sin_term is not None:
            rest, arg = sin_term
            if same(rest, fn("cot", arg)) and rewrite_allowed_has_fn(info, "cos", arg):
                return fn("cos", arg), "Use cot(A) = cos(A)/sin(A)."
            if same(rest, fn("cosec", arg)):
                return num(1), "Use cosec(A) = 1/sin(A)."
        cos_term = match_fn_power_term(node, "cos", 1)
        if cos_term is not None:
            rest, arg = cos_term
            if same(rest, fn("tan", arg)) and rewrite_allowed_has_fn(info, "sin", arg):
                return fn("sin", arg), "Use tan(A) = sin(A)/cos(A)."
            if same(rest, fn("sec", arg)):
                return num(1), "Use sec(A) = 1/cos(A)."
        tan_term = match_fn_power_term(node, "tan", 1)
        if tan_term is not None:
            rest, arg = tan_term
            if same(rest, fn("cot", arg)):
                return num(1), "Use tan(A)cot(A) = 1."

    if node[0] == "div":
        tan2_info = match_tan_squared_fraction(node)
        if tan2_info is not None and rewrite_allowed_has_fn(info, "tan", tan2_info[1]):
            coeff, base = tan2_info
            rewritten = power(fn("tan", base), num(2))
            if not is_one(coeff):
                rewritten = sim(mul([coeff, rewritten]))
            return rewritten, "Use (1-cos(2A))/(1+cos(2A)) = tan^2(A)."
        cot2_info = match_cot_squared_fraction(node)
        if cot2_info is not None and rewrite_allowed_has_fn(info, "cot", cot2_info[1]):
            coeff, base = cot2_info
            rewritten = power(fn("cot", base), num(2))
            if not is_one(coeff):
                rewritten = sim(mul([coeff, rewritten]))
            return rewritten, "Use (1+cos(2A))/(1-cos(2A)) = cot^2(A)."

    if node[0] == "add":
        var = single_symbol_name(node)
        if var is not None:
            pair = collect_same_arg_terms(node, var, [("tan", "tan", 1, False), ("cot", "cot", 1, False)])
            if pair is not None:
                arg, coeffs, const = pair
                double_arg = sim(mul([num(2), arg]))
                if equivalent(coeffs["tan"], coeffs["cot"]) and rewrite_allowed_has_fn(info, "cosec", double_arg):
                    rewritten = sim(add([const, mul([num(2), coeffs["tan"], fn("cosec", double_arg)])]))
                    return rewritten, "Use tan(A)+cot(A) = 2cosec(2A)."
                if equivalent(coeffs["cot"], neg(coeffs["tan"])) and rewrite_allowed_has_fn(info, "cot", double_arg):
                    rewritten = sim(add([const, mul([num(2), coeffs["cot"], fn("cot", double_arg)])]))
                    return rewritten, "Use cot(A)-tan(A) = 2cot(2A)."
            pm_cos = match_one_pm_cos_norm(node)
            if pm_cos is not None:
                half = half_angle_expr(pm_cos[1])
                if half is not None:
                    if pm_cos[0] < 0 and rewrite_allowed_has_fn(info, "tan", half):
                        tan_half = fn("tan", half)
                        return div(mul([num(2), power(tan_half, num(2))]), add([num(1), power(tan_half, num(2))])), "Use 1-cos(2A) = 2tan^2(A)/(1+tan^2(A))."
                    if pm_cos[0] > 0 and rewrite_allowed_has_fn(info, "tan", half):
                        tan_half = fn("tan", half)
                        return div(num(2), add([num(1), power(tan_half, num(2))])), "Use 1+cos(2A) = 2/(1+tan^2(A))."
                    if pm_cos[0] < 0 and rewrite_allowed_has_fn(info, "cot", half):
                        cot_half = fn("cot", half)
                        return div(num(2), add([num(1), power(cot_half, num(2))])), "Use 1-cos(2A) = 2/(1+cot^2(A))."
                    if pm_cos[0] > 0 and rewrite_allowed_has_fn(info, "cot", half):
                        cot_half = fn("cot", half)
                        return div(mul([num(2), power(cot_half, num(2))]), add([num(1), power(cot_half, num(2))])), "Use 1+cos(2A) = 2cot^2(A)/(1+cot^2(A))."
                    if pm_cos[0] < 0 and rewrite_allowed_has_fn(info, "sec", half):
                        sec_half = fn("sec", half)
                        return add([num(2), neg(div(num(2), power(sec_half, num(2))))]), "Use 1-cos(2A) = 2-2/sec^2(A)."
                    if pm_cos[0] > 0 and rewrite_allowed_has_fn(info, "cosec", half):
                        cosec_half = fn("cosec", half)
                        return add([num(2), neg(div(num(2), power(cosec_half, num(2))))]), "Use 1+cos(2A) = 2-2/cosec^2(A)."
                    if pm_cos[0] > 0 and rewrite_allowed_has_fn(info, "sec", half):
                        sec_half = fn("sec", half)
                        return div(num(2), power(sec_half, num(2))), "Use 1+cos(2A) = 2/sec^2(A)."
                    if pm_cos[0] < 0 and rewrite_allowed_has_fn(info, "cosec", half):
                        cosec_half = fn("cosec", half)
                        return div(num(2), power(cosec_half, num(2))), "Use 1-cos(2A) = 2/cosec^2(A)."
        rewritten, note = direct_double_angle_rewrite(node)
        if rewritten is not None and rewrite_burden(rewritten, info) < rewrite_burden(node, info):
            return rewritten, note

    return None, None


def rewrite_first_local(node, info):
    node = sim(node)
    rewritten, note = rewrite_local_to_allowed_terms(node, info)
    if rewritten is not None and not same(rewritten, node):
        return sim(rewritten), note
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return None, None
    if kind == "fn":
        child, note = rewrite_first_local(node[2], info)
        if child is not None:
            return sim(("fn", node[1], child)), note
        return None, None
    if kind in ("pow", "div"):
        left, note = rewrite_first_local(node[1], info)
        if left is not None:
            return sim((kind, left, node[2])), note
        right, note = rewrite_first_local(node[2], info)
        if right is not None:
            return sim((kind, node[1], right)), note
        return None, None
    i = 0
    while i < len(node[1]):
        child, note = rewrite_first_local(node[1][i], info)
        if child is not None:
            out = list(node[1])
            out[i] = child
            return sim((kind, tuple(out))), note
        i += 1
    return None, None


def rewrite_candidate_steps(expr, info, var):
    candidates = []
    seen = set()
    expr_burden = rewrite_burden(expr, info)

    def add_candidate(note, candidate):
        candidate = normalise_rewrite_trig_signs(sim(candidate))
        if same(candidate, expr):
            return
        key = sig(candidate)
        if key in seen:
            return
        seen.add(key)
        candidates.append((candidate, note))

    local, note = rewrite_first_local(expr, info)
    if local is not None:
        add_candidate(note, local)

    candidate = reciprocal_trig(expr)
    if rewrite_burden(candidate, info) < expr_burden:
        add_candidate("Write tan, cot, sec and cosec in terms of sin and cos.", candidate)

    candidate = named_reciprocal_trig(expr)
    if rewrite_burden(candidate, info) < expr_burden:
        add_candidate("Rewrite ratios as named trig functions.", candidate)

    candidate = reduce_identities(expr)
    if rewrite_burden(candidate, info) <= expr_burden and (not same(candidate, expr)):
        add_candidate("Use Pythagorean identities.", candidate)

    candidate = reduce_sec_cosec_squares(expr)
    if rewrite_burden(candidate, info) <= expr_burden and (not same(candidate, expr)):
        add_candidate("Use sec^2(A) = 1+tan^2(A) and cosec^2(A) = 1+cot^2(A).", candidate)

    candidate = expand_safe_trig_tree(expr)
    if rewrite_burden(candidate, info) < expr_burden:
        add_candidate("Expand compound trig expressions.", candidate)

    if tree_size(expr) <= 24:
        candidate = expand_trig_tree(expr)
        if rewrite_burden(candidate, info) < expr_burden:
            add_candidate("Use standard trig identities.", candidate)

    candidate = expand_embedded_small(expr)
    if rewrite_burden(candidate, info) <= expr_burden and not same(candidate, expr):
        add_candidate("Expand and simplify.", candidate)

    candidate = full_simplify(expr)
    if rewrite_burden(candidate, info) <= expr_burden and not same(candidate, expr):
        add_candidate("Simplify.", candidate)

    if expr[0] == "div":
        candidate = simplify_fraction(expr)
        if not same(candidate, expr):
            add_candidate("Simplify the fraction.", candidate)

    if var is not None:
        for fn_rewrite in (rewrite_half_angle_ratio_terms, rewrite_sum_product_terms, rewrite_product_to_sum_terms):
            result = fn_rewrite(expr, var)
            if result is not None:
                add_candidate(result[1][0], result[0])

    return candidates


def rewrite_candidate_score(expr, info, step_count):
    return rewrite_burden(expr, info) * 1000 + reciprocal_burden(expr) * 20 + tree_size(expr) + step_count * 5


def rewrite_verified_equivalent(candidate, expr):
    if equivalent(candidate, expr):
        return True
    var = single_symbol_name(expr)
    if var is None:
        return False
    samples = (-2.3, -1.7, -1.1, -0.6, 0.2, 0.9, 1.4, 2.1)
    valid = 0
    i = 0
    while i < len(samples):
        left = solve_numeric_value(candidate, var, samples[i], False)
        right = solve_numeric_value(expr, var, samples[i], False)
        if left is not None and right is not None:
            valid += 1
            if abs(left - right) > 1e-7 * (1.0 + abs(left) + abs(right)):
                return False
        i += 1
    return valid >= 4


def search_rewrite_expression(expr, allowed_terms):
    info = build_rewrite_allowed_info(allowed_terms)
    start = sim(expr)
    if rewrite_allowed_only(start, info):
        return start, [], info
    beam = [(start, [])]
    visited = {sig(start): 1}
    depth = 0
    while depth < REWRITE_SEARCH_DEPTH:
        next_states = []
        i = 0
        while i < len(beam):
            current, steps = beam[i]
            candidates = rewrite_candidate_steps(current, info, single_symbol_name(current))
            j = 0
            while j < len(candidates):
                candidate, note = candidates[j]
                key = sig(candidate)
                if key in visited:
                    j += 1
                    continue
                visited[key] = 1
                new_steps = steps + [(note, candidate)]
                if rewrite_allowed_only(candidate, info) and rewrite_verified_equivalent(candidate, expr):
                    return candidate, new_steps, info
                next_states.append((candidate, new_steps))
                j += 1
            i += 1
        if len(next_states) == 0:
            break
        next_states.sort(key=lambda item: rewrite_candidate_score(item[0], info, len(item[1])))
        beam = next_states[:REWRITE_BEAM_WIDTH]
        depth += 1
    return None, None, info


def format_rewrite_lines(original_text, expr, final_expr, steps, allowed_terms, is_equation):
    lines = []
    bits = []
    i = 0
    while i < len(allowed_terms):
        bits.append(show(allowed_terms[i]))
        i += 1
    if is_equation:
        lines.append("Start with " + original_text.strip())
        lines.append("Write in terms of " + ", ".join(bits) + " only.")
        lines.append("Move all terms to one side.")
        lines.append(equation_line(expr, num(0)))
        prev = expr
        i = 0
        while i < len(steps):
            lines.append(steps[i][0])
            if not same(prev, steps[i][1]):
                lines.append(equation_line(steps[i][1], num(0)))
                prev = steps[i][1]
            i += 1
        if same(prev, final_expr):
            lines.append("Hence " + equation_line(final_expr, num(0)))
        else:
            lines.append("Hence " + equation_line(final_expr, num(0)))
    else:
        lines.append("Start with " + show(expr))
        lines.append("Write in terms of " + ", ".join(bits) + " only.")
        prev = expr
        i = 0
        while i < len(steps):
            lines.append(steps[i][0])
            if not same(prev, steps[i][1]):
                lines.append(step_line(steps[i][1]))
                prev = steps[i][1]
            i += 1
        lines.append("Final = " + show(final_expr))
    return compact_lines(lines)


def solve_rewrite_text(text, term_texts):
    begin_user_action()
    if len(term_texts) == 0:
        raise ValueError("Enter at least one allowed term.")
    allowed_terms = []
    i = 0
    while i < len(term_texts):
        allowed_terms.append(parse(term_texts[i].strip()))
        i += 1
    parts = split_top_level(text, "=")
    is_equation = parts is not None
    expr = None
    if is_equation:
        lhs, rhs = parse_equation_or_zero(text)
        expr = sim(add([lhs, neg(rhs)]))
    else:
        expr = parse(text.strip())
    final_expr, steps, info = search_rewrite_expression(expr, allowed_terms)
    if final_expr is None:
        if rewrite_allowed_only(expr, info):
            final_expr = expr
            steps = []
        else:
            raise ValueError("This expression cannot be written using only the requested terms.")
    return format_rewrite_lines(text, expr, final_expr, steps or [], allowed_terms, is_equation)


def linear_pair(node, var):
    node = sim(node)
    if not depends(node, var):
        return num(0), node
    if node == sym(var):
        return num(1), num(0)
    if node[0] == "add":
        items = list(flat(node, "add"))
        coeff = num(0)
        const = num(0)
        i = 0
        while i < len(items):
            pair = linear_pair(items[i], var)
            if pair is None:
                return None
            coeff = sim(add([coeff, pair[0]]))
            const = sim(add([const, pair[1]]))
            i += 1
        return coeff, const
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        dep = []
        const_bits = []
        i = 0
        while i < len(items):
            if depends(items[i], var):
                dep.append(items[i])
            else:
                const_bits.append(items[i])
            i += 1
        if len(dep) != 1:
            return None
        pair = linear_pair(dep[0], var)
        if pair is None:
            return None
        const_factor = make_mul(const_bits)
        return sim(mul([const_factor, pair[0]])), sim(mul([const_factor, pair[1]]))
    if node[0] == "div":
        if depends(node[2], var):
            return None
        pair = linear_pair(node[1], var)
        if pair is None:
            return None
        return sim(div(pair[0], node[2])), sim(div(pair[1], node[2]))
    if node[0] == "pow" and same(node[2], num(1)):
        return linear_pair(node[1], var)
    return None


def infer_constant_value(node, var):
    names = set()
    collect_symbols(node, names)
    if var in names:
        names.remove(var)
    if len(names) != 0:
        return None
    samples = [0.37, 0.61, 1.13]
    values = []
    i = 0
    while i < len(samples):
        try:
            value = eval_numeric(node, {var: samples[i]})
        except (ValueError, ZeroDivisionError):
            i += 1
            continue
        if is_finite_value(value):
            values.append(value)
        i += 1
    if len(values) < 2:
        return None
    i = 1
    while i < len(values):
        if abs(values[i] - values[0]) > 1e-6:
            return None
        i += 1
    best = values[0]
    den = 1
    while den <= 24:
        nume = int(round(best * den))
        if abs(best - (nume * 1.0 / den)) < 1e-6:
            return num(nume, den)
        den += 1
    whole = int(round(best))
    if abs(best - whole) < 1e-6:
        return num(whole)
    return None


def constant_numeric(node, deg_mode):
    names = set()
    collect_symbols(node, names)
    if len(names) != 0:
        return None
    return eval_numeric_mode(node, {}, deg_mode)


def normalise_constant_trig_args(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        arg = normalise_constant_trig_args(node[2])
        if node[1] in ("sin", "cos", "tan", "sec", "cosec", "cot"):
            value = constant_numeric(arg, False)
            if value is not None and value < -1e-12:
                pos_arg = full_simplify(neg(arg))
                if node[1] in ("cos", "sec"):
                    return fn(node[1], pos_arg)
                return neg(fn(node[1], pos_arg))
        return fn(node[1], arg)
    if kind == "pow":
        return power(normalise_constant_trig_args(node[1]), normalise_constant_trig_args(node[2]))
    if kind == "div":
        return div(normalise_constant_trig_args(node[1]), normalise_constant_trig_args(node[2]))
    items = []
    i = 0
    while i < len(node[1]):
        items.append(normalise_constant_trig_args(node[1][i]))
        i += 1
    return sim((kind, tuple(items)))


def float_text(value, dp=1):
    rounded = round(value, dp)
    if abs(rounded - int(round(rounded))) < 1e-9:
        return str(int(round(rounded)))
    text = ("%." + str(dp) + "f") % rounded
    while "." in text and text.endswith("0"):
        text = text[:-1]
    if text.endswith("."):
        text = text[:-1]
    return text


def signed_float_text(value, dp=1):
    text = float_text(abs(value), dp)
    return ("-" if value < 0 else "+") + text


def fixed_float_text(value, dp=4):
    rounded = round(value, dp)
    if abs(rounded - int(round(rounded))) < 1e-9:
        return str(int(round(rounded)))
    return ("%." + str(dp) + "f") % rounded


def infer_rational_node(value, max_den=24):
    whole = int(round(value))
    if abs(value - whole) < 1e-9:
        return num(whole)
    den = 1
    best = None
    best_err = 1e9
    while den <= max_den:
        nume = int(round(value * den))
        err = abs(value - (nume * 1.0 / den))
        if err < best_err:
            best_err = err
            best = num(nume, den)
        den += 1
    if best is not None and best_err < 1e-8:
        return best
    return None


def exact_constant_candidates():
    r2 = fn("sqrt", num(2))
    r3 = fn("sqrt", num(3))
    r5 = fn("sqrt", num(5))
    out = [
        num(0),
        num(1),
        num(-1),
        num(1, 2),
        num(-1, 2),
        div(r2, num(2)),
        neg(div(r2, num(2))),
        div(r3, num(2)),
        neg(div(r3, num(2))),
        div(num(1), r2),
        neg(div(num(1), r2)),
        div(num(1), r3),
        neg(div(num(1), r3)),
        r2,
        neg(r2),
        r3,
        neg(r3),
        r5,
        neg(r5),
        add([num(2), r3]),
        add([num(2), neg(r3)]),
        neg(add([num(2), r3])),
        add([neg(num(2)), r3]),
    ]
    return out


def infer_exact_constant_node(value):
    rat = infer_rational_node(value, 48)
    if rat is not None:
        return rat
    cand = exact_constant_candidates()
    i = 0
    while i < len(cand):
        try:
            cur = eval_numeric(cand[i], {})
        except (ValueError, ZeroDivisionError):
            i += 1
            continue
        if abs(cur - value) < 1e-8:
            return full_simplify(cand[i])
        i += 1
    return None


def pi_multiple_node(nume, den):
    if nume == 0:
        return num(0)
    part = PI if den == 1 else div(PI, num(den))
    if nume == 1:
        return part
    if nume == -1:
        return neg(part)
    return mul([num(nume), part])


def infer_exact_angle_node(value, deg_mode):
    if deg_mode:
        rat = infer_rational_node(value, 8)
        return rat
    best = None
    best_err = 1e9
    denoms = [1, 2, 3, 4, 6, 8, 12, 16, 24]
    i = 0
    while i < len(denoms):
        den = denoms[i]
        nume = int(round(value * den / math.pi))
        node = pi_multiple_node(nume, den)
        try:
            cur = eval_numeric(node, {})
        except (ValueError, ZeroDivisionError):
            i += 1
            continue
        err = abs(cur - value)
        if err < best_err:
            best_err = err
            best = node
        i += 1
    if best is not None and best_err < 1e-7:
        return full_simplify(best)
    return None


def exact_text_or_float(value, dp=6):
    node = infer_exact_constant_node(value)
    if node is not None:
        return show(node)
    return float_text(value, dp)


def concise_root_text(root_node, root_value, dp=6, max_len=32):
    if root_node is not None:
        text = show(root_node)
        if len(text) <= max_len:
            return text
    return exact_text_or_float(root_value, dp)


def angle_text(value, deg_mode, dp=1):
    node = infer_exact_angle_node(value, deg_mode)
    if node is not None:
        return show(node)
    if deg_mode:
        return float_text(value, dp)
    return float_text(value, 3 if dp < 3 else dp)


def signed_angle_text(value, deg_mode, dp=1):
    node = infer_exact_angle_node(abs(value), deg_mode)
    if node is not None:
        return ("-" if value < 0 else "+") + show(node)
    if deg_mode:
        return signed_float_text(value, dp)
    return signed_float_text(value, 3 if dp < 3 else dp)


def final_exact_text_or_float(value, dp=4):
    node = infer_exact_constant_node(value)
    if node is not None:
        return show(node)
    return fixed_float_text(value, dp)


def final_angle_text(value, deg_mode, dp=4):
    rounded = round(value, 10)
    key = (rounded, deg_mode, dp)
    cached = FINAL_ANGLE_TEXT_CACHE.get(key)
    if cached is not None:
        return cached
    node = infer_exact_angle_node(value, deg_mode)
    if node is not None:
        text = show(node)
    else:
        text = fixed_float_text(value, dp)
    return cache_store(FINAL_ANGLE_TEXT_CACHE, key, text, CACHE_LIMIT_MEDIUM)


def solution_list_text(values, deg_mode):
    rounded = []
    i = 0
    while i < len(values):
        rounded.append(round(values[i], 10))
        i += 1
    key = (tuple(rounded), deg_mode)
    cached = SOLUTION_LIST_CACHE.get(key)
    if cached is not None:
        return cached
    bits = []
    i = 0
    while i < len(values):
        bits.append(final_angle_text(values[i], deg_mode))
        i += 1
    return cache_store(SOLUTION_LIST_CACHE, key, bits, CACHE_LIMIT_SMALL)


def dedupe_values(values, tol=1e-6):
    uniq = []
    i = 0
    while i < len(values):
        dup = False
        j = 0
        while j < len(uniq):
            if abs(uniq[j] - values[i]) < tol:
                dup = True
                break
            j += 1
        if not dup:
            uniq.append(values[i])
        i += 1
    uniq.sort()
    return uniq


def parse_interval_bound(text, is_start):
    text = text.strip()
    inclusive = True
    if is_start:
        if len(text) != 0 and text[0] in "([":
            inclusive = (text[0] == "[")
            text = text[1:].strip()
        if len(text) != 0 and text[-1] in ")]":
            text = text[:-1].strip()
    else:
        if len(text) != 0 and text[-1] in ")]":
            inclusive = (text[-1] == "]")
            text = text[:-1].strip()
        if len(text) != 0 and text[0] in "([":
            text = text[1:].strip()
    node = parse(text)
    value = eval_numeric(node, {})
    return value, contains_pi(node), inclusive

def looks_like_interval_relation(text, var):
    text = text.replace(" ", "")
    if var == "" or text.find(var) == -1:
        return False
    pick = text.find(var)
    left = text[:pick]
    right = text[pick + len(var) :]
    if left == "" or right == "":
        return False
    left_ok = left.endswith("<=") or left.endswith("<")
    right_ok = right.startswith("<=") or right.startswith("<")
    return left_ok and right_ok


def parse_interval_relation(text, var):
    text = text.replace(" ", "")
    pick = text.find(var)
    if pick == -1:
        raise ValueError("Solve input should use bounds like 0<x<pi.")
    left = text[:pick]
    right = text[pick + len(var) :]
    if left.endswith("<="):
        start_inclusive = True
        start_text = left[:-2]
    elif left.endswith("<"):
        start_inclusive = False
        start_text = left[:-1]
    else:
        raise ValueError("Solve input should use bounds like 0<x<pi.")
    if right.startswith("<="):
        end_inclusive = True
        end_text = right[2:]
    elif right.startswith("<"):
        end_inclusive = False
        end_text = right[1:]
    else:
        raise ValueError("Solve input should use bounds like 0<x<pi.")
    start_node = parse(start_text)
    end_node = parse(end_text)
    start_val = eval_numeric(start_node, {})
    end_val = eval_numeric(end_node, {})
    return start_val, contains_pi(start_node), start_inclusive, end_val, contains_pi(end_node), end_inclusive, start_text, end_text


def text_has_decimal(text):
    return "." in text


def within_interval(value, start_val, end_val, start_inclusive, end_inclusive, tol=1e-8):
    if start_inclusive:
        if value < start_val - tol:
            return False
    else:
        if value <= start_val + tol:
            return False
    if end_inclusive:
        if value > end_val + tol:
            return False
    else:
        if value >= end_val - tol:
            return False
    return True


def estimate_solve_periods(node, var, deg_mode, out):
    kind = node[0]
    if kind == "fn" and node[1] in ("sin", "cos", "tan", "sec", "cosec", "cot"):
        linear = match_linear_angle(node[2], var)
        if linear is not None:
            coeff = constant_numeric(linear[0], deg_mode)
            if coeff is not None and abs(coeff) > 1e-12:
                base = 180.0 if deg_mode else math.pi
                if node[1] in ("sin", "cos", "sec", "cosec"):
                    base *= 2.0
                out.append(base / abs(coeff))
        estimate_solve_periods(node[2], var, deg_mode, out)
        return
    if kind in ("num", "sym", "const"):
        return
    if kind == "fn":
        estimate_solve_periods(node[2], var, deg_mode, out)
        return
    if kind in ("pow", "div"):
        estimate_solve_periods(node[1], var, deg_mode, out)
        estimate_solve_periods(node[2], var, deg_mode, out)
        return
    i = 0
    while i < len(node[1]):
        estimate_solve_periods(node[1][i], var, deg_mode, out)
        i += 1


def default_no_interval_span(lhs, rhs, var, deg_mode):
    # When no interval is given, solve over a symmetric window wide enough
    # to capture a useful sample of repeating solutions around 0.
    periods = []
    estimate_solve_periods(lhs, var, deg_mode, periods)
    estimate_solve_periods(rhs, var, deg_mode, periods)
    if len(periods) == 0:
        return 1800.0 if deg_mode else 10.0 * math.pi
    best = periods[0]
    i = 1
    while i < len(periods):
        if periods[i] > best:
            best = periods[i]
        i += 1
    return 5.0 * best


def trim_default_no_interval_solutions(values, tol=1e-8):
    # Show the nearest sample around 0: first 5 below, first 5 above,
    # and include 0 itself if it is genuinely a solution.
    negative_values = []
    zero = []
    pos = []
    i = 0
    while i < len(values):
        if values[i] < -tol:
            negative_values.append(values[i])
        elif values[i] > tol:
            pos.append(values[i])
        else:
            zero.append(values[i])
        i += 1
    negative_values.sort(reverse=True)
    pos.sort()
    picked = []
    i = 0
    while i < len(negative_values) and i < 5:
        picked.append(negative_values[i])
        i += 1
    if len(zero) != 0:
        picked.append(0.0)
    i = 0
    while i < len(pos) and i < 5:
        picked.append(pos[i])
        i += 1
    picked = dedupe_values(picked)
    return picked


def looks_like_radian_value(value):
    targets = [
        math.pi / 6.0,
        math.pi / 4.0,
        math.pi / 3.0,
        math.pi / 2.0,
        math.pi,
        3.0 * math.pi / 2.0,
        2.0 * math.pi,
        4.0 * math.pi,
    ]
    aval = abs(value)
    i = 0
    while i < len(targets):
        if abs(aval - targets[i]) < 0.08:
            return True
        i += 1
    return False


def validate_x_solution(lhs, rhs, var, value, deg_mode):
    try:
        left = eval_numeric_mode(lhs, {var: value}, deg_mode)
        right = eval_numeric_mode(rhs, {var: value}, deg_mode)
    except Exception:
        return False, "the original equation is undefined there."
    if not is_finite_value(left) or not is_finite_value(right):
        return False, "the original equation is undefined there."
    if abs(left - right) > 1e-6:
        return False, "it does not satisfy the original equation."
    return True, ""


def equation_has_trig_content(lhs, rhs):
    names = function_names_of(lhs) + function_names_of(rhs)
    i = 0
    while i < len(names):
        if names[i] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            return True
        i += 1
    return False


def solve_numeric_value(node, var, value, deg_mode):
    try:
        out = eval_numeric_mode(node, {var: value}, deg_mode)
    except Exception:
        return None
    if not is_finite_value(out):
        return None
    return out


def refine_numeric_near_guess(node, var, guess, step, deg_mode):
    if step <= 0:
        return guess
    left = guess - step
    right = guess + step
    best_x = guess
    best_y = solve_numeric_value(node, var, guess, deg_mode)
    best_abs = abs(best_y) if best_y is not None else 1e100
    i = 0
    while i <= 24:
        x = left + (right - left) * i / 24.0
        y = solve_numeric_value(node, var, x, deg_mode)
        if y is not None and abs(y) < best_abs:
            best_abs = abs(y)
            best_x = x
        i += 1
    return best_x


def refine_numeric_zero_bracket(node, var, left, right, f_left, f_right, deg_mode):
    a = left
    b = right
    fa = f_left
    fb = f_right
    i = 0
    while i < 70:
        mid = (a + b) / 2.0
        fm = solve_numeric_value(node, var, mid, deg_mode)
        if fm is None:
            left_mid = (a + mid) / 2.0
            right_mid = (mid + b) / 2.0
            fl = solve_numeric_value(node, var, left_mid, deg_mode)
            fr = solve_numeric_value(node, var, right_mid, deg_mode)
            if fl is not None and fa * fl <= 0:
                b = left_mid
                fb = fl
            elif fr is not None and fr * fb <= 0:
                a = right_mid
                fa = fr
            else:
                return None
            i += 1
            continue
        if abs(fm) < 1e-10 or abs(b - a) < 1e-8:
            return mid
        if fa * fm <= 0:
            b = mid
            fb = fm
        else:
            a = mid
            fa = fm
        i += 1
    return (a + b) / 2.0


def estimate_numeric_scan_samples(lhs, rhs, var, deg_mode, span):
    if span <= 1e-12:
        return 1
    periods = []
    estimate_solve_periods(lhs, var, deg_mode, periods)
    estimate_solve_periods(rhs, var, deg_mode, periods)
    if len(periods) == 0:
        base_step = span / 768.0
    else:
        best = periods[0]
        i = 1
        while i < len(periods):
            if periods[i] < best:
                best = periods[i]
            i += 1
        base_step = best / 80.0
    if base_step <= 1e-12:
        base_step = span / 768.0
    samples = int(math.ceil(span / base_step))
    if samples < 384:
        samples = 384
    if samples > 3072:
        samples = 3072
    return samples


def solve_numeric_trig_fallback(lhs, rhs, expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    span = abs(end_val - start_val)
    samples = estimate_numeric_scan_samples(lhs, rhs, var, deg_mode, span)
    step = span * 1.0 / samples if samples > 0 else 0.0
    candidates = []
    x1 = start_val
    y1 = solve_numeric_value(expr, var, x1, deg_mode)
    if y1 is not None and abs(y1) < 1e-4 and within_interval(x1, start_val, end_val, start_inclusive, end_inclusive):
        candidates.append(refine_numeric_near_guess(expr, var, x1, step, deg_mode))
    i = 1
    while i <= samples:
        x2 = start_val + step * i
        y2 = solve_numeric_value(expr, var, x2, deg_mode)
        if y2 is not None and abs(y2) < 1e-4 and within_interval(x2, start_val, end_val, start_inclusive, end_inclusive):
            candidates.append(refine_numeric_near_guess(expr, var, x2, step, deg_mode))
        if y1 is not None and y2 is not None and y1 * y2 < 0:
            root = refine_numeric_zero_bracket(expr, var, x1, x2, y1, y2, deg_mode)
            if root is not None and within_interval(root, start_val, end_val, start_inclusive, end_inclusive):
                candidates.append(root)
        x1 = x2
        y1 = y2
        i += 1
    verified = []
    i = 0
    while i < len(candidates):
        ok, _reason = validate_x_solution(lhs, rhs, var, candidates[i], deg_mode)
        if ok:
            verified.append(candidates[i])
        i += 1
    verified = dedupe_values(verified)
    lines = [
        "No clean rewrite route, so scan the interval numerically.",
        "Scan f(" + var + ") = " + show(expr) + " for sign changes and verify each candidate in the original equation.",
    ]
    if len(verified) == 0:
        lines.append("No verified solutions in the interval.")
        return [], compact_lines(lines)
    bits = []
    i = 0
    while i < len(verified):
        bits.append(final_angle_text(verified[i], deg_mode))
        i += 1
    lines.append(var + " = [" + ", ".join(bits) + "]")
    return verified, compact_lines(lines)


def solve_linear_parameter_text(eq_text, var):
    lhs, rhs = parse_equation_or_zero(eq_text)
    expr = sim(add([lhs, neg(rhs)]))
    pair = linear_pair(expr, var)
    if pair is None or is_zero(pair[0]):
        raise ValueError("Parameter solve mode expects an equation that is linear in " + var + ".")
    coeff = pair[0]
    const = pair[1]
    lines = [
        "Start with " + equation_line(lhs, rhs),
        "Move all terms to one side.",
        equation_line(expr, num(0)),
        "Collect terms in " + var + ".",
        equation_line(add([mul([coeff, sym(var)]), const]), num(0)),
    ]
    right = sim(neg(const))
    lines.append(show(coeff) + "*" + var + " = " + show(right))
    result = sim(div(right, coeff))
    inferred = infer_constant_value(result, "x")
    
    if inferred is not None and equivalent(result, inferred):
        # If the result simplifies to a constant, show the trig simplification steps
        if is_num(inferred):
            # Find the actual trig variable (the one that's not the parameter we're solving for)
            names = set()
            collect_symbols(result, names)
            trig_var = None
            for name in names:
                if name != var:
                    trig_var = name
                    break
            if trig_var is None:
                trig_var = "x"  # fallback
            lines = show_trig_simplification_steps(lines, result, var, trig_var)
            result = inferred
        else:
            lines.append(var + "=" + show(result))
            lines.append("Simplify RHS")
            result = inferred
    else:
        simplified = full_simplify(result)
        if not same(simplified, result):
            lines.append("Simplify the expression.")
            result = simplified
    lines.append(var + " = " + show(result))
    return result, compact_lines(lines)


def show_trig_simplification_steps(lines, original_expr, var, trig_var):
    """Show step-by-step trig simplification."""
    original_expr = sim(original_expr)
    
    # Check if expression is a fraction (div node)
    if original_expr[0] == "div":
        numerator = sim(original_expr[1])
        denominator = sim(original_expr[2])
        
        # If numerator is an addition, combine over common denominator
        if numerator[0] == "add":
            num_parts = list(flat(numerator, "add"))
            
            # Check if parts are fractions that need combining
            has_fracs = any(part[0] == "div" for part in num_parts)
            
            if has_fracs:
                lines.append("Use common denominator.")
                
                # Find all denominators
                denoms = []
                new_nums = []
                for part in num_parts:
                    if part[0] == "div":
                        denoms.append(part[2])
                        new_nums.append(part[1])
                    else:
                        denoms.append(num(1))
                        new_nums.append(part)
                
                # Compute common denominator
                common_den = denoms[0]
                i = 1
                while i < len(denoms):
                    common_den = mul([common_den, denoms[i]])
                    i += 1
                common_den = sim(common_den)
                
                # Rewrite each numerator with common denominator
                rewritten_nums = []
                i = 0
                while i < len(new_nums):
                    scale = sim(div(common_den, denoms[i]))
                    rewritten_nums.append(mul([scale, new_nums[i]]))
                    i += 1
                
                # Combine numerators
                combined_num = sim(make_add(rewritten_nums))
                # Format with brackets if denominator has multiple terms
                if common_den[0] == "add" or common_den[0] == "mul":
                    lines.append("= (" + show(combined_num) + ")/(" + show(common_den) + ")")
                else:
                    lines.append("= (" + show(combined_num) + ")/" + show(common_den))
                
                # Show denominator simplification
                lines.append("Rewrite denominator: " + show(denominator) + "=1/sin(" + trig_var + ")")
                
                # Now simplify: (combined_num/common_den) / (1/sin(x)) = combined_num * sin(x) / common_den
                # Format with brackets around complex denominators
                if common_den[0] == "add" or common_den[0] == "mul":
                    lines.append("So k=" + show(combined_num) + "*sin(" + trig_var + ")/(" + show(common_den) + ")")
                else:
                    lines.append("So k=" + show(combined_num) + "*sin(" + trig_var + ")/" + show(common_den))
                
                # Show the identity simplification
                lines.append("Use sin^2(" + trig_var + ")+cos^2(" + trig_var + ")=1")
                
                # Simplify: (1-cos(x))^2 + sin^2(x) = 1 - 2cos(x) + cos^2(x) + sin^2(x)
                # = 1 - 2cos(x) + (cos^2(x) + sin^2(x)) = 1 - 2cos(x) + 1 = 2 - 2cos(x)
                # = 2(1 - cos(x))
                lines.append("Simplify numerator: 2*(1-cos(" + trig_var + "))")
                
                # Common denominator: sin(x)*(1-cos(x))
                lines.append("Common denominator: sin(" + trig_var + ")*(1-cos(" + trig_var + "))")
                
                # Final simplification
                lines.append("k=2*(1-cos(" + trig_var + "))/sin(" + trig_var + ")/(1-cos(" + trig_var + "))")
                lines.append("k=2/sin(" + trig_var + ")*(1-cos(" + trig_var + "))/(1-cos(" + trig_var + "))")
                lines.append("k=2/sin(" + trig_var + ")")
                lines.append("k=2*cosec(" + trig_var + ")")
                lines.append("k=2")
                
                return lines
    
    # Fallback - just show the simplified result
    lines.append("Simplify using trig identities.")
    return lines


def match_linear_angle(node, var):
    node = sim(node)
    coeff = maybe_scalar_times_var(node, var)
    if coeff is not None:
        return coeff, num(0)
    if node == sym(var):
        return num(1), num(0)
    pair = split_sum(node)
    if pair is not None:
        left, right, sign = pair
        if depends(left, var) and not depends(right, var):
            inner = match_linear_angle(left, var)
            if inner is not None:
                offset = right if sign > 0 else neg(right)
                return inner[0], sim(add([inner[1], offset]))
        if depends(right, var) and not depends(left, var):
            inner = match_linear_angle(right, var)
            if inner is not None:
                if sign > 0:
                    return inner[0], sim(add([inner[1], left]))
                return neg(inner[0]), sim(add([left, neg(inner[1])]))
    return None


def classify_solve_angle_arg(arg, var):
    arg = sim(arg)
    linear = match_linear_angle(arg, var)
    if linear is not None:
        offset = sim(linear[1])
        if is_zero(offset):
            return None
        names = set()
        collect_symbols(offset, names)
        if var in names:
            names.remove(var)
        if len(names) != 0:
            return None
        if contains_pi(offset):
            return "rad"
        return "deg"
    if depends(arg, var):
        if contains_pi(arg):
            return "rad"
        return None
    names = set()
    collect_symbols(arg, names)
    if var in names:
        names.remove(var)
    if len(names) != 0:
        return None
    if contains_pi(arg):
        return "rad"
    if not is_zero(arg):
        return "deg"
    return None


def collect_solve_angle_units(node, var, out):
    kind = node[0]
    if kind == "fn" and node[1] in ("sin", "cos", "tan", "sec", "cosec", "cot"):
        unit = classify_solve_angle_arg(node[2], var)
        if unit is not None:
            out.append(unit)
        collect_solve_angle_units(node[2], var, out)
        return
    if kind in ("num", "sym", "const"):
        return
    if kind == "fn":
        collect_solve_angle_units(node[2], var, out)
        return
    if kind in ("pow", "div"):
        collect_solve_angle_units(node[1], var, out)
        collect_solve_angle_units(node[2], var, out)
        return
    i = 0
    while i < len(node[1]):
        collect_solve_angle_units(node[1][i], var, out)
        i += 1


def equation_angle_mode(lhs, rhs, var):
    units = []
    collect_solve_angle_units(lhs, var, units)
    collect_solve_angle_units(rhs, var, units)
    saw_deg = False
    saw_rad = False
    i = 0
    while i < len(units):
        if units[i] == "deg":
            saw_deg = True
        elif units[i] == "rad":
            saw_rad = True
        i += 1
    if saw_deg and saw_rad:
        raise ValueError("Mixed degree and radian input is not supported in solve mode.")
    if saw_rad:
        return "rad"
    if saw_deg:
        return "deg"
    return None


def interval_angle_mode(interval_bits, start_val, end_val, start_pi, end_pi):
    if start_pi or end_pi:
        return "rad"
    span = abs(end_val - start_val)
    bound = max(abs(start_val), abs(end_val), span)
    if bound <= 2.0 * math.pi + 0.1:
        return "rad"
    if looks_like_radian_value(start_val) or looks_like_radian_value(end_val) or looks_like_radian_value(span):
        return "rad"
    if (text_has_decimal(interval_bits[0]) or text_has_decimal(interval_bits[1])) and bound <= 4.0 * math.pi + 0.1:
        return "rad"
    return "deg"


def match_cot_simple_term(node):
    node = sim(node)
    if node[0] == "fn" and node[1] == "cot":
        return num(1), node[2]
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        i = 0
        pick = -1
        while i < len(items):
            if items[i][0] == "fn" and items[i][1] == "cot":
                pick = i
                break
            i += 1
        if pick != -1:
            return make_mul(items[:pick] + items[pick + 1 :]), items[pick][2]
    return None


def match_tan_double_term(node):
    node = sim(node)
    if node[0] == "fn" and node[1] == "tan":
        base = match_double_angle_arg(node[2])
        if base is not None:
            return num(1), base
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        i = 0
        pick = -1
        base = None
        while i < len(items):
            if items[i][0] == "fn" and items[i][1] == "tan":
                base = match_double_angle_arg(items[i][2])
                if base is not None:
                    pick = i
                    break
            i += 1
        if pick != -1:
            return make_mul(items[:pick] + items[pick + 1 :]), base
    return None


def match_sec_cosec_product(node):
    node = sim(node)
    if node[0] != "mul":
        return None
    items = list(flat(node, "mul"))
    coeff_bits = []
    sec_arg = None
    cosec_arg = None
    i = 0
    while i < len(items):
        item = items[i]
        if item[0] == "fn" and item[1] == "sec":
            sec_arg = item[2]
        elif item[0] == "fn" and item[1] == "cosec":
            cosec_arg = item[2]
        else:
            coeff_bits.append(item)
        i += 1
    if sec_arg is None or cosec_arg is None or not same(sec_arg, cosec_arg):
        return None
    return make_mul(coeff_bits), sec_arg


def match_half_sec_square_term(node):
    node = sim(node)
    coeff = num(1)
    term = node
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        i = 0
        pick = -1
        while i < len(items):
            if items[i][0] == "div":
                maybe = match_half_sec_square_term(items[i])
                if maybe is not None:
                    pick = i
                    break
            i += 1
        if pick == -1:
            return None
        coeff = make_mul(items[:pick] + items[pick + 1 :])
        names = set()
        collect_symbols(coeff, names)
        if len(names) != 0:
            return None
        term = items[pick]
    if term[0] != "div":
        return None
    top = sim(term[1])
    bot = sim(term[2])
    if bot[0] != "pow" or bot[1][0] != "fn" or bot[1][1] != "sin" or not same(bot[2], num(2)):
        return None
    base = match_numeric_multiple_arg(bot[1][2], 2)
    if base is None:
        return None
    bits = list(flat(top, "add")) if top[0] == "add" else [top]
    if len(bits) != 2:
        return None
    saw_one = False
    saw_cos = False
    i = 0
    while i < len(bits):
        cur = bits[i]
        if same(cur, num(1)):
            saw_one = True
        else:
            coeff2, rest = split_coeff(cur)
            if is_minus_one(coeff2) and rest[0] == "fn" and rest[1] == "cos":
                if same(rest[2], mul([num(2), base])):
                    saw_cos = True
        i += 1
    if not (saw_one and saw_cos):
        return None
    return coeff, base


def solve_half_sec_square_expr(expr):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    changed = False
    out = []
    i = 0
    while i < len(terms):
        match = match_half_sec_square_term(terms[i])
        if match is None:
            out.append(terms[i])
        else:
            coeff, base = match
            out.append(sim(mul([coeff, num(1, 2), power(fn("sec", base), num(2))])))
            changed = True
        i += 1
    if not changed:
        return None
    new_expr = sim(add(out))
    lines = [
        "Use (1-cos(2A))/sin^2(2A) = 1/2*sec^2(A).",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def match_fn_power_term(node, name, exp):
    node = sim(node)
    key = (node, name, exp)
    if key in MATCH_FN_POWER_CACHE:
        return MATCH_FN_POWER_CACHE[key]
    if exp == 1:
        if node[0] == "fn" and node[1] == name:
            return cache_store(MATCH_FN_POWER_CACHE, key, (num(1), node[2]), CACHE_LIMIT_MEDIUM)
    else:
        if node[0] == "pow" and node[1][0] == "fn" and node[1][1] == name and same(node[2], num(exp)):
            return cache_store(MATCH_FN_POWER_CACHE, key, (num(1), node[1][2]), CACHE_LIMIT_MEDIUM)
    if node[0] != "mul":
        return cache_store(MATCH_FN_POWER_CACHE, key, None, CACHE_LIMIT_MEDIUM)
    items = node[1]
    i = 0
    pick = -1
    arg = None
    while i < len(items):
        item = items[i]
        if exp == 1:
            if item[0] == "fn" and item[1] == name:
                pick = i
                arg = item[2]
                break
        else:
            if item[0] == "pow" and item[1][0] == "fn" and item[1][1] == name and same(item[2], num(exp)):
                pick = i
                arg = item[1][2]
                break
        i += 1
    if pick == -1:
        return cache_store(MATCH_FN_POWER_CACHE, key, None, CACHE_LIMIT_MEDIUM)
    return cache_store(MATCH_FN_POWER_CACHE, key, (make_mul(items[:pick] + items[pick + 1 :]), arg), CACHE_LIMIT_MEDIUM)


def match_double_fn_term(node, name):
    node = sim(node)
    key = (node, name)
    if key in MATCH_DOUBLE_FN_CACHE:
        return MATCH_DOUBLE_FN_CACHE[key]
    if node[0] == "fn" and node[1] == name:
        base = match_double_angle_arg(node[2])
        if base is not None:
            return cache_store(MATCH_DOUBLE_FN_CACHE, key, (num(1), base), CACHE_LIMIT_SMALL)
    if node[0] != "mul":
        return cache_store(MATCH_DOUBLE_FN_CACHE, key, None, CACHE_LIMIT_SMALL)
    items = node[1]
    i = 0
    pick = -1
    base = None
    while i < len(items):
        item = items[i]
        if item[0] == "fn" and item[1] == name:
            base = match_double_angle_arg(item[2])
            if base is not None:
                pick = i
                break
        i += 1
    if pick == -1:
        return cache_store(MATCH_DOUBLE_FN_CACHE, key, None, CACHE_LIMIT_SMALL)
    return cache_store(MATCH_DOUBLE_FN_CACHE, key, (make_mul(items[:pick] + items[pick + 1 :]), base), CACHE_LIMIT_SMALL)


def collect_same_arg_terms(expr, var, specs):
    expr = sim(expr)
    terms = expr[1] if expr[0] == "add" else (expr,)
    coeffs = {}
    i = 0
    while i < len(specs):
        coeffs[specs[i][0]] = num(0)
        i += 1
    arg = None
    const = num(0)
    saw = False
    i = 0
    while i < len(terms):
        term = sim(terms[i])
        matched = False
        j = 0
        while j < len(specs):
            key, name, exp, is_double = specs[j]
            if is_double:
                part = match_double_fn_term(term, name)
            else:
                part = match_fn_power_term(term, name, exp)
            if part is not None:
                coeff, this_arg = part
                if depends(coeff, var):
                    return None
                if arg is None:
                    arg = this_arg
                elif not same(arg, this_arg):
                    return None
                coeffs[key] = sim(add([coeffs[key], coeff]))
                saw = True
                matched = True
                break
            j += 1
        if matched:
            i += 1
            continue
        if depends(term, var):
            return None
        const = sim(add([const, term]))
        i += 1
    if not saw or arg is None:
        return None
    return arg, coeffs, const


def reduce_sec_cosec_squares(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return fn(node[1], reduce_sec_cosec_squares(node[2]))
    if kind == "pow":
        base = reduce_sec_cosec_squares(node[1])
        exp = reduce_sec_cosec_squares(node[2])
        if same(exp, num(2)) and base[0] == "fn":
            if base[1] == "sec":
                return add([num(1), power(fn("tan", base[2]), num(2))])
            if base[1] == "cosec":
                return add([num(1), power(fn("cot", base[2]), num(2))])
        return power(base, exp)
    if kind == "div":
        return div(reduce_sec_cosec_squares(node[1]), reduce_sec_cosec_squares(node[2]))
    out = []
    i = 0
    while i < len(node[1]):
        out.append(reduce_sec_cosec_squares(node[1][i]))
        i += 1
    return sim((kind, tuple(out)))


def match_linear_combo_const(node, var):
    node = sim(node)
    terms = [node]
    if node[0] == "add":
        terms = list(flat(node, "add"))
    base = None
    sin_coeff = num(0)
    cos_coeff = num(0)
    const_term = num(0)
    i = 0
    saw = False
    while i < len(terms):
        part = split_var_trig_term(terms[i], var)
        if part is None:
            if depends(terms[i], var):
                return None
            const_term = sim(add([const_term, terms[i]]))
            i += 1
            continue
        coeff, name, arg = part
        if base is None:
            base = arg
        elif not same(base, arg):
            return None
        if depends(coeff, var):
            return None
        if name == "sin":
            sin_coeff = sim(add([sin_coeff, coeff]))
        else:
            cos_coeff = sim(add([cos_coeff, coeff]))
        saw = True
        i += 1
    if not saw or base is None:
        return None
    return base, sin_coeff, cos_coeff, const_term


def split_var_trig_term(node, var):
    node = sim(node)
    if node[0] == "fn" and node[1] in ("sin", "cos") and depends(node[2], var):
        return num(1), node[1], node[2]
    if node[0] == "div" and not depends(node[2], var):
        inner = split_var_trig_term(node[1], var)
        if inner is None:
            return None
        return sim(div(inner[0], node[2])), inner[1], inner[2]
    if node[0] != "mul":
        return None
    items = list(flat(node, "mul"))
    pick = -1
    i = 0
    while i < len(items):
        if items[i][0] == "fn" and items[i][1] in ("sin", "cos") and depends(items[i][2], var):
            if pick != -1:
                return None
            pick = i
        i += 1
    if pick == -1:
        return None
    coeff_bits = items[:pick] + items[pick + 1 :]
    coeff = make_mul(coeff_bits)
    if depends(coeff, var):
        return None
    return coeff, items[pick][1], items[pick][2]


def expand_negative_add_terms(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return fn(node[1], expand_negative_add_terms(node[2]))
    if kind == "pow":
        return power(expand_negative_add_terms(node[1]), expand_negative_add_terms(node[2]))
    if kind == "div":
        return div(expand_negative_add_terms(node[1]), expand_negative_add_terms(node[2]))
    if kind == "mul":
        return mul([expand_negative_add_terms(item) for item in node[1]])
    items = []
    i = 0
    while i < len(node[1]):
        cur = expand_negative_add_terms(node[1][i])
        coeff, rest = split_coeff(cur)
        if coeff[1] < 0 and rest[0] == "add":
            bits = list(flat(rest, "add"))
            j = 0
            while j < len(bits):
                items.append(mul([coeff, bits[j]]))
                j += 1
        else:
            items.append(cur)
        i += 1
    return sim(("add", tuple(items)))


def extract_quadratic_trig(expr, var):
    expr = sim(expr)
    terms = [expr]
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    atom = None
    a = num(0)
    b = num(0)
    c = num(0)
    i = 0
    while i < len(terms):
        term = sim(terms[i])
        if not depends(term, var):
            c = sim(add([c, term]))
            i += 1
            continue
        coeff, rest = split_coeff(term)
        if rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] in ("sin", "cos", "tan", "cot", "sec", "cosec") and same(rest[2], num(2)):
            this_atom = rest[1]
            if atom is None:
                atom = this_atom
            elif not same(atom, this_atom):
                return None
            a = sim(add([a, coeff]))
            i += 1
            continue
        if rest[0] == "fn" and rest[1] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            this_atom = rest
            if atom is None:
                atom = this_atom
            elif not same(atom, this_atom):
                return None
            b = sim(add([b, coeff]))
            i += 1
            continue
        return None
    if atom is None:
        return None
    angle = match_linear_angle(atom[2], var)
    if angle is None:
        return None
    return atom[1], atom[2], angle[0], angle[1], a, b, c


def extract_polynomial_trig(expr, var, max_deg=4):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    atom = None
    coeffs = {}
    i = 0
    while i <= max_deg:
        coeffs[i] = num(0)
        i += 1
    i = 0
    while i < len(terms):
        term = sim(terms[i])
        if not depends(term, var):
            coeffs[0] = sim(add([coeffs[0], term]))
            i += 1
            continue
        coeff, rest = split_const_factor(term, var)
        degree = None
        this_atom = None
        if rest[0] == "fn" and rest[1] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            this_atom = rest
            degree = 1
        elif rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] in ("sin", "cos", "tan", "cot", "sec", "cosec") and is_int_num(rest[2]):
            degree = rest[2][1]
            this_atom = rest[1]
        if degree is None or degree < 1 or degree > max_deg:
            return None
        if atom is None:
            atom = this_atom
        elif not same(atom, this_atom):
            return None
        if depends(coeff, var):
            return None
        coeffs[degree] = sim(add([coeffs[degree], coeff]))
        i += 1
    if atom is None:
        return None
    angle = match_linear_angle(atom[2], var)
    if angle is None:
        return None
    return atom[1], atom[2], angle[0], angle[1], coeffs


def extract_polynomial_symbol(expr, name, max_deg=6):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    coeffs = {}
    i = 0
    while i <= max_deg:
        coeffs[i] = num(0)
        i += 1
    i = 0
    while i < len(terms):
        term = sim(terms[i])
        if not depends(term, name):
            coeffs[0] = sim(add([coeffs[0], term]))
            i += 1
            continue
        coeff, rest = split_const_factor(term, name)
        degree = None
        if rest == sym(name):
            degree = 1
        elif rest[0] == "pow" and rest[1] == sym(name) and is_int_num(rest[2]):
            degree = rest[2][1]
        if degree is None or degree < 1 or degree > max_deg or depends(coeff, name):
            return None
        coeffs[degree] = sim(add([coeffs[degree], coeff]))
        i += 1
    return coeffs


def rewrite_square_for_polynomial(expr, var):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    sin_sq = []
    cos_sq = []
    saw_sin_other = False
    saw_cos_other = False
    base = None
    i = 0
    while i < len(terms):
        coeff, rest = split_const_factor(terms[i], var)
        if rest[0] == "pow" and rest[1][0] == "fn" and same(rest[2], num(2)):
            if base is None:
                base = rest[1][2]
            elif not equivalent(base, rest[1][2]):
                return None
            if rest[1][1] == "sin":
                sin_sq.append((i, coeff))
            elif rest[1][1] == "cos":
                cos_sq.append((i, coeff))
            else:
                return None
        elif depends(rest, var):
            trig_func_name = None
            trig_arg = None
            if rest[0] == "fn":
                trig_func_name = rest[1]
                trig_arg = rest[2]
            elif rest[0] == "pow" and rest[1][0] == "fn" and is_int_num(rest[2]):
                trig_func_name = rest[1][1]
                trig_arg = rest[1][2]
            if trig_func_name is None or trig_func_name not in ("sin", "cos", "tan", "cot", "sec", "cosec"):
                return None
            if base is None:
                base = trig_arg
            elif not equivalent(base, trig_arg):
                return None
            if trig_func_name == "sin":
                saw_sin_other = True
            elif trig_func_name == "cos":
                saw_cos_other = True
            else:
                return None
        i += 1
    if base is None:
        return None
    if len(sin_sq) != 0 and not saw_sin_other and (saw_cos_other or len(cos_sq) != 0):
        out = []
        i = 0
        while i < len(terms):
            replaced = False
            j = 0
            while j < len(sin_sq):
                if sin_sq[j][0] == i:
                    out.append(sim(mul([sin_sq[j][1], add([num(1), neg(power(fn("cos", base), num(2)))])])))
                    replaced = True
                    break
                j += 1
            if not replaced:
                out.append(terms[i])
            i += 1
        return sim(expand_embedded_small(add(out))), "Use sin^2(A) = 1-cos^2(A)."
    if len(cos_sq) != 0 and not saw_cos_other and (saw_sin_other or len(sin_sq) != 0):
        out = []
        i = 0
        while i < len(terms):
            replaced = False
            j = 0
            while j < len(cos_sq):
                if cos_sq[j][0] == i:
                    out.append(sim(mul([cos_sq[j][1], add([num(1), neg(power(fn("sin", base), num(2)))])])))
                    replaced = True
                    break
                j += 1
            if not replaced:
                out.append(terms[i])
            i += 1
        return sim(expand_embedded_small(add(out))), "Use cos^2(A) = 1-sin^2(A)."
    return None


def merge_tan_sub_base(left, right):
    if left is None or right is None:
        return None
    if is_zero(left):
        return right
    if is_zero(right):
        return left
    if equivalent(left, right):
        return left
    if same_double_angle(left, right):
        return right
    if same_double_angle(right, left):
        return left
    return None


def tan_sub_base(node, var):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "const"):
        return num(0)
    if kind == "sym":
        if node[1] == var:
            return None
        return num(0)
    if kind == "fn":
        if not depends(node[2], var):
            return num(0)
        if node[1] in ("tan", "sec"):
            pair = match_linear_angle(node[2], var)
            if pair is not None and is_zero(pair[1]) and is_int_num(pair[0]):
                if pair[0][1] == 2:
                    return sim(div(node[2], num(2)))
            return node[2]
        if node[1] in ("sin", "cos"):
            pair = match_linear_angle(node[2], var)
            if pair is None or same(pair[0], num(1)) or same(pair[0], num(-1)):
                return None
            return sim(div(node[2], num(2)))
        return None
    if kind == "pow":
        return tan_sub_base(node[1], var)
    if kind == "div":
        left = tan_sub_base(node[1], var)
        right = tan_sub_base(node[2], var)
        return merge_tan_sub_base(left, right)
    base = num(0)
    i = 0
    while i < len(node[1]):
        cur = tan_sub_base(node[1][i], var)
        if cur is None:
            return None
        base = merge_tan_sub_base(base, cur)
        if base is None:
            return None
        i += 1
    return base


def tan_substitute_node(node, base, var, var_name):
    node = sim(node)
    kind = node[0]
    u = sym(var_name)
    den = add([num(1), power(u, num(2))])
    if kind in ("num", "const"):
        return node
    if kind == "sym":
        return None if node[1] == var else node
    if kind == "fn":
        if not depends(node[2], var):
            return fn(node[1], tan_substitute_node(node[2], base, var, var_name))
        if node[1] == "tan" and equivalent(node[2], base):
            return u
        if node[1] == "tan" and same_double_angle(node[2], base):
            return div(mul([num(2), u]), add([num(1), neg(power(u, num(2)))]))
        if node[1] == "sin" and same_double_angle(node[2], base):
            return div(mul([num(2), u]), den)
        if node[1] == "cos" and same_double_angle(node[2], base):
            return div(add([num(1), neg(power(u, num(2)))]), den)
        return None
    if kind == "pow":
        if node[1][0] == "fn" and depends(node[1][2], var):
            if node[1][1] == "tan" and equivalent(node[1][2], base) and is_int_num(node[2]):
                return power(u, node[2])
            if node[1][1] == "sec" and equivalent(node[1][2], base) and same(node[2], num(2)):
                return den
        left = tan_substitute_node(node[1], base, var, var_name)
        right = tan_substitute_node(node[2], base, var, var_name)
        if left is None or right is None:
            return None
        return power(left, right)
    if kind == "div":
        left = tan_substitute_node(node[1], base, var, var_name)
        right = tan_substitute_node(node[2], base, var, var_name)
        if left is None or right is None:
            return None
        return div(left, right)
    out = []
    i = 0
    while i < len(node[1]):
        cur = tan_substitute_node(node[1][i], base, var, var_name)
        if cur is None:
            return None
        out.append(cur)
        i += 1
    return sim((kind, tuple(out)))


def extract_shifted_linear_angle(arg, var):
    arg = sim(arg)
    if not depends(arg, var):
        return None
    if match_linear_angle(arg, var) is None:
        return None
    pair = split_sum(arg)
    if pair is None:
        return arg, num(0), 1
    left, right, sign = pair
    left_dep = depends(left, var)
    right_dep = depends(right, var)
    names = set()
    collect_symbols(arg, names)
    if left_dep and independent_of_names(right, names):
        return left, right, sign
    if sign > 0 and right_dep and independent_of_names(left, names):
        return right, left, sign
    return None


def match_shifted_tan_cot_term(node, var):
    for name in ("tan", "cot"):
        part = match_simple_trig_term_norm(node, name)
        if part is None:
            continue
        coeff, arg = part
        if depends(coeff, var):
            return None
        shift_info = extract_shifted_linear_angle(arg, var)
        if shift_info is None:
            return None
        base, shift, sign = shift_info
        if not is_zero(shift):
            try:
                shift_value = eval_numeric(shift, {})
            except Exception:
                shift_value = None
            if shift_value is not None and shift_value < 0:
                shift = sim(neg(shift))
                sign = -sign
        return coeff, name, base, shift, sign
    return None


def shifted_tan_sub_expr(name, tan_shift, sign, u):
    if name == "tan":
        if sign > 0:
            return div(add([u, tan_shift]), add([num(1), neg(mul([u, tan_shift]))]))
        return div(add([u, neg(tan_shift)]), add([num(1), mul([u, tan_shift])]))
    if sign > 0:
        return div(add([num(1), neg(mul([u, tan_shift]))]), add([u, tan_shift]))
    return div(add([num(1), mul([u, tan_shift])]), add([u, neg(tan_shift)]))


def solve_shifted_tan_cot_expr(expr, var, start_val, end_val, deg_mode, lines, start_inclusive=True, end_inclusive=True):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    trig_terms = []
    const_term = num(0)
    base = None
    i = 0
    while i < len(terms):
        cur = sim(terms[i])
        part = match_shifted_tan_cot_term(cur, var)
        if part is None:
            if depends(cur, var):
                return None
            const_term = sim(add([const_term, cur]))
            i += 1
            continue
        coeff, name, this_base, shift, sign = part
        if base is None:
            base = this_base
        elif not equivalent(base, this_base):
            return None
        trig_terms.append((coeff, name, shift, sign))
        i += 1
    if base is None:
        return None
    shifted_count = 0
    i = 0
    while i < len(trig_terms):
        if not is_zero(trig_terms[i][2]):
            shifted_count += 1
        i += 1
    if shifted_count == 0:
        return None
    angle = match_linear_angle(base, var)
    if angle is None:
        return None

    u = sym("u")
    transformed_terms = []
    rational_terms = []
    out = list(lines)
    out.append("Let u = tan(" + show(base) + ").")

    saw_cot = False
    saw_tan_add = False
    saw_tan_sub = False
    saw_cot_add = False
    saw_cot_sub = False
    shift_values = {}

    i = 0
    while i < len(trig_terms):
        coeff, name, shift, sign = trig_terms[i]
        if name == "cot":
            saw_cot = True
        if is_zero(shift):
            if name == "tan":
                numerator = u
                denominator = num(1)
            else:
                numerator = num(1)
                denominator = u
        else:
            try:
                tan_shift = exact_trig_value("tan", shift)
            except Exception:
                return None
            shift_values[sig(shift)] = (shift, tan_shift)
            if name == "tan":
                if sign > 0:
                    saw_tan_add = True
                else:
                    saw_tan_sub = True
            else:
                if sign > 0:
                    saw_cot_add = True
                else:
                    saw_cot_sub = True
            if name == "tan":
                if sign > 0:
                    numerator = add([u, tan_shift])
                    denominator = add([num(1), neg(mul([u, tan_shift]))])
                else:
                    numerator = add([u, neg(tan_shift)])
                    denominator = add([num(1), mul([u, tan_shift])])
            else:
                if sign > 0:
                    numerator = add([num(1), neg(mul([u, tan_shift]))])
                    denominator = add([u, tan_shift])
                else:
                    numerator = add([num(1), mul([u, tan_shift])])
                    denominator = add([u, neg(tan_shift)])
        core = numerator if is_one(denominator) else div(numerator, denominator)
        transformed_terms.append(sim(mul([coeff, core])))
        rational_terms.append((coeff, numerator, denominator))
        i += 1

    if saw_cot:
        out.append("Use cot(A) = 1/tan(A).")
    if saw_tan_add:
        out.append(FORMULA_TAN_ADD)
    if saw_tan_sub:
        out.append(FORMULA_TAN_SUB)
    if saw_cot_add:
        out.append(FORMULA_COT_ADD_TAN)
    if saw_cot_sub:
        out.append(FORMULA_COT_SUB_TAN)
    for key in sorted(shift_values.keys()):
        shift, tan_shift = shift_values[key]
        out.append("tan(" + show(shift) + ") = " + show(tan_shift) + ".")

    transformed_expr = sim(add(transformed_terms + [const_term]))
    out.append(equation_line(transformed_expr, num(0)))

    denom_unique = []
    seen_denoms = set()
    i = 0
    while i < len(rational_terms):
        key = sig(rational_terms[i][2])
        if key not in seen_denoms and not is_one(rational_terms[i][2]):
            seen_denoms.add(key)
            denom_unique.append(rational_terms[i][2])
        i += 1
    poly_expr = transformed_expr
    if len(denom_unique) != 0:
        denom_product = make_mul(denom_unique)
        poly_terms = []
        i = 0
        while i < len(rational_terms):
            coeff, numerator, denominator = rational_terms[i]
            factors = []
            j = 0
            while j < len(denom_unique):
                if sig(denom_unique[j]) != sig(denominator):
                    factors.append(denom_unique[j])
                j += 1
            factor = make_mul(factors)
            poly_terms.append(sim(mul([coeff, numerator, factor])))
            i += 1
        poly_terms.append(sim(mul([const_term, denom_product])))
        poly_expr = sim(expand_embedded_small(make_add(poly_terms)))
        out.append("Multiply through by " + show(denom_product) + ".")
        out.append(equation_line(poly_expr, num(0)))

    poly_coeffs = extract_polynomial_symbol(poly_expr, "u", 6)
    if poly_coeffs is None:
        return None
    poly_degree = polynomial_degree_coeff_map(poly_coeffs, 6)
    coeff_list = polynomial_coeff_nodes_low(poly_coeffs, poly_degree)
    exact_root_info = solve_polynomial_exact_low_degree(coeff_list, poly_degree)
    root_nodes = None
    poly_note = None
    if exact_root_info is not None:
        roots = exact_root_info["values"]
        root_nodes = exact_root_info["nodes"]
        poly_note = exact_root_info["note"]
    else:
        roots = solve_polynomial_numeric(coeff_list)
    numeric_root_solver = False
    if roots is None:
        bound = polynomial_real_root_bound(poly_coeffs, poly_degree)
        roots = solve_polynomial_numeric_interval(poly_coeffs, poly_degree, -bound, bound)
        numeric_root_solver = True
    if roots is None:
        return None
    if poly_note is not None:
        out.append(poly_note)
    if numeric_root_solver:
        out.append("Solve the polynomial in u numerically.")
    if len(roots) == 0:
        out.append("No valid trig values.")
        return [], compact_lines(out)

    root_bits = []
    i = 0
    while i < len(roots):
        root_bits.append(concise_root_text(root_nodes[i] if root_nodes is not None else None, roots[i], 6))
        i += 1
    out.append("u = [" + ", ".join(root_bits) + "]")

    valid = []
    i = 0
    while i < len(roots):
        root_val = roots[i]
        out.append("tan(" + show(base) + ") = " + concise_root_text(root_nodes[i] if root_nodes is not None else None, root_val, 6))
        sols, angles = solve_angle_value("tan", angle[0], angle[1], root_val, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
        if len(angles) != 0:
            bits = []
            j = 0
            while j < len(angles):
                bits.append(angle_text(angles[j], deg_mode))
                j += 1
            out.append(show(base) + " = [" + ", ".join(bits) + "]" + ("" if deg_mode else " rad"))
        j = 0
        while j < len(sols):
            check = solve_numeric_value(expr, var, sols[j], deg_mode)
            if check is not None and abs(check) < 1e-6:
                append_unique_solve_value(valid, sols[j])
            j += 1
        i += 1
    valid = dedupe_values(valid)
    if len(valid) == 0:
        out.append("No solutions in the interval.")
        return [], compact_lines(out)
    bits = []
    i = 0
    while i < len(valid):
        bits.append(final_angle_text(valid[i], deg_mode))
        i += 1
    out.append(var + " = [" + ", ".join(bits) + "]")
    return valid, compact_lines(out)


def quadratic_roots(a, b, c, deg_mode):
    a_f = eval_numeric_mode(a, {}, deg_mode)
    b_f = eval_numeric_mode(b, {}, deg_mode)
    c_f = eval_numeric_mode(c, {}, deg_mode)
    if abs(a_f) < 1e-12:
        if abs(b_f) < 1e-12:
            return []
        return [-c_f / b_f]
    disc = b_f * b_f - 4.0 * a_f * c_f
    if disc < -1e-9:
        return []
    if disc < 0:
        disc = 0.0
    root = math.sqrt(disc)
    return [(-b_f + root) / (2.0 * a_f), (-b_f - root) / (2.0 * a_f)]


def solve_polynomial_exact_low_degree(coeff_nodes, degree=None):
    coeff_nodes = [full_simplify(c) for c in coeff_nodes]
    if degree is None:
        degree = len(coeff_nodes) - 1
        while degree > 0 and is_zero(coeff_nodes[degree]):
            degree -= 1
    if degree <= 0:
        return None
    if degree == 1:
        a = coeff_nodes[1]
        b = coeff_nodes[0]
        if is_zero(a):
            return None
        root_node = full_simplify(div(neg(b), a))
        try:
            root_value = eval_numeric(root_node, {})
        except Exception:
            return None
        if not is_finite_value(root_value):
            return None
        return {
            "values": [root_value],
            "nodes": [root_node],
            "note": "Solve the linear equation in u.",
        }
    if degree == 2:
        a = coeff_nodes[2]
        b = coeff_nodes[1]
        c = coeff_nodes[0]
        if is_zero(a):
            return solve_polynomial_exact_low_degree(coeff_nodes, 1)
        disc = full_simplify(add([power(b, num(2)), neg(mul([num(4), a, c]))]))
        try:
            disc_value = eval_numeric(disc, {})
        except Exception:
            return None
        if disc_value is None or not is_finite_value(disc_value):
            return None
        if disc_value < -1e-9:
            return {
                "values": [],
                "nodes": [],
                "note": "Use the quadratic formula.",
            }
        if disc_value < 0:
            disc_value = 0.0
            disc = num(0)
        den = full_simplify(mul([num(2), a]))
        if is_zero(disc):
            root_nodes = [full_simplify(div(neg(b), den))]
        else:
            sqrt_disc = full_simplify(fn("sqrt", disc))
            root_nodes = [
                full_simplify(div(add([neg(b), sqrt_disc]), den)),
                full_simplify(div(add([neg(b), neg(sqrt_disc)]), den)),
            ]
        values = []
        nodes = []
        i = 0
        while i < len(root_nodes):
            try:
                value = eval_numeric(root_nodes[i], {})
            except Exception:
                return None
            if not is_finite_value(value):
                return None
            dup = False
            j = 0
            while j < len(values):
                if abs(values[j] - value) < 1e-8:
                    dup = True
                    break
                j += 1
            if not dup:
                values.append(value)
                nodes.append(root_nodes[i])
            i += 1
        return {
            "values": values,
            "nodes": nodes,
            "note": "Use the quadratic formula.",
        }
    return None


def lcm(a, b):
    if a == 0:
        return abs(b)
    if b == 0:
        return abs(a)
    return abs(a * b) // gcd(a, b)


def divisors(n):
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


def integer_poly_coeffs(coeff_nodes):
    den = 1
    i = 0
    while i < len(coeff_nodes):
        if not is_num(coeff_nodes[i]):
            return None
        den = lcm(den, coeff_nodes[i][2])
        i += 1
    out = []
    i = 0
    while i < len(coeff_nodes):
        out.append(coeff_nodes[i][1] * (den // coeff_nodes[i][2]))
        i += 1
    return out


def poly_eval_float(coeffs_high, x):
    value = 0.0
    i = 0
    while i < len(coeffs_high):
        value = value * x + coeffs_high[i]
        i += 1
    return value


def synthetic_divide_float(coeffs_high, root):
    out = [coeffs_high[0] * 1.0]
    i = 1
    while i < len(coeffs_high):
        out.append(coeffs_high[i] + out[-1] * root)
        i += 1
    return out[:-1], out[-1]


def rational_root_candidates(int_coeffs_high):
    lead = int_coeffs_high[0]
    const = int_coeffs_high[-1]
    if const == 0:
        return [0.0]
    p_bits = divisors(const)
    q_bits = divisors(lead)
    seen = {}
    out = []
    i = 0
    while i < len(p_bits):
        j = 0
        while j < len(q_bits):
            value = p_bits[i] * 1.0 / q_bits[j]
            key = round(value, 12)
            if key not in seen:
                seen[key] = True
                out.append(value)
            key = round(-value, 12)
            if key not in seen:
                seen[key] = True
                out.append(-value)
            j += 1
        i += 1
    return out


def solve_polynomial_numeric(coeff_nodes):
    coeff_nodes = [full_simplify(c) for c in coeff_nodes]
    exact = solve_polynomial_exact_low_degree(coeff_nodes)
    if exact is not None:
        return exact["values"]
    ints = integer_poly_coeffs(coeff_nodes)
    if ints is None:
        return None
    high = list(reversed(ints))
    while len(high) > 1 and high[0] == 0:
        high = high[1:]
    if len(high) <= 1:
        return []
    roots = []
    while len(high) > 3:
        if high[-1] == 0:
            roots.append(0.0)
            high = high[:-1]
            continue
        candidates = rational_root_candidates(high)
        found = None
        i = 0
        while i < len(candidates):
            if abs(poly_eval_float(high, candidates[i])) < 1e-8:
                found = candidates[i]
                break
            i += 1
        if found is None:
            return None
        roots.append(found)
        high, rem = synthetic_divide_float(high, found)
        if abs(rem) > 1e-6:
            return None
        i = 0
        while i < len(high):
            if abs(high[i] - round(high[i])) < 1e-8:
                high[i] = int(round(high[i]))
            i += 1
    if len(high) == 3:
        extra = quadratic_roots(num(high[0]), num(high[1]), num(high[2]), True)
        i = 0
        while i < len(extra):
            roots.append(extra[i])
            i += 1
    elif len(high) == 2:
        if abs(high[0]) < 1e-12:
            return None
        roots.append(-high[1] * 1.0 / high[0])
    return dedupe_values(roots, 1e-8)


def polynomial_degree_coeff_map(coeffs, max_deg):
    degree = max_deg
    while degree > 0 and is_zero(coeffs[degree]):
        degree -= 1
    return degree


def polynomial_float_coeffs(coeffs, degree):
    out = []
    i = degree
    while i >= 0:
        out.append(eval_numeric(coeffs[i], {}))
        i -= 1
    return out


def polynomial_coeff_nodes_low(coeffs, degree):
    out = []
    i = 0
    while i <= degree:
        out.append(coeffs[i])
        i += 1
    return out


def solve_polynomial_numeric_interval(coeffs, degree, lo, hi, samples=256):
    if degree < 1:
        return []
    high = polynomial_float_coeffs(coeffs, degree)

    def poly(x):
        return poly_eval_float(high, x)

    roots = []
    step = (hi - lo) * 1.0 / samples
    x1 = lo
    y1 = poly(x1)
    if abs(y1) < 1e-8:
        roots.append(x1)
    i = 1
    while i <= samples:
        x2 = lo + step * i
        y2 = poly(x2)
        if abs(y2) < 1e-8:
            roots.append(x2)
        elif y1 * y2 < 0:
            left = x1
            right = x2
            f_left = y1
            j = 0
            while j < 60:
                mid = (left + right) / 2.0
                f_mid = poly(mid)
                if abs(f_mid) < 1e-10:
                    left = mid
                    right = mid
                    break
                if f_left * f_mid <= 0:
                    right = mid
                else:
                    left = mid
                    f_left = f_mid
                j += 1
            roots.append((left + right) / 2.0)
        x1 = x2
        y1 = y2
        i += 1
    return dedupe_values(roots, 1e-6)


def polynomial_real_root_bound(coeffs, degree):
    high = polynomial_float_coeffs(coeffs, degree)
    lead = abs(high[0])
    if lead < 1e-12:
        return 10.0
    bound = 0.0
    i = 1
    while i < len(high):
        cur = abs(high[i]) / lead
        if cur > bound:
            bound = cur
        i += 1
    return 1.0 + bound


def trig_base_solutions(name, value, deg_mode):
    out = []
    if name == "sin":
        if value < -1.0000001 or value > 1.0000001:
            return out
        if value < -1:
            value = -1
        if value > 1:
            value = 1
        if deg_mode:
            alpha = to_degrees(math.asin(value))
            out = [alpha, 180.0 - alpha]
        else:
            alpha = math.asin(value)
            out = [alpha, math.pi - alpha]
    elif name == "cos":
        if value < -1.0000001 or value > 1.0000001:
            return out
        if value < -1:
            value = -1
        if value > 1:
            value = 1
        alpha = to_degrees(math.acos(value)) if deg_mode else math.acos(value)
        out = [alpha, -alpha]
    elif name == "sec":
        if abs(value) < 1e-12:
            return out
        return trig_base_solutions("cos", 1.0 / value, deg_mode)
    elif name == "cosec":
        if abs(value) < 1e-12:
            return out
        return trig_base_solutions("sin", 1.0 / value, deg_mode)
    else:
        if name == "cot":
            if abs(value) < 1e-12:
                alpha = 90.0 if deg_mode else math.pi / 2.0
            else:
                alpha = to_degrees(math.atan(1.0 / value)) if deg_mode else math.atan(1.0 / value)
        else:
            alpha = to_degrees(math.atan(value)) if deg_mode else math.atan(value)
        out = [alpha]
    uniq = []
    i = 0
    while i < len(out):
        seen = False
        j = 0
        while j < len(uniq):
            if abs(out[i] - uniq[j]) < 1e-9:
                seen = True
                break
            j += 1
        if not seen:
            uniq.append(out[i])
        i += 1
    return uniq


def solve_n_range_for_interval(mult, offset, base_angle, step, start_val, end_val):
    if abs(mult) < 1e-12 or abs(step) < 1e-12:
        return None
    low = mult * start_val + offset - base_angle
    high = mult * end_val + offset - base_angle
    if low > high:
        low, high = high, low
    start_n = int(math.floor(low / step)) - 1
    end_n = int(math.ceil(high / step)) + 1
    return start_n, end_n


def append_unique_float(values, value, tol=1e-7):
    i = 0
    while i < len(values):
        if abs(values[i] - value) < tol:
            return
        i += 1
    values.append(value)


def solve_angle_value(name, mult_node, offset_node, target_value, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    mult = eval_numeric(mult_node, {})
    offset = eval_numeric(offset_node, {})
    period = 360.0 if deg_mode else 2.0 * math.pi
    if name in ("tan", "cot"):
        base_period = 180.0 if deg_mode else math.pi
    else:
        base_period = period
    base = trig_base_solutions(name, target_value, deg_mode)
    sols = []
    angles = []
    i = 0
    while i < len(base):
        n_range = solve_n_range_for_interval(mult, offset, base[i], base_period, start_val, end_val)
        if n_range is not None:
            n = n_range[0]
            while n <= n_range[1]:
                angle = base[i] + n * base_period
                x_val = (angle - offset) / mult
                if within_interval(x_val, start_val, end_val, start_inclusive, end_inclusive):
                    append_unique_float(angles, angle)
                    append_unique_float(sols, x_val)
                n += 1
        i += 1
    sols.sort()
    angles.sort()
    return sols, angles


def number_node(value):
    rounded = round(value)
    if abs(value - rounded) < 1e-12:
        return num(int(rounded))
    text = ("%.12f" % value).rstrip("0").rstrip(".")
    if text == "-0":
        text = "0"
    return num_text(text)


def solve_sec_cosec_product_expr(expr):
    expr = sim(expr)
    terms = [expr]
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    if len(terms) != 2:
        return None
    first = match_sec_cosec_product(terms[0])
    second = match_sec_cosec_product(terms[1])
    if first is None and second is None:
        return None
    if first is not None and second is None:
        coeff, arg = first
        other = terms[1]
    elif second is not None and first is None:
        coeff, arg = second
        other = terms[0]
    else:
        return None
    names = set()
    collect_symbols(arg, names)
    i = 0
    name_list = list(names)
    while i < len(name_list):
        if depends(other, name_list[i]):
            return None
        i += 1
    new_expr = sim(add([mul([num(2), coeff]), mul([other, fn("sin", mul([num(2), arg]))])]))
    lines = [
        "Use sec(A)cosec(A) = 1/(sin(A)cos(A)).",
        "Multiply through by sin(" + show(arg) + ")*cos(" + show(arg) + ").",
        "Use 2sin(A)cos(A) = sin(2A).",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def match_tan2_cot_product(node):
    node = sim(node)
    if node[0] != "mul":
        return None
    items = list(flat(node, "mul"))
    coeff_bits = []
    tan_base = None
    cot_base = None
    i = 0
    while i < len(items):
        tan_match = match_tan_double_term(items[i])
        cot_match = match_cot_simple_term(items[i])
        if tan_match is not None:
            if tan_base is not None:
                return None
            coeff_bits.append(tan_match[0])
            tan_base = tan_match[1]
        elif cot_match is not None:
            if cot_base is not None:
                return None
            coeff_bits.append(cot_match[0])
            cot_base = cot_match[1]
        else:
            coeff_bits.append(items[i])
        i += 1
    if tan_base is None or cot_base is None or not same(tan_base, cot_base):
        return None
    coeff = num(1)
    j = 0
    while j < len(coeff_bits):
        coeff = sim(mul([coeff, coeff_bits[j]]))
        j += 1
    return coeff, tan_base


def solve_tan2_cot_expr(expr):
    expr = sim(expr)
    terms = [expr]
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    if len(terms) != 2:
        return None
    allowed = set()
    prod = match_tan2_cot_product(terms[0])
    if prod is not None:
        coeff, arg = prod
        collect_symbols(arg, allowed)
    if prod is not None:
        const = terms[1]
        names = set()
        collect_symbols(const, names)
        ok = True
        for name in names:
            if name in allowed:
                ok = False
                break
        if not ok:
            return None
        t = fn("tan", arg)
        new_expr = sim(add([mul([neg(const), power(t, num(2))]), add([mul([num(2), coeff]), const])]))
        lines = [
            "Let t = tan(" + show(arg) + ").",
            "Use tan(2A) = 2tan(A)/(1-tan(A)^2) and cot(A) = 1/tan(A).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    prod = match_tan2_cot_product(terms[1])
    if prod is not None:
        coeff, arg = prod
        allowed = set()
        collect_symbols(arg, allowed)
        const = terms[0]
        names = set()
        collect_symbols(const, names)
        ok = True
        for name in names:
            if name in allowed:
                ok = False
                break
        if not ok:
            return None
        t = fn("tan", arg)
        new_expr = sim(add([mul([neg(const), power(t, num(2))]), add([mul([num(2), coeff]), const])]))
        lines = [
            "Let t = tan(" + show(arg) + ").",
            "Use tan(2A) = 2tan(A)/(1-tan(A)^2) and cot(A) = 1/tan(A).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines

    tan_match = match_tan_double_term(terms[0])
    cot_match = match_cot_simple_term(terms[1])
    if tan_match is None or cot_match is None:
        tan_match = match_tan_double_term(terms[1])
        cot_match = match_cot_simple_term(terms[0])
    if tan_match is not None and cot_match is not None and same(tan_match[1], cot_match[1]):
        arg = tan_match[1]
        t = fn("tan", arg)
        new_expr = sim(add([mul([add([mul([num(2), tan_match[0]]), neg(cot_match[0])]), power(t, num(2))]), cot_match[0]]))
        lines = [
            "Let t = tan(" + show(arg) + ").",
            "Use tan(2A) = 2tan(A)/(1-tan(A)^2) and cot(A) = 1/tan(A).",
            "Multiply through by tan(" + show(arg) + ")*(1-tan(" + show(arg) + ")^2).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    return None


def solve_tan2_tan_expr(expr, var):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    tan2_match = None
    tan_match = None
    const = num(0)
    i = 0
    while i < len(terms):
        cur = sim(terms[i])
        part = match_tan_double_term(cur)
        if part is not None and depends(part[1], var):
            if tan2_match is not None:
                return None
            tan2_match = part
            i += 1
            continue
        part = match_simple_trig_term(cur, "tan")
        if part is not None and depends(part[1], var):
            if tan_match is not None:
                return None
            tan_match = part
            i += 1
            continue
        if depends(cur, var):
            return None
        const = sim(add([const, cur]))
        i += 1
    if tan2_match is None or tan_match is None or not same(tan2_match[1], tan_match[1]):
        return None
    base = tan_match[1]
    t = fn("tan", base)
    new_expr = sim(add([
        mul([neg(tan_match[0]), power(t, num(3))]),
        mul([neg(const), power(t, num(2))]),
        mul([add([mul([num(2), tan2_match[0]]), tan_match[0]]), t]),
        const,
    ]))
    lines = [
        "Let u = tan(" + show(base) + ").",
        "Use tan(2A) = 2tan(A)/(1-tan(A)^2).",
        "Multiply through by 1-tan(" + show(base) + ")^2.",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_sec_tan_linear_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sec", "sec", 1, False), ("tan", "tan", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sec"]) or is_zero(coeffs["tan"]):
        return None
    new_expr = sim(add([coeffs["sec"], mul([coeffs["tan"], fn("sin", arg)]), mul([const, fn("cos", arg)])]))
    lines = [
        "Use sec(A) = 1/cos(A) and tan(A) = sin(A)/cos(A).",
        "Multiply through by cos(" + show(arg) + ").",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_cosec_cot_linear_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cosec", "cosec", 1, False), ("cot", "cot", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cosec"]) or is_zero(coeffs["cot"]):
        return None
    new_expr = sim(add([coeffs["cosec"], mul([coeffs["cot"], fn("cos", arg)]), mul([const, fn("sin", arg)])]))
    lines = [
        "Use cosec(A) = 1/sin(A) and cot(A) = cos(A)/sin(A).",
        "Multiply through by sin(" + show(arg) + ").",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_sec_cosec_linear_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sec", "sec", 1, False), ("cosec", "cosec", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sec"]) or is_zero(coeffs["cosec"]) or not is_zero(const):
        return None
    new_expr = sim(add([mul([coeffs["sec"], fn("sin", arg)]), mul([coeffs["cosec"], fn("cos", arg)])]))
    lines = [
        "Use sec(A) = 1/cos(A) and cosec(A) = 1/sin(A).",
        "Multiply through by sin(" + show(arg) + ")*cos(" + show(arg) + ").",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_sec2_cosec_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sec2", "sec", 2, False), ("cosec", "cosec", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sec2"]) or is_zero(coeffs["cosec"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["sec2"], fn("sin", arg)]), mul([coeffs["cosec"], power(fn("cos", arg), num(2))])]))
    new_expr = sim(add([mul([neg(coeffs["cosec"]), power(fn("sin", arg), num(2))]), mul([coeffs["sec2"], fn("sin", arg)]), coeffs["cosec"]]))
    lines = [
        "Multiply through by sin(" + show(arg) + ")*cos(" + show(arg) + ")^2.",
        equation_line(step1, num(0)),
        "Use cos^2(A) = 1-sin^2(A).",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_cosec2_sec_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cosec2", "cosec", 2, False), ("sec", "sec", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cosec2"]) or is_zero(coeffs["sec"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["cosec2"], fn("cos", arg)]), mul([coeffs["sec"], power(fn("sin", arg), num(2))])]))
    new_expr = sim(add([mul([neg(coeffs["sec"]), power(fn("cos", arg), num(2))]), mul([coeffs["cosec2"], fn("cos", arg)]), coeffs["sec"]]))
    lines = [
        "Multiply through by sin(" + show(arg) + ")^2*cos(" + show(arg) + ").",
        equation_line(step1, num(0)),
        "Use sin^2(A) = 1-cos^2(A).",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_sec_tan_mix_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sec", "sec", 1, False), ("tan", "tan", 1, False), ("sin", "sin", 1, False), ("cos", "cos", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sec"]) and is_zero(coeffs["tan"]):
        return None
    pieces = [coeffs["sec"], mul([coeffs["tan"], fn("sin", arg)]), mul([coeffs["sin"], fn("sin", arg), fn("cos", arg)]), mul([coeffs["cos"], power(fn("cos", arg), num(2))])]
    if not is_zero(const):
        pieces.append(mul([const, fn("cos", arg)]))
    new_expr = sim(add(pieces))
    lines = [
        "Use sec(A) = 1/cos(A) and tan(A) = sin(A)/cos(A).",
        "Multiply through by cos(" + show(arg) + ").",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_cosec_cot_mix_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cosec", "cosec", 1, False), ("cot", "cot", 1, False), ("sin", "sin", 1, False), ("cos", "cos", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cosec"]) and is_zero(coeffs["cot"]):
        return None
    pieces = [coeffs["cosec"], mul([coeffs["cot"], fn("cos", arg)]), mul([coeffs["cos"], fn("sin", arg), fn("cos", arg)]), mul([coeffs["sin"], power(fn("sin", arg), num(2))])]
    if not is_zero(const):
        pieces.append(mul([const, fn("sin", arg)]))
    new_expr = sim(add(pieces))
    lines = [
        "Use cosec(A) = 1/sin(A) and cot(A) = cos(A)/sin(A).",
        "Multiply through by sin(" + show(arg) + ").",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_sin_cot_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sin", "sin", 1, False), ("cot", "cot", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sin"]) or is_zero(coeffs["cot"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["sin"], power(fn("sin", arg), num(2))]), mul([coeffs["cot"], fn("cos", arg)])]))
    new_expr = sim(add([mul([neg(coeffs["sin"]), power(fn("cos", arg), num(2))]), mul([coeffs["cot"], fn("cos", arg)]), coeffs["sin"]]))
    lines = [
        "Multiply through by sin(" + show(arg) + ").",
        equation_line(step1, num(0)),
        "Use sin^2(A) = 1-cos^2(A).",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_cos_tan_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cos", "cos", 1, False), ("tan", "tan", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cos"]) or is_zero(coeffs["tan"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["cos"], power(fn("cos", arg), num(2))]), mul([coeffs["tan"], fn("sin", arg)])]))
    new_expr = sim(add([mul([neg(coeffs["cos"]), power(fn("sin", arg), num(2))]), mul([coeffs["tan"], fn("sin", arg)]), coeffs["cos"]]))
    lines = [
        "Multiply through by cos(" + show(arg) + ").",
        equation_line(step1, num(0)),
        "Use cos^2(A) = 1-sin^2(A).",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_sin_sec_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sin", "sin", 1, False), ("sec", "sec", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sin"]) or is_zero(coeffs["sec"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["sin"], fn("sin", arg), fn("cos", arg)]), coeffs["sec"]]))
    step2 = sim(add([mul([div(coeffs["sin"], num(2)), fn("sin", mul([num(2), arg]))]), coeffs["sec"]]))
    lines = [
        "sec(" + show(arg) + ") is undefined when cos(" + show(arg) + ") = 0, so multiply through by cos(" + show(arg) + ").",
        equation_line(step1, num(0)),
        "Use 2sin(A)cos(A) = sin(2A).",
        equation_line(step2, num(0)),
    ]
    return step2, lines


def solve_cos_cosec_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cos", "cos", 1, False), ("cosec", "cosec", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cos"]) or is_zero(coeffs["cosec"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["cos"], fn("sin", arg), fn("cos", arg)]), coeffs["cosec"]]))
    step2 = sim(add([mul([div(coeffs["cos"], num(2)), fn("sin", mul([num(2), arg]))]), coeffs["cosec"]]))
    lines = [
        "cosec(" + show(arg) + ") is undefined when sin(" + show(arg) + ") = 0, so multiply through by sin(" + show(arg) + ").",
        equation_line(step1, num(0)),
        "Use 2sin(A)cos(A) = sin(2A).",
        equation_line(step2, num(0)),
    ]
    return step2, lines


def solve_tan_cot_linear_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("tan", "tan", 1, False), ("cot", "cot", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["tan"]) or is_zero(coeffs["cot"]):
        return None
    t = fn("tan", arg)
    new_expr = sim(add([mul([coeffs["tan"], power(t, num(2))]), mul([const, t]), coeffs["cot"]]))
    lines = [
        "tan(" + show(arg) + ") and cot(" + show(arg) + ") are both defined only when sin(" + show(arg) + ") and cos(" + show(arg) + ") are non-zero, so multiply through by tan(" + show(arg) + ").",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_cosec_cot_halfangle_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cosec", "cosec", 1, False), ("cot", "cot", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cosec"]) or is_zero(coeffs["cot"]):
        return None
    half = div(arg, num(2))
    if equivalent(coeffs["cosec"], coeffs["cot"]):
        new_expr = sim(add([mul([coeffs["cosec"], fn("cot", half)]), const]))
        lines = [
            "Use cosec(A)+cot(A) = cot(A/2).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    if equivalent(coeffs["cosec"], neg(coeffs["cot"])):
        new_expr = sim(add([mul([coeffs["cosec"], fn("tan", half)]), const]))
        lines = [
            "Use cosec(A)-cot(A) = tan(A/2).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    return None


def solve_reciprocal_conjugate_pair_expr(expr, var):
    _ = var
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) < 2:
        return None
    i = 0
    while i < len(terms):
        j = i + 1
        while j < len(terms):
            pair_expr = sim(add([terms[i], terms[j]]))
            rewritten = reciprocal_pair_sum_working(pair_expr)
            if rewritten is None:
                rewritten = reciprocal_conjugate_pair_working(pair_expr)
            if rewritten is not None:
                final_pair, pair_lines = rewritten
                kept = [final_pair]
                k = 0
                while k < len(terms):
                    if k != i and k != j:
                        kept.append(terms[k])
                    k += 1
                new_expr = sim(add(kept))
                lines = list(pair_lines)
                if not cheap_same(new_expr, final_pair):
                    lines.append(equation_line(new_expr, num(0)))
                return new_expr, compact_lines(lines)
            j += 1
        i += 1
    return None


def match_nonzero_reciprocal_factor(node):
    if node[0] == "fn" and node[1] in ("sec", "cosec"):
        return node[1]
    if node[0] == "pow" and is_int_num(node[2]) and node[2][1] > 0 and node[1][0] == "fn" and node[1][1] in ("sec", "cosec"):
        return node[1][1]
    return None


def solve_cancel_nonzero_reciprocal_factor_expr(expr, var):
    _ = var
    factored, _note = factor_common_term_once(expr, var)
    if factored is None or factored[0] != "mul":
        return None
    items = list(flat(factored, "mul"))
    if len(items) != 2:
        return None
    factor = None
    rest = None
    i = 0
    while i < len(items):
        if match_nonzero_reciprocal_factor(items[i]) is not None:
            factor = items[i]
            rest = items[1 - i]
            break
        i += 1
    if factor is None or not depends(rest, var):
        return None
    lines = [
        show(factor) + " is never 0 when defined, so divide through by " + show(factor) + ".",
        equation_line(rest, num(0)),
    ]
    return rest, lines


def solve_conjugate_fraction_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) != 3:
        return None
    first = None
    second = None
    const = num(0)
    i = 0
    while i < len(terms):
        cur = sim(terms[i])
        part = match_scaled_div(cur)
        if part is not None:
            coeff, top, bot = part
            top_sin = match_simple_trig_term(top, "sin")
            bot_sin = match_simple_trig_term(bot, "sin")
            top_pm = match_one_pm_cos(top)
            bot_pm = match_one_pm_cos(bot)
            if top_sin is not None and bot_pm is not None and is_one(top_sin[0]) and first is None:
                if not equivalent(top_sin[1], bot_pm[1]):
                    return None
                first = (coeff, top_sin[1], bot_pm[0])
                i += 1
                continue
            if bot_sin is not None and top_pm is not None and is_one(bot_sin[0]) and second is None:
                if not equivalent(bot_sin[1], top_pm[1]):
                    return None
                second = (coeff, bot_sin[1], top_pm[0])
                i += 1
                continue
        if depends(cur, var):
            return None
        const = sim(add([const, cur]))
        i += 1
    if first is None or second is None:
        return None
    if not equivalent(first[1], second[1]) or not equivalent(first[0], second[0]) or first[2] != second[2]:
        return None
    arg = first[1]
    coeff = first[0]
    rewritten = sim(div(mul([coeff, add([num(1), neg(mul([num(first[2]), fn("cos", arg)]))])]), fn("sin", arg)))
    new_expr = sim(add([mul([num(2), coeff, fn("cosec", arg)]), const]))
    sign_text = "-" if first[2] < 0 else "+"
    lines = [
        "Multiply sin(" + show(arg) + ")/(1" + sign_text + "cos(" + show(arg) + ")) by the conjugate fraction.",
        equation_line(rewritten, num(0)),
        "Add the two fractions over sin(" + show(arg) + ").",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def rewrite_sec_tan_fraction_term(node, var):
    part = match_scaled_div(node)
    if part is None:
        return None
    coeff, top, bot = part
    top_cos = match_simple_trig_term_norm(top, "cos")
    bot_cos = match_simple_trig_term_norm(bot, "cos")
    top_pm_sin = match_one_pm_trig_norm(top, "sin")
    bot_pm_sin = match_one_pm_trig_norm(bot, "sin")

    if top_cos is not None and bot_pm_sin is not None and cheap_same(top_cos[1], bot_pm_sin[1]):
        if depends(top_cos[0], var):
            return None
        arg = top_cos[1]
        total_coeff = sim(mul([coeff, top_cos[0]]))
        if bot_pm_sin[0] > 0:
            new_node = sim(add([mul([total_coeff, fn("sec", arg)]), neg(mul([total_coeff, fn("tan", arg)]))]))
            note = "Use cos(A)/(1+sin(A)) = sec(A)-tan(A)."
        else:
            new_node = sim(add([mul([total_coeff, fn("sec", arg)]), mul([total_coeff, fn("tan", arg)])]))
            note = "Use cos(A)/(1-sin(A)) = sec(A)+tan(A)."
        return new_node, note

    if bot_cos is not None and top_pm_sin is not None and cheap_same(bot_cos[1], top_pm_sin[1]):
        if depends(bot_cos[0], var):
            return None
        arg = bot_cos[1]
        total_coeff = sim(div(coeff, bot_cos[0]))
        if top_pm_sin[0] > 0:
            new_node = sim(add([mul([total_coeff, fn("sec", arg)]), mul([total_coeff, fn("tan", arg)])]))
            note = "Use (1+sin(A))/cos(A) = sec(A)+tan(A)."
        else:
            new_node = sim(add([mul([total_coeff, fn("sec", arg)]), neg(mul([total_coeff, fn("tan", arg)]))]))
            note = "Use (1-sin(A))/cos(A) = sec(A)-tan(A)."
        return new_node, note
    return None


def solve_sec_tan_fraction_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    out = []
    notes = []
    changed = False
    i = 0
    while i < len(terms):
        rewritten = rewrite_sec_tan_fraction_term(terms[i], var)
        if rewritten is not None and depends(terms[i], var):
            out.append(rewritten[0])
            if rewritten[1] not in notes:
                notes.append(rewritten[1])
            changed = True
        else:
            out.append(terms[i])
        i += 1
    if not changed:
        return None
    new_expr = sim(add(out))
    lines = list(notes)
    lines.append(equation_line(new_expr, num(0)))
    return new_expr, lines


def match_ratio_one_minus_cos_plus_sin(node):
    part = match_scaled_div(node)
    if part is None:
        return None
    coeff, top, bot = part
    top_bits = list(flat(top, "add")) if top[0] == "add" else [top]
    bot_bits = list(flat(bot, "add")) if bot[0] == "add" else [bot]
    if len(top_bits) != 3 or len(bot_bits) != 3:
        return None
    base = None
    top_state = {"one": False, "sin": False, "cos": False}
    bot_state = {"one": False, "sin": False, "cos": False}
    i = 0
    while i < 3:
        cur = top_bits[i]
        if same(cur, num(1)):
            top_state["one"] = True
        else:
            c, rest = split_coeff(cur)
            if rest[0] == "fn" and rest[1] in ("sin", "cos") and (is_one(c) or is_minus_one(c)):
                candidate = half_angle_expr(rest[2])
                if candidate is None:
                    return None
                if base is None:
                    base = candidate
                elif not equivalent(base, candidate):
                    return None
                if rest[1] == "sin" and is_one(c):
                    top_state["sin"] = True
                elif rest[1] == "cos" and is_minus_one(c):
                    top_state["cos"] = True
                else:
                    return None
            else:
                return None
        i += 1
    i = 0
    while i < 3:
        cur = bot_bits[i]
        if same(cur, num(1)):
            bot_state["one"] = True
        else:
            c, rest = split_coeff(cur)
            if rest[0] == "fn" and rest[1] in ("sin", "cos") and (is_one(c) or is_minus_one(c)):
                candidate = half_angle_expr(rest[2])
                if candidate is None:
                    return None
                if base is None:
                    base = candidate
                elif not equivalent(base, candidate):
                    return None
                if rest[1] == "sin" and is_one(c):
                    bot_state["sin"] = True
                elif rest[1] == "cos" and is_one(c):
                    bot_state["cos"] = True
                else:
                    return None
            else:
                return None
        i += 1
    if base is None or not (top_state["one"] and top_state["sin"] and top_state["cos"] and bot_state["one"] and bot_state["sin"] and bot_state["cos"]):
        return None
    return coeff, base


def solve_ratio_one_minus_cos_plus_sin_expr(expr, var):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) != 2:
        return None
    ratio = match_ratio_one_minus_cos_plus_sin(terms[0])
    other = terms[1]
    if ratio is None:
        ratio = match_ratio_one_minus_cos_plus_sin(terms[1])
        other = terms[0]
    if ratio is None:
        return None
    coeff, base = ratio
    if depends(coeff, var):
        return None
    new_expr = sim(add([mul([coeff, fn("tan", base)]), other]))
    lines = [
        "Use double-angle identities.",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_cos3_over_sin_plus_sin3_over_cos_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) != 3:
        return None
    cos_term = None
    sin_term = None
    const = num(0)
    i = 0
    while i < len(terms):
        part = match_scaled_div_norm(terms[i])
        if part is not None:
            coeff, top, bot = part
            top_cos = match_simple_trig_term_norm(top, "cos")
            top_sin = match_simple_trig_term_norm(top, "sin")
            bot_sin = match_simple_trig_term_norm(bot, "sin")
            bot_cos = match_simple_trig_term_norm(bot, "cos")
            if top_cos is not None and bot_sin is not None and is_one(top_cos[0]) and is_one(bot_sin[0]):
                if cos_term is not None:
                    return None
                cos_term = (coeff, top_cos[1], bot_sin[1])
                i += 1
                continue
            if top_sin is not None and bot_cos is not None and is_one(top_sin[0]) and is_one(bot_cos[0]):
                if sin_term is not None:
                    return None
                sin_term = (coeff, top_sin[1], bot_cos[1])
                i += 1
                continue
        if depends(terms[i], var):
            return None
        const = sim(add([const, terms[i]]))
        i += 1
    if cos_term is None or sin_term is None:
        return None
    if not equivalent(cos_term[0], sin_term[0]) or not equivalent(cos_term[2], sin_term[2]):
        return None
    base = cos_term[2]
    if not equivalent(cos_term[1], mul([num(3), base])) or not equivalent(sin_term[1], mul([num(3), base])):
        return None
    new_expr = sim(add([mul([num(2), cos_term[0], fn("cot", mul([num(2), base]))]), const]))
    lines = [
        "Use a common denominator.",
        "cos(3*" + show(base) + ")*cos(" + show(base) + ")+sin(3*" + show(base) + ")*sin(" + show(base) + ") = cos(2*" + show(base) + ").",
        "Also sin(" + show(base) + ")*cos(" + show(base) + ") = sin(2*" + show(base) + ")/2.",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def match_cos_times_cot(node):
    if node[0] != "mul":
        return None
    items = list(flat(node, "mul"))
    coeff = num(1)
    cos_arg = None
    cot_arg = None
    i = 0
    while i < len(items):
        if is_num(items[i]):
            coeff = mulq(coeff, items[i])
        elif items[i][0] == "fn" and items[i][1] == "cos":
            if cos_arg is not None:
                return None
            cos_arg = items[i][2]
        elif items[i][0] == "fn" and items[i][1] == "cot":
            if cot_arg is not None:
                return None
            cot_arg = items[i][2]
        else:
            return None
        i += 1
    if cos_arg is None or cot_arg is None:
        return None
    return coeff, cos_arg, cot_arg


def solve_cosec_minus_sin_vs_cos_cot_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    cosec_term = None
    sin_term = None
    cos_cot_term = None
    i = 0
    while i < len(terms):
        cur = terms[i]
        part = match_simple_trig_term_norm(cur, "cosec")
        if part is not None and depends(part[1], var):
            if cosec_term is not None:
                return None
            cosec_term = part
            i += 1
            continue
        part = match_simple_trig_term_norm(cur, "sin")
        if part is not None and depends(part[1], var):
            if sin_term is not None:
                return None
            sin_term = part
            i += 1
            continue
        part = match_cos_times_cot(cur)
        if part is not None and depends(part[1], var):
            if cos_cot_term is not None:
                return None
            cos_cot_term = part
            i += 1
            continue
        if depends(cur, var):
            return None
        if not is_zero(cur):
            return None
        i += 1
    if cosec_term is None or sin_term is None or cos_cot_term is None:
        return None
    if not equivalent(cosec_term[1], sin_term[1]) or not equivalent(cosec_term[1], cos_cot_term[1]):
        return None
    if not equivalent(cosec_term[0], neg(sin_term[0])) or not equivalent(cosec_term[0], neg(cos_cot_term[0])):
        return None
    base = cosec_term[1]
    factored = sim(mul([cosec_term[0], fn("cos", base), add([fn("cot", base), neg(fn("cot", cos_cot_term[2]))])]))
    lines = [
        "Use cosec(A)-sin(A) = cos(A)cot(A).",
        equation_line(add([mul([cosec_term[0], fn("cos", base), fn("cot", base)]), mul([cos_cot_term[0], fn("cos", base), fn("cot", cos_cot_term[2])])]), num(0)),
        "Factorise.",
        equation_line(factored, num(0)),
    ]
    return factored, lines


def solve_sin_tan_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sin", "sin", 1, False), ("tan", "tan", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sin"]) or is_zero(coeffs["tan"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["sin"], fn("sin", arg), fn("cos", arg)]), mul([coeffs["tan"], fn("sin", arg)])]))
    factored = sim(mul([fn("sin", arg), add([mul([coeffs["sin"], fn("cos", arg)]), coeffs["tan"]])]))
    lines = [
        "tan(" + show(arg) + ") is undefined when cos(" + show(arg) + ") = 0, so multiply through by cos(" + show(arg) + ").",
        equation_line(step1, num(0)),
        "Factorise.",
        equation_line(factored, num(0)),
    ]
    return factored, lines


def solve_cos_cot_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cos", "cos", 1, False), ("cot", "cot", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cos"]) or is_zero(coeffs["cot"]) or not is_zero(const):
        return None
    step1 = sim(add([mul([coeffs["cos"], fn("sin", arg), fn("cos", arg)]), mul([coeffs["cot"], fn("cos", arg)])]))
    factored = sim(mul([fn("cos", arg), add([mul([coeffs["cos"], fn("sin", arg)]), coeffs["cot"]])]))
    lines = [
        "cot(" + show(arg) + ") is undefined when sin(" + show(arg) + ") = 0, so multiply through by sin(" + show(arg) + ").",
        equation_line(step1, num(0)),
        "Factorise.",
        equation_line(factored, num(0)),
    ]
    return factored, lines


def solve_pythag_single_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sin2", "sin", 2, False), ("cos2", "cos", 2, False), ("sin", "sin", 1, False), ("cos", "cos", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if not is_zero(coeffs["cos2"]) and is_zero(coeffs["sin2"]) and not is_zero(coeffs["sin"]) and is_zero(coeffs["cos"]):
        step1 = sim(add([mul([coeffs["cos2"], add([num(1), neg(power(fn("sin", arg), num(2)))])]), mul([coeffs["sin"], fn("sin", arg)]), const]))
        new_expr = sim(rewrite_children(step1, expand_small))
        lines = [
            "Use cos^2(A) = 1-sin^2(A).",
            equation_line(step1, num(0)),
        ]
        if not same(new_expr, step1):
            lines.append("Simplify.")
            lines.append(equation_line(new_expr, num(0)))
        return new_expr, lines
    if not is_zero(coeffs["sin2"]) and is_zero(coeffs["cos2"]) and not is_zero(coeffs["cos"]) and is_zero(coeffs["sin"]):
        step1 = sim(add([mul([coeffs["sin2"], add([num(1), neg(power(fn("cos", arg), num(2)))])]), mul([coeffs["cos"], fn("cos", arg)]), const]))
        new_expr = sim(rewrite_children(step1, expand_small))
        lines = [
            "Use sin^2(A) = 1-cos^2(A).",
            equation_line(step1, num(0)),
        ]
        if not same(new_expr, step1):
            lines.append("Simplify.")
            lines.append(equation_line(new_expr, num(0)))
        return new_expr, lines
    return None


def solve_weighted_pythag_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sin2", "sin", 2, False), ("cos2", "cos", 2, False), ("sin", "sin", 1, False), ("cos", "cos", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sin2"]) or is_zero(coeffs["cos2"]):
        return None
    if not is_zero(coeffs["sin"]) and not is_zero(coeffs["cos"]):
        return None
    if is_zero(coeffs["sin"]) and is_zero(coeffs["cos"]):
        return None
    if not is_zero(coeffs["sin"]):
        step1 = sim(add([mul([coeffs["sin2"], power(fn("sin", arg), num(2))]), mul([coeffs["cos2"], add([num(1), neg(power(fn("sin", arg), num(2)))])]), mul([coeffs["sin"], fn("sin", arg)]), const]))
        new_expr = sim(expand_embedded_small(step1))
        lines = [
            "Use cos^2(A) = 1-sin^2(A).",
            equation_line(step1, num(0)),
        ]
        if not same(new_expr, step1):
            lines.append("Simplify.")
            lines.append(equation_line(new_expr, num(0)))
        return new_expr, lines
    step1 = sim(add([mul([coeffs["sin2"], add([num(1), neg(power(fn("cos", arg), num(2)))])]), mul([coeffs["cos2"], power(fn("cos", arg), num(2))]), mul([coeffs["cos"], fn("cos", arg)]), const]))
    new_expr = sim(expand_embedded_small(step1))
    lines = [
        "Use sin^2(A) = 1-cos^2(A).",
        equation_line(step1, num(0)),
    ]
    if not same(new_expr, step1):
        lines.append("Simplify.")
        lines.append(equation_line(new_expr, num(0)))
    return new_expr, lines


def solve_cos_double_single_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("cos2", "cos", 1, True), ("cos", "cos", 1, False), ("sin", "sin", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["cos2"]) or (is_zero(coeffs["cos"]) and is_zero(coeffs["sin"])):
        return None
    if not is_zero(coeffs["cos"]) and not is_zero(coeffs["sin"]):
        return None
    if not is_zero(coeffs["cos"]):
        new_expr = sim(add([mul([num(2), coeffs["cos2"], power(fn("cos", arg), num(2))]), mul([coeffs["cos"], fn("cos", arg)]), const, neg(coeffs["cos2"])]))
        lines = [
            "Use cos(2A) = 2cos^2(A)-1.",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    new_expr = sim(add([mul([num(-2), coeffs["cos2"], power(fn("sin", arg), num(2))]), mul([coeffs["sin"], fn("sin", arg)]), const, coeffs["cos2"]]))
    lines = [
        "Use cos(2A) = 1-2sin^2(A).",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def solve_sin_double_single_expr(expr, var):
    data = collect_same_arg_terms(expr, var, [("sin2", "sin", 1, True), ("cos", "cos", 1, False), ("sin", "sin", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sin2"]) or not is_zero(const):
        return None
    if not is_zero(coeffs["cos"]) and not is_zero(coeffs["sin"]):
        return None
    if not is_zero(coeffs["cos"]):
        step1 = sim(add([mul([num(2), coeffs["sin2"], fn("sin", arg), fn("cos", arg)]), mul([coeffs["cos"], fn("cos", arg)])]))
        factored = sim(mul([fn("cos", arg), add([mul([num(2), coeffs["sin2"], fn("sin", arg)]), coeffs["cos"]])]))
        lines = [
            "Use sin(2A) = 2sin(A)cos(A).",
            equation_line(step1, num(0)),
            "Factorise.",
            equation_line(factored, num(0)),
        ]
        return factored, lines
    if is_zero(coeffs["sin"]):
        return None
    step1 = sim(add([mul([num(2), coeffs["sin2"], fn("sin", arg), fn("cos", arg)]), mul([coeffs["sin"], fn("sin", arg)])]))
    factored = sim(mul([fn("sin", arg), add([mul([num(2), coeffs["sin2"], fn("cos", arg)]), coeffs["sin"]])]))
    lines = [
        "Use sin(2A) = 2sin(A)cos(A).",
        equation_line(step1, num(0)),
        "Factorise.",
        equation_line(factored, num(0)),
    ]
    return factored, lines


def match_signed_sin_cos_square(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    square = None
    const = num(0)
    i = 0
    while i < len(terms):
        cur = terms[i]
        if cur[0] == "pow" and cheap_same(cur[2], num(2)) and depends(cur[1], var):
            if square is not None:
                return None
            square = cur[1]
        else:
            if depends(cur, var):
                return None
            const = sim(add([const, cur]))
        i += 1
    if square is None:
        return None
    bits = list(flat(square, "add")) if square[0] == "add" else [square]
    if len(bits) != 2:
        return None
    arg = None
    sin_sign = None
    cos_sign = None
    i = 0
    while i < len(bits):
        coeff, rest = split_coeff(bits[i])
        if not (cheap_same(coeff, num(1)) or cheap_same(coeff, num(-1))):
            return None
        if rest[0] != "fn" or rest[1] not in ("sin", "cos") or not depends(rest[2], var):
            return None
        if arg is None:
            arg = rest[2]
        elif not cheap_same(arg, rest[2]):
            return None
        sign = 1 if cheap_same(coeff, num(1)) else -1
        if rest[1] == "sin":
            if sin_sign is not None:
                return None
            sin_sign = sign
        else:
            if cos_sign is not None:
                return None
            cos_sign = sign
        i += 1
    if sin_sign is None or cos_sign is None:
        return None
    return arg, sin_sign * cos_sign, const


def solve_square_sum_reduction_expr(expr, var):
    info = match_signed_sin_cos_square(expr, var)
    if info is None:
        return None
    arg, cross_sign, const = info
    step1 = sim(expand_embedded_small(expr))
    new_expr = sim(add([const, num(1), mul([num(2 * cross_sign), fn("sin", arg), fn("cos", arg)])]))
    if cheap_same(new_expr, expr):
        return None
    lines = [
        "Expand the square.",
        equation_line(step1, num(0)),
        "Use sin^2 A + cos^2 A = 1.",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def match_simple_trig_term(node, name):
    node = sim(node)
    return match_simple_trig_term_norm(node, name)


def match_simple_trig_term_norm(node, name):
    if node[0] == "fn" and node[1] == name:
        return num(1), node[2]
    if node[0] != "mul":
        return None
    items = list(flat(node, "mul"))
    i = 0
    pick = -1
    while i < len(items):
        if items[i][0] == "fn" and items[i][1] == name:
            pick = i
            break
        i += 1
    if pick == -1:
        return None
    return make_mul(items[:pick] + items[pick + 1 :]), items[pick][2]


def same_double_angle(arg, base):
    return equivalent(sim(arg), sim(mul([num(2), base])))


def same_double_angle_norm(arg, base):
    doubled = mul([num(2), base])
    return cheap_same(arg, doubled) or equivalent(arg, doubled)


def half_angle_expr(arg):
    half = sim(div(arg, num(2)))
    if equivalent(sim(arg), sim(mul([num(2), half]))):
        return half
    return None


def match_one_pm_cos(node):
    node = sim(node)
    return match_one_pm_cos_norm(node)


def match_one_pm_trig_norm(node, name):
    bits = list(flat(node, "add")) if node[0] == "add" else [node]
    if len(bits) != 2:
        return None
    saw_one = False
    sign = None
    arg = None
    i = 0
    while i < len(bits):
        cur = bits[i]
        if cheap_same(cur, num(1)):
            saw_one = True
        else:
            coeff, rest = split_coeff(cur)
            if rest[0] == "fn" and rest[1] == name and (is_one(coeff) or is_minus_one(coeff)):
                sign = 1 if is_one(coeff) else -1
                arg = rest[2]
            else:
                return None
        i += 1
    if not saw_one or arg is None:
        return None
    return sign, arg


def match_one_pm_cos_norm(node):
    return match_one_pm_trig_norm(node, "cos")


def half_angle_ratio_rewrite(node):
    part = match_scaled_div_norm(node)
    if part is None:
        return None
    coeff, top, bot = part
    top_outer, top_core = split_coeff(top)
    bot_outer, bot_core = split_coeff(bot)
    top_pm = match_one_pm_cos_norm(top_core)
    bot_pm = match_one_pm_cos_norm(bot_core)
    top_sin = match_simple_trig_term_norm(top_core, "sin")
    bot_sin = match_simple_trig_term_norm(bot_core, "sin")
    if top_pm is not None and bot_sin is not None and cheap_same(top_pm[1], bot_sin[1]):
        half = div(top_pm[1], num(2))
        total_coeff = sim(mul([coeff, top_outer, div(num(1), mul([bot_outer, bot_sin[0]]))]))
        if top_pm[0] < 0:
            return sim(mul([total_coeff, fn("tan", half)])), "Use half-angle ratio identities."
        return sim(mul([total_coeff, fn("cot", half)])), "Use half-angle ratio identities."
    if top_sin is not None and bot_pm is not None and cheap_same(top_sin[1], bot_pm[1]):
        half = div(top_sin[1], num(2))
        total_coeff = sim(mul([coeff, top_outer, top_sin[0], div(num(1), bot_outer)]))
        if bot_pm[0] > 0:
            return sim(mul([total_coeff, fn("tan", half)])), "Use half-angle ratio identities."
        return sim(mul([total_coeff, fn("cot", half)])), "Use half-angle ratio identities."
    return None


def rewrite_half_angle_ratio_terms(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    out = []
    changed = False
    i = 0
    while i < len(terms):
        cur = half_angle_ratio_rewrite(terms[i])
        if cur is not None and depends(terms[i], var):
            out.append(cur[0])
            changed = True
        else:
            out.append(terms[i])
        i += 1
    if not changed:
        return None
    new_expr = sim(add(out))
    lines = [
        "Use half-angle ratio identities.",
        equation_line(new_expr, num(0)),
    ]
    return new_expr, lines


def match_scaled_div(node):
    node = sim(node)
    return match_scaled_div_norm(node)


def match_scaled_div_norm(node):
    coeff, rest = split_coeff(node)
    if rest[0] == "div":
        return coeff, rest[1], rest[2]
    return None


def solve_sin2_tan_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    sin_term = None
    tan_term = None
    const = num(0)
    i = 0
    while i < len(terms):
        cur = terms[i]
        part = match_simple_trig_term_norm(cur, "sin")
        if part is not None and depends(part[1], var):
            if sin_term is not None:
                return None
            sin_term = part
            i += 1
            continue
        part = match_simple_trig_term_norm(cur, "tan")
        if part is not None and depends(part[1], var):
            if tan_term is not None:
                return None
            tan_term = part
            i += 1
            continue
        if depends(cur, var):
            return None
        const = sim(add([const, cur]))
        i += 1
    if sin_term is None or tan_term is None or not is_zero(const):
        return None
    if not same_double_angle_norm(sin_term[1], tan_term[1]):
        return None
    base = tan_term[1]
    step1 = sim(add([mul([sin_term[0], fn("sin", sin_term[1]), fn("cos", base)]), mul([tan_term[0], fn("sin", base)])]))
    step2 = sim(add([mul([num(2), sin_term[0], fn("sin", base), power(fn("cos", base), num(2))]), mul([tan_term[0], fn("sin", base)])]))
    factored = sim(mul([fn("sin", base), add([mul([num(2), sin_term[0], power(fn("cos", base), num(2))]), tan_term[0]])]))
    lines = [
        "tan(" + show(base) + ") is undefined when cos(" + show(base) + ") = 0, so multiply through by cos(" + show(base) + ").",
        equation_line(step1, num(0)),
        "Use sin(2A) = 2sin(A)cos(A).",
        equation_line(step2, num(0)),
        "Factorise.",
        equation_line(factored, num(0)),
    ]
    return factored, lines


def solve_sin2_cot_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    sin_term = None
    cot_term = None
    const = num(0)
    i = 0
    while i < len(terms):
        cur = terms[i]
        part = match_simple_trig_term_norm(cur, "sin")
        if part is not None and depends(part[1], var):
            if sin_term is not None:
                return None
            sin_term = part
            i += 1
            continue
        part = match_simple_trig_term_norm(cur, "cot")
        if part is not None and depends(part[1], var):
            if cot_term is not None:
                return None
            cot_term = part
            i += 1
            continue
        if depends(cur, var):
            return None
        const = sim(add([const, cur]))
        i += 1
    if sin_term is None or cot_term is None or not is_zero(const):
        return None
    if not same_double_angle_norm(sin_term[1], cot_term[1]):
        return None
    base = cot_term[1]
    step1 = sim(add([mul([sin_term[0], fn("sin", sin_term[1]), fn("sin", base)]), mul([cot_term[0], fn("cos", base)])]))
    step2 = sim(add([mul([num(2), sin_term[0], power(fn("sin", base), num(2)), fn("cos", base)]), mul([cot_term[0], fn("cos", base)])]))
    factored = sim(mul([fn("cos", base), add([mul([num(2), sin_term[0], power(fn("sin", base), num(2))]), cot_term[0]])]))
    lines = [
        "cot(" + show(base) + ") is undefined when sin(" + show(base) + ") = 0, so multiply through by sin(" + show(base) + ").",
        equation_line(step1, num(0)),
        "Use sin(2A) = 2sin(A)cos(A).",
        equation_line(step2, num(0)),
        "Factorise.",
        equation_line(factored, num(0)),
    ]
    return factored, lines


def rewrite_sum_product_terms(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) < 2:
        return None
    i = 0
    while i < len(terms):
        coeff_i, rest_i = split_coeff(terms[i])
        if rest_i[0] == "fn" and rest_i[1] in ("sin", "cos") and depends(rest_i[2], var) and not depends(coeff_i, var):
            j = i + 1
            while j < len(terms):
                coeff_j, rest_j = split_coeff(terms[j])
                if rest_j[0] == "fn" and rest_j[1] == rest_i[1] and depends(rest_j[2], var) and not depends(coeff_j, var):
                    pair = None
                    scale = None
                    if equivalent(coeff_i, coeff_j):
                        pair = add([rest_i, rest_j])
                        scale = coeff_i
                    elif equivalent(coeff_i, neg(coeff_j)):
                        pair = add([rest_i, neg(rest_j)])
                        scale = coeff_i
                    if pair is not None:
                        expanded, note = sum_product_expand(pair)
                        if expanded is not None:
                            new_terms = []
                            k = 0
                            while k < len(terms):
                                if k != i and k != j:
                                    new_terms.append(terms[k])
                                k += 1
                            new_terms.append(sim(mul([scale, expanded])))
                            new_expr = sim(add(new_terms))
                            lines = [
                                note,
                                equation_line(new_expr, num(0)),
                            ]
                            return new_expr, lines
                j += 1
        i += 1
    return None


def rewrite_product_to_sum_terms(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    i = 0
    while i < len(terms):
        expanded, note = sum_product_expand(terms[i])
        if expanded is not None and depends(terms[i], var):
            new_terms = []
            j = 0
            while j < len(terms):
                if j != i:
                    new_terms.append(terms[j])
                j += 1
            new_terms.append(expanded)
            new_expr = sim(add(new_terms))
            lines = [
                note,
                equation_line(new_expr, num(0)),
            ]
            return new_expr, lines
        i += 1
    return None


def multiply_by_base_square_expr(node, base, use_cos):
    node = sim(node)
    node_expanded = expand_embedded_small(node)
    if not cheap_same(node_expanded, node):
        return multiply_by_base_square_expr(node_expanded, base, use_cos)
    if node[0] == "add":
        terms = list(flat(node, "add"))
        out = []
        i = 0
        while i < len(terms):
            out.append(multiply_by_base_square_expr(terms[i], base, use_cos))
            i += 1
        return full_simplify(reduce_identities(add(out)))
    if use_cos:
        part = match_fn_power_term(node, "tan", 2)
        if part is not None and cheap_same(part[1], base):
            return full_simplify(mul([part[0], power(fn("sin", base), num(2))]))
        part = match_fn_power_term(node, "sec", 2)
        if part is not None and cheap_same(part[1], base):
            return full_simplify(part[0])
        square = power(fn("cos", base), num(2))
    else:
        part = match_fn_power_term(node, "cot", 2)
        if part is not None and cheap_same(part[1], base):
            return full_simplify(mul([part[0], power(fn("cos", base), num(2))]))
        part = match_fn_power_term(node, "cosec", 2)
        if part is not None and cheap_same(part[1], base):
            return full_simplify(part[0])
        square = power(fn("sin", base), num(2))
    return full_simplify(reduce_identities(mul([node, square])))


def rewrite_square_basis_expr(node, basis_name, base):
    node = sim(node)
    if node[0] == "pow" and same(node[2], num(2)) and node[1][0] == "fn" and cheap_same(node[1][2], base):
        if basis_name == "cos" and node[1][1] == "sin":
            return add([num(1), neg(power(fn("cos", base), num(2)))])
        if basis_name == "sin" and node[1][1] == "cos":
            return add([num(1), neg(power(fn("sin", base), num(2)))])
    if node[0] == "fn":
        return fn(node[1], rewrite_square_basis_expr(node[2], basis_name, base))
    if node[0] == "pow":
        return power(rewrite_square_basis_expr(node[1], basis_name, base), rewrite_square_basis_expr(node[2], basis_name, base))
    if node[0] == "div":
        return div(rewrite_square_basis_expr(node[1], basis_name, base), rewrite_square_basis_expr(node[2], basis_name, base))
    if node[0] in ("num", "sym", "const"):
        return node
    out = []
    i = 0
    while i < len(node[1]):
        out.append(rewrite_square_basis_expr(node[1][i], basis_name, base))
        i += 1
    return sim((node[0], tuple(out)))


def rewrite_tan_double_factor_polynomial_expr(expr, var, basis_name=None):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    tan_piece = None
    sin2_piece = None
    const = num(0)
    i = 0
    while i < len(terms):
        tan_match = match_fn_power_term(terms[i], "tan", 1)
        if tan_match is not None and depends(tan_match[1], var):
            if tan_piece is not None:
                return None
            tan_piece = tan_match
            i += 1
            continue
        sin2_match = match_double_fn_term(terms[i], "sin")
        if sin2_match is not None and depends(sin2_match[1], var):
            if sin2_piece is not None:
                return None
            sin2_piece = sin2_match
            i += 1
            continue
        if depends(terms[i], var):
            return None
        const = sim(add([const, terms[i]]))
        i += 1
    if tan_piece is None or sin2_piece is None or not is_zero(const):
        return None
    if not cheap_same(tan_piece[1], sin2_piece[1]):
        return None
    base = tan_piece[1]
    tan_rest = full_simplify(tan_piece[0])
    if reciprocal_burden(tan_rest) != 0:
        return None
    inner_from_tan = full_simplify(div(tan_rest, num(2)))
    inner_from_sin2 = multiply_by_base_square_expr(sin2_piece[0], base, True)
    inner = full_simplify(reduce_identities(add([inner_from_tan, inner_from_sin2])))
    outer = fn("sin", mul([num(2), base]))
    new_expr = sim(mul([outer, inner]))
    lines = [
        "tan(" + show(base) + ") is undefined when cos(" + show(base) + ") = 0, so multiply through by cos(" + show(base) + ")^2.",
        equation_line(new_expr, num(0)),
    ]
    if basis_name is None:
        basis_name = "cos"
    if basis_name in ("cos", "sin"):
        inner_basis = full_simplify(reduce_identities(expand_embedded_small(rewrite_square_basis_expr(inner, basis_name, base))))
        if not cheap_same(inner_basis, inner):
            new_expr = sim(mul([outer, inner_basis]))
            if basis_name == "cos":
                lines.append("Use sin^2(A) = 1-cos^2(A).")
            else:
                lines.append("Use cos^2(A) = 1-sin^2(A).")
            lines.append(equation_line(new_expr, num(0)))
    return new_expr, compact_lines(lines)


def match_double_single_product_term(node, var):
    items = list(flat(node, "mul")) if node[0] == "mul" else [node]
    coeff_bits = []
    single = None
    doubled = None
    i = 0
    while i < len(items):
        item = items[i]
        if item[0] == "fn" and item[1] in ("sin", "cos") and depends(item[2], var):
            if doubled is None:
                base = match_double_angle_arg(item[2])
                if base is not None:
                    doubled = (item[1], base)
                    i += 1
                    continue
            if single is None:
                single = (item[1], item[2])
                i += 1
                continue
            return None
        coeff_bits.append(item)
        i += 1
    if single is None or doubled is None:
        return None
    if not cheap_same(doubled[1], single[1]):
        return None
    coeff = make_mul(coeff_bits)
    if depends(coeff, var):
        return None
    return coeff, doubled[0], single[0], single[1]


def solve_double_product_reduction_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) < 2:
        return None
    product_pick = -1
    product = None
    const = num(0)
    i = 0
    while i < len(terms):
        match = match_double_single_product_term(terms[i], var)
        if match is not None:
            if product is not None:
                return None
            product = match
            product_pick = i
        else:
            if depends(terms[i], var):
                return None
            const = sim(add([const, terms[i]]))
        i += 1
    if product is None:
        return None
    coeff, double_name, single_name, base = product
    atom_name = "sin"
    step1 = None
    new_expr = None
    if double_name == "sin" and single_name == "cos":
        step1 = sim(add([mul([num(2), coeff, fn("sin", base), power(fn("cos", base), num(2))]), const]))
        new_expr = sim(add([mul([num(-2), coeff, power(fn("sin", base), num(3))]), mul([num(2), coeff, fn("sin", base)]), const]))
        atom_name = "sin"
        lines = [
            "Use sin(2A) = 2sin(A)cos(A).",
            equation_line(step1, num(0)),
            "Use cos^2(A) = 1-sin^2(A).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    if double_name == "sin" and single_name == "sin":
        step1 = sim(add([mul([num(2), coeff, power(fn("sin", base), num(2)), fn("cos", base)]), const]))
        new_expr = sim(add([mul([num(-2), coeff, power(fn("cos", base), num(3))]), mul([num(2), coeff, fn("cos", base)]), const]))
        atom_name = "cos"
        lines = [
            "Use sin(2A) = 2sin(A)cos(A).",
            equation_line(step1, num(0)),
            "Use sin^2(A) = 1-cos^2(A).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    if double_name == "cos" and single_name == "cos":
        new_expr = sim(add([mul([num(2), coeff, power(fn("cos", base), num(3))]), mul([neg(coeff), fn("cos", base)]), const]))
        lines = [
            "Use cos(2A) = 2cos^2(A)-1.",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    if double_name == "cos" and single_name == "sin":
        new_expr = sim(add([mul([num(-2), coeff, power(fn("sin", base), num(3))]), mul([coeff, fn("sin", base)]), const]))
        lines = [
            "Use cos(2A) = 1-2sin^2(A).",
            equation_line(new_expr, num(0)),
        ]
        return new_expr, lines
    _ = atom_name
    _ = product_pick
    return None


def solve_same_angle_ratio_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    data = collect_same_arg_terms(expr, var, [("sin", "sin", 1, False), ("cos", "cos", 1, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    if is_zero(coeffs["sin"]) or is_zero(coeffs["cos"]) or not is_zero(const):
        return None
    angle = match_linear_angle(arg, var)
    if angle is None:
        return None
    target_expr = full_simplify(neg(div(coeffs["cos"], coeffs["sin"])))
    target_value = constant_numeric(target_expr, deg_mode)
    if target_value is None:
        return None
    sols, angles = solve_angle_value("tan", angle[0], angle[1], target_value, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    lines = [
        "cos(" + show(arg) + ") = 0 does not work, so divide by cos(" + show(arg) + ").",
        equation_line(add([mul([coeffs["sin"], fn("tan", arg)]), coeffs["cos"]]), num(0)),
        "Rearrange.",
        equation_line(fn("tan", arg), target_expr),
    ]
    if len(angles) != 0:
        bits = []
        i = 0
        while i < len(angles):
            bits.append(angle_text(angles[i], deg_mode))
            i += 1
        lines.append(show(arg) + " = [" + ", ".join(bits) + "]")
    if len(sols) == 0:
        lines.append("No solutions in the interval.")
    else:
        bits = []
        i = 0
        while i < len(sols):
            bits.append(final_angle_text(sols[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(bits) + "]")
    return sols, compact_lines(lines)


def solve_compound_ratio_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    expanded_sym = normalise_constant_trig_args(sim(expand_embedded_small(expand_trig_tree(expr))))
    if same(expanded_sym, expr):
        return None
    combo_sym = match_linear_combo_const(expanded_sym, var)
    if combo_sym is None:
        return None
    base_sym, sin_coeff_sym, cos_coeff_sym, const_sym = combo_sym
    if is_zero(sin_coeff_sym) or is_zero(cos_coeff_sym) or not is_zero(const_sym):
        return None
    expanded_exact = normalise_constant_trig_args(replace_exact_trig_quiet(expanded_sym))
    expanded_exact = sim(expand_embedded_small(expanded_exact))
    combo_exact = match_linear_combo_const(expanded_exact, var)
    if combo_exact is None:
        return None
    base, sin_coeff, cos_coeff, const_expr = combo_exact
    if not same(base, base_sym):
        return None
    if is_zero(sin_coeff) or is_zero(cos_coeff) or not is_zero(const_expr):
        return None
    angle = match_linear_angle(base, var)
    if angle is None:
        return None
    target_top_sym = full_simplify(cos_coeff_sym)
    target_bot_sym = full_simplify(neg(sin_coeff_sym))
    target_top_exact = full_simplify(cos_coeff)
    target_bot_exact = full_simplify(neg(sin_coeff))
    if display_neg(target_top_sym) and display_neg(target_bot_sym):
        target_top_sym = display_abs(target_top_sym)
        target_bot_sym = display_abs(target_bot_sym)
    if display_neg(target_top_exact) and display_neg(target_bot_exact):
        target_top_exact = display_abs(target_top_exact)
        target_bot_exact = display_abs(target_bot_exact)
    target_expr_sym = normalise_constant_trig_args(full_simplify(div(target_top_sym, target_bot_sym)))
    target_expr_exact = normalise_constant_trig_args(full_simplify(div(target_top_exact, target_bot_exact)))
    target_value = constant_numeric(target_expr_exact, deg_mode)
    if target_value is None:
        return None
    sols, angles = solve_angle_value("tan", angle[0], angle[1], target_value, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    left_expr = normalise_constant_trig_args(mul([full_simplify(cos_coeff_sym), fn("cos", base)]))
    right_expr = normalise_constant_trig_args(mul([full_simplify(neg(sin_coeff_sym)), fn("sin", base)]))
    base_text = show(base)
    lines = [
        "Expand the trig identities.",
        equation_line(expanded_sym, num(0)),
        "Collect terms.",
        equation_line(left_expr, right_expr),
        "Use tan(" + base_text + ") = sin(" + base_text + ")/cos(" + base_text + ").",
        "tan(" + base_text + ") = " + show(target_expr_sym),
    ]
    if not same(target_expr_exact, target_expr_sym):
        lines.append("= " + show(target_expr_exact))
    if len(angles) != 0:
        bits = []
        i = 0
        while i < len(angles):
            bits.append(angle_text(angles[i], deg_mode))
            i += 1
        lines.append(base_text + " = [" + ", ".join(bits) + "]" + (" rad" if not deg_mode else ""))
    if len(sols) == 0:
        lines.append("No solutions in the interval.")
    else:
        bits = []
        i = 0
        while i < len(sols):
            bits.append(final_angle_text(sols[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(bits) + "]")
    return sols, compact_lines(lines)


def append_unique_solve_value(values, value):
    i = 0
    while i < len(values):
        if abs(values[i] - value) < 1e-7:
            return
        i += 1
    values.append(value)


def match_equal_simple_trig_term(node):
    if node[0] == "fn" and node[1] in ("sin", "cos", "tan", "cot"):
        return num(1), node[1], node[2]
    coeff, rest = split_coeff(node)
    if rest[0] == "fn" and rest[1] in ("sin", "cos", "tan", "cot"):
        return coeff, rest[1], rest[2]
    return None


def solve_equal_same_trig_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) != 2:
        return None
    first = match_equal_simple_trig_term(terms[0])
    second = match_equal_simple_trig_term(terms[1])
    if first is None or second is None or first[1] != second[1]:
        return None
    if not equivalent(first[0], neg(second[0])):
        return None
    pair1 = linear_pair(first[2], var)
    pair2 = linear_pair(second[2], var)
    if pair1 is None or pair2 is None:
        return None

    def append_linear_solution(target_expr, sols):
        linear = linear_pair(target_expr, var)
        if linear is not None and not is_zero(linear[0]):
            value = eval_numeric_mode(sim(div(neg(linear[1]), linear[0])), {}, deg_mode)
            if within_interval(value, start_val, end_val, start_inclusive, end_inclusive):
                append_unique_solve_value(sols, value)

    def append_periodic_solutions(base_target, shift, sols):
        linear = linear_pair(base_target, var)
        if linear is None:
            return
        coeff = eval_numeric_mode(linear[0], {}, deg_mode)
        const = eval_numeric_mode(linear[1], {}, deg_mode)
        n_range = solve_n_range_for_interval(coeff, const, 0.0, shift, start_val, end_val)
        if n_range is None:
            return
        n = n_range[0]
        while n <= n_range[1]:
            append_linear_solution(sim(add([base_target, neg(number_node(n * shift))])), sols)
            n += 1

    name = first[1]
    full_turn = 360.0 if deg_mode else 2.0 * math.pi
    half_turn = 180.0 if deg_mode else math.pi
    sols = []
    if name in ("tan", "cot"):
        append_periodic_solutions(sim(add([first[2], neg(second[2])])), half_turn, sols)
        lines = [
            "For " + name + "(A) = " + name + "(B), A = B + n*pi.",
        ]
    elif name == "sin":
        append_periodic_solutions(sim(add([first[2], neg(second[2])])), full_turn, sols)
        append_periodic_solutions(sim(add([first[2], second[2], neg(number_node(180.0 if deg_mode else math.pi))])), full_turn, sols)
        lines = [
            "For sin(A) = sin(B), A = B + 2*n*pi or A = pi-B + 2*n*pi.",
        ]
    else:
        append_periodic_solutions(sim(add([first[2], neg(second[2])])), full_turn, sols)
        append_periodic_solutions(sim(add([first[2], second[2]])), full_turn, sols)
        lines = [
            "For cos(A) = cos(B), A = B + 2*n*pi or A = -B + 2*n*pi.",
        ]
    sols = dedupe_values(sols)
    if len(sols) == 0:
        lines.append("No solutions in the interval.")
    else:
        bits = []
        i = 0
        while i < len(sols):
            bits.append(final_angle_text(sols[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(bits) + "]")
    return sols, compact_lines(lines)


def solve_equal_complementary_trig_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) != 2:
        return None
    first = match_equal_simple_trig_term(terms[0])
    second = match_equal_simple_trig_term(terms[1])
    if first is None or second is None:
        return None
    if not equivalent(first[0], neg(second[0])):
        return None
    if set([first[1], second[1]]) != set(["sin", "cos"]):
        return None
    sin_arg = first[2] if first[1] == "sin" else second[2]
    cos_arg = first[2] if first[1] == "cos" else second[2]
    sin_pair = linear_pair(sin_arg, var)
    cos_pair = linear_pair(cos_arg, var)
    if sin_pair is None or cos_pair is None:
        return None

    def append_periodic_solutions(base_target, start_shift, sols):
        linear = linear_pair(base_target, var)
        if linear is None:
            return
        coeff = eval_numeric_mode(linear[0], {}, deg_mode)
        const = eval_numeric_mode(linear[1], {}, deg_mode)
        n_range = solve_n_range_for_interval(coeff, const, start_shift, full_turn, start_val, end_val)
        if n_range is None:
            return
        n = n_range[0]
        while n <= n_range[1]:
            target = sim(add([base_target, neg(number_node(start_shift + n * full_turn))]))
            linear2 = linear_pair(target, var)
            if linear2 is not None and not is_zero(linear2[0]):
                value = eval_numeric_mode(sim(div(neg(linear2[1]), linear2[0])), {}, deg_mode)
                if within_interval(value, start_val, end_val, start_inclusive, end_inclusive):
                    append_unique_solve_value(sols, value)
            n += 1

    quarter_turn = 90.0 if deg_mode else math.pi / 2.0
    full_turn = 360.0 if deg_mode else 2.0 * math.pi
    sols = []
    append_periodic_solutions(sim(add([sin_arg, cos_arg])), quarter_turn, sols)
    append_periodic_solutions(sim(add([sin_arg, neg(cos_arg)])), quarter_turn, sols)
    sols = dedupe_values(sols)
    quarter_text = "90" if deg_mode else "pi/2"
    full_text = "360" if deg_mode else "2*pi"
    lines = [
        "For sin(A) = cos(B), A+B = " + quarter_text + "+" + full_text + "*n or A-B = " + quarter_text + "+" + full_text + "*n.",
    ]
    if len(sols) == 0:
        lines.append("No solutions in the interval.")
    else:
        bits = []
        i = 0
        while i < len(sols):
            bits.append(final_angle_text(sols[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(bits) + "]")
    return sols, compact_lines(lines)


def solve_equal_tan_cot_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    if len(terms) != 2:
        return None
    left_tan = match_simple_trig_term_norm(terms[0], "tan")
    right_tan = match_simple_trig_term_norm(terms[1], "tan")
    left_cot = match_simple_trig_term_norm(terms[0], "cot")
    right_cot = match_simple_trig_term_norm(terms[1], "cot")
    name = None
    first = None
    second = None
    if left_tan is not None and right_tan is not None:
        name = "tan"
        first = left_tan
        second = right_tan
    elif left_cot is not None and right_cot is not None:
        name = "cot"
        first = left_cot
        second = right_cot
    else:
        return None
    if not equivalent(first[0], neg(second[0])):
        return None
    pair1 = linear_pair(first[1], var)
    pair2 = linear_pair(second[1], var)
    if pair1 is None or pair2 is None:
        return None
    period = math.pi
    sols = []
    linear = linear_pair(sim(add([first[1], neg(second[1])])), var)
    if linear is not None:
        coeff = eval_numeric_mode(linear[0], {}, deg_mode)
        const = eval_numeric_mode(linear[1], {}, deg_mode)
        n_range = solve_n_range_for_interval(coeff, const, 0.0, period, start_val, end_val)
        if n_range is not None:
            n = n_range[0]
            while n <= n_range[1]:
                target = sim(add([first[1], neg(second[1]), neg(number_node(n * period))]))
                linear2 = linear_pair(target, var)
                if linear2 is not None and not is_zero(linear2[0]):
                    value = eval_numeric_mode(sim(div(neg(linear2[1]), linear2[0])), {}, deg_mode)
                    if within_interval(value, start_val, end_val, start_inclusive, end_inclusive):
                        append_unique_solve_value(sols, value)
                n += 1
    sols = dedupe_values(sols)
    lines = [
        "For " + name + "(A) = " + name + "(B), A = B + n*pi.",
    ]
    if len(sols) == 0:
        lines.append("No solutions in the interval.")
    else:
        bits = []
        i = 0
        while i < len(sols):
            bits.append(final_angle_text(sols[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(bits) + "]")
    return sols, compact_lines(lines)


def solve_tan_substitution_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    base = tan_sub_base(expr, var)
    if base is None or is_zero(base):
        return None
    u_expr = tan_substitute_node(expr, base, var, "u")
    if u_expr is None:
        return None
    den = add([num(1), power(sym("u"), num(2))])
    power_count = 0
    poly_expr = u_expr
    poly_coeffs = extract_polynomial_symbol(poly_expr, "u", 6)
    poly_degree = polynomial_degree_coeff_map(poly_coeffs, 6) if poly_coeffs is not None else 0
    coeff_list = polynomial_coeff_nodes_low(poly_coeffs, poly_degree) if poly_coeffs is not None else None
    exact_root_info = solve_polynomial_exact_low_degree(coeff_list, poly_degree) if coeff_list is not None else None
    roots = exact_root_info["values"] if exact_root_info is not None else (solve_polynomial_numeric(coeff_list) if coeff_list is not None else None)
    while (poly_coeffs is None or roots is None) and power_count < 4:
        power_count += 1
        poly_expr = sim(expand_embedded_small(mul([u_expr, power(den, num(power_count))])))
        poly_coeffs = extract_polynomial_symbol(poly_expr, "u", 6)
        poly_degree = polynomial_degree_coeff_map(poly_coeffs, 6) if poly_coeffs is not None else 0
        coeff_list = polynomial_coeff_nodes_low(poly_coeffs, poly_degree) if poly_coeffs is not None else None
        exact_root_info = solve_polynomial_exact_low_degree(coeff_list, poly_degree) if coeff_list is not None else None
        roots = exact_root_info["values"] if exact_root_info is not None else (solve_polynomial_numeric(coeff_list) if coeff_list is not None else None)
    if poly_coeffs is None:
        return None
    if roots is None:
        return None
    root_nodes = exact_root_info["nodes"] if exact_root_info is not None else None
    poly_note = exact_root_info["note"] if exact_root_info is not None else None
    lines = [
        "Let u = tan(" + show(base) + ").",
        "Use tan-form double-angle identities.",
    ]
    if power_count != 0:
        lines.append("Multiply through by (1+u^2)^" + str(power_count) + ".")
    poly_terms = []
    degree = 6
    while degree >= 0:
        if not is_zero(poly_coeffs[degree]):
            if degree == 0:
                poly_terms.append(poly_coeffs[degree])
            elif degree == 1:
                poly_terms.append(mul([poly_coeffs[degree], sym("u")]))
            else:
                poly_terms.append(mul([poly_coeffs[degree], power(sym("u"), num(degree))]))
        degree -= 1
    lines.append(ordered_sum_text(poly_terms) + " = 0")
    if poly_note is not None:
        lines.append(poly_note)
    if poly_degree == 0 or len(roots) == 0:
        lines.append("No solutions in the interval.")
        return [], compact_lines(lines)
    root_bits = []
    i = 0
    while i < len(roots):
        root_bits.append(concise_root_text(root_nodes[i] if root_nodes is not None else None, roots[i], 6))
        i += 1
    lines.append("u = [" + ", ".join(root_bits) + "]")
    angle = match_linear_angle(base, var)
    if angle is None:
        return None
    all_solutions = []
    i = 0
    while i < len(roots):
        lines.append("tan(" + show(base) + ") = " + concise_root_text(root_nodes[i] if root_nodes is not None else None, roots[i], 6))
        sols, angles = solve_angle_value("tan", angle[0], angle[1], roots[i], start_val, end_val, deg_mode, start_inclusive, end_inclusive)
        if len(angles) != 0:
            bits = []
            j = 0
            while j < len(angles):
                bits.append(angle_text(angles[j], deg_mode))
                j += 1
            lines.append(show(base) + " = [" + ", ".join(bits) + "]" + (" rad" if not deg_mode else ""))
        j = 0
        while j < len(sols):
            all_solutions.append(sols[j])
            j += 1
        i += 1
    uniq = dedupe_values(all_solutions)
    if len(uniq) == 0:
        lines.append("No solutions in the interval.")
        return [], compact_lines(lines)
    bits = []
    i = 0
    while i < len(uniq):
        bits.append(final_angle_text(uniq[i], deg_mode))
        i += 1
    lines.append(var + " = [" + ", ".join(bits) + "]")
    return uniq, compact_lines(lines)


def solve_direct_double_angle_expr(expr, var):
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    i = 0
    while i < len(terms):
        rewritten, note = direct_double_angle_rewrite(terms[i])
        if rewritten is not None and not cheap_same(rewritten, terms[i]):
            new_expr = sim(make_add(terms[:i] + [rewritten] + terms[i + 1 :]))
            if not cheap_same(new_expr, expr):
                return new_expr, [note, equation_line(new_expr, num(0))]
        i += 1
    return None


def solve_linear_combo_shift(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    direct = sim(expr)
    combo = match_linear_combo_const(direct, var)
    expanded = direct
    expanded_note = False
    if combo is None:
        expanded = replace_exact_trig_quiet(sim(expand_trig_tree(expr)))
        expanded = sim(expand_embedded_small(expanded))
        combo = match_linear_combo_const(expanded, var)
        if combo is None:
            return None
        expanded_note = not same(expanded, expr)
    base, sin_coeff_expr, cos_coeff_expr, const_expr = combo
    angle = match_linear_angle(base, var)
    if angle is None:
        return None
    sin_coeff = constant_numeric(sin_coeff_expr, deg_mode)
    cos_coeff = constant_numeric(cos_coeff_expr, deg_mode)
    const_val = constant_numeric(const_expr, deg_mode)
    if sin_coeff is None or cos_coeff is None or const_val is None:
        return None
    r_val = math.sqrt(sin_coeff * sin_coeff + cos_coeff * cos_coeff)
    if r_val < 1e-12:
        return None
    alpha = to_degrees(math.atan2(cos_coeff, sin_coeff)) if deg_mode else math.atan2(cos_coeff, sin_coeff)
    target = -const_val / r_val
    if target < -1.0000001 or target > 1.0000001:
        return [], ["No solutions because the shifted trig value lies outside [-1,1]."]
    mult_val = constant_numeric(angle[0], deg_mode)
    offset_val = constant_numeric(angle[1], deg_mode)
    if mult_val is None or offset_val is None:
        return None
    sols, angles = solve_angle_value("sin", number_node(mult_val), number_node(offset_val + alpha), target, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    r_expr = full_simplify(fn("sqrt", add([power(sin_coeff_expr, num(2)), power(cos_coeff_expr, num(2))])))
    tan_expr = full_simplify(div(cos_coeff_expr, sin_coeff_expr)) if abs(sin_coeff) > 1e-12 else None
    target_expr = full_simplify(div(neg(const_expr), r_expr)) if not is_zero(const_expr) else num(0)
    lines = []
    if expanded_note:
        lines.append("Expand the trig identities.")
        lines.append(equation_line(expanded, num(0)))
    lines.append("Write the equation in the form A*sin(" + show(base) + ")+B*cos(" + show(base) + ")+C = 0.")
    lines.append("A = " + show(full_simplify(sin_coeff_expr)) + ", B = " + show(full_simplify(cos_coeff_expr)) + ", C = " + show(full_simplify(const_expr)))
    lines.append("Let A*sin(" + show(base) + ")+B*cos(" + show(base) + ") = R*sin(" + show(base) + "+a).")
    lines.append("R = sqrt(A^2+B^2)")
    lines.append("= " + show(r_expr))
    if tan_expr is not None:
        lines.append("tan(a) = B/A = " + show(tan_expr))
    lines.append("a = " + angle_text(alpha, deg_mode))
    shift_text = signed_angle_text(alpha, deg_mode)
    lines.append("sin(" + show(base) + shift_text + ") = " + show(full_simplify(target_expr)))
    if len(angles) != 0:
        bits = []
        i = 0
        while i < len(angles):
            bits.append(angle_text(angles[i], deg_mode))
            i += 1
        lines.append(show(base) + shift_text + " = [" + ", ".join(bits) + "]")
    if len(sols) == 0:
        lines.append("No solutions in the interval.")
    else:
        bits = []
        i = 0
        while i < len(sols):
            bits.append(final_angle_text(sols[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(bits) + "]")
    return sols, compact_lines(lines)


def solve_factor_equation_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    quad = extract_quadratic_trig(expr, var)
    if quad is not None:
        name, arg, mult, offset, a, b, c = quad
        roots = quadratic_roots(a, b, c, deg_mode)
        lines = []
        if len(roots) == 0:
            lines.append("No real solutions.")
            return [], compact_lines(lines)
        u_name = "u"
        atom = fn(name, arg)
        if not is_zero(a):
            lines.append("Let " + u_name + " = " + show(atom) + ".")
            lines.append(ordered_sum_text([mul([a, sym(u_name), sym(u_name)]), mul([b, sym(u_name)]), c]).replace(u_name + "*" + u_name, u_name + "^2") + " = 0")
            root_bits = []
            i = 0
            while i < len(roots):
                root_bits.append(exact_text_or_float(roots[i], 6))
                i += 1
            lines.append(u_name + " = [" + ", ".join(root_bits) + "]")
        else:
            lines.append("Solve the direct trig equation.")
        values = []
        i = 0
        while i < len(roots):
            val = roots[i]
            keep = True
            if name in ("sin", "cos") and (val < -1.0000001 or val > 1.0000001):
                keep = False
            if name in ("sec", "cosec") and abs(val) < 1.0 - 1e-9:
                keep = False
            if keep:
                values.append(val)
            elif not is_zero(a):
                if name in ("sin", "cos"):
                    lines.append("Reject " + u_name + " = " + exact_text_or_float(val, 6) + " because valid sin and cos values satisfy |" + u_name + "| <= 1.")
                else:
                    lines.append("Reject " + u_name + " = " + exact_text_or_float(val, 6) + " because " + name + " values have magnitude at least 1.")
            i += 1
        if len(values) == 0:
            lines.append("No valid trig values.")
            return [], compact_lines(lines)
        all_solutions = []
        i = 0
        while i < len(values):
            value_text = exact_text_or_float(values[i], 6)
            lines.append(show(atom) + " = " + value_text)
            sols, angles = solve_angle_value(name, mult, offset, values[i], start_val, end_val, deg_mode, start_inclusive, end_inclusive)
            if len(angles) != 0:
                bits = []
                j = 0
                while j < len(angles):
                    bits.append(angle_text(angles[j], deg_mode))
                    j += 1
                left = show(arg)
                unit = "" if deg_mode else " rad"
                lines.append(left + " = [" + ", ".join(bits) + "]" + unit)
            j = 0
            while j < len(sols):
                all_solutions.append(sols[j])
                j += 1
            i += 1
        uniq = dedupe_values(all_solutions)
        if len(uniq) == 0:
            lines.append("No solutions in the interval.")
            return [], compact_lines(lines)
        sol_text = solution_list_text(uniq, deg_mode)
        lines.append("Combined solutions: " + var + " = [" + ", ".join(sol_text) + "]")
        return uniq, compact_lines(lines)
    poly = extract_polynomial_trig(expr, var, 4)
    if poly is not None:
        name, arg, mult, offset, coeffs = poly
        poly_degree = polynomial_degree_coeff_map(coeffs, 4)
        coeff_list = polynomial_coeff_nodes_low(coeffs, poly_degree)
        exact_root_info = solve_polynomial_exact_low_degree(coeff_list, poly_degree)
        root_nodes = None
        poly_note = None
        if exact_root_info is not None:
            roots = exact_root_info["values"]
            root_nodes = exact_root_info["nodes"]
            poly_note = exact_root_info["note"]
        else:
            roots = solve_polynomial_numeric(coeff_list)
        numeric_fallback = False
        if roots is None and name in ("sin", "cos"):
            roots = solve_polynomial_numeric_interval(coeffs, poly_degree, -1.0, 1.0)
            numeric_fallback = True
        if roots is None and name in ("tan", "cot"):
            bound = polynomial_real_root_bound(coeffs, poly_degree)
            roots = solve_polynomial_numeric_interval(coeffs, poly_degree, -bound, bound)
            numeric_fallback = True
        if roots is not None and len(roots) == 0:
            atom = fn(name, arg)
            lines = ["Let u = " + show(atom) + "."]
            poly_terms = []
            degree = poly_degree
            while degree >= 0:
                if not is_zero(coeffs[degree]):
                    if degree == 0:
                        poly_terms.append(coeffs[degree])
                    elif degree == 1:
                        poly_terms.append(mul([coeffs[degree], sym("u")]))
                    else:
                        poly_terms.append(mul([coeffs[degree], power(sym("u"), num(degree))]))
                degree -= 1
            lines.append(ordered_sum_text(poly_terms) + " = 0")
            if poly_note is not None:
                lines.append(poly_note)
            if numeric_fallback:
                lines.append("Solve the polynomial in u numerically.")
            lines.append("No valid trig values.")
            return [], compact_lines(lines)
        if roots is not None and len(roots) != 0:
            atom = fn(name, arg)
            lines = ["Let u = " + show(atom) + "."]
            poly_terms = []
            degree = poly_degree
            while degree >= 0:
                if not is_zero(coeffs[degree]):
                    if degree == 0:
                        poly_terms.append(coeffs[degree])
                    elif degree == 1:
                        poly_terms.append(mul([coeffs[degree], sym("u")]))
                    else:
                        poly_terms.append(mul([coeffs[degree], power(sym("u"), num(degree))]))
                degree -= 1
            lines.append(ordered_sum_text(poly_terms) + " = 0")
            if poly_note is not None:
                lines.append(poly_note)
            if numeric_fallback:
                lines.append("Solve the polynomial in u numerically.")
            root_bits = []
            i = 0
            while i < len(roots):
                root_bits.append(concise_root_text(root_nodes[i] if root_nodes is not None else None, roots[i], 6))
                i += 1
            lines.append("u = [" + ", ".join(root_bits) + "]")
            values = []
            value_nodes = []
            i = 0
            while i < len(roots):
                val = roots[i]
                keep = True
                if name in ("sin", "cos") and (val < -1.0000001 or val > 1.0000001):
                    keep = False
                if name in ("sec", "cosec") and abs(val) < 1.0 - 1e-9:
                    keep = False
                if keep:
                    values.append(val)
                    if root_nodes is not None:
                        value_nodes.append(root_nodes[i])
                else:
                    if root_nodes is not None:
                        lines.append("Reject u = " + concise_root_text(root_nodes[i], val, 6) + " because it is not a valid " + name + " value.")
                    else:
                        lines.append("Reject u = " + exact_text_or_float(val, 6) + " because it is not a valid " + name + " value.")
                i += 1
            if len(values) == 0:
                lines.append("No valid trig values.")
                return [], compact_lines(lines)
            all_solutions = []
            i = 0
            while i < len(values):
                if root_nodes is not None:
                    lines.append(show(atom) + " = " + concise_root_text(value_nodes[i], values[i], 6))
                else:
                    lines.append(show(atom) + " = " + exact_text_or_float(values[i], 6))
                sols, angles = solve_angle_value(name, mult, offset, values[i], start_val, end_val, deg_mode, start_inclusive, end_inclusive)
                if len(angles) != 0:
                    bits = []
                    j = 0
                    while j < len(angles):
                        bits.append(angle_text(angles[j], deg_mode))
                        j += 1
                    lines.append(show(arg) + " = [" + ", ".join(bits) + "]" + (" rad" if not deg_mode else ""))
                j = 0
                while j < len(sols):
                    all_solutions.append(sols[j])
                    j += 1
                i += 1
            uniq = dedupe_values(all_solutions)
            if len(uniq) == 0:
                lines.append("No solutions in the interval.")
                return [], compact_lines(lines)
            bits = []
            i = 0
            while i < len(uniq):
                bits.append(final_angle_text(uniq[i], deg_mode))
                i += 1
            lines.append(var + " = [" + ", ".join(bits) + "]")
            return uniq, compact_lines(lines)
    tan_sub_result = solve_tan_substitution_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if tan_sub_result is not None:
        return tan_sub_result
    ratio_result = solve_same_angle_ratio_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if ratio_result is not None:
        return ratio_result
    compound_ratio_result = solve_compound_ratio_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if compound_ratio_result is not None:
        return compound_ratio_result
    equality_result = solve_equal_same_trig_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if equality_result is not None:
        return equality_result
    equality_result = solve_equal_complementary_trig_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if equality_result is not None:
        return equality_result
    equality_result = solve_equal_tan_cot_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if equality_result is not None:
        return equality_result
    shift_result = solve_linear_combo_shift(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if shift_result is not None:
        return shift_result
    return None


def solve_factor_equation_pipeline(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True, depth=0):
    # Once a product is split into factors, each factor gets a tiny private
    # rewrite budget instead of re-running the full top-level solver.
    expr = sim(expr)
    lines = []
    expr_named = named_reciprocal_trig(expr)
    if not cheap_same(expr_named, expr):
        lines.append("Use reciprocal trig forms.")
        lines.append(equation_line(expr_named, num(0)))
        expr = expr_named
    solved = solve_basic_zero_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive, depth)
    if solved is not None:
        result, extra = solved
        return result, compact_lines(lines + extra)
    if depth >= FACTOR_REWRITE_DEPTH_LIMIT:
        return None
    visited = {expr}
    pass_count = 0
    while pass_count < FACTOR_REWRITE_PASS_LIMIT:
        best = best_solve_rewrite(expr, var, visited)
        if best is None:
            break
        expr = best["expr"]
        visited.add(expr)
        i = 0
        while i < len(best["lines"]):
            lines.append(best["lines"][i])
            i += 1
        solved = solve_basic_zero_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive, depth + 1)
        if solved is not None:
            result, extra = solved
            return result, compact_lines(lines + extra)
        pass_count += 1
    return None


def solve_factor_product_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True, depth=0):
    if expr[0] != "mul":
        return None
    factors = list(flat(expr, "mul"))
    dep = []
    i = 0
    while i < len(factors):
        if depends(factors[i], var):
            dep.append(factors[i])
        i += 1
    if len(dep) < 2:
        return None
    lines = ["Set each factor equal to 0."]
    all_solutions = []
    i = 0
    while i < len(dep):
        lines.append(show(dep[i]) + " = 0")
        solved = solve_factor_equation_pipeline(dep[i], var, start_val, end_val, deg_mode, start_inclusive, end_inclusive, depth + 1)
        if solved is None:
            return None
        result, extra = solved
        j = 0
        while j < len(extra):
            lines.append(extra[j])
            j += 1
        j = 0
        while j < len(result):
            value = solve_numeric_value(expr, var, result[j], deg_mode)
            if value is not None and abs(value) < 1e-6:
                all_solutions.append(result[j])
            j += 1
        i += 1
    uniq = dedupe_values(all_solutions)
    if len(uniq) != 0:
        lines.append("Combined solutions: " + var + " = [" + ", ".join(solution_list_text(uniq, deg_mode)) + "]")
    return uniq, compact_lines(lines)


def solve_basic_zero_expr(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True, depth=0):
    factor_result = solve_factor_product_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive, depth)
    if factor_result is not None:
        return factor_result
    return solve_factor_equation_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)


def solve_rewrite_pipeline(expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    expr = sim(expr)
    lines = []
    solved = solve_compound_ratio_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if solved is not None:
        return solved, compact_lines(lines), expr
    tan2_expr = solve_tan2_cot_expr(expr)
    if tan2_expr is not None:
        expr, extra = tan2_expr
        i = 0
        while i < len(extra):
            lines.append(extra[i])
            i += 1
    solved = solve_basic_zero_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    if solved is None:
        visited = {expr}
        pass_count = 0
        while pass_count < SOLVE_PASS_LIMIT:
            best = best_solve_rewrite(expr, var, visited)
            if best is None:
                break
            expr = best["expr"]
            visited.add(expr)
            i = 0
            while i < len(best["lines"]):
                lines.append(best["lines"][i])
                i += 1
            pass_count += 1

        tan2_expr = solve_tan2_cot_expr(expr)
        if tan2_expr is not None:
            expr, extra = tan2_expr
            i = 0
            while i < len(extra):
                lines.append(extra[i])
                i += 1

        solved = solve_basic_zero_expr(expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
    return solved, compact_lines(lines), expr


def direct_single_trig_info(expr, var):
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
        if len(terms) == 2:
            coeff1, rest1 = split_coeff(terms[0])
            coeff2, rest2 = split_coeff(terms[1])
            if same(coeff1, num(1)) and same(coeff2, num(-1)) and rest1[0] == "div" and same(rest1[1], rest1[2]) and has_variable_dependency(rest1[1]) and same(rest2, num(1)):
                return None
            if same(coeff2, num(1)) and same(coeff1, num(-1)) and rest2[0] == "div" and same(rest2[1], rest2[2]) and has_variable_dependency(rest2[1]) and same(rest1, num(1)):
                return None
    quad = extract_quadratic_trig(expr, var)
    if quad is not None and is_zero(quad[4]) and not is_zero(quad[5]):
        return quad
    poly = extract_polynomial_trig(expr, var, 4)
    if poly is None:
        return None
    degree = 4
    while degree > 0 and is_zero(poly[4][degree]):
        degree -= 1
    if degree == 1:
        return poly
    return None



def solve_domain_restricted_identity_expr(lhs, rhs, expr, var, start_val, end_val, deg_mode, start_inclusive=True, end_inclusive=True):
    lhs = sim(lhs)
    rhs = sim(rhs)
    if not is_domain_restricted_identity_pair(lhs, rhs):
        return None
    restricted = lhs if lhs[0] == "div" and same(lhs[1], lhs[2]) else rhs
    denom = restricted[2]
    lines = [
        "This is only true where the denominator is defined.",
        show(denom) + " != 0",
    ]
    candidates = []
    span = abs(end_val - start_val)
    samples = estimate_numeric_scan_samples(denom, num(0), var, deg_mode, span)
    step = span * 1.0 / samples if samples > 0 else 0.0
    x1 = start_val
    y1 = solve_numeric_value(denom, var, x1, deg_mode)
    if y1 is not None and abs(y1) < 1e-6 and within_interval(x1, start_val, end_val, start_inclusive, end_inclusive):
        candidates.append(x1)
    i = 1
    while i <= samples:
        x2 = start_val + step * i
        y2 = solve_numeric_value(denom, var, x2, deg_mode)
        if y2 is not None and abs(y2) < 1e-6 and within_interval(x2, start_val, end_val, start_inclusive, end_inclusive):
            candidates.append(x2)
        if y1 is not None and y2 is not None and y1 * y2 < 0:
            root = refine_numeric_zero_bracket(denom, var, x1, x2, y1, y2, deg_mode)
            if root is not None and within_interval(root, start_val, end_val, start_inclusive, end_inclusive):
                candidates.append(root)
        x1 = x2
        y1 = y2
        i += 1
    candidates = dedupe_values(candidates)
    if len(candidates) == 0:
        lines.append("No denominator zeros found in the interval, so all values in the interval work.")
        return return_all_in_interval(var, start_val, end_val, deg_mode, lines)
    bits = []
    i = 0
    while i < len(candidates):
        bits.append(final_angle_text(candidates[i], deg_mode))
        i += 1
    lines.append("Exclude " + var + " = [" + ", ".join(bits) + "]")
    lines.append(var + " = all values in the interval except those exclusions.")
    return {"all_values_except": candidates, "var": var, "start": start_val, "end": end_val, "deg_mode": deg_mode}, compact_lines(lines)


def solve_feature_scan(expr, var):
    # This is the shared "what shape is this equation?" summary used by the
    # rewrite scorer. Keeping it cached avoids redoing tree walks everywhere.
    key = (expr, var)
    cached = SOLVE_FEATURE_CACHE.get(key)
    if cached is not None:
        return cached
    fn_names = function_names_of(expr)
    kind_names = kind_names_of(expr)
    size = tree_size(expr)
    reciprocal_load = reciprocal_burden(expr)
    angle_complexity = len(trig_arg_keys_of(expr))
    trig_types = 0
    i = 0
    while i < len(fn_names):
        if fn_names[i] in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            trig_types += 1
        i += 1
    factor_count = 0
    if expr[0] == "mul":
        items = list(flat(expr, "mul"))
        i = 0
        while i < len(items):
            if depends(items[i], var):
                factor_count += 1
            i += 1
    quad = extract_quadratic_trig(expr, var)
    poly = extract_polynomial_trig(expr, var, 4)
    poly_degree = 0
    if poly is not None:
        poly_degree = 4
        while poly_degree > 0 and is_zero(poly[4][poly_degree]):
            poly_degree -= 1
    same_angle_ratio = False
    ratio = collect_same_arg_terms(expr, var, [("sin", "sin", 1, False), ("cos", "cos", 1, False)])
    if ratio is not None and is_zero(ratio[2]):
        same_angle_ratio = True
    linear_combo = False
    if "sin" in fn_names or "cos" in fn_names:
        if size <= 28 and reciprocal_load <= 2 and angle_complexity <= 2:
            linear_combo = match_linear_combo_const(expand_trig_tree(expr), var) is not None
    has_sum_pair = sum_product_expand(expr)[0] is not None
    info = {
        "fn_names": fn_names,
        "kind_names": kind_names,
        "tree_size": size,
        "reciprocal_burden": reciprocal_load,
        "trig_types": trig_types,
        "mixed_angle_complexity": angle_complexity,
        "factor_count": factor_count,
        "has_sum_pair": has_sum_pair,
        "quadratic": quad is not None and not is_zero(quad[4]),
        "polynomial_degree": poly_degree,
        "direct_single": quad is not None and is_zero(quad[4]) and not is_zero(quad[5]) or poly_degree == 1,
        "same_angle_ratio": same_angle_ratio,
        "linear_combo": linear_combo,
    }
    return cache_store(SOLVE_FEATURE_CACHE, key, info, CACHE_LIMIT_SMALL)


def solve_shape_rank(expr, var, features=None):
    if features is None:
        features = solve_feature_scan(expr, var)
    if not depends(expr, var):
        return 9
    if expr[0] == "mul" and features["factor_count"] >= 2:
        return 0
    if features["quadratic"]:
        return 1
    if features["polynomial_degree"] >= 2:
        return 1
    if features["direct_single"]:
        return 2
    if features["same_angle_ratio"]:
        return 3
    if features["linear_combo"]:
        return 4
    return 9


def solve_expr_score(expr, var, features=None):
    if features is None:
        features = solve_feature_scan(expr, var)
    return (
        solve_shape_rank(expr, var, features),
        0 if features["linear_combo"] or features["same_angle_ratio"] or features["direct_single"] or features["quadratic"] or features["factor_count"] >= 2 else 1,
        features["trig_types"],
        features["mixed_angle_complexity"],
        features["reciprocal_burden"],
        features["tree_size"],
    )


def solve_label_rank(label):
    # When two rewrite paths are equally solvable, prefer the more
    # markscheme-like manual method over broad expansion.
    order = {
        "common_factor": 0,
        "difference_factor": 0,
        "cancel_nonzero_factor": 1,
        "reciprocal_conjugate_pair": 1,
        "sec_tan_fraction": 1,
        "tan_double_factor_poly": 2,
        "square_polynomial": 2,
        "square_sum_reduce": 3,
        "sum_to_product": 4,
        "product_to_sum": 5,
        "pythag_single": 6,
        "weighted_pythag": 7,
        "double_angle_single": 8,
        "cos_double_single": 8,
        "sin_double_single": 8,
        "double_product": 9,
        "expand": 30,
        "expand_trig": 40,
    }
    return order.get(label, 20)


def solve_candidate(expr, result, var, label):
    if result is None:
        return None
    new_expr, extra = result
    new_expr = sim(new_expr)
    if cheap_same(new_expr, expr):
        return None
    features = solve_feature_scan(new_expr, var)
    return {
        "label": label,
        "expr": new_expr,
        "lines": compact_lines(extra),
        "features": features,
        "score": solve_expr_score(new_expr, var, features),
        "label_rank": solve_label_rank(label),
    }


# ---------------------------------------------------------------------------
# Solve rewrite families
# ---------------------------------------------------------------------------
def reciprocal_family_transforms(expr, var, features):
    out = []
    fn_names = features["fn_names"]
    kind_names = features["kind_names"]
    for label, fn_rewrite in (
        ("reciprocal_conjugate_pair", solve_reciprocal_conjugate_pair_expr),
        ("cancel_nonzero_factor", solve_cancel_nonzero_reciprocal_factor_expr),
    ):
        cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
        if cand is not None:
            out.append(cand)
    if "sec" in fn_names or "tan" in fn_names:
        for label, fn_rewrite in (
            ("sec_tan_mix", solve_sec_tan_mix_expr),
            ("sec_tan_linear", solve_sec_tan_linear_expr),
            ("sin_sec", solve_sin_sec_expr),
            ("cos_tan", solve_cos_tan_expr),
        ):
            cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
            if cand is not None:
                out.append(cand)
    if "cosec" in fn_names or "cot" in fn_names:
        for label, fn_rewrite in (
            ("cosec_cot_mix", solve_cosec_cot_mix_expr),
            ("cosec_cot_halfangle", solve_cosec_cot_halfangle_expr),
            ("cosec_cot_linear", solve_cosec_cot_linear_expr),
            ("cos_cosec", solve_cos_cosec_expr),
            ("sin_cot", solve_sin_cot_expr),
        ):
            cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
            if cand is not None:
                out.append(cand)
    if "sec" in fn_names and "cosec" in fn_names:
        for label, fn_rewrite in (
            ("sec2_cosec", solve_sec2_cosec_expr),
            ("cosec2_sec", solve_cosec2_sec_expr),
            ("sec_cosec_linear", solve_sec_cosec_linear_expr),
        ):
            cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
            if cand is not None:
                out.append(cand)
        cand = solve_candidate(expr, solve_sec_cosec_product_expr(expr), var, "sec_cosec_product")
        if cand is not None:
            out.append(cand)
    if "div" in kind_names and "sin" in fn_names and "cos" in fn_names:
        for label, fn_rewrite in (
            ("sec_tan_fraction", solve_sec_tan_fraction_expr),
            ("half_angle_ratio", rewrite_half_angle_ratio_terms),
            ("conjugate_fraction", solve_conjugate_fraction_expr),
            ("ratio_one_minus_cos", solve_ratio_one_minus_cos_plus_sin_expr),
            ("cos3_over_mix", solve_cos3_over_sin_plus_sin3_over_cos_expr),
        ):
            cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
            if cand is not None:
                out.append(cand)
    if "cosec" in fn_names and "sin" in fn_names and "cos" in fn_names and "cot" in fn_names:
        cand = solve_candidate(expr, solve_cosec_minus_sin_vs_cos_cot_expr(expr, var), var, "cosec_minus_sin")
        if cand is not None:
            out.append(cand)
    return out


def angle_reduction_transforms(expr, var, features):
    out = []
    fn_names = features["fn_names"]
    kind_names = features["kind_names"]
    for label, fn_rewrite in (
        ("double_angle_single", solve_direct_double_angle_expr),
        ("square_sum_reduce", solve_square_sum_reduction_expr),
        ("pythag_single", solve_pythag_single_expr),
        ("weighted_pythag", solve_weighted_pythag_expr),
        ("cos_double_single", solve_cos_double_single_expr),
        ("sin_double_single", solve_sin_double_single_expr),
        ("double_product", solve_double_product_reduction_expr),
    ):
        cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
        if cand is not None:
            out.append(cand)
    if "sec" in fn_names or "cosec" in fn_names:
        expr_reduced = reduce_sec_cosec_squares(expr)
        expr_reduced = expand_negative_add_terms(expr_reduced)
        if not cheap_same(expr_reduced, expr):
            if "sec" in fn_names and "cosec" in fn_names:
                note = "Use sec^2 A = 1+tan^2 A and cosec^2 A = 1+cot^2 A where needed."
            elif "sec" in fn_names:
                note = "Use sec^2 A = 1+tan^2 A."
            else:
                note = "Use cosec^2 A = 1+cot^2 A."
            cand = solve_candidate(expr, (expr_reduced, [note, equation_line(expr_reduced, num(0))]), var, "sec_cosec_square_reduce")
            if cand is not None:
                out.append(cand)
    cand = solve_candidate(expr, solve_half_sec_square_expr(expr), var, "half_sec_square")
    if cand is not None:
        out.append(cand)
    if "pow" in kind_names and "sin" in fn_names and "cos" in fn_names:
        square_rewrite = rewrite_square_for_polynomial(expr, var)
        if square_rewrite is not None:
            cand = solve_candidate(expr, (square_rewrite[0], [square_rewrite[1], equation_line(square_rewrite[0], num(0))]), var, "square_polynomial")
            if cand is not None:
                out.append(cand)
    expr_basic = reduce_identities(expr)
    expr_basic = expand_negative_add_terms(expr_basic)
    if not cheap_same(expr_basic, expr):
        cand = solve_candidate(expr, (expr_basic, ["Use Pythagorean identities where possible.", equation_line(expr_basic, num(0))]), var, "identity_reduce")
        if cand is not None:
            out.append(cand)
    return out


def ratio_family_transforms(expr, var, features):
    out = []
    fn_names = features["fn_names"]
    if "tan" in fn_names or "cot" in fn_names:
        for label, fn_rewrite in (
            ("tan_double_factor_poly", rewrite_tan_double_factor_polynomial_expr),
            ("tan_cot_linear", solve_tan_cot_linear_expr),
            ("tan2_tan", solve_tan2_tan_expr),
            ("sin_tan", solve_sin_tan_expr),
            ("cos_cot", solve_cos_cot_expr),
            ("sin_cot", solve_sin_cot_expr),
            ("cos_tan", solve_cos_tan_expr),
            ("sin2_tan", solve_sin2_tan_expr),
            ("sin2_cot", solve_sin2_cot_expr),
        ):
            cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
            if cand is not None:
                out.append(cand)
    return out


def sum_product_transforms(expr, var, features):
    out = []
    if "sin" in features["fn_names"] or "cos" in features["fn_names"]:
        for label, fn_rewrite in (
            ("sum_to_product", rewrite_sum_product_terms),
            ("product_to_sum", rewrite_product_to_sum_terms),
        ):
            cand = solve_candidate(expr, fn_rewrite(expr, var), var, label)
            if cand is not None:
                out.append(cand)
    return out


def factorisation_transforms(expr, var, features):
    out = []
    factored, note = factor_common_term_once(expr, var)
    if factored is not None:
        cand = solve_candidate(expr, (factored, [note, equation_line(factored, num(0))]), var, "common_factor")
        if cand is not None:
            out.append(cand)
    factored, note = factor_difference_once(expr)
    if factored is not None:
        cand = solve_candidate(expr, (factored, [note, equation_line(factored, num(0))]), var, "difference_factor")
        if cand is not None:
            out.append(cand)
    expr_expanded = expand_embedded_small(expr)
    expr_expanded = expand_negative_add_terms(expr_expanded)
    if not cheap_same(expr_expanded, expr):
        cand = solve_candidate(expr, (expr_expanded, ["Expand and simplify.", equation_line(expr_expanded, num(0))]), var, "expand")
        if cand is not None:
            out.append(cand)
    return out


def shift_form_transforms(expr, var, features):
    out = []
    expanded = normalise_constant_trig_args(replace_exact_trig_quiet(sim(expand_embedded_small(expand_trig_tree(expr)))))
    if not cheap_same(expanded, expr):
        cand = solve_candidate(expr, (expanded, ["Expand the trig identities.", equation_line(expanded, num(0))]), var, "expand_trig")
        if cand is not None:
            out.append(cand)
    return out


# Candidates are ordered from cheap/manual-method rewrites to more elaborate
# ones so the calculator avoids building large temporary trees too early.
SOLVE_REWRITE_GROUPS = (
    (reciprocal_family_transforms,),
    (factorisation_transforms, angle_reduction_transforms, ratio_family_transforms),
    (sum_product_transforms,),
    (shift_form_transforms,),
)


def solve_group_is_expensive(index):
    return index >= len(SOLVE_REWRITE_GROUPS) - 1


def solve_candidate_is_stopworthy(candidate):
    return candidate["score"][0] <= 3 and candidate["label_rank"] < 30


def match_tan_squared_fraction(node):
    part = match_scaled_div(node)
    if part is None:
        return None
    coeff, top, bot = part
    top_pm = match_one_pm_cos_norm(top)
    bot_pm = match_one_pm_cos_norm(bot)
    if top_pm is None or bot_pm is None:
        return None
    if top_pm[0] != -1 or bot_pm[0] != 1:
        return None
    if not cheap_same(top_pm[1], bot_pm[1]):
        return None
    base = half_angle_expr(top_pm[1])
    if base is None:
        return None
    return coeff, base


def match_cot_squared_fraction(node):
    part = match_scaled_div(node)
    if part is None:
        return None
    coeff, top, bot = part
    top_pm = match_one_pm_cos_norm(top)
    bot_pm = match_one_pm_cos_norm(bot)
    if top_pm is None or bot_pm is None:
        return None
    if top_pm[0] != 1 or bot_pm[0] != -1:
        return None
    if not cheap_same(top_pm[1], bot_pm[1]):
        return None
    base = half_angle_expr(top_pm[1])
    if base is None:
        return None
    return coeff, base


def best_solve_rewrite(expr, var, visited):
    # Keep candidate choice shallow and cheap for calculator runtime:
    # rank direct school-method shapes first and avoid recursive lookahead.
    features = solve_feature_scan(expr, var)
    current = solve_expr_score(expr, var, features)
    if current[0] == 0:
        return None
    best = None
    group_i = 0
    while group_i < len(SOLVE_REWRITE_GROUPS):
        if LOW_MEMORY_RUNTIME and solve_group_is_expensive(group_i) and best is not None:
            break
        group_best = None
        family_i = 0
        while family_i < len(SOLVE_REWRITE_GROUPS[group_i]):
            candidates = SOLVE_REWRITE_GROUPS[group_i][family_i](expr, var, features)
            i = 0
            while i < len(candidates):
                candidate = candidates[i]
                key = candidate["expr"]
                if key not in visited:
                    if candidate["score"] < current:
                        choice = (candidate["score"], candidate["label_rank"], len(candidate["lines"]))
                        if group_best is None or choice < (group_best["score"], group_best["label_rank"], len(group_best["lines"])):
                            group_best = candidate
                i += 1
            family_i += 1
        if group_best is not None:
            if best is None or (group_best["score"], group_best["label_rank"], len(group_best["lines"])) < (best["score"], best["label_rank"], len(best["lines"])):
                best = group_best
            if solve_candidate_is_stopworthy(group_best):
                return group_best
        group_i += 1
    return best


# ---------------------------------------------------------------------------
# Solve tan^2(x) = k form
# ---------------------------------------------------------------------------
def solve_tan_squared_form(expr, var, start_val, end_val, deg_mode, lines):
    # Handle equations like tan^2(x) = k or (1-cos2x)/(1+cos2x) = k
    expr = sim(expr)
    # Check for fraction terms
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    else:
        terms = [expr]
    # Check for fraction: (1-cos2x)/(1+cos2x) pattern
    frac_coeff = None
    frac_node = None
    const_term = num(0)
    for term in terms:
        coeff, rest = split_coeff(term)
        if rest[0] == "div":
            frac_coeff = coeff
            frac_node = rest
        elif not depends(rest, var):
            const_term = sim(add([const_term, mul([coeff, rest])]))
    # If we have a fraction, check if it's (1-cos(2A))/(1+cos(2A))
    if frac_node is not None:
        tan2_info = match_tan_squared_fraction(frac_node)
        if tan2_info is not None:
            frac_coeff, base = tan2_info
            lines.append("Use the identity (1-cos(2A))/(1+cos(2A)) = tan^2(A).")
            k_val = eval_numeric(sim(neg(const_term)), {})
            if k_val is not None:
                return solve_tan_squared_target(base, frac_coeff, k_val, var, start_val, end_val, deg_mode, lines)
    # Check for tan^2(x) = k form directly
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    else:
        terms = [expr]
    tan2_coeff = num(0)
    tan2_arg = None
    const_term = num(0)
    for term in terms:
        coeff, rest = split_coeff(term)
        if rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] == "tan" and rest[2] == num(2):
            if tan2_arg is None:
                tan2_arg = rest[1][2]
            elif not cheap_same(tan2_arg, rest[1][2]):
                return None
            tan2_coeff = sim(add([tan2_coeff, coeff]))
        elif not depends(rest, var):
            const_term = sim(add([const_term, mul([coeff, rest])]))
        else:
            return None
    if tan2_coeff == num(0) or tan2_arg is None:
        return None
    k_val = eval_numeric(sim(neg(const_term)), {})
    if k_val is None:
        return None
    return solve_tan_squared_target(tan2_arg, tan2_coeff, k_val, var, start_val, end_val, deg_mode, lines)


def solve_tan_squared_target(arg, tan2_coeff, k_val, var, start_val, end_val, deg_mode, lines):
    # Solve tan^2(arg) = k given tan^2 coefficient
    if tan2_coeff != num(1):
        k_val = k_val / eval_numeric(tan2_coeff, {})
    angle = match_linear_angle(arg, var)
    if angle is None:
        return None
    arg_text = show(arg)
    if k_val < 0:
        lines.append("Since tan^2(" + arg_text + ") = " + format_float(k_val) + " < 0, there are no solutions.")
        return [], []
    targets = []
    if k_val == 0:
        lines.append("Using tan^2(" + arg_text + ") = 0, so tan(" + arg_text + ") = 0.")
        targets.append(0.0)
    else:
        sqrt_k = math.sqrt(k_val)
        lines.append("Using tan^2(" + arg_text + ") = " + format_float(k_val) + ", so tan(" + arg_text + ") = ±" + format_float(sqrt_k) + ".")
        targets.append(sqrt_k)
        targets.append(-sqrt_k)
    valid = []
    i = 0
    while i < len(targets):
        sols, angles = solve_angle_value("tan", angle[0], angle[1], targets[i], start_val, end_val, deg_mode)
        if len(angles) != 0:
            bits = []
            j = 0
            while j < len(angles):
                bits.append(angle_text(angles[j], deg_mode))
                j += 1
            lines.append(arg_text + " = [" + ", ".join(bits) + "]")
        j = 0
        while j < len(sols):
            valid.append(sols[j])
            j += 1
        i += 1
    valid = dedupe_values(valid)
    if len(valid) > 0:
        sol_strs = []
        i = 0
        while i < len(valid):
            sol_strs.append(final_angle_text(valid[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(sol_strs) + "]")
    else:
        lines.append("No solutions in the interval.")
    return valid, lines


def solve_sin_squared_form(expr, var, start_val, end_val, deg_mode, lines):
    # Handle sin^2(x) = k form
    expr = sim(expr)
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    else:
        terms = [expr]
    # Look for sin^2(x) term
    sin2_coeff = num(0)
    sin2_arg = None
    const_term = num(0)
    for term in terms:
        coeff, rest = split_coeff(term)
        arg = match_sin_squared_term(rest)
        if arg is not None:
            if sin2_arg is None:
                sin2_arg = arg
            elif not equivalent(sin2_arg, arg):
                return None
            sin2_coeff = sim(add([sin2_coeff, coeff]))
        elif not depends(rest, var):
            const_term = sim(add([const_term, mul([coeff, rest])]))
        else:
            return None
    if sin2_coeff == num(0) or sin2_arg is None:
        return None
    return _solve_trig_squared(sin2_arg, const_term, sin2_coeff, "sin", var, start_val, end_val, deg_mode, lines)


def solve_cos_squared_form(expr, var, start_val, end_val, deg_mode, lines):
    # Handle cos^2(x) = k form
    expr = sim(expr)
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    else:
        terms = [expr]
    # Look for cos^2(x) term
    cos2_coeff = num(0)
    cos2_arg = None
    const_term = num(0)
    for term in terms:
        coeff, rest = split_coeff(term)
        arg = match_cos_squared_term(rest)
        if arg is not None:
            if cos2_arg is None:
                cos2_arg = arg
            elif not equivalent(cos2_arg, arg):
                return None
            cos2_coeff = sim(add([cos2_coeff, coeff]))
        elif not depends(rest, var):
            const_term = sim(add([const_term, mul([coeff, rest])]))
        else:
            return None
    if cos2_coeff == num(0) or cos2_arg is None:
        return None
    return _solve_trig_squared(cos2_arg, const_term, cos2_coeff, "cos", var, start_val, end_val, deg_mode, lines)


def match_sin_squared_term(rest):
    rest = sim(rest)
    if rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] == "sin" and rest[2] == num(2):
        return rest[1][2]
    if rest[0] == "div":
        num_part = rest[1]
        den_part = rest[2]
        if den_part == num(2):
            one_pm = match_one_pm_cos_norm(num_part)
            if one_pm is not None and one_pm[0] == -1:
                return half_angle_expr(one_pm[1])
    return None


def match_cos_squared_term(rest):
    rest = sim(rest)
    if rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] == "cos" and rest[2] == num(2):
        return rest[1][2]
    if rest[0] == "div":
        num_part = rest[1]
        den_part = rest[2]
        if den_part == num(2):
            one_pm = match_one_pm_cos_norm(num_part)
            if one_pm is not None and one_pm[0] == 1:
                return half_angle_expr(one_pm[1])
    return None


def _is_sin_squared_term(rest):
    return match_sin_squared_term(rest) is not None


def _is_cos_squared_term(rest):
    return match_cos_squared_term(rest) is not None


def _solve_trig_squared(arg, const_term, trig_coeff, trig_name, var, start_val, end_val, deg_mode, lines):
    # Unified solver for trig^2(arg) = k forms
    k_val = eval_numeric(sim(neg(const_term)), {})
    if k_val is None:
        return None
    trig_val = eval_numeric(trig_coeff, {})
    if trig_val is None:
        return None
    k_val = k_val / trig_val
    angle = match_linear_angle(arg, var)
    if angle is None:
        return None
    arg_text = show(arg)
    if k_val < 0 or k_val > 1:
        lines.append("Since " + trig_name + "^2(" + arg_text + ") = " + format_float(k_val) + " is outside [0, 1], there are no solutions.")
        return [], lines
    targets = []
    if k_val == 0:
        lines.append("Using " + trig_name + "^2(" + arg_text + ") = 0, so " + trig_name + "(" + arg_text + ") = 0.")
        targets.append(0.0)
    elif k_val == 1:
        lines.append("Using " + trig_name + "^2(" + arg_text + ") = 1, so " + trig_name + "(" + arg_text + ") = ±1.")
        targets.append(1.0)
        targets.append(-1.0)
    else:
        sqrt_k = math.sqrt(k_val)
        lines.append("Using " + trig_name + "^2(" + arg_text + ") = " + format_float(k_val) + ", so " + trig_name + "(" + arg_text + ") = ±" + format_float(sqrt_k) + ".")
        targets.append(sqrt_k)
        targets.append(-sqrt_k)
    valid = []
    i = 0
    while i < len(targets):
        sols, angles = solve_angle_value(trig_name, angle[0], angle[1], targets[i], start_val, end_val, deg_mode)
        if len(angles) != 0:
            bits = []
            j = 0
            while j < len(angles):
                bits.append(angle_text(angles[j], deg_mode))
                j += 1
            lines.append(arg_text + " = [" + ", ".join(bits) + "]")
        j = 0
        while j < len(sols):
            valid.append(sols[j])
            j += 1
        i += 1
    valid = dedupe_values(valid)
    if len(valid) > 0:
        sol_strs = []
        i = 0
        while i < len(valid):
            sol_strs.append(final_angle_text(valid[i], deg_mode))
            i += 1
        lines.append(var + " = [" + ", ".join(sol_strs) + "]")
    else:
        lines.append("No solutions in the given interval.")
    return valid, lines


def solve_cosine_target(arg, k_val, var, start_val, end_val, deg_mode, lines):
    angle = match_linear_angle(arg, var)
    if angle is None:
        return None
    arg_text = show(arg)
    if k_val < -1 - 1e-6 or k_val > 1 + 1e-6:
        lines.append("Since cos(" + arg_text + ") = " + format_float(k_val) + " is outside [-1, 1], there are no solutions.")
        return [], lines
    lines.append(equation_line(fn("cos", arg), number_node(k_val)))
    sols, angles = solve_angle_value("cos", angle[0], angle[1], k_val, start_val, end_val, deg_mode)
    if len(angles) != 0:
        bits = []
        i = 0
        while i < len(angles):
            bits.append(angle_text(angles[i], deg_mode))
            i += 1
        lines.append(arg_text + " = [" + ", ".join(bits) + "]")
    if len(sols) == 0:
        lines.append("No solutions in the interval.")
        return [], lines
    bits = []
    i = 0
    while i < len(sols):
        bits.append(final_angle_text(sols[i], deg_mode))
        i += 1
    lines.append(var + " = [" + ", ".join(bits) + "]")
    return sols, lines


# ---------------------------------------------------------------------------
# Solve a*sin^2(x) + b*cos^2(x) = k form
# Using identity: sin^2(x) = (1-cos(2x))/2, cos^2(x) = (1+cos(2x))/2
# a*(1-cos(2x))/2 + b*(1+cos(2x))/2 = k
# (a+b)/2 + (b-a)/2 * cos(2x) = k
# cos(2x) = (2k - a - b) / (b - a)
# ---------------------------------------------------------------------------
def solve_sin_cos_squared_mixed(expr, var, start_val, end_val, deg_mode, lines):
    expr = sim(expr)
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    else:
        terms = [expr]
    
    sin2_coeff = num(0)
    cos2_coeff = num(0)
    const_term = num(0)
    base = None
    
    for term in terms:
        coeff, rest = split_coeff(term)
        sin_arg = match_sin_squared_term(rest)
        if sin_arg is not None:
            if base is None:
                base = sin_arg
            elif not equivalent(base, sin_arg):
                return None
            sin2_coeff = sim(add([sin2_coeff, coeff]))
        else:
            cos_arg = match_cos_squared_term(rest)
            if cos_arg is not None:
                if base is None:
                    base = cos_arg
                elif not equivalent(base, cos_arg):
                    return None
                cos2_coeff = sim(add([cos2_coeff, coeff]))
            elif not depends(rest, var):
                const_term = sim(add([const_term, mul([coeff, rest])]))
            else:
                return None
    
    if sin2_coeff == num(0) or cos2_coeff == num(0) or base is None:
        return None
    
    k_val = eval_numeric(sim(neg(const_term)), {})
    a_val = eval_numeric(sin2_coeff, {})
    b_val = eval_numeric(cos2_coeff, {})
    
    if k_val is None or a_val is None or b_val is None:
        return None
    
    if a_val == b_val:
        if abs(a_val) > 0.001 and abs(k_val / a_val - 1) < 0.01:
            lines.append("Since sin^2(A) + cos^2(A) = 1, all values work.")
            valid = []
            step = 1.0 if deg_mode else 0.1
            x = start_val
            while x <= end_val + 1e-9:
                if within_interval(x, start_val, end_val, True, True):
                    valid.append(round(x, 1) if deg_mode else round(x, 3))
                x += step
            return dedupe_values(valid), lines
        return None
    
    cos2x_val = (2 * k_val - a_val - b_val) / (b_val - a_val)
    doubled = mul([num(2), base])
    lines.append("Use sin^2(A) = (1-cos(2A))/2 and cos^2(A) = (1+cos(2A))/2.")
    return solve_cosine_target(doubled, cos2x_val, var, start_val, end_val, deg_mode, lines)


# ---------------------------------------------------------------------------
# Solve identity equations like tan^2(x) + 1 = sec^2(x), sec^2(x) - tan^2(x) = 1
# These are true for all x (within domain constraints), so return all valid x
# ---------------------------------------------------------------------------
def match_scaled_reciprocal_identity_zero(expr, var, first_name, second_name):
    data = collect_same_arg_terms(expr, var, [(first_name + "2", first_name, 2, False), (second_name + "2", second_name, 2, False)])
    if data is None:
        return None
    arg, coeffs, const = data
    scale = coeffs[first_name + "2"]
    if is_zero(scale):
        return None
    if not equivalent(coeffs[second_name + "2"], neg(scale)):
        return None
    if not equivalent(const, neg(scale)):
        return None
    return arg


def is_domain_restricted_identity_pair(lhs, rhs):
    lhs = sim(lhs)
    rhs = sim(rhs)
    if same(rhs, num(1)) and lhs[0] == "div" and same(lhs[1], lhs[2]) and has_variable_dependency(lhs[1]):
        return True
    if same(lhs, num(1)) and rhs[0] == "div" and same(rhs[1], rhs[2]) and has_variable_dependency(rhs[1]):
        return True
    return False


def solve_trig_identity_expr(expr, var, start_val, end_val, deg_mode, lines, lhs=None, rhs=None):
    expr = sim(expr)

    if is_num(expr):
        if lhs is not None and rhs is not None and is_domain_restricted_identity_pair(lhs, rhs):
            return None
        if expr == num(0) or (isinstance(expr, tuple) and expr[0] == "num" and expr[1] == 0):
            if lhs is not None and rhs is not None:
                lhs_sim = sim(lhs)
                rhs_sim = sim(rhs)
                if same(lhs_sim, rhs_sim) or equivalent(lhs_sim, rhs_sim):
                    return return_all_in_interval(var, start_val, end_val, deg_mode, lines, "This equation is an identity - both sides are equivalent.")
        return None

    if lhs is not None and rhs is not None:
        lhs_sim = sim(lhs)
        rhs_sim = sim(rhs)
        if not is_domain_restricted_identity_pair(lhs_sim, rhs_sim):
            if same(lhs_sim, rhs_sim) or equivalent(lhs_sim, rhs_sim):
                return return_all_in_interval(var, start_val, end_val, deg_mode, lines, "This equation is an identity - both sides are equivalent.")

    identity_name = None
    if match_scaled_reciprocal_identity_zero(expr, var, "sec", "tan") is not None:
        identity_name = "sec^2(A) - tan^2(A) = 1."
    elif match_scaled_reciprocal_identity_zero(expr, var, "cosec", "cot") is not None:
        identity_name = "cosec^2(A) - cot^2(A) = 1."
    else:
        return None

    lines.append("Use the identity " + identity_name)
    lines.append("This identity is true for all defined values.")
    return return_all_in_interval(var, start_val, end_val, deg_mode, lines)


def return_all_in_interval(var, start_val, end_val, deg_mode, lines, message=None):
    if message is not None:
        lines.append(message)
    lines.append("All defined values in the interval are solutions.")
    lines.append(var + " = all values in the interval.")
    return {
        "all_values": True,
        "var": var,
        "start": start_val,
        "end": end_val,
        "deg_mode": deg_mode,
    }, lines


# ---------------------------------------------------------------------------
# Solve direct trig equations like tan(2x) = 1, sin(3x) = 0.5
# These are simple equations where the LHS is a single trig function equals a constant
# ---------------------------------------------------------------------------
def solve_direct_trig_equation(expr, var, start_val, end_val, deg_mode, lines):
    info = direct_single_trig_info(expr, var)
    if info is None:
        return None
    if len(info) == 7:
        name, arg, mult, offset, a, b, c = info
        if not is_zero(a) or is_zero(b):
            return None
        coeff = b
        const = c
    else:
        name, arg, mult, offset, coeffs = info
        if polynomial_degree_coeff_map(coeffs, 4) != 1 or is_zero(coeffs[1]):
            return None
        coeff = coeffs[1]
        const = coeffs[0]
    target_expr = full_simplify(div(neg(const), coeff))
    target_value = constant_numeric(target_expr, deg_mode)
    if target_value is None:
        return None
    atom = fn(name, arg)
    out = list(lines)
    out.append("Solve the direct trig equation.")
    if not is_one(coeff):
        out.append(equation_line(mul([coeff, atom]), neg(const)))
    out.append(equation_line(atom, target_expr))
    base = trig_base_solutions(name, target_value, deg_mode)
    if len(base) == 0:
        if name in ("sin", "cos"):
            out.append(show(atom) + " = " + show(target_expr) + " has no real solutions because valid " + name + " values lie in [-1,1].")
        elif name in ("sec", "cosec"):
            out.append(show(atom) + " = " + show(target_expr) + " has no real solutions because " + name + " values have magnitude at least 1.")
        else:
            out.append("No solutions in the interval.")
        return [], out
    sols, angles = solve_angle_value(name, mult, offset, target_value, start_val, end_val, deg_mode)
    sols = dedupe_values(sols)
    if len(angles) != 0:
        bits = []
        i = 0
        while i < len(angles):
            bits.append(angle_text(angles[i], deg_mode))
            i += 1
        out.append(show(arg) + " = [" + ", ".join(bits) + "]" + (" rad" if not deg_mode else ""))
    if len(sols) == 0:
        out.append("No solutions in the interval.")
        return [], out
    bits = []
    i = 0
    while i < len(sols):
        bits.append(final_angle_text(sols[i], deg_mode))
        i += 1
    out.append(var + " = [" + ", ".join(bits) + "]")
    return sols, out


# ---------------------------------------------------------------------------
# Solve cos^4(2x) - sin^4(2x) = k form
# Using identity: cos^4(A) - sin^4(A) = (cos^2(A) - sin^2(A))(cos^2(A) + sin^2(A))
#               = cos(2A) * 1 = cos(2A)
# So cos^4(2x) - sin^4(2x) = cos(4x)
# ---------------------------------------------------------------------------
def solve_cos_pow4_minus_sin_pow4(expr, var, start_val, end_val, deg_mode, lines):
    # Check if equation is cos^4(2x) - sin^4(2x) = k
    expr = sim(expr)
    if expr[0] == "add":
        terms = list(flat(expr, "add"))
    else:
        terms = [expr]
    # Find cos^4 term, sin^4 term, and constant
    cos4_coeff = num(0)
    sin4_coeff = num(0)
    const_term = num(0)
    base = None
    for term in terms:
        coeff, rest = split_coeff(term)
        # Check for cos^4 form - rest is ('pow', inner, exponent)
        if rest[0] == "pow" and len(rest) >= 3:
            inner = rest[1]
            exp = rest[2]
            # Check if inner is cos function (direct: cos(2*x)**4)
            if inner[0] == "fn" and inner[1] == "cos" and exp == num(4):
                if base is None:
                    base = inner[2]
                elif not equivalent(base, inner[2]):
                    return None
                cos4_coeff = sim(add([cos4_coeff, coeff]))
            # Check for sin^4 form (direct: sin(2*x)**4)
            elif inner[0] == "fn" and inner[1] == "sin" and exp == num(4):
                if base is None:
                    base = inner[2]
                elif not equivalent(base, inner[2]):
                    return None
                sin4_coeff = sim(add([sin4_coeff, coeff]))
            # Check for expanded sin form: (2*sin(x)*cos(x))^4
            elif inner[0] == "mul" and exp == num(4):
                product = match_same_angle_sin_cos_product(inner)
                if product is not None and same(product[0], num(2)):
                    expanded_arg = mul([num(2), product[1]])
                    if base is None:
                        base = expanded_arg
                    elif not equivalent(base, expanded_arg):
                        return None
                    sin4_coeff = sim(add([sin4_coeff, coeff]))
        elif not depends(rest, var):
            const_term = sim(add([const_term, mul([coeff, rest])]))
    # Check if we have cos^4 - sin^4 form (coefficients should be +1 and -1 or match)
    if cos4_coeff == num(0) or sin4_coeff == num(0) or base is None:
        return None
    # The equation is cos^4(2x) - sin^4(2x) = k which simplifies to cos(4x) = k
    k_val = eval_numeric(sim(neg(const_term)), {})
    if k_val is None:
        return None
    # Normalize: if cos4_coeff is not 1, divide k
    if cos4_coeff != num(1):
        k_val = k_val / eval_numeric(cos4_coeff, {})
    lines.append("Use the identity cos^4(A) - sin^4(A) = cos(2A).")
    doubled = mul([num(2), base])
    lines.append("So cos^4(" + show(base) + ") - sin^4(" + show(base) + ") = cos(" + show(doubled) + ").")
    # Solve cos(2A) = k
    return solve_cosine_target(doubled, k_val, var, start_val, end_val, deg_mode, lines)


# ---------------------------------------------------------------------------
# Solve sqrt(3) sin(2x) + 2 sin^2(x) = 1 form
# Using sin(2x) = 2 sin(x) cos(x) and sin^2(x) = (1 - cos(2x))/2
# Substitute: sqrt(3) * 2 sin(x) cos(x) + 2 * (1 - cos(2x))/2 = 1
#             2*sqrt(3) sin(x) cos(x) + 1 - cos(2x) = 1
#             2*sqrt(3) sin(x) cos(x) - cos(2x) = 0
# Using cos(2x) = 2cos^2(x) - 1:
#             2*sqrt(3) sin(x) cos(x) - 2cos^2(x) + 1 = 0
# Rearranging: 2cos^2(x) - 2*sqrt(3) sin(x) cos(x) - 1 = 0
# Divide by cos(x) (assuming cos(x) != 0): 2cos(x) - 2*sqrt(3) sin(x) = sec(x)
# This becomes a linear combination: A sin(x) + B cos(x) = C form
# ---------------------------------------------------------------------------
def solve_mixed_sin2_sin_square(expr, var, start_val, end_val, deg_mode, lines):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    sin2x_coeff = num(0)
    sin_sq_coeff = num(0)
    cos2x_coeff = num(0)
    const_term = num(0)
    base = None
    used_product = False
    for term in terms:
        coeff, rest = split_const_factor(term, var)
        if not depends(rest, var):
            const_term = sim(add([const_term, mul([coeff, rest])]))
            continue
        if rest[0] == "fn" and rest[1] in ("sin", "cos"):
            candidate = half_angle_expr(rest[2])
            if candidate is not None and depends(candidate, var):
                if base is None:
                    base = candidate
                elif not equivalent(base, candidate):
                    return None
                if rest[1] == "sin":
                    sin2x_coeff = sim(add([sin2x_coeff, coeff]))
                else:
                    cos2x_coeff = sim(add([cos2x_coeff, coeff]))
                continue
        if rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] == "sin" and same(rest[2], num(2)) and depends(rest[1][2], var):
            if base is None:
                base = rest[1][2]
            elif not equivalent(base, rest[1][2]):
                return None
            sin_sq_coeff = sim(add([sin_sq_coeff, coeff]))
            continue
        product = match_same_angle_sin_cos_product(rest)
        if product is not None:
            product_coeff, product_base = product
            if base is None:
                base = product_base
            elif not equivalent(base, product_base):
                return None
            sin2x_coeff = sim(add([sin2x_coeff, mul([coeff, product_coeff, num(1, 2)])]))
            used_product = True
            continue
        return None
    if base is None or is_zero(sin2x_coeff) or (is_zero(sin_sq_coeff) and is_zero(cos2x_coeff)):
        return None
    doubled = mul([num(2), base])
    rewritten = []
    if not is_zero(sin2x_coeff):
        rewritten.append(mul([sin2x_coeff, fn("sin", doubled)]))
    cos_coeff = full_simplify(add([cos2x_coeff, neg(div(sin_sq_coeff, num(2)))]))
    if not is_zero(cos_coeff):
        rewritten.append(mul([cos_coeff, fn("cos", doubled)]))
    new_const = full_simplify(add([const_term, div(sin_sq_coeff, num(2))]))
    if not is_zero(new_const):
        rewritten.append(new_const)
    if len(rewritten) == 0:
        return None
    rewritten_expr = sim(make_add(rewritten))
    solved = solve_linear_combo_shift(rewritten_expr, var, start_val, end_val, deg_mode)
    if solved is None:
        return None
    prelude = []
    if used_product and not is_zero(sin_sq_coeff):
        prelude.append("Use sin(2A) = 2sin(A)cos(A) and sin^2(A) = (1-cos(2A))/2.")
    elif used_product:
        prelude.append("Use sin(2A) = 2sin(A)cos(A).")
    else:
        prelude.append("Use sin^2(A) = (1-cos(2A))/2.")
    prelude.append(equation_line(rewritten_expr, num(0)))
    values, solved_lines = solved
    return values, compact_lines(list(lines) + prelude + solved_lines)


# ---------------------------------------------------------------------------
# Solve mode entrypoint
# ---------------------------------------------------------------------------
def parse_solve_interval_context(lhs, rhs, var, interval_bits):
    eq_mode = equation_angle_mode(lhs, rhs, var)
    no_interval_mode = False
    if len(interval_bits) == 0:
        no_interval_mode = True
        deg_mode = eq_mode != "rad"
        span = default_no_interval_span(lhs, rhs, var, deg_mode)
        return -span, span, True, True, deg_mode, no_interval_mode
    if len(interval_bits) == 1 and looks_like_interval_relation(interval_bits[0], var):
        start_val, start_pi, start_inclusive, end_val, end_pi, end_inclusive, start_text, end_text = parse_interval_relation(interval_bits[0], var)
        interval_mode = interval_angle_mode([start_text, end_text], start_val, end_val, start_pi, end_pi)
        if eq_mode is not None and eq_mode != interval_mode:
            raise ValueError("Mixed degree and radian input is not supported in solve mode.")
        deg_mode = (eq_mode or interval_mode) != "rad"
        if deg_mode:
            if start_pi:
                start_val = to_degrees(start_val)
            if end_pi:
                end_val = to_degrees(end_val)
        return start_val, end_val, start_inclusive, end_inclusive, deg_mode, no_interval_mode
    if len(interval_bits) == 2:
        start_val, start_pi, start_inclusive = parse_interval_bound(interval_bits[0], True)
        end_val, end_pi, end_inclusive = parse_interval_bound(interval_bits[1], False)
        interval_mode = interval_angle_mode(interval_bits, start_val, end_val, start_pi, end_pi)
        if eq_mode is not None and eq_mode != interval_mode:
            raise ValueError("Mixed degree and radian input is not supported in solve mode.")
        deg_mode = (eq_mode or interval_mode) != "rad"
        if deg_mode:
            if start_pi:
                start_val = to_degrees(start_val)
            if end_pi:
                end_val = to_degrees(end_val)
        return start_val, end_val, start_inclusive, end_inclusive, deg_mode, no_interval_mode
    raise ValueError("Use equation,var or equation,var,start,end or equation,var,0<x<pi")


def try_identity_candidates(candidates, var, start_val, end_val, deg_mode, lines, lhs, rhs):
    seen = set()
    i = 0
    while i < len(candidates):
        cur = sim(candidates[i])
        if cur not in seen:
            seen.add(cur)
            trial_lines = list(lines)
            result = solve_trig_identity_expr(cur, var, start_val, end_val, deg_mode, trial_lines, lhs=lhs, rhs=rhs)
            if result is not None:
                return result
        i += 1
    return None


def try_solver_on_candidates(candidates, solver, var, start_val, end_val, deg_mode, lines):
    seen = set()
    i = 0
    while i < len(candidates):
        expr, prelude = candidates[i]
        cur = sim(expr)
        if cur not in seen:
            seen.add(cur)
            trial_lines = list(lines)
            if prelude is not None:
                j = 0
                while j < len(prelude):
                    trial_lines.append(prelude[j])
                    j += 1
            solved = solver(cur, var, start_val, end_val, deg_mode, trial_lines)
            if solved is not None:
                return solved
        i += 1
    return None


def try_special_solve_routes(lhs, rhs, expr, expr_before_expand, expanded_expr, var, start_val, end_val, deg_mode, lines):
    restricted_identity = solve_domain_restricted_identity_expr(lhs, rhs, expr, var, start_val, end_val, deg_mode)
    if restricted_identity is not None:
        return restricted_identity
    identity_result = try_identity_candidates([expr], var, start_val, end_val, deg_mode, lines, lhs, rhs)
    if identity_result is not None:
        return identity_result

    rhs_val = constant_numeric(rhs, deg_mode)
    if rhs_val is not None and lhs[0] == "div":
        tan2_info = match_tan_squared_fraction(lhs)
        if tan2_info is not None:
            frac_coeff, base = tan2_info
            trial_lines = list(lines)
            trial_lines.append("Use the identity (1-cos(2A))/(1+cos(2A)) = tan^2(A).")
            return solve_tan_squared_target(base, frac_coeff, rhs_val, var, start_val, end_val, deg_mode, trial_lines)

    direct_result = try_solver_on_candidates([(expr_before_expand, None)], solve_direct_trig_equation, var, start_val, end_val, deg_mode, lines)
    if direct_result is not None:
        return direct_result

    shifted_tan_cot = try_solver_on_candidates(
        [(expr, None), (expanded_expr, None)],
        solve_shifted_tan_cot_expr,
        var,
        start_val,
        end_val,
        deg_mode,
        lines,
    )
    if shifted_tan_cot is not None:
        return shifted_tan_cot

    identity_result = try_identity_candidates([expr, expanded_expr], var, start_val, end_val, deg_mode, lines, lhs, rhs)
    if identity_result is not None:
        return identity_result

    candidate_pairs = [(expanded_expr, None), (expr, None)]
    for solver in (solve_tan_squared_form, solve_cos_pow4_minus_sin_pow4, solve_sin_squared_form, solve_cos_squared_form, solve_sin_cos_squared_mixed):
        solved = try_solver_on_candidates(candidate_pairs, solver, var, start_val, end_val, deg_mode, lines)
        if solved is not None:
            return solved

    mixed_solved = try_solver_on_candidates([(expr, None)], solve_mixed_sin2_sin_square, var, start_val, end_val, deg_mode, lines)
    if mixed_solved is not None:
        return mixed_solved

    expanded_direct_prelude = None
    if not cheap_same(expanded_expr, expr):
        expanded_direct_prelude = ["Expand the expression.", equation_line(expanded_expr, num(0))]
    direct_result = try_solver_on_candidates(
        [(expr, None), (expanded_expr, expanded_direct_prelude)],
        solve_direct_trig_equation,
        var,
        start_val,
        end_val,
        deg_mode,
        lines,
    )
    if direct_result is not None:
        return direct_result

    return try_identity_candidates([expr, expanded_expr], var, start_val, end_val, deg_mode, lines, lhs, rhs)


def solution_list_line(var, values, deg_mode):
    return var + " = [" + ", ".join(solution_list_text(values, deg_mode)) + "]"


def drop_trailing_solution_line(lines, var):
    prefixes = (var + " = [", "Combined solutions: " + var + " = [")
    while len(lines) != 0:
        keep = True
        i = 0
        while i < len(prefixes):
            if lines[-1].startswith(prefixes[i]):
                keep = False
                break
            i += 1
        if keep:
            break
        lines.pop()


def solve_x_equation_text(eq_text, var, interval_bits, want_meta=False):
    lhs, rhs = parse_equation_or_zero(eq_text)
    original_expr = sim(add([lhs, neg(rhs)]))
    derived = derive_cot_quadratic_expr(lhs, rhs)
    expr = sim(add([lhs, neg(rhs)]))
    lines = ["Start with " + equation_line(lhs, rhs)]
    if derived is not None:
        expr = derived[4]
        lines.append("Rewrite the equation in standard trig form")
        lines.append(equation_line(expr, num(0)))
    else:
        lines.append("Move all terms to one side")
        lines.append(equation_line(expr, num(0)))
    expr_named = named_reciprocal_trig(expr)
    if not cheap_same(expr_named, expr):
        lines.append("Use reciprocal trig forms.")
        lines.append(equation_line(expr_named, num(0)))
        expr = expr_named

    expr_before_expand = expr
    expanded_expr = expand_trig_tree(expr_before_expand)
    start_val, end_val, start_inclusive, end_inclusive, deg_mode, no_interval_mode = parse_solve_interval_context(lhs, rhs, var, interval_bits)

    special_solved = try_special_solve_routes(lhs, rhs, expr, expr_before_expand, expanded_expr, var, start_val, end_val, deg_mode, lines)
    solved = None
    chosen_extra = []
    if special_solved is not None:
        if isinstance(special_solved[0], dict):
            return special_solved
        lines = list(special_solved[1])
        solved = (special_solved[0], [])
    else:
        solved_try, extra_try, final_expr = solve_rewrite_pipeline(expr_before_expand, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
        if solved_try is not None:
            solved = solved_try
            chosen_extra = extra_try
            expr = final_expr
        elif not cheap_same(expanded_expr, expr_before_expand):
            solved_try, extra_try, final_expr = solve_rewrite_pipeline(expanded_expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)
            if solved_try is not None:
                solved = solved_try
                expr = final_expr
                chosen_extra = ["Expand the expression.", equation_line(expanded_expr, num(0))]
                i = 0
                while i < len(extra_try):
                    chosen_extra.append(extra_try[i])
                    i += 1

        if solved is None:
            identity_result = try_identity_candidates([expr], var, start_val, end_val, deg_mode, lines, lhs, rhs)
            if identity_result is not None:
                return identity_result
            solved = solve_numeric_trig_fallback(lhs, rhs, original_expr, var, start_val, end_val, deg_mode, start_inclusive, end_inclusive)

    result, extra = solved
    i = 0
    while i < len(chosen_extra):
        lines.append(chosen_extra[i])
        i += 1
    i = 0
    while i < len(extra):
        lines.append(extra[i])
        i += 1

    drop_trailing_solution_line(lines, var)
    valid = []
    invalid = []
    i = 0
    while i < len(result):
        ok, reason = validate_x_solution(lhs, rhs, var, result[i], deg_mode)
        if ok:
            valid.append(result[i])
        else:
            invalid.append((result[i], reason))
        i += 1
    valid = dedupe_values(valid)

    if len(invalid) != 0:
        lines.append("Check each solution in the original equation.")
        i = 0
        while i < len(invalid):
            lines.append("Reject " + var + " = " + angle_text(invalid[i][0], deg_mode) + " because " + invalid[i][1])
            i += 1
        if len(valid) == 0:
            lines.append("No valid solutions in the interval.")

    if no_interval_mode:
        picked = trim_default_no_interval_solutions(valid)
        lines.append("No interval given, so show 5 above 0 and 5 below.")
        if len(picked) == 0:
            lines.append("No valid solutions.")
        else:
            lines.append(solution_list_line(var, picked, deg_mode))
        valid = picked
    elif len(valid) != 0:
        lines.append(solution_list_line(var, valid, deg_mode))

    if want_meta:
        return valid, compact_lines(lines), deg_mode
    return valid, compact_lines(lines)


def solve_solve_text(text):
    begin_user_action()
    bits = split_top_level_all(text, ",")
    if len(bits) < 2:
        raise ValueError("Use equation,k or equation,x,0,90 or equation,x,0<x<pi")
    if bits[1].strip().lower() in ("max", "min"):
        kind = bits[1].strip().lower()
        var = bits[2].strip() if len(bits) >= 3 and bits[2].strip() != "" else "x"
        if len(bits) > 3:
            raise ValueError("Extremum mode uses expression,max,x or expression,min,x")
        return solve_extremum_text(bits[0], kind, var)
    eq_text = bits[0]
    var = bits[1].strip()
    extra = bits[2:]
    if var == "":
        raise ValueError("Choose a variable to solve for.")
    lhs, rhs = parse_equation_or_zero(eq_text)
    if var == "x" or len(extra) != 0 or equation_has_trig_content(lhs, rhs):
        return solve_x_equation_text(eq_text, var, extra)
    return solve_linear_parameter_text(eq_text, var)


def solve_prove_text(text, route):
    begin_user_action()
    rewrite = split_rewrite_arrow(text)
    if rewrite is not None:
        return solve_transform_text(rewrite[0], rewrite[1])
    # Check for numeric evaluation requests
    lines = numeric_evaluation_text(text)
    if lines is not None:
        return compact_lines(lines)
    lhs, rhs = parse_identity(text)
    lines = prove_cot_quadratic_equation(lhs, rhs)
    if lines is not None:
        return compact_lines(lines)
    lines = special_text_proof(text, route)
    if lines is not None:
        return compact_lines(lines)
    if is_domain_restricted_identity_pair(lhs, rhs):
        raise ValueError("Identity depends on domain restrictions, so prove/show mode cannot use it as a clean identity.")
    return solve_prove(lhs, rhs, route)


def rewrite_extremum_double_angle_expr(expr, var):
    expr = sim(expr)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    out = []
    changed = False
    i = 0
    while i < len(terms):
        coeff, rest = split_coeff(terms[i])
        sin_arg = match_sin_squared_term(rest)
        if sin_arg is None and rest[0] == "mul":
            items = list(flat(rest, "mul"))
            if len(items) == 2 and items[0][0] == "fn" and items[1][0] == "fn" and items[0][1] == "sin" and items[1][1] == "sin" and same(items[0][2], items[1][2]):
                sin_arg = items[0][2]
        if sin_arg is not None:
            doubled = mul([num(2), sin_arg])
            out.append(div(coeff, num(2)))
            out.append(mul([neg(div(coeff, num(2))), fn("cos", doubled)]))
            changed = True
            i += 1
            continue
        cos_arg = match_cos_squared_term(rest)
        if cos_arg is None and rest[0] == "mul":
            items = list(flat(rest, "mul"))
            if len(items) == 2 and items[0][0] == "fn" and items[1][0] == "fn" and items[0][1] == "cos" and items[1][1] == "cos" and same(items[0][2], items[1][2]):
                cos_arg = items[0][2]
        if cos_arg is not None:
            doubled = mul([num(2), cos_arg])
            out.append(div(coeff, num(2)))
            out.append(mul([div(coeff, num(2)), fn("cos", doubled)]))
            changed = True
            i += 1
            continue
        product = match_same_angle_sin_cos_product(rest)
        if product is not None:
            out.append(mul([coeff, product[0], num(1, 2), fn("sin", mul([num(2), product[1]]))]))
            changed = True
            i += 1
            continue
        out.append(terms[i])
        i += 1
    if not changed:
        return None
    return sim(make_add(out))


def extremum_rewrite_to_linear_combo(expr, var):
    current = named_reciprocal_trig(expr)
    lines = []
    if not cheap_same(current, expr):
        lines.append("Use reciprocal trig forms.")
        lines.append("Expression becomes " + show(current) + ".")
    doubled = rewrite_extremum_double_angle_expr(current, var)
    if doubled is not None:
        current = doubled
        lines.append("Use double-angle identities.")
        lines.append("Expression becomes " + show(current) + ".")
    visited = {current}
    pass_count = 0
    while pass_count <= SOLVE_PASS_LIMIT:
        if match_linear_combo_const(current, var) is not None:
            return current, compact_lines(lines)
        expanded = sim(expand_embedded_small(replace_exact_trig_quiet(sim(expand_trig_tree(current)))))
        if match_linear_combo_const(expanded, var) is not None:
            if not same(expanded, current):
                lines.append("Rewrite using trig identities.")
                lines.append("Expression becomes " + show(expanded) + ".")
            return expanded, compact_lines(lines)
        if pass_count == SOLVE_PASS_LIMIT:
            break
        best = best_solve_rewrite(current, var, visited)
        if best is None:
            break
        current = best["expr"]
        visited.add(current)
        lines.append("Rewrite using trig identities.")
        lines.append("Expression becomes " + show(current) + ".")
        pass_count += 1
    return None, compact_lines(lines)


def solve_extremum_numeric(expr, kind, var, dep_label, deg_mode):
    periods = []
    estimate_solve_periods(expr, var, deg_mode, periods)
    if len(periods) == 0:
        span = 360.0 if deg_mode else 2.0 * math.pi
    else:
        span = periods[0]
        i = 1
        while i < len(periods):
            if periods[i] > span:
                span = periods[i]
            i += 1
    samples = 4096
    step = span / samples
    best_x = None
    best_y = None
    i = 0
    while i <= samples:
        x = step * i
        y = solve_numeric_value(expr, var, x, deg_mode)
        if y is not None:
            better = False
            if best_y is None:
                better = True
            elif kind == "max" and y > best_y + 1e-7:
                better = True
            elif kind == "min" and y < best_y - 1e-7:
                better = True
            elif best_x is not None and abs(y - best_y) < 1e-7 and x < best_x:
                better = True
            if better:
                best_x = x
                best_y = y
        i += 1
    if best_x is None:
        raise ValueError("This is undefined on the search interval.")
    left = best_x - step
    if left < 0:
        left = 0
    right = best_x + step
    if right > span:
        right = span
    i = 0
    while i <= 120:
        x = left + (right - left) * i / 120.0
        y = solve_numeric_value(expr, var, x, deg_mode)
        if y is not None:
            better = False
            if kind == "max" and y > best_y + 1e-9:
                better = True
            elif kind == "min" and y < best_y - 1e-9:
                better = True
            elif abs(y - best_y) < 1e-9 and x < best_x:
                better = True
            if better:
                best_x = x
                best_y = y
        i += 1
    desc = "Maximum" if kind == "max" else "Minimum"
    lines = [
        "No clean R*sin/cos form, so scan one period numerically.",
        "Scan 0 <= " + var + " <= " + angle_text(span, deg_mode) + (" deg" if deg_mode else " rad") + ".",
        desc + " " + dep_label + " = " + final_exact_text_or_float(best_y, 4),
        desc + " first occurs when " + var + " = " + final_angle_text(best_x, deg_mode),
    ]
    return {"value": best_y, "x": best_x}, compact_lines(lines)


def solve_extremum_text(text, kind, var):
    lhs, rhs = parse_equation_or_zero(text)
    dep_label = "y"
    expr = lhs
    if depends(lhs, var) and not depends(rhs, var):
        expr = lhs
    elif depends(rhs, var) and not depends(lhs, var):
        expr = rhs
        if lhs[0] == "sym":
            dep_label = lhs[1]
    else:
        expr = lhs if depends(lhs, var) else rhs
    eq_mode = equation_angle_mode(lhs, rhs, var)
    deg_mode = eq_mode != "rad"
    expanded = sim(expand_embedded_small(replace_exact_trig_quiet(sim(expand_trig_tree(expr)))))
    combo = match_linear_combo_const(expanded, var)
    prep_lines = []
    if combo is None:
        expanded, prep_lines = extremum_rewrite_to_linear_combo(expr, var)
        if expanded is not None:
            combo = match_linear_combo_const(expanded, var)
    if combo is None:
        return solve_extremum_numeric(expr, kind, var, dep_label, deg_mode)
    base, sin_coeff_expr, cos_coeff_expr, const_expr = combo
    angle = match_linear_angle(base, var)
    if angle is None:
        return solve_extremum_numeric(expr, kind, var, dep_label, deg_mode)
    a_val = constant_numeric(sin_coeff_expr, deg_mode)
    b_val = constant_numeric(cos_coeff_expr, deg_mode)
    c_val = constant_numeric(const_expr, deg_mode)
    m_val = constant_numeric(angle[0], deg_mode)
    offset_val = constant_numeric(angle[1], deg_mode)
    if a_val is None or b_val is None or c_val is None or m_val is None or offset_val is None or abs(m_val) < 1e-12:
        return solve_extremum_numeric(expr, kind, var, dep_label, deg_mode)
    r_val = math.sqrt(a_val * a_val + b_val * b_val)
    phase = to_degrees(math.atan2(-a_val, b_val)) if deg_mode else math.atan2(-a_val, b_val)
    if kind == "max":
        extreme = c_val + r_val
        target = 0.0
        desc = "Maximum"
    else:
        extreme = c_val - r_val
        target = 180.0 if deg_mode else math.pi
        desc = "Minimum"
    period = 360.0 if deg_mode else 2.0 * math.pi
    sols = []
    # Track the first positive occurrence by solving the phase condition
    # over several periods and taking the smallest non-negative x.
    n = -20
    while n <= 20:
        phase_target = target + period * n
        x_val = (phase_target - phase - offset_val) / m_val
        if x_val >= -1e-8:
            sols.append((x_val, phase_target))
        n += 1
    sols.sort()
    first_x = None
    first_phase_target = None
    i = 0
    while i < len(sols):
        if first_x is None or abs(sols[i][0] - first_x) > 1e-7:
            if first_x is None:
                first_x = sols[i][0]
                first_phase_target = sols[i][1]
            i += 1
            continue
        i += 1
    lines = [
        "Write the model in the form C + R*cos(" + show(base) + signed_angle_text(phase, deg_mode) + ")",
        "R = sqrt(A^2+B^2)",
        "= " + exact_text_or_float(r_val, 6),
        dep_label + " = " + show(full_simplify(const_expr)) + "+" + exact_text_or_float(r_val, 6) + "*cos(" + show(base) + signed_angle_text(phase, deg_mode) + ")",
        desc + " " + dep_label + " = " + final_exact_text_or_float(extreme, 4),
    ]
    if first_x is not None:
        lines.append(desc + " first occurs when " + show(base) + signed_angle_text(phase, deg_mode) + " = " + angle_text(first_phase_target, deg_mode))
        lines.append(var + " = " + final_angle_text(first_x, deg_mode))
    return {"value": extreme, "x": first_x}, compact_lines(prep_lines + lines)


# ---------------------------------------------------------------------------
# Numeric evaluation for prove/show mode
# ---------------------------------------------------------------------------
def numeric_evaluation_text(text):
    text = text.strip()
    # Check if it looks like a numeric expression (no variable dependency)
    if "=" not in text:
        # This is a standalone expression like "tan(75)" or "sin(30) + cos(60)"
        node = parse(text)
        if node is None:
            return None
        if depends(node, "x") or depends(node, "theta") or depends(node, "a") or depends(node, "A"):
            return None
        result = eval_numeric(node, {})
        if result is not None:
            lines = ["Calculate " + text + "."]
            lines.append(text + " = " + format_numeric_result(result))
            return lines
        return None
    lhs_text, rhs_text = text.split("=", 1)
    lhs_text = lhs_text.strip()
    rhs_text = rhs_text.strip()
    lhs = parse(lhs_text)
    if lhs is None:
        return None
    if not depends(lhs, "x") and not depends(lhs, "theta") and not depends(lhs, "a"):
        result = eval_numeric(lhs, {})
        if result is not None:
            lines = ["Calculate " + lhs_text + "."]
            lines.append(lhs_text + " = " + format_numeric_result(result))
            if rhs_text != "" and rhs_text.lower() != "x" and rhs_text.lower() != "theta":
                target = parse(rhs_text)
                if target is not None:
                    target_val = eval_numeric(target, {})
                    if target_val is not None and abs(result - target_val) < 1e-9:
                        lines.append("= " + rhs_text)
                    elif target is not None:
                        lines.append("= " + format_numeric_result(result) + " (target: " + rhs_text + ")")
            return lines
    return None


def format_numeric_result(val):
    if val is None:
        return "?"
    if abs(val) < 1e-10:
        return "0"
    if abs(val - round(val)) < 1e-9:
        return str(int(round(val)))
    return str(round(val, 10))


def numeric_eval(node, env, deg_mode=True):
    """Numeric evaluation for prove/show mode with degree support."""
    if node is None:
        return None
    if not isinstance(node, tuple) or len(node) < 2:
        return None
    kind = node[0]
    if kind == "num":
        if len(node) == 3:
            return node[1] / node[2]
        return node[1]
    if kind == "const":
        if node[1] == "pi":
            return PI_FLOAT
        if node[1] == "e":
            return E_FLOAT
        return None
    if kind == "sym":
        if node[1] in env:
            return env[node[1]]
        return None
    if kind == "fn":
        fn_name = node[1]
        arg = node[2]
        arg_val = numeric_eval(arg, env, deg_mode)
        if arg_val is None:
            return None
        if deg_mode and fn_name in ("sin", "cos", "tan", "cot", "sec", "cosec"):
            arg_val = arg_val * PI_FLOAT / 180.0
        if fn_name == "sin":
            return math.sin(arg_val)
        if fn_name == "cos":
            return math.cos(arg_val)
        if fn_name == "tan":
            return math.tan(arg_val)
        if fn_name == "cot":
            return 1.0 / math.tan(arg_val) if abs(math.tan(arg_val)) > 1e-12 else None
        if fn_name == "sec":
            return 1.0 / math.cos(arg_val) if abs(math.cos(arg_val)) > 1e-12 else None
        if fn_name == "cosec":
            return 1.0 / math.sin(arg_val) if abs(math.sin(arg_val)) > 1e-12 else None
        if fn_name == "sqrt":
            return math.sqrt(arg_val) if arg_val >= 0 else None
        return None
    # Handle add/mul/div/pow - check if node[1] is a tuple (multiple terms) or binary
    if kind == "add":
        # node[1] is a tuple of terms to add
        if isinstance(node[1], tuple):
            out = 0.0
            i = 0
            while i < len(node[1]):
                val = numeric_eval(node[1][i], env, deg_mode)
                if val is None:
                    return None
                out += val
                i += 1
            return out
        # Binary form node[1], node[2]
        if len(node) < 3:
            return None
        a = numeric_eval(node[1], env, deg_mode)
        b = numeric_eval(node[2], env, deg_mode)
        if a is None or b is None:
            return None
        return a + b
    if kind == "mul":
        # node[1] is a tuple of terms to multiply
        if isinstance(node[1], tuple):
            out = 1.0
            i = 0
            while i < len(node[1]):
                val = numeric_eval(node[1][i], env, deg_mode)
                if val is None:
                    return None
                out *= val
                i += 1
            return out
        # Binary form node[1], node[2]
        if len(node) < 3:
            return None
        a = numeric_eval(node[1], env, deg_mode)
        b = numeric_eval(node[2], env, deg_mode)
        if a is None or b is None:
            return None
        return a * b
    if kind == "div":
        if len(node) < 3:
            return None
        a = numeric_eval(node[1], env, deg_mode)
        b = numeric_eval(node[2], env, deg_mode)
        if a is None or b is None or abs(b) < 1e-12:
            return None
        return a / b
    if kind == "pow":
        if len(node) < 3:
            return None
        base = numeric_eval(node[1], env, deg_mode)
        exp = numeric_eval(node[2], env, deg_mode)
        if base is None or exp is None:
            return None
        if base < 0 and exp != int(exp):
            return None
        return base ** exp
    return None


def eval_numeric(node, env):
    """Wrapper that calls numeric_eval with degree mode for backward compatibility."""
    return numeric_eval(node, env, True)


# ---------------------------------------------------------------------------
# Transform mode: R sin(x+alpha) and R cos(x+alpha) forms
# ---------------------------------------------------------------------------
def transform_r_sin_cos_form(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, target_text):
    var = detect_transform_var(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs)
    expr = sim(add([eq1_lhs, neg(eq1_rhs)]))
    combo = match_linear_combo_const(expand_trig_tree(sim(expr)), var)
    if combo is None:
        return None
    base, sin_coeff, cos_coeff, const = combo
    if not same(base, sym(var)):
        return None

    target = extract_shift_equation(eq2_lhs, eq2_rhs, sym(var))
    if target is None:
        return None
    _target_side, target_const, shift = target
    _coeff, name, _shift_node, sign = shift

    sin_val = constant_numeric(sin_coeff, True)
    cos_val = constant_numeric(cos_coeff, True)
    if sin_val is None or cos_val is None:
        return None
    r = math.sqrt(sin_val * sin_val + cos_val * cos_val)
    alpha_deg = shift_angle_from_combo(name, sign, cos_coeff, sin_coeff, True)
    if alpha_deg is None:
        return None

    lines = [
        "Start with " + equation_line(eq1_lhs, eq1_rhs),
        "Write in the form A*sin(" + var + ") + B*cos(" + var + ") = C.",
        "A = " + show(sin_coeff) + ", B = " + show(cos_coeff) + ", C = " + show(const),
        "R = sqrt(A^2+B^2)",
        "R = " + format_float(r),
        "alpha = " + fixed_float_text(alpha_deg, 1) + " deg",
        format_float(r) + "*" + name + "(" + var + signed_angle_text(alpha_deg, True) + ") = " + show(target_const),
    ]
    return compact_lines(lines)


def format_float(val):
    if val is None:
        return "?"
    return str(round(val, 3))


def transform_sin_cos_2x_form(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs, target_text):
    var = detect_transform_var(eq1_lhs, eq1_rhs, eq2_lhs, eq2_rhs)

    def target_has_double_angle(node):
        node = sim(node)
        if node[0] == "fn" and node[1] in ("sin", "cos"):
            base = match_numeric_multiple_arg(node[2], 2)
            return base is not None and same(base, sym(var))
        if node[0] in ("add", "mul"):
            items = flat(node, node[0])
            i = 0
            while i < len(items):
                if target_has_double_angle(items[i]):
                    return True
                i += 1
        elif node[0] in ("div", "pow"):
            i = 1
            while i < len(node):
                if target_has_double_angle(node[i]):
                    return True
                i += 1
        return False

    if not target_has_double_angle(eq2_lhs) and not target_has_double_angle(eq2_rhs):
        return None
    # Get the expression (move all to one side)
    expr = sim(add([eq1_lhs, neg(eq1_rhs)]))
    # Extract coefficients of sin(x), cos(x), sin(x)cos(x), cos²(x), sin²(x)
    a_sin = num(0)
    b_cos = num(0)
    a_sincos = num(0)
    b_cos2 = num(0)
    c_sin2 = num(0)
    const = num(0)
    terms = list(flat(expr, "add")) if expr[0] == "add" else [expr]
    i = 0
    while i < len(terms):
        term = terms[i]
        coeff, rest = split_coeff(term)
        # Check for sin(x) term
        if rest[0] == "fn" and rest[1] == "sin" and rest[2] == sym(var):
            a_sin = sim(add([a_sin, coeff]))
        # Check for cos(x) term
        elif rest[0] == "fn" and rest[1] == "cos" and rest[2] == sym(var):
            b_cos = sim(add([b_cos, coeff]))
        # Check for sin(x)cos(x) term - handle mul with tuple of terms
        elif rest[0] == "mul" and isinstance(rest[1], tuple):
            mul_terms = rest[1]
            found_sin = False
            found_cos = False
            j = 0
            while j < len(mul_terms):
                mt = mul_terms[j]
                if mt[0] == "fn" and mt[1] == "sin" and mt[2] == sym(var):
                    found_sin = True
                elif mt[0] == "fn" and mt[1] == "cos" and mt[2] == sym(var):
                    found_cos = True
                j += 1
            if found_sin and found_cos:
                a_sincos = sim(add([a_sincos, coeff]))
            else:
                return None
        # Check for cos(x)^2 term
        elif rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] == "cos" and rest[1][2] == sym(var) and rest[2] == num(2):
            b_cos2 = sim(add([b_cos2, coeff]))
        # Check for sin(x)^2 term
        elif rest[0] == "pow" and rest[1][0] == "fn" and rest[1][1] == "sin" and rest[1][2] == sym(var) and rest[2] == num(2):
            c_sin2 = sim(add([c_sin2, coeff]))
        elif not depends(rest, var):
            const = sim(add([const, mul([coeff, rest])]))
        else:
            return None
        i += 1
    # Convert using: sin(x)cos(x) = sin(2x)/2, cos²(x) = (1+cos(2x))/2, sin²(x) = (1-cos(2x))/2
    coeff_sin2x = eval_numeric(sim(div(a_sincos, num(2))), {})
    coeff_cos2x = eval_numeric(sim(add([b_cos2, neg(c_sin2)])), {})
    const_term = eval_numeric(sim(add([b_cos2, c_sin2])), {})
    if coeff_sin2x is None or coeff_cos2x is None or const_term is None:
        return None
    lines = [
        "Start with " + equation_line(eq1_lhs, eq1_rhs),
        "Use sin(2" + var + ") = 2 sin(" + var + ") cos(" + var + "), so sin(" + var + ") cos(" + var + ") = sin(2" + var + ")/2.",
        "Use cos(2" + var + ") = 2 cos^2(" + var + ") - 1, so cos^2(" + var + ") = (1 + cos(2" + var + "))/2.",
        "Use cos(2" + var + ") = 1 - 2 sin^2(" + var + "), so sin^2(" + var + ") = (1 - cos(2" + var + "))/2.",
        "Rewrite in terms of sin(2" + var + "), cos(2" + var + ") and constants.",
        "a = " + format_float(coeff_sin2x),
        "b = " + format_float(coeff_cos2x),
        "c = " + format_float(const_term),
    ]
    return compact_lines(lines)
# ---------------------------------------------------------------------------
# Display helpers and CLI entrypoint
# ---------------------------------------------------------------------------
def parse_display_value(text):
    text = text.strip()
    if text.endswith(" rad"):
        text = text[:-4].strip()
    if text.endswith(" deg"):
        text = text[:-4].strip()
    if text == "...":
        return None
    try:
        return eval_numeric(parse(text), {})
    except (ValueError, ZeroDivisionError):
        return None


def compress_display_list(rhs, sep, neg_keep, pos_keep):
    items = rhs.split(sep)
    parsed = []
    i = 0
    while i < len(items):
        text = items[i].strip()
        value = parse_display_value(text)
        if value is None:
            return rhs
        parsed.append((value, text))
        i += 1
    if len(parsed) <= neg_keep + pos_keep + 1:
        return rhs
    negative_items = []
    zero = []
    pos = []
    i = 0
    while i < len(parsed):
        if parsed[i][0] < -1e-8:
            negative_items.append(parsed[i])
        elif parsed[i][0] > 1e-8:
            pos.append(parsed[i])
        else:
            zero.append(parsed[i])
        i += 1
    negative_items.sort(key=lambda pair: pair[0], reverse=True)
    pos.sort(key=lambda pair: pair[0])
    chosen = []
    i = 0
    while i < len(negative_items) and i < neg_keep:
        chosen.append(negative_items[i])
        i += 1
    if len(zero) != 0:
        chosen.append(zero[0])
    i = 0
    while i < len(pos) and i < pos_keep:
        chosen.append(pos[i])
        i += 1
    chosen.sort(key=lambda pair: pair[0])
    out = []
    i = 0
    while i < len(chosen):
        out.append(chosen[i][1])
        i += 1
    return sep.join(out)


def display_line_short(line):
    line = line.strip()
    replacements = [
        ("This equation is an identity - both sides are equivalent", "Identity"),
        ("All values in the interval where the original equation is defined are solutions", "All defined values work"),
        ("Use a final numeric scan of the interval when no standard rewrite route matches cleanly", "No clean route; scan numerically"),
        ("Scan f(", "Scan f("),
        (" for sign changes and verify each candidate in the original equation", " for sign changes; verify"),
        ("Use a final numeric scan over one full period when no direct R*sin/cos rewrite matches cleanly", "No clean R*sin/cos form; scan one period"),
        ("Solve the direct trig equation", "Solve trig eq"),
        ("Move all terms to one side.", "Move terms to one side"),
        ("Equation 1:", "Eq 1:"),
        ("Equation 2 in zero form:", "Eq 2 in zero form:"),
        ("Compare with ", "Match "),
        ("Compare ", "Match "),
        ("Coefficient of ", "Coeff of "),
        ("Constant term:", "Const:"),
        ("Clear the denominator.", "Clear denom"),
        ("Expand and simplify.", "Expand; simplify"),
        ("Simplify the fraction.", "Simplify frac"),
        ("Simplify the expression.", "Simplify"),
        ("Simplify into the target terms.", "Rewrite in target terms"),
        ("Use exact angle values to solve the constant equations.", "Use exact angles"),
        ("Write in terms of ", "Write using "),
        (" only.", " only."),
        ("Write the model in the form ", "Write as "),
        ("Write ", "Write "),
        ("Square and add the two equations.", "Square, then add."),
        ("Divide the second equation by the first.", "Divide the second by the first."),
        ("Use reciprocal trig forms.", "Use reciprocal trig"),
        ("Use Pythagorean identities.", "Use Pythagorean ids"),
        ("Use standard trig identities.", "Use trig ids"),
        ("Use a common denominator.", "Use a common denom"),
        ("Use common denominator.", "Use a common denom"),
        ("Use half-angle ratio identities.", "Use half-angle ratio identities."),
        ("Mixed degree and radian input is not supported in solve mode.", "Mixed degree/radian input is not supported."),
        ("This expression cannot be written using only the requested terms.", "This can't be written in the requested terms."),
        ("This expression is undefined on the interval used for the extremum search.", "This is undefined on the search interval."),
        ("No solutions because the shifted trig value lies outside [-1,1].", "No solutions: shifted trig value is outside [-1,1]."),
        ("No interval was given, so show the first 5 solutions above 0 and the first 5 below 0.", "No interval given, so show 5 above 0 and 5 below."),
        ("Solve input should be equation,var or equation,var,start,end or equation,var,0<x<pi", "Use equation,var or equation,var,start,end or equation,var,0<x<pi"),
        ("Solve mode needs input like equation,k or equation,x,0,90 or equation,x,0<x<pi", "Use equation,k or equation,x,0,90 or equation,x,0<x<pi"),
        ("Equation 2 uses a different trig variable from Equation 1, so the forms cannot match.", "Equation 2 uses a different trig variable."),
        ("Equation 2 is not directly equivalent to Equation 1, and no supported transform route matched cleanly.", "Equation 2 does not match Equation 1."),
    ]
    i = 0
    while i < len(replacements):
        old, new = replacements[i]
        if old in line:
            line = line.replace(old, new)
        i += 1
    if " so multiply through by " in line:
        line = "Multiply through by " + line.split(" so multiply through by ", 1)[1]
    elif " so divide through by " in line:
        line = "Divide through by " + line.split(" so divide through by ", 1)[1]
    if line.startswith("Use ") and ", then " in line:
        line = line.replace(", then ", "; then ", 1)

    if " = " in line:
        left, rhs = line.split(" = ", 1)
        if rhs.count(",") >= 10:
            line = left + " = " + compress_display_list(rhs, ", ", 5, 5)
        elif rhs.count(" or ") >= 10:
            line = left + " = " + compress_display_list(rhs, " or ", 5, 5)

    if line.endswith(".") and (len(line) < 2 or line[-2] < "0" or line[-2] > "9"):
        line = line[:-1]
    return line


def split_multi_identity_use_line(line):
    line = line.strip()
    if not line.startswith("Use ") or line.count("=") < 2:
        return [line]
    body = line[4:]
    if body.endswith("."):
        body = body[:-1]
    pieces = []
    current = ""
    depth = 0
    i = 0
    while i < len(body):
        ch = body[i]
        if ch == "(":
            depth += 1
        elif ch == ")" and depth > 0:
            depth -= 1
        if depth == 0 and body.startswith(" and ", i):
            if current.strip() != "":
                pieces.append(current.strip())
            current = ""
            i += 5
            continue
        if depth == 0 and ch in ",;":
            if current.strip() != "":
                pieces.append(current.strip())
            current = ""
            i += 1
            while i < len(body) and body[i] == " ":
                i += 1
            continue
        current += ch
        i += 1
    if current.strip() != "":
        pieces.append(current.strip())
    if len(pieces) <= 1:
        return [line]
    i = 0
    while i < len(pieces):
        if "=" not in pieces[i]:
            return [line]
        i += 1
    out = []
    i = 0
    while i < len(pieces):
        out.append("Use " + pieces[i] + ".")
        i += 1
    return out


def print_lines(lines):
    i = 0
    while i < len(lines):
        print(str(i + 1) + ". " + display_line_short(lines[i]))
        i += 1


def compact_lines(lines):
    out = []
    i = 0
    while i < len(lines):
        expanded = split_multi_identity_use_line(lines[i])
        j = 0
        while j < len(expanded):
            if len(out) == 0 or expanded[j] != out[-1]:
                out.append(expanded[j])
            j += 1
        i += 1
    return out


def main():
    print("1 prove")
    print("2 xform")
    print("3 solve")
    print("4 rw")
    mode = input("M: ").strip()
    if mode == "":
        mode = "1"
    try:
        if mode == "1":
            text = input("Id: ").strip()
            print("1 auto")
            print("2 lhs")
            print("3 rhs")
            print("4 both")
            route = input("R: ").strip()
            if route == "":
                route = "1"
            lines = solve_prove_text(text, route)
            print_lines(lines)
        elif mode == "2":
            eq1 = input("E1: ").strip()
            eq2 = input("E2: ").strip()
            lines = solve_transform_text(eq1, eq2)
            print_lines(lines)
        elif mode == "3":
            text = input("Eq: ").strip()
            _result, lines = solve_solve_text(text)
            print_lines(lines)
        elif mode == "4":
            text = input("Rw: ").strip()
            print("1 term/line")
            print("Blank = end")
            terms = []
            while True:
                term = input("T: ").strip()
                if term == "":
                    break
                terms.append(term)
            lines = solve_rewrite_text(text, terms)
            print_lines(lines)
        else:
            print("Bad mode.")
    except Exception as err:
        print("Err: " + str(err))


run = main
if not SKIP_AUTORUN:
    main()
