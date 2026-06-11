#!/usr/bin/env python3
from __future__ import annotations

import os
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


def build_xform_probe(tmp: Path) -> Path:
    src = tmp / "xform_probe.cpp"
    src.write_text(
        '#include <iostream>\n'
        '#include <string>\n'
        '#include "khicas/upstream/giac90_1addin/cascas_working.h"\n'
        'static bool diag_eval(const char*,cascas::working_string &out){\n'
        '  out="Warning adding 2 ) at end of input\\n// :1: syntax error at Error: Bad Argument Value";\n'
        '  return true;\n'
        '}\n'
        'int main(){\n'
        '  cascas::set_khicas_eval_callback(diag_eval);\n'
        '  cascas::working_string out;\n'
        '  if(!cascas::eval_with_working("xform((sin(x)-1)^2=2,cos(x)^2=(8*sin(x)^2-6*sin(x)))",out)) return 2;\n'
        '  std::string s=out.c_str();\n'
        '  std::cout << s << "\\n";\n'
        '  if(s.find("Warning")!=std::string::npos || s.find("syntax error")!=std::string::npos ||\n'
        '     s.find("Bad Argument")!=std::string::npos || s.find("//")!=std::string::npos ||\n'
        '     s.find("Error:")!=std::string::npos || s.find("Ans:\\\\nxform")!=std::string::npos ||\n'
        '     s.find("Search:")==std::string::npos || s.find("Target")==std::string::npos) return 1;\n'
        '  return 0;\n'
        '}\n'
    )
    exe = tmp / "xform_probe"
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
        tmp = Path(d)
        exe = build_probe(tmp)
        for expr in plain:
            proc = subprocess.run([str(exe), expr], cwd=ROOT, text=True, capture_output=True)
            text = (proc.stdout + proc.stderr).strip()
            if "Bad Argument Type" in text:
                bad.append(f"{expr}: leaked callback error: {text}")
            elif proc.returncode not in (0, 1):
                bad.append(f"{expr}: unexpected return {proc.returncode}: {text}")
            elif proc.returncode == 1 and not text.startswith("="):
                bad.append(f"{expr}: unexpected local output: {text}")
        xform_exe = build_xform_probe(tmp)
        proc = subprocess.run([str(xform_exe)], cwd=ROOT, text=True, capture_output=True)
        if proc.returncode:
            bad.append(f"xform diagnostic leak: {proc.stdout}{proc.stderr}")
    env = dict(os.environ, CASCAS_HOST_PRODUCTION="1")
    for expr, want in [("2*x+1", "2*x + 1"), ("coeff((1+x+x^3)^7,x,9)", "252")]:
        proc = subprocess.run(
            [str(ROOT / "tools/khicas_host_runner"), expr],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=30,
            env=env,
        )
        text = (proc.stdout + proc.stderr).strip()
        if proc.returncode or want not in text:
            bad.append(f"{expr}: missing exact host fallback: {text}")
        if "Verified" in text or "KhiCAS exact" in text:
            bad.append(f"{expr}: stale exact fallback wording: {text}")
    if bad:
        print("FAIL plain eval callback guard")
        print("\n".join(bad))
        return 1
    print(f"OK plain eval callback guard cases={len(plain)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
