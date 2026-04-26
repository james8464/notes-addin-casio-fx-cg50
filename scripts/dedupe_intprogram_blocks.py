#!/usr/bin/env python3
"""
One-shot: remove known duplicate function blocks from intProgram.py (lines from audit).
Run from repo root: python3 scripts/dedupe_intprogram_blocks.py
"""
from pathlib import Path

def main():
    path = Path(__file__).resolve().parents[1] / "src" / "Math" / "intProgram.py"
    lines = path.read_text(encoding="utf-8").splitlines(keepends=True)
    n = len(lines)
    # 1-based inclusive ranges to drop
    one_based = [
        (133, 148),
        (2435, 2499),
        (5012, 5118),
        (5211, 5357),
        (7188, 7233),
    ]
    drop = set()
    for a, b in one_based:
        for i in range(a, b + 1):
            idx = i - 1
            if 0 <= idx < n:
                drop.add(idx)
    new_lines = [ln for i, ln in enumerate(lines) if i not in drop]
    text = "".join(new_lines)

    help_block = (
        "def cancellation_requested():\n"
        "    return False\n\n\n"
        "def integral_candidate_score(title, ans, lines):\n"
        "    if ans is None:\n"
        "        return (10**9, 10**9, 10**9)\n"
        "    title_score = {\n"
        "        'std': 0, 'f(ax+b)': 1, 'Rev chain': 2, 'Subst': 3, 'Trig id': 4,\n"
        "        'Parts': 5, 'Pfrac': 6, 'Poly div': 6, 'PolyQ exp': 2,\n"
        "    }.get(title, 7)\n"
        "    return (len(pretty(ans)), len(lines or []), title_score)\n\n"
    )
    # Insert before first "def fallback_attempts" in result (the active one for solve())
    mark = "def fallback_attempts(node, var, forced_u=None):"
    pos = text.find("\n" + mark)
    if pos == -1:
        pos = text.find(mark)
    if pos == -1:
        raise SystemExit("fallback_attempts not found")
    # insert after newline if we found it with leading \n
    if text[pos:].startswith("\n" + mark):
        insert_at = pos + 1
    else:
        insert_at = 0
    if help_block not in text:
        text = text[:insert_at] + help_block + text[insert_at:]

    path.write_text(text, encoding="utf-8")
    print("wrote", path, "dropped", len(drop), "lines, inserted helpers before fallback_attempts")


if __name__ == "__main__":
    main()
