#!/usr/bin/env python3
"""
Generate golden fixtures from the current Python engines (oracle).

Why this exists:
- The C++ port needs a strict target to avoid feature regressions.
- Your repo already contains a conservative RTF question-bank mapper in
  `python/tests/run_rtf_tests.py`. We reuse the same parsing/mapping so we test
  the same "exam bank" questions you care about.

Output:
- JSON lines (one case per line) to stdout by default, or to --out.

Each record contains:
- id / label
- program script path used (eg src/Math/trigProgram.py)
- stdin payload used to drive the CLI program
- stdout text produced
- expected answer text (when extracted from the bank)
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from dataclasses import asdict
from pathlib import Path
from typing import List, Optional, Tuple


REPO = Path(__file__).resolve().parents[3]
PY_ROOT = REPO / "python"


def _import_rtf_runner():
    sys.path.insert(0, str(PY_ROOT))
    sys.path.insert(0, str(PY_ROOT / "tests"))
    # Import the existing conservative mapper/runner.
    import run_rtf_tests as RTF  # type: ignore

    return RTF


def _read_txt_lines(path: Path) -> List[str]:
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def _write_jsonl(out_path: Optional[Path], rows: List[dict]) -> None:
    if out_path is None:
        for row in rows:
            sys.stdout.write(json.dumps(row, ensure_ascii=False) + "\n")
        return
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as f:
        for row in rows:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--rtf",
        default=str(PY_ROOT / "Tests.rtf"),
        help="Path to Tests.rtf (default: python/Tests.rtf)",
    )
    ap.add_argument(
        "--out",
        default=None,
        help="Write JSONL to this path (default: stdout)",
    )
    ap.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Limit number of mapped cases",
    )
    ap.add_argument(
        "--timeout-s",
        type=int,
        default=15,
        help="Per-case timeout for running python programs",
    )
    args = ap.parse_args()

    rtf_path = Path(args.rtf)
    if not rtf_path.exists():
        raise SystemExit(f"RTF file not found: {rtf_path}")

    RTF = _import_rtf_runner()

    # Convert RTF -> txt using the same mechanism as the runner (macOS textutil).
    txt_path = rtf_path.with_suffix(".txt")
    if sys.platform == "darwin":
        RTF.convert_rtf_to_txt(rtf_path, txt_path)
    else:
        # If not on macOS, allow users to pre-generate the .txt themselves.
        if not txt_path.exists():
            raise SystemExit(
                "Non-macOS: pre-generate Tests.txt next to Tests.rtf, or run on macOS.\n"
                f"Expected: {txt_path}"
            )

    blocks = RTF.split_blocks(_read_txt_lines(txt_path))
    rows: List[dict] = []

    mapped = 0
    for i, block in enumerate(blocks):
        block.expected_answer_text = RTF.extract_expected_answer(block)
        mapped_info: Optional[Tuple[str, str]] = RTF.map_to_program_input(block)
        if not mapped_info:
            continue
        script_rel, payload = mapped_info
        code, out = RTF._run(  # noqa: SLF001 (we intentionally reuse runner internals)
            [sys.executable, script_rel],
            cwd=PY_ROOT,
            timeout_s=int(args.timeout_s),
            input_text=payload,
        )
        rows.append(
            {
                "case_index": mapped,
                "qid": block.qid,
                "program": block.program,
                "subtopic": block.subtopic,
                "script_relpath": script_rel,
                "stdin": payload,
                "exit_code": code,
                "stdout": out,
                "expected_answer_text": block.expected_answer_text,
                "header": block.header_line,
            }
        )
        mapped += 1
        if args.limit is not None and mapped >= args.limit:
            break

    out_path = Path(args.out) if args.out else None
    _write_jsonl(out_path, rows)

    sys.stderr.write(f"Mapped {len(rows)} cases out of {len(blocks)} blocks.\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

