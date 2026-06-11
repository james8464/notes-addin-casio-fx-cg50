#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "working_engine" / "host" / "build" / "casio_host"

CASES = [
    (
        "xform((sec(x)+tan(x))^2,(1+sin(x))/(1-sin(x)))",
        [
            "normal:",
            "Use cos^2=1-sin^2",
            "Cancel common factor",
            "Target:",
        ],
    ),
    (
        "xform((x+1)^2,x^2+2*x+1)",
        [
            "texpand:",
            "x^2+2*x+1",
        ],
    ),
    (
        "xform(((cos(3*x)/sin(x))+((sin(3*x))/cos(x))),2*cot(2*x))",
        [
            "t=tan(x/2)",
            "Simplify in t",
            "2*cot(2*x)",
        ],
    ),
    (
        "xform(cos(x)^2=8*sin(x)^2-6*sin(x),(3*sin(x)-1)^2=2)",
        [
            "cos^2=1-sin^2",
            "9sin(x)^2",
            "(3sin(x)-1)^2=2",
        ],
    ),
    (
        "xform((3*sin(x)-1)^2=2,cos(x)^2=(8*sin(x)^2-6*sin(x)))",
        [
            "Expand",
            "sin^2+cos^2=1",
            "cos(x)^2=8sin(x)^2-6sin(x)",
        ],
    ),
    (
        "xform(2*cot(x)^2-9*cosec(x)-3=0,5*sin(x)^2+9*sin(x)-2=0)",
        [
            "cot=cos/sin",
            "csc=1/sin",
            "*sin(u)^2",
            "cos^2=1-sin^2",
            "5*sin(x)^2+9*sin(x)-2=0",
        ],
    ),
    (
        "xform(3*cot(2*x)^2-4*cosec(2*x)+1=0,-2*sin(2*x)^2-4*sin(2*x)+3=0)",
        [
            "cot=cos/sin",
            "csc=1/sin",
            "*sin(u)^2",
            "cos^2=1-sin^2",
            "-2*sin(2*x)^2-4*sin(2*x)+3=0",
        ],
    ),
    (
        "xform(cot(t)^2+csc(t)-2=0,-3*sin(t)^2+sin(t)+1=0)",
        [
            "cot=cos/sin",
            "csc=1/sin",
            "*sin(u)^2",
            "cos^2=1-sin^2",
            "-3*sin(t)^2+sin(t)+1=0",
        ],
    ),
    (
        "xform((sin(x)-1)^2=2,cos(x)^2=(8*sin(x)^2-6*sin(x)))",
        [
            "Search:",
            "Target",
            "cos(x)^2=(8*sin(x)^2-6*sin(x))",
        ],
    ),
]

FORBIDDEN = [
    "xform(start,target)",
    "no route",
    "not equivalent",
    "not verified",
    "Verified",
    "Warning adding",
    "syntax error",
    "Bad Argument",
    "//",
    "Error:",
]


def run(expr: str) -> str:
    env = dict(os.environ, CASCAS_HOST_PRODUCTION="1")
    proc = subprocess.run(
        [str(RUNNER), "--alg", expr],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=30,
        env=env,
    )
    if proc.returncode:
        raise AssertionError(f"{expr}: rc={proc.returncode}\n{proc.stdout}\n{proc.stderr}")
    return proc.stdout.strip()


def main() -> int:
    subprocess.check_call(
        ["cmake", "--build", str(ROOT / "tools" / "working_engine" / "host" / "build")],
        cwd=ROOT,
        stdout=subprocess.DEVNULL,
    )
    for expr, markers in CASES:
        out = run(expr)
        flat = out.replace(" ", "")
        missing = [m for m in markers if m not in out and m.replace(" ", "") not in flat]
        if missing:
            raise AssertionError(f"{expr}: missing {missing}\n{out}")
        bad = [m for m in FORBIDDEN if m in out]
        if bad:
            raise AssertionError(f"{expr}: forbidden {bad}\n{out}")
    print(f"OK xform general planner cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
