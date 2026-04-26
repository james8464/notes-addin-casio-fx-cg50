# Minimal fallback stubs for calculator (no external modules)
# These are used when shared_cache/shared_helpers aren't available

try:
    RecursionError
except NameError:
    RecursionError = RuntimeError

def cache_store(cache, key, value, limit):
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
    return node is not None and node[0] == 'num'

def is_sym(node):
    return node is not None and node[0] == 'sym'

def is_zero(node):
    return is_num(node) and node[1] == 0

def is_one(node):
    return is_num(node) and node[1] == node[2]

def fn(*args):
    return tuple(args)

def neg(node):
    if is_num(node):
        return ('num', -node[1], node[2])
    return ('mul', ('num', -1), node)

def ensure_reasoning_marker(*args):
    # Match shared_helpers signature: may be called as (lines) or (lines, default_prefix).
    if not args:
        return args
    return args[0]

def normalize_input_text(text):
    return text.strip() if isinstance(text, str) else text

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