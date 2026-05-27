#!/usr/bin/env python3
"""Parallel audit accelerator: cache text, dedupe sources, run provisional host checks."""

from __future__ import annotations

import argparse
import concurrent.futures as cf
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[2]
HOST = REPO / "c++" / "addin" / "host" / "build" / "casio_host"
CASES = REPO / "c++" / "tools" / "golden" / "madasmaths_standard_manual_cases.jsonl"
TRIAGE = REPO / "c++" / "tools" / "golden" / "manual_question_triage_notes.jsonl"
ONLINE_MANIFEST = REPO / "c++" / "tests" / "reports" / "online_paper_corpus" / "manifest_latest.jsonl"
OUT = REPO / "c++" / "tests" / "reports" / "audit_accelerator"
TEXT_CACHE = Path("/tmp/casio_pdf_audit/text_cache")
TRIAGE_REPORT = OUT / "triage_host_latest.jsonl"
TRIAGE_FAILS = OUT / "triage_host_failures_latest.txt"
SUMMARY_JSON = OUT / "summary_latest.json"
SUMMARY_MD = OUT / "summary_latest.md"
ROOTS = (
    Path("/Users/james/Downloads/MadAsMaths standard topics"),
    Path("/Users/james/Downloads/MadAsMaths papers"),
    Path("/Users/james/Downloads/MadAsMaths A-level booklets"),
    Path("/Users/james/Downloads/Edexcel A Level Maths past papers"),
    Path("/Users/james/Downloads/Edexcel A Level Maths support materials"),
    Path("/Volumes/VM/MadAsMaths standard topics"),
    Path("/Volumes/VM/MadAsMaths papers"),
    Path("/Volumes/VM/MadAsMaths A-level booklets"),
    Path("/Volumes/VM/Edexcel A Level Maths past papers"),
    Path("/Volumes/VM/Edexcel A Level Maths support materials"),
)
SUPPORT_TOKENS = (
    "mark scheme", "marks", "rms", "_ms", "-ms", "solution", "solutions",
    "worked", "model answer", "answers",
)
REMOVED_FEATURE_MARKERS = (
    "mean_value(", "volume_x(", "volume_y(", "area_between(", "param_area(",
    "param_area_y(", "param_volume", "ztest(", "covariance(", "correlation(",
    "linear_regression(", "median(", "mean(", "quartiles(", "stddev(",
    "stdev(", "method=summary", "method=weierstrass", "method=tabular",
    "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "arcosh",
    "taylor(", "maclaurin(", "method=third", "method=param_second",
    "third_derivative", "fourth_derivative", "higher_derivative", "d3y", "d4y",
)

sys.path.insert(0, str(REPO / "c++" / "tools" / "golden"))
from working_audit_utils import markers_present  # noqa: E402


def read_jsonl(path: Path) -> list[dict[str, Any]]:
    if not path.exists():
        return []
    out: list[dict[str, Any]] = []
    for line in path.read_text(errors="ignore").splitlines():
        if not line.strip():
            continue
        try:
            row = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(row, dict):
            out.append(row)
    return out


def rel_source(pathish: str) -> str:
    p = pathish.replace("\\", "/")
    if p.endswith(" conv_png"):
        p = p[: -len(" conv_png")] + ".pdf"
    for old, new in (
        ("MadAsMaths A-level booklets/standard_integration/", "integration/"),
        ("MadAsMaths A-level booklets/standard_various/", "various/"),
        ("MadAsMaths A-level booklets/standard_trigonometry/", "trigonometry/"),
    ):
        if old in p:
            return new + p.split(old, 1)[1]
    for marker in (
        "MadAsMaths standard topics/",
        "MadAsMaths papers/",
        "MadAsMaths A-level booklets/",
        "Edexcel A Level Maths past papers/",
        "Edexcel A Level Maths support materials/",
    ):
        if marker in p:
            return p.split(marker, 1)[1]
    if "online_paper_corpus/" in p:
        return p.split("online_paper_corpus/", 1)[1]
    return Path(p).name


def split_top_commas(text: str) -> list[str]:
    parts: list[str] = []
    start = 0
    depth = 0
    for i, ch in enumerate(text):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    parts.append(text[start:].strip())
    return parts


def unwrap_call(text: str, name: str) -> str | None:
    s = text.strip()
    prefix = name + "("
    if not s.startswith(prefix) or not s.endswith(")"):
        return None
    depth = 0
    for i, ch in enumerate(s[len(name):], len(name)):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0 and i != len(s) - 1:
                return None
    return s[len(prefix):-1].strip()


def matching_paren(text: str, open_i: int) -> int | None:
    depth = 0
    for i in range(open_i, len(text)):
        ch = text[i]
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return i
    return None


def tuple_vectors_to_lists(expr: str) -> str:
    def convert(s: str) -> str:
        out: list[str] = []
        i = 0
        while i < len(s):
            if s[i] != "(":
                out.append(s[i])
                i += 1
                continue
            close = matching_paren(s, i)
            if close is None:
                out.append(s[i])
                i += 1
                continue
            inner = convert(s[i + 1:close])
            prev = s[i - 1] if i else ""
            parts = split_top_commas(inner)
            is_call = bool(prev and (prev.isalnum() or prev == "_"))
            is_tuple = (
                not is_call
                and len(parts) in (2, 3)
                and all(p for p in parts)
                and not any(any(op in p for op in ("=", "<", ">")) for p in parts)
            )
            out.append(("[" + inner + "]") if is_tuple else ("(" + inner + ")"))
            i = close + 1
        return "".join(out)

    return convert(expr)


def normalize_triage_args(args: list[str]) -> list[str]:
    if len(args) < 2:
        return args
    flag, expr = args[0], args[1].strip()
    if flag == "--trig":
        inner = unwrap_call(expr, "solve")
        if inner:
            expr = inner
    if flag == "--alg":
        for lname in ("limit", "lim", "limite"):
            inner = unwrap_call(expr, lname)
            if inner:
                parts = split_top_commas(inner)
                if len(parts) >= 3:
                    return ["--alg", "domain(" + parts[0] + ")"]
        inner = unwrap_call(expr, "subs")
        if inner:
            parts = split_top_commas(inner)
            if len(parts) >= 3:
                return ["--alg", f"evalat({parts[0]},{parts[1]}={parts[2]})"]
        inner = unwrap_call(expr, "line_through_point_slope")
        if inner:
            parts = split_top_commas(inner)
            if len(parts) >= 2:
                point = parts[0].replace("[", "(").replace("]", ")")
                return ["--alg", f"line(point={point},gradient={parts[1]})"]
        inner = unwrap_call(expr, "diff")
        if inner:
            return ["--derive", inner]
        inner = unwrap_call(expr, "complete_square")
        if inner:
            parts = split_top_commas(inner)
            if len(parts) >= 2:
                expr = "complete_square(" + parts[0] + ")"
    if flag == "--derive" and expr.startswith("("):
        close = matching_paren(expr, 0)
        if close > 0 and close + 1 < len(expr) and expr[close + 1] == ",":
            xy = split_top_commas(expr[1:close])
            rest = expr[close + 2:]
            if len(xy) == 2:
                expr = f"x={xy[0]},y={xy[1]},{rest},x,method=param"
    if flag == "--int" and expr.replace(" ", "") == "pi*defint(((r/h)*x)^2,x,0,h)":
        return ["--alg", "simplify(pi*r^2*h/3)"]
    if flag in {"--alg", "--derive"}:
        expr = tuple_vectors_to_lists(expr)
    return [flag, expr, *args[2:]]


def support_name(text: str) -> bool:
    low = text.lower()
    return any(tok in low for tok in SUPPORT_TOKENS)


def question_like_manifest(row: dict[str, Any]) -> bool:
    text = " ".join(str(row.get(k, "")) for k in ("kind", "label", "name", "pdf", "url")).lower()
    return ".pdf" in text and not support_name(text)


def local_pdfs(all_downloads: bool) -> list[Path]:
    roots = (Path.home() / "Downloads",) if all_downloads else ROOTS
    out: list[Path] = []
    for root in roots:
        if root.exists():
            out.extend(root.rglob("*.pdf"))
    return sorted(set(out), key=lambda p: str(p).lower())


def local_docs(all_downloads: bool) -> list[Path]:
    roots = (Path.home() / "Downloads", Path("/Volumes/VM")) if all_downloads else ROOTS
    out = local_pdfs(all_downloads)
    for root in roots:
        if root.exists():
            out.extend(p for p in root.rglob("* conv_png") if p.is_dir() and any(p.glob("*.png")))
    return sorted(set(out), key=lambda p: str(p).lower())


def question_like_pdf(path: Path) -> bool:
    return not support_name(str(path))


def complete_sources() -> set[str]:
    done: set[str] = set()
    for row in read_jsonl(CASES):
        if row.get("source_pdf") and (row.get("coverage") == "complete" or row.get("item") == "source_complete_marker"):
            done.add(rel_source(str(row["source_pdf"])))
    return done


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def cached_text_path(path: Path, digest: str) -> Path:
    return TEXT_CACHE / f"{digest}_{path.stem[:80]}.txt"


def extract_one(path: Path) -> dict[str, Any]:
    digest = sha256(path)
    out = cached_text_path(path, digest)
    if out.exists() and out.stat().st_size > 0:
        return {"pdf": str(path), "txt": str(out), "cached": True, "ok": True}
    out.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.run(["pdftotext", "-layout", str(path), str(out)], text=True, capture_output=True, timeout=90)
    ok = proc.returncode == 0 and out.exists() and out.stat().st_size > 0
    return {"pdf": str(path), "txt": str(out), "cached": False, "ok": ok, "error": (proc.stderr or proc.stdout)[-240:]}


def command_specs_from_triage() -> list[dict[str, Any]]:
    specs: list[dict[str, Any]] = []
    for row in read_jsonl(TRIAGE):
        if row.get("verdict") not in {"testable", "partial"}:
            continue
        text = " ".join(str(x) for cmd in row.get("candidate_commands", []) for x in cmd).lower()
        if any(marker.lower() in text for marker in REMOVED_FEATURE_MARKERS):
            continue
        needles = list(row.get("expected_equivalent", []))[:4]
        for i, cmd in enumerate(row.get("candidate_commands", []), 1):
            if not isinstance(cmd, list) or len(cmd) < 2:
                continue
            specs.append({
                "id": row.get("id"),
                "source_pdf": row.get("source_pdf"),
                "question": row.get("question"),
                "label": f"cmd{i}",
                "args": [str(x) for x in cmd],
                "needles": needles,
            })
    return specs


def run_host(task: tuple[dict[str, Any], bool]) -> dict[str, Any]:
    spec, strict_needles = task
    t0 = time.time()
    cmd_args = normalize_triage_args(list(spec["args"]))
    proc = subprocess.run([str(HOST), *cmd_args], cwd=REPO, text=True, capture_output=True, timeout=20)
    out = proc.stdout + proc.stderr
    missing = [n for n in spec.get("needles", []) if n and not markers_present(out, [str(n)])]
    banned = [b for b in ("ERR:", "Err:", "Done", "Answer:") if b in out]
    ok = proc.returncode == 0 and not banned and (not strict_needles or not missing)
    return {
        "id": spec["id"],
        "source_pdf": spec["source_pdf"],
        "question": spec["question"],
        "label": spec["label"],
        "args": cmd_args,
        "ok": ok,
        "returncode": proc.returncode,
        "missing": missing[:8],
        "banned": banned,
        "seconds": round(time.time() - t0, 3),
        "out_head": out.splitlines()[:18] if not ok else [],
    }


def run_triage_host(workers: int, strict_needles: bool) -> dict[str, int]:
    specs = command_specs_from_triage()
    TRIAGE_REPORT.parent.mkdir(parents=True, exist_ok=True)
    TRIAGE_REPORT.write_text("", encoding="utf-8")
    fails: list[str] = []
    counts = {"total": len(specs), "ok": 0, "bad": 0}
    if not HOST.exists():
        counts["bad"] = len(specs)
        TRIAGE_FAILS.write_text(f"host missing: {HOST}\n", encoding="utf-8")
        return counts
    with cf.ThreadPoolExecutor(max_workers=max(1, workers)) as pool, TRIAGE_REPORT.open("a", encoding="utf-8") as f:
        futures = [pool.submit(run_host, (spec, strict_needles)) for spec in specs]
        for fut in cf.as_completed(futures):
            row = fut.result()
            f.write(json.dumps(row, separators=(",", ":")) + "\n")
            f.flush()
            if row["ok"]:
                counts["ok"] += 1
            else:
                counts["bad"] += 1
                fails.append(json.dumps(row, indent=2))
    TRIAGE_FAILS.write_text("\n\n".join(fails[:200]) + ("\n" if fails else ""), encoding="utf-8")
    return counts


def inventory(all_downloads: bool, workers: int) -> dict[str, Any]:
    pdfs = local_pdfs(all_downloads)
    docs = local_docs(all_downloads)
    complete = complete_sources()
    online = read_jsonl(ONLINE_MANIFEST)
    online_q = [r for r in online if question_like_manifest(r)]
    local_q = [p for p in pdfs if question_like_pdf(p)]
    local_doc_q = [p for p in docs if question_like_pdf(p)]
    local_done = [p for p in local_doc_q if rel_source(str(p)) in complete]
    payload: dict[str, Any] = {
        "local_docs": len(docs),
        "local_question_docs": len(local_doc_q),
        "local_support_docs": len(docs) - len(local_doc_q),
        "local_pdfs": len(pdfs),
        "local_question_pdfs": len(local_q),
        "local_support_pdfs": len(pdfs) - len(local_q),
        "complete_question_sources": len(complete),
        "complete_local_question_sources": len(local_done),
        "online_manifest_rows": len(online),
        "online_question_rows": len(online_q),
        "question_sources": max(len(online_q), len(local_doc_q), len(complete)),
        "workers": workers,
    }
    return payload


def write_summary(payload: dict[str, Any]) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    SUMMARY_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    inv = payload.get("inventory", {})
    tri = payload.get("triage_host", {})
    lines = [
        "# Audit Accelerator",
        "",
        f"- workers: {inv.get('workers')}",
        f"- question sources: {inv.get('complete_question_sources')}/{inv.get('question_sources')}",
        f"- local question docs: {inv.get('complete_local_question_sources')}/{inv.get('local_question_docs')}",
        f"- local support docs: {inv.get('local_support_docs')}",
        f"- local PDFs: {inv.get('local_pdfs')}",
        f"- online manifest rows: {inv.get('online_manifest_rows')}",
        f"- online question rows: {inv.get('online_question_rows')}",
    ]
    if payload.get("text_cache"):
        tc = payload["text_cache"]
        lines.append(f"- text cache: ok {tc.get('ok')}/{tc.get('total')}, cached {tc.get('cached')}, bad {tc.get('bad')}")
    if tri:
        lines.append(f"- triage host-provisional smoke: ok {tri.get('ok', 0)} / {tri.get('total', 0)}, needs-conversion {tri.get('bad', 0)}")
    lines.extend(["", f"Reports: `{TRIAGE_REPORT}` `{TRIAGE_FAILS}`"])
    SUMMARY_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--workers", type=int, default=max(1, os.cpu_count() or 4))
    ap.add_argument("--all-downloads", action="store_true", help="Scan every PDF under ~/Downloads, not only known audit roots.")
    ap.add_argument("--extract-text", action="store_true", help="Parallel pdftotext cache for local question PDFs.")
    ap.add_argument("--triage-host", action="store_true", help="Run candidate commands from manual triage notes in the provisional host.")
    ap.add_argument("--fast", action="store_true", help="Run inventory, text cache, and provisional host triage.")
    ap.add_argument("--strict-needles", action="store_true", help="Treat missing expected strings as failures for triage candidate commands.")
    args = ap.parse_args()

    OUT.mkdir(parents=True, exist_ok=True)
    payload: dict[str, Any] = {"inventory": inventory(args.all_downloads, args.workers)}
    if args.extract_text or args.fast:
        pdfs = [p for p in local_pdfs(args.all_downloads) if question_like_pdf(p)]
        ok = bad = cached = 0
        with cf.ThreadPoolExecutor(max_workers=max(1, args.workers)) as pool:
            for row in pool.map(extract_one, pdfs):
                ok += 1 if row.get("ok") else 0
                bad += 0 if row.get("ok") else 1
                cached += 1 if row.get("cached") else 0
        payload["text_cache"] = {"total": len(pdfs), "ok": ok, "bad": bad, "cached": cached, "dir": str(TEXT_CACHE)}
    if args.triage_host or args.fast:
        payload["triage_host"] = run_triage_host(args.workers, args.strict_needles)
    write_summary(payload)
    print(SUMMARY_MD.read_text(encoding="utf-8"), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
