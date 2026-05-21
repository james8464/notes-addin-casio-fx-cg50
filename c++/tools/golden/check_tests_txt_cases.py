#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = REPO / "c++" / "tools" / "golden" / "rtf_cases.jsonl"


NEEDLES: dict[str, list[str]] = {
    "8c": ["x = f(y)", "f^-1(x) = (- e^(x) + 4)/2", "Given domain: x<2"],
    "9": ["1/y dy = (- x + 1)/(x + 1) dx", "y = (x + 1)^2*e^(-x)"],
    "H7a": ["F_x", "F_y", "dy/dx = (2 - 2*x*y)/(x^2 - 2*y)"],
    "M13": ["Multiply by", "1.475", "3.525"],
    "U3": ["u = cos(x - pi/3)", "u = -1/2", "x = [pi, 5*pi/3]"],
}


def compact(text: str) -> str:
    return (text or "").replace(" ", "").replace("\r", "")


def script_name(script_relpath: str) -> str:
    path = Path(script_relpath)
    if "ComputerScience" in path.parts:
        return "ComputerScience/" + path.name
    return path.name


def main() -> int:
    if not HOST.exists():
        raise SystemExit(f"Missing host binary: {HOST}")

    rows = [json.loads(line) for line in CASES.read_text().splitlines() if line.strip()]
    bad = 0
    for row in rows:
        qid = row["qid"]
        if qid not in NEEDLES:
            continue
        proc = subprocess.run(
            [str(HOST), "--stdin-program", script_name(row["script_relpath"])],
            input=row["stdin"],
            cwd=str(REPO),
            text=True,
            capture_output=True,
            timeout=12,
        )
        out = proc.stdout + proc.stderr
        flat = compact(out)
        ok = proc.returncode == 0 and "ERR:" not in out and "Error:" not in out
        ok = ok and all(compact(n) in flat for n in NEEDLES[qid])
        if ok:
            print("OK", qid, row["header"])
            continue
        bad += 1
        print("FAIL", qid, row["header"])
        print(out)

    print("done mismatches", bad, "/", len(NEEDLES))
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
