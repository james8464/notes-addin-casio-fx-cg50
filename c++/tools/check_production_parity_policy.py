#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
QUEUE = ROOT / "c++/tools/golden/run_exact_calculator_input_queue.py"
README = ROOT / "README.md"
GRAPH = ROOT / "c++/GRAPH.md"
SCOPE = ROOT / "c++/tools/golden/a_level_scope.md"


def need(path: Path, marker: str) -> None:
    text = path.read_text(errors="ignore")
    if marker not in text:
        raise SystemExit(f"FAIL {path}: missing {marker!r}")


def main() -> int:
    need(QUEUE, "--engine")
    need(QUEUE, "host-provisional is not calculator proof")
    need(QUEUE, "production engine runner missing")
    need(README, "Host modules under `c++/addin/src/modules/` are fast regression/prototype code")
    need(README, "After any host/prototype solver change, run `./compile` before trusting calculator parity")
    need(GRAPH, "Never claim calculator behavior from host-only evidence")
    need(SCOPE, "they are not the default `.g3a` working engine")
    print("OK production parity policy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
