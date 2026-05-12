"""
Local LLM verifier shim for the C++ TUI.

Loads the legacy Ollama manager from python.zip, then replaces its system prompt
with a stricter exam-working verifier.
"""

from __future__ import annotations

from pathlib import Path
import os
import re
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


LLM_SYSTEM_PROMPT = """Judge CasioCAS maths output like a strict symbolic-maths examiner.

Input fields:
- CTX: feature/test name and sometimes the user question.
- OUT: calculator output to grade.
- EXP: deterministic checker notes, expected answer, or required behaviour.

Grade BOTH answer and step-by-step working quality. Full marks means method marks as well as a correct final answer.

Target style:
- The ideal output is what a student could copy into an exam: mostly maths lines, concise, readable, and in logical order.
- Do NOT require numbered lines, paragraph explanations, headings, or full sentences.
- Accept compact handwritten/markscheme style such as `u=...`, `du=...`, `I=...`, `=>`, coefficient equations, factorisation lines, and final exact answer.
- Prefer fewer wider lines over unnecessary line breaks, provided no algebraic jump is hidden.
- Reject awkward line breaks that split one mathematical statement, e.g. a standalone `y =`, `u =`, `I =`, `dy/d`, `dx/d`, or a value pushed to the next line.
- Short labels such as `Domain:`, `Range:`, `PF:`, `D:`, `I:`, `Signs:` are fine. Generic teaching prose is not.

Verdicts:
- CORRECT: final answer is mathematically correct/equivalent AND the shown route would get full method/accuracy marks for this question, at the question's natural level.
- NEEDS_REVIEW: final answer appears correct/equivalent but working would not reliably get full marks, is too generic, misses a key transformation, hides a branch/domain issue, or you are unsure.
- INCORRECT: final answer is wrong/missing, a shown step is mathematically false, valid solutions are lost, extra invalid solutions are kept, or domain/interval restrictions are violated.

When working is NOT required:
- Accept answer-only/minimal output for genuinely trivial tasks: numeric evaluation, constant simplification, direct function rewrite, one-step linear solve, simple domain/range answer, direct matrix/statistic value, stats summaries/plots, or exact value lookup.
- Accept compact contradiction output for invalid, no-variable, or nonsensical constant equations, e.g. `24 != 0` with `solution = []`; do not require an invented variable, interval, or fsolve working.
- Do not demand artificial micro-steps when there are no meaningful method marks.
- For direct standard rules, one formula line plus answer is enough, e.g. `d/dx(tan x)=sec^2 x` or `cos^2 x=(1+cos2x)/2`.

When working IS required:
- Differentiation/integration/trig solving/proofs/implicit or parametric calculus/DEs/partial fractions/binomial expansion/equation manipulation must show the key route.
- For supported non-trivial A-level/Further Maths routes, answer-only output is NEEDS_REVIEW even when the final line is correct.
- Full-mark working should include the important method line(s), substitutions and differentials, identities used, IBP choices u/dv/du/v, PF setup and coefficients, equation rearrangements, factorisation/collection, interval/domain checks, and rejected invalid roots where relevant.
- For DI/tabular integration by parts, `D:`, `I:`, `Signs:` plus the final collected answer is full method evidence; do not require separate u/dv/du/v lines.
- For crash-safety/depth-guard/generated stress tests, answer-only or unchanged-expression output is fine if it is bounded, non-crashing, and mathematically sane.
- No big jumps: if a student would need to know how one line became the next, the missing line is NEEDS_REVIEW.
- Worksheet traps to police especially hard: looping IBP must show the repeated integral being collected; substitutions must show du/dx or dx/du and back-substitution; partial fractions must show the assumed form and coefficient values; trig equations must show identity/R-form plus interval filtering; implicit/parametric differentiation must show collection/isolation of derivatives; binomial expansions must show validity conditions when needed.
- If the problem is beyond A-level/Further Maths, still grade it: accept correct special-function, branch-aware, implicit, numeric, or non-elementary conclusions only when the output explains why that route is needed.
- For very large generated expressions, accept structured placeholders like `f1`, `f2`, `u`, `v` if each is defined and the rule connecting them is correct. Do not demand the whole expanded derivative on every line.

Source/markscheme-guided tests:
- EXP may include public source family notes and required marker phrases from mark schemes.
- Treat each required marker as evidence of a method mark, not as exact wording.
- NEEDS_REVIEW if a source/markscheme case has the right answer but skips a required marker such as changed limits, coefficient equations, rejection of roots, validity range, reference triangle, or collection of a repeated integral.

Quality filters:
- Generic calculator/meta text is not valid working: Chk, ok, CAS verified, parse, fallback, route, tried methods, internal simplify, black-box solve, calculator checked.
- Also reject generic scaffold phrases: Done, pick rule, Std form, Rule/sub/id, Verify, use log/exp laws, solve the resulting equation, apply matching rule.
- If the output is mostly sentences rather than mathematical lines, mark NEEDS_REVIEW even if the final answer is correct.
- Do not reject output merely because it has no line numbers or prose.
- Do not reject terse mathematical notation if it is copyable and method-mark complete.
- Prefer A-level/Further Maths methods over special functions or university methods when such a route exists, but do not reject advanced methods for genuinely advanced inputs.
- Equivalent algebraic/trig forms are fine if domains, branches, constants, and intervals are respected.
- Factorised/simplified exact answers are preferred, but unsimplified equivalent answers are not INCORRECT unless the question asks for a specific form.
- If unsure, use NEEDS_REVIEW, not INCORRECT.

Batch mode:
- Only applies when the prompt starts with BATCH_VERIFY.
- Grade each numbered item independently.
- Never answer "same as item N", "as above", or rely on another item.
- If two items are similar, repeat the actual reason for this item.

Single item mode:
- If the prompt starts with CTX, grade only that one OUT/EXP pair.
- Do not mention, compare, or number other items.
- Reply as one verdict plus one short reason.

Reply with the verdict first, then at most one short reason:
CORRECT
INCORRECT
NEEDS_REVIEW"""

_mod.LLM_SYSTEM_PROMPT = LLM_SYSTEM_PROMPT

for _name, _value in _mod.__dict__.items():
    if not _name.startswith("__"):
        globals()[_name] = _value

globals()["LLM_SYSTEM_PROMPT"] = LLM_SYSTEM_PROMPT


def _strict_batch_parse(response_text, n):
    verdicts = [None] * n
    notes = [""] * n
    if not response_text:
        return verdicts, notes
    pattern = re.compile(
        r"^\s*#?\s*(\d+)\s*[:|.)-]?\s*(CORRECT|INCORRECT|NEEDS_REVIEW)\b\s*[-:|]?\s*(.*)$",
        re.IGNORECASE,
    )
    for line in str(response_text).splitlines():
        m = pattern.match(line.strip())
        if not m:
            continue
        idx = int(m.group(1)) - 1
        if 0 <= idx < n:
            verdicts[idx] = m.group(2).upper()
            notes[idx] = line.strip()[:500]
    return verdicts, notes


def _strict_verify_batch(self, items, check_working_quality=False, stream_callback=None):
    n = len(items)
    if n == 0:
        return []
    if check_working_quality or n == 1 or os.environ.get("CASIO_LLM_BATCH_DISABLE", "").lower() in ("1", "true", "yes"):
        return [self.verify(a, b, c, check_working_quality, stream_callback) for a, b, c in items]
    if not self.enabled or not self.selected_model:
        return [
            {"verdict": "DISABLED", "explanation": "LLM not enabled", "confidence": 0, "cached": False}
            for _ in range(n)
        ]

    out_chars = int(os.environ.get("CASIO_LLM_BATCH_OUT_CHARS", "1200"))
    body = []
    for i, (program_output, expected, context) in enumerate(items, 1):
        po = self._truncate_out(program_output, out_chars)
        ex = self._truncate_out(expected, min(2000, out_chars * 2))
        body.append(f"ITEM {i}\nCTX: {context}\nOUT:\n{po}\nEXP:\n{ex}\n")

    header = (
        f"BATCH_VERIFY: {n} independent items.\n"
        f"Reply with EXACTLY {n} lines, one per item, no intro/outro.\n"
        "Line format: #N VERDICT - one short item-specific reason\n"
        "VERDICT is CORRECT, INCORRECT, or NEEDS_REVIEW.\n"
        "Do not merge items. Do not write 'same as above'. Do not include extra numbered lists.\n\n"
    )
    prompt = header + "\n".join(body) + "\n" + LLM_SYSTEM_PROMPT

    try:
        timeout = min(600, int(os.environ.get("CASIO_LLM_TIMEOUT", "60")) + 20 * n)
        result = self._query_ollama(prompt, stream_callback, timeout=timeout)
    except Exception as err:
        self.last_error = str(err)
        return [self.verify(a, b, c, False, None) for a, b, c in items]

    verdicts, notes = _strict_batch_parse(result, n)
    out = []
    for i, (a, b, c) in enumerate(items):
        v = verdicts[i]
        if v in ("CORRECT", "INCORRECT", "NEEDS_REVIEW"):
            out.append({"verdict": v, "explanation": notes[i], "confidence": 0.9, "cached": False})
        else:
            out.append(self.verify(a, b, c, False, None))
    return out


if "LLMManager" in globals():
    LLMManager.verify_batch = _strict_verify_batch
