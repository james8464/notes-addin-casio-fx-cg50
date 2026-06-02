#!/usr/bin/env python3
import argparse
import json
import random
import signal
import string
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"
PROGRESS = ROOT / "progress" / "state.jsonl"

OPS = ["+", "-", "*", "/"]
FUNCS = ["sin", "cos", "tan", "ln", "sqrt", "exp"]
COMMANDS = [
    "diff", "integrate", "simplify", "solve", "range",
    "xform", "xform", "xform",
    "log", "expand", "binomial", "defint", "trig",
]
WEIRD_CHARS = string.ascii_letters + string.digits + "+-*/^=(),[]{} ._"

TEMPLATES = {
    "diff": [
        "diff((x+1)(x-2)ln(x),x)",
        "diff(ln(((x)-(6))*((x)^2)),x)",
        "diff((sin(x))^3,x)",
        "diff((sin(x))*(7-x)-x*x,x)",
        "diff((x^2+1)*sin(3*x-2),x)",
        "diff((x^2+1)/(x-1),x)",
    ],
    "integrate": [
        "integrate((ln(x))^2,x,2)",
        "integrate(x^2*ln(x),x)",
        "integrate(3*x^2*ln(x),x)",
        "integrate((x+1)/(x^2+2*x+3)^3,x)",
        "integrate(x^2+sin(2*x)+exp(3*x),x)",
        "integrate(exp(ln(x)),x,3)",
        "integrate(tan(x),x)",
        "integrate(((x)^2)((4)^2))",
    ],
    "simplify": [
        "simplify((x^2+3*x+2)/(x+1))",
        "simplify((x^2-1)/(1-x))",
        "simplify((x^2+2*x+1)/(x+1)^2)",
        "simplify(sin(x))",
    ],
    "solve": [
        "solve(0=(10-0.4x)*ln(x+1),x)",
        "solve(x=-2.7,x)",
        "solve(x=x+6,x)",
        "solve(tan(x)=1/2,x)",
        "solve(dy/dx=y,y)",
    ],
    "range": [
        "range(2*x+3)",
        "range(x^2+4*x+7)",
        "range(2*sin(3*x-1)-5)",
        "range(exp(x))",
        "range(ln(x))",
    ],
    "xform": [
        "xform(2*sin(x-60)=cos(x-30),tan(x)=3*sqrt(3))",
        "xform(2*sin(x-60)-cos(x-30)=0,tan(x)=3*sqrt(3))",
        "xform(1+tan(x)^2,sec(x)^2)",
        "xform(1+cot(x)^2,cosec(x)^2)",
        "xform(sin(x)^2+cos(x)^2,1)",
        "xform(log(a,x),ln(x)/ln(a))",
        "xform(log(2,x^3),3log(2,x))",
        "xform((x+2)^2,x^2+4*x+4)",
        "xform(sin(x)+2cos(x),sqrt(5)*sin(x+atan(2)))",
    ],
    "log": ["log(2,x)", "log(5,x^2)"],
    "expand": ["expand((3*x-2)*(x+5))", "expand((2*x+3)^2)"],
    "binomial": ["binomial((1+8x)^(1/2))", "binomial((1-2x)^(-1))"],
    "apart": ["apart(6/(u(3+2u)))"],
    "defint": ["defint(sin(2x),x,0,pi/2)", "defint(9/(2x+1)^2,x,0,1)"],
    "trig": ["xform(1+tan(x)^2,sec(x)^2)"],
}


def maybe_space(rng: random.Random) -> str:
    return " " * rng.randrange(0, 3)


def var_or_num(rng: random.Random) -> str:
    if rng.random() < 0.45:
        return "x"
    if rng.random() < 0.25:
        return f"{rng.choice(['', '-'])}{rng.randrange(1, 10)}.{rng.randrange(0, 10)}"
    return str(rng.randrange(-9, 10))


def power(rng: random.Random, depth: int) -> str:
    base = expr(rng, depth - 1)
    p = rng.choice(["2", "3", "-1", "1/2", str(rng.randrange(2, 6))])
    return f"({base})^{p}"


def func(rng: random.Random, depth: int) -> str:
    name = rng.choice(FUNCS)
    return f"{name}({expr(rng, depth - 1)})"


def product(rng: random.Random, depth: int) -> str:
    left = expr(rng, depth - 1)
    right = expr(rng, depth - 1)
    op = rng.choice(OPS)
    if op == "*" and rng.random() < 0.35:
        return f"({left})({right})"
    return f"({left}){maybe_space(rng)}{op}{maybe_space(rng)}({right})"


def expr(rng: random.Random, depth: int) -> str:
    if depth <= 0:
        return var_or_num(rng)
    choice = rng.random()
    if choice < 0.22:
        return var_or_num(rng)
    if choice < 0.42:
        return func(rng, depth)
    if choice < 0.57:
        return power(rng, depth)
    return product(rng, depth)


def equation(rng: random.Random, depth: int) -> str:
    return f"{expr(rng, depth)}={expr(rng, depth)}"


def nz(rng: random.Random, lo: int = -7, hi: int = 7) -> int:
    n = 0
    while n == 0:
        n = rng.randrange(lo, hi + 1)
    return n


def lin(rng: random.Random) -> str:
    a = nz(rng, -6, 6)
    b = rng.randrange(-9, 10)
    if b == 0:
        return f"{a}*x"
    return f"{a}*x{'+' if b > 0 else ''}{b}"


def affine_x(rng: random.Random, *, nonzero_b: bool = False) -> str:
    a = nz(rng, -6, 6)
    b = rng.randrange(-9, 10)
    if nonzero_b:
        while b == 0:
            b = rng.randrange(-9, 10)
    if b == 0:
        return f"{a}*x"
    return f"{a}*x{b:+d}"


def quad_from_roots(rng: random.Random) -> tuple[str, int, int]:
    r1, r2 = rng.randrange(-6, 7), rng.randrange(-6, 7)
    b = -(r1 + r2)
    c = r1 * r2
    parts = ["x^2"]
    if b:
        parts.append(f"{b:+d}*x")
    if c:
        parts.append(f"{c:+d}")
    return "".join(parts), r1, r2


def rand_diff(rng: random.Random) -> str:
    case = rng.randrange(6)
    if case == 0:
        return f"diff(({lin(rng)})^2,x)"
    if case == 1:
        return f"diff(sin(({lin(rng)})^2),x)"
    if case == 2:
        return f"diff(({lin(rng)})*ln({lin(rng)}),x)"
    if case == 3:
        return f"diff((x^2+{rng.randrange(1,8)})/(x{rng.choice(['-','+'])}{rng.randrange(1,6)}),x)"
    if case == 4:
        return f"diff((x+{rng.randrange(1,6)})(x-{rng.randrange(1,6)})ln(x),x)"
    return f"diff(({lin(rng)})+sin({lin(rng)}),x)"


def rand_integrate(rng: random.Random) -> str:
    case = rng.randrange(7)
    if case == 0:
        a, p = nz(rng, -8, 8), rng.randrange(1, 6)
        return f"integrate({a}*x^{p}+{rng.randrange(-9,10)},x)"
    if case == 1:
        k = nz(rng, 1, 6)
        return f"integrate(sin({k}*x),x)"
    if case == 2:
        k = nz(rng, 1, 6)
        return f"integrate(exp({k}*x),x)"
    if case == 3:
        p = rng.randrange(1, 5)
        return f"integrate(x^{p}*ln(x),x)"
    if case == 4:
        a, b, p = nz(rng, 1, 5), rng.randrange(-5, 6), rng.randrange(2, 5)
        return f"integrate({a}*x*({b}+x^2)^{p},x)"
    if case == 5:
        c = rng.randrange(2, 8)
        return f"integrate(((x)^2)(({c})^2))"
    return f"integrate(({lin(rng)})+cos({nz(rng,1,6)}*x),x)"


def rand_solve(rng: random.Random) -> str:
    case = rng.randrange(5)
    if case == 0:
        a, b, r = nz(rng, -7, 7), rng.randrange(-9, 10), rng.randrange(-9, 10)
        return f"solve({a}*x+{b}={r},x)"
    if case == 1:
        q, r1, r2 = quad_from_roots(rng)
        return f"solve({q}=0,x)"
    if case == 2:
        n = rng.randrange(2, 10)
        return f"solve({n}=(x)^2,x)"
    if case == 3:
        a, b = nz(rng, 1, 6), rng.randrange(-4, 5)
        return f"solve(({a}*x+{b})*ln(x+1)=0,x)"
    return f"solve(tan(x)={rng.randrange(1,6)}/{rng.randrange(2,7)},x)"


def rand_range(rng: random.Random) -> str:
    case = rng.randrange(5)
    if case == 0:
        return f"range({lin(rng)})"
    if case == 1:
        q, _, _ = quad_from_roots(rng)
        return f"range({q})"
    if case == 2:
        return f"range((x)^{rng.choice([3,5])})"
    if case == 3:
        a, b, c = nz(rng, 1, 5), nz(rng, 1, 5), rng.randrange(-8, 9)
        return f"range({a}*sin({b}*x{c:+d}){rng.randrange(-8,9):+d})"
    return rng.choice(["range(exp(x))", "range(ln(x))", "range(sqrt(x))"])


def rand_simplify(rng: random.Random) -> str:
    case = rng.randrange(4)
    if case == 0:
        a = rng.randrange(1, 7)
        return f"simplify((x^2+{2*a}*x+{a*a})/(x+{a})^2)"
    if case == 1:
        a = rng.randrange(1, 7)
        return f"simplify((x^2-{a*a})/(x-{a}))"
    if case == 2:
        return f"simplify(((x)^{rng.randrange(2,6)}))"
    return f"simplify({expr(rng, 3)})"


def rand_xform(rng: random.Random) -> str:
    case = rng.randrange(10)
    if case == 0:
        terms = ["1", "tan(x)^2"]
        rng.shuffle(terms)
        return f"xform({terms[0]}+{terms[1]},sec(x)^2)"
    if case == 1:
        terms = ["sin(x)^2", "cos(x)^2"]
        rng.shuffle(terms)
        return f"xform({terms[0]}+{terms[1]},1)"
    if case == 2:
        terms = ["1", "cot(x)^2"]
        rng.shuffle(terms)
        return f"xform({terms[0]}+{terms[1]},cosec(x)^2)"
    if case == 3:
        n = rng.randrange(2, 7)
        return f"xform(ln(x^{n}),{n}*ln(x))"
    if case == 4:
        base, n = rng.randrange(2, 8), rng.randrange(2, 6)
        return f"xform(log({base},x^{n}),{n}*log({base},x))"
    if case == 5:
        a = rng.randrange(1, 6)
        return f"xform((x+{a})^2,x^2+{2*a}*x+{a*a})"
    if case == 6:
        a = rng.randrange(1, 6)
        return f"xform(x^2+{2*a}*x+{a*a},(x+{a})^2)"
    if case == 7:
        a, b = rng.randrange(1, 6), rng.randrange(1, 6)
        while b == a:
            b = rng.randrange(1, 6)
        return f"xform({a}*sin(x-60)={b}*cos(x-30),tan(x))"
    if case == 8:
        a, b = rng.randrange(1, 6), rng.randrange(1, 6)
        while b == a:
            b = rng.randrange(1, 6)
        return f"xform({a}*sin(x-60)-{b}*cos(x-30)=0,tan(x))"
    a, b = rng.randrange(1, 6), rng.randrange(1, 6)
    return f"xform({a}*sin(x)+{b}cos(x),sqrt({a*a+b*b})*sin(x+atan({b}/{a})))"


def rand_log(rng: random.Random) -> str:
    base = rng.randrange(2, 10)
    arg = rng.choice(["x", f"x^{rng.randrange(2, 6)}", f"({lin(rng)})"])
    return f"log({base},{arg})"


def rand_expand(rng: random.Random) -> str:
    case = rng.randrange(3)
    if case == 0:
        return f"expand(({affine_x(rng)})^2)"
    if case == 1:
        return f"expand(({affine_x(rng, nonzero_b=True)})({affine_x(rng, nonzero_b=True)}))"
    a = rng.randrange(1, 7)
    b = rng.randrange(1, 7)
    return f"expand((x+{a})(x-{b}))"


def rand_binomial(rng: random.Random) -> str:
    sign = rng.choice(["+", "-"])
    a = rng.randrange(1, 9)
    p = rng.choice(["1/2", "-1", "-1/2", "2/3"])
    return f"binomial((1{sign}{a}x)^({p}))"


def rand_apart(rng: random.Random) -> str:
    return "apart(6/(u(3+2u)))"


def rand_defint(rng: random.Random) -> str:
    case = rng.randrange(4)
    if case == 0:
        k = rng.randrange(1, 6)
        return f"integrate(sin({k}x),x,0,pi/{k})"
    if case == 1:
        p = rng.randrange(1, 5)
        return f"integrate(x^{p},x,{rng.randrange(0,3)},{rng.randrange(4,9)})"
    if case == 2:
        k = rng.randrange(1, 6)
        return f"integrate(exp({k}x),x,0,1)"
    return f"integrate(1/(x+{rng.randrange(1,6)}),x,0,{rng.randrange(1,6)})"


def rand_trig(rng: random.Random) -> str:
    case = rng.randrange(4)
    if case == 0:
        a = rng.randrange(1, 6)
        b = rng.randrange(1, 6)
        return f"xform({a}*sin(x)+{b}cos(x),sqrt({a*a+b*b})*sin(x+atan({b}/{a})))"
    if case == 1:
        terms = ["1", "tan(x)^2"]
        rng.shuffle(terms)
        return f"xform({terms[0]}+{terms[1]},sec(x)^2)"
    if case == 2:
        terms = ["1", "cot(x)^2"]
        rng.shuffle(terms)
        return f"xform({terms[0]}+{terms[1]},cosec(x)^2)"
    terms = ["sin(x)^2", "cos(x)^2"]
    rng.shuffle(terms)
    return f"xform({terms[0]}+{terms[1]},1)"


def command_input(rng: random.Random, depth: int, template_rate: float, commands: list[str] | None = None) -> tuple[str, str]:
    cmd = rng.choice(commands or COMMANDS)
    if rng.random() < template_rate and cmd in TEMPLATES:
        return cmd, rng.choice(TEMPLATES[cmd])
    if cmd == "diff":
        return cmd, rand_diff(rng)
    if cmd == "integrate":
        return cmd, rand_integrate(rng)
    if cmd == "simplify":
        return cmd, rand_simplify(rng)
    if cmd == "solve":
        return cmd, rand_solve(rng)
    if cmd == "range":
        return cmd, rand_range(rng)
    if cmd == "xform":
        return cmd, rand_xform(rng)
    if cmd == "log":
        return cmd, rand_log(rng)
    if cmd == "expand":
        return cmd, rand_expand(rng)
    if cmd == "binomial":
        return cmd, rand_binomial(rng)
    if cmd == "apart":
        return cmd, rand_apart(rng)
    if cmd == "defint":
        return cmd, rand_defint(rng)
    if cmd == "trig":
        return cmd, rand_trig(rng)
    start = expr(rng, depth)
    target = rng.choice([f"expand({start})", f"factor({start})", expr(rng, depth), "tan(x)=3*sqrt(3)"])
    return cmd, f"xform({start},{target})"


def noisy_input(rng: random.Random) -> tuple[str, str]:
    n = rng.randrange(1, 80)
    return "noise", "".join(rng.choice(WEIRD_CHARS) for _ in range(n)).strip()


def make_case(rng: random.Random, depth: int, noise_rate: float, template_rate: float, commands: list[str] | None = None) -> tuple[str, str]:
    if rng.random() < noise_rate:
        return noisy_input(rng)
    return command_input(rng, depth, template_rate, commands)


def classify(kind: str, src: str, out: str, code: int, elapsed: float, timeout: bool) -> str:
    stripped = out.strip()
    if timeout:
        return "timeout"
    if code:
        return "crash"
    if not stripped:
        return "empty"
    lines = [line.strip() for line in stripped.splitlines() if line.strip()]
    if "Exact:" in stripped:
        return "exact-label"
    if lines and lines[-1].startswith("Answer:"):
        return "answer-label-final"
    lowered = stripped.lower()
    if any(x in lowered for x in ["segmentation", "abort", "system error", "reboot", "traceback"]):
        return "fatal-text"
    if kind != "noise" and stripped == src.strip():
        return "echo-only"
    if kind != "noise" and ("A-level route:" in stripped or "KhiCAS exact result:" in stripped):
        return "weak-fallback"
    if elapsed > 2.5:
        return "slow"
    return "ok"


def append_progress(done: int, bad: int, status: str) -> None:
    PROGRESS.parent.mkdir(parents=True, exist_ok=True)
    event = {
        "phase": "fuzz",
        "last_event": f"random fuzz {done} done, {bad} bad",
        "queue": f"fuzz {done} cases",
        "tests": status,
    }
    with PROGRESS.open("a", encoding="utf-8") as f:
        f.write(json.dumps(event, separators=(",", ":")) + "\n")


def run_case(src: str, timeout_s: float) -> tuple[int, str, float, bool]:
    start = time.monotonic()
    try:
        p = subprocess.run(
            [str(RUNNER), src],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=timeout_s,
        )
        return p.returncode, (p.stdout or "") + (p.stderr or ""), time.monotonic() - start, False
    except subprocess.TimeoutExpired as exc:
        out = (exc.stdout or "") + (exc.stderr or "")
        return 124, out, time.monotonic() - start, True


def main() -> int:
    ap = argparse.ArgumentParser(description="Random CAS working-output fuzzer.")
    ap.add_argument("--count", type=int, default=200, help="number of tests; ignored with --forever")
    ap.add_argument("--forever", action="store_true", help="run until interrupted")
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("--depth", type=int, default=4)
    ap.add_argument("--timeout", type=float, default=4.0)
    ap.add_argument("--noise-rate", type=float, default=0.08)
    ap.add_argument("--template-rate", type=float, default=0.0, help="optional fixed regression seed rate; default is fully random")
    ap.add_argument("--only", default="", help="comma-separated command names to generate")
    ap.add_argument("--stop-on-fail", action="store_true")
    ap.add_argument("--strict", action="store_true", help="count weak fallback as failure")
    ap.add_argument("--jsonl", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.jsonl")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    only = [x.strip() for x in args.only.split(",") if x.strip()] or None
    args.jsonl.parent.mkdir(parents=True, exist_ok=True)
    done = bad = 0
    append_progress(0, 0, "random-fuzz running")

    def interrupted(_signum, _frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGINT, interrupted)
    with args.jsonl.open("w", encoding="utf-8") as report:
        try:
            while args.forever or done < args.count:
                kind, src = make_case(rng, args.depth, args.noise_rate, args.template_rate, only)
                code, out, elapsed, timeout = run_case(src, args.timeout)
                verdict = classify(kind, src, out, code, elapsed, timeout)
                fail = verdict not in {"ok", "slow"} and (args.strict or verdict != "weak-fallback")
                rec = {
                    "i": done,
                    "kind": kind,
                    "input": src,
                    "verdict": verdict,
                    "returncode": code,
                    "elapsed": round(elapsed, 4),
                    "output": out[:1200],
                }
                report.write(json.dumps(rec, ensure_ascii=False) + "\n")
                if fail:
                    bad += 1
                    print(json.dumps(rec, ensure_ascii=False))
                    done += 1
                    if args.stop_on_fail:
                        break
                    continue
                done += 1
                if done % 25 == 0:
                    append_progress(done, bad, f"random-fuzz {done} done")
        except KeyboardInterrupt:
            append_progress(done, bad, "random-fuzz interrupted")
            print(f"INTERRUPTED random fuzz done={done} bad={bad} report={args.jsonl}")
            return 130 if bad else 0

    append_progress(done, bad, "random-fuzz complete")
    status = "OK" if bad == 0 else "DONE"
    print(f"{status} random fuzz done={done} bad={bad} report={args.jsonl}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
