"""
Local LLM verifier shim for the C++ TUI.

Loads the legacy Ollama manager from python.zip, then replaces its system prompt
with a stricter exam-working verifier.
"""

from __future__ import annotations

from pathlib import Path
import types
import zipfile


REPO_ROOT = Path(__file__).resolve().parents[3]
LEGACY_ZIP = REPO_ROOT / "python.zip"
LEGACY_MEMBER = "python/src/shared_llm.py"


if not LEGACY_ZIP.exists():
    raise ImportError("missing python.zip legacy shared_llm")

_mod = types.ModuleType("_casio_legacy_shared_llm")
_mod.__file__ = str(REPO_ROOT / "python/src/shared_llm.py")
_mod.__name__ = "_casio_legacy_shared_llm"

with zipfile.ZipFile(LEGACY_ZIP) as _zip:
    _code = _zip.read(LEGACY_MEMBER).decode("utf-8", errors="replace")

exec(compile(_code, str(LEGACY_ZIP / LEGACY_MEMBER), "exec"), _mod.__dict__)


LLM_SYSTEM_PROMPT = """Judge CasioCAS maths output like an Edexcel A-level/Further Maths examiner.

Check BOTH:
1. Final answer is mathematically correct/equivalent.
2. Working lines are what a student should write in an exam.

Working-quality rules:
- CORRECT only if the route has enough mathematical steps for marks.
- NEEDS_REVIEW if the answer is correct but working is generic, calculator-meta, or missing key transformations.
- INCORRECT if a shown step is mathematically false, loses solutions, creates invalid domains, or the final answer is wrong.
- Generic/meta lines are not valid working: Chk, CAS verified, parse, fallback, route, simplify only, try methods, calculator checked.
- Valid exam lines include: define substitution, compute du/dx or dx, transform integrand/equation, apply identity, set up partial fractions, equate coefficients, state IBP u/dv/du/v, collect terms, factor, reject invalid roots, state domain/branch/poles when needed.
- Do not demand micro-steps for trivial linear rearrangement.
- For calculus/trig/proofs, big jumps are NEEDS_REVIEW unless the omitted step is genuinely immediate.
- Prefer Edexcel A-level methods over special-function/university methods when possible.
- Equivalent algebraic/trig forms are fine if domains/intervals are respected.
- If unsure, use NEEDS_REVIEW, not INCORRECT.

Reply with the verdict first:
CORRECT
INCORRECT
NEEDS_REVIEW"""

_mod.LLM_SYSTEM_PROMPT = LLM_SYSTEM_PROMPT

for _name, _value in _mod.__dict__.items():
    if not _name.startswith("__"):
        globals()[_name] = _value

globals()["LLM_SYSTEM_PROMPT"] = LLM_SYSTEM_PROMPT
