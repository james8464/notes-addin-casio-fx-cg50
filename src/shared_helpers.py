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

# Mathematical constants
E = ("const", "e")
PI = ("const", "pi")


def casio_hw_sim_from_env():
    """
    True when host sets CASIO_HW_SIM (used by run_device_sim.py for desktop
    "calculator" cache limits; not the same as OS memory capping).
    """
    try:
        import os
        v = os.environ.get("CASIO_HW_SIM", "")
    except (AttributeError, OSError, TypeError, ImportError):
        return False
    if v is None:
        return False
    v = v.lower()
    return v in ("1", "true", "yes", "y", "on")


def is_num(node):
    """Check if node is a number."""
    return isinstance(node, (tuple, list)) and len(node) > 0 and node[0] == 'num'


def is_sym(node):
    """Check if node is a symbol."""
    return isinstance(node, (tuple, list)) and len(node) > 0 and node[0] == 'sym'


def is_const(node):
    """Check if node is a named constant."""
    return isinstance(node, (tuple, list)) and len(node) > 0 and node[0] == 'const'


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


def _previous_significant_char(text, index):
    i = index - 1
    while i >= 0:
        ch = text[i]
        if ch not in " \t\r\n":
            return ch
        i -= 1
    return None


def _should_open_abs_pipe(text, index):
    prev = _previous_significant_char(text, index)
    if prev is None:
        return True
    return prev in "([{,+-*/^=<>"


def _convert_abs_pipes(text):
    if text.count('|') % 2 != 0:
        return text
    out = []
    depth = 0
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == '|':
            if depth == 0 or _should_open_abs_pipe(text, i):
                out.append('abs(')
                depth += 1
            else:
                out.append(')')
                depth -= 1
        else:
            out.append(ch)
        i += 1
    if depth != 0:
        return text
    return ''.join(out)


def normalize_input_text(text):
    """Normalize user-facing maths syntax into parser-friendly ASCII."""
    if text is None:
        return text
    text = text.strip()
    text = text.replace('\u2212', '-')
    text = text.replace('\u2013', '-')
    text = text.replace('\u2014', '-')
    text = text.replace('\u00d7', '*')
    text = text.replace('\u2217', '*')
    text = text.replace('\u22c5', '*')
    text = text.replace('\u00f7', '/')
    text = text.replace('\u2044', '/')
    text = text.replace('\u03c0', 'pi')
    text = text.replace('\u03a0', 'pi')
    text = text.replace('\u00b0', '')
    text = text.replace('\u00bd', '(1/2)')
    text = text.replace('\u00bc', '(1/4)')
    text = text.replace('\u00be', '(3/4)')
    text = _normalize_superscripts(text)
    text = _normalize_inverse_trig_notation(text)
    text = _normalize_sqrt_symbol(text)
    if '|' in text:
        text = _normalize_function_pipe_calls(text)
        text = _convert_abs_pipes(text)
    text = _normalize_compact_function_words(text)
    return text


_SUPERSCRIPT_DIGITS = {
    '\u2070': '0', '\u00b9': '1', '\u00b2': '2', '\u00b3': '3',
    '\u2074': '4', '\u2075': '5', '\u2076': '6', '\u2077': '7',
    '\u2078': '8', '\u2079': '9', '\u207b': '-', '\u207a': '+',
}


def _normalize_superscripts(text):
    out = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch in _SUPERSCRIPT_DIGITS:
            run = []
            while i < len(text) and text[i] in _SUPERSCRIPT_DIGITS:
                run.append(_SUPERSCRIPT_DIGITS[text[i]])
                i += 1
            out.append('^' + ''.join(run))
            continue
        out.append(ch)
        i += 1
    return ''.join(out)


def _normalize_sqrt_symbol(text):
    if '\u221a' not in text:
        return text
    out = []
    i = 0
    while i < len(text):
        if text[i] != '\u221a':
            out.append(text[i])
            i += 1
            continue
        i += 1
        while i < len(text) and text[i] in ' \t':
            i += 1
        if i < len(text) and text[i] == '(':
            out.append('sqrt')
            continue
        start = i
        if i < len(text) and (is_name_start(text[i]) or is_digit_char(text[i]) or text[i] == '.'):
            i += 1
            while i < len(text) and (is_name_char(text[i]) or text[i] == '.'):
                i += 1
            out.append('sqrt(' + text[start:i] + ')')
        else:
            out.append('sqrt')
    return ''.join(out)


_COMPACT_FUNC_WORDS = (
    'arcsin', 'arccos', 'arctan',
    'cosec',
    'asin', 'acos', 'atan',
    'sinh', 'cosh', 'tanh',
    'sqrt', 'sin', 'cos', 'tan', 'sec', 'cot',
    'abs', 'exp', 'log', 'ln', 'csc',
)


def _normalize_inverse_trig_notation(text):
    pairs = (
        ('arcsin^-1', 'asin'),
        ('arccos^-1', 'acos'),
        ('arctan^-1', 'atan'),
        ('sin**-1', 'asin'),
        ('cos**-1', 'acos'),
        ('tan**-1', 'atan'),
        ('sin^-1', 'asin'),
        ('cos^-1', 'acos'),
        ('tan^-1', 'atan'),
    )
    i = 0
    while i < len(pairs):
        text = text.replace(pairs[i][0], pairs[i][1])
        i += 1
    return text


def _split_compact_function_word(word):
    low = word.lower()
    if low in _COMPACT_FUNC_WORDS or low in ('pi', 'e'):
        return None, None
    i = 0
    while i < len(_COMPACT_FUNC_WORDS):
        token = _COMPACT_FUNC_WORDS[i]
        if low.startswith(token) and len(word) > len(token):
            rest = word[len(token):]
            name = token
            if name == 'ln':
                name = 'log'
            elif name == 'csc':
                name = 'cosec'
            return name, rest
        i += 1
    return None, None


def _normalize_compact_function_words(text):
    out = []
    i = 0
    while i < len(text):
        ch = text[i]
        if is_name_start(ch):
            j = i + 1
            while j < len(text) and is_name_char(text[j]):
                j += 1
            word = text[i:j]
            name, rest = _split_compact_function_word(word)
            if name is not None:
                out.append(name)
                out.append('(')
                out.append(_normalize_compact_function_words(rest))
                out.append(')')
            else:
                out.append(word)
            i = j
            continue
        out.append(ch)
        i += 1
    return ''.join(out)


def _normalize_function_pipe_calls(text):
    out = []
    i = 0
    while i < len(text):
        matched = False
        j = 0
        while j < len(_COMPACT_FUNC_WORDS):
            token = _COMPACT_FUNC_WORDS[j]
            end = i + len(token)
            if text[i:end].lower() == token:
                prev_ok = i == 0 or not is_name_char(text[i - 1])
                if prev_ok and end < len(text) and text[end] == '|':
                    close = end + 1
                    while close < len(text) and text[close] != '|':
                        close += 1
                    if close < len(text):
                        out.append(text[i:end])
                        out.append('(')
                        out.append(text[end:close + 1])
                        out.append(')')
                        i = close + 1
                        matched = True
                        break
            j += 1
        if matched:
            continue
        out.append(text[i])
        i += 1
    return ''.join(out)


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
