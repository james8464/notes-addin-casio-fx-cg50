"""
Shared helper utilities for CASIO programs.

Keep this file MicroPython v1.9.4 friendly: no f-strings, type hints, or
large runtime dependencies.
"""

try:
    from src.shared_reasoning_markers import REASONING_MARKERS
except ImportError:
    try:
        from shared_reasoning_markers import REASONING_MARKERS
    except ImportError:
        REASONING_MARKERS = ("method:", "use ", "using ", "let ", "solve ", "answer:")

def is_num(node):
    """Check if node is a number."""
    return node is not None and node[0] == 'num'


def is_sym(node):
    """Check if node is a symbol."""
    return node is not None and node[0] == 'sym'


def is_const(node):
    """Check if node is a named constant."""
    return node is not None and node[0] == 'const'


def is_zero(node):
    """Check if node is zero."""
    return is_num(node) and node[1] == 0


def is_one(node):
    """Check if node is one."""
    return is_num(node) and node[1] == node[2]


def is_int_num(node):
    """Check if node is an integer number."""
    return is_num(node) and node[2] == 1


def is_neg_one(node):
    """Check if node is -1."""
    return is_minus_one(node)


def is_minus_one(node):
    """Check if node is -1."""
    return is_num(node) and node[1] == -node[2]


def is_neg(node):
    """Check if node is negative."""
    if is_num(node):
        return node[1] < 0
    return False


def _shared_sig(node):
    kind = node[0]
    if kind in ('num', 'sym', 'const'):
        return node
    if kind == 'fn':
        if node[1] == 'exp':
            return 'pow', ('const', 'e'), _shared_sig(node[2])
        return 'fn', node[1], _shared_sig(node[2])
    if kind in ('pow', 'div'):
        return kind, _shared_sig(node[1]), _shared_sig(node[2])
    if kind in ('add', 'mul'):
        parts = []
        i = 0
        while i < len(node[1]):
            item = node[1][i]
            if item[0] == kind:
                inner = _shared_sig(item)
                if inner[0] == kind:
                    parts.extend(list(inner[1]))
                else:
                    parts.append(inner)
            else:
                parts.append(_shared_sig(item))
            i += 1
        parts.sort()
        return kind, tuple(parts)
    return node


def same(a, b):
    """Check if two expressions are equivalent up to add/mul ordering."""
    if a == b:
        return True
    if a is None or b is None:
        return False
    return _shared_sig(a) == _shared_sig(b)


def same_by_sig(a, b, sig_func, cache=None, cache_store_func=None, cache_limit=None):
    """Check equivalence with the caller's canonical signature function."""
    if a is b or a == b:
        return True
    if a is None or b is None:
        return False
    key = (a, b)
    if cache is not None:
        cached = cache.get(key)
        if cached is not None:
            return cached
    result = sig_func(a) == sig_func(b)
    if cache is not None:
        if cache_store_func is not None and cache_limit is not None:
            cache_store_func(cache, key, result, cache_limit)
            cache_store_func(cache, (b, a), result, cache_limit)
        else:
            cache[key] = result
            cache[(b, a)] = result
    return result


def cheap_same(a, b):
    """Fast structural comparison."""
    if a == b:
        return True
    if a is None or b is None:
        return False
    if a[0] != b[0]:
        return False
    if a[0] in ('num', 'sym', 'const'):
        return False
    if a[0] == 'fn':
        return a[1] == b[1] and a[2] == b[2]
    if a[0] in ('pow', 'div'):
        return a[1] == b[1] and a[2] == b[2]
    if len(a[1]) != len(b[1]):
        return False
    return a[1] == b[1]


def is_num_token_start(text, i):
    """Check if position starts a number token."""
    if i >= len(text):
        return False
    ch = text[i]
    return is_digit_char(ch) or (ch == '.' and i + 1 < len(text) and is_digit_char(text[i + 1]))


def is_digit_char(ch):
    return '0' <= ch <= '9'


def is_alpha_char(ch):
    return ('A' <= ch <= 'Z') or ('a' <= ch <= 'z')


def is_name_start(ch):
    return is_alpha_char(ch) or ch == '_'


def is_name_char(ch):
    return is_name_start(ch) or is_digit_char(ch)


def ensure_reasoning_marker(lines, default_prefix="Method: "):
    """Add a reasoning marker to output lines when one is missing."""
    if not lines:
        return lines
    text = "\n".join(lines).lower()
    i = 0
    while i < len(REASONING_MARKERS):
        if REASONING_MARKERS[i] in text:
            return lines
        i += 1
    lines = list(lines)
    prefixes = ("use", "using", "let", "method", "hence", "therefore", "thus")
    if lines and not lines[0].lower().startswith(prefixes):
        lines.insert(0, default_prefix)
    return lines


def compact_duplicate_answer_lines(lines):
    """Remove immediate pre-answer duplicates like `x = 1` before `Answer: x = 1`."""
    if not lines:
        return lines
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith('Answer: ') and len(out) > 0 and out[-1] == line[8:]:
            out.pop()
        if line.startswith('Solution: ') and i + 1 < len(lines) and lines[i + 1] == 'Answer: ' + line[10:]:
            i += 1
            continue
        out.append(line)
        i += 1
    return out


def fn(name, arg, sim_func=None):
    """Build a function AST node with common aliases."""
    if name == 'ln':
        name = 'log'
    if name == 'csc':
        name = 'cosec'
    node = ('fn', name, arg)
    if sim_func is not None:
        return sim_func(node)
    return node


def neg(node, num_func=None, mul_func=None):
    """Build -node using caller supplied constructors when available."""
    n = ('num', -1, 1)
    if num_func is not None:
        n = num_func(-1)
    if mul_func is not None:
        return mul_func([n, node])
    return 'mul', (n, node)
