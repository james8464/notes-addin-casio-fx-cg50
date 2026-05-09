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
)

GENERIC_WORKING = (
    "tried methods",
    "fallback",
    "use cas",
    "calculator",
    "verified ok",
    "classify the integrand",
    "rewrite into a standard useful form",
    "apply the matching rule",
    "differentiate the result as a check",
    "pick rule",
    "std form",
    "rule/sub/id",
    "verify",
    "done",
    "route:",
    "method:",
    "answer:",
    "rearrange to lhs-rhs=0",
    "use log/exp laws",
    "exponentiate, solve",
    "solve the resulting equation",
    "solve the polynomial in u",
    "u=inner",
    "alpha=base angle",
)

LONE_BAD_LINES = {
    "y =",
    "u =",
    "v =",
    "w =",
    "i =",
    "j =",
    "dy/d",
    "dx/d",
    "du/dx =",
    "dx/du =",
    "i = int(",
}

ALLOWED_TEXT_PREFIXES = (
    "domain:",
    "range:",
    "err:",
    "no elementary",
    "copy casiocas.pak",
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
    return bool(
        re.search(r"\bd[uvwxyzt]\s*/\s*d[uvwxyzt]\b", text)
        or re.search(r"\bd[uvwxyzt]\s*=", text)
        or re.search(r"\bdx\s*=", text)
        or re.search(r"\bdu\s*=", text)
        or re.search(r"\bdv\s*=", text)
        or re.search(r"\bdw\s*=", text)
        or re.search(r"\bdt\s*=", text)
        or re.search(r"\bdx\b\s*/", text)
    )


def _has_bad_line_break(text: str) -> bool:
    for line in _lines(text):
        compact = re.sub(r"\s+", " ", line.strip().lower())
        if compact in LONE_BAD_LINES:
            return True
    return False


def _is_prose_line(line: str) -> bool:
    low = line.strip().lower()
    if not low or low.startswith(ALLOWED_TEXT_PREFIXES):
        return False
    if any(sym in line for sym in "=+-*/^()[]{}<>|"):
        return False
    words = re.findall(r"[a-zA-Z]{3,}", line)
    return len(words) >= 4


def _has_too_much_prose(text: str) -> bool:
    prose = sum(1 for line in _lines(text) if _is_prose_line(line))
    return prose >= 2


def _has_exam_math_route(text: str) -> bool:
    """Accept compact mark-scheme style maths lines without prose markers."""
    lines = _lines(text)
    if len(lines) < 2:
        return False
    route_lines = 0
    for line in lines:
        low = line.lower()
        if (
            "=" in line
            or "=>" in line
            or "integral_" in low
            or "int(" in low
            or re.search(r"\bd[xyuwt]\s*/\s*d[xt]\b", low)
            or re.search(r"\bd[uwt]\s*=", low)
        ):
            route_lines += 1
    return route_lines >= 2


def classify_output_quality(output: str, expects_working: bool = True) -> OutputQuality:
    text = _norm(output)
    if not text.strip():
        return OutputQuality("fail", "empty output")
    if any(item in text for item in BAD_SNIPPETS):
        return OutputQuality("fail", "error/debug/fallback text")
    if _has_bad_line_break(output):
        return OutputQuality("review", "awkward exam line break")
    if "unsupported" in text and "not supported by this route" not in text:
        return OutputQuality("fail", "unsupported without useful route explanation")
    if re.search(r"\b(answer|ans)\s*:\s*(int|integrate|d/dx|diff)\s*\(", text):
        return OutputQuality("fail", "unevaluated supported form")
    final_line = _final_math_line(text)
    if re.search(r"\b(int|integrate|d/dx|diff)\s*\(", final_line) and "no elementary" not in text:
        return OutputQuality("fail", "unevaluated supported form")
    if not expects_working:
        return OutputQuality("pass", "answer-only allowed")
    if "method: forced di" in text or "di table" in text or ("d:" in text and "i:" in text and "signs:" in text):
        if not all(item in text for item in ("d:", "i:", "signs:")):
            return OutputQuality("review", "DI table/alternating signs missing")
    ibp_context = (
        "integration by parts" in text
        or "use parts" in text
        or "method: forced parts" in text
        or re.search(r"\bibp\b", text) is not None
        or "int(v du)" in text
        or "dv=" in text
        or "dv =" in text
    )
    if ibp_context:
        required = ("u", "dv", "du", "v")
        if not all(_has_assignment(text, name) for name in required):
            return OutputQuality("review", "IBP missing u/dv/du/v choices")
    if "partial fraction" in text or re.search(r"\bpf\b", text) or "pf:" in text:
        has_setup = bool(
            re.search(r"\([a-z][a-z0-9+\-\s]*\)\s*/\s*\(", text)
            or re.search(r"[a-z]\s*/\s*\(", text)
            or "a/(x" in text
            or "pf:" in text
        )
        has_coeffs = bool(re.search(r"(^|[^a-z])[abcde]\s*=", text))
        if not (has_setup and has_coeffs):
            return OutputQuality("review", "partial fraction setup/coefficients missing")
    domain_range_context = "domain:" in text or "range:" in text
    calculus_context = (not domain_range_context) and any(item in text for item in ("integral", "differentiate", "dx", "dw", "dt", "ln(", "log("))
    if calculus_context and re.search(r"\blet\s+u\s*=|\bu\s*=", text) and not _has_differential(text):
        return OutputQuality("review", "substitution differential missing")
    if "integral becomes" in text and not _has_differential(text):
        return OutputQuality("review", "substitution differential missing")
    if "implicit" in text or "differentiate both sides" in text:
        if not any(word in text for word in ("collect", "isolate", "factor", "solve for")):
            return OutputQuality("review", "implicit isolation step missing")
    if ("let u=cos" in text or "let u=sin" in text) and "u^2" in text:
        if "u=" not in text or not any(word in text for word in ("reject", "sin(", "cos(")):
            return OutputQuality("review", "trig polynomial roots/rejections missing")
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
    if _has_too_much_prose(output):
        return OutputQuality("review", "too much prose for exam-style working")
    markers = (
        "let ",
        "u =",
        "u=",
        "w=",
        "i=",
        "du =",
        "du=",
        "dw=",
        "dv=",
        "method:",
        "di table",
        "d:",
        "i:",
        "signs:",
        "use ",
        "=>",
        "king property",
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
    if not any(marker in text for marker in markers) and not _has_exam_math_route(text):
        return OutputQuality("review", "missing method markers")
    if any(item in text for item in GENERIC_WORKING):
        return OutputQuality("review", "generic calculator-style working")
    lines = _lines(text)
    if len(lines) < 2:
        return OutputQuality("review", "too few lines for supported working")
    return OutputQuality("pass", "step markers present")
