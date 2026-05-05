#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
SOURCE = Path("/Users/james/Desktop/casio_fx_cg50_cas_stress_test_100_questions.txt")
REPORT = REPO / "c++" / "tests" / "reports" / "casio_fx_cg50_stress_latest.txt"


def compact(text: str) -> str:
    return "".join(ch for ch in text if ch not in " \t\r\n*")


def run(cmd: list[str]) -> str:
    proc = subprocess.run(cmd, cwd=str(REPO), text=True, capture_output=True, timeout=20)
    return proc.stdout + proc.stderr


DIRECT: dict[int, tuple[list[str], list[str]]] = {
    1: (
        [str(HOST), "--int", "integrate((sqrt(x)*(2*x-5))/3,x)"],
        ["Rewrite sqrt(x) as x^(1/2)", "Integrate each power", "4/15*x^(5/2)", "-10/9*x^(3/2)", "+ C"],
    ),
    2: (
        [str(HOST), "--int", "integrate(1/sqrt(x^2+4*x-5),x)"],
        ["Complete square", "(x + 2)^2 - 9", "log(abs(", "+ C"],
    ),
    3: (
        [str(HOST), "--int", "mean_value(1/sqrt(x^2+4*x-5),x,3,13)"],
        ["Mean value", "Integral_3^13", "F(13) - F(3)", "/10"],
    ),
    6: (
        [str(HOST), "--int", "integrate(diff(f(x),x)/f(x),x)"],
        ["Let u=f(x)", "Integral becomes Integral(1/u) du", "log(abs(f(x))) + C", "f(x) != 0"],
    ),
    18: (
        [str(HOST), "--alg", "complete_square(x^2+4*x-5)"],
        ["Complete square", "h = b/(2a) = 2", "k = c - b^2/(4a) = -9", "(x + 2)^2 - 9"],
    ),
    96: (
        [str(HOST), "--suvat", "suvat(u=0,a=3.2,t=5,find=[v,s])"],
        ["v = u + at", "v = 16", "s = ut + 1/2at^2", "s = 40"],
    ),
}


def question_count(text: str) -> int:
    return len(re.findall(r"^## Q\d+\.", text, flags=re.MULTILINE))


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")
    if not SOURCE.exists():
        raise SystemExit(f"Missing stress file: {SOURCE}")

    text = SOURCE.read_text(errors="replace")
    total = question_count(text)
    source_required = max(0, total - len(DIRECT))

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    report = [
        "Casio fx-CG50 stress latest report",
        f"source: {SOURCE}",
        f"questions_in_file: {total}",
        f"concrete_runnable_cases: {len(DIRECT)}",
        f"source_locator_cases_needing_official_pdf/data: {source_required}",
        "",
    ]

    bad = 0
    for qid, (cmd, needles) in sorted(DIRECT.items()):
        out = run(cmd)
        ok = "ERR:" not in out and "not recognised" not in out
        ok = ok and all(compact(n) in compact(out) for n in needles)
        status = "OK" if ok else "FAIL"
        print(status, qid)
        report.append(f"{status} Q{qid}: {' '.join(cmd[2:])}")
        if not ok:
            bad += 1
            report.append("  missing: " + ", ".join(n for n in needles if compact(n) not in compact(out)))
            report.extend("  " + line for line in out.splitlines()[:16])
        report.append("")

    if source_required:
        report.append(
            "NOTE: remaining cases are copyright-safe locators/placeholders; exact answer/working checks require opening the linked official papers."
        )
    report.append(f"summary: concrete_bad={bad}, concrete={len(DIRECT)}, source_required={source_required}, total={total}")
    REPORT.write_text("\n".join(report) + "\n")
    print(f"report {REPORT}")
    print(f"done concrete_bad {bad} concrete {len(DIRECT)} source_required {source_required} / {total}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
