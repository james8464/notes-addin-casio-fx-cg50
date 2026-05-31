#!/usr/bin/env python3
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"

QUALITY_CASES = [
    (
        "int(2*x+3,x)",
        4,
        [
            "Integrate term by term:",
            "Use int(a*x) dx = a*x^2/2",
            "Answer: x^2 + 3*x + C",
        ],
    ),
    (
        "integrate(2*x+3,x)",
        4,
        [
            "Integrate term by term:",
            "Use int(a*x) dx = a*x^2/2",
            "Answer: x^2 + 3*x + C",
        ],
    ),
    (
        "int(cos(x),x)",
        4,
        [
            "Use formula: int(cos(x)) dx = sin(x) + C",
            "d/dx sin(x)=cos(x)",
            "Answer: sin(x) + C",
        ],
    ),
    (
        "int(sec(x)^2,x)",
        4,
        [
            "Use formula: int(sec(x)^2) dx = tan(x) + C",
            "d/dx tan(x)=sec(x)^2",
            "Answer: tan(x) + C",
        ],
    ),
    (
        "int(1/x,x)",
        4,
        [
            "x != 0",
            "d/dx ln(abs(x))=1/x",
            "Answer: ln(abs(x)) + C",
        ],
    ),
    (
        "range(x^2+4*x+7)",
        4,
        [
            "vertex",
            "x = -2",
            "minimum y = 3",
            "Answer: y >= 3",
        ],
    ),
]


def nonblank_lines(text: str) -> list[str]:
    return [line.strip() for line in text.splitlines() if line.strip()]


def main() -> int:
    bad = []
    for expr, min_lines, markers in QUALITY_CASES:
        proc = subprocess.run([str(RUNNER), expr], cwd=ROOT, text=True, capture_output=True)
        out = (proc.stdout or "") + (proc.stderr or "")
        lines = nonblank_lines(out)
        missing = [marker for marker in markers if marker not in out]
        if proc.returncode or len(lines) < min_lines or missing:
            bad.append((expr, proc.returncode, len(lines), missing, out))
    if bad:
        for expr, code, line_count, missing, out in bad:
            print(f"FAIL {expr}: rc={code} lines={line_count} missing={missing}")
            print(out)
        return 1
    print(f"OK working quality cases={len(QUALITY_CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
