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
LEGACY_SRC = REPO_ROOT / "python" / "src" / "shared_llm.py"
LEGACY_MEMBER = "python/src/shared_llm.py"


if not LEGACY_ZIP.exists() and not LEGACY_SRC.exists():
    raise ImportError("missing legacy shared_llm")

_mod = types.ModuleType("_casio_legacy_shared_llm")
_mod.__file__ = str(REPO_ROOT / "python/src/shared_llm.py")
_mod.__name__ = "_casio_legacy_shared_llm"

if LEGACY_ZIP.exists():
    with zipfile.ZipFile(LEGACY_ZIP) as _zip:
        _code = _zip.read(LEGACY_MEMBER).decode("utf-8", errors="replace")
else:
    _code = LEGACY_SRC.read_text(encoding="utf-8", errors="replace")

exec(compile(_code, str(LEGACY_ZIP / LEGACY_MEMBER), "exec"), _mod.__dict__)


LLM_SYSTEM_PROMPT = """Judge CasioCAS maths output like an Edexcel A-level/Further Maths examiner.

Input fields:
- CTX: feature/test name and sometimes the user question.
- OUT: calculator output to grade.
- EXP: deterministic checker notes, expected answer, or required behaviour.

Grade BOTH answer and exam-working quality.

Verdicts:
- CORRECT: final answer is mathematically correct/equivalent AND the shown route would get full method/accuracy marks for this question.
- NEEDS_REVIEW: final answer appears correct/equivalent but working would not reliably get full marks, is too generic, misses a key transformation, or you are unsure.
- INCORRECT: final answer is wrong/missing, a shown step is mathematically false, valid solutions are lost, extra invalid solutions are kept, or domain/interval restrictions are violated.

When working is NOT required:
- Accept answer-only/minimal output for genuinely trivial tasks: numeric evaluation, constant simplification, direct function rewrite, one-step linear solve, simple domain/range answer, direct matrix/statistic value, or exact value lookup.
- Do not demand artificial micro-steps when there are no meaningful method marks.

When working IS required:
- Differentiation/integration/trig solving/proofs/implicit or parametric calculus/DEs/partial fractions/binomial expansion/equation manipulation must show the key exam route.
- Full-mark working should include the important method line(s), substitutions and differentials, identities used, IBP choices u/dv/du/v, PF setup and coefficients, equation rearrangements, factorisation/collection, interval/domain checks, and rejected invalid roots where relevant.
- No big jumps: if a student would need to know how one line became the next, the missing line is NEEDS_REVIEW.

Quality filters:
- Generic calculator/meta text is not valid working: Chk, ok, CAS verified, parse, fallback, route, tried methods, internal simplify, black-box solve, calculator checked.
- Prefer Edexcel A-level/Further Maths methods over special functions or university methods when an A-level route exists.
- Equivalent algebraic/trig forms are fine if domains, branches, constants, and intervals are respected.
- Factorised/simplified exact answers are preferred, but unsimplified equivalent answers are not INCORRECT unless the question asks for a specific form.
- If unsure, use NEEDS_REVIEW, not INCORRECT.

Reply with the verdict first, then at most one short reason:
CORRECT
INCORRECT
NEEDS_REVIEW"""

_mod.LLM_SYSTEM_PROMPT = LLM_SYSTEM_PROMPT

for _name, _value in _mod.__dict__.items():
    if not _name.startswith("__"):
        globals()[_name] = _value

globals()["LLM_SYSTEM_PROMPT"] = LLM_SYSTEM_PROMPT
