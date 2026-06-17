#!/usr/bin/env python3
import importlib.util
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FUZZER = ROOT / "tests" / "random_working_fuzzer.py"
RUNNER = ROOT / "tools" / "host" / "khicas_host_runner"

spec = importlib.util.spec_from_file_location("random_working_fuzzer", FUZZER)
fuzzer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fuzzer)

rng = fuzzer.random.Random(980699)
scheduler = fuzzer.ChaosCommandScheduler(rng, ["rewrite"])
src = ""
for i in range(9):
    _kind, src = fuzzer.chaos_case(rng, 10, i, ["rewrite"], scheduler)

start = time.monotonic()
try:
    p = subprocess.run(
        [str(RUNNER), src],
        cwd=ROOT,
        text=True,
        capture_output=True,
        timeout=1.5,
    )
except subprocess.TimeoutExpired as exc:
    raise SystemExit(f"rewrite complexity guard timed out for {len(src)} chars") from exc

elapsed = time.monotonic() - start
if p.returncode:
    raise SystemExit(f"runner exited {p.returncode}: {(p.stderr or p.stdout)[:300]}")
if not (p.stdout or "").strip():
    raise SystemExit("runner returned empty output")
print(f"rewrite complexity guard ok len={len(src)} elapsed={elapsed:.3f}s")
