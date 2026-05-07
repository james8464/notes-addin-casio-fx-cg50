#!/usr/bin/env python3
"""Unit checks for the MadAsMaths full-audit ledger helper."""

from __future__ import annotations

import importlib.util
from pathlib import Path


REPO = Path(__file__).resolve().parents[3]
AUDIT = REPO / "c++" / "tools" / "golden" / "check_madasmaths_full_audit.py"


def load_audit():
    spec = importlib.util.spec_from_file_location("check_madasmaths_full_audit", AUDIT)
    mod = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    spec.loader.exec_module(mod)
    return mod


def main() -> int:
    mod = load_audit()
    assert mod.case_paper_code("A10 trig-rational range") == "a"
    assert mod.case_paper_code("S Q14 substitution integral") == "s"
    assert mod.case_paper_code("mp2_z Q9 separable DE") == "z"
    assert mod.case_qid("A10 trig-rational range") == "10"
    assert mod.case_qid("S Q14 substitution integral") == "14"
    assert mod.compact("a + b\n") == "a+b"
    print("OK madasmaths full-audit unit")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
