try:
    import math
except ImportError:
    math = None

try:
    import sys
except ImportError:
    sys = None

SKIP_AUTORUN = sys is not None and getattr(sys, '_suvat_no_autorun', False)

# ============================================================================
# Core Constants and Configuration
# ============================================================================

FAST_GCD = math.gcd if math is not None and hasattr(math, 'gcd') else None
FAST_ISQRT = math.isqrt if math is not None and hasattr(math, 'isqrt') else None

G_DEFAULT = ('num', 49, 5)
G_DEFAULT_FLOAT = 9.8
G_ALIASES = ('g', 'G')

# ============================================================================
# Cache Dictionaries for Performance
# ============================================================================

SIG_CACHE = {}
SHOW_CACHE = {}
SPLIT_COEFF_CACHE = {}
FLAT_CACHE = {}
SAME_CACHE = {}

ALL_CACHES = (SIG_CACHE, SHOW_CACHE, SPLIT_COEFF_CACHE, FLAT_CACHE, SAME_CACHE)


def clear_all_caches():
    i = 0
    while i < len(ALL_CACHES):
        ALL_CACHES[i].clear()
        i += 1


def begin_user_action():
    clear_all_caches()


begin_user_action()

# ============================================================================
# Arithmetic Helpers
# ============================================================================

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


def is_num(node):
    return node[0] == 'num'


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


def int_sqrt(n):
    if n < 0:
        return None
    if FAST_ISQRT is not None:
        r = FAST_ISQRT(n)
        return r if r * r == n else None
    r = 0
    while r * r < n:
        r += 1
    return r if r * r == n else None


def simplify_surd(n):
    """Simplify sqrt(n) into (coeff, remainder) where sqrt(n) = coeff * sqrt(remainder).
    Returns (coeff, remainder) with remainder square-free."""
    if n <= 0:
        return None
    coeff = 1
    remainder = n
    d = 2
    while d * d <= remainder:
        while remainder % (d * d) == 0:
            coeff *= d
            remainder //= (d * d)
        d += 1
    return coeff, remainder


def node_to_float(node):
    """Convert a numeric AST node to float. Returns None if not purely numeric."""
    if node is None:
        return None
    node = sim(node)
    if node[0] == 'num':
        return node[1] / node[2]
    if node[0] == 'const':
        if node[1] == 'e':
            return math.e if math is not None else 2.718281828459045
        if node[1] == 'pi':
            return math.pi if math is not None else 3.141592653589793
        if node[1] == 'g':
            return G_DEFAULT_FLOAT
    return None


def format_decimal(value, sig_figs=None):
    """Format a float to a clean decimal string."""
    if value is None:
        return None
    if sig_figs is not None and sig_figs > 0:
        if value == 0:
            return '0'
        import math as _m
        order = _m.floor(_m.log10(abs(value)))
        decimals = max(0, sig_figs - 1 - order)
        decimals = min(decimals, 10)
        return '{:.{}f}'.format(value, decimals).rstrip('0').rstrip('.')
    s = '{:.10f}'.format(value).rstrip('0').rstrip('.')
    return s


def count_sig_figs(text):
    """Estimate significant figures from input text."""
    text = text.strip().lstrip('-')
    if '.' in text:
        digits = text.replace('.', '').replace('-', '')
        return max(1, len(digits.lstrip('0')))
    digits = text.rstrip('0')
    return max(1, len(digits))


# ============================================================================
# AST Node Constructors
# ============================================================================

def flat(node, kind):
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
        FLAT_CACHE[key] = result
        return result
    out = []
    i = 0
    while i < len(items):
        if items[i][0] == kind:
            out.extend(flat(items[i], kind))
        else:
            out.append(items[i])
        i += 1
    FLAT_CACHE[key] = out
    return out


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
    return 'add', tuple(out)


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
    return 'mul', tuple(out)


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
    return ('fn', name, arg)


def neg(node):
    return mul([num(-1), node])


def sub(a, b):
    return add([a, neg(b)])


# ============================================================================
# AST Utilities
# ============================================================================

def sig(node):
    key = node
    cached = SIG_CACHE.get(key)
    if cached is not None:
        return cached
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return node
    if kind == 'fn':
        result = ('fn', node[1], sig(node[2]))
    elif kind == 'pow':
        result = ('pow', sig(node[1]), sig(node[2]))
    elif kind == 'div':
        result = ('div', sig(node[1]), sig(node[2]))
    elif kind in ('add', 'mul'):
        parts = []
        i = 0
        while i < len(node[1]):
            parts.append(sig(node[1][i]))
            i += 1
        parts.sort()
        result = (kind, tuple(parts))
    else:
        result = node
    SIG_CACHE[key] = result
    return result


def same(a, b):
    if a is b or a == b:
        return True
    key = (a, b)
    cached = SAME_CACHE.get(key)
    if cached is not None:
        return cached
    result = sig(a) == sig(b)
    SAME_CACHE[key] = result
    SAME_CACHE[(b, a)] = result
    return result


def split_coeff(node):
    key = node
    cached = SPLIT_COEFF_CACHE.get(key)
    if cached is not None:
        return cached
    if is_num(node):
        result = (node, num(1))
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
    SPLIT_COEFF_CACHE[key] = result
    return result


def depends(node, name):
    kind = node[0]
    if kind == 'sym':
        return node[1] == name
    if kind in ('num', 'const'):
        return False
    if kind == 'fn':
        return depends(node[2], name)
    if kind in ('pow', 'div'):
        return depends(node[1], name) or depends(node[2], name)
    if kind in ('add', 'mul'):
        i = 0
        while i < len(node[1]):
            if depends(node[1][i], name):
                return True
            i += 1
    return False


# ============================================================================
# Simplification
# ============================================================================

def sim(node):
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return node
    if kind == 'fn':
        arg = sim(node[2])
        if node[1] == 'sqrt' and is_num(arg):
            if arg[1] < 0:
                return node
            a = int_sqrt(arg[1])
            b = int_sqrt(arg[2])
            if a is not None and b is not None:
                return num(a, b)
            # Surd simplification - build raw nodes to avoid recursion through div/mul
            surd = simplify_surd(arg[1])
            if surd is not None:
                coeff, rem = surd
                if rem == 1:
                    if b is not None and b != 1:
                        return num(coeff, b)
                    return num(coeff)
                sqrt_rem = ('fn', 'sqrt', num(rem))
                if b is not None and b != 1:
                    return ('div', mul([num(coeff), sqrt_rem]), num(b))
                if arg[2] == 1:
                    if coeff == 1:
                        return sqrt_rem
                    return ('mul', (num(coeff), sqrt_rem))
            # Can't simplify perfectly, return as-is
            return ('fn', 'sqrt', arg)
        if node[1] == 'log' and is_one(arg):
            return num(0)
        if node[1] == 'log' and arg[0] == 'const' and arg[1] == 'e':
            return num(1)
        if node[1] == 'log' and arg[0] == 'pow' and arg[1][0] == 'const' and arg[1][1] == 'e':
            return arg[2]
        return 'fn', node[1], arg
    if kind == 'pow':
        base = sim(node[1])
        exp = sim(node[2])
        if is_zero(exp):
            return num(1)
        if is_one(exp):
            return base
        if is_zero(base) and is_num(exp) and exp[1] > 0:
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
        i = 0
        while i < len(items):
            item = items[i]
            if is_zero(item):
                return num(0)
            if is_num(item):
                coeff = mulq(coeff, item)
            elif item[0] == 'pow':
                key = sig(item[1])
                if key not in data:
                    order.append(key)
                    data[key] = [item[1], num(0)]
                data[key][1] = addq(data[key][1], item[2])
            else:
                key = sig(item)
                if key not in data:
                    order.append(key)
                    data[key] = [item, num(0)]
                data[key][1] = addq(data[key][1], num(1))
            i += 1
        out = []
        if is_zero(coeff):
            return num(0)
        if not is_one(coeff):
            out.append(coeff)
        i = 0
        while i < len(order):
            base, exp = data[order[i]]
            if is_zero(exp):
                i += 1
                continue
            if is_one(exp):
                out.append(base)
            else:
                out.append(('pow', base, exp))
            i += 1
        if len(out) == 0:
            return num(1)
        if len(out) == 1:
            return out[0]
        return 'mul', tuple(out)
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
        if top[0] == 'div':
            return div(top[1], mul([top[2], bot]))
        if bot[0] == 'div':
            return div(mul([top, bot[2]]), bot[1])
        if top[0] == 'pow' and same(top[1], bot) and is_int_num(top[2]) and top[2][1] > 1:
            return power(bot, num(top[2][1] - 1))
        if top[0] == 'pow' and bot[0] == 'pow' and same(top[1], bot[1]) and is_int_num(top[2]) and is_int_num(bot[2]):
            new_exp = num(top[2][1] - bot[2][1])
            if is_zero(new_exp):
                return num(1)
            if new_exp[1] > 0:
                return power(top[1], new_exp)
        return 'div', top, bot
    return node


# ============================================================================
# Pretty-Printing
# ============================================================================

def pr(node):
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return 5
    if kind == 'fn':
        return 4
    if kind == 'pow':
        return 3
    if kind in ('mul', 'div'):
        return 2
    return 1


def display_neg(node):
    c, _ = split_coeff(node)
    if c[1] < 0:
        return True
    if node[0] == 'div' and is_num(node[1]) and node[1][1] < 0:
        return True
    if node[0] == 'div' and node[1][0] == 'mul' and len(node[1][1]) > 0 and is_num(node[1][1][0]) and node[1][1][0][1] < 0:
        return True
    return False


def display_abs(node):
    if node[0] == 'div' and is_num(node[1]) and node[1][1] < 0:
        return div(num(-node[1][1], node[1][2]), node[2])
    if node[0] == 'div' and node[1][0] == 'mul' and len(node[1][1]) > 0 and is_num(node[1][1][0]) and node[1][1][0][1] < 0:
        d = [num(-node[1][1][0][1], node[1][1][0][2])]
        c = 1
        while c < len(node[1][1]):
            d.append(node[1][1][c])
            c += 1
        return div(make_mul(d), node[2])
    c, rest = split_coeff(node)
    if c[1] < 0:
        return rest if is_minus_one(c) else mul([num(-c[1], c[2]), rest])
    return node


def _show(node, parent=0):
    kind = node[0]
    if kind == 'num':
        if node[2] == 1:
            text = str(node[1])
        else:
            text = str(node[1]) + '/' + str(node[2])
    elif kind == 'sym':
        text = node[1]
    elif kind == 'const':
        text = node[1]
    elif kind == 'fn':
        if node[1] == 'log' and node[2][0] == 'fn' and node[2][1] == 'abs':
            text = 'ln|' + _show(node[2][2], 0) + '|'
        elif node[1] == 'log':
            text = 'ln(' + _show(node[2], 0) + ')'
        elif node[1] == 'exp':
            inner = _show(node[2], pr(node))
            if pr(node[2]) < pr(node):
                inner = '(' + _show(node[2], 0) + ')'
            text = 'e^' + inner
        else:
            text = node[1] + '(' + _show(node[2], 0) + ')'
    elif kind == 'pow':
        if same(node[2], num(1, 2)):
            text = 'sqrt(' + _show(node[1], 0) + ')'
        elif same(node[2], num(-1, 2)):
            text = '1/sqrt(' + _show(node[1], 0) + ')'
        elif is_int_num(node[2]) and node[2][1] < 0:
            n = _show(node[1], pr(node))
            if pr(node[1]) < pr(node) or node[1][0] in ('add', 'div'):
                n = '(' + _show(node[1], 0) + ')'
            q = num(-node[2][1])
            if same(q, num(1)):
                text = '1/' + n
            else:
                text = '1/' + n + '^' + _show(q, pr(node))
        else:
            left = _show(node[1], pr(node))
            right = _show(node[2], pr(node))
            if pr(node[1]) < pr(node) or node[1][0] in ('add', 'div'):
                left = '(' + _show(node[1], 0) + ')'
            if pr(node[2]) < pr(node) or is_num(node[2]) and node[2][2] != 1:
                right = '(' + _show(node[2], 0) + ')'
            text = left + '^' + right
    elif kind == 'mul':
        items = list(node[1])
        if items and is_minus_one(items[0]):
            rest = make_mul(items[1:])
            inner = _show(rest, pr(node))
            if pr(rest) < pr(node):
                inner = '(' + _show(rest, 0) + ')'
            text = '-' + inner
        else:
            parts = []
            i = 0
            while i < len(items):
                chunk = _show(items[i], pr(node))
                if pr(items[i]) < pr(node) or items[i][0] == 'add':
                    chunk = '(' + _show(items[i], 0) + ')'
                parts.append(chunk)
                i += 1
            text = '*'.join(parts)
    elif kind == 'div':
        if node[1][0] == 'mul' and len(node[1][1]) == 2 and is_num(node[1][1][0]) and is_num(node[2]):
            o = divq(node[1][1][0], node[2])
            d = node[1][1][1]
            h = _show(d, pr(('mul', (o, d))))
            if pr(d) < pr(('mul', (o, d))) or d[0] == 'add':
                h = '(' + _show(d, 0) + ')'
            text = _show(o, pr(node)) + '*' + h
            if pr(node) < parent:
                return '(' + text + ')'
            return text
        left = _show(node[1], pr(node))
        right = _show(node[2], pr(node))
        if pr(node[1]) < pr(node) or node[1][0] == 'add':
            left = '(' + _show(node[1], 0) + ')'
        if pr(node[2]) <= pr(node):
            right = '(' + _show(node[2], 0) + ')'
        text = left + '/' + right
    else:
        bits = flat(node, 'add')
        text = ''
        i = 0
        while i < len(bits):
            if i == 0:
                h = display_abs(bits[i]) if display_neg(bits[i]) else bits[i]
                text = _show(h, pr(node))
                if display_neg(bits[i]):
                    text = '-' + text
            elif display_neg(bits[i]):
                text += '-' + _show(display_abs(bits[i]), pr(node))
            else:
                text += '+' + _show(bits[i], pr(node))
            i += 1
    if pr(node) < parent:
        return '(' + text + ')'
    return text


def show(node, parent=0):
    key = (node, parent)
    cached = SHOW_CACHE.get(key)
    if cached is not None:
        return cached
    result = _show(sim(node), parent)
    SHOW_CACHE[key] = result
    return result


# ============================================================================
# Parsing
# ============================================================================

FUNC_NAMES = ('sin', 'cos', 'tan', 'sec', 'cosec', 'cot', 'exp', 'log', 'sqrt', 'abs', 'ln', 'atan', 'asin', 'acos', 'sinh', 'cosh', 'tanh')
FUNC_ALIASES = {'csc': 'cosec', 'arcsin': 'asin', 'arccos': 'acos', 'arctan': 'atan', 'ln': 'log'}


def is_name_start(ch):
    return ('A' <= ch <= 'Z') or ('a' <= ch <= 'z') or ch == '_'


def is_name_char(ch):
    return is_name_start(ch) or ('0' <= ch <= '9')


def is_digit_char(ch):
    return '0' <= ch <= '9'


def is_num_token_start(text, i):
    ch = text[i]
    return is_digit_char(ch) or (ch == '.' and i + 1 < len(text) and is_digit_char(text[i + 1]))


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
            if low in FUNC_ALIASES or low in FUNC_NAMES or low in ('pi', 'e', 'g'):
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
        low = t.lower()
        if low in FUNC_NAMES or low in FUNC_ALIASES or low in ('ln', 'csc'):
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
        if is_digit_char(t[0]) or t[0] == '.':
            p += 1
            return num_text(t)
        if t == 'e':
            p += 1
            return 'const', 'e'
        if t == 'pi':
            p += 1
            return 'const', 'pi'
        if t == 'g':
            p += 1
            return 'const', 'g'
        if t:
            p += 1
            low = t.lower()
            if low == 'ln':
                low = 'log'
            if low in FUNC_ALIASES:
                low = FUNC_ALIASES[low]
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
            return sym(t)
        raise ValueError('Unexpected token: ' + t)

    def implicit():
        nonlocal p
        lhs = atom()
        while starts_implicit(cur()):
            lhs = mul([lhs, atom()])
        return lhs

    def parse_power():
        nonlocal p
        lhs = implicit()
        if cur() == '^' or cur() == '**':
            eat(cur())
            rhs = parse_power()
            lhs = power(lhs, rhs)
        return lhs

    def unary():
        nonlocal p
        if cur() == '-':
            eat('-')
            return neg(unary())
        if cur() == '+':
            eat('+')
            return unary()
        return parse_power()

    def term():
        nonlocal p
        lhs = unary()
        while cur() in ('*', '/'):
            op = cur()
            eat(op)
            rhs = unary()
            if op == '*':
                lhs = mul([lhs, rhs])
            else:
                lhs = div(lhs, rhs)
        return lhs

    def expr():
        nonlocal p
        lhs = term()
        while cur() in ('+', '-'):
            op = cur()
            eat(op)
            rhs = term()
            if op == '+':
                lhs = add([lhs, rhs])
            else:
                lhs = sub(lhs, rhs)
        return lhs

    return sim(expr())


def parse_value(text):
    text = text.strip()
    if text == '':
        return None
    if text == ',':
        return 'target'
    return parse(text)


# ============================================================================
# Keyword Presets
# ============================================================================

PRESET_KEYWORDS = {
    'dropped': {'u': num(0)},
    'from rest': {'u': num(0)},
    'at rest': {'u': num(0)},
    'stationary': {'u': num(0)},
    'falls': {'a': num(-98, 10)},
    'free fall': {'a': num(-98, 10)},
    'gravity': {'a': num(-98, 10)},
    'thrown upwards': {'a': num(-98, 10)},
    'thrown up': {'a': num(-98, 10)},
    'projectile': {'a': num(-98, 10)},
    'maximum height': {'v': num(0)},
    'max height': {'v': num(0)},
    'highest point': {'v': num(0)},
    'returns to ground': {'s': num(0)},
    'back to start': {'s': num(0)},
    'comes to rest': {'v': num(0)},
    'stops': {'v': num(0)},
    'decelerates': None,
    'accelerates': None,
}


def detect_presets(text):
    """Detect keyword presets in input text. Returns dict of variable overrides."""
    text = text.lower()
    overrides = {}
    for keyword, preset in PRESET_KEYWORDS.items():
        if keyword in text and preset is not None:
            for var, val in preset.items():
                if var not in overrides:
                    overrides[var] = val
    return overrides


# ============================================================================
# SUVAT Engine
# ============================================================================

def _build_s(s_val, u_val, v_val, a_val, t_val):
    half = num(1, 2)
    return add([mul([u_val, t_val]), mul([half, a_val, power(t_val, num(2))])])


def _build_s2(s_val, u_val, v_val, a_val, t_val):
    half = num(1, 2)
    return sub(mul([v_val, t_val]), mul([half, a_val, power(t_val, num(2))]))


def _build_s3(s_val, u_val, v_val, a_val, t_val):
    return mul([num(1, 2), add([u_val, v_val]), t_val])


def _build_s4(s_val, u_val, v_val, a_val, t_val):
    return div(sub(power(v_val, num(2)), power(u_val, num(2))), mul([num(2), a_val]))


def _build_u(s_val, u_val, v_val, a_val, t_val):
    return div(sub(s_val, mul([num(1, 2), a_val, t_val])), t_val)


def _build_u2(s_val, u_val, v_val, a_val, t_val):
    return sub(v_val, mul([a_val, t_val]))


def _build_u3(s_val, u_val, v_val, a_val, t_val):
    return sub(div(mul([num(2), s_val]), t_val), v_val)


def _build_v(s_val, u_val, v_val, a_val, t_val):
    return add([u_val, mul([a_val, t_val])])


def _build_v2(s_val, u_val, v_val, a_val, t_val):
    return add([div(s_val, t_val), mul([num(1, 2), a_val, t_val])])


def _build_v3(s_val, u_val, v_val, a_val, t_val):
    return power(add([power(u_val, num(2)), mul([num(2), a_val, s_val])]), num(1, 2))


def _build_a(s_val, u_val, v_val, a_val, t_val):
    return div(sub(v_val, u_val), t_val)


def _build_a2(s_val, u_val, v_val, a_val, t_val):
    return div(mul([num(2), sub(s_val, mul([u_val, t_val]))]), power(t_val, num(2)))


def _build_a3(s_val, u_val, v_val, a_val, t_val):
    return div(sub(power(v_val, num(2)), power(u_val, num(2))), mul([num(2), s_val]))


def _build_a4(s_val, u_val, v_val, a_val, t_val):
    return div(mul([num(2), sub(mul([v_val, t_val]), s_val)]), power(t_val, num(2)))


def _build_t(s_val, u_val, v_val, a_val, t_val):
    return div(sub(v_val, u_val), a_val)


def _build_t2(s_val, u_val, v_val, a_val, t_val):
    return div(mul([num(2), s_val]), add([u_val, v_val]))


def _sub_s(s_val, u_val, v_val, a_val, t_val):
    return 's = ' + show(u_val) + '*' + show(t_val) + ' + 1/2*' + show(a_val) + '*' + show(t_val) + '^2'


def _sub_s2(s_val, u_val, v_val, a_val, t_val):
    return 's = ' + show(v_val) + '*' + show(t_val) + ' - 1/2*' + show(a_val) + '*' + show(t_val) + '^2'


def _sub_s3(s_val, u_val, v_val, a_val, t_val):
    return 's = 1/2*(' + show(u_val) + ' + ' + show(v_val) + ')*' + show(t_val)


def _sub_s4(s_val, u_val, v_val, a_val, t_val):
    return 's = (' + show(v_val) + '^2 - ' + show(u_val) + '^2) / (2*' + show(a_val) + ')'


def _sub_u(s_val, u_val, v_val, a_val, t_val):
    return 'u = (' + show(s_val) + ' - 1/2*' + show(a_val) + '*' + show(t_val) + ') / ' + show(t_val)


def _sub_u2(s_val, u_val, v_val, a_val, t_val):
    return 'u = ' + show(v_val) + ' - ' + show(a_val) + '*' + show(t_val)


def _sub_u3(s_val, u_val, v_val, a_val, t_val):
    return 'u = 2*' + show(s_val) + '/' + show(t_val) + ' - ' + show(v_val)


def _sub_v(s_val, u_val, v_val, a_val, t_val):
    return 'v = ' + show(u_val) + ' + ' + show(a_val) + '*' + show(t_val)


def _sub_v2(s_val, u_val, v_val, a_val, t_val):
    return 'v = ' + show(s_val) + '/' + show(t_val) + ' + 1/2*' + show(a_val) + '*' + show(t_val)


def _sub_v3(s_val, u_val, v_val, a_val, t_val):
    return 'v = sqrt(' + show(u_val) + '^2 + 2*' + show(a_val) + '*' + show(s_val) + ')'


def _sub_a(s_val, u_val, v_val, a_val, t_val):
    return 'a = (' + show(v_val) + ' - ' + show(u_val) + ') / ' + show(t_val)


def _sub_a2(s_val, u_val, v_val, a_val, t_val):
    return 'a = 2*(' + show(s_val) + ' - ' + show(u_val) + '*' + show(t_val) + ') / ' + show(t_val) + '^2'


def _sub_a3(s_val, u_val, v_val, a_val, t_val):
    return 'a = (' + show(v_val) + '^2 - ' + show(u_val) + '^2) / (2*' + show(s_val) + ')'


def _sub_a4(s_val, u_val, v_val, a_val, t_val):
    return 'a = 2*(' + show(v_val) + '*' + show(t_val) + ' - ' + show(s_val) + ') / ' + show(t_val) + '^2'


def _sub_t(s_val, u_val, v_val, a_val, t_val):
    return 't = (' + show(v_val) + ' - ' + show(u_val) + ') / ' + show(a_val)


def _sub_t2(s_val, u_val, v_val, a_val, t_val):
    return 't = 2*' + show(s_val) + ' / (' + show(u_val) + ' + ' + show(v_val) + ')'


EQUATIONS = [
    ('s', ('u', 'a', 't'), 's = ut + 1/2at^2', 's = ut + 1/2at^2', _build_s, _sub_s, 1),
    ('s', ('v', 'a', 't'), 's = vt - 1/2at^2', 's = vt - 1/2at^2', _build_s2, _sub_s2, 2),
    ('s', ('u', 'v', 't'), 's = 1/2(u+v)t', 's = 1/2(u+v)t', _build_s3, _sub_s3, 3),
    ('s', ('u', 'v', 'a'), 's = (v^2-u^2)/2a', 'v^2 = u^2 + 2as', _build_s4, _sub_s4, 4),
    ('u', ('v', 'a', 't'), 'u = v - at', 'v = u + at', _build_u2, _sub_u2, 1),
    ('u', ('s', 'a', 't'), 'u = s/t - 1/2at', 's = ut + 1/2at^2', _build_u, _sub_u, 2),
    ('u', ('s', 'v', 't'), 'u = 2s/t - v', 's = 1/2(u+v)t', _build_u3, _sub_u3, 3),
    ('v', ('u', 'a', 't'), 'v = u + at', 'v = u + at', _build_v, _sub_v, 1),
    ('v', ('u', 'a', 's'), 'v^2 = u^2 + 2as', 'v^2 = u^2 + 2as', _build_v3, _sub_v3, 2),
    ('v', ('s', 'a', 't'), 'v = s/t + 1/2at', 's = ut + 1/2at^2', _build_v2, _sub_v2, 3),
    ('a', ('v', 'u', 't'), 'a = (v-u)/t', 'v = u + at', _build_a, _sub_a, 1),
    ('a', ('v', 'u', 's'), 'a = (v^2-u^2)/2s', 'v^2 = u^2 + 2as', _build_a3, _sub_a3, 2),
    ('a', ('s', 'u', 't'), 'a = 2(s-ut)/t^2', 's = ut + 1/2at^2', _build_a2, _sub_a2, 3),
    ('a', ('v', 's', 't'), 'a = 2(vt-s)/t^2', 's = vt - 1/2at^2', _build_a4, _sub_a4, 4),
    ('t', ('v', 'u', 'a'), 't = (v-u)/a', 'v = u + at', _build_t, _sub_t, 1),
    ('t', ('s', 'u', 'v'), 't = 2s/(u+v)', 's = 1/2(u+v)t', _build_t2, _sub_t2, 2),
]

VAR_MAP = {'s': 0, 'u': 1, 'v': 2, 'a': 3, 't': 4}
VAR_NAMES = ['s', 'u', 'v', 'a', 't']
VAR_UNITS = {'s': 'm', 'u': 'm/s', 'v': 'm/s', 'a': 'm/s^2', 't': 's'}


def _has_required(required, vals):
    for name in required:
        if vals[VAR_MAP[name]] is None:
            return False
    return True


def _exact_value(node):
    if node is None:
        return None
    node = sim(node)
    if node[0] == 'num':
        return node
    return None


def solve_quadratic_time(s_val, u_val, v_val, a_val, t_val):
    """Solve for t using the quadratic from s = ut + 1/2at^2."""
    if s_val is None or u_val is None or a_val is None:
        return None, ['Insufficient information for quadratic time solution.'], []
    a_num = _exact_value(a_val)
    if a_num is not None and is_zero(a_num):
        return None, ['Acceleration is zero; use t = s/u instead.'], []

    half = num(1, 2)
    A = mul([half, a_val])
    B = u_val
    C = neg(s_val)

    discriminant = sub(power(B, num(2)), mul([num(4), A, C]))
    disc_val = _exact_value(discriminant)

    steps = []
    steps.append('Quadratic: 1/2*' + show(a_val) + '*t^2 + ' + show(u_val) + '*t - ' + show(s_val) + ' = 0')
    steps.append('Discriminant = ' + show(B) + '^2 - 4*' + show(A) + '*' + show(C))
    steps.append('Discriminant = ' + show(discriminant))

    if disc_val is not None:
        if disc_val[1] < 0:
            return None, ['No real solution (discriminant is negative: ' + show(discriminant) + '.'], steps
        disc_sqrt = int_sqrt(disc_val[1] * disc_val[2]) if disc_val[2] == 1 else None
        if disc_sqrt is None:
            disc_sqrt_val = power(discriminant, num(1, 2))
        else:
            disc_sqrt_val = num(disc_sqrt)

        denom = mul([num(2), A])
        t1 = div(add([neg(B), disc_sqrt_val]), denom)
        t2 = div(add([neg(B), neg(disc_sqrt_val)]), denom)

        t1_val = _exact_value(t1)
        t2_val = _exact_value(t2)

        if t1_val is not None and t2_val is not None and same(t1_val, t2_val):
            steps.append('t = ' + show(t1))
            return t1, steps, [t1]

        t1_neg = t1_val is not None and t1_val[1] < 0
        t2_neg = t2_val is not None and t2_val[1] < 0

        if t1_neg and t2_neg:
            steps.append('t = ' + show(t1) + ' or t = ' + show(t2))
            steps.append('(both roots negative - check inputs)')
            return t1, steps, [t1, t2]
        if t2_neg:
            steps.append('t = ' + show(t1) + ' (positive root)')
            return t1, steps, [t1]
        if t1_neg:
            steps.append('t = ' + show(t2) + ' (positive root)')
            return t2, steps, [t2]

        steps.append('t = ' + show(t1) + ' or t = ' + show(t2))
        return t1, steps, [t1, t2]

    disc_sqrt_val = power(discriminant, num(1, 2))
    denom = mul([num(2), A])
    t1 = div(add([neg(B), disc_sqrt_val]), denom)
    steps.append('t = (-' + show(B) + ' + sqrt(' + show(discriminant) + ')) / ' + show(denom))
    return t1, steps, [t1]


def solve_quadratic_v(u_val, a_val, s_val):
    """Solve for v using v^2 = u^2 + 2as."""
    if u_val is None or a_val is None or s_val is None:
        return None, 'Insufficient information.'
    inside = add([power(u_val, num(2)), mul([num(2), a_val, s_val])])
    inside_val = _exact_value(inside)
    if inside_val is not None and inside_val[1] < 0:
        return None, 'No real solution (u^2 + 2as = ' + show(inside) + ' is negative).'
    return power(inside, num(1, 2)), None


def find_equation(target, vals):
    candidates = []
    for eq in EQUATIONS:
        if eq[0] == target and _has_required(eq[1], vals):
            candidates.append(eq)
    if not candidates:
        return None
    candidates.sort(key=lambda e: e[6])
    return candidates[0]


def build_suvat_solution(s_val, u_val, v_val, a_val, t_val, target):
    vals = (s_val, u_val, v_val, a_val, t_val)

    if target == 't':
        a_num = _exact_value(a_val)
        if a_num is not None and is_zero(a_num) and v_val is not None and u_val is not None:
            v_num = _exact_value(v_val)
            u_num = _exact_value(u_val)
            if v_num is not None and u_num is not None and not same(v_num, u_num):
                return None, 'No solution: a=0 but v!=u.', None, None
            if v_num is not None and u_num is not None and same(v_num, u_num):
                return None, 'Infinite solutions: a=0 and v=u.', None, None

    if target == 'u' and t_val is not None:
        t_num = _exact_value(t_val)
        if t_num is not None and is_zero(t_num):
            if s_val is not None:
                s_num = _exact_value(s_val)
                if s_num is not None and not is_zero(s_num):
                    return None, 'No solution: t=0 but s!=0.', None, None

    if target == 'a' and s_val is not None:
        s_num = _exact_value(s_val)
        if s_num is not None and is_zero(s_num) and u_val is not None and v_val is not None:
            u_num = _exact_value(u_val)
            v_num = _exact_value(v_val)
            if u_num is not None and v_num is not None and not same(u_num, v_num):
                return None, 'No solution: s=0 but u!=v.', None, None

    if target == 'v' and u_val is not None and a_val is not None and s_val is not None and t_val is None:
        result, err = solve_quadratic_v(u_val, a_val, s_val)
        if err:
            return None, err, None, None
        sub_text = 'v = sqrt(' + show(u_val) + '^2 + 2*' + show(a_val) + '*' + show(s_val) + ')'
        return show(result), 'v^2 = u^2 + 2as', 'v^2 = u^2 + 2as', sub_text

    if target == 't' and s_val is not None and u_val is not None and a_val is not None and v_val is None:
        result, steps, roots = solve_quadratic_time(s_val, u_val, v_val, a_val, t_val)
        if result is not None:
            return show(result), 's = ut + 1/2at^2 (quadratic)', 's = ut + 1/2at^2', steps[-1] if steps else None
        else:
            return None, steps[-1] if steps else 'No quadratic solution for t.', None, None

    eq = find_equation(target, vals)
    if eq is None:
        return None, 'insufficient information', None, None

    _, _, formula_name, base_equation, build_fn, sub_fn, _ = eq
    result = build_fn(s_val, u_val, v_val, a_val, t_val)
    sub_text = sub_fn(s_val, u_val, v_val, a_val, t_val)

    return show(result), formula_name, base_equation, sub_text


def solve_all_variables(s_val, u_val, v_val, a_val, t_val):
    """Given 3 knowns, compute all remaining variables. Returns dict of {var: (exact_node, decimal_str)}."""
    results = {}
    for var_name in VAR_NAMES:
        vals = (s_val, u_val, v_val, a_val, t_val)
        if vals[VAR_MAP[var_name]] is not None:
            node = vals[VAR_MAP[var_name]]
            dec = format_decimal(node_to_float(node))
            results[var_name] = (show(node), dec)
            continue
        result, equation, _, sub_text = build_suvat_solution(s_val, u_val, v_val, a_val, t_val, var_name)
        if result is not None:
            node = None
            for v in (s_val, u_val, v_val, a_val, t_val):
                pass
            dec = None
            if var_name == 's':
                dec = format_decimal(node_to_float(build_suvat_solution(s_val, u_val, v_val, a_val, t_val, 's')[0]))
            elif var_name == 'u':
                dec = format_decimal(node_to_float(build_suvat_solution(s_val, u_val, v_val, a_val, t_val, 'u')[0]))
            elif var_name == 'v':
                dec = format_decimal(node_to_float(build_suvat_solution(s_val, u_val, v_val, a_val, t_val, 'v')[0]))
            elif var_name == 'a':
                dec = format_decimal(node_to_float(build_suvat_solution(s_val, u_val, v_val, a_val, t_val, 'a')[0]))
            elif var_name == 't':
                dec = format_decimal(node_to_float(build_suvat_solution(s_val, u_val, v_val, a_val, t_val, 't')[0]))
            results[var_name] = (result, dec)
    return results


def format_output_with_units(target, result, equation, original_eq, sub_text, sig_figs):
    """Format the output with decimal approximation and units."""
    lines = []
    if original_eq != equation:
        lines.append('= ' + original_eq)
    lines.append(equation)
    if sub_text is not None:
        lines.append('= ' + sub_text)
    lines.append(target + ' = ' + result)
    return lines


# ============================================================================
# Main Solver
# ============================================================================

def solve_suvat():
    print('SUVAT Solver')
    print('Enter values (leave blank for unknown, use , for target)')
    print('Keywords: dropped, from rest, gravity, max height, returns to ground')
    print('Use g for gravity constant (9.8 m/s^2)')
    print()

    s_text = input('s: ').strip()
    u_text = input('u: ').strip()
    v_text = input('v: ').strip()
    a_text = input('a: ').strip()
    t_text = input('t: ').strip()

    all_text = ' '.join([s_text, u_text, v_text, a_text, t_text]).lower()
    presets = detect_presets(all_text)

    target = None

    s = parse_value(s_text)
    if s == 'target':
        target = 's'
        s = None
    elif s is None:
        target = 's'

    u = parse_value(u_text)
    if u == 'target':
        target = 'u'
        u = None
    elif u is None and target is None:
        target = 'u'

    v = parse_value(v_text)
    if v == 'target':
        target = 'v'
        v = None
    elif v is None and target is None:
        target = 'v'

    a = parse_value(a_text)
    if a == 'target':
        target = 'a'
        a = None
    elif a is None and target is None:
        target = 'a'

    t = parse_value(t_text)
    if t == 'target':
        target = 't'
        t = None
    elif t is None and target is None:
        target = 't'

    if target is None:
        print('Error: No target variable specified. Use , to mark the unknown.')
        return

    knowns = sum(1 for x in (s, u, v, a, t) if x is not None)
    if knowns < 2:
        print('Error: At least 2 known values are required.')
        return

    if presets:
        for var, val in presets.items():
            idx = VAR_MAP[var]
            current = (s, u, v, a, t)[idx]
            if current is None:
                if var == 's':
                    s = val
                elif var == 'u':
                    u = val
                elif var == 'v':
                    v = val
                elif var == 'a':
                    a = val
                elif var == 't':
                    t = val

    result, equation, original_eq, sub_text = build_suvat_solution(s, u, v, a, t, target)

    if result is None:
        print('Error: ' + equation)
        return

    sig_figs = 3
    for txt in [s_text, u_text, v_text, a_text, t_text]:
        txt = txt.strip()
        if txt and txt != ',' and txt.replace('.', '').replace('-', '').isdigit():
            sf = count_sig_figs(txt)
            if sf > sig_figs:
                sig_figs = sf

    result_node = None
    for v_name in (s, u, v, a, t):
        if v_name is not None:
            pass
    result_val = node_to_float(result)
    dec_str = format_decimal(result_val, sig_figs)

    unit = VAR_UNITS.get(target, '')

    print()
    if original_eq != equation:
        print('= ' + original_eq)
    print(equation)
    if sub_text is not None:
        print('= ' + sub_text)
    print(target + ' = ' + result)
    if dec_str is not None and result != dec_str:
        print(target + ' = ' + dec_str + (' ' + unit if unit else ''))

    print()
    print('--- All variables ---')
    all_results = solve_all_variables(s, u, v, a, t)
    for var_name in VAR_NAMES:
        if var_name in all_results:
            exact, dec = all_results[var_name]
            unit = VAR_UNITS.get(var_name, '')
            if dec is not None and exact != dec:
                print(var_name + ' = ' + exact + ' (' + dec + ' ' + unit + ')')
            else:
                print(var_name + ' = ' + exact + (' ' + unit if unit else ''))


def main():
    begin_user_action()
    try:
        solve_suvat()
    except Exception as err:
        print('Err: ' + str(err))


run = main
if not SKIP_AUTORUN:
    main()
