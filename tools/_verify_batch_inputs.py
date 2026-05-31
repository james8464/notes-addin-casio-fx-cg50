#!/usr/bin/env python3
"""Verify khicas inputs from batch row JSON files."""
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools" / "khicas_host_runner"


def run_one(module: str, inp: str) -> tuple[int, str]:
    low = inp.lower()
    if module == "derive" or low.startswith(("diff(", "derive(")):
        argv = [str(RUNNER), "--derive", inp]
    elif module == "integrate" or low.startswith(("int(", "integrate(", "defint(")):
        argv = [str(RUNNER), "--int", inp]
    elif module == "trig":
        argv = [str(RUNNER), "--trig", inp]
    else:
        argv = [str(RUNNER), "--alg", inp]
    r = subprocess.run(argv, capture_output=True, text=True, cwd=ROOT)
    out = (r.stdout or "") + (r.stderr or "")
    return r.returncode, out


def verify_file(path: Path) -> list[str]:
    rows = json.loads(path.read_text())
    issues: list[str] = []
    for row in rows:
        if row.get("verdict") == "skip":
            continue
        for i, item in enumerate(row.get("inputs", []), 1):
            rc, out = run_one(item["module"], item["input"])
            if rc != 0:
                issues.append(f"{row['id']} input {i}: rc={rc}\n  {item['input']}\n  {out[:500]}")
                continue
            for m in item.get("expected_output_markers", []):
                if m and m not in out:
                    issues.append(
                        f"{row['id']} input {i}: missing marker {m!r}\n"
                        f"  {item['module']} {item['input']}\n  OUT: {out[:800]}"
                    )
    return issues


def main() -> int:
    paths = [Path(p) for p in sys.argv[1:]]
    if not paths:
        print("usage: _verify_batch_inputs.py file.json ...", file=sys.stderr)
        return 2
    total = 0
    for path in paths:
        issues = verify_file(path)
        total += len(issues)
        if issues:
            print(f"=== {path.name} ({len(issues)} issues) ===")
            for issue in issues:
                print(issue)
                print()
    if total:
        print(f"FAIL {total} marker issues")
        return 1
    print(f"OK {len(paths)} files verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
