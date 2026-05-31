#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from check_shared_working import CASES

ROOT = Path(__file__).resolve().parents[1]


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
        "-DCASCAS_DISABLE_GOLDEN_QUEUE=1",
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
        for expr, marker in CASES:
            proc = subprocess.run([str(exe), expr], cwd=ROOT, text=True, capture_output=True)
            out = proc.stdout + proc.stderr
            if proc.returncode != 0 or marker not in out:
                bad.append(f"rc={proc.returncode} missing={marker!r} input={expr}")
    if bad:
        print("FAIL shared core without golden")
        print("\n".join(bad[:50]))
        return 1
    print(f"OK shared core without golden cases={len(CASES)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
