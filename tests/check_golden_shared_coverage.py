#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QUEUE = ROOT / "tests/golden/exact_calculator_input_queue.jsonl"


def specs() -> list[dict]:
    out: list[dict] = []
    for line in QUEUE.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            out.append({
                "id": row["id"],
                "index": i,
                "input": item["input"],
                "markers": item.get("expected_output_markers", []),
            })
    return out


def build_probe(tmp: Path) -> Path:
    src = tmp / "probe.cpp"
    src.write_text(
        '#include <iostream>\n'
        '#include "khicas/upstream/giac90_1addin/cascas_working.h"\n'
        'int main(int argc,char**argv){\n'
        '  if(argc<2) return 2;\n'
        '  cascas::working_string out;\n'
        '  if(!cascas::eval_with_working(argv[1],out)) return 1;\n'
        '  std::cout << out;\n'
        '  return 0;\n'
        '}\n'
    )
    exe = tmp / "probe"
    subprocess.check_call([
        "c++",
        "-std=c++11",
        "-DCASCAS_HOST_STD_STRING=1",
        "-I",
        str(ROOT),
        str(src),
        str(ROOT / "khicas/upstream/giac90_1addin/cascas_working.cc"),
        "-o",
        str(exe),
    ], cwd=ROOT)
    return exe


def main() -> int:
    bad: list[str] = []
    with tempfile.TemporaryDirectory() as d:
        exe = build_probe(Path(d))
        for spec in specs():
            proc = subprocess.run([str(exe), spec["input"]], cwd=ROOT, text=True, capture_output=True)
            out = proc.stdout + proc.stderr
            missing = [m for m in spec["markers"] if m and m not in out]
            if proc.returncode != 0 or missing:
                bad.append(f"{spec['id']}#{spec['index']} rc={proc.returncode} missing={missing} input={spec['input']}")
    if bad:
        print("FAIL shared golden coverage")
        print("\n".join(bad[:50]))
        return 1
    print(f"OK shared golden coverage cases={len(specs())}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
