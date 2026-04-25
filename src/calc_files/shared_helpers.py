# Minimal stub for calculator
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

def fn(name, *args):
    return (name,) + args

def neg(node):
    if is_num(node):
        return ('num', -node[1], node[2])
    return ('mul', ('num', -1), node)

def ensure_reasoning_marker(text):
    return text

def normalize_input_text(text):
    return text.strip()

def same_by_sig(a, b):
    return a == b