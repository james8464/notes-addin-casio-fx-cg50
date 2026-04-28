try:
    import math
except ImportError:
    math = None

GCD = math.gcd if math is not None and hasattr(math, "gcd") else None

E = ("const", "e")
PI = ("const", "pi")

FUNC_NAMES = (
    "sin", "cos", "tan", "sec", "cosec", "cot",
    "exp", "log", "log10", "sqrt", "abs", "ln",
    "atan", "asin", "acos", "sinh", "cosh", "tanh",
)
FUNC_ALIASES = {
    "ln": "log",
    "csc": "cosec",
    "arcsin": "asin",
    "arccos": "acos",
    "arctan": "atan",
}


def _gcd(a, b):
    if a < 0:
        a = -a
    if b < 0:
        b = -b
    if GCD is not None:
        return GCD(a, b)
    while b:
        a, b = b, a % b
    return a or 1


def num(a, b=1):
    if b == 0:
        raise ZeroDivisionError("zero denominator")
    if b < 0:
        a = -a
        b = -b
    g = _gcd(int(a), int(b))
    return ("num", int(a) // g, int(b) // g)


def num_text(text):
    if "." not in text:
        return num(int(text), 1)
    whole, frac = text.split(".", 1)
    if whole == "":
        whole = "0"
    scale = 10 ** len(frac)
    sign = -1 if whole[:1] == "-" else 1
    value = abs(int(whole)) * scale + int(frac or "0")
    return num(sign * value, scale)


def sym(name):
    return ("sym", name)


def is_num(node):
    return isinstance(node, tuple) and len(node) > 0 and node[0] == "num"


def is_one(node):
    return is_num(node) and node[1] == node[2]


def is_zero(node):
    return is_num(node) and node[1] == 0


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


def flat(node, kind):
    if node[0] != kind:
        return [node]
    out = []
    for item in node[1]:
        if item[0] == kind:
            out.extend(flat(item, kind))
        else:
            out.append(item)
    return out


def make_add(parts):
    out = []
    const = num(0)
    for item in parts:
        item = simplify(item)
        if item[0] == "add":
            for child in item[1]:
                out.append(child)
        elif is_num(item):
            const = addq(const, item)
        elif is_zero(item):
            pass
        else:
            out.append(item)
    if not is_zero(const):
        out.append(const)
    if len(out) == 0:
        return num(0)
    if len(out) == 1:
        return out[0]
    return ("add", tuple(out))


def make_mul(parts):
    out = []
    const = num(1)
    for item in parts:
        item = simplify(item)
        if item[0] == "mul":
            for child in item[1]:
                out.append(child)
        elif is_num(item):
            const = mulq(const, item)
        else:
            out.append(item)
    if is_zero(const):
        return num(0)
    if not is_one(const):
        out.insert(0, const)
    if len(out) == 0:
        return num(1)
    if len(out) == 1:
        return out[0]
    return ("mul", tuple(out))


def add(parts):
    return simplify(make_add(parts))


def mul(parts):
    return simplify(make_mul(parts))


def div(a, b):
    return simplify(("div", a, b))


def power(a, b):
    return simplify(("pow", a, b))


def fn(name, arg):
    if name == "ln":
        name = "log"
    if name == "csc":
        name = "cosec"
    if name == "exp":
        return power(E, arg)
    return simplify(("fn", name, arg))


def neg(node):
    if is_num(node):
        return negq(node)
    return mul([num(-1), node])


def sub(a, b):
    return add([a, neg(b)])


def log_abs(node):
    return fn("log", fn("abs", node))


def simplify(node):
    if node is None:
        return None
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        name = node[1]
        if name == "ln":
            name = "log"
        if name == "csc":
            name = "cosec"
        if name == "exp":
            return simplify(("pow", E, simplify(node[2])))
        return ("fn", name, simplify(node[2]))
    if kind == "pow":
        base = simplify(node[1])
        exp = simplify(node[2])
        if is_zero(exp):
            return num(1)
        if is_one(exp):
            return base
        if is_num(base) and is_int_num(exp):
            p = exp[1]
            if p >= 0:
                return num(base[1] ** p, base[2] ** p)
            p = -p
            return num(base[2] ** p, base[1] ** p)
        if base[0] == "pow" and is_num(base[2]) and is_num(exp):
            return power(base[1], mulq(base[2], exp))
        return ("pow", base, exp)
    if kind == "div":
        top = simplify(node[1])
        bot = simplify(node[2])
        if is_zero(top):
            return num(0)
        if is_one(bot):
            return top
        if is_num(top) and is_num(bot):
            return divq(top, bot)
        if bot[0] == "pow" and is_num(bot[2]):
            return simplify(("mul", (top, ("pow", bot[1], negq(bot[2])))))
        return ("div", top, bot)
    if kind == "add":
        return make_add(node[1])
    if kind == "mul":
        return make_mul(node[1])
    return node


def depends(node, name):
    kind = node[0]
    if kind == "sym":
        return node[1] == name
    if kind in ("num", "const"):
        return False
    if kind == "fn":
        return depends(node[2], name)
    if kind in ("pow", "div"):
        return depends(node[1], name) or depends(node[2], name)
    for child in node[1]:
        if depends(child, name):
            return True
    return False


def sig(node):
    kind = node[0]
    if kind in ("num", "sym", "const"):
        return node
    if kind == "fn":
        return ("fn", node[1], sig(node[2]))
    if kind in ("pow", "div"):
        return (kind, sig(node[1]), sig(node[2]))
    parts = []
    for child in flat(node, kind):
        parts.append(sig(child))
    parts.sort(key=repr)
    return (kind, tuple(parts))


def same(a, b):
    return a == b or sig(simplify(a)) == sig(simplify(b))


def _prec(node):
    kind = node[0]
    if kind == "add":
        return 1
    if kind in ("mul", "div"):
        return 2
    if kind == "pow":
        return 3
    return 4


def _num_text(node):
    if node[2] == 1:
        return str(node[1])
    return str(node[1]) + "/" + str(node[2])


def format_expr(node, parent=0):
    node = simplify(node)
    kind = node[0]
    if kind == "num":
        return _num_text(node)
    if kind == "sym":
        return node[1]
    if kind == "const":
        return node[1]
    if kind == "fn":
        return node[1] + "(" + format_expr(node[2]) + ")"
    if kind == "pow":
        if same(node[1], E):
            text = "e^(" + format_expr(node[2]) + ")"
        else:
            text = format_expr(node[1], 3) + "^" + format_expr(node[2], 3)
        return "(" + text + ")" if _prec(node) < parent else text
    if kind == "div":
        text = format_expr(node[1], 2) + "/" + format_expr(node[2], 3)
        return "(" + text + ")" if _prec(node) < parent else text
    if kind == "mul":
        parts = []
        for item in flat(node, "mul"):
            parts.append(format_expr(item, 2))
        text = "*".join(parts)
        return "(" + text + ")" if _prec(node) < parent else text
    if kind == "add":
        parts = []
        for item in flat(node, "add"):
            if item[0] == "num" and item[1] < 0:
                parts.append("- " + _num_text(num(-item[1], item[2])))
            elif item[0] == "mul" and len(item[1]) > 0 and is_num(item[1][0]) and item[1][0][1] < 0:
                rest = make_mul((num(-item[1][0][1], item[1][0][2]),) + item[1][1:])
                parts.append("- " + format_expr(rest, 1))
            else:
                if parts:
                    parts.append("+ " + format_expr(item, 1))
                else:
                    parts.append(format_expr(item, 1))
        text = " ".join(parts)
        return "(" + text + ")" if _prec(node) < parent else text
    return repr(node)


def parse_expr(text):
    text = text.strip()
    tokens = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in " \t\r\n":
            i += 1
        elif text[i:i + 2] == "**":
            tokens.append("**")
            i += 2
        elif ch in "()+-*/^=,":
            tokens.append(ch)
            i += 1
        elif ch.isdigit() or (ch == "." and i + 1 < len(text) and text[i + 1].isdigit()):
            j = i + 1
            dots = 1 if ch == "." else 0
            while j < len(text) and (text[j].isdigit() or text[j] == "."):
                if text[j] == ".":
                    dots += 1
                j += 1
            if dots > 1:
                raise ValueError("Bad number.")
            tokens.append(text[i:j])
            i = j
        elif ch.isalpha() or ch == "_":
            j = i + 1
            while j < len(text) and (text[j].isalnum() or text[j] == "_"):
                j += 1
            tokens.append(text[i:j])
            i = j
        else:
            raise ValueError("Unexpected character: " + ch)

    pos = [0]

    def peek():
        if pos[0] >= len(tokens):
            return None
        return tokens[pos[0]]

    def take(tok=None):
        cur = peek()
        if tok is not None and cur != tok:
            raise ValueError("Expected " + repr(tok) + " but found " + repr(cur))
        pos[0] += 1
        return cur

    def starts_atom(tok):
        if tok is None:
            return False
        if tok == "(" or tok == "-":
            return True
        return tok not in ("+", "*", "/", "^", "**", ")", ",", "=")

    def atom_ok(tok):
        if tok is None:
            return False
        if tok in ("+", "-", "*", "/", "^", "**", ")", ",", "="):
            return False
        return True

    def parse_atom():
        tok = peek()
        if tok == "(":
            take("(")
            out = parse_add()
            take(")")
            return out
        if not atom_ok(tok):
            raise ValueError("Bad token: " + repr(tok))
        take()
        if tok[0].isdigit() or tok[0] == ".":
            return num_text(tok)
        name = tok.lower()
        if name in FUNC_ALIASES:
            name = FUNC_ALIASES[name]
        if name == "pi":
            return PI
        if name == "e":
            return E
        if name in FUNC_NAMES:
            if peek() == "(":
                take("(")
                arg = parse_add()
                if peek() == ",":
                    take(",")
                    arg2 = parse_add()
                    take(")")
                    if name == "log":
                        return div(fn("log", arg2), fn("log", arg))
                    return fn(name, mul([arg, arg2]))
                take(")")
                return fn(name, arg)
            if starts_atom(peek()):
                return fn(name, parse_atom())
        return sym(tok)

    def parse_power():
        out = parse_atom()
        if peek() in ("^", "**"):
            take(peek())
            out = power(out, parse_unary())
        return out

    def parse_unary():
        if peek() == "-":
            take("-")
            return neg(parse_unary())
        return parse_power()

    def parse_mul():
        out = parse_unary()
        while True:
            tok = peek()
            if tok == "*":
                take("*")
                out = mul([out, parse_unary()])
            elif tok == "/":
                take("/")
                out = div(out, parse_unary())
            elif starts_atom(tok):
                out = mul([out, parse_unary()])
            else:
                return out

    def parse_add():
        out = parse_mul()
        while peek() in ("+", "-"):
            if peek() == "+":
                take("+")
                out = add([out, parse_mul()])
            else:
                take("-")
                out = add([out, neg(parse_mul())])
        return out

    result = parse_add()
    if pos[0] != len(tokens):
        raise ValueError("Unexpected token: " + repr(peek()))
    return simplify(result)


def _split_num_coeff(node):
    node = simplify(node)
    if is_num(node):
        return node, num(1)
    if node[0] == "mul":
        coeff = num(1)
        rest = []
        for item in node[1]:
            if is_num(item):
                coeff = mulq(coeff, item)
            else:
                rest.append(item)
        return coeff, make_mul(rest)
    return num(1), node


def _reciprocal_power_coeff(node, var, positive_power):
    coeff, rest = _split_num_coeff(node)
    x = sym(var)
    if positive_power == 1 and rest[0] == "div" and is_one(rest[1]) and same(rest[2], x):
        return coeff
    if rest[0] == "div" and is_one(rest[1]):
        den = rest[2]
        if den[0] == "pow" and same(den[1], x) and is_int_num(den[2]) and den[2][1] == positive_power:
            return coeff
    if rest[0] == "pow" and same(rest[1], x) and is_int_num(rest[2]) and rest[2][1] == -positive_power:
        return coeff
    return None


def _is_exp_reciprocal(node, var):
    node = simplify(node)
    if node[0] == "fn" and node[1] == "exp":
        arg = node[2]
    elif node[0] == "pow" and same(node[1], E):
        arg = node[2]
    else:
        return None
    coeff = _reciprocal_power_coeff(arg, var, 1)
    if coeff is None:
        return None
    return coeff


def _find_exp_reciprocal_case(node, var):
    node = simplify(node)
    factors = flat(node, "mul") if node[0] == "mul" else [node]
    exp_index = -1
    c = None
    i = 0
    while i < len(factors):
        c_try = _is_exp_reciprocal(factors[i], var)
        if c_try is not None:
            exp_index = i
            c = c_try
            break
        i += 1
    if exp_index < 0 or is_zero(c):
        return None
    rest = make_mul(factors[:exp_index] + factors[exp_index + 1:])
    terms = flat(rest, "add") if rest[0] == "add" else [rest]
    coeff2 = num(0)
    coeff3 = num(0)
    for term in terms:
        c2 = _reciprocal_power_coeff(term, var, 2)
        c3 = _reciprocal_power_coeff(term, var, 3)
        if c2 is not None:
            coeff2 = addq(coeff2, c2)
        elif c3 is not None:
            coeff3 = addq(coeff3, c3)
        elif not is_zero(term):
            return None
    if is_zero(coeff2) and is_zero(coeff3):
        return None
    return c, coeff2, coeff3


def integrate_reciprocal_exp(node, var, forced_u=None):
    data = _find_exp_reciprocal_case(node, var)
    if data is None:
        return None
    c, coeff2, coeff3 = data
    if forced_u is not None and not same(simplify(forced_u), div(num(1), sym(var))):
        if not same(simplify(forced_u), div(c, sym(var))):
            return None
    x = sym(var)
    u_expr = div(c, x)
    exp_part = power(E, u_expr)
    one_over_c = divq(num(1), c)
    one_over_c2 = divq(num(1), mulq(c, c))
    const_part = addq(negq(mulq(coeff2, one_over_c)), mulq(coeff3, one_over_c2))
    x_part = neg(div(mul([coeff3, exp_part]), mul([c, x]))) if not is_zero(coeff3) else num(0)
    ans = add([mul([const_part, exp_part]), x_part])
    if is_zero(const_part):
        ans = x_part
    if same(c, num(1)):
        du_text = "-1/" + var + "^2"
    else:
        du_text = "-" + format_expr(c) + "/" + var + "^2"
    lines = [
        "Let u = " + format_expr(u_expr) + ".",
        "du/d" + var + " = " + du_text,
        "Rewrite the remaining powers of " + var + " in u.",
        "Integrate in u, then substitute back.",
    ]
    return "sub", simplify(ans), lines


def substitution_candidates(node, var, mode):
    out = []
    data = _find_exp_reciprocal_case(node, var)
    if data is not None:
        c = data[0]
        out.append(div(c, sym(var)))
        if not same(c, num(1)):
            out.append(div(num(1), sym(var)))
    return out


def _match_trig_sum_den(den):
    den = simplify(den)
    if den[0] != "add":
        return None
    parts = flat(den, "add")
    if len(parts) != 2:
        return None
    left = parts[0]
    right = parts[1]
    if left[0] != "fn" or right[0] != "fn":
        return None
    if not same(left[2], right[2]):
        return None
    names = (left[1], right[1])
    if names == ("tan", "sec") or names == ("sec", "tan"):
        return "tan_sec", left[2]
    if names == ("cot", "cosec") or names == ("cosec", "cot"):
        return "cot_cosec", left[2]
    return None


def _extract_outer_and_trig(top, arg, names):
    top = simplify(top)
    factors = flat(top, "mul") if top[0] == "mul" else [top]
    found = None
    rest = []
    for item in factors:
        if found is None and item[0] == "fn" and item[1] in names and same(item[2], arg):
            found = item[1]
        else:
            rest.append(item)
    if found is None:
        return None
    return make_mul(rest), found


def _outer_kind(outer, var):
    outer = simplify(outer)
    if is_one(outer):
        return num(1), "one"
    coeff, rest = _split_num_coeff(outer)
    if same(rest, sym(var)):
        return coeff, "x"
    return None


def integrate_trig_conjugate_ratio(node, var):
    node = simplify(node)
    if node[0] != "div":
        return None
    den = _match_trig_sum_den(node[2])
    if den is None:
        return None
    family, arg = den
    if not same(arg, sym(var)):
        return None
    if family == "tan_sec":
        data = _extract_outer_and_trig(node[1], arg, ("tan", "sec"))
    else:
        data = _extract_outer_and_trig(node[1], arg, ("cot", "cosec"))
    if data is None:
        return None
    outer, trig_name = data
    outer_data = _outer_kind(outer, var)
    if outer_data is None:
        return None
    coeff, outer_type = outer_data
    x = sym(var)
    if family == "tan_sec":
        sec = fn("sec", arg)
        tan = fn("tan", arg)
        sin = fn("sin", arg)
        if trig_name == "tan":
            v = add([sec, neg(tan)])
            primitive_v = log_abs(add([num(1), sin]))
            if outer_type == "one":
                base = add([x, v])
                explain = "Use tan(A)/(tan(A)+sec(A)) = 1 + d(sec(A)-tan(A))/dA."
            else:
                base = add([div(power(x, num(2)), num(2)), mul([x, v]), neg(primitive_v)])
                explain = "Use tan(A)/(tan(A)+sec(A)) = 1 + d(sec(A)-tan(A))/dA, then integrate by parts."
        else:
            v = add([tan, neg(sec)])
            primitive_v = log_abs(add([num(1), sin]))
            if outer_type == "one":
                base = v
                explain = "Use sec(A)/(tan(A)+sec(A)) = d(tan(A)-sec(A))/dA."
            else:
                base = add([mul([x, v]), primitive_v])
                explain = "Use sec(A)/(tan(A)+sec(A)) = d(tan(A)-sec(A))/dA, then integrate by parts."
    else:
        cot = fn("cot", arg)
        csc = fn("cosec", arg)
        sin = fn("sin", arg)
        primitive_cot = log_abs(sin)
        primitive_csc = log_abs(add([csc, neg(cot)]))
        if trig_name == "cot":
            v = add([cot, neg(csc)])
            primitive_v = add([primitive_cot, neg(primitive_csc)])
            if outer_type == "one":
                base = add([x, v])
                explain = "Use cot(A)/(cot(A)+cosec(A)) = 1 + d(cot(A)-cosec(A))/dA."
            else:
                base = add([div(power(x, num(2)), num(2)), mul([x, v]), neg(primitive_v)])
                explain = "Use cot(A)/(cot(A)+cosec(A)) = 1 + d(cot(A)-cosec(A))/dA, then integrate by parts."
        else:
            v = add([csc, neg(cot)])
            primitive_v = add([primitive_csc, neg(primitive_cot)])
            if outer_type == "one":
                base = v
                explain = "Use cosec(A)/(cot(A)+cosec(A)) = d(cosec(A)-cot(A))/dA."
            else:
                base = add([mul([x, v]), neg(primitive_v)])
                explain = "Use cosec(A)/(cot(A)+cosec(A)) = d(cosec(A)-cot(A))/dA, then integrate by parts."
    ans = simplify(mul([coeff, base]))
    return "trig", ans, [
        explain,
        "So I = Int[" + format_expr(mul([coeff, base if outer_type == "one" else node])) + "] d" + var,
    ]


def rewrite_variants(node, purpose=None):
    variants = [simplify(node)]
    trig = integrate_trig_conjugate_ratio(node, "x")
    if trig is not None:
        variants.append(trig[1])
    return variants


def equivalent(a, b, var="x"):
    return same(simplify(a), simplify(b))


def differentiate(node, var="x"):
    raise NotImplementedError("casio_core.differentiate is supplied by program adapters")


def integrate(node, var="x", mode="auto", forced_u=None):
    if mode in ("auto", "1", "sub", "4"):
        out = integrate_reciprocal_exp(node, var, forced_u)
        if out is not None:
            return out
    if mode in ("auto", "1", "trig", "3"):
        out = integrate_trig_conjugate_ratio(node, var)
        if out is not None:
            return out
    return None


def solve_task(program, mode, payload):
    return None
