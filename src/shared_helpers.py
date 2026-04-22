"""
Shared helper utilities for CASIO programs.
These functions are duplicated across programs - intended for future consolidation.
"""

def is_num(node):
    """Check if node is a number."""
    return node and node[0] == 'num'


def is_sym(node):
    """Check if node is a symbol."""
    return node and node[0] == 'sym'


def is_zero(node):
    """Check if node is zero."""
    return is_num(node) and node[1] == 0


def is_one(node):
    """Check if node is one."""
    return is_num(node) and node[1] == node[2] == 1


def is_neg_one(node):
    """Check if node is -1."""
    return is_num(node) and node[1] == -1 and node[2] == 1


def is_neg(node):
    """Check if node is negative."""
    if is_num(node):
        return node[1] < 0
    return False


def same(a, b):
    """Check if two expressions are structurally identical."""
    if a == b:
        return True
    if a is None or b is None:
        return False
    if a[0] != b[0]:
        return False
    if a[0] == 'num':
        return a[1] == b[1] and a[2] == b[2]
    if a[0] == 'sym':
        return a[1] == b[1]
    if a[0] == 'add':
        return len(a) == len(b) and all(same(a[i], b[i]) for i in range(1, len(a)))
    if a[0] == 'mul':
        return len(a) == len(b) and all(same(a[i], b[i]) for i in range(1, len(a)))
    if a[0] == 'pow':
        return same(a[1], b[1]) and same(a[2], b[2])
    if a[0] == 'div':
        return same(a[1], b[1]) and same(a[2], b[2])
    if a[0] == 'fn':
        return a[1] == b[1] and same(a[2], b[2])
    return False


def cheap_same(a, b):
    """Fast structural comparison."""
    return same(a, b)


def is_num_token_start(text, i):
    """Check if position starts a number token."""
    if i < len(text):
        ch = text[i]
        return ch.isdigit() or ch == '.'
    return False