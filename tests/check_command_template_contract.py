#!/usr/bin/env python3
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
P3_HOST = Path("/tmp/p3_engine_host")
CS_HOST = Path("/tmp/cscalc_engine_host")

P3_TEMPLATES = [
    "suvat(u=2,a=3,t=4)",
    "projectile(20,30)",
    "force(12,3)",
    "incline(5,30,0.2)",
    "pulley(3,5)",
    "beam(10,30,4,20)",
    "varacct(6,-4,0,3,5)",
    "hypbinom(20,0.4,4,0.05,-1)",
    "normalprob(40,60,50,10)",
    "binom(10,0.4,3)",
    "cond(0.2,0.5)",
    "regresscalc(5,20,30,10,18,8)",
    "groupmean(5,12,15,30,25,18)",
]

CS_TEMPLATES = [
    "floatdec(0101100,11101)",
    "convert(45,10,16)",
    "twos(-5,8)",
    "binadd(1011,0110,4)",
    "fixedenc(5.625,3,3)",
    "floatenc(12.75,8,4)",
    "floatnorm(00011010,0110)",
    "floatrange(8,4)",
    "image(800,600,24)",
    "sound(44100,60,16,2)",
    "bitrate(48000000,12)",
    "compress(1000,250)",
    "chars(120,8)",
    "records(1200,32)",
    "binarysearch(7,1,3,5,7,9)",
]

BANNED = ["Verified", "not checked", "syntax error", "Bad Argument", "Unsupported"]


def build() -> None:
    subprocess.check_call([
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Wno-unused-function", "-Itools",
        "tools/p3_engine.cpp", "tools/p3_engine_host.cpp", "-o", str(P3_HOST),
    ], cwd=ROOT)
    subprocess.check_call([
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Wno-unused-function", "-Itools",
        "tools/cscalc_engine.cpp", "tools/cscalc_engine_host.cpp", "-o", str(CS_HOST),
    ], cwd=ROOT)


def require_source(path: str, templates: list[str]) -> None:
    text = (ROOT / path).read_text(errors="ignore")
    for needle in ["Command input", "command_templates", "KEY_CTRL_EXE", "KEY_CTRL_F1"]:
        if needle not in text:
            raise AssertionError(f"{path}: missing {needle}")
    for template in templates:
        if template not in text:
            raise AssertionError(f"{path}: missing template {template}")


def require_eval(host: Path, expr: str) -> None:
    out = subprocess.check_output([str(host), expr], cwd=ROOT, text=True)
    if len([line for line in out.splitlines() if line.strip()]) < 2:
        raise AssertionError(f"{expr}: too few working lines\n{out}")
    for bad in BANNED:
        if bad in out:
            raise AssertionError(f"{expr}: banned text {bad}\n{out}")


def main() -> int:
    require_source("tools/p3_app.cc", P3_TEMPLATES)
    require_source("tools/cscalc_app.cc", CS_TEMPLATES)
    build()
    for expr in P3_TEMPLATES:
        require_eval(P3_HOST, expr)
    for expr in CS_TEMPLATES:
        require_eval(CS_HOST, expr)
    print("OK command template contract")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
