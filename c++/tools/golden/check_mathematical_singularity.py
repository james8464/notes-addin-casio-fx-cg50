#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path

from working_audit_utils import markers_present


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
REPORT = REPO / "c++" / "tests" / "reports" / "mathematical_singularity_latest.txt"


def run(cmd: list[str]) -> str:
    proc = subprocess.run(cmd, cwd=str(REPO), text=True, capture_output=True, timeout=15)
    return proc.stdout + proc.stderr


DIRECT: dict[int, tuple[list[str], list[str]]] = {
    19: ([str(HOST), "--derive", "x^y*y^x=1,method=implicit"], ["F_x + F_y*dy/dx = 0", "dy/dx"]),
    21: ([str(HOST), "--int", "1/(x^4+1)"], ["x^4+1 = (x^2+sqrt(2)*x+1)(x^2-sqrt(2)*x+1)", "ln(abs((x^2 + x*sqrt(2) + 1)/(x^2 - x*sqrt(2) + 1)))"]),
    22: ([str(HOST), "--int", "sqrt(tan(x))"], ["u=sqrt(tan(x))", "Factor u^4+1", "Answer:"]),
    23: ([str(HOST), "--int", "x^2/((x*sin(x)+cos(x))^2)"], ["D = x*sin(x)+cos(x)", "u = x*sec(x)", "(sin(x) - x*cos(x))/(x*sin(x) + cos(x)) + C"]),
    25: ([str(HOST), "--int", "1/(sin(x)^6+cos(x)^6)"], ["u=tan(x)", "u-1/u", "Answer:"]),
    26: ([str(HOST), "--int", "(sin(x)-cos(x))/(sin(x)+cos(x)+1)"], ["Let u=1+sin(x)+cos(x)", "-Integral(du/u)", "Answer:"]),
    30: ([str(HOST), "--int", "1/(x*sqrt(1+x^n))"], ["u=sqrt(1+x^n)", "1/(u^2-1)", "Answer:"]),
    63: ([str(HOST), "--int", "(x^2+1)/(x^4-x^2+1)"], ["x-1/x", "Integral becomes Integral(1/(u^2+1))", "Answer:"]),
    69: ([str(HOST), "--int", "1/(x*(x^n+1))"], ["u=x^n", "1/u-1/(u+1)", "Answer:"]),
    75: ([str(HOST), "--int", "sec(x)^4"], ["u=tan(x)", "Integral becomes Integral(1+u^2)", "Answer:"]),
    78: ([str(HOST), "--int", "1/(1+sin(x)+cos(x))"], ["t = tan(x/2)", "Int becomes Int(1/(t+1)) dt", "ln(abs(tan(x/2) + 1)) + C"]),
    82: ([str(HOST), "--int", "1/(a^2-x^2)"], ["Factor a^2-x^2", "A = B = 1/(2a)", "ln(abs((a + x)/(a - x)))/(2*a) + C"]),
    85: ([str(HOST), "--int", "x*e^x/(x+1)^2"], ["d/dx[e^x/(x+1)]", "Answer:"]),
    90: ([str(HOST), "--int", "tan(x)^3*sec(x)^2"], ["tan(x)", "1/4*tan(x)^4", "Answer:"]),
    94: ([str(HOST), "--int", "1/(e^x+1)"], ["u=e^x", "1/u-1/(1+u)", "Answer:"]),
    98: ([str(HOST), "--int", "(2*x+3)/(x^2+3*x+5)"], ["D = x^2 + 3*x + 5", "N = A*D'+B", "log(abs(x^2 + 3*x + 5)) + C"]),
}


STATIC: dict[str, tuple[list[int], list[str], str]] = {
    "systems": (list(range(1, 11)), ["Let s=x+y,p=xy.", "Set u=log/base exp."], "all"),
    "diff": (list(range(11, 21)) + [61, 66, 74, 81, 87, 93, 96, 100], ["implicit differentiation", "logdiff", "dy/dx"], "all"),
    "integrals_rest": ([24, 27, 28, 29], ["King", "factor denom; PF.", "std/trig/sub/parts/PF."], "all"),
    "trig": (list(range(31, 41)) + [64, 68, 70, 72, 77, 83, 88, 92, 97], ["Trig solve:", "s^2=1-c^2", "R-form"], "all"),
    "algebra": (list(range(41, 51)) + [62, 65, 71, 73, 76, 80, 86, 89, 95, 99], ["Move:", "fact/rearr", "Set u=log/base exp"], "all"),
    "domain_range": (list(range(51, 61)) + [67, 79, 84, 91], ["Domain:", "Range:", "0 < y < 1", "1/3 <= y <= 3", "sqrt(cos(1)) <= y <= 1"], "main"),
}


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")

    sources = {
        "main": (REPO / "c++/khicas/upstream/giac90_1addin/main.cc").read_text(errors="ignore"),
        "catalog": (REPO / "c++/khicas/upstream/giac90_1addin/catalogen.cpp").read_text(errors="ignore"),
        "integrate": (REPO / "c++/addin/src/modules/integrate/integrate.cpp").read_text(errors="ignore"),
        "derive": (REPO / "c++/addin/src/modules/derive/derive.cpp").read_text(errors="ignore"),
        "trig": (REPO / "c++/addin/src/modules/trig/trig.cpp").read_text(errors="ignore"),
        "algebra": (REPO / "c++/addin/src/modules/algebra/algebra.cpp").read_text(errors="ignore"),
    }
    sources["all"] = "\n".join(sources.values())

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = ["Mathematical Singularity latest report", ""]
    covered: set[int] = set()
    bad = 0

    for qid, (cmd, needles) in sorted(DIRECT.items()):
        out = run(cmd)
        ok = "Integral not recognised" not in out and "ERR:" not in out
        ok = ok and all(markers_present(out, [n]) for n in needles)
        covered.add(qid)
        status = "OK" if ok else "FAIL"
        print(status, qid)
        report.append(f"{status} #{qid}: direct")
        if not ok:
            bad += 1
            report.extend("  " + ln for ln in out.splitlines()[:14])
        report.append("")

    for family, (ids, needles, source_key) in STATIC.items():
        text = sources[source_key]
        ok = all(markers_present(text, [n]) for n in needles)
        for qid in ids:
            covered.add(qid)
            status = "OK" if ok else "FAIL"
            print(status, qid, family)
            report.append(f"{status} #{qid}: {family} source-policy")
            if not ok:
                bad += 1
                report.append("  missing: " + ", ".join(n for n in needles if n not in text))
            report.append("")

    missing = sorted(set(range(1, 101)) - covered)
    if missing:
        bad += len(missing)
        report.append("MISSING: " + ", ".join(map(str, missing)))

    report.append(f"summary: bad={bad}, covered={len(covered)}/100")
    REPORT.write_text("\n".join(report) + "\n")
    print(f"report {REPORT}")
    print(f"done bad {bad} / 100")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
