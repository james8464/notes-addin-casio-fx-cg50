#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HELP = ROOT / "c++/prizm/help/CASIOCAS.HLP"

BAD = (
    "PReq",
    "Req/Opt",
    "Req before []. Opt inside []",
    "Req before []",
    "Opt inside []",
)

EXEMPT_EXAMPLES = {
    "loop for",
    "loop in list",
    "loop while",
    "test if",
    "test else",
    "function def",
    "local j,k;",
    "return res",
    "edit list",
    "edit matrix",
}


def records() -> list[tuple[str, list[str]]]:
    out: list[tuple[str, list[str]]] = []
    name: str | None = None
    body: list[str] = []
    for raw in HELP.read_text(encoding="utf-8", errors="ignore").splitlines():
        if raw.startswith("@END"):
            if name:
                out.append((name, body))
            name = None
            body = []
        elif raw.startswith("@"):
            name = raw[1:].strip()
            body = []
        elif name:
            body.append(raw)
    return out


def main() -> int:
    bad: list[str] = []
    for name, body in records():
        text = "\n".join(body)
        for phrase in BAD:
            if phrase in text:
                bad.append(f"{name}: vague phrase {phrase!r}")
                break
        if name not in EXEMPT_EXAMPLES and "(" in name and "Ex F" not in text:
            bad.append(f"{name}: missing Ex F example")
        if "(" in name and "Req:" not in text and "Args:" not in text:
            bad.append(f"{name}: missing input/parameter line")
    if bad:
        print("FAIL help quality")
        for item in bad[:80]:
            print(item)
        if len(bad) > 80:
            print(f"... {len(bad)-80} more")
        return 1
    print(f"OK help quality records={len(records())}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
