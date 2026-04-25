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
        ensure_reasoning_marker, fn as shared_fn, is_num,
        is_one, is_zero, normalize_input_text, neg as shared_neg, same_by_sig,
    )
    from src.shared_cache import clear_all_caches as shared_clear_all_caches
    from src.shared_reasoning_markers import REASONING_MARKERS
except ImportError:
    try:
        from shared_helpers import (
            ensure_reasoning_marker, fn as shared_fn, is_num,
            is_one, is_zero, normalize_input_text, neg as shared_neg, same_by_sig,
        )
        from shared_cache import clear_all_caches as shared_clear_all_caches
        from shared_reasoning_markers import REASONING_MARKERS
    except ImportError:
        try:
            from shared_fallback import (
                clear_all_caches as shared_clear_all_caches,
                ensure_reasoning_marker, fn as shared_fn, is_num,
                is_one, is_zero, normalize_input_text, neg as shared_neg,
                same_by_sig, REASONING_MARKERS,
            )
        except ImportError:
            shared_clear_all_caches = lambda *c: None
            ensure_reasoning_marker = lambda x: x
            shared_fn = lambda *a: tuple(a)
            is_num = lambda n: n is not None and n[0] == 'num'
            is_one = lambda n: is_num(n) and n[1] == n[2]
            is_zero = lambda n: is_num(n) and n[1] == 0
            normalize_input_text = lambda t: t.strip() if isinstance(t, str) else t
            shared_neg = lambda n: n
            same_by_sig = lambda a, b: a == b
            REASONING_MARKERS = ("method:", "use ", "using ", "let ", "solve ", "answer:")

SKIP_AUTORUN = sys is not None and getattr(sys, '_suvat_no_autorun', False)


FAST_GCD = math.gcd if math is not None and hasattr(math, 'gcd') else None
FAST_ISQRT = math.isqrt if math is not None and hasattr(math, 'isqrt') else None

G_DEFAULT = ('num', 49, 5)
G_DEFAULT_FLOAT = 9.8
G_ALIASES = ('g', 'G')


SIG_CACHE = {}
SHOW_CACHE = {}
SPLIT_COEFF_CACHE = {}
FLAT_CACHE = {}
SAME_CACHE = {}

ALL_CACHES = (SIG_CACHE, SHOW_CACHE, SPLIT_COEFF_CACHE, FLAT_CACHE, SAME_CACHE)


def clear_all_caches():
    shared_clear_all_caches(*ALL_CACHES)


def begin_user_action():
    clear_all_caches()


begin_user_action()


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
    text = text.strip()
    if text == '' or text in ('.', '+.', '-.'):
        raise ValueError('Invalid number format')
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
    kind = node[0]
    if kind == 'num':
        return node[1] / node[2]
    if kind == 'const':
        if node[1] == 'e':
            return math.e if math is not None else 2.718281828459045
        if node[1] == 'pi':
            return math.pi if math is not None else 3.141592653589793
        if node[1] == 'g':
            return G_DEFAULT_FLOAT
        return None
    if kind == 'sym':
        return None
    if kind == 'add':
        total = 0.0
        i = 0
        while i < len(node[1]):
            part = node_to_float(node[1][i])
            if part is None:
                return None
            total += part
            i += 1
        return total
    if kind == 'mul':
        total = 1.0
        i = 0
        while i < len(node[1]):
            part = node_to_float(node[1][i])
            if part is None:
                return None
            total *= part
            i += 1
        return total
    if kind == 'div':
        top = node_to_float(node[1])
        bot = node_to_float(node[2])
        if top is None or bot is None or bot == 0:
            return None
        return top / bot
    if kind == 'pow':
        base = node_to_float(node[1])
        exp = node_to_float(node[2])
        if base is None or exp is None:
            return None
        try:
            return base ** exp
        except Exception:
            return None
    if kind == 'fn':
        arg = node_to_float(node[2])
        if arg is None or math is None:
            return None
        name = node[1]
        try:
            if name == 'sqrt':
                return math.sqrt(arg)
            if name == 'log':
                return math.log(arg)
            if name == 'exp':
                return math.exp(arg)
            if name == 'abs':
                return abs(arg)
        except Exception:
            return None
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
        text = '{:.{}f}'.format(value, decimals)
        if '.' in text:
            return text.rstrip('0').rstrip('.')
        return text
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
    return shared_fn(name, arg)


def neg(node):
    return shared_neg(node, num, mul)


def sub(a, b):
    return add([a, neg(b)])



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
    return same_by_sig(a, b, sig, SAME_CACHE)


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


def norm_pow_base(node):
    a = node
    if a[0] != 'pow' or not is_num(a[1]):
        return a
    n = a[1]
    exp = a[2]
    if n[1] <= 0 or n[2] != 1:
        return a
    value = n[1]
    d = 2
    while d * d <= value:
        if value % d == 0:
            count = 0
            while value % d == 0:
                value //= d
                count += 1
            if value == 1 and count > 1:
                return 'pow', num(d), add([num(count), neg(num(0)), exp])
            return a
        d += 1
    return a


def factor_map(node):
    items = list(node[1]) if node[0] == 'mul' else [node]
    coeff = num(1)
    order = []
    data = {}
    i = 0
    while i < len(items):
        item = norm_pow_base(items[i])
        if is_num(item):
            coeff = mulq(coeff, item)
        elif item[0] == 'fn' and item[1] == 'sqrt':
            key = sig(item[2])
            if key not in data:
                order.append(key)
                data[key] = [item[2], num(0)]
            old = data[key][1]
            data[key][1] = addq(old, num(1, 2)) if is_num(old) else add([old, num(1, 2)])
        elif item[0] == 'pow':
            key = sig(item[1])
            if key not in data:
                order.append(key)
                data[key] = [item[1], num(0)]
            old = data[key][1]
            data[key][1] = addq(old, item[2]) if is_num(old) and is_num(item[2]) else add([old, item[2]])
        else:
            key = sig(item)
            if key not in data:
                order.append(key)
                data[key] = [item, num(0)]
            old = data[key][1]
            data[key][1] = addq(old, num(1)) if is_num(old) else add([old, num(1)])
        i += 1
    return coeff, order, data


def all_neg_add(node):
    if node[0] != 'add':
        return False
    items = flat(node, 'add')
    i = 0
    while i < len(items):
        coeff, _ = split_coeff(items[i])
        if coeff[1] >= 0 or is_zero(coeff):
            return False
        i += 1
    return True


def flip_add(node):
    out = []
    for item in flat(node, 'add'):
        out.append(neg(item))
    return add(out)


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


def contains_const(node, name):
    kind = node[0]
    if kind == 'const':
        return node[1] == name
    if kind in ('num', 'sym'):
        return False
    if kind == 'fn':
        return contains_const(node[2], name)
    if kind in ('pow', 'div'):
        return contains_const(node[1], name) or contains_const(node[2], name)
    if kind in ('add', 'mul'):
        i = 0
        while i < len(node[1]):
            if contains_const(node[1][i], name):
                return True
            i += 1
    return False



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
            if base[0] == 'fn' and base[1] == 'sqrt' and exp[1] > 0:
                whole = exp[1] // 2
                if exp[1] % 2 == 0:
                    return base[2] if whole == 1 else power(base[2], num(whole))
                if whole == 0:
                    return base
                return mul([base, base[2] if whole == 1 else power(base[2], num(whole))])
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
        if all_neg_add(bot):
            return div(neg(top), flip_add(bot))
        if top[0] == 'div':
            return div(top[1], mul([top[2], bot]))
        if bot[0] == 'div':
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
                top_data[key][1] = add([top_data[key][1], neg(bot_data[key][1])])
                bot_data[key][1] = num(0)
            i += 1
        num_parts = []
        den_parts = []
        if not is_one(coeff):
            if coeff[2] == 1:
                num_parts.append(num(coeff[1]))
            else:
                if coeff[1] != 1:
                    num_parts.append(num(coeff[1], 1))
                den_parts.append(num(coeff[2], 1))
        i = 0
        while i < len(top_order):
            base, exp = top_data[top_order[i]]
            if not is_zero(exp):
                num_parts.append(base if is_one(exp) else ('pow', base, exp))
            i += 1
        i = 0
        while i < len(bot_order):
            base, exp = bot_data[bot_order[i]]
            if not is_zero(exp):
                den_parts.append(base if is_one(exp) else ('pow', base, exp))
            i += 1
        num_node = make_mul(num_parts)
        den_node = make_mul(den_parts)
        if is_one(den_node):
            return num_node
        return 'div', num_node, den_node
    return node



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


def display_recip(node):
    a = node
    if a[0] == 'div' and is_one(a[1]):
        return a[2]
    if a[0] == 'pow' and is_minus_one(a[2]):
        return a[1]
    return None


def display_rearrange(node):
    a = node
    kind = a[0]
    if kind in ('num', 'sym', 'const'):
        return a
    if kind == 'fn':
        return 'fn', a[1], display_rearrange(a[2])
    if kind in ('pow', 'div'):
        return kind, display_rearrange(a[1]), display_rearrange(a[2])
    if kind == 'mul':
        num_bits = []
        den_bits = []
        for item in flat(a, 'mul'):
            item = display_rearrange(item)
            recip = display_recip(item)
            if recip is None:
                num_bits.append(item)
            else:
                den_bits.append(recip)
        if len(den_bits) == 0:
            return make_mul(num_bits)
        num_node = make_mul(num_bits)
        den_node = make_mul(den_bits)
        if is_one(num_node):
            return 'div', num(1), den_node
        return 'div', num_node, den_node
    out = []
    for item in flat(a, 'add'):
        out.append(display_rearrange(item))
    if len(out) == 2 and is_num(out[1]) and out[1][1] > 0 and display_neg(out[0]):
        return 'add', (out[1], out[0])
    return 'add', tuple(out)


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
                text += ' - ' + _show(display_abs(bits[i]), pr(node))
            else:
                text += ' + ' + _show(bits[i], pr(node))
            i += 1
    if pr(node) < parent:
        return '(' + text + ')'
    return text


def show(node, parent=0):
    key = (node, parent)
    cached = SHOW_CACHE.get(key)
    if cached is not None:
        return cached
    result = _show(display_rearrange(sim(node)), parent)
    SHOW_CACHE[key] = result
    return result



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
    text = normalize_input_text(text)
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


def dedupe_solution_nodes(nodes):
    out = []
    i = 0
    while i < len(nodes):
        node = sim(nodes[i])
        seen = False
        j = 0
        while j < len(out):
            if same(out[j], node):
                seen = True
                break
            j += 1
        if not seen:
            out.append(node)
        i += 1
    return out


def format_solution_texts(result_node, sig_figs=None):
    nodes = result_node if isinstance(result_node, list) else [result_node]
    nodes = dedupe_solution_nodes(nodes)
    exact_bits = []
    dec_bits = []
    allow_decimal = True
    i = 0
    while i < len(nodes):
        exact_bits.append(show(nodes[i]))
        if contains_const(nodes[i], 'g'):
            allow_decimal = False
        else:
            dec_val = format_decimal(node_to_float(nodes[i]), sig_figs)
            if dec_val is not None:
                dec_bits.append(dec_val)
        i += 1
    exact_text = ' or '.join(exact_bits)
    if not allow_decimal or len(dec_bits) != len(exact_bits):
        return exact_text, None
    dec_text = ' or '.join(dec_bits)
    if dec_text == exact_text:
        return exact_text, None
    return exact_text, dec_text



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


def _build_t3(s_val, u_val, v_val, a_val, t_val):
    return div(s_val, u_val)


def _sub_s(s_val, u_val, v_val, a_val, t_val):
    return 's = ' + show(u_val) + '*' + show(t_val) + ' + 1/2*' + show(a_val) + '*' + show(t_val) + '^2'


def _sub_s2(s_val, u_val, v_val, a_val, t_val):
    return 's = ' + show(v_val) + '*' + show(t_val) + ' - 1/2*' + show(a_val) + '*' + show(t_val) + '^2'


def _sub_s3(s_val, u_val, v_val, a_val, t_val):
    return 's = 1/2*(' + show(u_val) + ' + ' + show(v_val) + ')*' + show(t_val)


def _sub_s4(s_val, u_val, v_val, a_val, t_val):
    return 's = (' + show(power(v_val, num(2))) + ' - ' + show(power(u_val, num(2))) + ') / (' + show(mul([num(2), a_val])) + ')'


def _sub_u(s_val, u_val, v_val, a_val, t_val):
    return 'u = (' + show(s_val) + ' - 1/2*' + show(a_val) + '*' + show(power(t_val, num(2))) + ') / (' + show(t_val) + ')'


def _sub_u2(s_val, u_val, v_val, a_val, t_val):
    return 'u = ' + show(v_val) + ' - ' + show(a_val) + '*' + show(t_val)


def _sub_u3(s_val, u_val, v_val, a_val, t_val):
    return 'u = 2*' + show(s_val) + '/(' + show(t_val) + ') - ' + show(v_val)


def _sub_v(s_val, u_val, v_val, a_val, t_val):
    return 'v = ' + show(u_val) + ' + ' + show(a_val) + '*' + show(t_val)


def _sub_v2(s_val, u_val, v_val, a_val, t_val):
    return 'v = ' + show(s_val) + '/(' + show(t_val) + ') + 1/2*' + show(a_val) + '*' + show(t_val)


def _sub_v3(s_val, u_val, v_val, a_val, t_val):
    return 'v = sqrt(' + show(power(u_val, num(2))) + ' + 2*' + show(a_val) + '*' + show(s_val) + ')'


def _sub_a(s_val, u_val, v_val, a_val, t_val):
    return 'a = (' + show(v_val) + ' - ' + show(u_val) + ') / (' + show(t_val) + ')'


def _sub_a2(s_val, u_val, v_val, a_val, t_val):
    return 'a = 2*(' + show(s_val) + ' - ' + show(u_val) + '*' + show(t_val) + ') / (' + show(power(t_val, num(2))) + ')'


def _sub_a3(s_val, u_val, v_val, a_val, t_val):
    return 'a = (' + show(power(v_val, num(2))) + ' - ' + show(power(u_val, num(2))) + ') / (' + show(mul([num(2), s_val])) + ')'


def _sub_a4(s_val, u_val, v_val, a_val, t_val):
    return 'a = 2*(' + show(v_val) + '*' + show(t_val) + ' - ' + show(s_val) + ') / (' + show(power(t_val, num(2))) + ')'


def _sub_t(s_val, u_val, v_val, a_val, t_val):
    return 't = (' + show(v_val) + ' - ' + show(u_val) + ') / (' + show(a_val) + ')'


def _sub_t2(s_val, u_val, v_val, a_val, t_val):
    return 't = 2*' + show(s_val) + ' / (' + show(u_val) + ' + ' + show(v_val) + ')'


def _sub_t3(s_val, u_val, v_val, a_val, t_val):
    return 't = ' + show(s_val) + ' / (' + show(u_val) + ')'


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
        u_num = _exact_value(u_val)
        s_num = _exact_value(s_val)
        if u_num is not None and is_zero(u_num):
            if s_num is not None and is_zero(s_num):
                return None, ['Infinite solutions: a=0, u=0 and s=0.'], []
            return None, ['No solution: a=0 and u=0 but s!=0.'], []
        result = sim(_build_t3(s_val, u_val, v_val, a_val, t_val))
        return result, ['Since a = 0, use s = ut.', 't = ' + show(result)], [result]

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
        t1_float = node_to_float(t1)
        t2_float = node_to_float(t2)

        def is_negative_exact_or_float(exact_val, float_val):
            if exact_val is not None:
                return exact_val[1] < 0
            return float_val is not None and float_val < -1e-9

        def is_nonnegative_exact_or_float(exact_val, float_val):
            if exact_val is not None:
                return exact_val[1] >= 0
            return float_val is not None and float_val >= -1e-9

        if t1_val is not None and t2_val is not None and same(t1_val, t2_val):
            steps.append('t = ' + show(t1))
            return t1, steps, [t1]

        t1_neg = is_negative_exact_or_float(t1_val, t1_float)
        t2_neg = is_negative_exact_or_float(t2_val, t2_float)

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
        if is_nonnegative_exact_or_float(t1_val, t1_float) and not is_nonnegative_exact_or_float(t2_val, t2_float):
            steps.append('t = ' + show(t1) + ' (positive root)')
            return t1, steps, [t1]
        if is_nonnegative_exact_or_float(t2_val, t2_float) and not is_nonnegative_exact_or_float(t1_val, t1_float):
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
    root = power(inside, num(1, 2))
    return dedupe_solution_nodes([root, neg(root)]), None


def resolve_target_inputs(parsed_values):
    target = None
    values = list(parsed_values)
    blanks = []
    i = 0
    while i < len(values):
        if values[i] == 'target':
            if target is not None:
                return None, None, 'Error: Mark only one target variable with ,.'
            target = VAR_NAMES[i]
            values[i] = None
        elif values[i] is None:
            blanks.append(VAR_NAMES[i])
        i += 1
    if target is None:
        if len(blanks) == 0:
            return None, None, 'Error: No target variable specified. Use , to mark the unknown.'
        if len(blanks) > 1:
            return None, None, 'Error: Multiple unknowns detected (' + ', '.join(blanks) + '). Use , to mark exactly one target.'
        target = blanks[0]
    return tuple(values), target, None


def should_show_all_variables(target, values):
    missing = []
    i = 0
    while i < len(values):
        if values[i] is None and VAR_NAMES[i] != target:
            missing.append(VAR_NAMES[i])
        i += 1
    return len(missing) == 0


def format_all_variables(values, target, sig_figs=None):
    results = {}
    i = 0
    while i < len(VAR_NAMES):
        var_name = VAR_NAMES[i]
        node = values[i]
        if node is not None:
            exact, dec = format_solution_texts(node, sig_figs)
            results[var_name] = (exact, dec)
        elif var_name == target:
            result_node, _equation, _original_eq, _sub_text = _build_suvat_solution_data(
                values[0], values[1], values[2], values[3], values[4], var_name
            )
            if result_node is not None:
                results[var_name] = format_solution_texts(result_node, sig_figs)
        i += 1
    return results


def find_equation(target, vals):
    candidates = []
    for eq in EQUATIONS:
        if eq[0] == target and _has_required(eq[1], vals):
            candidates.append(eq)
    if not candidates:
        return None
    candidates.sort(key=lambda e: e[6])
    return candidates[0]


def _symbolic_values(vals):
    out = []
    i = 0
    while i < len(VAR_NAMES):
        if vals[i] is None:
            out.append(sym(VAR_NAMES[i]))
        else:
            out.append(vals[i])
        i += 1
    return tuple(out)


def find_symbolic_equation(target, vals):
    candidates = []
    for eq in EQUATIONS:
        if eq[0] != target:
            continue
        missing = 0
        for name in eq[1]:
            if vals[VAR_MAP[name]] is None:
                missing += 1
        candidates.append((missing, eq[6], eq))
    if not candidates:
        return None

    candidates.sort(key=lambda item: (item[0], item[1]))
    filled = _symbolic_values(vals)

    for _missing, _priority, eq in candidates:
        _target, _required, formula_name, base_equation, build_fn, sub_fn, _ = eq
        try:
            result = sim(build_fn(filled[0], filled[1], filled[2], filled[3], filled[4]))
            sub_text = sub_fn(filled[0], filled[1], filled[2], filled[3], filled[4])
            return result, formula_name, base_equation, sub_text
        except Exception:
            pass

    return None


def _build_suvat_solution_data(s_val, u_val, v_val, a_val, t_val, target):
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

    if target == 'a' and s_val is not None and t_val is None:
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
        sub_text = 'v = ±sqrt(' + show(power(u_val, num(2))) + ' + 2*' + show(a_val) + '*' + show(s_val) + ')'
        return result, 'v^2 = u^2 + 2as', 'v^2 = u^2 + 2as', sub_text

    if target == 't' and s_val is not None and u_val is not None and a_val is not None and v_val is None:
        a_num = _exact_value(a_val)
        if a_num is not None and is_zero(a_num):
            u_num = _exact_value(u_val)
            s_num = _exact_value(s_val)
            if u_num is not None and is_zero(u_num):
                if s_num is not None and is_zero(s_num):
                    return None, 'Infinite solutions: a=0, u=0 and s=0.', None, None
                return None, 'No solution: a=0 and u=0 but s!=0.', None, None
            result = sim(_build_t3(s_val, u_val, v_val, a_val, t_val))
            result_float = node_to_float(result)
            if result_float is not None and result_float < -1e-9:
                return None, 'No solution: time must be positive.', None, None
            return result, 't = s/u', 's = ut', _sub_t3(s_val, u_val, v_val, a_val, t_val)
        
        result, steps, roots = solve_quadratic_time(s_val, u_val, v_val, a_val, t_val)
        if result is not None:
            root_values = []
            i = 0
            while i < len(roots):
                exact = _exact_value(roots[i])
                if exact is not None:
                    root_values.append(exact)
                else:
                    approx = node_to_float(roots[i])
                    if approx is not None:
                        root_values.append(num(int(round(approx * 1000000)), 1000000))
                i += 1
            if len(root_values) == 0:
                return sim(result), 's = ut + 1/2at^2 (quadratic)', 's = ut + 1/2at^2', steps[-1] if steps else None
            has_nonnegative = False
            i = 0
            while i < len(root_values):
                if root_values[i][1] >= 0:
                    has_nonnegative = True
                    break
                i += 1
            if not has_nonnegative:
                return None, 'No solution: time must be positive.', None, None
            return sim(result), 's = ut + 1/2at^2 (quadratic)', 's = ut + 1/2at^2', steps[-1] if steps else None
        else:
            return None, steps[-1] if steps else 'No quadratic solution for t.', None, None

    if target == 't' and u_val is not None and v_val is not None and a_val is not None:
        t_direct = sim(_build_t(s_val, u_val, v_val, a_val, t_val))
        t_exact = _exact_value(t_direct)
        if t_exact is not None and t_exact[1] < 0:
            return None, 'No solution: time must be positive.', None, None
        if t_exact is not None:
            return t_direct, 't = (v-u)/a', 'v = u + at', _sub_t(s_val, u_val, v_val, a_val, t_val)

    if target == 'a' and v_val is not None and u_val is not None and t_val is not None:
        t_num = _exact_value(t_val)
        if t_num is not None and is_zero(t_num):
            v_num = _exact_value(v_val)
            u_num = _exact_value(u_val)
            if v_num is not None and u_num is not None and same(v_num, u_num):
                return None, 'Infinite solutions: t=0 and v=u.', None, None
            return None, 'No solution: division by zero in a formula.', None, None
        return sim(_build_a(s_val, u_val, v_val, a_val, t_val)), 'a = (v-u)/t', 'v = u + at', _sub_a(s_val, u_val, v_val, a_val, t_val)

    if target == 's' and u_val is not None and a_val is not None and t_val is not None:
        return sim(_build_s(s_val, u_val, v_val, a_val, t_val)), 's = ut + 1/2at^2', 's = ut + 1/2at^2', _sub_s(s_val, u_val, v_val, a_val, t_val)

    if target == 't' and s_val is not None and u_val is not None and v_val is not None:
        uv_sum = _exact_value(add([u_val, v_val]))
        if uv_sum is not None and is_zero(uv_sum):
            return None, 'No solution: division by zero in t formula.', None, None
        return sim(_build_t2(s_val, u_val, v_val, a_val, t_val)), 't = 2s/(u+v)', 's = 1/2(u+v)t', _sub_t2(s_val, u_val, v_val, a_val, t_val)

    if target == 'a' and v_val is not None and u_val is not None and s_val is not None:
        return sim(_build_a3(s_val, u_val, v_val, a_val, t_val)), 'a = (v^2-u^2)/2s', 'v^2 = u^2 + 2as', _sub_a3(s_val, u_val, v_val, a_val, t_val)

    if target == 's' and u_val is not None and v_val is not None and t_val is not None:
        return sim(_build_s3(s_val, u_val, v_val, a_val, t_val)), 's = 1/2(u+v)t', 's = 1/2(u+v)t', _sub_s3(s_val, u_val, v_val, a_val, t_val)

    if target == 'v' and u_val is not None and a_val is not None and t_val is not None:
        return sim(_build_v(s_val, u_val, v_val, a_val, t_val)), 'v = u + at', 'v = u + at', _sub_v(s_val, u_val, v_val, a_val, t_val)

    if target == 'u' and v_val is not None and a_val is not None and t_val is not None:
        return sim(_build_u2(s_val, u_val, v_val, a_val, t_val)), 'u = v - at', 'v = u + at', _sub_u2(s_val, u_val, v_val, a_val, t_val)

    if target == 't' and v_val is not None and u_val is not None and a_val is not None:
        return sim(_build_t(s_val, u_val, v_val, a_val, t_val)), 't = (v-u)/a', 'v = u + at', _sub_t(s_val, u_val, v_val, a_val, t_val)

    if target == 'u' and s_val is not None and a_val is not None and t_val is not None:
        return sim(_build_u(s_val, u_val, v_val, a_val, t_val)), 'u = s/t - 1/2at', 's = ut + 1/2at^2', _sub_u(s_val, u_val, v_val, a_val, t_val)

    if target == 'a' and s_val is not None and u_val is not None and t_val is not None:
        return sim(_build_a2(s_val, u_val, v_val, a_val, t_val)), 'a = 2(s-ut)/t^2', 's = ut + 1/2at^2', _sub_a2(s_val, u_val, v_val, a_val, t_val)

    if target == 'v' and s_val is not None and a_val is not None and t_val is not None:
        return sim(_build_v2(s_val, u_val, v_val, a_val, t_val)), 'v = s/t + 1/2at', 's = ut + 1/2at^2', _sub_v2(s_val, u_val, v_val, a_val, t_val)

    if target == 'u' and s_val is not None and v_val is not None and t_val is not None:
        return sim(_build_u3(s_val, u_val, v_val, a_val, t_val)), 'u = 2s/t - v', 's = 1/2(u+v)t', _sub_u3(s_val, u_val, v_val, a_val, t_val)

    if target == 'a' and v_val is not None and s_val is not None and t_val is not None:
        return sim(_build_a4(s_val, u_val, v_val, a_val, t_val)), 'a = 2(vt-s)/t^2', 's = vt - 1/2at^2', _sub_a4(s_val, u_val, v_val, a_val, t_val)

    if target == 's' and v_val is not None and a_val is not None and t_val is not None:
        return sim(_build_s2(s_val, u_val, v_val, a_val, t_val)), 's = vt - 1/2at^2', 's = vt - 1/2at^2', _sub_s2(s_val, u_val, v_val, a_val, t_val)

    if target == 's' and u_val is not None and v_val is not None and a_val is not None:
        return sim(_build_s4(s_val, u_val, v_val, a_val, t_val)), 's = (v^2-u^2)/2a', 'v^2 = u^2 + 2as', _sub_s4(s_val, u_val, v_val, a_val, t_val)

    if target == 'v' and u_val is not None and a_val is not None and s_val is not None and t_val is None:
        result, err = solve_quadratic_v(u_val, a_val, s_val)
        if err:
            return None, err, None, None
        sub_text = 'v = ±sqrt(' + show(power(u_val, num(2))) + ' + 2*' + show(a_val) + '*' + show(s_val) + ')'
        return result, 'v^2 = u^2 + 2as', 'v^2 = u^2 + 2as', sub_text

    eq = find_equation(target, vals)
    if eq is None:
        symbolic = find_symbolic_equation(target, vals)
        if symbolic is not None:
            return symbolic
        return None, 'insufficient information', None, None

    _, _, formula_name, base_equation, build_fn, sub_fn, _ = eq
    try:
        result = sim(build_fn(s_val, u_val, v_val, a_val, t_val))
        sub_text = sub_fn(s_val, u_val, v_val, a_val, t_val)
    except ValueError as err:
        return None, 'No solution: ' + str(err), None, None

    return result, formula_name, base_equation, sub_text


def build_suvat_solution(s_val, u_val, v_val, a_val, t_val, target):
    result, formula_name, base_equation, sub_text = _build_suvat_solution_data(
        s_val, u_val, v_val, a_val, t_val, target
    )
    if result is None:
        return None, formula_name, base_equation, sub_text
    exact_text, _dec_text = format_solution_texts(result)
    return exact_text, formula_name, base_equation, sub_text


def solve_all_variables(s_val, u_val, v_val, a_val, t_val, sig_figs=None):
    return format_all_variables((s_val, u_val, v_val, a_val, t_val), None, sig_figs)


def format_output_with_units(target, exact_text, dec_text, equation, original_eq, sub_text, unit):
    lines = []
    if original_eq != equation:
        lines.append('= ' + original_eq)
    lines.append(equation)
    if sub_text is not None:
        prefix = target + ' = '
        if sub_text.startswith(prefix):
            lines.append('= ' + sub_text[len(prefix):])
        else:
            lines.append('= ' + sub_text)
    lines.append(target + ' = ' + exact_text)
    if dec_text is not None:
        lines.append(target + ' = ' + dec_text + (' ' + unit if unit else ''))
    return lines



def solve_suvat():
    print('Use , to mark exactly one target and enter the other known values.')
    s_text = input('s: ').strip()
    u_text = input('u: ').strip()
    v_text = input('v: ').strip()
    a_text = input('a: ').strip()
    t_text = input('t: ').strip()

    all_text = ' '.join([s_text, u_text, v_text, a_text, t_text]).lower()
    presets = detect_presets(all_text)

    parsed_values = (
        parse_value(s_text),
        parse_value(u_text),
        parse_value(v_text),
        parse_value(a_text),
        parse_value(t_text),
    )
    values, target, target_error = resolve_target_inputs(parsed_values)
    if target_error is not None:
        print(target_error)
        return

    s, u, v, a, t = values

    if presets:
        current_values = [s, u, v, a, t]
        for var, val in presets.items():
            idx = VAR_MAP[var]
            if current_values[idx] is None:
                current_values[idx] = val
        s, u, v, a, t = tuple(current_values)

    result_node, equation, original_eq, sub_text = _build_suvat_solution_data(s, u, v, a, t, target)

    if result_node is None:
        print('Error: ' + equation)
        return

    sig_figs = 3
    for txt in [s_text, u_text, v_text, a_text, t_text]:
        txt = txt.strip()
        if txt and txt != ',' and txt.replace('.', '').replace('-', '').isdigit():
            sf = count_sig_figs(txt)
            if sf > sig_figs:
                sig_figs = sf

    exact_text, dec_text = format_solution_texts(result_node, sig_figs)
    unit = VAR_UNITS.get(target, '')

    print()
    output_lines = format_output_with_units(target, exact_text, dec_text, equation, original_eq, sub_text, unit)
    i = 0
    while i < len(output_lines):
        print(output_lines[i])
        i += 1

    if should_show_all_variables(target, (s, u, v, a, t)):
        print()
        print('--- All variables ---')
        all_results = format_all_variables((s, u, v, a, t), target, sig_figs)
        for var_name in VAR_NAMES:
            if var_name in all_results:
                exact, dec = all_results[var_name]
                unit = VAR_UNITS.get(var_name, '')
                if dec is not None:
                    print(var_name + ' = ' + exact + ' (' + dec + ' ' + unit + ')')
                else:
                    print(var_name + ' = ' + exact + (' ' + unit if unit else ''))
    else:
        print()
        print('--- All variables ---')
        print('Not shown because more than one variable is unknown.')
        print('Use , to mark exactly one target and fill the others to see a full consistent set.')


def main():
    begin_user_action()
    try:
        solve_suvat()
    except Exception as err:
        print('Err: ' + str(err))


run = main
if not SKIP_AUTORUN:
    main()
