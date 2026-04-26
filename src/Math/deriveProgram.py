"""
Differentiation engine for CASIO programs.
Handles symbolic differentiation with support for:
- Basic arithmetic operations
- Trigonometric functions and their inverses
- Exponential and logarithmic functions
- Power functions
- Implicit differentiation
- Parametric differentiation
"""

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
        ensure_reasoning_marker, fn as shared_fn, is_num, is_one,
        is_zero, normalize_input_text, same_by_sig, E, PI,
    )
    from src.shared_cache import cache_store as shared_cache_store, clear_all_caches as shared_clear_all_caches
    from src.shared_reasoning_markers import REASONING_MARKERS
except ImportError:
    try:
        from shared_helpers import (
            ensure_reasoning_marker, fn as shared_fn, is_num, is_one,
            is_zero, normalize_input_text, same_by_sig, E, PI,
        )
        from shared_cache import cache_store as shared_cache_store, clear_all_caches as shared_clear_all_caches
        from shared_reasoning_markers import REASONING_MARKERS
    except ImportError:
        try:
            from shared_fallback import (
                cache_store as shared_cache_store, clear_all_caches as shared_clear_all_caches,
                ensure_reasoning_marker, fn as shared_fn, is_num, is_one,
                is_zero, normalize_input_text, same_by_sig, E, PI, REASONING_MARKERS,
            )
        except ImportError:
            shared_cache_store = lambda c, k, v, l: c.__setitem__(k, v) or v
            shared_clear_all_caches = lambda *c: None
            ensure_reasoning_marker = lambda x: x
            shared_fn = lambda *a: tuple(a)
            is_num = lambda n: n is not None and n[0] == 'num'
            is_one = lambda n: is_num(n) and n[1] == n[2]
            is_zero = lambda n: is_num(n) and n[1] == 0
            normalize_input_text = lambda t: t.strip() if isinstance(t, str) else t
            same_by_sig = lambda a, b, sig_func, cache=None, cache_store_func=None, cache_limit=None: a == b
            E = ("const", "e")
            PI = ("const", "pi")
            REASONING_MARKERS = ("method:", "use ", "using ", "let ", "solve ", "answer:")


FUNC_NAMES = (
    "sin", "cos", "tan", "sec", "cosec", "cot",
    "asin", "acos", "atan", "log", "log10", "sqrt", "exp", "abs",
    "sinh", "cosh", "tanh"
)

FUNC_ALIASES = {
    "ln": "log",
    "csc": "cosec",
    "arcsin": "asin",
    "arccos": "acos",
    "arctan": "atan"
}

MAX_NESTING_DEPTH = 200
MAX_INPUT_LENGTH = 10000
MAX_TOKEN_COUNT = 2000

SKIP_AUTORUN = sys is not None and (
    getattr(sys, "_derive_no_autorun", False) or
    len(sys.argv) > 1
)
MICROPYTHON_RUNTIME = sys is not None and getattr(getattr(sys, "implementation", None), "name", "") == "micropython"
LOW_MEMORY_RUNTIME = False

TIDY_EXPAND_LIMIT = 5
TIDY_SKIP_SIZE = 260
_CACHE_MISS = object()
_ENGINE_CACHES = {}


def apply_runtime_profile(low_memory=None):
    global LOW_MEMORY_RUNTIME, TIDY_EXPAND_LIMIT
    if low_memory is None:
        low_memory = MICROPYTHON_RUNTIME
    LOW_MEMORY_RUNTIME = bool(low_memory)
    TIDY_EXPAND_LIMIT = 3 if LOW_MEMORY_RUNTIME else 5


def begin_user_action():
    clear_engine_caches()


def clear_engine_caches():
    shared_clear_all_caches(_ENGINE_CACHES)


def _cache_limit(name):
    if LOW_MEMORY_RUNTIME:
        limits = {
            "sim": 320,
            "depends": 384,
            "show": 512,
            "format_final_answer": 96,
        }
    else:
        limits = {
            "sim": 1536,
            "depends": 2048,
            "show": 2048,
            "format_final_answer": 256,
        }
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
    return shared_cache_store(bucket, key, value, _cache_limit(name))


def _force_low_memory_runtime(flag):
    apply_runtime_profile(flag)


apply_runtime_profile()



def is_digit_char(ch):
    return "0" <= ch <= "9"


def is_name_start(ch):
    return ("A" <= ch <= "Z") or ("a" <= ch <= "z") or ch == "_"


def is_name_char(ch):
    return is_name_start(ch) or is_digit_char(ch)


def is_num_token_start(text, i):
    ch = text[i]
    if is_digit_char(ch):
        return True
    return ch == "." and i + 1 < len(text) and is_digit_char(text[i + 1])


def is_valid_symbol_name(text):
    if text == "" or not is_name_start(text[0]):
        return False
    i = 1
    while i < len(text):
        if not is_name_char(text[i]):
            return False
        i += 1
    return True



def gcd(a, b):
    if math is not None and hasattr(math, "gcd"):
        return math.gcd(a, b) or 1
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
    text = text.strip()
    if text == "" or text in (".", "+.", "-."):
        raise ValueError("Invalid number format.")
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


def is_const(x):
    return x[0] == "const"


def is_minus_one(x):
    return is_num(x) and x[1] == -x[2]


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


def is_int_num(x):
    return is_num(x) and x[2] == 1


def int_pow(a, n):
    if n == 0:
        return num(1)
    if n < 0:
        return divq(num(1), int_pow(a, -n))
    out = num(1)
    while n:
        out = mulq(out, a)
        n -= 1
    return out



def add(parts):
    return sim(("add", tuple(parts)))


def sub(a, b):
    return sim(("add", (a, neg(b))))


def mul(parts):
    return sim(("mul", tuple(parts)))


def div(a, b):
    return sim(("div", a, b))


def power(a, b):
    return sim(("pow", a, b))


def fn(name, arg):
    return shared_fn(name, arg, sim)


def neg(x):
    if x[0] == "add":
        return add([neg(item) for item in flat(x, "add")])
    return mul([num(-1), x])



def flat(node, kind):
    if node[0] != kind:
        return [node]
    if len(node[1]) == 0:
        return []
    first = node[1][0]
    if first[0] != kind:
        return list(node[1])
    out = []
    for item in node[1]:
        if item[0] == kind:
            out.extend(flat(item, kind))
        else:
            out.append(item)
    return out


def sig(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return (kind, node[1])
    if kind == "fn":
        return ("fn", node[1], sig(node[2]))
    if kind == "pow":
        return ("pow", sig(node[1]), sig(node[2]))
    if kind == "div":
        return ("div", sig(node[1]), sig(node[2]))
    parts = []
    for item in flat(node, kind):
        parts.append(sig(item))
    parts.sort()
    return (kind, tuple(parts))


def same(a, b):
    return same_by_sig(a, b, sig)


def tree_size(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return 1
    if kind == "fn":
        return 1 + tree_size(node[2])
    if kind in ("pow", "div"):
        return 1 + tree_size(node[1]) + tree_size(node[2])
    total = 1
    for item in flat(node, kind):
        total += tree_size(item)
    return total


def depends(node, names):
    kind = node[0]
    if kind == "sym":
        return node[1] in names
    if kind in ("num", "const"):
        return False
    if kind == "fn":
        return depends(node[2], names)
    if kind in ("pow", "div"):
        return depends(node[1], names) or depends(node[2], names)
    for item in node[1]:
        if depends(item, names):
            return True
    return False


def sort_key(node):
    kind = node[0]
    order = {"num": 0, "sym": 1, "const": 2, "fn": 3, "pow": 4, "div": 5, "mul": 6, "add": 7}
    return str(order.get(kind, 9)) + ":" + show(node)


def split_coeff(node):
    if is_num(node):
        return node, num(1)
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        if items and is_num(items[0]):
            if len(items) == 2:
                return items[0], items[1]
            return items[0], ("mul", tuple(items[1:]))
    return num(1), node


def factor_map(node):
    if node[0] == "mul":
        items = list(flat(node, "mul"))
    else:
        items = [node]
    coeff = num(1)
    order = []
    data = {}
    for item in items:
        item = norm_pow_base(item)
        if is_num(item):
            coeff = mulq(coeff, item)
        elif item[0] == "pow":
            key = sig(item[1])
            if key not in data:
                order.append(key)
                data[key] = [item[1], num(0)]
            data[key][1] = sim(add([data[key][1], item[2]]))
        else:
            key = sig(item)
            if key not in data:
                order.append(key)
                data[key] = [item, num(0)]
            data[key][1] = sim(add([data[key][1], num(1)]))
    return coeff, order, data



def single_prime_power(n):
    if n <= 1:
        return None
    p = 2
    while p * p <= n:
        if n % p == 0:
            k = 0
            while n % p == 0:
                n //= p
                k += 1
            if n == 1:
                return p, k
            return None
        p += 1
    return n, 1


def norm_pow_base(node):
    if node[0] != "pow" or not is_num(node[1]):
        return node
    base = node[1]
    exp = node[2]
    if base[1] <= 0:
        return node
    if base[2] == 1:
        info = single_prime_power(base[1])
        if info is not None and info[1] > 1:
            return ("pow", num(info[0]), sim(mul([num(info[1]), exp])))
        return node
    if base[1] == 1:
        info = single_prime_power(base[2])
        if info is not None:
            return ("pow", num(info[0]), sim(neg(mul([num(info[1]), exp]))))
    return node


def make_mul(parts):
    out = []
    for item in parts:
        if item[0] == "mul":
            out.extend(flat(item, "mul"))
        else:
            out.append(item)
    if len(out) == 0:
        return num(1)
    if len(out) == 1:
        return out[0]
    return ("mul", tuple(out))


def factor_add_num(node):
    if node[0] != "add":
        return num(1), node
    bits = flat(node, "add")
    g = 0
    data = []
    for item in bits:
        c, rest = split_coeff(item)
        if not is_int_num(c):
            return num(1), node
        n = c[1]
        if n < 0:
            n = -n
        if g == 0:
            g = n
        else:
            g = gcd(g, n)
        data.append((c, rest))
    if g <= 1:
        return num(1), node
    out = []
    for c, rest in data:
        k = num(c[1] // g)
        if is_one(rest):
            out.append(k)
        elif is_one(k):
            out.append(rest)
        elif is_minus_one(k):
            out.append(neg(rest))
        else:
            out.append(mul([k, rest]))
    return num(g), add(out)


def all_neg_add(node):
    if node[0] != "add":
        return False
    for item in flat(node, "add"):
        c, rest = split_coeff(item)
        if c[1] >= 0 or is_zero(c):
            return False
    return True


def flip_add(node):
    out = []
    for item in flat(node, "add"):
        out.append(neg(item))
    return sim(("add", tuple(out)))


def rem_match(items, target):
    out = []
    done = False
    for item in items:
        if not done and same(item, target):
            done = True
        else:
            out.append(item)
    return done, out


def subst(node, name, value):
    kind = node[0]
    if kind == "sym":
        if node[1] == name:
            return value
        return node
    if kind in ("num", "const"):
        return node
    if kind == "fn":
        return ("fn", node[1], subst(node[2], name, value))
    if kind == "pow":
        return ("pow", subst(node[1], name, value), subst(node[2], name, value))
    if kind == "div":
        return ("div", subst(node[1], name, value), subst(node[2], name, value))
    out = []
    for item in node[1]:
        out.append(subst(item, name, value))
    return (kind, tuple(out))



def rule_u(name):
    u = sym("u")
    if name == "exp":
        return fn("exp", u)
    if name == "sin":
        return fn("cos", u)
    if name == "cos":
        return neg(fn("sin", u))
    if name == "tan":
        return power(fn("sec", u), num(2))
    if name == "sinh":
        return fn("cosh", u)
    if name == "cosh":
        return fn("sinh", u)
    if name == "tanh":
        return div(num(1), power(fn("cosh", u), num(2)))
    if name == "sec":
        return mul([fn("sec", u), fn("tan", u)])
    if name == "cosec":
        return neg(mul([fn("cosec", u), fn("cot", u)]))
    if name == "cot":
        return neg(power(fn("cosec", u), num(2)))
    if name == "asin":
        return div(num(1), fn("sqrt", add([num(1), neg(power(u, num(2)))])))
    if name == "acos":
        return neg(div(num(1), fn("sqrt", add([num(1), neg(power(u, num(2)))]))))
    if name == "atan":
        return div(num(1), add([num(1), power(u, num(2))]))
    if name == "log":
        return div(num(1), u)
    if name == "log10":
        return div(num(1), mul([u, fn("log", num(10))]))
    if name == "sqrt":
        return div(num(1), mul([num(2), fn("sqrt", u)]))
    if name == "abs":
        return div(u, fn("abs", u))
    raise ValueError("Unsupported function: " + name)



def sim(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        arg = sim(node[2])
        if node[1] == "log" and same(arg, E):
            return num(1)
        return ("fn", node[1], arg)
    if kind == "pow":
        base = sim(node[1])
        exp = sim(node[2])
        if is_num(exp):
            if is_zero(exp):
                return num(1)
            if is_one(exp):
                return base
            if is_num(base) and is_int_num(exp):
                return int_pow(base, exp[1])
            if base[0] == "div" and is_int_num(exp):
                if exp[1] > 0:
                    return sim(("div", ("pow", base[1], exp), ("pow", base[2], exp)))
                return sim(("div", ("pow", base[2], num(-exp[1])), ("pow", base[1], num(-exp[1]))))
            if base[0] == "mul" and is_int_num(exp) and exp[1] > 0:
                items = flat(base, "mul")
                powered = []
                for item in items:
                    powered.append(sim(("pow", item, exp)))
                return sim(("mul", tuple(powered)))
            if base[0] == "fn" and base[1] == "exp" and is_num(exp):
                new_exp = sim(("mul", (base[2], exp)))
                return ("fn", "exp", new_exp)
        if is_one(base):
            return num(1)
        if base[0] == "pow" and is_num(exp):
            if is_num(base[2]):
                return sim(("pow", base[1], mulq(base[2], exp)))
            return sim(("pow", base[1], sim(("mul", (base[2], exp)))))
        return ("pow", base, exp)
    if kind == "add":
        items = []
        for item in node[1]:
            item = sim(item)
            if item[0] == "add":
                items.extend(flat(item, "add"))
            else:
                items.append(item)
        total = num(0)
        order = []
        data = {}
        for item in items:
            c, rest = split_coeff(item)
            if is_one(rest):
                total = addq(total, c)
            else:
                key = sig(rest)
                if key not in data:
                    order.append(key)
                    data[key] = [rest, num(0)]
                data[key][1] = addq(data[key][1], c)
        out = []
        for key in order:
            rest, c = data[key]
            if is_zero(c):
                continue
            if is_one(c):
                out.append(rest)
            elif is_minus_one(c):
                out.append(make_mul([num(-1), rest]))
            else:
                out.append(make_mul([c, rest]))
        if not is_zero(total) or len(out) == 0:
            if out and total[1] > 0 and neg_term(out[0]):
                out.insert(0, total)
            else:
                out.append(total)
        if len(out) == 0:
            return num(0)
        if len(out) == 1:
            return out[0]
        if len(out) == 2:
            a = out[0]
            b = out[1]
            if a[0] == "div" and b[0] == "div":
                return sim(("div", add([mul([a[1], b[2]]), mul([b[1], a[2]])]), mul([a[2], b[2]])))
            if a[0] == "div":
                return sim(("div", add([a[1], mul([b, a[2]])]), a[2]))
            if b[0] == "div":
                return sim(("div", add([mul([a, b[2]]), b[1]]), b[2]))
        return ("add", tuple(out))
    if kind == "mul":
        items = []
        for item in node[1]:
            item = sim(item)
            if item[0] == "mul":
                items.extend(flat(item, "mul"))
            else:
                items.append(item)
        has_div = False
        for item in items:
            if item[0] == "div":
                has_div = True
                break
        if has_div:
            top_bits = []
            bot_bits = []
            for item in items:
                if item[0] == "div":
                    top_bits.append(item[1])
                    bot_bits.append(item[2])
                else:
                    top_bits.append(item)
            return sim(("div", make_mul(top_bits), make_mul(bot_bits)))
        coeff = num(1)
        order = []
        data = {}
        for item in items:
            item = norm_pow_base(item)
            if is_zero(item):
                return num(0)
            if is_num(item):
                coeff = mulq(coeff, item)
            elif item[0] == "pow":
                key = sig(item[1])
                if key not in data:
                    order.append(key)
                    data[key] = [item[1], num(0)]
                data[key][1] = sim(add([data[key][1], item[2]]))
            else:
                key = sig(item)
                if key not in data:
                    order.append(key)
                    data[key] = [item, num(0)]
                data[key][1] = sim(add([data[key][1], num(1)]))
        out = []
        if is_zero(coeff):
            return num(0)
        if not is_one(coeff):
            out.append(coeff)
        rest = []
        for key in order:
            base, exp = data[key]
            if is_zero(exp):
                continue
            if is_one(exp):
                rest.append(base)
            else:
                rest.append(("pow", base, exp))
        if is_minus_one(coeff) and len(rest) == 1 and rest[0][0] == "add":
            return neg(rest[0])
        out.extend(rest)
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
            return num(1)
        if all_neg_add(bot):
            return sim(("div", neg(top), flip_add(bot)))
        if top[0] == "add" and bot[0] == "pow" and is_num(bot[1]):
            out = []
            for item in flat(top, "add"):
                out.append(("div", item, bot))
            return sim(("add", tuple(out)))
        cbot, restbot = factor_add_num(bot)
        if not is_one(cbot):
            return sim(("div", top, make_mul([cbot, restbot])))
        if top[0] == "div":
            return sim(("div", top[1], make_mul([top[2], bot])))
        if bot[0] == "div":
            return sim(("div", make_mul([top, bot[2]]), bot[1]))
        if is_num(top) and is_num(bot):
            return divq(top, bot)
        top_items = list(flat(top, "mul"))
        bot_items = list(flat(bot, "mul"))
        i = 0
        while i < len(bot_items):
            item = bot_items[i]
            if item[0] == "fn" and item[1] == "sqrt":
                found, rest_top = rem_match(top_items, item[2])
                if found:
                    rest_bot = bot_items[:i] + bot_items[i + 1 :]
                    return sim(("div", make_mul(rest_top + [item]), make_mul(rest_bot)))
            i += 1
        c1, o1, d1 = factor_map(top)
        c2, o2, d2 = factor_map(bot)
        c = divq(c1, c2)
        num_top = c
        num_bot = num(1)
        if c[2] != 1:
            num_top = num(c[1], 1)
            num_bot = num(c[2], 1)
        for key in d1:
            if key in d2:
                d1[key][1] = sim(add([d1[key][1], neg(d2[key][1])]))
                d2[key][1] = num(0)
        top_parts = []
        bot_parts = []
        if not is_one(num_top):
            top_parts.append(num_top)
        if not is_one(num_bot):
            bot_parts.append(num_bot)
        for key in o1:
            base, exp = d1[key]
            if is_zero(exp):
                continue
            if is_one(exp):
                top_parts.append(base)
            else:
                top_parts.append(("pow", base, exp))
        for key in o2:
            base, exp = d2[key]
            if is_zero(exp):
                continue
            if is_one(exp):
                bot_parts.append(base)
            else:
                bot_parts.append(("pow", base, exp))
        top = make_mul(top_parts)
        bot = make_mul(bot_parts)
        if is_one(bot):
            return top
        return ("div", top, bot)
    return node



def diff(node, var, deps_list):
    deps_list_list = list(deps_list.keys()) if isinstance(deps_list, dict) else []
    kind = node[0]
    if kind == "num" or kind == "const":
        return num(0)
    if kind == "sym":
        if node[1] == var:
            return num(1)
        if node[1] in deps_list:
            return sym("d" + node[1] + "/d" + var)
        return num(0)
    if kind == "add":
        out = []
        for item in node[1]:
            out.append(diff(item, var, deps_list))
        return add(out)
    if kind == "mul":
        items = list(node[1])
        n = len(items)
        if n == 2:
            a, b = items[0], items[1]
            da = diff(a, var, deps_list)
            db = diff(b, var, deps_list)
            return add([mul([a, db]), mul([b, da])])
        out = []
        i = 0
        while i < n:
            bit = []
            j = 0
            while j < n:
                if i == j:
                    bit.append(diff(items[j], var, deps_list))
                else:
                    bit.append(items[j])
                j += 1
            out.append(mul(bit))
            i += 1
        return add(out)
    if kind == "div":
        u = node[1]
        v = node[2]
        du = diff(u, var, deps_list)
        dv = diff(v, var, deps_list)
        return div(add([mul([v, du]), neg(mul([u, dv]))]), power(v, num(2)))
    if kind == "pow":
        base = node[1]
        exp = node[2]
        if is_num(exp):
            return mul([exp, power(base, subq(exp, num(1))), diff(base, var, deps_list)])
        if is_num(base) and base[1] <= 0:
            raise ValueError("Exponential base must be positive.")
        if not depends(base, [var] + deps_list_list):
            return mul([power(base, exp), fn("log", base), diff(exp, var, deps_list)])
        if not depends(exp, [var] + deps_list_list):
            if is_num(exp):
                return mul([exp, power(base, subq(exp, num(1))), diff(base, var, deps_list)])
            return mul([exp, power(base, sub(exp, num(1))), diff(base, var, deps_list)])
        return mul([power(base, exp), add([mul([exp, div(diff(base, var, deps_list), base)]), mul([fn("log", base), diff(exp, var, deps_list)])])])
    if kind == "fn":
        name = node[1]
        arg = node[2]
        darg = diff(arg, var, deps_list)
        if name == "exp":
            return mul([fn("exp", arg), darg])
        if name == "sin":
            return mul([fn("cos", arg), darg])
        if name == "cos":
            return neg(mul([fn("sin", arg), darg]))
        if name == "tan":
            return mul([power(fn("sec", arg), num(2)), darg])
        if name == "sinh":
            return mul([fn("cosh", arg), darg])
        if name == "cosh":
            return mul([fn("sinh", arg), darg])
        if name == "tanh":
            return div(darg, power(fn("cosh", arg), num(2)))
        if name == "sec":
            return mul([fn("sec", arg), fn("tan", arg), darg])
        if name == "cosec":
            return neg(mul([fn("cosec", arg), fn("cot", arg), darg]))
        if name == "cot":
            return neg(mul([power(fn("cosec", arg), num(2)), darg]))
        if name == "asin":
            return div(darg, fn("sqrt", add([num(1), neg(power(arg, num(2)))])))
        if name == "acos":
            return neg(div(darg, fn("sqrt", add([num(1), neg(power(arg, num(2)))]))))
        if name == "atan":
            return div(darg, add([num(1), power(arg, num(2))]))
        if name == "log":
            return div(darg, arg)
        if name == "log10":
            return div(darg, mul([arg, fn("log", num(10))]))
        if name == "sqrt":
            return div(darg, mul([num(2), fn("sqrt", arg)]))
        if name == "abs":
            return div(mul([arg, darg]), fn("abs", arg))
        raise ValueError("Unsupported function: " + name)
    raise ValueError("Unsupported expression.")


def coeff_d(node, dname):
    node = sim(node)
    kind = node[0]
    if kind == "sym":
        if node[1] == dname:
            return num(1), num(0)
        return num(0), node
    if kind in ("num", "const"):
        return num(0), node
    if kind == "add":
        c = num(0)
        r = num(0)
        for item in node[1]:
            a, b = coeff_d(item, dname)
            c = add([c, a])
            r = add([r, b])
        return sim(c), sim(r)
    if kind == "mul":
        coef = None
        rest_all = []
        rest_others = []
        hits = 0
        for item in node[1]:
            a, b = coeff_d(item, dname)
            rest_all.append(b)
            if not is_zero(a):
                hits += 1
                coef = a
            else:
                rest_others.append(b)
        if hits == 0:
            return num(0), node
        if hits > 1:
            raise ValueError("Equation is not linear in dy/dx.")
        return sim(mul([coef, mul(rest_others)])), sim(mul(rest_all))
    if kind == "div":
        a1, b1 = coeff_d(node[1], dname)
        a2, b2 = coeff_d(node[2], dname)
        if not is_zero(a2):
            raise ValueError("Equation is not linear in dy/dx.")
        return div(a1, b2), div(b1, b2)
    if kind == "pow" or kind == "fn":
        if depends(node, [dname]):
            raise ValueError("Equation is not linear in dy/dx.")
        return num(0), node
    return num(0), node



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


def simple(node):
    return node[0] in ("num", "sym", "const", "fn")


def abs_term(node):
    c, rest = split_coeff(node)
    if c[1] < 0:
        if is_one(rest):
            return num(-c[1], c[2])
        if is_minus_one(c):
            return rest
        return mul([num(-c[1], c[2]), rest])
    return node


def neg_term(node):
    c, rest = split_coeff(node)
    return c[1] < 0 and not is_zero(c)


def show(node, parent=0):
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
        if node[1] == "log" and node[2][0] == "fn" and node[2][1] == "abs":
            text = "ln|" + show(node[2][2], 0) + "|"
        elif node[1] == "log":
            text = "ln(" + show(node[2], 0) + ")"
        else:
            text = node[1] + "(" + show(node[2], 0) + ")"
    elif kind == "pow":
        if is_num(node[2]) and node[2][1] == 1 and node[2][2] == 2:
            text = "sqrt(" + show(node[1], 0) + ")"
        elif is_num(node[2]) and node[2][1] == -1 and node[2][2] == 2:
            text = "1/sqrt(" + show(node[1], 0) + ")"
        else:
            left = show(node[1], pr(node))
            right = show(node[2], pr(node))
            if node[1][0] == "fn" or (is_num(node[1]) and (node[1][2] != 1 or node[1][1] < 0)):
                left = "(" + show(node[1], 0) + ")"
            if not simple(node[2]) or (is_num(node[2]) and node[2][2] != 1):
                right = "(" + show(node[2], 0) + ")"
            text = left + "^" + right
    elif kind == "mul":
        items = list(flat(node, "mul"))
        if items and is_minus_one(items[0]):
            rest = make_mul(items[1:])
            body = show(rest, pr(node))
            if pr(rest) < pr(node):
                body = "(" + show(rest, 0) + ")"
            text = "-" + body
        elif items and is_num(items[0]) and items[0][1] < 0:
            head = num(-items[0][1], items[0][2])
            rest = [head] + items[1:]
            body = show(make_mul(rest), pr(node))
            text = "-" + body
        else:
            parts = []
            for item in items:
                part = show(item, pr(node))
                if pr(item) < pr(node) or item[0] == "div" or (is_num(item) and item[2] != 1):
                    part = "(" + show(item, 0) + ")"
                parts.append(part)
            text = "*".join(parts)
    elif kind == "div":
        left = show(node[1], pr(node))
        right = show(node[2], pr(node))
        if pr(node[1]) < pr(node):
            left = "(" + show(node[1], 0) + ")"
        if pr(node[2]) <= pr(node):
            right = "(" + show(node[2], 0) + ")"
        text = left + "/" + right
    else:
        bits = list(flat(node, "add"))
        text = ""
        i = 0
        while i < len(bits):
            item = bits[i]
            if i == 0:
                text = show(abs_term(item), pr(node))
                if neg_term(item):
                    text = "-" + text
            else:
                if neg_term(item):
                    text += " - " + show(abs_term(item), pr(node))
                else:
                    text += " + " + show(item, pr(node))
            i += 1
    if pr(node) < parent:
        return "(" + text + ")"
    return text


def format_equation_human_readable(node, parent=0):
    """
    Format an equation node into a human-readable string with clear operator precedence.
    This function provides consistent formatting across all programs.
    """
    kind = node[0]
    
    if kind == 'num':
        # Format fractions clearly
        if node[2] == 1:
            return str(node[1])
        return '(' + str(node[1]) + '/' + str(node[2]) + ')'
    
    elif kind == 'sym':
        return node[1]
    
    elif kind == 'const':
        return node[1]
    
    elif kind == 'fn':
        # Handle special functions
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
        
        # Add parentheses for complex bases
        if node[1][0] in ('add', 'mul', 'div') or (node[1][0] == 'num' and node[1][2] != 1):
            base = '(' + base + ')'
        
        # Add parentheses for non-integer exponents or complex expressions
        if node[2][0] not in ('num',) or node[2][2] != 1:
            exponent = '(' + exponent + ')'
        
        return base + '^' + exponent
    
    elif kind == 'mul':
        # Handle multiplication with proper spacing
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for item in items:
            part = format_equation_human_readable(item, 2)
            # Add parentheses for additions/subtractions in multiplication
            if item[0] == 'add':
                part = '(' + part + ')'
            parts.append(part)
        
        return '*'.join(parts)
    
    elif kind == 'div':
        # Handle division with clear numerator/denominator
        numerator = format_equation_human_readable(node[1], 2)
        denominator = format_equation_human_readable(node[2], 2)
        
        # Add parentheses for complex numerator or denominator
        if node[1][0] in ('add', 'mul'):
            numerator = '(' + numerator + ')'
        if node[2][0] in ('add', 'mul'):
            denominator = '(' + denominator + ')'
        
        return numerator + '/' + denominator
    
    elif kind == 'add':
        # Handle addition with proper term separation
        items = node[1] if hasattr(node, '__iter__') and len(node) > 1 else [node]
        parts = []
        
        for i, item in enumerate(items):
            coeff, rest = split_coeff(item) if hasattr(item, '__iter__') else (item, None)
            
            # Format the term
            if rest is None:
                term = format_equation_human_readable(item, 1)
            else:
                term = format_equation_human_readable(rest, 1)
                # Add coefficient if not 1
                if not (coeff[0] == 'num' and coeff[1] == 1 and coeff[2] == 1):
                    coeff_str = format_equation_human_readable(coeff, 1)
                    term = coeff_str + '*' + term
            
            parts.append(term)
        
        # Join with +, handling negative terms
        result = ' + '.join(parts)
        return result
    
    return str(node)

def split_coeff(node):
    """Helper function to split coefficient from term."""
    if is_num(node):
        return node, num(1)
    if node[0] == "mul":
        items = list(flat(node, "mul"))
        if items and is_num(items[0]):
            if len(items) == 2:
                return items[0], items[1]
            return items[0], ("mul", tuple(items[1:]))
    return num(1), node



KNOWN_PARSE_NAMES = tuple(
    sorted(
        set(FUNC_NAMES) |
        set(FUNC_ALIASES.keys()) |
        {"e", "pi", "ln", "csc"},
        key=len,
        reverse=True))


def normalize_name_token(token):
    low = token.lower()
    if low in FUNC_ALIASES:
        return FUNC_ALIASES[low]
    if low == "ln":
        return "log"
    if low == "csc":
        return "cosec"
    if low in FUNC_NAMES or low in ("e", "pi"):
        return low
    return token


def decompose_name_word(word, next_char):
    low = word.lower()
    known = low in FUNC_NAMES or low in ("e", "pi", "ln", "csc") or low in FUNC_ALIASES
    has_digit = False
    k = 0
    while k < len(word):
        if is_digit_char(word[k]) or word[k] == "_":
            has_digit = True
            break
        k += 1
    if known or len(word) == 1 or has_digit:
        return [normalize_name_token(word)]
    if next_char == "(":
        for token in KNOWN_PARSE_NAMES:
            if low.endswith(token) and len(word) > len(token):
                prefix = word[:len(word) - len(token)]
                if prefix != "":
                    return list(prefix) + [normalize_name_token(token)]
    if len(word) == 2:
        return [word[0], word[1]]
    raise ValueError(
        "Unsupported multi-letter name: " +
        word +
        ". Use single-letter variables.")


def split_at_equals(text):
    """Split equation text at top-level equals, handling parentheses."""
    text = text.strip()
    depth = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "=" and depth == 0:
            return text[:i].strip(), text[i + 1:].strip()
        i += 1
    
    # Handle case like "(x+y=1)" - strip outer parens if balanced
    if text.startswith("(") and text.endswith(")"):
        inner = text[1:-1]
        if inner.count("(") == inner.count(")") and "=" in inner:
            return split_at_equals(inner)
    
    return None, text


def split_explicit_var(text):
    body = text.strip()
    if body[:1] == "(" and body[-1:] == ")":
        body = body[1:-1].strip()
    depth = 0
    split_at = -1
    i = 0
    while i < len(body):
        ch = body[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            split_at = i
            break
        i += 1
    if split_at == -1:
        return None
    expr = body[:split_at].strip()
    var = body[split_at + 1:].strip()
    if expr == "" or var == "":
        raise ValueError("Use expr,var with a single-letter variable.")
    return expr, var


def normalize_explicit_var(name):
    var = name.strip()
    if not is_valid_symbol_name(var) or len(var) != 1:
        raise ValueError("Use a single-letter variable after the comma.")
    return var


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
            word = text[i:j]
            next_char = text[j] if j < len(text) else ""
            toks.extend(decompose_name_word(word, next_char))
            i = j
        else:
            raise ValueError("Unexpected character: " + ch)

    p = 0

    def cur():
        if p >= len(toks):
            return ""
        return toks[p]

    def eat(x):
        nonlocal p
        if cur() != x:
            raise ValueError("Expected '" + x + "' but found '" + cur() + "'.")
        p += 1

    def is_atom_start(t):
        if t == "" or t in (")", "+", "-", "*", "/", "**", "=", ","):
            return False
        if t == "(":
            return True
        if is_digit_char(t[0]) or t[0] == ".":
            return True
        if is_name_start(t[0]):
            return True
        return False

    def starts_implicit(t):
        return is_atom_start(t)

    def atom():
        nonlocal p
        t = cur()
        if t == "(":
            eat("(")
            out = expr()
            eat(")")
            return out
        if t and (is_digit_char(t[0]) or t[0] == "."):
            p += 1
            return num_text(t)
        if t == "e":
            p += 1
            return E
        if t == "pi":
            p += 1
            return PI
        if t:
            p += 1
            if t in FUNC_ALIASES:
                t = FUNC_ALIASES[t]
            elif t == "ln":
                t = "log"
            elif t == "csc":
                t = "cosec"
            if t in FUNC_NAMES:
                if cur() == "**":
                    eat("**")
                    if cur() == "(":
                        eat("(")
                        exp = expr()
                        eat(")")
                    else:
                        exp = unary()
                    out = atom()
                    return power(fn(t, out), exp)
                if cur() == "(":
                    eat("(")
                    if t == "log" and cur() != ")":
                        arg1 = expr()
                        if cur() == ",":
                            eat(",")
                            arg2 = expr()
                            eat(")")
                            return div(fn("log", arg2), fn("log", arg1))
                        else:
                            eat(")")
                            return fn(t, arg1)
                    else:
                        out = expr()
                        eat(")")
                        return fn(t, out)
                if starts_implicit(cur()):
                    return fn(t, atom())
            return sym(t)
        raise ValueError("Unexpected end.")

    def power_part():
        left = atom()
        if cur() == "**":
            eat("**")
            return power(left, unary())
        return left

    def unary():
        if cur() == "-":
            eat("-")
            return neg(unary())
        return power_part()

    def term():
        out = unary()
        while True:
            if cur() == "*":
                eat("*")
                out = mul([out, unary()])
            elif cur() == "/":
                eat("/")
                out = div(out, unary())
            elif starts_implicit(cur()):
                out = mul([out, unary()])
            else:
                break
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

    try:
        out = expr()
        if cur():
            raise ValueError("Unexpected token: " + cur())
        return sim(out)
    except RecursionError:
        raise ValueError("Expression too nested (simplify before solving).")


def parse_normal_input(text):
    text = text.strip()
    left_text, right_text = split_at_equals(text)
    if left_text is not None:
        left_side = left_text.strip()
        right_side = right_text.strip()
        if left_side in ("y", "EQ1") or left_side.startswith("f("):
            return parse(right_side), "x"
        else:
            parts = split_explicit_var(text)
            if parts is not None:
                expr_text, var_text = parts
                var = normalize_explicit_var(var_text)
                return parse(expr_text), var
            raise ValueError("For equations, use implicit mode (mode 2). Or for EQ1 = f(x), put just f(x) on right side.")
    parts = split_explicit_var(text)
    if parts is not None:
        expr_text, var_text = parts
        var = normalize_explicit_var(var_text)
        return parse(expr_text), var
    return parse(text), "x"


def parse_second_derivative_input(text):
    raw = text.strip()
    if raw == "":
        return None
    left = None
    expr_text = None
    if "=" in raw:
        parts = raw.split("=", 1)
        left = parts[0].strip()
        expr_text = parts[1].strip()
    elif ":" in raw:
        parts = raw.split(":", 1)
        left = parts[0].strip()
        expr_text = parts[1].strip()
    else:
        bits = raw.split(None, 1)
        if len(bits) == 2:
            left = bits[0].strip()
            expr_text = bits[1].strip()
    if left is None or expr_text is None or expr_text == "":
        return None
    head = left.replace(chr(178), "2").replace("^", "").replace(" ", "")
    if head.startswith("d2/d") and head.endswith("2"):
        var = head[4:-1]
    elif head.startswith("d2y/d") and head.endswith("2"):
        var = head[5:-1]
    elif head.startswith("d2f/d") and head.endswith("2"):
        var = head[5:-1]
    else:
        return None
    if not is_valid_symbol_name(var) or len(var) != 1:
        raise ValueError("Use a single-letter variable in d2/dx2.")
    return parse(expr_text), var



def reciprocal_trig_quotient_steps(name, arg, var, darg, final):
    lines = ["Method: Convert to sin/cos"]
    if name == "sec":
        top = num(1)
        bot = fn("cos", arg)
        dtop = num(0)
        dbot = neg(fn("sin", arg))
        raw = div(fn("sin", arg), power(fn("cos", arg), num(2)))
        lines.append("sec(" + show(arg) + ") = 1/cos(" + show(arg) + ")")
    elif name == "cosec":
        top = num(1)
        bot = fn("sin", arg)
        dtop = num(0)
        dbot = fn("cos", arg)
        raw = neg(div(fn("cos", arg), power(fn("sin", arg), num(2))))
        lines.append("cosec(" + show(arg) + ") = 1/sin(" + show(arg) + ")")
    else:
        top = fn("cos", arg)
        bot = fn("sin", arg)
        dtop = neg(fn("sin", arg))
        dbot = fn("cos", arg)
        raw = neg(div(add([power(fn("sin", arg), num(2)), power(fn("cos", arg), num(2))]), power(fn("sin", arg), num(2))))
        lines.append("cot(" + show(arg) + ") = cos(" + show(arg) + ")/sin(" + show(arg) + ")")
    lines.append("Use quotient rule: d/d" + var + "[u/v] = (u'v - uv')/v^2")
    lines.append("u = " + show(top) + ", v = " + show(bot))
    lines.append("u' = " + show(dtop) + ", v' = " + show(dbot))
    lines.append("dy/d" + var + " = (" + show(dtop) + "*" + show(bot) + " - " + show(top) + "*" + show(dbot) + ")/" + show(power(bot, num(2))))
    lines.append("= " + show(raw))
    if not is_one(darg):
        lines.append("Chain rule: multiply by d/d" + var + "[" + show(arg) + "] = " + show(darg))
    lines.append("= " + show(final))
    return lines


def explain(node, var, deps_list):
    d = tidy(diff(node, var, deps_list))
    lines = ['Method: Differentiate with respect to ' + var]
    if node[0] == "add":
        lines.append("Term by term differentiation")
        lines.append("dy/d" + var + " = " + show(d))
        return d, lines
    if node[0] == "mul":
        items = list(node[1])
        u = items[0]
        v = make_mul(items[1:])
        du = tidy(diff(u, var, deps_list))
        dv = tidy(diff(v, var, deps_list))
        lines.append("Using product rule")
        lines.append("Let u = " + show(u) + ", v = " + show(v))
        lines.append("du/d" + var + " = " + show(du) + ", dv/d" + var + " = " + show(dv))
        lines.append("dy/d" + var + " = u*(dv/d" + var + ") + v*(du/d" + var + ")")
        lines.append("= " + show(tidy(add([mul([u, dv]), mul([v, du])]))))
        return d, lines
    if node[0] == "div":
        u = node[1]
        v = node[2]
        du = tidy(diff(u, var, deps_list))
        dv = tidy(diff(v, var, deps_list))
        lines.append("Using quotient rule")
        lines.append("Let u = " + show(u) + ", v = " + show(v))
        lines.append("du/d" + var + " = " + show(du) + ", dv/d" + var + " = " + show(dv))
        lines.append("dy/d" + var + " = (v*du-u*dv)/v^2")
        lines.append("= " + show(tidy(div(add([mul([v, du]), neg(mul([u, dv]))]), power(v, num(2))))))
        lines.append("= " + show(d))
        return d, lines
    if node[0] == "pow":
        base = node[1]
        exp = node[2]
        if is_num(exp):
            db = tidy(diff(base, var, deps_list))
            if same(base, sym(var)):
                lines.append("Using power rule")
                lines.append("dy/d" + var + " = " + show(d))
            else:
                lines.append("Using chain rule")
                lines.append("Let u = " + show(base))
                lines.append("dy/d" + var + " = " + show(exp) + "*u^(" + show(subq(exp, num(1))) + ")*du/d" + var)
                lines.append("= " + show(d))
            return d, lines
        if is_num(base) and base[1] <= 0:
            raise ValueError("Exponential base must be positive.")
        deps_list_list = list(deps_list.keys()) if isinstance(deps_list, dict) else []
        if not depends(base, [var] + deps_list_list):
            de = tidy(diff(exp, var, deps_list))
            if same(base, E):
                lines.append("Exp rule")
                lines.append("= " + show(d))
            else:
                lines.append("Chain rule")
                lines.append("For a^u, use a^u*ln(a)*du/d" + var)
                lines.append("= " + show(d))
            return d, lines
        if not depends(exp, [var] + deps_list_list):
            lines.append("Using generalized power rule")
            lines.append("dy/d" + var + " = " + show(d))
            return d, lines
        lines.append("Using logarithmic differentiation")
        lines.append("Let y = " + show(node))
        lines.append("Let u = " + show(base) + " and v = " + show(exp))
        lines.append("u' = " + show(tidy(diff(base, var, deps_list))) + ", v' = " + show(tidy(diff(exp, var, deps_list))))
        lines.append("dy/d" + var + " = y*(v*(u')/u + ln(u)*v')")
        lines.append("= " + show(d))
        return d, lines
    if node[0] == "fn":
        name = node[1]
        arg = node[2]
        darg = tidy(diff(arg, var, deps_list))
        if name in ("sec", "cosec", "cot"):
            lines = reciprocal_trig_quotient_steps(name, arg, var, darg, d)
            return d, lines
        if same(arg, sym(var)) or arg[0] == "sym":
            label = {
                "sin": "Using sin rule", "cos": "Using cos rule", "tan": "Using tan rule",
                "asin": "Using asin rule", "acos": "Using acos rule", "atan": "Using atan rule",
                "log": "Using log rule", "log10": "Using log10 rule", "exp": "Using exp rule",
                "sqrt": "Using sqrt rule", "abs": "Using abs rule",
                "sinh": "Using sinh rule", "cosh": "Using cosh rule", "tanh": "Using tanh rule"
            }.get(name, "Differentiate")
            lines.append(label)
            lines.append("dy/d" + var + " = " + show(d))
        else:
            dydu = rule_u(name)
            subbed = tidy(mul([subst(dydu, "u", arg), darg]))
            lines.append("Chain rule")
            lines.append("u = " + show(arg))
            lines.append("du/d" + var + " = " + show(darg))
            lines.append("= " + show(subbed))
        return d, lines
    lines.append("Diff")
    lines.append("= " + show(d))
    return d, lines



def expand(node):
    node = sim(node)
    kind = node[0]
    if kind == "mul":
        items = []
        for item in node[1]:
            items.append(expand(item))
        i = 0
        while i < len(items):
            if items[i][0] == "add":
                out = []
                for part in items[i][1]:
                    out.append(expand(make_mul(items[:i] + [part] + items[i + 1 :])))
                return sim(("add", tuple(out)))
            i += 1
        return sim(("mul", tuple(items)))
    if kind == "add":
        out = []
        for item in node[1]:
            out.append(expand(item))
        return sim(("add", tuple(out)))
    if kind == "div":
        top = expand(node[1])
        bot = sim(node[2])
        return sim(("div", top, bot))
    if kind == "pow":
        base = expand(node[1])
        exp = expand(node[2])
        if base[0] == "mul" and is_num(exp) and is_int_num(exp) and exp[1] > 0:
            items = flat(base, "mul")
            powered = []
            for item in items:
                powered.append(sim(("pow", item, exp)))
            return sim(("mul", tuple(powered)))
        if base[0] == "add" and is_num(exp) and exp[2] == 1 and exp[1] == 2:
            parts = flat(base, "add")
            out = []
            i = 0
            while i < len(parts):
                j = 0
                while j < len(parts):
                    out.append(expand(make_mul([parts[i], parts[j]])))
                    j += 1
                i += 1
            return sim(("add", tuple(out)))
        return sim(("pow", base, exp))
    if kind == "fn":
        return ("fn", node[1], expand(node[2]))
    return node


def tidy(node):
    out = sim(node)
    if tree_size(out) > TIDY_SKIP_SIZE:
        return out
    i = 0
    while i < TIDY_EXPAND_LIMIT:
        newer = sim(expand(out))
        if sig(newer) == sig(out):
            return newer
        if tree_size(newer) > 220 and tree_size(newer) > tree_size(out) * 2:
            return out
        out = newer
        i += 1
    return out



def as_rat(node):
    node = sim(node)
    kind = node[0]
    if kind in ("num", "sym", "const", "fn"):
        return node, num(1)
    if kind == "pow":
        if is_num(node[2]) and is_int_num(node[2]) and node[2][1] < 0:
            return num(1), power(node[1], num(-node[2][1]))
        return node, num(1)
    if kind == "div":
        n1, d1 = as_rat(node[1])
        n2, d2 = as_rat(node[2])
        return sim(mul([n1, d2])), sim(mul([d1, n2]))
    if kind == "mul":
        tops = []
        bots = []
        for item in flat(node, "mul"):
            ni, di = as_rat(item)
            tops.append(ni)
            bots.append(di)
        return sim(make_mul(tops)), sim(make_mul(bots))
    if kind == "add":
        parts = []
        common = num(1)
        for item in flat(node, "add"):
            ni, di = as_rat(item)
            parts.append((ni, di))
            common = sim(mul([common, di]))
        out = []
        for ni, di in parts:
            out.append(sim(mul([ni, div(common, di)])))
        return sim(add(out)), common
    return node, num(1)


def make_display_mul(parts):
    out = []
    for item in parts:
        if is_one(item):
            continue
        if item[0] == "mul":
            out.extend(flat(item, "mul"))
        else:
            out.append(item)
    if len(out) == 0:
        return num(1)
    if len(out) == 1:
        return out[0]
    return ("mul", tuple(out))


def make_display_add(parts):
    out = []
    for item in parts:
        if item[0] == "add":
            out.extend(flat(item, "add"))
        else:
            out.append(item)
    if len(out) == 0:
        return num(0)
    if len(out) == 1:
        return out[0]
    return ("add", tuple(out))


def as_rat_display(node):
    kind = node[0]
    if kind in ("num", "sym", "const", "fn"):
        return node, num(1)
    if kind == "pow":
        if is_num(node[2]) and is_int_num(node[2]) and node[2][1] < 0:
            return num(1), power(node[1], num(-node[2][1]))
        return node, num(1)
    if kind == "div":
        n1, d1 = as_rat_display(node[1])
        n2, d2 = as_rat_display(node[2])
        return make_display_mul([n1, d2]), make_display_mul([d1, n2])
    if kind == "mul":
        tops = []
        bots = []
        for item in flat(node, "mul"):
            ni, di = as_rat_display(item)
            tops.append(ni)
            bots.append(di)
        return make_display_mul(tops), make_display_mul(bots)
    if kind == "add":
        parts = []
        denominators = []
        for item in flat(node, "add"):
            ni, di = as_rat_display(item)
            den_items = [] if is_one(di) else (flat(di, "mul") if di[0] == "mul" else [di])
            parts.append((ni, den_items))
            denominators.append(den_items)
        common_bits = []
        for den_items in denominators:
            common_bits.extend(den_items)
        out = []
        i = 0
        while i < len(parts):
            ni, _di = parts[i]
            multiplier = []
            j = 0
            while j < len(denominators):
                if j != i:
                    multiplier.extend(denominators[j])
                j += 1
            out.append(make_display_mul([ni] + multiplier))
            i += 1
        return make_display_add(out), make_display_mul(common_bits)
    return node, num(1)


def readable_show(node):
    text = show(node)
    out = ""
    depth = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "(":
            depth += 1
            out += ch
        elif ch == ")":
            depth -= 1
            out += ch
        elif ch in "+-" and i != 0:
            prev = text[i - 1]
            next_ch = text[i + 1] if i + 1 < len(text) else ""
            if prev in "*/^(":
                out += ch
            elif prev == " " or next_ch == " ":
                out += ch
            else:
                out += " " + ch + " "
        else:
            out += ch
        i += 1
    return out



def prefer_trig_recip(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return ("fn", node[1], prefer_trig_recip(node[2]))
    if kind == "pow":
        return ("pow", prefer_trig_recip(node[1]), prefer_trig_recip(node[2]))
    if kind == "div":
        top = prefer_trig_recip(node[1])
        bot = prefer_trig_recip(node[2])
        if bot[0] == "fn" and bot[1] == "cos":
            return sim(("mul", (top, ("fn", "sec", bot[2]))))
        if bot[0] == "fn" and bot[1] == "sin":
            return sim(("mul", (top, ("fn", "cosec", bot[2]))))
        if bot[0] == "fn" and bot[1] == "tan":
            return sim(("mul", (top, ("fn", "cot", bot[2]))))
        if bot[0] == "pow" and is_num(bot[2]) and bot[2][2] == 1 and bot[2][1] > 0:
            base = bot[1]
            if base[0] == "fn" and base[1] == "cos":
                return sim(("mul", (top, ("pow", ("fn", "sec", base[2]), bot[2]))))
            if base[0] == "fn" and base[1] == "sin":
                return sim(("mul", (top, ("pow", ("fn", "cosec", base[2]), bot[2]))))
            if base[0] == "fn" and base[1] == "tan":
                return sim(("mul", (top, ("pow", ("fn", "cot", base[2]), bot[2]))))
        return sim(("div", top, bot))
    out = []
    for item in node[1]:
        out.append(prefer_trig_recip(item))
    return sim((kind, tuple(out)))


def recip_trig_den(node):
    if node[0] == "mul":
        out = []
        for item in flat(node, "mul"):
            repl = recip_trig_den(item)
            if repl is None:
                return None
            out.append(repl)
        return sim(make_mul(out))
    if node[0] == "fn":
        if node[1] == "cos":
            return fn("sec", node[2])
        if node[1] == "sin":
            return fn("cosec", node[2])
        if node[1] == "tan":
            return fn("cot", node[2])
    if node[0] == "pow" and node[1][0] == "fn":
        if node[1][1] == "cos":
            return power(fn("sec", node[1][2]), node[2])
        if node[1][1] == "sin":
            return power(fn("cosec", node[1][2]), node[2])
        if node[1][1] == "tan":
            return power(fn("cot", node[1][2]), node[2])
    return None


def trig_sq_name(node):
    if node[0] == "pow" and node[1][0] == "fn" and same(node[2], num(2)):
        return node[1][1], node[1][2]
    return None, None


def trig_pair(a, b):
    if a[0] == "fn" and b[0] == "fn" and same(a[2], b[2]):
        if (a[1] == "sin" and b[1] == "sec") or (a[1] == "sec" and b[1] == "sin"):
            return fn("tan", a[2])
        if (a[1] == "cos" and b[1] == "cosec") or (a[1] == "cosec" and b[1] == "cos"):
            return fn("cot", a[2])
        if (a[1] == "cos" and b[1] == "sec") or (a[1] == "sec" and b[1] == "cos"):
            return num(1)
        if (a[1] == "sin" and b[1] == "cosec") or (a[1] == "cosec" and b[1] == "sin"):
            return num(1)
        if (a[1] == "tan" and b[1] == "cot") or (a[1] == "cot" and b[1] == "tan"):
            return num(1)
    if a[0] == "fn" and b[0] == "pow" and b[1][0] == "fn" and same(a[2], b[1][2]):
        if b[2][0] == "num" and is_int_num(b[2]) and b[2][1] == -1:
            if a[1] == "cos" and b[1][1] == "sin":
                return fn("cot", a[2])
            if a[1] == "sin" and b[1][1] == "cos":
                return fn("tan", a[2])
    if b[0] == "fn" and a[0] == "pow" and a[1][0] == "fn" and same(b[2], a[1][2]):
        if a[2][0] == "num" and is_int_num(a[2]) and a[2][1] == -1:
            if b[1] == "cos" and a[1][1] == "sin":
                return fn("cot", b[2])
            if b[1] == "sin" and a[1][1] == "cos":
                return fn("tan", b[2])
    name_a, arg_a = trig_sq_name(a)
    name_b, arg_b = trig_sq_name(b)
    if a[0] == "fn" and name_b is not None and same(a[2], arg_b):
        if a[1] == "sin" and name_b == "sec":
            return mul([fn("tan", a[2]), fn("sec", a[2])])
        if a[1] == "cos" and name_b == "sec":
            return fn("sec", a[2])
        if a[1] == "sin" and name_b == "cosec":
            return fn("cosec", a[2])
        if a[1] == "cos" and name_b == "cosec":
            return mul([fn("cot", a[2]), fn("cosec", a[2])])
    if b[0] == "fn" and name_a is not None and same(b[2], arg_a):
        if b[1] == "sin" and name_a == "sec":
            return mul([fn("tan", b[2]), fn("sec", b[2])])
        if b[1] == "cos" and name_a == "sec":
            return fn("sec", b[2])
        if b[1] == "sin" and name_a == "cosec":
            return fn("cosec", b[2])
        if b[1] == "cos" and name_a == "cosec":
            return mul([fn("cot", b[2]), fn("cosec", b[2])])
    return None


def reduce_trig_mul(items):
    changed = True
    while changed:
        changed = False
        i = 0
        while i < len(items):
            j = i + 1
            while j < len(items):
                repl = trig_pair(items[i], items[j])
                if repl is not None:
                    items = items[:i] + [repl] + items[i + 1 : j] + items[j + 1 :]
                    changed = True
                    break
                j += 1
            if changed:
                break
            i += 1
    return items



def trig_normal(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return sim(("fn", node[1], trig_normal(node[2])))
    if kind == "pow":
        base = trig_normal(node[1])
        exp = trig_normal(node[2])
        if base[0] == "div" and same(base[1], num(1)):
            repl = recip_trig_den(base[2])
            if repl is not None:
                return sim(power(repl, exp))
        return sim(("pow", base, exp))
    if kind == "div":
        top = trig_normal(node[1])
        bot = trig_normal(node[2])
        repl = recip_trig_den(bot)
        if repl is not None:
            if same(top, num(1)):
                return repl
            return sim(make_mul(reduce_trig_mul(list(flat(make_mul([top, repl]), "mul")))))
        return sim(("div", top, bot))
    out = []
    for item in node[1]:
        out.append(trig_normal(item))
    if kind == "mul":
        return sim(make_mul(reduce_trig_mul(list(flat(make_mul(out), "mul")))))
    return sim((kind, tuple(out)))


def pick_implicit_vars(left, right):
    if depends(left, ["x"]) or depends(right, ["x"]):
        main = "x"
    elif depends(left, ["X"]) or depends(right, ["X"]):
        main = "X"
    else:
        main = "x"
    if depends(left, ["y"]) or depends(right, ["y"]):
        dep = "y"
    elif depends(left, ["Y"]) or depends(right, ["Y"]):
        dep = "Y"
    else:
        dep = "y"
    return main, dep


def collect_symbols(node, out):
    kind = node[0]
    if kind == "sym":
        out.add(node[1])
        return
    if kind in ("num", "const"):
        return
    if kind == "fn":
        collect_symbols(node[2], out)
        return
    if kind in ("pow", "div"):
        collect_symbols(node[1], out)
        collect_symbols(node[2], out)
        return
    for item in node[1]:
        collect_symbols(item, out)


def linear_pair(node, var_name):
    node = sim(node)
    if not depends(node, [var_name]):
        return num(0), node
    if node == sym(var_name):
        return num(1), num(0)
    if node[0] == "add":
        coeff = num(0)
        rest = num(0)
        for item in flat(node, "add"):
            pair = linear_pair(item, var_name)
            if pair is None:
                return None
            coeff = sim(add([coeff, pair[0]]))
            rest = sim(add([rest, pair[1]]))
        return coeff, rest
    if node[0] == "mul":
        dep = []
        const_bits = []
        for item in flat(node, "mul"):
            if depends(item, [var_name]):
                dep.append(item)
            else:
                const_bits.append(item)
        if len(dep) != 1:
            return None
        pair = linear_pair(dep[0], var_name)
        if pair is None:
            return None
        const_factor = make_mul(const_bits)
        return sim(mul([const_factor, pair[0]])), sim(mul([const_factor, pair[1]]))
    if node[0] == "div":
        if depends(node[2], [var_name]):
            return None
        pair = linear_pair(node[1], var_name)
        if pair is None:
            return None
        return sim(div(pair[0], node[2])), sim(div(pair[1], node[2]))
    return None


def cartesian_term(base_name, offset):
    core = sym(base_name)
    if not is_zero(offset):
        core = add([core, neg(offset)])
    return power(core, num(2))


def match_shifted_scaled_trig(node, trig_name, param):
    node = sim(node)
    terms = flat(node, "add") if node[0] == "add" else [node]
    offset = num(0)
    scaled = None
    for item in terms:
        if depends(item, [param]):
            coeff, rest = split_coeff(item)
            if rest[0] == "fn" and rest[1] == trig_name and rest[2] == sym(param) and scaled is None:
                scaled = coeff
            else:
                return None
        else:
            offset = sim(add([offset, item]))
    if scaled is None or is_zero(scaled):
        return None
    return offset, scaled


def cartesian_from_param_exprs(x_expr, y_expr, param="t"):
    if x_expr == sym(param):
        return sym("y"), sim(subst(y_expr, param, sym("x"))), "Substitute t = x."
    if y_expr == sym(param):
        return sym("x"), sim(subst(x_expr, param, sym("y"))), "Substitute t = y."
    x_pair = linear_pair(x_expr, param)
    if x_pair is not None and not is_zero(x_pair[0]):
        t_expr = sim(div(add([sym("x"), neg(x_pair[1])]), x_pair[0]))
        return sym("y"), sim(subst(y_expr, param, t_expr)), "Cartesian form"
    y_pair = linear_pair(y_expr, param)
    if y_pair is not None and not is_zero(y_pair[0]):
        t_expr = sim(div(add([sym("y"), neg(y_pair[1])]), y_pair[0]))
        return sym("x"), sim(subst(x_expr, param, t_expr)), "Cartesian form"
    x_cos = match_shifted_scaled_trig(x_expr, "cos", param)
    y_sin = match_shifted_scaled_trig(y_expr, "sin", param)
    x_sin = match_shifted_scaled_trig(x_expr, "sin", param)
    y_cos = match_shifted_scaled_trig(y_expr, "cos", param)
    pair = (x_cos, y_sin) if x_cos is not None and y_sin is not None else (x_sin, y_cos)
    if pair[0] is not None and pair[1] is not None:
        x_off, x_scale = pair[0]
        y_off, y_scale = pair[1]
        left = add([
            div(cartesian_term("x", x_off), power(x_scale, num(2))),
            div(cartesian_term("y", y_off), power(y_scale, num(2))),
        ])
        return sim(left), num(1), "Cartesian form"
    return None



def convert_neg_powers_to_fractions(node):
    if node[0] == "pow":
        base = convert_neg_powers_to_fractions(node[1])
        exp = convert_neg_powers_to_fractions(node[2])
        if exp[0] == "num" and exp[1] < 0:
            pos_exp = num(-exp[1], exp[2])
            return sim(("div", num(1), ("pow", base, pos_exp)))
        return ("pow", base, exp)
    if node[0] == "mul":
        items = []
        for item in node[1]:
            items.append(convert_neg_powers_to_fractions(item))
        return sim(("mul", tuple(items)))
    if node[0] == "div":
        return sim(("div", convert_neg_powers_to_fractions(node[1]), convert_neg_powers_to_fractions(node[2])))
    if node[0] == "add":
        items = []
        for item in node[1]:
            items.append(convert_neg_powers_to_fractions(item))
        return sim(("add", tuple(items)))
    if node[0] == "fn":
        return ("fn", node[1], convert_neg_powers_to_fractions(node[2]))
    return node


def simplify_complex_fractions(node):
    if node[0] == "div":
        top = node[1]
        bot = node[2]
        if top[0] == "div" and bot[0] == "div":
            new_top = sim(("mul", (top[1], bot[2])))
            new_bot = sim(("mul", (top[2], bot[1])))
            return sim(("div", new_top, new_bot))
        if bot[0] == "div":
            new_top = sim(("mul", (top, bot[2])))
            return sim(("div", new_top, bot[1]))
        if top[0] == "div":
            new_bot = sim(("mul", (top[2], bot)))
            return sim(("div", top[1], new_bot))
    if node[0] == "mul":
        items = []
        for item in node[1]:
            items.append(simplify_complex_fractions(item))
        return sim(("mul", tuple(items)))
    if node[0] == "add":
        items = []
        for item in node[1]:
            items.append(simplify_complex_fractions(item))
        return sim(("add", tuple(items)))
    if node[0] == "fn":
        return ("fn", node[1], simplify_complex_fractions(node[2]))
    return node


def collect_and_factor_terms(coef, rest, dname):
    dterm = ("mul", (coef, sym(dname)))
    text = readable_show(dterm)
    if not is_zero(rest):
        if neg_term(rest):
            text += " - " + readable_show(abs_term(rest))
        else:
            text += " + " + readable_show(rest)
    return text


def format_final_answer(node):
    result = sim(node)
    result = prefer_trig_recip(result)
    result = simplify_trig_identity(result)

    if result[0] == "pow" and result[2][0] == "num" and result[2][1] == 1:
        return result[1]

    if result[0] == "mul":
        items = list(flat(result, "mul"))
        if len(items) == 2:
            if is_one(items[0]):
                return items[1]
            if is_one(items[1]):
                return items[0]

    if result[0] == "div":
        bot = result[2]
        if bot[0] == "pow" and bot[1][0] == "add" and bot[2][0] == "num" and bot[2][1] == 2:
            expanded = expand(bot)
            if sig(expanded) != sig(bot) and tree_size(result[1]) <= 60 and tree_size(expanded) <= 120:
                return sim(("div", result[1], expanded))

    return result


def solve_normal_mode(text):
    second = parse_second_derivative_input(text)
    if second is not None:
        expr, var = second
        expr = trig_normal(expr)
        first = tidy(diff(expr, var, []))
        second_ans = tidy(diff(first, var, []))
        final = prefer_trig_recip(second_ans)
        formatted = format_final_answer(final)
        label = "d2y/d" + var + "2"
        steps = [
            "Method: Find second derivative with respect to " + var,
            "First differentiate y = " + show(expr),
            "dy/d" + var + " = " + show(first),
            "Differentiate again.",
            label + " = " + show(final),
        ]
        return var, steps, final, formatted
    expr, var = parse_normal_input(text)
    expr = trig_normal(expr)
    ans, steps = explain(expr, var, [])
    final = prefer_trig_recip(tidy(ans))
    formatted = format_final_answer(final)
    return var, steps, final, formatted


def normal_derivative_label(steps, var):
    if steps:
        i = 0
        prefix = "d2y/d" + var + "2"
        while i < len(steps):
            if steps[i].startswith(prefix):
                return prefix
            i += 1
    return "dy/d" + var


_depends_uncached = depends


def depends(node, names):
    key = (node, tuple(names))
    cached = _cache_get("depends", key)
    if cached is not _CACHE_MISS:
        return cached
    return _cache_set("depends", key, _depends_uncached(node, names))


_sim_uncached = sim


def sim(node):
    cached = _cache_get("sim", node)
    if cached is not _CACHE_MISS:
        return cached
    return _cache_set("sim", node, _sim_uncached(node))


_show_uncached = show


def show(node, parent=0):
    key = (node, parent)
    cached = _cache_get("show", key)
    if cached is not _CACHE_MISS:
        return cached
    return _cache_set("show", key, _show_uncached(node, parent))


_format_final_answer_uncached = format_final_answer


def format_final_answer(node):
    cached = _cache_get("format_final_answer", node)
    if cached is not _CACHE_MISS:
        return cached
    return _cache_set(
        "format_final_answer",
        node,
        _format_final_answer_uncached(node))


def simplify_trig_identity(node):
    if node[0] == "add":
        terms = list(flat(node, "add"))
        new_terms = []
        i = 0
        while i < len(terms):
            term = terms[i]
            if is_one(term) and i+2 < len(terms):
                next1 = terms[i+1]
                next2 = terms[i+2]
                if next1[0] == "mul" and len(flat(next1, "mul")) == 2:
                    items = flat(next1, "mul")
                    if is_minus_one(items[0]) and items[1][0] == "pow" and items[1][1][0] == "fn" and items[1][1][1] == "cos":
                        cos_squared = items[1]
                        if next2[0] == "pow" and next2[1][0] == "fn" and next2[1][1] == "sin" and same(next2[1][2], cos_squared[1][2]):
                            two_sin_squared = sim(("mul", (num(2), next2)))
                            new_terms.append(two_sin_squared)
                            i += 3
                            continue
            new_terms.append(term)
            i += 1

        if len(new_terms) != len(terms):
            return sim(("add", tuple(new_terms)))

    if node[0] == "add" and len(flat(node, "add")) == 2:
        terms = flat(node, "add")
        term1, term2 = terms[0], terms[1]
        if (is_sin_squared(term1) and is_cos_squared(term2) and same(get_sin_arg(term1), get_cos_arg(term2))) or \
           (is_sin_squared(term2) and is_cos_squared(term1) and same(get_sin_arg(term2), get_cos_arg(term1))):
            return num(1)

    if node[0] == "mul":
        items = []
        for item in node[1]:
            items.append(simplify_trig_identity(item))
        return sim(("mul", tuple(items)))
    if node[0] == "div":
        return sim(("div", simplify_trig_identity(node[1]), simplify_trig_identity(node[2])))
    if node[0] == "pow":
        return ("pow", simplify_trig_identity(node[1]), simplify_trig_identity(node[2]))
    if node[0] == "fn":
        return ("fn", node[1], simplify_trig_identity(node[2]))

    return node


def is_sin_squared(node):
    return node[0] == "pow" and node[1][0] == "fn" and node[1][1] == "sin" and node[2][0] == "num" and node[2][1] == 2


def is_cos_squared(node):
    return node[0] == "pow" and node[1][0] == "fn" and node[1][1] == "cos" and node[2][0] == "num" and node[2][1] == 2


def get_sin_arg(node):
    if is_sin_squared(node):
        return node[1][2]
    return None


def get_cos_arg(node):
    if is_cos_squared(node):
        return node[1][2]
    return None


MENU_LINE_WIDTH = 20
MENU_PAGE_LINES = 5


def wrap_menu_text(text, width=MENU_LINE_WIDTH):
    text = text.strip()
    if text == "":
        return [""]
    out = []
    remaining = text
    while remaining != "":
        if len(remaining) <= width:
            out.append(remaining)
            break
        split = remaining.rfind(" ", 0, width + 1)
        if split <= 0:
            out.append(remaining[:width])
            remaining = remaining[width:]
        else:
            out.append(remaining[:split])
            remaining = remaining[split + 1:]
        while remaining[:1] == " ":
            remaining = remaining[1:]
    return out


def build_menu_pages(options):
    pages = []
    page = []
    used = 0
    i = 0
    while i < len(options):
        key, label = options[i]
        lines = wrap_menu_text(key + " " + label)
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
            prompt += " " + str(page_index + 1) + "/" + str(len(pages)) + " n/p"
        prompt += ": "
        choice = input(prompt).strip()
        if choice == "":
            if default is not None:
                return default
        else:
            lowered = choice.lower()
            if lowered == "n" and len(pages) > 1:
                page_index += 1
                if page_index >= len(pages):
                    page_index = 0
                continue
            if lowered == "p" and len(pages) > 1:
                page_index -= 1
                if page_index < 0:
                    page_index = len(pages) - 1
                continue
            if choice in valid:
                return choice
        print("Bad mode.")



def main():
    mode = paged_menu_input("M", [
        ("1", "n"),
        ("2", "imp"),
        ("3", "par"),
        ("4", "2nd"),
    ], "1")
    begin_user_action()

    try:
        if mode == "1":
            text = input("y: ").strip()
            if text == "":
                raise ValueError("Enter an expression.")
            var, steps, final, formatted = solve_normal_mode(text)

            i = 1
            while i <= len(steps):
                print(str(i) + ". " + steps[i - 1])
                i += 1

            out_label = normal_derivative_label(steps, var)
            print("Answer: " + out_label + " = " + readable_show(final))

            if sig(formatted) != sig(final):
                print(out_label + " = " + readable_show(formatted))

        elif mode == "2":
            text = input("Eq: ").strip()
            if "=" not in text:
                raise ValueError("Use left=right.")
            left_text, right_text = split_at_equals(text)
            if left_text is None:
                left_text, right_text = text.split("=", 1)
            left = trig_normal(parse(left_text))
            right = trig_normal(parse(right_text))
            var, dep = pick_implicit_vars(left, right)
            dname = "d" + dep + "/d" + var
            start_display = make_display_add([left, neg(right)])
            start = tidy(add([left, neg(right)]))
            display_cleared, display_den = as_rat_display(start_display)
            cleared, cleared_den = as_rat(start)
            cleared = tidy(cleared)
            work = cleared
            if is_one(cleared_den):
                step = 1
            else:
                print("1. Clear fracs")
                if not is_one(display_den):
                    print("2. " + readable_show(display_cleared) + " = 0")
                else:
                    print("2. " + readable_show(cleared) + " = 0")
                step = 3
            dleft = sim(diff(work, var, [dep]))
            dright = num(0)
            whole = sim(add([dleft, neg(dright)]))
            coef, rest = coeff_d(whole, dname)
            if is_zero(coef):
                if not depends(work, [dep]):
                    raise ValueError("No " + dep + ".")
                raise ValueError("No " + dname + ".")
            ans = tidy(div(neg(rest), coef))
            print(str(step) + ". Method: differentiate implicitly.")
            print(str(step + 1) + ". d/d" + var + "(LHS)=d/d" + var + "(RHS)")
            print(str(step + 2) + ". " + readable_show(dleft) + " = " + readable_show(dright))
            print(str(step + 3) + ". Rearrange: make " + dname)
            grouped = collect_and_factor_terms(coef, rest, dname)
            print(str(step + 4) + ". " + grouped + " = 0")
            print(str(step + 5) + ". Answer: " + dname + " = " + readable_show(ans))
            pretty_ans = prefer_trig_recip(ans)
            if sig(pretty_ans) != sig(ans):
                print(dname + " = " + readable_show(pretty_ans))
            formatted_ans = format_final_answer(ans)
            if sig(formatted_ans) != sig(ans) and sig(formatted_ans) != sig(pretty_ans):
                print(dname + " = " + readable_show(formatted_ans))
            normalized_ans = tidy(ans)
            if sig(normalized_ans) != sig(ans) and sig(normalized_ans) != sig(pretty_ans) and sig(normalized_ans) != sig(formatted_ans):
                print(dname + " = " + readable_show(normalized_ans))
            if coef[0] == 'mul' and rest[0] == 'num' and len(flat(coef, 'mul')) == 2:
                coeff_items = flat(coef, 'mul')
                if coeff_items[1][0] == 'add' and same(rest, num(0)):
                    reduced = tidy(div(neg(rest), coef_items[0]))
                    if sig(reduced) != sig(ans):
                        print(dname + " = " + show(reduced))
            if ans[0] == 'div' and ans[1][0] == 'add' and ans[2][0] == 'mul' and len(flat(ans[2], 'mul')) == 2:
                den_items = flat(ans[2], 'mul')
                if den_items[1][0] == 'add':
                    alt = tidy(div(ans[1], den_items[1]))
                    if sig(alt) != sig(ans):
                        print(dname + " = " + show(alt))
                        alt_pretty = prefer_trig_recip(alt)
                        if sig(alt_pretty) != sig(alt) and sig(alt_pretty) != sig(ans):
                            print(dname + " = " + show(alt_pretty))
                        alt_fmt = format_final_answer(alt)
                        if sig(alt_fmt) != sig(alt) and sig(alt_fmt) != sig(alt_pretty) and sig(alt_fmt) != sig(ans):
                            print(dname + " = " + show(alt_fmt))
                        raw = div(neg(rest), den_items[1])
                        if sig(raw) != sig(ans) and sig(raw) != sig(alt) and sig(raw) != sig(alt_pretty) and sig(raw) != sig(alt_fmt):
                            print(dname + " = " + show(raw))
            if ans[0] == 'div' and ans[2][0] == 'pow' and ans[2][1][0] == 'fn' and ans[2][1][1] == 'cos' and ans[2][2][0] == 'num' and ans[2][2][1] == 2:
                alt = tidy(mul([ans[1], power(fn('sec', ans[2][1][2]), num(2))]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'fn' and ans[2][1] == 'cos':
                alt = tidy(mul([ans[1], fn('sec', ans[2][2])]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'fn' and ans[2][1] == 'sin':
                alt = tidy(mul([ans[1], fn('cosec', ans[2][2])]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            simplified_linear = tidy(ans)
            if simplified_linear[0] == 'div' and simplified_linear[1][0] == 'add' and simplified_linear[2][0] == 'mul':
                den_items = flat(simplified_linear[2], 'mul')
                if len(den_items) == 2 and is_num(den_items[0]):
                    alt = tidy(div(simplified_linear[1], den_items[1]))
                    if sig(alt) != sig(simplified_linear):
                        print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'fn' and ans[2][1] == 'exp':
                alt = tidy(div(neg(num(1)), ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'fn' and ans[2][1] == 'cosh':
                alt = tidy(div(neg(num(1)), ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'mul' and len(flat(ans[2], 'mul')) == 2:
                den_items = flat(ans[2], 'mul')
                if den_items[0][0] == 'num' and den_items[1][0] == 'sym':
                    alt = tidy(div(ans[1], ans[2]))
                    if sig(alt) != sig(ans):
                        print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'mul' and len(flat(ans[1], 'mul')) == 2 and ans[2][0] == 'pow':
                num_items = flat(ans[1], 'mul')
                if num_items[1][0] == 'sym':
                    alt = tidy(div(num_items[0], ans[2]))
                    if sig(alt) != sig(ans):
                        print(dname + " = " + show(alt))
            if ans[0] == 'num':
                pass
            elif ans[0] == 'div' and same(ans[1], num(-1)):
                alt = tidy(neg(div(num(1), ans[2])))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'add' and ans[2][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'add' and ans[2][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'mul':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'mul':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'pow' and ans[2][1][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'pow' and ans[2][1][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'pow' and ans[2][1][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'pow' and ans[2][1][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'fn' and ans[2][1] == 'sqrt':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'fn' and ans[2][1] == 'sqrt':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'add' and ans[2][0] == 'pow':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'pow' and ans[2][0] == 'pow':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'pow' and ans[2][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'pow' and ans[2][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'mul' and ans[2][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'mul' and ans[2][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'mul' and ans[2][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'add' and ans[2][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'add' and ans[2][0] == 'mul':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'add' and ans[2][0] == 'pow':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'sym' and ans[2][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'sym' and ans[2][0] == 'mul':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'sym' and ans[2][0] == 'pow':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'sym' and ans[2][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'fn' and ans[2][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'fn' and ans[2][0] == 'mul':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'fn' and ans[2][0] == 'pow':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'fn' and ans[2][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'sym' and ans[2][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'fn' and ans[2][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'sym' and ans[2][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'num':
                pass
            if ans[0] == 'div' and ans[2][0] == 'sym' and ans[1][0] == 'num' and ans[1][1] == -1:
                alt = neg(div(num(1), ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'fn' and ans[1][0] == 'num' and ans[1][1] == -1:
                alt = neg(div(num(1), ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'pow' and ans[1][0] == 'num' and ans[1][1] == -1:
                alt = neg(div(num(1), ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'mul' and ans[1][0] == 'num' and ans[1][1] == -1:
                alt = neg(div(num(1), ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'add' and ans[1][0] == 'num' and ans[1][1] == -1:
                alt = neg(div(num(1), ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[1][1] == -1 and ans[2][0] == 'mul' and len(flat(ans[2], 'mul')) == 2:
                den_items = flat(ans[2], 'mul')
                if den_items[0][0] == 'num':
                    alt = neg(div(num(1), ans[2]))
                    if sig(alt) != sig(ans):
                        print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'mul' and len(flat(ans[2], 'mul')) == 2 and ans[2][1][0] == 'sym':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'mul' and len(flat(ans[2], 'mul')) == 2 and ans[2][1][0] == 'add':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'mul' and len(flat(ans[2], 'mul')) == 2 and ans[2][1][0] == 'fn':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[1][0] == 'num' and ans[2][0] == 'mul' and len(flat(ans[2], 'mul')) == 2 and ans[2][1][0] == 'pow':
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'pow' and ans[2][1][0] == 'sym' and ans[2][2][0] == 'num' and ans[2][2][1] == 2:
                alt = tidy(div(ans[1], ans[2]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'pow' and ans[2][1][0] == 'fn' and ans[2][1][1] == 'sin' and ans[2][2][0] == 'num' and ans[2][2][1] == 2:
                alt = tidy(mul([ans[1], power(fn('cosec', ans[2][1][2]), num(2))]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))
            if ans[0] == 'div' and ans[2][0] == 'pow' and ans[2][1][0] == 'fn' and ans[2][1][1] == 'cos' and ans[2][2][0] == 'num' and ans[2][2][1] == 2:
                alt = tidy(mul([ans[1], power(fn('sec', ans[2][1][2]), num(2))]))
                if sig(alt) != sig(ans):
                    print(dname + " = " + show(alt))

        elif mode == "3":
            xt = trig_normal(parse(input("x(t): ").strip()))
            yt = trig_normal(parse(input("y(t): ").strip()))
            dx = sim(diff(xt, "t", []))
            dy = sim(diff(yt, "t", []))
            if is_zero(dx):
                raise ValueError("dx/dt=0.")
            ans = prefer_trig_recip(sim(div(dy, dx)))
            print("1. Differentiate x(t) with respect to t.")
            print("2. dx/dt = " + readable_show(dx))
            print("3. Differentiate y(t) with respect to t.")
            print("4. dy/dt = " + readable_show(dy))
            print("5. Use dy/dx = (dy/dt)/(dx/dt).")
            print("6. Answer: dy/dx = " + readable_show(ans))
            cart = cartesian_from_param_exprs(xt, yt, "t")
            if cart is not None:
                print("7. " + cart[2] + ": " + readable_show(cart[0]) + " = " + readable_show(cart[1]))

        elif mode == "4":
            text = input("y: ").strip()
            if text == "":
                raise ValueError("Enter an expression.")
            var, steps, final, formatted = solve_normal_mode(text)
            out_label = normal_derivative_label(steps, var)
            first_deriv = final
            print("First derivative found:")
            i = 1
            while i <= len(steps):
                print(str(i) + ". " + steps[i - 1])
                i += 1
            print("dy/dx = " + readable_show(first_deriv))
            print()
            print("Now differentiating dy/dx:")
            var2, steps2, final2, formatted2 = solve_normal_mode(show(first_deriv))
            out_label2 = "d2y/dx2" if var == "x" else "d/dx(" + out_label + ")"
            i = 1
            while i <= len(steps2):
                print(str(len(steps) + i) + ". " + steps2[i - 1])
                i += 1
            print("Answer: " + out_label2 + " = " + readable_show(final2))
            if sig(formatted2) != sig(final2):
                print(out_label2 + " = " + readable_show(formatted2))

        else:
            print("Bad mode.")

    except Exception as err:
        print("Err: " + str(err))


run = main
if not SKIP_AUTORUN:
    main()
