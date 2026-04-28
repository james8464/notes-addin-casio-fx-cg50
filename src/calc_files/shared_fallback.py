# Minimal fallback stubs for calculator builds with no external modules.
# Keep these intentionally small: they mirror the shared helpers closely enough
# that copied program files still run if shared_cache/shared_helpers are absent.

try:
    RecursionError
except NameError:
    RecursionError = RuntimeError

def cache_store(cache, key, value, limit):
    """Tiny bounded cache store for calculator memory limits."""
    if limit is not None and limit > 0 and key not in cache and len(cache) >= limit:
        keys = list(cache.keys())
        target = limit - (limit // 8)
        if target >= limit:
            target = limit - 1
        if target < 0:
            target = 0
        i = 0
        while len(cache) > target and i < len(keys):
            try:
                del cache[keys[i]]
            except KeyError:
                pass
            i += 1
    cache[key] = value
    return value

def clear_all_caches(*caches):
    for c in caches:
        if hasattr(c, 'clear'):
            c.clear()

def enforce_total_cache_limit(caches, total_limit):
    pass

E = ("const", "e")
PI = ("const", "pi")

def is_num(node):
    return isinstance(node, (tuple, list)) and len(node) > 0 and node[0] == 'num'

def is_sym(node):
    return isinstance(node, (tuple, list)) and len(node) > 0 and node[0] == 'sym'

def is_zero(node):
    return is_num(node) and node[1] == 0

def is_one(node):
    return is_num(node) and node[1] == node[2]

def fn(name, arg, sim_func=None):
    if name == 'ln':
        name = 'log'
    if name == 'csc':
        name = 'cosec'
    node = ('fn', name, arg)
    if sim_func is not None:
        return sim_func(node)
    return node

def neg(node, num_func=None, mul_func=None):
    if is_num(node):
        return ('num', -node[1], node[2])
    n = ('num', -1, 1)
    if num_func is not None:
        n = num_func(-1)
    if mul_func is not None:
        return mul_func([n, node])
    return ('mul', (n, node))

def compact_working_lines(lines):
    """Trim repeated or purely scaffolding lines before printing answers."""
    if not lines:
        return lines
    out = []
    i = 0
    while i < len(lines):
        line = str(lines[i]).strip()
        low = line.lower()
        if line == "":
            i += 1
            continue
        if low.startswith("method:") or low.startswith("attempt "):
            i += 1
            continue
        if low.startswith("equation 1:") or low.startswith("input = ") or low.startswith("expr = "):
            i += 1
            continue
        if low.startswith("solve for ") and low.endswith(":"):
            i += 1
            continue
        if low in ("no solution exists", "all x satisfy this identity"):
            i += 1
            continue
        if low in ("simplify", "simplify.", "diff", "differentiate", "term by term differentiation"):
            i += 1
            continue
        if line.startswith("Step ") and ": " in line:
            line = line.split(": ", 1)[1].strip()
        if line.startswith("Answer: ") and len(out) > 0 and out[-1] == line[8:].strip():
            out.pop()
        if len(out) == 0 or out[-1] != line:
            out.append(line)
        i += 1
    return out

def ensure_reasoning_marker(*args):
    # Match shared_helpers signature: may be called as (lines) or (lines, default_prefix).
    if not args:
        return args
    return compact_working_lines(args[0])

def _previous_significant_char(text, index):
    i = index - 1
    while i >= 0:
        if text[i] not in " \t\r\n":
            return text[i]
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
    return ''.join(out)

def normalize_input_text(text):
    """Normalise the same common calculator symbols as shared_helpers."""
    if not isinstance(text, str):
        return text
    text = text.strip()
    text = text.replace('−', '-')
    text = text.replace('–', '-')
    text = text.replace('—', '-')
    text = text.replace('×', '*')
    text = text.replace('∗', '*')
    text = text.replace('⋅', '*')
    text = text.replace('÷', '/')
    text = text.replace('⁄', '/')
    text = text.replace('π', 'pi')
    text = text.replace('Π', 'pi')
    text = text.replace('°', '')
    text = text.replace('½', '(1/2)')
    text = text.replace('¼', '(1/4)')
    text = text.replace('¾', '(3/4)')
    text = _normalize_superscripts(text)
    text = _normalize_sqrt_symbol(text)
    if '|' in text:
        text = _convert_abs_pipes(text)
    return text


_SUPERSCRIPT_DIGITS = {
    '⁰': '0', '¹': '1', '²': '2', '³': '3', '⁴': '4',
    '⁵': '5', '⁶': '6', '⁷': '7', '⁸': '8', '⁹': '9',
    '⁻': '-', '⁺': '+',
}


def _is_digit_char(ch):
    return '0' <= ch <= '9'


def _is_alpha_char(ch):
    return ('A' <= ch <= 'Z') or ('a' <= ch <= 'z')


def _is_name_start(ch):
    return _is_alpha_char(ch) or ch == '_'


def _is_name_char(ch):
    return _is_name_start(ch) or _is_digit_char(ch)


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
    if '√' not in text:
        return text
    out = []
    i = 0
    while i < len(text):
        if text[i] != '√':
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
        if i < len(text) and (_is_name_start(text[i]) or _is_digit_char(text[i]) or text[i] == '.'):
            i += 1
            while i < len(text) and (_is_name_char(text[i]) or text[i] == '.'):
                i += 1
            out.append('sqrt(' + text[start:i] + ')')
        else:
            out.append('sqrt')
    return ''.join(out)

def same_by_sig(a, b, sig_func=None, cache=None, cache_store_func=None, cache_limit=None):
    return a == b

def compact_duplicate_answer_lines(lines):
    return lines

REASONING_MARKERS = ("method:", "use ", "using ", "let ", "solve ", "answer:")


def casio_hw_sim_from_env():
    try:
        import os
        v = os.environ.get("CASIO_HW_SIM", "")
    except (AttributeError, OSError, TypeError, ImportError):
        return False
    if v is None:
        return False
    v = v.lower()
    return v in ("1", "true", "yes", "y", "on")
