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
COMMANDS = ["diff", "integrate", "simplify", "solve", "range", "xform"]
WEIRD_CHARS = string.ascii_letters + string.digits + "+-*/^=(),[]{} ._"


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


def command_input(rng: random.Random, depth: int) -> tuple[str, str]:
    cmd = rng.choice(COMMANDS)
    if cmd == "diff":
        return cmd, f"diff({expr(rng, depth)},{rng.choice(['x', ' x ', 'x '])})"
    if cmd == "integrate":
        method = "" if rng.random() < 0.75 else f",x,{rng.choice(['1', '2', '3'])}"
        return cmd, f"integrate({expr(rng, depth)}{method})"
    if cmd == "simplify":
        return cmd, f"simplify({expr(rng, depth)})"
    if cmd == "solve":
        return cmd, f"solve({equation(rng, depth)},x)"
    if cmd == "range":
        return cmd, f"range({expr(rng, depth)})"
    start = expr(rng, depth)
    target = rng.choice([f"expand({start})", f"factor({start})", expr(rng, depth)])
    return cmd, f"xform({start},{target})"


def noisy_input(rng: random.Random) -> tuple[str, str]:
    n = rng.randrange(1, 80)
    return "noise", "".join(rng.choice(WEIRD_CHARS) for _ in range(n)).strip()


def make_case(rng: random.Random, depth: int, noise_rate: float) -> tuple[str, str]:
    if rng.random() < noise_rate:
        return noisy_input(rng)
    return command_input(rng, depth)


def classify(kind: str, src: str, out: str, code: int, elapsed: float, timeout: bool) -> str:
    stripped = out.strip()
    if timeout:
        return "timeout"
    if code:
        return "crash"
    if not stripped:
        return "empty"
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
    ap.add_argument("--stop-on-fail", action="store_true")
    ap.add_argument("--strict", action="store_true", help="count weak fallback as failure")
    ap.add_argument("--jsonl", type=Path, default=ROOT / "tests" / "reports" / "random_fuzz_latest.jsonl")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    args.jsonl.parent.mkdir(parents=True, exist_ok=True)
    done = bad = 0
    append_progress(0, 0, "random-fuzz running")

    def interrupted(_signum, _frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGINT, interrupted)
    with args.jsonl.open("w", encoding="utf-8") as report:
        try:
            while args.forever or done < args.count:
                kind, src = make_case(rng, args.depth, args.noise_rate)
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
