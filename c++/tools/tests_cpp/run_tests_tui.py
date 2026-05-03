#!/usr/bin/env python3
from __future__ import annotations

import json
import random
import shlex
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
FAIL_LOG = REPO / "c++" / "tools" / "fuzz" / "tui_failures.jsonl"


@dataclass(frozen=True)
class Case:
    name: str
    argv: list[str]
    stdin: str = ""
    must: tuple[str, ...] = ("Answer:",)
    reject: tuple[str, ...] = ("err:", "not recognised", "unsupported")


PROGRAMS = {
    "derive": "deriveProgram.py",
    "int": "intProgram.py",
    "alg": "algebraProgram.py",
    "trig": "trigProgram.py",
    "suvat": "SUVATprogram.py",
    "bool": "booleanProgram.py",
    "stats": "statsProgram.py",
}


SMOKE_CASES = [
    Case("derive chain/product", ["--derive", "(2*x+ln(x))^3"]),
    Case("derive implicit", ["--stdin-program", PROGRAMS["derive"]], "2\n1/(2x+1)+1/(y+1)=x^2\n1,1\n"),
    Case("derive parametric", ["--stdin-program", PROGRAMS["derive"]], "3\nt^2+1\nsin(t)\n"),
    Case("integrate reverse chain", ["--int", "5*x*(x^2+4)^6"]),
    Case("integrate quotient log", ["--int", "2*x/(x^2+4)"]),
    Case("integrate parts", ["--int", "x*ln(x)"]),
    Case("integrate trig", ["--int", "tan(x)"]),
    Case("solve rational", ["--alg", "25-105/(2*x-5)^2=0"]),
    Case("trig solve", ["--stdin-program", PROGRAMS["trig"]], "3\n2+sec(x-pi/3)=0, x, 0, 2*pi\n"),
    Case("suvat", ["--suvat", "u=4,a=3,t=5,target=v"], must=("v = 19",)),
    Case("boolean", ["--bool", "A.(B+C)"], must=("STEP:", "WHY:")),
    Case("stats mean", ["--stats", "mean([1,2,3,4])"]),
]


def run(cmd: list[str], *, stdin: str = "", timeout: float = 30.0) -> tuple[int, str]:
    try:
        p = subprocess.run(
            cmd,
            input=stdin,
            text=True,
            capture_output=True,
            cwd=str(REPO),
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as err:
        out = ""
        if isinstance(err.stdout, str):
            out += err.stdout
        if isinstance(err.stderr, str):
            out += "\n" + err.stderr
        return 124, out.strip() + "\nTIMEOUT"
    out = (p.stdout or "") + (("\n" + p.stderr) if p.stderr else "")
    return p.returncode, out.strip()


def build_host() -> bool:
    rc, out = run(["./c++/tools/build_host.sh"], timeout=180)
    print(out or "host build finished")
    return rc == 0 and HOST.exists()


def ensure_host() -> bool:
    if HOST.exists():
        return True
    print("host missing; building...")
    return build_host()


def host_case(case: Case) -> tuple[bool, str]:
    if not ensure_host():
        return False, f"missing host: {HOST}"
    rc, out = run([str(HOST), *case.argv], stdin=case.stdin)
    low = out.lower()
    ok = rc == 0 and all(m.lower() in low for m in case.must) and not any(r in low for r in case.reject)
    return ok, out


def log_failure(kind: str, payload: dict[str, object]) -> None:
    FAIL_LOG.parent.mkdir(parents=True, exist_ok=True)
    row = {"ts": time.strftime("%Y-%m-%dT%H:%M:%S"), "kind": kind, **payload}
    with FAIL_LOG.open("a", encoding="utf-8") as f:
        f.write(json.dumps(row, ensure_ascii=False) + "\n")


def run_suite() -> bool:
    bad = 0
    for i, case in enumerate(SMOKE_CASES, 1):
        print(f"\n[{i}/{len(SMOKE_CASES)}] {case.name}")
        ok, out = host_case(case)
        print(out)
        if not ok:
            bad += 1
            log_failure("smoke", {"case": case.name, "argv": case.argv, "stdin": case.stdin, "out": out})
            print("FAIL")
        else:
            print("OK")
    print(f"\nsummary: {len(SMOKE_CASES)-bad}/{len(SMOKE_CASES)} ok")
    return bad == 0


def random_poly(var: str = "x", degree: int = 4) -> str:
    terms: list[str] = []
    for p in range(degree, -1, -1):
        c = random.randint(-9, 9)
        if c == 0:
            continue
        if p == 0:
            terms.append(str(c))
        elif p == 1:
            terms.append(f"{c}*{var}")
        else:
            terms.append(f"{c}*{var}^{p}")
    return "+".join(terms).replace("+-", "-") or "0"


def random_case() -> Case:
    kind = random.choice(["derive", "int", "alg", "trig"])
    a = random.randint(2, 9)
    b = random.randint(-7, 7)
    n = random.randint(2, 8)
    if kind == "derive":
        expr = random.choice([
            f"({a}*x+ln(x))^{n}",
            f"sin({a}*x+{b})*exp(x)",
            f"({random_poly()})/({a}*x+{b})",
            f"tan({a}*x)^2",
        ])
        return Case(f"stress derive {expr}", ["--derive", expr])
    if kind == "int":
        expr = random.choice([
            f"{a}*x*(x^2+{abs(b)+1})^{n}",
            f"{a}*x/(x^2+{abs(b)+1})",
            f"x*ln({a}*x)",
            f"sin({a}*x+{b})",
            f"tan({a}*x)",
        ])
        return Case(f"stress int {expr}", ["--int", expr])
    if kind == "trig":
        expr = random.choice([
            f"sin(x)^2+cos(x)^2",
            f"tan(x)-sin(x)/cos(x)",
            f"{a}+sec(x-pi/{random.choice([3,4,6])})=0, x, 0, 2*pi",
        ])
        if "=" in expr:
            return Case(f"stress trig solve {expr}", ["--stdin-program", PROGRAMS["trig"]], f"3\n{expr}\n")
        return Case(f"stress trig {expr}", ["--trig", expr])
    poly = random_poly()
    expr = random.choice([
        f"{poly}=0",
        f"({a}*x+{b})^2-{random.randint(1, 40)}=0",
        f"{a}/{a}*(x-{b})^2-{n}=0",
    ])
    return Case(f"stress alg {expr}", ["--alg", expr])


def run_stress(count: int) -> bool:
    bad = 0
    for i in range(1, count + 1):
        case = random_case()
        ok, out = host_case(case)
        print(f"[{i}/{count}] {'OK' if ok else 'FAIL'} {case.name}")
        if not ok:
            bad += 1
            log_failure("stress", {"case": case.name, "argv": case.argv, "stdin": case.stdin, "out": out})
            print(out)
    print(f"stress summary: {count-bad}/{count} ok; failures: {FAIL_LOG}")
    return bad == 0


def compile_g3a() -> bool:
    rc, out = run(["./compile"], timeout=600)
    print(out)
    return rc == 0


def full_tests() -> bool:
    rc, out = run([sys.executable, "run_tests.py"], timeout=300)
    print(out)
    return rc == 0


def print_help() -> None:
    print(
        """
commands:
  /help                 show commands
  /build                build C++ host
  /tests                run full repo C++ checks
  /suite                run focused working-line smoke suite
  /stress [n]           generate n hostile C++ host cases
  /fuzz                 replay regression fuzz cases
  /compile              build CasioCAS.g3a at repo root
  /infinite [stress n]  loop until Ctrl-C or first failure
  /derive <expr>        run host --derive
  /int <expr>           run host --int
  /alg <expr>           run host --alg
  /trig <expr>          run host --trig
  /stats <expr>         run host --stats
  /bool <expr>          run host --bool
  /suvat <kv>           e.g. u=4,a=3,t=5,target=v
  /raw <args...>        direct casio_host args
  /quit                 exit
""".strip()
    )


def command(line: str) -> bool:
    if not line.strip():
        return True
    if not line.startswith("/"):
        ok, out = host_case(Case("raw algebra", ["--alg", line]))
        print(out)
        return ok
    parts = shlex.split(line)
    cmd = parts[0].lower()
    rest = line[len(parts[0]):].strip()
    if cmd in ("/q", "/quit", "/exit"):
        raise SystemExit(0)
    if cmd == "/help":
        print_help()
        return True
    if cmd == "/build":
        return build_host()
    if cmd == "/tests":
        return full_tests()
    if cmd == "/suite":
        return run_suite()
    if cmd == "/stress":
        n = int(parts[1]) if len(parts) > 1 else 100
        return run_stress(n)
    if cmd == "/fuzz":
        rc, out = run([sys.executable, "c++/tools/run_tests_cpp.py", "--fuzz"], timeout=300)
        print(out)
        return rc == 0
    if cmd == "/compile":
        return compile_g3a()
    if cmd == "/infinite":
        n = int(parts[2]) if len(parts) > 2 and parts[1] == "stress" else 250
        i = 1
        while True:
            print(f"\nloop {i}")
            ok = run_stress(n) if "stress" in parts else run_suite()
            if not ok:
                return False
            i += 1
    if cmd in ("/derive", "/int", "/alg", "/trig", "/stats", "/bool", "/suvat"):
        flag = "--" + cmd[1:]
        ok, out = host_case(Case(cmd, [flag, rest]))
        print(out)
        return ok
    if cmd == "/raw":
        ok, out = host_case(Case("raw", parts[1:]))
        print(out)
        return ok
    print(f"unknown command: {cmd}")
    return False


def main() -> int:
    print("CasioCAS C++ TUI")
    print(f"repo: {REPO}")
    print(f"fail log: {FAIL_LOG}")
    print_help()
    while True:
        try:
            line = input("\ncasio-cpp> ")
            command(line)
        except KeyboardInterrupt:
            print("\ninterrupted")
        except EOFError:
            print()
            return 0


if __name__ == "__main__":
    raise SystemExit(main())
