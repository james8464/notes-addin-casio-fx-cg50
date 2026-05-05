from __future__ import annotations


def shrink_expression_text(expr: str) -> str:
    out = (expr or "").strip()
    changed = True
    while changed:
        changed = False
        for old, new in (
            ("+0", ""),
            ("0+", ""),
            ("*1", ""),
            ("1*", ""),
            ("((", "("),
            ("))", ")"),
        ):
            if old in out:
                out = out.replace(old, new)
                changed = True
    if out.startswith("(") and out.endswith(")") and out.count("(") == out.count(")"):
        inner = out[1:-1]
        if inner.count("(") == inner.count(")"):
            out = inner
    return out
