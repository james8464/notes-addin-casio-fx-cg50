#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    src = r'''
#include <iostream>
#include <string>
#include "khicas/upstream/giac90_1addin/cascas_working.h"

static bool calc_like_eval(const char *expr,cascas::working_string &out){
  std::string s=expr?expr:"";
  if (s.rfind("xform(",0)==0){
    out="1";
    return true;
  }
  out="// Error: Bad Argument Type";
  return true;
}

int main(){
  cascas::set_khicas_eval_callback(calc_like_eval);
  cascas::working_string out;
  const char *inputs[]={
    "xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2*x),k)",
    "xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2x),k)",
  };
  for (int i=0;i<2;++i){
    out.clear();
    if (!cascas::eval_with_working(inputs[i],out))
      return 2;
    std::cout << out << "\n";
    std::string s=out.c_str();
    if (s.find("Parameter isolation:")==std::string::npos ||
        s.find("k = 2")==std::string::npos ||
        s.find("Verified by substitution")==std::string::npos ||
        s.find("not equivalent")!=std::string::npos ||
        s.find("k = 1")!=std::string::npos ||
        s.find("not verified")!=std::string::npos)
      return 1;
  }
  cascas::set_khicas_eval_callback(0);
  out.clear();
  if (!cascas::eval_with_working("xform((cos(x)+sin(x))*(cosec(x)-sec(x))=k*cot(2x),k)",out))
    return 3;
  std::string s=out.c_str();
  if (s.find("Parameter isolation:")==std::string::npos ||
      s.find("k = 2")==std::string::npos ||
      s.find("not equivalent")!=std::string::npos ||
      s.find("not verified")!=std::string::npos)
    return 4;
  return 0;
}
'''
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        cpp = tmp / "probe.cpp"
        exe = tmp / "probe"
        cpp.write_text(src)
        subprocess.check_call([
            "c++",
            "-std=c++11",
            "-DCASCAS_HOST_STD_STRING=1",
            "-DCASCAS_DISABLE_GOLDEN_QUEUE=1",
            "-I",
            str(ROOT),
            str(cpp),
            str(ROOT / "khicas/upstream/giac90_1addin/cascas_working.cc"),
            "-o",
            str(exe),
        ], cwd=ROOT)
        proc = subprocess.run([str(exe)], cwd=ROOT, text=True, capture_output=True)
    if proc.returncode:
      print("FAIL calculator callback xform route")
      print(proc.stdout)
      print(proc.stderr)
      return proc.returncode
    print("OK calculator callback xform route")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
