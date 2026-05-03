#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]


def run_cpp(host: Path, stdin: str) -> str:
    p = subprocess.run(
        [str(host), "--stdin-program", "trigProgram.py"],
        input=stdin,
        text=True,
        capture_output=True,
        cwd=str(REPO),
        timeout=20,
    )
    return p.stdout


def has_answer(text: str) -> bool:
    return any(re.search(r"\banswer:\s*", ln, flags=re.I) for ln in text.splitlines())


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=str(REPO / "c++" / "addin" / "host" / "build" / "casio_host"))
    args = ap.parse_args()

    host = Path(args.host)
    if not host.exists():
        raise SystemExit(f"Missing host bin: {host} (build with ./tools/build_host.sh)")

    cases = [
        # Prove
        ("prove", "1\nsin(x)^2+cos(x)^2\n1\n1\n"),
        # Transform
        ("transform", "2\n1/cos(x)\nsec(x)\n"),
        # Solve
        ("solve", "3\nsin(x)=1/2,x,0,360\n"),
    ]

    bad = 0
    for label, stdin in cases:
        cpp = run_cpp(host, stdin)
        if label == "solve":
            ok = has_answer(cpp) and ("x = [30, 150]" in cpp)
        else:
            ok = has_answer(cpp) and ("err:" not in (cpp or "").lower())
        if not ok:
            bad += 1
            print("MISMATCH", label)
            print("C  has Answer:", has_answer(cpp))
            print("C out head:", "\n".join(cpp.splitlines()[:10]))
            print("")
        else:
            print("OK", label)

    print("done mismatches", bad, "/", len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
