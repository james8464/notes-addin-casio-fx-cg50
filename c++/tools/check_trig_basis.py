#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAIN = ROOT / "c++/khicas/upstream/giac90_1addin/main.cc"
CAT_EN = ROOT / "c++/khicas/upstream/giac90_1addin/catalogen.cpp"
CAT_FR = ROOT / "c++/khicas/upstream/giac90_1addin/catalogfr.cpp"
HELP = ROOT / "c++/prizm/help/CASIOCAS.HLP"


def fail(msg: str) -> int:
    print(f"FAIL {msg}")
    return 1


def main() -> int:
    main = MAIN.read_text(errors="ignore")
    cat_en = CAT_EN.read_text(errors="ignore")
    cat_fr = CAT_FR.read_text(errors="ignore")
    help_text = HELP.read_text(errors="ignore") if HELP.exists() else ""

    required_main = [
        "cascas_rewrite_trig_basis_call",
        "sincos(",
        "texpand(",
        "tcollect(",
        "factor(normal(",
        "out += \"=0\"",
        "cascas_rewrite_trig_basis_call(input,\"trigcos(\"",
        "cascas_rewrite_trig_basis_call(input,\"trigsin(\"",
        "cascas_rewrite_trig_basis_call(input,\"trigtan(\"",
        "sin^2=1-cos^2; tan^2=1/cos^2-1.",
        "cos^2=1-sin^2; tan^2=sin^2/(1-sin^2).",
        "sin^2=tan^2/(1+tan^2); cos^2=1/(1+tan^2).",
        "5. sin/cos; expd; coll; fact.",
    ]
    missing = [x for x in required_main if x not in main]
    if missing:
        return fail("trig-basis source markers missing: " + ", ".join(missing))

    for name, text in (("catalogen", cat_en), ("catalogfr", cat_fr)):
        for fn in ("trigcos(expr/eq)", "trigsin(expr/eq)", "trigtan(expr/eq)"):
            if fn not in text:
                return fail(f"{name} missing {fn}")
    if "sin(x)^4+tan(x)^2=cos(x)^2" not in help_text:
        return fail("external help missing extreme trigcos example")

    print("OK trig-basis policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
