#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def build_probe(tmp: Path) -> Path:
    src = tmp / "probe.cpp"
    src.write_text(
        '#include <iostream>\n'
        '#include "khicas/upstream/giac90_1addin/cascas_working.h"\n'
        'static bool bad_eval(const char*,cascas::working_string &out){\n'
        '  out="// Error: Bad Argument Type";\n'
        '  return true;\n'
        '}\n'
        'int main(int argc,char **argv){\n'
        '  if(argc<2) return 2;\n'
        '  cascas::set_khicas_eval_callback(bad_eval);\n'
        '  cascas::working_string out;\n'
        '  if(cascas::eval_with_working(argv[1],out)){\n'
        '    std::cerr << out << "\\n";\n'
        '    return 1;\n'
        '  }\n'
        '  return 0;\n'
        '}\n'
    )
    exe = tmp / "probe"
    subprocess.check_call(
        [
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
        ],
        cwd=ROOT,
    )
    return exe


def main() -> int:
    plain = ["2+2", "2*2", "4-2", "4/2", "(3+5)/2", "2*x+1"]
    bad: list[str] = []
    with tempfile.TemporaryDirectory() as d:
        exe = build_probe(Path(d))
        for expr in plain:
            proc = subprocess.run([str(exe), expr], cwd=ROOT, text=True, capture_output=True)
            text = (proc.stdout + proc.stderr).strip()
            if "Bad Argument Type" in text:
                bad.append(f"{expr}: leaked callback error: {text}")
            elif proc.returncode not in (0, 1):
                bad.append(f"{expr}: unexpected return {proc.returncode}: {text}")
            elif proc.returncode == 1 and not text.startswith("="):
                bad.append(f"{expr}: unexpected local output: {text}")
    good_src = (
        '#include <iostream>\n'
        '#include <string>\n'
        '#include "khicas/upstream/giac90_1addin/cascas_working.h"\n'
        'static bool exact_eval(const char *expr,cascas::working_string &out){\n'
        '  std::string s=expr?expr:"";\n'
        '  if(s=="2*x+1"){ out="2*x + 1"; return true; }\n'
        '  return false;\n'
        '}\n'
        'int main(){\n'
        '  cascas::set_khicas_eval_callback(exact_eval);\n'
        '  cascas::working_string out;\n'
        '  if(!cascas::eval_with_working("2*x+1",out)) return 2;\n'
        '  std::string s=out.c_str();\n'
        '  std::cout << s << "\\n";\n'
        '  return s.find("KhiCAS exact evaluation:")==std::string::npos || s.find("Verified")==std::string::npos;\n'
        '}\n'
    )
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        src = tmp / "good.cpp"
        exe = tmp / "good"
        src.write_text(good_src)
        subprocess.check_call(
            [
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
            ],
            cwd=ROOT,
        )
        proc = subprocess.run([str(exe)], cwd=ROOT, text=True, capture_output=True)
        if proc.returncode:
            bad.append("2*x+1: missing KhiCAS exact fallback route: " + (proc.stdout + proc.stderr).strip())
    if bad:
        print("FAIL plain eval callback guard")
        print("\n".join(bad))
        return 1
    print(f"OK plain eval callback guard cases={len(plain)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
