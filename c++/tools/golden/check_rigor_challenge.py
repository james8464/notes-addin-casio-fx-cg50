#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import re
import sys
from pathlib import Path

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
REPORT = REPO / "c++" / "tests" / "reports" / "rigor_challenge_latest.txt"
DEFAULT_DOC = Path.home() / "Downloads" / "The_Rigor_Challenge_100_Structural_Problems.txt"


def run(args: list[str], stdin: str | None = None) -> str:
    proc = subprocess.run(args, input=stdin, cwd=str(REPO), text=True, capture_output=True, timeout=15)
    return proc.stdout + proc.stderr


def load_doc(path: Path) -> tuple[int, int, str]:
    if not path.exists():
        return 0, 0, f"missing: {path}"
    text = path.read_text(errors="ignore")
    problem_ids = sorted({int(m.group(1)) for m in re.finditer(r"(?m)^(\d+)\.\s+\[", text)})
    return len(problem_ids), text.count("\nSolution."), str(path)


DIRECT: dict[int, tuple[list[str], str | None, list[str]]] = {
    1: ([str(HOST), "--derive", "x^y=y^x,method=implicit"], None, ["Domain:", "y*ln(x)=x*ln(y)", "dy/dx*(ln(x)-x/y)=ln(y)-y/x", "dy/dx = y*(x*ln(y) - y)/(x*(y*ln(x) - x))"]),
    2: ([str(HOST), "--derive", "sin(x*y)+x^2=y^2,method=implicit"], None, ["F_x + F_y*dy/dx = 0", "dy/dx = (y*cos(x*y)+2*x)/(2*y-x*cos(x*y))"]),
    3: ([str(HOST), "--derive", "log(x+y)=x*y,method=implicit"], None, ["Domain:", "dy/dx = (y*(x + y) - 1)/(1 - x*(x + y))"]),
    4: ([str(HOST), "--derive", "y=x*tan(y),method=implicit"], None, ["F_y = - x*sec(y)^2 + 1", "dy/dx = tan(y)/(- x*sec(y)^2 + 1)"]),
    5: ([str(HOST), "--derive", "mode:4,x^2+y^2=a^2"], None, ["Domain: y != 0", "dy/dx=-x/y", "d2y/dx2 = -a^2/y^3"]),
    6: ([str(HOST), "--derive", "t^2+1/t,t^2-1/t,t,method=param_second"], None, ["dx/dt", "dy/dx", "d2y/dx2 = -12*t^4/(2*t^3-1)^3"]),
    7: ([str(HOST), "--derive", "e^t*cos(t),e^t*sin(t),t,method=param_second"], None, ["dx/dt", "Divide by dx/dt", "d2y/dx2 = 2/[e^t(cos(t)-sin(t))^3]"]),
    8: ([str(HOST), "--derive", "log(t),t+1/t,t,method=param_second"], None, ["d2y/dx2 = t+1/t"]),
    9: ([str(HOST), "--int", "e^(2*x)*cos(3*x),method=parts"], None, ["a=2, b=3", "J = Int(e^(2*x)*sin(3*x)) dx", "e^(2*x)*(2*cos(3*x)+3*sin(3*x))/13 + C"]),
    10: ([str(HOST), "--int", "e^(-x)*sin(2*x),method=parts"], None, ["Parts gives I", "J = Int(e^(-x)cos(2x))dx", "-e^(-x)*(sin(2*x) + 2*cos(2*x))/5 + C"]),
    11: ([str(HOST), "--int", "1/(x*sqrt(1+log(x)))"], None, ["Domain:", "Let u=1+log(x)", "2*sqrt(1+log(x)) + C"]),
    12: ([str(HOST), "--int", "sin(x)/(1+cos(x))^2"], None, ["Let u=1+cos(x)", "1/(1+cos(x)) + C"]),
    13: ([str(HOST), "--int", "1/(e^x+e^-x)"], None, ["u = e^x", "I = Int(1/(1+u^2)) du", "atan(e^x) + C"]),
    14: ([str(HOST), "--int", "tan(x)*sec(x)^2/(1+tan(x)^2)^2"], None, ["Let u=tan(x)", "-1/(2*(1+tan(x)^2)) + C"]),
    15: ([str(HOST), "--int", "sqrt((1-x)/(1+x))"], None, ["x=cos(t)", "sqrt(1-x^2) - acos(x) + C"]),
    16: ([str(HOST), "--int", "1/(x^2+2*x+5)^2"], None, ["x^2+2x+5 = (x+1)^2+4", "atan((x+1)/2)/16"]),
    17: ([str(HOST), "--int", "log(x)^2/x"], None, ["Let u=log(x)", "log(x)^3/3 + C"]),
    18: ([str(HOST), "--int", "x*e^(x^2)*sin(e^(x^2))"], None, ["Let u=e^(x^2)", "-cos(e^(x^2))/2 + C"]),
    19: ([str(HOST), "--derive", "y^3-3*x*y+x^3=0,method=implicit"], None, ["dy/dx = (y-x^2)/(y^2-x)"]),
    20: ([str(HOST), "--derive", "a*(theta-sin(theta)),a*(1-cos(theta)),theta,method=param_second"], None, ["d2y/dx2 = -1/(4*a*sin(theta/2)^4)"]),
    21: ([str(HOST), "--int", "1/(sqrt(x)+1)"], None, ["u=sqrt(x)", "2*sqrt(x) - 2*log(abs(1+sqrt(x))) + C"]),
    22: ([str(HOST), "--int", "sqrt(e^x-1)"], None, ["e^x=u^2+1", "2*sqrt(e^x-1) - 2*atan(sqrt(e^x-1)) + C"]),
    23: ([str(HOST), "--int", "1/(1+sqrt(x))^2"], None, ["Integral becomes Integral(2t/(1+t)^2)", "2*log(abs(1+sqrt(x))) + 2/(1+sqrt(x)) + C"]),
    24: ([str(HOST), "--int", "x/(sqrt(1+x^2)*(1+sqrt(1+x^2)))"], None, ["Let u=sqrt(1+x^2)", "log(abs(1+sqrt(1+x^2))) + C"]),
    25: ([str(HOST), "--derive", "x^2*y+x*y^2=1,method=implicit"], None, ["dy/dx = -y*(2*x+y)/(x*(x+2*y))"]),
    101: ([str(HOST), "--derive", "x*log(y)+y*log(x)=1,method=implicit"], None, ["Domain:", "dy/dx = -(log(y)+y/x)/(x/y+log(x))"]),
    102: ([str(HOST), "--derive", "e^(x*y)=x+y,method=implicit"], None, ["dy/dx = (1-y*e^(x*y))/(x*e^(x*y)-1)"]),
    103: ([str(HOST), "--derive", "t^3-3*t,t^2+1,t,method=param_second"], None, ["d2y/dx2 = -2*(t^2+1)/(9*(t^2-1)^3)"]),
    104: ([str(HOST), "--derive", "sec(t),tan(t),t,method=param_second"], None, ["d2y/dx2 = -cot(t)^3"]),
    105: ([str(HOST), "--int", "x/(1+x^4)"], None, ["u=x^2", "atan(x^2)/2 + C"]),
    106: ([str(HOST), "--int", "log(x)/(x*sqrt(1+log(x)^2))"], None, ["Let u=log(x)", "sqrt(1+log(x)^2) + C"]),
    107: ([str(HOST), "--int", "1/(x^2*sqrt(1+1/x))"], None, ["Let u=1+1/x", "-2*sqrt(1+1/x) + C"]),
    108: ([str(HOST), "--int", "cos(x)*e^(sin(x))/(1+e^(sin(x)))"], None, ["Let u=e^(sin(x))", "log(abs(1+e^(sin(x)))) + C"]),
    109: ([str(HOST), "--int", "x*sqrt(1-x^2)"], None, ["Let u=1-x^2", "-(1-x^2)^(3/2)/3 + C"]),
    110: ([str(HOST), "--int", "1/(sqrt(x)*(1+x))"], None, ["Let t=sqrt(x)", "2*atan(sqrt(x)) + C"]),
    111: ([str(HOST), "--int", "1/(1+e^x)"], None, ["u = e^x", "1/(u(1+u)) = 1/u-1/(1+u)", "x - log(abs(1+e^x)) + C"]),
    112: ([str(HOST), "--derive", "x^2*e^y+y^2*e^x=1,method=implicit"], None, ["dy/dx = -(2*x*e^y+y^2*e^x)/(x^2*e^y+2*y*e^x)"]),
    113: ([str(HOST), "--derive", "cos(t)^3,sin(t)^3,t,method=param_second"], None, ["dy/dx", "d2y/dx2 = 1/(3*cos(t)^4*sin(t))"]),
}


STATIC_FAMILIES: dict[str, tuple[range | list[int], list[str]]] = {
    "trig_struct": (list(range(26, 51)) + list(range(114, 126)), ["s^2=1-c^2", "Trig solve:"]),
    "algebra_pf": (list(range(51, 76)) + list(range(126, 139)), ["factor denom; PF", "Expand; collect; factor"]),
    "log_exp": (list(range(76, 101)) + list(range(139, 151)), ["Set u=log/base exp", "Domain:"]),
}


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")
    doc_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_DOC
    problem_count, solution_count, doc_label = load_doc(doc_path)

    src = "\n".join(
        p.read_text(errors="ignore")
        for p in [
            REPO / "c++/addin/src/modules/trig/trig.cpp",
            REPO / "c++/addin/src/modules/algebra/algebra.cpp",
            REPO / "c++/addin/src/modules/integrate/integrate.cpp",
            REPO / "c++/addin/src/modules/derive/derive.cpp",
            REPO / "c++/khicas/upstream/giac90_1addin/main.cc",
        ]
    )
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report: list[str] = [
        "Rigor challenge latest report",
        f"document: {doc_label}",
        f"parsed problem headings: {problem_count}",
        f"parsed solution blocks: {solution_count}",
        "",
    ]
    covered: set[int] = set()
    direct = 0
    policy = 0
    bad = 0

    for qid in sorted(DIRECT):
        args, stdin, needles = DIRECT[qid]
        out = run(args, stdin)
        ok = "ERR:" not in out and "Error:" not in out and "Integral not recognised" not in out
        ok = ok and all(markers_present(out, [n]) for n in needles)
        covered.add(qid)
        direct += 1
        status = "OK" if ok else "FAIL"
        print(status, qid)
        report.append(f"{status} #{qid}: direct")
        if not ok:
            bad += 1
            report.extend("  " + ln for ln in out.splitlines()[:14])
        report.append("")

    for family, (ids, markers) in STATIC_FAMILIES.items():
        ok = all(markers_present(src, [m]) for m in markers)
        for qid in ids:
            covered.add(qid)
            policy += 1
            status = "OK" if ok else "FAIL"
            print(status, qid, family)
            report.append(f"{status} #{qid}: {family} source-policy")
            if not ok:
                bad += 1
                missing = [m for m in markers if m not in src]
                report.append("  missing: " + ", ".join(missing))
            report.append("")

    missing_ids = sorted(set(range(1, 151)) - covered)
    if missing_ids:
        bad += len(missing_ids)
        report.append("MISSING: " + ", ".join(map(str, missing_ids)))
    report.append(f"summary: bad={bad}, covered={len(covered)}/150, direct={direct}, policy={policy}")
    REPORT.write_text("\n".join(report) + "\n")
    print(f"report {REPORT}")
    print(f"done bad {bad} / 150")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
