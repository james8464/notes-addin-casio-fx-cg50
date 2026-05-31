#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path

from check_golden_shared_coverage import ROOT, specs


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
    ap = argparse.ArgumentParser()
    ap.add_argument("--min-ok", type=int, default=133)
    ap.add_argument("--show-failures", type=int, default=0)
    args = ap.parse_args()
    bad: list[str] = []
    ok = 0
    work = specs()
    with tempfile.TemporaryDirectory() as d:
        exe = build_probe(Path(d))
        for spec in work:
            proc = subprocess.run([str(exe), spec["input"]], cwd=ROOT, text=True, capture_output=True)
            out = proc.stdout + proc.stderr
            missing = [m for m in spec["markers"] if m and m not in out]
            if proc.returncode == 0 and not missing:
                ok += 1
            else:
                bad.append(f"{spec['id']}#{spec['index']} rc={proc.returncode} missing={missing} input={spec['input']}")
    total = len(work)
    if ok < args.min_ok:
        print(f"FAIL no-golden queue coverage ok={ok} total={total} min={args.min_ok} bad={len(bad)}")
        print("\n".join(bad[:max(1, args.show_failures)]))
        return 1
    print(f"OK no-golden queue coverage ok={ok} total={total} min={args.min_ok} bad={len(bad)}")
    if args.show_failures:
        print("\n".join(bad[:args.show_failures]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
