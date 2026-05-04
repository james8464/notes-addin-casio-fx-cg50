#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FORMAT_EXAM = ROOT / "c++/addin/src/core/format_exam.cpp"


def fail(msg: str) -> int:
    print(f"FAIL {msg}")
    return 1


def main() -> int:
    src = FORMAT_EXAM.read_text(errors="ignore")
    required = [
        "FORMAT_EXAM_MAX_DEPTH",
        "format_equation_human_readable_impl",
        "if(depth > FORMAT_EXAM_MAX_DEPTH)",
        "NodeId s = simplify(arena, node);",
        "format_equation_human_readable_impl(arena, s, parent, 0)",
    ]
    missing = [m for m in required if m not in src]
    if missing:
        return fail("formatter memory guard missing: " + ", ".join(missing))

    public_pos = src.find("std::string format_equation_human_readable(")
    impl_pos = src.find("format_equation_human_readable_impl")
    if public_pos < 0 or impl_pos < 0:
        return fail("formatter public/impl split missing")
    body = src[impl_pos:public_pos]
    if "simplify(arena" in body:
        return fail("formatter recursively simplifies while rendering")

    print("OK formatter memory policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
