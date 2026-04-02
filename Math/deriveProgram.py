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


# ============================================================================
# SECTION 1: Core Constants and Configuration
# ============================================================================

E = ("const", "e")
PI = ("const", "pi")

FUNC_NAMES = (
    "sin", "cos", "tan", "sec", "cosec", "cot",
    "asin", "acos", "atan", "log", "log10", "sqrt", "exp", "abs"
)

FUNC_ALIASES = {
    "ln": "log",
    "csc": "cosec",
    "arcsin": "asin",
    "arccos": "acos",
    "arctan": "atan"
}

SKIP_AUTORUN = sys is not None and getattr(sys, "_derive_no_autorun", False)
MICROPYTHON_RUNTIME = sys is not None and getattr(getattr(sys, "implementation", None), "name", "") == "micropython"
LOW_MEMORY_RUNTIME = False
TIDY_EXPAND_LIMIT = 5


def apply_runtime_profile(low_memory=None):
    global LOW_MEMORY_RUNTIME, TIDY_EXPAND_LIMIT
    if low_memory is None:
        low_memory = MICROPYTHON_RUNTIME
    LOW_MEMORY_RUNTIME = bool(low_memory)
    TIDY_EXPAND_LIMIT = 3 if LOW_MEMORY_RUNTIME else 5


def begin_user_action():
    return None


def _force_low_memory_runtime(flag):
    apply_runtime_profile(flag)


apply_runtime_profile()


# ============================================================================
# SECTION 2: Character Classification Utilities
# ============================================================================

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


# ============================================================================
# SECTION 3: Arithmetic Operations
# ============================================================================

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


def is_num(x):
    return x[0] == "num"


def is_const(x):
    return x[0] == "const"


def is_zero(x):
    return is_num(x) and x[1] == 0


def is_one(x):
    return is_num(x) and x[1] == x[2]


def is_minus_one(x):
    return is_num(x) and x[1] == -x[2]


def same(a, b):
    return a == b or sig(a) == sig(b)


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


# ============================================================================
# SECTION 4: AST Node Constructors
# ============================================================================

def add(parts):
    return sim(("add", tuple(parts)))


def mul(parts):
    return sim(("mul", tuple(parts)))


def div(a, b):
    return sim(("div", a, b))


def power(a, b):
    return sim(("pow", a, b))


def fn(name, arg):
    return sim(("fn", name, arg))


def neg(x):
    return mul([num(-1), x])


# ============================================================================
# SECTION 5: AST Utilities
# ============================================================================

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


# ============================================================================
# SECTION 6: Power Normalization
# ============================================================================

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


# ============================================================================
# SECTION 7: Differentiation Rules
# ============================================================================

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


# ============================================================================
# SECTION 8: Symbolic Simplification
# ============================================================================

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


# ============================================================================
# SECTION 9: Core Differentiation
# ============================================================================

def diff(node, var, deps):
    kind = node[0]
    if kind == "num" or kind == "const":
        return num(0)
    if kind == "sym":
        if node[1] == var:
            return num(1)
        if node[1] in deps:
            return sym("d" + node[1] + "/d" + var)
        return num(0)
    if kind == "add":
        out = []
        for item in node[1]:
            out.append(diff(item, var, deps))
        return add(out)
    if kind == "mul":
        items = list(node[1])
        n = len(items)
        if n == 2:
            a, b = items[0], items[1]
            da = diff(a, var, deps)
            db = diff(b, var, deps)
            return add([mul([a, db]), mul([b, da])])
        out = []
        i = 0
        while i < n:
            bit = []
            j = 0
            while j < n:
                if i == j:
                    bit.append(diff(items[j], var, deps))
                else:
                    bit.append(items[j])
                j += 1
            out.append(mul(bit))
            i += 1
        return add(out)
    if kind == "div":
        u = node[1]
        v = node[2]
        du = diff(u, var, deps)
        dv = diff(v, var, deps)
        return div(add([mul([v, du]), neg(mul([u, dv]))]), power(v, num(2)))
    if kind == "pow":
        base = node[1]
        exp = node[2]
        if is_num(exp):
            return mul([exp, power(base, subq(exp, num(1))), diff(base, var, deps)])
        if is_num(base) and base[1] <= 0:
            raise ValueError("Exponential base must be positive.")
        if not depends(base, [var] + deps):
            return mul([power(base, exp), fn("log", base), diff(exp, var, deps)])
        if not depends(exp, [var] + deps):
            return mul([exp, power(base, subq(exp, num(1))), diff(base, var, deps)])
        return mul([power(base, exp), add([mul([exp, div(diff(base, var, deps), base)]), mul([fn("log", base), diff(exp, var, deps)])])])
    if kind == "fn":
        name = node[1]
        arg = node[2]
        darg = diff(arg, var, deps)
        if name == "exp":
            return mul([fn("exp", arg), darg])
        if name == "sin":
            return mul([fn("cos", arg), darg])
        if name == "cos":
            return neg(mul([fn("sin", arg), darg]))
        if name == "tan":
            return mul([power(fn("sec", arg), num(2)), darg])
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


# ============================================================================
# SECTION 10: Pretty-Printing
# ============================================================================

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
                    text += "-" + show(abs_term(item), pr(node))
                else:
                    text += "+" + show(item, pr(node))
            i += 1
    if pr(node) < parent:
        return "(" + text + ")"
    return text


# ============================================================================
# SECTION 11: Parsing
# ============================================================================

def parse(text):
    toks = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in " \t\r\n":
            i += 1
        elif text[i : i + 2] == "**":
            toks.append("**")
            i += 2
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
                if low in FUNC_ALIASES:
                    toks.append(FUNC_ALIASES[low])
                elif low in ("e", "pi", "ln", "csc"):
                    if low == "ln":
                        toks.append("log")
                    elif low == "csc":
                        toks.append("cosec")
                    else:
                        toks.append(low)
                elif low in FUNC_NAMES:
                    toks.append(low)
                else:
                    toks.append(word)
            else:
                k = 0
                while k < len(word):
                    toks.append(word[k])
                    k += 1
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
        if not is_atom_start(t):
            return False
        if t in FUNC_NAMES:
            return False
        return True

    def atom():
        nonlocal p
        t = cur()
        if t == "(":
            eat("(")
            out = expr()
            eat(")")
            return out
        if t == "-":
            eat("-")
            return neg(atom())
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
            if cur() == "(" and t in FUNC_NAMES:
                eat("(")
                out = expr()
                eat(")")
                return fn(t, out)
            return sym(t)
        raise ValueError("Unexpected end.")

    def power_part():
        left = atom()
        if cur() == "**":
            eat("**")
            return power(left, power_exp())
        return left

    def power_exp():
        out = power_part()
        while starts_implicit(cur()):
            out = mul([out, power_part()])
        return out

    def term():
        out = power_part()
        while True:
            if cur() == "*":
                eat("*")
                out = mul([out, power_part()])
            elif cur() == "/":
                eat("/")
                den = power_part()
                while starts_implicit(cur()):
                    den = mul([den, power_part()])
                out = div(out, den)
            elif starts_implicit(cur()):
                out = mul([out, power_part()])
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

    out = expr()
    if cur():
        raise ValueError("Unexpected token: " + cur())
    return sim(out)


def parse_normal_input(text):
    equals_pos = text.find("=")
    if equals_pos != -1:
        left_side = text[:equals_pos].strip()
        right_side = text[equals_pos+1:].strip()
        if left_side == "y" or left_side.startswith("f("):
            return parse(right_side), "x"
        else:
            raise ValueError("For equations, use implicit mode (mode 2). Or for y = f(x), put just f(x) on right side.")
    if "," in text:
        parts = text.split(",", 1)
        return parse(parts[0]), parts[1]
    return parse(text), "x"


# ============================================================================
# SECTION 12: Differentiation with Working Steps
# ============================================================================

def explain(node, var, deps):
    d = tidy(diff(node, var, deps))
    lines = []
    if node[0] == "add":
        bits = []
        for item in node[1]:
            bits.append("d/d" + var + "[" + show(item) + "]")
        lines.append("Term by term")
        lines.append("d/d" + var + "[" + show(node) + "] = " + " + ".join(bits))
        lines.append("= " + show(d))
        return d, lines
    if node[0] == "mul":
        items = list(node[1])
        u = items[0]
        v = make_mul(items[1:])
        du = tidy(diff(u, var, deps))
        dv = tidy(diff(v, var, deps))
        lines.append("Product rule")
        lines.append("u = " + show(u))
        lines.append("v = " + show(v))
        lines.append("du/d" + var + " = " + show(du))
        lines.append("dv/d" + var + " = " + show(dv))
        lines.append("d/d" + var + "[uv] = u*dv/d" + var + "+v*du/d" + var)
        lines.append("= " + show(tidy(add([mul([u, dv]), mul([v, du])]))))
        return d, lines
    if node[0] == "div":
        u = node[1]
        v = node[2]
        du = tidy(diff(u, var, deps))
        dv = tidy(diff(v, var, deps))
        lines.append("Quotient rule")
        lines.append("u = " + show(u))
        lines.append("v = " + show(v))
        lines.append("du/d" + var + " = " + show(du))
        lines.append("dv/d" + var + " = " + show(dv))
        lines.append("d/d" + var + "[u/v] = (v*du/d" + var + "-u*dv/d" + var + ")/v^2")
        lines.append("= " + show(d))
        return d, lines
    if node[0] == "pow":
        base = node[1]
        exp = node[2]
        if is_num(exp):
            db = tidy(diff(base, var, deps))
            if same(base, sym(var)):
                lines.append("Power rule")
                lines.append("d/d" + var + "[" + show(node) + "] = " + show(d))
            else:
                lines.append("Chain rule")
                lines.append("u = " + show(base))
                lines.append("du/d" + var + " = " + show(db))
                lines.append("d/d" + var + "[u^" + show(exp) + "] = " + show(exp) + "*u^(" + show(subq(exp, num(1))) + ")*du/d" + var)
                lines.append("= " + show(d))
            return d, lines
        if is_num(base) and base[1] <= 0:
            raise ValueError("Exponential base must be positive.")
        if not depends(base, [var] + deps):
            de = tidy(diff(exp, var, deps))
            lines.append("Exponential rule")
            lines.append("u = " + show(exp))
            lines.append("du/d" + var + " = " + show(de))
            lines.append("d/d" + var + "[a^u] = a^u*ln(a)*du/d" + var)
            lines.append("= " + show(d))
            return d, lines
    if node[0] == "fn":
        name = node[1]
        arg = node[2]
        darg = tidy(diff(arg, var, deps))
        if same(arg, sym(var)) or arg[0] == "sym":
            label = {
                "sin": "Differentiate sin", "cos": "Differentiate cos", "tan": "Differentiate tan",
                "sec": "Differentiate sec", "cosec": "Differentiate cosec", "cot": "Differentiate cot",
                "asin": "Differentiate arcsin", "acos": "Differentiate arccos", "atan": "Differentiate arctan",
                "log": "Differentiate ln", "log10": "Differentiate log10", "exp": "Differentiate e^x",
                "sqrt": "Differentiate sqrt", "abs": "Differentiate abs"
            }.get(name, "Differentiate")
            lines.append(label)
            lines.append("d/d" + var + "[" + show(node) + "] = " + show(d))
        else:
            dydu = rule_u(name)
            joined = tidy(mul([dydu, darg]))
            subbed = tidy(mul([subst(dydu, "u", arg), darg]))
            lines.append("Chain rule")
            lines.append("u = " + show(arg))
            lines.append("du/d" + var + " = " + show(darg))
            lines.append("dy/du = " + show(dydu))
            lines.append("dy/d" + var + " = dy/du*du/d" + var)
            lines.append("= " + show(joined))
            lines.append("Sub u = " + show(arg))
            lines.append("= " + show(subbed))
        return d, lines
    lines.append("Differentiate")
    lines.append("d/d" + var + "[" + show(node) + "] = " + show(d))
    return d, lines


# ============================================================================
# SECTION 13: Expansion and Tidying
# ============================================================================

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
    i = 0
    while i < TIDY_EXPAND_LIMIT:
        newer = sim(expand(out))
        if sig(newer) == sig(out):
            return newer
        out = newer
        i += 1
    return out


# ============================================================================
# SECTION 14: Rational Number Conversion
# ============================================================================

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


# ============================================================================
# SECTION 15: Trigonometric Reciprocal Conversion
# ============================================================================

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


# ============================================================================
# SECTION 16: Trig-Friendly Output
# ============================================================================

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


# ============================================================================
# SECTION 17: Post-Output Simplification
# ============================================================================

def simplify_steps(node, var):
    lines = []
    current = sim(node)
    current = prefer_trig_recip(current)
    step_num = 1

    lines.append(str(step_num) + ". After differentiation: " + show(current))
    step_num += 1

    newer = convert_neg_powers_to_fractions(current)
    if sig(newer) != sig(current):
        lines.append(str(step_num) + ". Convert negative powers to fractions: " + show(newer))
        step_num += 1
        current = newer

    newer = simplify_complex_fractions(current)
    if sig(newer) != sig(current):
        lines.append(str(step_num) + ". Simplify complex fractions: " + show(newer))
        step_num += 1
        current = newer

    newer = collect_and_factor_terms(current)
    if sig(newer) != sig(current):
        lines.append(str(step_num) + ". Collect and factor terms: " + show(newer))
        step_num += 1
        current = newer

    final_result = format_final_answer(current)
    if sig(final_result) != sig(current):
        lines.append(str(step_num) + ". Final formatted answer: " + show(final_result))
        step_num += 1
        current = final_result
    else:
        cleaned = prefer_trig_recip(current)
        if sig(cleaned) != sig(current):
            current = cleaned

    return current, lines


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


def collect_and_factor_terms(node):
    return node


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
            if sig(expanded) != sig(bot):
                return sim(("div", result[1], expanded))

    return result


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


# ============================================================================
# SECTION 18: CLI Entrypoint
# ============================================================================

def main():
    print("1 normal")
    print("2 implicit")
    print("3 param")
    mode = input("Mode: ").strip()
    if mode == "":
        mode = "1"
    begin_user_action()

    try:
        if mode == "1":
            text = input("y = ").strip()
            expr, var = parse_normal_input(text)
            expr = trig_normal(expr)
            ans, steps = explain(expr, var, [])
            final = prefer_trig_recip(tidy(ans))

            i = 1
            while i <= len(steps):
                print(str(i) + ". " + steps[i - 1])
                i += 1

            print("dy/d" + var + " = " + show(final))

            formatted = format_final_answer(final)
            if sig(formatted) != sig(final):
                print("dy/d" + var + " = " + show(formatted))

        elif mode == "2":
            text = input("Eqn: ").strip()
            if "=" not in text:
                raise ValueError("Use left=right for implicit differentiation.")
            left_text, right_text = text.split("=", 1)
            left = trig_normal(parse(left_text))
            right = trig_normal(parse(right_text))
            var, dep = pick_implicit_vars(left, right)
            dname = "d" + dep + "/d" + var
            start = tidy(add([left, neg(right)]))
            cleared, cleared_den = as_rat(start)
            cleared = tidy(cleared)
            work = cleared
            if is_one(cleared_den):
                step = 1
            else:
                print("1. Clear fractions")
                print("2. " + show(cleared) + " = 0")
                step = 3
            dleft = sim(diff(work, var, [dep]))
            dright = num(0)
            whole = sim(add([dleft, neg(dright)]))
            coef, rest = coeff_d(whole, dname)
            if is_zero(coef):
                if not depends(work, [dep]):
                    raise ValueError("The equation does not contain " + dep + ".")
                raise ValueError("Could not find " + dname + ".")
            ans = prefer_trig_recip(tidy(div(neg(rest), coef)))
            print(str(step) + ". d/d" + var + "(LHS) = d/d" + var + "(RHS)")
            print(str(step + 1) + ". " + show(dleft) + " = " + show(dright))
            print(str(step + 2) + ". Make " + dname + " the subject")
            if is_one(coef):
                lhs = dname
            elif is_minus_one(coef):
                lhs = "-" + dname
            else:
                lhs = show(coef) + "*" + dname
            if not is_zero(rest):
                if neg_term(rest):
                    lhs = lhs + " - " + show(abs_term(rest))
                else:
                    lhs = lhs + " + " + show(rest)
            print(str(step + 3) + ". " + lhs + " = 0")
            print(dname + " = " + show(ans))

        elif mode == "3":
            xt = trig_normal(parse(input("x(t) = ").strip()))
            yt = trig_normal(parse(input("y(t) = ").strip()))
            dx = sim(diff(xt, "t", []))
            dy = sim(diff(yt, "t", []))
            if is_zero(dx):
                raise ValueError("dx/dt = 0: dy/dx undefined.")
            ans = prefer_trig_recip(tidy(div(dy, dx)))
            print("dx/dt = " + show(dx))
            print("dy/dt = " + show(dy))
            print("dy/dx = (dy/dt)/(dx/dt) = " + show(ans))

        else:
            print("Mode must be 1, 2 or 3.")

    except Exception as err:
        print("Input error: " + str(err))


run = main
if not SKIP_AUTORUN:
    main()