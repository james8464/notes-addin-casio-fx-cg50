try:
    import math
except ImportError:
    math = None

try:
    import sys
except ImportError:
    sys = None

SKIP_AUTORUN = sys is not None and getattr(sys, '_algebra_no_autorun', False)

# ============================================================================
# Core Helpers (from algebraProgram)
# ============================================================================

def gcd(a, b):
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


# ============================================================================
# Expression Building
# ============================================================================

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
    return make_add(parts)


def mul(parts):
    return make_mul(parts)


def div(a, b):
    return 'div', a, b


def power(a, b):
    return 'pow', a, b


def fn(name, arg):
    return 'fn', name, arg


def neg(node):
    return mul([num(-1), node])


def sub(a, b):
    return add([a, neg(b)])


def sqrt(node):
    return fn('sqrt', node)


# ============================================================================
# Expression Display
# ============================================================================

SHOW_CACHE = {}


def show(node, parent=0):
    key = (node, parent)
    cached = SHOW_CACHE.get(key)
    if cached is not None:
        return cached
    
    kind = node[0]
    if kind == 'num':
        if node[2] == 1:
            result = str(node[1])
            SHOW_CACHE[key] = result
            return result
        result = str(node[1]) + '/' + str(node[2])
        SHOW_CACHE[key] = result
        return result
    if kind == 'sym':
        SHOW_CACHE[key] = node[1]
        return node[1]
    if kind == 'const':
        SHOW_CACHE[key] = node[1]
        return node[1]
    if kind == 'fn':
        if node[1] == 'sqrt':
            result = 'sqrt(' + show(node[2], 0) + ')'
        else:
            result = node[1] + '(' + show(node[2], 0) + ')'
        SHOW_CACHE[key] = result
        return result
    if kind == 'pow':
        left = show(node[1], 3)
        right = show(node[2], 3)
        if node[1][0] == 'fn' or (is_num(node[1]) and (node[1][2] != 1 or node[1][1] < 0)) or node[1][0] in ('sym', 'mul'):
            left = '(' + left + ')'
        if is_num(node[2]) and node[2][2] != 1:
            right = '(' + right + ')'
        result = left + '^' + right
        SHOW_CACHE[key] = result
        return result
    if kind == 'mul':
        items = node[1] if isinstance(node[1], tuple) else (node[1],)
        parts = []
        i = 0
        while i < len(items):
            part = show(items[i], 2)
            if items[i][0] == 'div' or (is_num(items[i]) and items[i][2] != 1):
                part = '(' + part + ')'
            parts.append(part)
            i += 1
        result = '*'.join(parts)
        SHOW_CACHE[key] = result
        return result
    if kind == 'div':
        left = show(node[1], 0)
        right = show(node[2], 0)
        if node[1][0] in ('add', 'mul') or (is_num(node[1]) and node[1][2] != 1):
            left = '(' + left + ')'
        if node[2][0] in ('add', 'mul', 'div') or (is_num(node[2]) and node[2][2] != 1):
            right = '(' + right + ')'
        result = left + '/' + right
        SHOW_CACHE[key] = result
        return result
    if kind == 'add':
        parts = node[1] if isinstance(node[1], tuple) else (node[1],)
        text = ''
        i = 0
        while i < len(parts):
            item = parts[i]
            if i == 0:
                text = show(item, 1)
            else:
                text += '+' + show(item, 1)
            i += 1
        if parent > 1:
            result = '(' + text + ')'
        else:
            result = text
        SHOW_CACHE[key] = result
        return result
    return str(node)


# ============================================================================
# Simplification
# ============================================================================

def split_coeff(node):
    if is_num(node):
        return (node, num(1))
    if node[0] == 'pow':
        if is_int_num(node[2]) and node[2][1] == 1:
            return (num(1), node[1])
        else:
            return (num(1), node)
    if node[0] == 'mul':
        items = list(node[1])
        if items and is_num(items[0]):
            if len(items) == 2:
                return (items[0], items[1])
            else:
                return (items[0], ('mul', tuple(items[1:])))
        else:
            return (num(1), node)
    else:
        return (num(1), node)


def same(a, b):
    if a is b or a == b:
        return True
    if a[0] != b[0]:
        return False
    if a[0] == 'num':
        return a[1] == b[1] and a[2] == b[2]
    if a[0] == 'sym':
        return a[1] == b[1]
    if a[0] == 'const':
        return a[1] == b[1]
    if a[0] in ('add', 'mul'):
        return a[1] == b[1]
    if a[0] in ('pow', 'div', 'fn'):
        return a[1] == b[1] and a[2] == b[2]
    return False


def sim(node):
    SHOW_CACHE.clear()
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return node
    if kind == 'fn':
        arg = sim(node[2])
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
                key = str(rest)
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
                out.append(neg(rest))
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
        others = []
        i = 0
        while i < len(items):
            item = items[i]
            if is_zero(item):
                return num(0)
            if is_num(item):
                coeff = mulq(coeff, item)
            else:
                others.append(item)
            i += 1
        if is_zero(coeff):
            return num(0)
        if not is_one(coeff):
            others.insert(0, coeff)
        if len(others) == 0:
            return num(1)
        if len(others) == 1:
            return others[0]
        return 'mul', tuple(others)
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
        return 'div', top, bot
    return node


def show_simple(node):
    return show(sim(node))


# ============================================================================
# SUVAT Equations
# ============================================================================

def parse_value(text):
    text = text.strip()
    if text == '':
        return None
    if text.lower() == 'x':
        return 'target'
    return num_text(text)


def build_suvat_solution(s, u, v, a, t, target):
    half = num(1, 2)
    two = num(2)
    
    if target == 'v':
        if u is not None and a is not None and t is not None:
            result = add([u, mul([a, t])])
            return show_simple(result), "v = u + at"
        if s is not None and u is not None and a is not None and t is not None:
            result = div(sub(mul([num(2), s]), mul([u, t])), mul([num(2), t]))
            return show_simple(result), "v = (2s - ut) / 2t"
        if u is not None and a is not None and s is not None:
            result = power(add([power(u, num(2)), mul([num(2), a, s])]), half)
            return show_simple(result), "v = sqrt(u^2 + 2as)"
    
    elif target == 's':
        if u is not None and v is not None and t is not None:
            result = mul([add([u, v]), t])
            result = mul([result, half])
            return show_simple(result), "s = (u+v)/2 * t"
        if u is not None and a is not None and t is not None:
            term1 = mul([u, t])
            term2 = mul([mul([a, t]), t])
            term2 = mul([term2, half])
            result = add([term1, term2])
            return show_simple(result), "s = ut + 1/2at^2"
        if v is not None and a is not None and t is not None:
            term1 = mul([v, t])
            term2 = mul([mul([a, t]), t])
            term2 = mul([term2, half])
            result = sub(term1, term2)
            return show_simple(result), "s = vt - 1/2at^2"
        if v is not None and u is not None and a is not None:
            diff = sub(power(v, num(2)), power(u, num(2)))
            result = div(diff, mul([num(2), a]))
            return show_simple(result), "s = (v^2 - u^2) / 2a"
    
    elif target == 'u':
        if v is not None and a is not None and t is not None:
            result = sub(v, mul([a, t]))
            return show_simple(result), "u = v - at"
        if s is not None and v is not None and t is not None:
            result = div(sub(mul([num(2), s]), mul([v, t])), mul([num(2), t]))
            return show_simple(result), "u = (2s - vt) / 2t"
        if v is not None and a is not None and s is not None:
            inside = sub(power(v, num(2)), mul([num(2), a, s]))
            result = power(inside, half)
            return show_simple(result), "u = sqrt(v^2 - 2as)"
    
    elif target == 'a':
        if v is not None and u is not None and t is not None:
            diff = sub(v, u)
            result = div(diff, t)
            return show_simple(result), "a = (v - u) / t"
        if s is not None and u is not None and t is not None:
            term = mul([num(2), s])
            denom = mul([t, t])
            part = div(term, denom)
            inner = sub(part, mul([num(2), u]))
            result = div(inner, t)
            return show_simple(result), "a = (2s/t^2 - 2u) / t"
        if v is not None and u is not None and s is not None:
            diff = sub(power(v, num(2)), power(u, num(2)))
            result = div(diff, mul([num(2), s]))
            return show_simple(result), "a = (v^2 - u^2) / 2s"
        if v is not None and s is not None and t is not None:
            term = mul([num(2), s])
            numerator = sub(term, mul([v, t]))
            denom = mul([t, t])
            result = div(numerator, denom)
            return show_simple(result), "a = (2s - vt) / t^2"
    
    elif target == 't':
        if v is not None and u is not None and a is not None:
            if is_zero(a):
                return "undefined", "a = 0, cannot solve for t"
            diff = sub(v, u)
            result = div(diff, a)
            return show_simple(result), "t = (v - u) / a"
        if s is not None and u is not None and a is not None:
            disc = add([power(u, num(2)), mul([num(2), a, s])])
            if is_num(disc) and disc[1] < 0:
                return "no real solution", "discriminant negative"
            root = power(disc, half)
            term1 = add([root, neg(u)])
            result1 = div(term1, a)
            term2 = sub(neg(u), root)
            result2 = div(term2, a)
            return show_simple(result1) + " or " + show_simple(result2), "t = (-u ± sqrt(u^2+2as)) / a"
        if s is not None and u is not None and v is not None:
            sum_uv = add([u, v])
            if is_zero(sum_uv):
                return "undefined", "u + v = 0, cannot solve for t"
            result = div(mul([num(2), s]), sum_uv)
            return show_simple(result), "t = 2s / (u + v)"
    
    return None, "insufficient information"


def solve_suvat():
    print('SUVAT Solver')
    print('Enter values for s, u, v, a, t')
    print('Leave blank or enter x for the variable you want to solve for')
    print()
    
    s_text = input('s: ').strip()
    u_text = input('u: ').strip()
    v_text = input('v: ').strip()
    a_text = input('a: ').strip()
    t_text = input('t: ').strip()
    
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
        print('Error: No target variable specified')
        return
    
    result, equation = build_suvat_solution(s, u, v, a, t, target)
    
    print()
    print('Equation used: ' + equation)
    print('Answer: ' + result)


def main():
    solve_suvat()


run = main
if not SKIP_AUTORUN:
    main()
