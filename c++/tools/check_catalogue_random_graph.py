#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys


REPO = Path(__file__).resolve().parents[2]
TUI = REPO / "c++" / "tools" / "tests_cpp" / "run_tests_tui.py"


def main() -> int:
    text = TUI.read_text(encoding="utf-8")
    required = [
        "class RandomExplorationGraph",
        "load_all_catalogue_functions",
        "build_catalogue_explorer_cases",
        "announce_catalogue_graph",
        "CATALOGUE_GRAPH_PATH",
        "CATALOGUE_MANIFEST_PATH",
        "record_random_graph_result",
        "if program is None:",
        "return [(\"CatalogueGraph\", self.build_catalogue_explorer_cases)]",
        "self.random_graph.nodes = {}",
    ]
    missing = [item for item in required if item not in text]
    if missing:
        print("Missing catalogue random graph hooks:", ", ".join(missing), file=sys.stderr)
        return 1
    print("OK: catalogue random graph")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
