# Minimal fallback stubs for calculator (no external modules)
# These are used when shared_cache/shared_helpers aren't available

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

def ensure_reasoning_marker(text):
    return text

def normalize_input_text(text):
    return text.strip() if isinstance(text, str) else text

def same_by_sig(a, b, sig_func=None, cache=None, cache_store_func=None, cache_limit=None):
    return a == b

def compact_duplicate_answer_lines(lines):
    return lines

REASONING_MARKERS = ("method:", "use ", "using ", "let ", "solve ", "answer:")