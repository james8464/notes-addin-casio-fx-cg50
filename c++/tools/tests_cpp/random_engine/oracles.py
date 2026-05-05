from __future__ import annotations

from dataclasses import dataclass
import re


BAD_SNIPPETS = (
    "parser error",
    "traceback",
    "timeout after",
    "dimension mismatch",
    "could not",
    "unable",
    "no working",
    "chk:",
    "check:",
)

GENERIC_WORKING = (
    "tried methods",
    "fallback",
    "use cas",
    "calculator",
    "verified ok",
)


@dataclass
class OutputQuality:
    status: str
    reason: str


def _norm(text: str) -> str:
    return (text or "").replace("\r\n", "\n").lower()


def classify_output_quality(output: str, expects_working: bool = True) -> OutputQuality:
    text = _norm(output)
    if not text.strip():
        return OutputQuality("fail", "empty output")
    if any(item in text for item in BAD_SNIPPETS):
        return OutputQuality("fail", "error/debug/fallback text")
    if "unsupported" in text and "not supported by this route" not in text:
        return OutputQuality("fail", "unsupported without useful route explanation")
    if re.search(r"\b(answer|ans)\s*:\s*(int|integrate|d/dx|diff)\s*\(", text):
        return OutputQuality("fail", "unevaluated supported form")
    if re.search(r"\b(int|integrate|d/dx|diff)\s*\(", text) and "no elementary" not in text:
        return OutputQuality("fail", "unevaluated supported form")
    if not expects_working:
        return OutputQuality("pass", "answer-only allowed")
    markers = (
        "let ",
        "u =",
        "du =",
        "method:",
        "use ",
        "no elementary",
        "special-function",
        "branch",
        "domain",
        "parts",
        "factor",
        "identity",
        "differentiate",
        "substitute",
        "partial fraction",
        "range",
    )
    if not any(marker in text for marker in markers):
        return OutputQuality("review", "missing method markers")
    if any(item in text for item in GENERIC_WORKING):
        return OutputQuality("review", "generic calculator-style working")
    lines = [ln for ln in text.splitlines() if ln.strip()]
    if len(lines) < 2:
        return OutputQuality("review", "too few lines for supported working")
    return OutputQuality("pass", "step markers present")
