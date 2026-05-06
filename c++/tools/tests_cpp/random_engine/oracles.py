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


def _lines(text: str) -> list[str]:
    return [ln.strip() for ln in text.splitlines() if ln.strip()]


def _final_math_line(text: str) -> str:
    lines = _lines(text)
    return lines[-1] if lines else ""


def _has_assignment(text: str, name: str) -> bool:
    return re.search(r"(^|[^a-z]){0}\s*=".format(re.escape(name)), text) is not None


def _has_differential(text: str) -> bool:
    return bool(re.search(r"\bd[uwt]\b|\bdx\s*=", text))


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
    final_line = _final_math_line(text)
    if re.search(r"\b(int|integrate|d/dx|diff)\s*\(", final_line) and "no elementary" not in text:
        return OutputQuality("fail", "unevaluated supported form")
    if not expects_working:
        return OutputQuality("pass", "answer-only allowed")
    if "parts" in text:
        required = ("u", "dv", "du", "v")
        if not all(_has_assignment(text, name) for name in required):
            return OutputQuality("review", "IBP missing u/dv/du/v choices")
    if "partial fraction" in text or re.search(r"\bpf\b", text):
        has_setup = bool(re.search(r"[a-z]\s*/\s*\(", text) or "a/(x" in text)
        has_coeffs = bool(re.search(r"(^|[^a-z])[abcde]\s*=", text))
        if not (has_setup and has_coeffs):
            return OutputQuality("review", "partial fraction setup/coefficients missing")
    calculus_context = any(item in text for item in ("integral", "differentiate", "dx", "dw", "dt", "ln(", "log("))
    if calculus_context and re.search(r"\blet\s+u\s*=|\bu\s*=", text) and not _has_differential(text):
        return OutputQuality("review", "substitution differential missing")
    if "integral becomes" in text and not _has_differential(text):
        return OutputQuality("review", "substitution differential missing")
    if "implicit" in text or "differentiate both sides" in text:
        if not any(word in text for word in ("collect", "isolate", "factor", "solve for")):
            return OutputQuality("review", "implicit isolation step missing")
    if "dx/dt" in text or "dy/dt" in text:
        if "dy/dx" not in text:
            return OutputQuality("review", "parametric dy/dx step missing")
    if ("mod 2*pi" in text or "modulo 2*pi" in text or "interval" in text) and "x =" in text:
        if not any(word in text for word in ("base", "arcsin", "arccos", "arctan", "atan", "asin", "acos")):
            return OutputQuality("review", "trig solve base angle missing")
        if not any(word in text for word in ("keep", "interval", "check", "reject")):
            return OutputQuality("review", "trig interval/check step missing")
    if "binomial" in text and not any(word in text for word in ("valid", "|x|", "range")):
        return OutputQuality("review", "binomial validity range missing")
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
    lines = _lines(text)
    if len(lines) < 2:
        return OutputQuality("review", "too few lines for supported working")
    return OutputQuality("pass", "step markers present")
