#!/usr/bin/env python3
# Copy/paste in terminal to launch TUI:
# cd /Users/james/Developer/Python/CASIO && python3 run_tests.py tui
# Then type /run, /random, /llm 2, or /infinite.

"""
CASIO Test Suite - Interactive TUI
Expandable test results with rich interface.
"""

from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from fractions import Fraction
import cmath
import json
import math
import os
import random
import re
import resource
import threading
import subprocess
import sys
from pathlib import Path
from dataclasses import dataclass
from enum import Enum

try:
    import sympy as SP
    from sympy.parsing.sympy_parser import (
        parse_expr as SP_PARSE_EXPR,
        standard_transformations as SP_STANDARD_TRANSFORMS,
        implicit_multiplication_application as SP_IMPLICIT_MULT,
        convert_xor as SP_CONVERT_XOR,
    )
    SYMPY_AVAILABLE = True
except Exception:
    SP = None
    SP_PARSE_EXPR = None
    SP_STANDARD_TRANSFORMS = ()
    SP_IMPLICIT_MULT = None
    SP_CONVERT_XOR = None
    SYMPY_AVAILABLE = False

try:
    from textual.app import App, ComposeResult
    from textual.containers import Container, Vertical, Horizontal
    from textual.widgets import Static, Input, RichLog
    TEXTUAL_AVAILABLE = True
except ImportError:
    TEXTUAL_AVAILABLE = False

_TESTS_DIR = Path(__file__).resolve().parent
REPO_ROOT = Path(__file__).resolve().parents[3]
_LEGACY_PY_ROOT = REPO_ROOT / "python"
_LEGACY_ZIP = REPO_ROOT / "python.zip"
SRC_ROOT = _LEGACY_PY_ROOT / 'src'
ROOT = SRC_ROOT / 'Math'
PY = sys.executable
if _LEGACY_ZIP.exists():
    sys.path.insert(0, str(_LEGACY_ZIP) + "/python/src")
    sys.path.insert(0, str(_LEGACY_ZIP) + "/python/src/Math")
    sys.path.insert(0, str(_LEGACY_ZIP) + "/python/tests")
if SRC_ROOT.exists():
    sys.path.insert(0, str(SRC_ROOT))
if ROOT.exists():
    sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(_TESTS_DIR))
try:
    from llm_test_prompts import LLM_GENERATION_PROMPTS
except Exception:
    LLM_GENERATION_PROMPTS = {}

try:
    from shared_quality import (
        has_working_steps as shared_has_working_steps,
        has_solution as shared_has_solution,
        is_exam_format as shared_is_exam_format,
    )
    from shared_reasoning_markers import REASONING_MARKERS as SHARED_REASONING_MARKERS
except Exception:
    pass

try:
    from shared_llm import LLMManager, check_ollama_available, get_ollama_models
    LLM_AVAILABLE = True
except ImportError:
    LLM_AVAILABLE = False
    LLMManager = None
    check_ollama_available = None
    get_ollama_models = None

try:
    from random_engine import AdversarialGenerator, EXAM_GAP_TOPICS, RunReportStore, classify_output_quality
    ADVERSARIAL_ENGINE_AVAILABLE = True
except Exception:
    AdversarialGenerator = None
    RunReportStore = None
    classify_output_quality = None
    EXAM_GAP_TOPICS = ()
    ADVERSARIAL_ENGINE_AVAILABLE = False

if "SHARED_REASONING_MARKERS" not in globals():
    SHARED_REASONING_MARKERS = ("method:", "use ", "let ", "solve ", "answer:")

if "shared_has_working_steps" not in globals():
    def shared_has_working_steps(output):
        text = normalized_text(output) if "normalized_text" in globals() else (output or "").lower()
        return any(marker.lower() in text for marker in SHARED_REASONING_MARKERS)

if "shared_has_solution" not in globals():
    def shared_has_solution(output):
        text = normalized_text(output) if "normalized_text" in globals() else (output or "").lower()
        return any(marker in text for marker in ("answer:", "ans =", "final =", "result:", "x =", "dy/dx", "+ c"))

if "shared_is_exam_format" not in globals():
    def shared_is_exam_format(output):
        return shared_has_working_steps(output) and shared_has_solution(output)

try:
    import algebraProgram as ALG
    import trigProgram as TRIG
    import deriveProgram as DERIVE
    import intProgram as INTEGRATE
    import SUVATprogram as SUVAT
except Exception:
    ALG = TRIG = DERIVE = INTEGRATE = SUVAT = None

class TestStatus(Enum):
    PASS = "pass"
    FAIL = "fail"


class RunState(Enum):
    # Long random runs need a real lifecycle, especially when /stop arrives from
    # the TUI or a stop file.  run_state is the single source of truth.
    IDLE = "idle"
    RUNNING = "running"
    STOPPING = "stopping"
    STOPPED = "stopped"


@dataclass
class TestRecord:
    test_id: int
    label: str
    status: TestStatus
    output: str
    program: str
    input_text: str = ""
    check_info: str = ""
    feature: str = ""
    sympy_verdict: str = ""
    llm_verdict: str = ""
    llm_explanation: str = ""
    # Aligned with status; updated to LLM-weighted result when the random runner uses LLM.
    passed: bool = True
    # Result from the harness checker only; not modified after add_test.
    harness_status: TestStatus = TestStatus.PASS
    # True when LLM disagrees or is unsure but the harness did not fail.
    review_needed: bool = False
    graph_node: str = ""
    graph_meta: object = None


@dataclass
class CaseSpec:
    label: str
    program: str
    runner: object
    input_text: str = ""
    expected: str = ""
    check_info: str = ""
    feature: str = ""
    script: str = ""
    checker: object = None
    graph_node: str = ""
    graph_meta: object = None
    use_calculated: bool = True


@dataclass
class CatalogueFunction:
    signature: str
    name: str
    params: tuple


CATALOGUE_GRAPH_PATH = REPO_ROOT / "c++" / "tools" / "fuzz" / "random_exploration_graph.json"
CATALOGUE_MANIFEST_PATH = REPO_ROOT / "c++" / "tools" / "fuzz" / "catalogue_manifest_latest.txt"

RANDOM_SUPPORTED_MATH_THINGS = (
    "sin", "cos", "tan", "sec", "cot", "cosec", "csc",
    "abs", "sqrt", "ln", "log", "log_base", "exp",
    "asin", "acos", "atan", "arcsin", "arccos", "arctan",
    "sign",
    "factorial_bang", "factorial_fn",
)
RANDOM_SUPPORTED_MATH_SHAPES = tuple("math_" + name for name in RANDOM_SUPPORTED_MATH_THINGS)
RANDOM_REQUIRED_MATH_SHAPE_SENTINELS = ("math_log_base", "math_factorial_bang")

CATALOGUE_HIDDEN_PREFIXES = (
    "debug", "python", "read", "write", "purge", "append", "map", "seq", "sort", "quote",
    "rand", "ranm", "ranv", "draw_", "plotfield", "plotode", "plotcontour", "plotseq",
    "avance", "recule", "saute", "tourne", "crayon", "tortue", "rgb", "display", "gl_",
    "axes", "time", "input", "charpoly", "hilbert", "lu", "qr", "svd", "jordan", "curl",
    "a2q", "q2a", "cond", "erf", "erfc", "hermite", "laguerre", "legendre", "tchebyshev",
    "powmod", "nextprime", "ichinrem", "iabcuv", "idivis", "iegcd", "residue", "bool_",
    "prove_bool",
)
CATALOGUE_HIDDEN_EXACT = {
    "!", "_", "a and b", "a or b", "circle", "inf", "line", "mult_conjugate",
    "nand", "nor", "not", "point", "polygon", "segment",
}


def _catalogue_hidden(name):
    base = (name or "").split("(", 1)[0].strip()
    if not base:
        return True
    if " " in base:
        return True
    if base != "!" and not any(ch.isalnum() or ch == "_" for ch in base):
        return True
    if base in CATALOGUE_HIDDEN_EXACT:
        return True
    return any(base.startswith(prefix) for prefix in CATALOGUE_HIDDEN_PREFIXES)


def _strip_cpp_if0_blocks(text):
    out = []
    disabled = 0
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("#if 0"):
            disabled += 1
            continue
        if disabled:
            if stripped.startswith("#if"):
                disabled += 1
            elif stripped.startswith("#endif"):
                disabled -= 1
            continue
        out.append(line)
    return "\n".join(out)


def _split_top_level_csv(text):
    parts = []
    cur = []
    depth = 0
    for ch in text:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth = max(0, depth - 1)
        if ch == "," and depth == 0:
            parts.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    tail = "".join(cur).strip()
    if tail or parts:
        parts.append(tail)
    return parts


def parse_catalogue_signature(signature):
    sig = signature.strip()
    if "(" not in sig or not sig.endswith(")"):
        return sig, ()
    name, rest = sig.split("(", 1)
    raw_params = rest[:-1]
    params = []
    for raw in _split_top_level_csv(raw_params):
        if not raw:
            continue
        optional = raw.startswith("[") and raw.endswith("]")
        clean = raw[1:-1] if optional else raw
        params.append({"name": clean.strip(), "optional": optional})
    return name.strip(), tuple(params)


def load_all_catalogue_functions():
    path = REPO_ROOT / "c++" / "khicas" / "upstream" / "giac90_1addin" / "catalogen.cpp"
    try:
        text = _strip_cpp_if0_blocks(path.read_text(encoding="utf-8", errors="ignore"))
    except OSError:
        return []
    m = re.search(r"completeCat\[\]\s*=\s*\{(.*?)\n\};", text, re.S)
    if not m:
        return []
    seen = set()
    funcs = []
    for line in m.group(1).splitlines():
        if line.strip().startswith("//"):
            continue
        hit = re.search(r'\{\s*"([^"]+)"\s*,', line)
        if not hit:
            continue
        sig = hit.group(1)
        name, params = parse_catalogue_signature(sig)
        if _catalogue_hidden(name) or sig in seen:
            continue
        seen.add(sig)
        funcs.append(CatalogueFunction(sig, name, params))
    return funcs


class RandomExplorationGraph:
    def __init__(self, path):
        self.path = Path(path)
        self.nodes = {}
        self._dirty = 0
        self.load()

    def load(self):
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
            self.nodes = data.get("nodes", {}) if isinstance(data, dict) else {}
        except Exception:
            self.nodes = {}

    def save(self):
        if not self._dirty:
            return
        self.path.parent.mkdir(parents=True, exist_ok=True)
        payload = {"version": 1, "nodes": self.nodes}
        tmp = self.path.with_name(
            f"{self.path.name}.{os.getpid()}.{threading.get_ident()}.tmp"
        )
        tmp.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
        tmp.replace(self.path)
        self._dirty = 0

    def ensure_node(self, key, meta):
        node = self.nodes.setdefault(
            key,
            {
                "attempts": 0,
                "pass": 0,
                "fail": 0,
                "weak": 0,
                "last_status": "new",
                "meta": dict(meta or {}),
            },
        )
        if meta:
            node["meta"] = dict(meta)
        return node

    def score(self, key):
        node = self.nodes.get(key)
        if not node:
            return (0, 0)
        if node.get("pass", 0) > 0:
            return (3, node.get("attempts", 0))
        if node.get("fail", 0) or node.get("weak", 0):
            return (1, node.get("attempts", 0))
        return (0, node.get("attempts", 0))

    def record(self, key, meta, status, input_text="", output=""):
        if not key:
            return
        node = self.ensure_node(key, meta)
        node["attempts"] = int(node.get("attempts", 0)) + 1
        if status == "pass":
            node["pass"] = int(node.get("pass", 0)) + 1
        elif status == "weak":
            node["weak"] = int(node.get("weak", 0)) + 1
        else:
            node["fail"] = int(node.get("fail", 0)) + 1
        node["last_status"] = status
        node["last_input"] = (input_text or "")[:240]
        node["last_output"] = (output or "").replace("\n", " ")[:320]
        self._dirty += 1
        if self._dirty >= 25:
            self.save()

    def summary(self):
        total = len(self.nodes)
        passed = sum(1 for n in self.nodes.values() if n.get("pass", 0) > 0)
        failed = sum(1 for n in self.nodes.values() if n.get("fail", 0) > 0)
        weak = sum(1 for n in self.nodes.values() if n.get("weak", 0) > 0)
        return total, passed, failed, weak


@dataclass
class RandomTestBatch:
    program: str
    builder: object
    count: int
    cycle: int
    chunk_index: int
    first_for_program: bool


_DEFAULT_FORBIDDEN_SNIPPETS = (
    "err:",
    "traceback",
    "unexpected token",
    "answer: int(",
    "answer: d/dx(",
    "division by zero",
    "needs poly support",
    "unsupported inverse family",
    "bad mode",
)

_NON_EXAM_QUALITY_PATTERNS = (
    "scan one period numerically",
    "scan numerically",
    "compute f(x) at",
    "bisection method",
    "newton-raphson",
    "numerical scan",
    "brute force",
    "f(x) at n points",
    "points across",
    "scan one period",
    "scan the interval",
    "out of scope.",
    "no standard exam-method antiderivative found",
)


def _default_case_workers():
    override = os.environ.get("CASIO_TEST_WORKERS")
    if override:
        try:
            return max(1, int(override))
        except ValueError:
            pass
    return max(2, min(8, os.cpu_count() or 8))


CASE_WORKERS = _default_case_workers()

LLM_GENERATION_CHANCE = float(os.environ.get("CASIO_LLM_GENERATION_CHANCE", "0.1"))
# Infinite /random: cases per program per cycle (was forced to 1 with LLM; now configurable).
_CASIO_INFINITE_GENERATION_CHUNK = max(1, int(os.environ.get("CASIO_INFINITE_GENERATION_CHUNK", "12")))
# LLM: verify N subprocess results per Ollama call (set 1 to disable batching).
_CASIO_LLM_BATCH_SIZE = max(1, int(os.environ.get("CASIO_LLM_BATCH_SIZE", "10")))

def _fast_case_workers():
    try:
        return os.cpu_count() or 8
    except Exception:
        return 16

FAST_CASE_WORKERS = _fast_case_workers()
CASE_WORKERS = max(CASE_WORKERS, FAST_CASE_WORKERS)
try:
    CASIO_TUI_FPS = max(4, min(30, int(os.environ.get("CASIO_TUI_FPS", "12"))))
except ValueError:
    CASIO_TUI_FPS = 12

# Feature tags used by random chaos so we can report gaps (prefix of CaseSpec.feature).
RANDOM_CHAOS_EXPECTED_ALGEBRA = (
    "algebra_compare",
    "algebra_transform",
    "algebra_expand",
    "algebra_poly",
    "algebra_complete_square",
    "algebra_solve",
    "algebra_comp",
    "algebra_inverse",
    "algebra_rewrite",
    "algebra_domain",
    "algebra_range",
    "algebra_domain_interval",
    "algebra_cartesian",
    "algebra_hidden_quadratic",
    "algebra_simultaneous_hard",
    "algebra_circle_hard",
    "algebra_discriminant",
)
FEATURE_PARITY_EXPECTED = {
    "Algebra": RANDOM_CHAOS_EXPECTED_ALGEBRA + (
        "algebra_rearrange",
        "algebra_hard_solve",
        "algebra_solve_letter",
        "algebra_compare_letter",
        "algebra_expand_letter",
    ),
    "Trigonometry": (
        "trig_prove",
        "trig_transform",
        "trig_solve",
        "trig_rewrite",
        "trig_rearrange",
        "trig_madas",
        "trig_identity_hard",
        "trig_multi",
        "trig_radian",
    ),
    "Derive": (
        "derive_normal",
        "derive_implicit",
        "derive_parametric",
        "derive_product",
        "derive_chain_quotient",
        "derive_implicit_product",
        "derive_log_diff",
    ),
    "Integrate": (
        "integrate_auto",
        "integrate_sub",
        "integrate_parts",
        "integrate_trig",
        "integrate_pf",
        "integrate_div",
        "integrate_direct",
        "integrate_extreme_rewrite",
        "integrate_parts_twice",
        "integrate_partial",
        "integrate_trig_power",
    ),
    "Stats": (
        "stats_one_var",
        "stats_regression",
        "stats_binomial",
        "stats_normal",
        "stats_ztest",
        "stats_plot",
    ),
    "SUVAT": (
        "suvat_find_s",
        "suvat_find_u",
        "suvat_find_v",
        "suvat_find_a",
        "suvat_find_t",
        "suvat_edge",
        "suvat_expected_error",
        "suvat_uni",
    ),
    "Boolean": (
        "boolean_simplify",
        "boolean_nand",
        "boolean_nor",
        "boolean_prove",
    ),
    "MethodSurface": (
        "method_surface:int",
        "method_surface:derive",
        "method_surface:trig",
        "method_surface:alg",
        "method_surface:stats",
        "method_surface:suvat",
        "method_surface:invalid",
    ),
    "ExamMix": (
        "algebra_",
        "trig_",
        "derive_",
        "integrate_",
        "stats_",
        "suvat_",
        "boolean_",
    ),
}
FEATURE_PARITY_NOTES = {
    "Algebra": "Python modes 1-9: compare, transform, expand, polynomial, complete square, solve, compose, inverse, rewrite, domain/range/cartesian helpers.",
    "Trigonometry": "Python modes 1-4: prove, transform, solve, rewrite; degree/radian and rearranged identity forms.",
    "Derive": "Normal, implicit, parametric; chain/product/quotient/log cases.",
    "Integrate": "Python modes 1-2 plus methods: direct, substitution, parts, trig, partial fractions, division, DE.",
    "Stats": "C++ extension: one-var, regression, binomial, normal, z-test, graph sparkline.",
    "SUVAT": "Python solver parameters s/u/v/a/t with target inferred/marked; exact rationals/surds and edge errors.",
    "Boolean": "Python modes 1-4: simplify, NAND, NOR, prove.",
    "MethodSurface": "Direct host method/options surface across every supported feature group, including invalid method fallback.",
    "ExamMix": "Cross-feature stress loop; catches route gaps between program-specific builders.",
    "ExamGap": "Worksheet-style traps: right answer but missing full-mark method lines.",
}
SYMPY_MAX_CHARS = int(os.environ.get("CASIO_SYMPY_MAX_CHARS", "260"))
SYMPY_MAX_CALC_CHARS = int(os.environ.get("CASIO_SYMPY_MAX_CALC_CHARS", "1400"))
SYMPY_MAX_OPS = int(os.environ.get("CASIO_SYMPY_MAX_OPS", "90"))
CASE_TIMEOUT_SECONDS = float(os.environ.get("CASIO_TEST_TIMEOUT", "12"))
CALCULATED_CHECK_MAX_INPUT_CHARS = int(os.environ.get("CASIO_CALCULATED_CHECK_MAX_INPUT_CHARS", "80"))

# Exam-style random: higher weight = more cases per builder in chaos (see _feature_count_alloc).
# Set CASIO_EXAM_STRESS=0 for flat round-robin. CASIO_EXAM_BOOST=name:mult,name2:mult overrides.
EXAM_BUILDER_STRESS_WEIGHTS = {
    "random_trig_equation_multi_case": 1.5,
    "random_trig_solve_case": 1.28,
    "random_trig_mad_as_maths_case": 1.22,
    "random_trig_radian_hard_case": 1.15,
    "random_trig_identity_hard_case": 1.18,
    "random_trig_rearrange_case": 1.22,
    "random_algebra_solve_case": 1.3,
    "random_algebra_extreme_rearranged_case": 1.42,
    "random_algebra_hidden_quadratic_case": 1.2,
    "random_algebra_simultaneous_hard_case": 1.15,
    "random_algebra_circle_line_hard_case": 1.1,
    "random_algebra_discriminant_case": 1.1,
    "random_algebra_rearrange_case": 1.18,
    "random_integrate_auto_case": 1.12,
    "random_integrate_sub_case": 1.05,
    "random_integrate_parts_case": 1.08,
    "random_integrate_partial_frac_case": 1.08,
    "random_integrate_extreme_rewrite_case": 1.35,
    "random_method_surface_case": 1.5,
    "random_derive_normal_case": 1.1,
    "random_suvat_edge_case": 1.12,
}


def _parse_casio_exam_boost_env():
    raw = (os.environ.get("CASIO_EXAM_BOOST", "") or "").strip()
    if not raw:
        return {}
    out = {}
    for part in raw.split(","):
        part = part.strip()
        if not part or ":" not in part:
            continue
        k, v = part.split(":", 1)
        k, v = k.strip(), v.strip()
        if not k:
            continue
        try:
            out[k] = float(v)
        except ValueError:
            pass
    return out


def limited_by(limit, value):
    return limit > 0 and value > limit


def _safe_worker_cap():
    try:
        soft_limit = resource.getrlimit(resource.RLIMIT_NOFILE)[0]
    except Exception:
        soft_limit = 256
    cpu_cap = max(2, min(16, (os.cpu_count() or 8) * 2))
    fd_cap = max(2, min(16, soft_limit // 16))
    return max(2, min(cpu_cap, fd_cap))


def split_top_level_text(text, ch=","):
    out = []
    depth = 0
    start = 0
    i = 0
    while i < len(text):
        cur = text[i]
        if cur == "(":
            depth += 1
        elif cur == ")":
            depth -= 1
        elif cur == ch and depth == 0:
            out.append(text[start:i].strip())
            start = i + 1
        i += 1
    out.append(text[start:].strip())
    return out


def split_equation_text(text):
    def wraps_outer_parens(value):
        value = value.strip()
        if not (value.startswith('(') and value.endswith(')')):
            return False
        depth = 0
        i = 0
        while i < len(value):
            cur = value[i]
            if cur == "(":
                depth += 1
            elif cur == ")":
                depth -= 1
                if depth == 0 and i != len(value) - 1:
                    return False
            if depth < 0:
                return False
            i += 1
        return depth == 0

    depth = 0
    i = 0
    while i < len(text):
        cur = text[i]
        if cur == "(":
            depth += 1
        elif cur == ")":
            depth -= 1
        elif cur == "=" and depth == 0:
            left = text[:i].strip()
            right = text[i + 1 :].strip()
            # Clean up right side - strip outer parens if they wrap a simple value
            right = right.strip()
            if wraps_outer_parens(right):
                right = right[1:-1]
            return left, right
        i += 1
    
    # Handle case like "(x^2-4=0)" or "(x^2-4 = 0)" - strip outer parens if they wrap the entire content
    text = text.strip()
    if wraps_outer_parens(text):
        inner = text[1:-1]
        # Check if inner has balanced parens and contains '='
        if inner.count('(') == inner.count(')') and '=' in inner:
            # Recursively split on the inner content
            return split_equation_text(inner)
    
    return text.strip(), "0"


def trig_prove_cli(left, right=None, route="1"):
    if right is None:
        left, right = split_equation_text(left)
    return "1\n{}\n{}\n{}\n".format(left, right, route)


def _log_fn(*args):
    if len(args) == 1:
        return math.log(args[0])
    if len(args) == 2:
        return math.log(args[1], args[0])
    raise ValueError("log expects 1 or 2 arguments")


def _clog_fn(*args):
    if len(args) == 1:
        return cmath.log(args[0])
    if len(args) == 2:
        return cmath.log(args[1], args[0])
    raise ValueError("log expects 1 or 2 arguments")


def to_python_expr(expr):
    out = (expr or "").strip()
    out = out.replace("^", "**")
    out = re.sub(r"ln\|([^|]+)\|", r"log(abs(\1))", out)
    out = re.sub(r"\|([^|]+)\|", r"abs(\1)", out)
    out = out.replace("ln(", "log(")
    out = out.replace("arcsin(", "asin(")
    out = out.replace("arccos(", "acos(")
    out = out.replace("arctan(", "atan(")
    out = re.sub(r"\((\d+\s*[+\-]\s*\d+)\)!", r"factorial(\1)", out)
    out = re.sub(r"(?<![A-Za-z0-9_])(\d+)!", r"factorial(\1)", out)
    out = re.sub(r"(?<![A-Za-z_])e(?![A-Za-z_])", "E_CONST", out)
    out = re.sub(r"(?<=\d)(?=[A-Za-z_(])", "*", out)
    out = re.sub(r"(?<=\))(?=[A-Za-z_(])", "*", out)
    out = re.sub(r"(?<=[A-Za-z0-9_])(?=\()", lambda m: "" if m.string[m.start()-len_of_trailing_ident(m.string, m.start()):m.start()] in _PY_FUNC_NAMES else "*", out)
    return out


_PY_FUNC_NAMES = {"sin","cos","tan","sec","cosec","csc","cot","sqrt","log","ln","asin","acos","atan","exp","abs","arcsin","arccos","arctan","factorial"}


def len_of_trailing_ident(s, end):
    i = end
    while i > 0:
        c = s[i-1]
        if c == "_" or ("a" <= c <= "z") or ("A" <= c <= "Z") or ("0" <= c <= "9"):
            i -= 1
        else:
            break
    return end - i


def to_sympy_expr_text(expr):
    out = (expr or "").strip()
    out = re.sub(r"ln\|([^|]+)\|", r"log(abs(\1))", out)
    out = re.sub(r"\|([^|]+)\|", r"abs(\1)", out)
    out = out.replace("ln(", "log(")
    out = out.replace("arcsin(", "asin(")
    out = out.replace("arccos(", "acos(")
    out = out.replace("arctan(", "atan(")
    out = re.sub(r"\bcosec\s*\(", "csc(", out)
    return out


def sympy_locals():
    if not SYMPY_AVAILABLE:
        return {}
    x, y, t = SP.symbols("x y t")
    return {
        "x": x,
        "y": y,
        "t": t,
        "pi": SP.pi,
        "e": SP.E,
        "E_CONST": SP.E,
        "sin": SP.sin,
        "cos": SP.cos,
        "tan": SP.tan,
        "sec": SP.sec,
        "cosec": SP.csc,
        "csc": SP.csc,
        "cot": SP.cot,
        "exp": SP.exp,
        "log": SP.log,
        "ln": SP.log,
        "sqrt": SP.sqrt,
        "abs": SP.Abs,
        "asin": SP.asin,
        "acos": SP.acos,
        "atan": SP.atan,
    }


def sympy_parse_expr(expr, max_chars=None):
    if not SYMPY_AVAILABLE:
        return None
    limit = SYMPY_MAX_CHARS if max_chars is None else max_chars
    if limited_by(limit, len(expr or "")):
        return None
    transformations = SP_STANDARD_TRANSFORMS + (SP_IMPLICIT_MULT, SP_CONVERT_XOR)
    try:
        return SP_PARSE_EXPR(
            to_sympy_expr_text(expr),
            local_dict=sympy_locals(),
            transformations=transformations,
            evaluate=True,
        )
    except Exception:
        return None


def sympy_safe_text(*exprs):
    joined = " ".join(expr or "" for expr in exprs)
    if limited_by(SYMPY_MAX_CHARS, len(joined)):
        return False
    return True


def sympy_safe_text_limit(limit, *exprs):
    joined = " ".join(expr or "" for expr in exprs)
    if limited_by(limit, len(joined)):
        return False
    return True


def sympy_simplify_difference(left, right):
    if not SYMPY_AVAILABLE:
        return None
    try:
        diff = left - right
        op_count = SP.count_ops(diff)
        if limited_by(SYMPY_MAX_OPS, op_count):
            return None
        candidates = [diff, SP.expand(diff), SP.cancel(diff), SP.factor(diff), SP.together(diff)]
        if SYMPY_MAX_OPS <= 0 or op_count <= SYMPY_MAX_OPS // 2 or (diff.has(SP.sin, SP.cos, SP.tan, SP.sec, SP.csc, SP.cot) and op_count <= 120):
            candidates.extend([SP.trigsimp(diff), SP.simplify(diff), SP.simplify(SP.trigsimp(SP.together(diff)))])
        for candidate in candidates:
            if candidate == 0:
                return True
            equals = candidate.equals(0)
            if equals is True:
                return True
            if equals is False:
                return False
    except Exception:
        return None
    return None


def sympy_expressions_equivalent(left_text, right_text, var="x"):
    if not SYMPY_AVAILABLE:
        return None
    if not sympy_safe_text(left_text, right_text):
        return None
    left = sympy_parse_expr(left_text)
    right = sympy_parse_expr(right_text)
    if left is None or right is None:
        return None
    return sympy_simplify_difference(left, right)


def sympy_derivative_equivalent(expr_text, candidate_text, var="x"):
    if not SYMPY_AVAILABLE:
        return None
    if not sympy_safe_text_limit(SYMPY_MAX_CALC_CHARS, expr_text, candidate_text):
        return None
    expr = sympy_parse_expr(expr_text, max_chars=SYMPY_MAX_CALC_CHARS)
    candidate = sympy_parse_expr(candidate_text, max_chars=SYMPY_MAX_CALC_CHARS)
    if expr is None or candidate is None:
        return None
    if limited_by(SYMPY_MAX_CALC_CHARS, len(expr_text or "") + len(candidate_text or "")):
        return None
    if limited_by(SYMPY_MAX_OPS * 3 if SYMPY_MAX_OPS > 0 else 0, SP.count_ops(expr) + SP.count_ops(candidate)):
        return None
    try:
        symbol = sympy_locals().get(var, SP.symbols(var))
        expected = SP.diff(expr, symbol)
    except Exception:
        return None
    return sympy_simplify_difference(candidate, expected)


def sympy_implicit_derivative_equivalent(eq_text, candidate_text):
    if not SYMPY_AVAILABLE:
        return None
    if not sympy_safe_text_limit(SYMPY_MAX_CALC_CHARS, eq_text, candidate_text):
        return None
    try:
        lhs, rhs = split_equation_text(eq_text)
    except Exception:
        return None
    left = sympy_parse_expr(lhs, max_chars=SYMPY_MAX_CALC_CHARS)
    right = sympy_parse_expr(rhs, max_chars=SYMPY_MAX_CALC_CHARS)
    candidate = sympy_parse_expr(candidate_text, max_chars=SYMPY_MAX_CALC_CHARS)
    if left is None or right is None or candidate is None:
        return None
    try:
        x = sympy_locals().get("x", SP.symbols("x"))
        y = sympy_locals().get("y", SP.symbols("y"))
        relation = left - right
        expected = -SP.diff(relation, x) / SP.diff(relation, y)
    except Exception:
        return None
    return sympy_simplify_difference(candidate, expected)


def sympy_antiderivative_equivalent(candidate_text, integrand_text, var="x"):
    if not SYMPY_AVAILABLE:
        return None
    if not sympy_safe_text_limit(SYMPY_MAX_CALC_CHARS, candidate_text, integrand_text):
        return None
    candidate = sympy_parse_expr(candidate_text, max_chars=SYMPY_MAX_CALC_CHARS)
    integrand = sympy_parse_expr(integrand_text, max_chars=SYMPY_MAX_CALC_CHARS)
    if candidate is None or integrand is None:
        return None
    if limited_by(SYMPY_MAX_CALC_CHARS, len(candidate_text or "") + len(integrand_text or "")):
        return None
    if limited_by(SYMPY_MAX_OPS * 3 if SYMPY_MAX_OPS > 0 else 0, SP.count_ops(candidate) + SP.count_ops(integrand)):
        return None
    try:
        symbol = sympy_locals().get(var, SP.symbols(var))
        derivative = SP.diff(candidate, symbol)
    except Exception:
        return None
    return sympy_simplify_difference(derivative, integrand)


def safe_eval_expr(expr, values=None):
    scope = {
        "sin": math.sin,
        "cos": math.cos,
        "tan": math.tan,
        "sec": lambda x: 1.0 / math.cos(x),
        "cosec": lambda x: 1.0 / math.sin(x),
        "cot": lambda x: 1.0 / math.tan(x),
        "exp": math.exp,
        "sqrt": math.sqrt,
        "log": _log_fn,
        "asin": math.asin,
        "acos": math.acos,
        "atan": math.atan,
        "abs": abs,
        "factorial": math.factorial,
        "pi": math.pi,
        "E_CONST": math.e,
    }
    if values:
        scope.update(values)
    return float(eval(to_python_expr(expr), {"__builtins__": {}}, scope))


def safe_eval_expr_angle(expr, values=None, degrees=False):
    if not degrees:
        return safe_eval_expr(expr, values)

    def deg_wrap(fn):
        return lambda x: fn(math.radians(x))

    scope = {
        "sin": deg_wrap(math.sin),
        "cos": deg_wrap(math.cos),
        "tan": deg_wrap(math.tan),
        "sec": lambda x: 1.0 / math.cos(math.radians(x)),
        "cosec": lambda x: 1.0 / math.sin(math.radians(x)),
        "cot": lambda x: 1.0 / math.tan(math.radians(x)),
        "exp": math.exp,
        "sqrt": math.sqrt,
        "log": _log_fn,
        "asin": math.asin,
        "acos": math.acos,
        "atan": math.atan,
        "abs": abs,
        "factorial": math.factorial,
        "pi": math.pi,
        "E_CONST": math.e,
    }
    if values:
        scope.update(values)
    return float(eval(to_python_expr(expr), {"__builtins__": {}}, scope))


def safe_eval_expr_complex(expr, values=None):
    scope = {
        "sin": cmath.sin,
        "cos": cmath.cos,
        "tan": cmath.tan,
        "sec": lambda x: 1.0 / cmath.cos(x),
        "cosec": lambda x: 1.0 / cmath.sin(x),
        "cot": lambda x: 1.0 / cmath.tan(x),
        "exp": cmath.exp,
        "sqrt": cmath.sqrt,
        "log": _clog_fn,
        "asin": cmath.asin,
        "acos": cmath.acos,
        "atan": cmath.atan,
        "abs": abs,
        "pi": math.pi,
        "E_CONST": math.e,
    }
    if values:
        scope.update(values)
    return eval(to_python_expr(expr), {"__builtins__": {}}, scope)


def numeric_close(a, b, rel_tol=1e-4, abs_tol=1e-5):
    return math.isfinite(a) and math.isfinite(b) and abs(a - b) <= max(abs_tol, rel_tol * max(1.0, abs(a), abs(b)))


def finite_difference(expr, var, value):
    h = 1e-6 * max(1.0, abs(value))
    try:
        hi = safe_eval_expr(expr, {var: value + h})
        lo = safe_eval_expr(expr, {var: value - h})
    except Exception:
        return None
    return (hi - lo) / (2.0 * h)


def stable_finite_difference(expr, var, value):
    scale = max(1.0, abs(value))
    estimates = []
    for factor in (1e-3, 3e-4, 1e-4, 3e-5, 1e-5):
        h = factor * scale
        try:
            hi = safe_eval_expr(expr, {var: value + h})
            lo = safe_eval_expr(expr, {var: value - h})
        except Exception:
            continue
        if not math.isfinite(hi) or not math.isfinite(lo):
            continue
        estimates.append((h, (hi - lo) / (2.0 * h)))
    if len(estimates) < 2:
        return None
    best = None
    best_gap = None
    i = 0
    while i < len(estimates) - 1:
        left = estimates[i][1]
        right = estimates[i + 1][1]
        if not math.isfinite(left) or not math.isfinite(right):
            i += 1
            continue
        gap = abs(left - right)
        if numeric_close(left, right, rel_tol=6e-2, abs_tol=6e-2):
            return right
        if best_gap is None or gap < best_gap:
            best_gap = gap
            best = right
        i += 1
    return best if best_gap is not None and best_gap < 1e-1 else None


def complex_step_derivative(expr, var, value):
    python_expr = to_python_expr(expr)
    if "abs(" in python_expr:
        return None
    h = 1e-20 * max(1.0, abs(value))
    try:
        result = safe_eval_expr_complex(expr, {var: value + 1j * h})
    except Exception:
        return None
    if not math.isfinite(result.real) or not math.isfinite(result.imag):
        return None
    return result.imag / h


def structurally_same_derivative(expr, candidate):
    try:
        _var, _steps, expected, _formatted = DERIVE.solve_normal_mode(expr)
        shown = DERIVE.parse(candidate)
    except Exception:
        return False

    def simplify_node(node):
        try:
            return DERIVE.sim(node)
        except Exception:
            return node

    def exp_pow_to_fn(node):
        kind = node[0]
        if kind in ("num", "sym", "const"):
            return node
        if kind == "pow" and node[1][0] == "const" and node[1][1] == "e":
            arg = exp_pow_to_fn(node[2])
            try:
                if DERIVE.tree_size(arg) <= 240:
                    arg = simplify_node(DERIVE.expand(arg))
            except Exception:
                pass
            return DERIVE.sim(("fn", "exp", arg))
        if kind == "fn":
            arg = exp_pow_to_fn(node[2])
            try:
                if DERIVE.tree_size(arg) <= 240:
                    arg = simplify_node(DERIVE.expand(arg))
            except Exception:
                pass
            return DERIVE.sim(("fn", node[1], arg))
        if kind == "pow":
            return DERIVE.sim(("pow", exp_pow_to_fn(node[1]), exp_pow_to_fn(node[2])))
        if kind == "div":
            return DERIVE.sim(("div", exp_pow_to_fn(node[1]), exp_pow_to_fn(node[2])))
        if kind == "mul":
            items = [exp_pow_to_fn(item) for item in node[1]]
            exp_args = []
            kept = []
            for item in items:
                if item[0] == "fn" and item[1] == "exp":
                    exp_args.append(item[2])
                else:
                    kept.append(item)
            if len(exp_args) > 1:
                kept.append(DERIVE.sim(("fn", "exp", DERIVE.sim(("add", tuple(exp_args))))))
                return DERIVE.sim(("mul", tuple(kept)))
            return DERIVE.sim(("mul", tuple(items)))
        if kind == "add":
            return DERIVE.sim(("add", tuple(exp_pow_to_fn(item) for item in node[1])))
        return node

    left = simplify_node(exp_pow_to_fn(shown))
    right = simplify_node(exp_pow_to_fn(expected))
    try:
        if DERIVE.same(left, right):
            return True
    except Exception:
        pass

    try:
        size = DERIVE.tree_size(left) + DERIVE.tree_size(right)
    except Exception:
        size = 999999
    if size <= 1400:
        try:
            left_expanded = simplify_node(DERIVE.expand(left))
            right_expanded = simplify_node(DERIVE.expand(right))
            if DERIVE.same(left_expanded, right_expanded):
                return True
        except Exception:
            pass

    try:
        cross_left = simplify_node(DERIVE.expand(DERIVE.mul([left[1], right[2]]))) if left[0] == "div" and right[0] == "div" else None
        cross_right = simplify_node(DERIVE.expand(DERIVE.mul([right[1], left[2]]))) if left[0] == "div" and right[0] == "div" else None
        if cross_left is not None and cross_right is not None and DERIVE.same(cross_left, cross_right):
            return True
    except Exception:
        pass

    return False


def _expand_parametric_solution(rhs, var_letter="n", lower=None, upper=None,
                                samples=(0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5,
                                         6, -6, 7, -7, 8, -8, 9, -9, 10, -10)):
    cleaned = re.sub(r"\s*\([^)]*\)\s*$", "", rhs).strip()
    if var_letter not in cleaned:
        return None
    try:
        out = []
        bounded = lower is not None and upper is not None
        if bounded:
            lo, hi = (lower, upper) if lower <= upper else (upper, lower)
            tol = max(1e-7, 1e-7 * max(1.0, abs(lo), abs(hi)))
            v0 = safe_eval_expr(cleaned, {var_letter: 0.0})
            v1 = safe_eval_expr(cleaned, {var_letter: 1.0})
            v2 = safe_eval_expr(cleaned, {var_letter: 2.0})
            step = v1 - v0
            if abs((v2 - v1) - step) <= 1e-7 * max(1.0, abs(step)):
                if abs(step) <= 1e-12:
                    n_values = (0,)
                else:
                    a = int(math.floor((lo - v0) / step)) if step > 0 else int(math.floor((hi - v0) / step))
                    b = int(math.ceil((hi - v0) / step)) if step > 0 else int(math.ceil((lo - v0) / step))
                    n_min, n_max = (a, b) if a <= b else (b, a)
                    n_min -= 2
                    n_max += 2
                    if n_max - n_min > 200:
                        n_min = max(n_min, -100)
                        n_max = min(n_max, 100)
                    n_values = range(n_min, n_max + 1)
            else:
                n_values = samples
        else:
            n_values = samples
        for n_val in n_values:
            value = safe_eval_expr(cleaned, {var_letter: float(n_val)})
            if bounded:
                if value < lo - tol or value > hi + tol:
                    continue
            out.append(value)
        if bounded:
            uniq = []
            for v in out:
                if not any(abs(v - u) <= 1e-6 * max(1.0, abs(u)) for u in uniq):
                    uniq.append(v)
            return uniq
        return out
    except Exception:
        return None


def extract_last_solution_values(output, var="x", lower=None, upper=None):
    lines = (output or "").splitlines()
    prefixes = (f"{var} = [", f"Combined solutions: {var} = [")
    for line in reversed(lines):
        stripped = re.sub(r"^\s*\d+\.\s*", "", line.strip())
        if stripped.startswith("Answer: "):
            stripped = stripped[8:].strip()
        for prefix in prefixes:
            if stripped.startswith(prefix) and stripped.endswith("]"):
                inner = stripped[stripped.find("[") + 1 : -1]
                bits = [bit for bit in split_top_level_text(inner, ",") if bit]
                values = []
                for bit in bits:
                    try:
                        values.append(safe_eval_expr(bit))
                    except Exception:
                        return None
                return values
        if stripped.startswith(f"{var} = "):
            rhs = stripped.split("=", 1)[1].strip()
            try:
                return [safe_eval_expr(rhs)]
            except Exception:
                expanded = _expand_parametric_solution(rhs, lower=lower, upper=upper)
                if expanded is not None:
                    return expanded
                return None
    return None


def infer_algebra_solve_var_from_eq_line(eq_text):
    if not eq_text or "=" not in eq_text:
        return "x"
    try:
        left, right = eq_text.split("=", 1)
        node = ALG.parse(left.strip() + "-(" + right.strip() + ")")
        v = ALG.choose_primary_var(node)
        return v if v is not None else "x"
    except Exception:
        return "x"


def extract_last_derivative_expr(output):
    for line in reversed((output or "").splitlines()):
        stripped = re.sub(r"^\s*\d+\.\s*", "", line.strip())
        if stripped.lower().startswith("answer:"):
            stripped = stripped.split(":", 1)[1].strip()
        if stripped.startswith("dy/d") and "=" in stripped:
            return stripped.rsplit("=", 1)[1].strip()
    return None


def extract_last_antiderivative_expr(output):
    for line in reversed((output or "").splitlines()):
        stripped = re.sub(r"^\s*\d+\.\s*", "", line.strip())
        if stripped.lower().startswith("answer:"):
            rhs = stripped.split(":", 1)[1].strip()
            if rhs.endswith("+ C"):
                return rhs[:-4].strip()
            continue
        if stripped.endswith("+ C"):
            return stripped[:-4].strip()
        if stripped.startswith("="):
            rhs = stripped[1:].strip()
        elif "=" in stripped:
            rhs = stripped.split("=", 1)[1].strip()
        else:
            continue
        if rhs.endswith("+ C"):
            return rhs[:-4].strip()
    return None


def _strip_numbered_prefix(line):
    return re.sub(r"^\s*\d+\.\s*", "", (line or "").strip())


def extract_labeled_expr(output, labels):
    wanted = tuple(label.lower() for label in labels)
    for line in reversed((output or "").splitlines()):
        stripped = _strip_numbered_prefix(line)
        lower = stripped.lower()
        for label in wanted:
            if lower.startswith(label):
                return stripped[len(label):].strip()
    return None


def extract_reverse_chain_data(output):
    u_expr = None
    du_expr = None
    primitive_fn = None
    for line in (output or "").splitlines():
        stripped = _strip_numbered_prefix(line)
        if stripped.startswith("u = "):
            u_expr = stripped.split("=", 1)[1].strip().rstrip(".")
        elif stripped.startswith("du/dx = "):
            du_expr = stripped.split("=", 1)[1].strip().rstrip(".")
        elif "I = Int[" in stripped and "(u)] du" in stripped:
            match = re.search(r"I = Int\[(sin|cos|tan|sec|cosec|cot|exp)\(u\)\] du", stripped)
            if match:
                primitive_fn = match.group(1)
    if u_expr and du_expr and primitive_fn:
        return u_expr, du_expr, primitive_fn
    return None


def reverse_chain_integrand_matches(output, integrand, var="x"):
    data = extract_reverse_chain_data(output)
    if data is None:
        return False
    u_expr, du_expr, primitive_fn = data
    reconstructed = f"({du_expr})*{primitive_fn}({u_expr})"
    return expressions_equivalent_numeric(reconstructed, integrand, var=var, min_good=4, rel_tol=2e-4, abs_tol=2e-4)


def extract_final_algebra_expr(output):
    expr = extract_labeled_expr(output, ("final =", "out =", "ans =", "answer:", "f(g(x)) =", "hence "))
    if expr:
        return expr
    for line in reversed((output or "").splitlines()):
        stripped = _strip_numbered_prefix(line)
        if stripped.startswith("="):
            return stripped[1:].strip()
    return None


def expressions_equivalent_numeric(left, right, var="x", min_good=6, rel_tol=1e-5, abs_tol=1e-5):
    if (left or "").replace(" ", "") == (right or "").replace(" ", ""):
        return True
    if structural_reciprocal_trig_equivalent(left, right):
        return True
    symbolic = sympy_expressions_equivalent(left, right, var=var)
    if symbolic is True:
        return True
    if symbolic is False:
        # Keep numeric sampling as a safety net for expressions SymPy cannot
        # prove under implicit domain assumptions.
        pass
    good = 0
    bad = 0
    for point in domain_aware_sample_points(left, right, var=var):
        try:
            left_value = safe_eval_expr(left, {var: point})
            right_value = safe_eval_expr(right, {var: point})
        except Exception:
            continue
        if not math.isfinite(left_value) or not math.isfinite(right_value):
            continue
        if numeric_close(left_value, right_value, rel_tol=rel_tol, abs_tol=abs_tol):
            good += 1
        else:
            bad += 1
    return good >= min_good and bad == 0


def expression_value_checker(source_expr, candidate_expr, var="x"):
    try:
        return expressions_equivalent_numeric(source_expr, candidate_expr, var=var)
    except Exception:
        return False


def structural_reciprocal_trig_equivalent(left, right):
    reciprocal_pairs = {
        "sin": "cosec",
        "cos": "sec",
        "tan": "cot",
    }

    def parse_alg(text):
        try:
            return ALG.parse(text)
        except Exception:
            return None

    def match_pair(a, b):
        if a is None or b is None:
            return False
        a = ALG.sim(a)
        b = ALG.sim(b)
        if a[0] == "div" and a[1] == ("num", 1, 1) and a[2][0] == "fn" and b[0] == "fn":
            expected = reciprocal_pairs.get(a[2][1])
            return expected == b[1] and ALG.same(a[2][2], b[2])
        return False

    a = parse_alg(left)
    b = parse_alg(right)
    if a is not None and b is not None and ALG.same(ALG.sim(a), ALG.sim(b)):
        return True
    return match_pair(a, b) or match_pair(b, a)


def equation_residual(eq_text, var, value):
    lhs, rhs = split_equation_text(eq_text)
    left_value = safe_eval_expr(lhs, {var: value})
    right_value = safe_eval_expr(rhs, {var: value})
    return left_value - right_value


def equation_residual_angle(eq_text, var, value, degrees=False):
    lhs, rhs = split_equation_text(eq_text)
    left_value = safe_eval_expr_angle(lhs, {var: value}, degrees=degrees)
    right_value = safe_eval_expr_angle(rhs, {var: value}, degrees=degrees)
    return left_value - right_value


def numeric_value_sets_match(shown, expected, rel_tol=2e-4, abs_tol=2e-4):
    if expected is None:
        return None
    if len(shown) != len(expected):
        return False
    unmatched = list(expected)
    for value in shown:
        match_index = None
        for index, target in enumerate(unmatched):
            if numeric_close(value, target, rel_tol=rel_tol, abs_tol=abs_tol):
                match_index = index
                break
        if match_index is None:
            return False
        unmatched.pop(match_index)
    return not unmatched


def sympy_equation_real_values(eq_text, var="x"):
    if not SYMPY_AVAILABLE or not sympy_safe_text(eq_text):
        return None
    try:
        lhs, rhs = split_equation_text(eq_text)
    except Exception:
        return None
    left = sympy_parse_expr(lhs)
    right = sympy_parse_expr(rhs)
    if left is None or right is None:
        return None
    expr = None
    try:
        expr = SP.together(left - right)
        if limited_by(SYMPY_MAX_OPS, SP.count_ops(expr)):
            return None
        symbol = sympy_locals().get(var, SP.symbols(var))
        roots = SP.solve(expr, symbol)
    except Exception:
        roots = None
    if roots is None:
        if expr is None:
            return None
        try:
            numer = SP.together(expr).as_numer_denom()[0]
            poly = SP.Poly(numer, symbol)
            if poly.degree() < 0 or poly.degree() > 20:
                return None
            roots = poly.nroots(n=30, maxsteps=100)
        except Exception:
            return None
    elif roots == []:
        try:
            numer = SP.together(expr).as_numer_denom()[0]
            poly = SP.Poly(numer, symbol)
            if 0 <= poly.degree() <= 20:
                roots = poly.nroots(n=30, maxsteps=100)
        except Exception:
            pass
    values = []
    for root in roots:
        try:
            root = SP.N(root, 30)
            if abs(float(SP.im(root))) > 1e-9:
                continue
            value = float(SP.re(root))
            if not math.isfinite(value):
                continue
            if abs(equation_residual(eq_text, var, value)) > 1e-4:
                continue
            if not any(numeric_close(value, seen, rel_tol=1e-6, abs_tol=1e-6) for seen in values):
                values.append(value)
        except Exception:
            continue
    return sorted(values)


def _bisect_residual(eq_text, var, left, right, degrees=False, iterations=50):
    try:
        f_left = equation_residual_angle(eq_text, var, left, degrees=degrees)
        f_right = equation_residual_angle(eq_text, var, right, degrees=degrees)
    except Exception:
        return None
    if not (math.isfinite(f_left) and math.isfinite(f_right)):
        return None
    if abs(f_left) < 1e-8:
        return left
    if abs(f_right) < 1e-8:
        return right
    if f_left * f_right > 0:
        return None
    lo, hi = left, right
    for _ in range(iterations):
        mid = (lo + hi) / 2
        try:
            f_mid = equation_residual_angle(eq_text, var, mid, degrees=degrees)
        except Exception:
            return None
        if not math.isfinite(f_mid):
            return None
        if abs(f_mid) < 1e-10:
            return mid
        if f_left * f_mid <= 0:
            hi = mid
            f_right = f_mid
        else:
            lo = mid
            f_left = f_mid
    return (lo + hi) / 2


def expected_trig_solutions_numeric(eq_text, var="x", lower=0.0, upper=2 * math.pi, degrees=False):
    if upper <= lower:
        return None
    span = upper - lower
    # This oracle intentionally oversamples: generated solve cases can contain
    # quadratic angle brackets, so roots may be much closer together near the
    # end of the interval than a normal A-level example would be.
    steps = min(40000, max(4000, int(span * (25 if degrees else 1600))))
    step = span / steps
    candidates = []

    def add_candidate(value):
        if value < lower - 1e-7 or value > upper + 1e-7:
            return
        try:
            residual = equation_residual_angle(eq_text, var, value, degrees=degrees)
        except Exception:
            return
        if math.isfinite(residual) and abs(residual) < 1e-5:
            if not any(numeric_close(value, seen, rel_tol=1e-5, abs_tol=1e-5) for seen in candidates):
                candidates.append(value)

    prev_x = lower
    try:
        prev_y = equation_residual_angle(eq_text, var, prev_x, degrees=degrees)
    except Exception:
        prev_y = None
    add_candidate(prev_x)
    for index in range(1, steps + 1):
        x_value = lower + index * step
        try:
            y_value = equation_residual_angle(eq_text, var, x_value, degrees=degrees)
        except Exception:
            prev_x = x_value
            prev_y = None
            continue
        if math.isfinite(y_value):
            if abs(y_value) < 1e-5:
                add_candidate(x_value)
            if prev_y is not None and math.isfinite(prev_y) and prev_y * y_value < 0:
                root = _bisect_residual(eq_text, var, prev_x, x_value, degrees=degrees)
                if root is not None:
                    add_candidate(root)
        prev_x = x_value
        prev_y = y_value if math.isfinite(y_value) else None
    add_candidate(upper)
    candidates = sorted(candidates)
    if not candidates:
        return candidates
    cluster_tol = max(step * 8, 0.05 if degrees else 1e-2)
    clustered = []
    current = [candidates[0]]
    for value in candidates[1:]:
        if abs(value - current[-1]) <= cluster_tol:
            current.append(value)
        else:
            clustered.append(current)
            current = [value]
    clustered.append(current)
    out = []
    for cluster in clustered:
        best = cluster[0]
        best_residual = None
        for value in cluster:
            try:
                residual = abs(equation_residual_angle(eq_text, var, value, degrees=degrees))
            except Exception:
                residual = None
            if residual is not None and (best_residual is None or residual < best_residual):
                best = value
                best_residual = residual
        out.append(best)
    return out


def candidate_sample_points():
    return (0.0, 0.05, -0.05, 0.1, -0.1, 0.2, -0.2, 0.35, -0.35, 0.6, -0.6, 1.0)


def domain_aware_sample_points(*exprs, var="x"):
    base = [
        0.0, 0.05, -0.05, 0.1, -0.1, 0.2, -0.2, 0.35, -0.35, 0.6, -0.6, 1.0,
        -1.0, 1.5, -1.5, 2.0, -2.0, 2.5, -2.5, 3.0, -3.0, 3.5, -3.5,
        4.0, -4.0, 5.0, -5.0, 6.0, -6.0, 8.0, -8.0, 10.0, -10.0,
    ]
    out = []
    seen = set()
    for point in base:
        key = round(point, 10)
        if key in seen:
            continue
        usable = True
        for expr in exprs:
            try:
                value = safe_eval_expr(expr, {var: point})
            except Exception:
                usable = False
                break
            if not math.isfinite(value):
                usable = False
                break
        if usable:
            out.append(point)
            seen.add(key)
    return tuple(out)


def expression_needs_positive_real_base(expr, var="x"):
    text = to_python_expr(expr or "")
    compact = re.sub(r"\s+", "", text)
    v = re.escape(var)
    return bool(
        re.search(rf"(^|[^A-Za-z0-9_]){v}\*\*", compact)
        or re.search(rf"\*\*[^,\n]*{v}", compact)
    )


def positive_sample_points():
    return (0.05, 0.1, 0.2, 0.35, 0.6, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0)


def normalized_text(text):
    return " ".join((text or "").lower().split())


def numbered_step_count(text):
    # Historical tests counted "1. ..." lines. The calculator now deliberately
    # emits exam-copyable maths lines without numbering, so count any real
    # working line as a step for coverage/oracle purposes.
    count = 0
    for line in (text or "").splitlines():
        s = line.strip().lower()
        if not s:
            continue
        if re.match(r"^\s*\d+\.", s) or re.search(r":\s*\d+\.", s):
            count += 1
            continue
        if any(tok in s for tok in ("=", "int", "du", "dx/dt", "dy/dt", "dy/dx", "u=", "d:", "i:", "signs:", "domain:", "range:")):
            count += 1
    return count


def nonempty_line_count(text):
    return sum(1 for line in (text or "").splitlines() if line.strip())


_DENSE_LOW_PRECEDENCE_OP = re.compile(r"(?<![eE])(?<=[A-Za-z0-9)\]])[+-](?=[A-Za-z0-9(])")
_MATH_RESULT_MARKERS = (
    " = ",
    "dy/dx",
    "dx/dt",
    "dy/dt",
    "out =",
    "ans =",
    "final =",
    "answer:",
    "equation:",
    "substitute:",
    "rearranged equation:",
)
_READABILITY_SKIP_MARKERS = (
    "write the equation in the form",
    "let a*sin",
    "u^",
    "d/dx(lhs)=d/dx(rhs)",
    "dx/dt=dx/dt",
    "cos a + cos b",
)


def output_readability_issues(output):
    issues = []
    for raw_line in (output or "").splitlines():
        line = raw_line.strip()
        if not line:
            continue
        lower = line.lower()
        content_lower = re.sub(r"^(?:[a-z]:\s*)*\d+\.\s*", "", lower)
        if content_lower.startswith("use ") or content_lower.startswith("use:"):
            continue
        if content_lower.startswith("start with"):
            continue
        if content_lower.startswith("divide:"):
            continue
        if line.count("  ") >= 2:
            continue
        if any(marker in content_lower for marker in _READABILITY_SKIP_MARKERS):
            continue
        if not any(marker in lower for marker in _MATH_RESULT_MARKERS):
            continue

        matches = _DENSE_LOW_PRECEDENCE_OP.findall(line)
        if len(matches) >= 4:
            issues.append(f"dense low-precedence operators: {line}")
        if "'" not in line and re.search(r"/\s*[A-Za-z_][A-Za-z0-9_^*]*[+-]", line):
            issues.append(f"ungrouped compound denominator: {line}")
        if "..." in line or ".." in line:
            continue
        if line.count("(") != line.count(")"):
            issues.append(f"unbalanced brackets: {line}")
    return issues


def output_readability_ok(output):
    return not output_readability_issues(output)


def readable_output_checker(base_checker=None, *, contains_all=(), contains_any=()):
    quality = base_checker or build_checker(contains_all=contains_all, contains_any=contains_any)

    def check(output):
        return bool(quality(output)) and output_readability_ok(output)

    return check


def build_checker(*, contains_all=(), contains_any=(), min_steps=0, min_lines=0, forbid=()):
    required_all = tuple(item.lower() for item in contains_all if item)
    required_any = tuple(item.lower() for item in contains_any if item)
    forbidden = tuple(item.lower() for item in (_DEFAULT_FORBIDDEN_SNIPPETS + tuple(forbid)))

    def check(output):
        compact = normalized_text(output)
        if any(item in compact for item in forbidden):
            return False
        if min_steps and numbered_step_count(output) < min_steps:
            return False
        if min_lines and nonempty_line_count(output) < min_lines:
            return False
        if required_all and not all(item in compact for item in required_all):
            return False
        if required_any and not any(item in compact for item in required_any):
            return False
        return True

    return check


def algebra_compare_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("equivalent",),
        contains_any=("e1", "lhs", "source-target", "equivalent"),
        min_steps=0,
        min_lines=3,
    )


def algebra_transform_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("source-target", "target", "final", "=", "already in target form"),
        min_steps=0,
        min_lines=2,
    )


def algebra_solve_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("= 0", "x =", "[", "factor", "domain"),
        min_steps=2,
        min_lines=2,
    )


def algebra_inverse_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("y =",),
        contains_any=("f^-1", "no inverse on all real x", "not invertible"),
        min_steps=1,
        min_lines=3,
    )


def algebra_no_inverse_checker():
    return build_checker(
        contains_all=("y =",),
        contains_any=("no inverse on all real x", "not invertible"),
        min_steps=0,
        min_lines=2,
    )


def algebra_comp_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("f(x) =", "g(x) =", "f(g(x))"),
        min_steps=4,
        min_lines=3,
    )


def algebra_rewrite_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("u =", "source-target", "=", "already written in terms of"),
        min_steps=2,
        min_lines=3,
    )


def algebra_expand_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("^", "*", "+"),
        min_steps=0,
        min_lines=3,
    )


def algebra_complete_square_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("h =", "k =", "("),
        min_steps=0,
        min_lines=4,
    )


def trig_prove_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("= rhs",),
        contains_any=("use ", "= rhs", "= lhs", "lhs = rhs", "hence", "calculate"),
        min_steps=3,
        min_lines=3,
    )


def trig_transform_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("use ", "hence", "= lhs", "answer:", "source = target", "target ="),
        min_steps=3,
        min_lines=4,
    )


def trig_solve_checker(*tokens):
    base_required = tokens
    structure = build_checker(
        contains_any=("start with", "solve trig eq", "standard trig equation", "product = 0", "factor ", "use cos", "let a=", "let u=", "u=", "move all terms", "base angles", "alpha =", "sin(a)", "cos(a)", "tan(a)", "tan(", "0 <=", "keep values", "cos a + cos b", "2cos(", "x = ["),
        min_steps=3,
        min_lines=3,
    )

    def check(out):
        text = normalized_text(out)
        if any(item in text for item in _DEFAULT_FORBIDDEN_SNIPPETS):
            return False
        if not structure(out):
            return False
        no_solution_branch = (
            "no real solutions" in text
            or "no valid trig values" in text
            or "no trig values" in text
            or "no solution" in text
            or "no sol" in text
        )
        if no_solution_branch:
            return True
        for token in base_required + ("x =",):
            if token.lower() not in text:
                return False
        return True

    return check


def trig_rewrite_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("=", "source", "target", "err:"),
        min_steps=0,
        min_lines=1,
    )


def algebra_compare_output_checker(expr1, expr2, expected_equal=True):
    quality = algebra_compare_checker()

    def algebra_program_equivalent(left, right):
        try:
            return ALG.equivalent(ALG.parse(left), ALG.parse(right))
        except Exception:
            return None

    def check(output):
        if not quality(output):
            return False
        text = normalized_text(output)
        says_equivalent = "equivalent" in text and "not equivalent" not in text
        says_not_equivalent = "not equivalent" in text
        if expected_equal and not says_equivalent:
            return False
        if not expected_equal and not says_not_equivalent:
            return False
        actual_equal = algebra_program_equivalent(expr1, expr2)
        if actual_equal is None:
            actual_equal = expressions_equivalent_numeric(expr1, expr2)
        return actual_equal == expected_equal

    return check


def _has_non_exam_quality_output(output):
    text = normalized_text(output)
    return any(item in text for item in _NON_EXAM_QUALITY_PATTERNS)


def final_expression_output_checker(source_expr, labels=("final =", "out =", "ans ="), var="x"):
    def check(output):
        if any(item in normalized_text(output) for item in _DEFAULT_FORBIDDEN_SNIPPETS):
            return False
        output = normalized_text(output)
        if _has_non_exam_quality_output(output):
            return False
        candidate = extract_labeled_expr(output, labels)
        if not candidate:
            candidate = extract_final_algebra_expr(output)
        if not candidate:
            return False
        return expression_value_checker(source_expr, candidate, var=var)

    return check


def transform_output_checker(source_expr, target_expr, program="Algebra", var="x"):
    quality = algebra_transform_checker() if program == "Algebra" else trig_transform_checker()

    def program_equivalent(left, right):
        try:
            engine = ALG if program == "Algebra" else TRIG
            return engine.equivalent(engine.parse(left), engine.parse(right))
        except Exception:
            return None

    def check(output):
        if not quality(output):
            return False
        candidate = extract_labeled_expr(output, ("answer:", "final ="))
        if not candidate:
            candidate = extract_final_algebra_expr(output)
        if not candidate:
            for raw in reversed((output or "").splitlines()):
                s = raw.strip()
                if s and not s.lower().startswith(("source", "target", "lhs", "rhs")):
                    candidate = s
                    break
        if not candidate:
            return False
        candidate_matches_target = program_equivalent(candidate, target_expr)
        candidate_matches_source = program_equivalent(candidate, source_expr)
        if candidate_matches_target is True and candidate_matches_source is True:
            return True
        return (
            expression_value_checker(candidate, target_expr, var=var)
            and expression_value_checker(candidate, source_expr, var=var)
        )

    return check


def trig_identity_output_checker(eq_text):
    lhs, rhs = split_equation_text(eq_text)
    quality = trig_prove_checker()

    def trig_program_equivalent(left, right):
        try:
            return TRIG.equivalent(TRIG.parse(left), TRIG.parse(right))
        except Exception:
            return None

    def check(output):
        if not quality(output):
            return False
        actual_equal = trig_program_equivalent(lhs, rhs)
        if actual_equal is None:
            actual_equal = expressions_equivalent_numeric(lhs, rhs, rel_tol=2e-5, abs_tol=2e-5)
        return actual_equal

    return check


def derive_checker(*tokens):
    quality = build_checker(
        contains_all=tokens + ("dy/dx",),
        contains_any=(
            "chain rule", "product rule", "quotient rule", "log diff", "logdiff", "logarithmic differentiation",
            "implicit", "term by term", " rule",
            "u =", "du/dx", "f1 =", "f1'", "ln(y) =", "y' =", "dy/dx =",
        ),
        min_steps=1,
        min_lines=2,
    )

    def check(output):
        text = normalized_text(output)
        return quality(output)

    return check


def derive_implicit_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("dy/dx",),
        contains_any=("d/dx(lhs)=d/dx(rhs)", "make dy/dx", "differentiate both sides", "f_x", "f_y", "-f_x/f_y"),
        min_steps=3,
        min_lines=5,
    )


def derive_param_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("dx/dt", "dy/dt", "dy/dx ="),
        min_steps=0,
        min_lines=3,
    )


def integrate_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("+ c",),
        contains_any=(
            "method:",
            "met:",
            "integrate each term",
            "constant rule",
            "u =",
            "use the standard result",
            "standard integral",
            "power-reduction",
            "consider y",
            "dy/dx",
            "integration by parts",
            "integrate by parts",
            "let u=",
            "du=",
            "resulting polynomial",
            "odd sine power",
            "use ",
            "use parts",
            "by parts",
            "use:",
            "split the numerator",
            "polynomial division",
            "cancel common factor",
            "integrand simplifies",
            "complete the square",
            "partial fractions",
            "integral du/u",
            "int(du/u)",
            "n/d=q+r/d",
            "n/d = q + r/d",
        ),
        min_steps=0,
        min_lines=2,
    )


def working_quality_ok(output, program, feature):
    text = normalized_text(output)
    if any(item in text for item in _DEFAULT_FORBIDDEN_SNIPPETS):
        return False
    if _has_non_exam_quality_output(text):
        return False

    steps = numbered_step_count(output)
    lines = nonempty_line_count(output)
    feature = feature or ""

    if program == "Algebra":
        if lines < 3:
            return False
        if feature.startswith("algebra_solve") or feature.startswith("algebra_hard_solve"):
            if "no sol" in text or "no solution" in text or "no real solution" in text:
                return "expr =" in text and ("solve" in text or steps >= 2)
            return "expr =" in text and "x =" in text and ("solve" in text or steps >= 2)
        if feature.startswith("algebra_inverse"):
            return "f(x)" in text and "y =" in text and ("f^-1" in text or "no inverse" in text)
        if feature.startswith("algebra_transform"):
            return ("answer:" in text or "final =" in text) and ("use " in text or "rewrite" in text or "match coefficients" in text or "factor out" in text or "already in target form" in text or "simplify source" in text)
        if feature.startswith("algebra_rewrite"):
            return ("answer:" in text or "final =" in text) and ("write in terms of" in text or "already written in terms of" in text)
        if feature.startswith("algebra_complete_square"):
            return "complete square" in text and ("answer:" in text or "ans =" in text)
        return steps >= 2 or "result:" in text or "out =" in text

    if program == "Trigonometry":
        if lines < 4:
            return False
        if feature.startswith("trig_solve"):
            return "start with" in text and "x =" in text and (
                "solve trig eq" in text
                or "standard trig equation" in text
                or "for sin" in text
                or "for cos" in text
                or "for tan" in text
            )
        if feature.startswith("trig_prove"):
            return "lhs = rhs" in text and ("use " in text or "hence" in text)
        if feature.startswith("trig_transform"):
            return ("use " in text or "hence" in text) and ("answer:" in text or "final =" in text or "=" in output)
        if feature.startswith("trig_rewrite"):
            return ("answer:" in text or "final =" in text) and ("use " in text or "write using" in text)
        return steps >= 3

    if program == "Derive":
        if "dy/dx" not in text or lines < 3:
            return False
        if feature.startswith("derive_normal:quotient"):
            return "quotient rule" in text and "u =" in text and "v =" in text and ("du" in text or "dv" in text)
        if feature.startswith("derive_implicit"):
            return "dy/dx" in text and ("d/dx(lhs)=d/dx(rhs)" in text or "make dy/dx" in text)
        if feature.startswith("derive_parametric"):
            return "dx/dt" in text and "dy/dt" in text and "dy/dx =" in text and steps >= 4
        return any(marker in text for marker in ("chain rule", "product rule", "quotient rule", "log diff", "logdiff", "logarithmic differentiation", "implicit", "term by term", "dy/dx ="))

    if program == "Integrate":
        if "+ c" not in text or lines < 2:
            return False
        if feature.startswith("integrate_sub"):
            return "u =" in text or "substitution" in text
        if feature.startswith("integrate_parts"):
            return "integration by parts" in text or "i = u*v" in text
        if feature.startswith("integrate_pf"):
            return "partial fractions" in text or "decompose" in text
        if feature.startswith("integrate_div"):
            return "divide" in text or "polynomial division" in text or "cancel common factor" in text or "integrand simplifies" in text
        if feature.startswith("integrate_trig"):
            return "identity" in text or "use " in text or "reverse chain rule" in text or "u =" in text
        return any(marker in text for marker in ("method:", "use the standard result", "split the numerator", "complete the square", "u ="))

    if program == "SUVAT":
        if feature.startswith("suvat_expected_error"):
            return "no solution" in text or "infinite solutions" in text
        synthetic_working = "answer:" in text and ("equation:" in text or "original equation:" in text) and ("substitute:" in text or "rearranged equation:" in text)
        cli_working = any(marker in text for marker in ("s =", "u =", "v =", "a =", "t =")) and any(marker in text for marker in ("= s =", "= u =", "= v =", "= a =", "= t =", "v = u + at", "s = ut", "v^2 ="))
        return synthetic_working or cli_working

    return True


_EXAM_REASONING_MARKERS = tuple(
    dict.fromkeys(
        tuple(marker.lower() for marker in SHARED_REASONING_MARKERS)
        + (
            "start with",
            "coefficient of",
            "half the",
            "complete square",
            "input =",
            "step",
            "rewrite",
            "method",
            "move terms",
            "collect terms",
            "square:",
            "write in terms",
            "already written",
            "rewrite to target",
            "m ",
            "n/p",
            "expression:",
            "check",
            "verify",
            "simplif",
            "derive ",
            "d/dx",
            "dy/dx",
            "du/dx",
            "dv/dx",
            "u'",
            "v'",
            "by parts",
            "by substitution",
            "by the standard",
            "reverse chain",
            "power-reduce",
            "angle ",
            "evaluate",
            "apply ",
            "split ",
            "separate",
            "partial fraction",
            "decompos",
            "simplest form",
            "collect like",
            "multiply both",
            "divide both",
            "common denominator",
            "power rule",
            "cancel common factor",
        )
    )
)


def exam_working_quality_issues(output, program, feature):
    issues = []
    if output is None:
        return issues
    text = normalized_text(output)
    if not text:
        return issues
    feature = feature or ""

    if any(item in text for item in _DEFAULT_FORBIDDEN_SNIPPETS):
        issues.append("contains an error/unsupported marker")
    if _has_non_exam_quality_output(text):
        issues.append("uses non-exam-quality numerical methods (scan/compute/bisection)")
    
    readability = output_readability_issues(output)
    issues.extend(f"readability: {issue}" for issue in readability)

    lines = nonempty_line_count(output)
    steps = numbered_step_count(output)
    
    return issues


def exam_working_quality_ok(output, program, feature):
    return not exam_working_quality_issues(output, program, feature)


def format_suvat_working(result, equation, original_eq, substitution):
    lines = []
    if original_eq:
        lines.append(f"Original equation: {original_eq}")
    if equation and str(equation) != str(original_eq):
        lines.append(f"Rearranged equation: {equation}")
    elif equation:
        lines.append(f"Equation: {equation}")
    if substitution:
        lines.append(f"Substitute: {substitution}")
    if result is not None:
        lines.append(f"Answer: {SUVAT.show(result)}")
    return "\n".join(lines)


def suvat_checker(*tokens):
    return build_checker(
        contains_all=tokens + ("equation:", "substitute:", "answer:"),
        min_lines=3,
        forbid=("error",),
    )


def stats_checker(*tokens):
    return build_checker(
        contains_all=tokens,
        contains_any=("mean", "r =", "p(", "standardise", "z =", "spark"),
        min_lines=3,
    )


def stats_normal_output_checker(input_line):
    def check(output):
        try:
            parts = split_top_level_text(input_line, ",")
            mu, sigma, lo, hi = [safe_eval_expr(p) for p in parts[:4]]
            if sigma <= 0:
                return False
            expected = 0.5 * (1.0 + math.erf((hi - mu) / (sigma * math.sqrt(2.0)))) - 0.5 * (1.0 + math.erf((lo - mu) / (sigma * math.sqrt(2.0))))
            text = normalized_text(output)
            m = re.search(r"answer:\s*([-+]?\d+(?:\.\d+)?(?:e[-+]?\d+)?)", text)
            value = float(m.group(1)) if m else None
            if value is None:
                for raw in reversed((output or "").splitlines()):
                    raw = raw.strip()
                    if re.fullmatch(r"[-+]?\d+(?:\.\d+)?(?:e[-+]?\d+)?", raw, flags=re.I):
                        value = float(raw)
                        break
                    nums = re.findall(r"[-+]?\d+(?:\.\d+)?(?:e[-+]?\d+)?", raw, flags=re.I)
                    if nums:
                        value = float(nums[-1])
                        break
            return value is not None and abs(value - expected) < 5e-5 and ("z1 =" in text or "standardise" in text)
        except Exception:
            return False
    check.__name__ = "normal probability oracle"
    return check


def suvat_expected_float(expected):
    if isinstance(expected, Fraction):
        return expected.numerator / expected.denominator
    if isinstance(expected, tuple) and len(expected) == 3 and expected[0] == "num":
        return expected[1] / expected[2]
    return safe_eval_expr(str(expected))


def parse_suvat_answer_values(text):
    values = []
    for bit in split_top_level_text(text, ","):
        for part in re.split(r"\s+or\s+", bit):
            part = part.strip()
            if not part:
                continue
            part = part.split("(", 1)[0].strip()
            part = re.sub(r"\s+(m/s\^2|m/s|m|s)$", "", part).strip()
            try:
                values.append(safe_eval_expr(part))
            except Exception:
                pass
    return values


def suvat_cli_checker(target, expected):
    expected_value = suvat_expected_float(expected)

    def check(output):
        if "error:" in normalized_text(output):
            return False
        found = False
        for line in (output or "").splitlines():
            stripped = line.strip()
            if not re.match(rf"^{re.escape(target)}\s*=", stripped):
                continue
            rhs = stripped.split("=", 1)[1].strip()
            values = parse_suvat_answer_values(rhs)
            if not values:
                continue
            found = True
            for value in values:
                if not numeric_close(value, expected_value, rel_tol=5e-3, abs_tol=5e-4):
                    return False
        return found

    check.__name__ = f"suvat_cli_checker_{target}_{expected}"
    return check


def suvat_symbolic_output_checker(target, *tokens):
    def check(output):
        text = normalized_text(output)
        compact = text.replace(" ", "")
        if "error:" in text:
            return False
        if f"{target}=" not in compact:
            return False
        return all(token.replace(" ", "") in compact for token in tokens)

    check.__name__ = f"suvat_symbolic_output_checker_{target}"
    return check


def suvat_expected_error_checker(*tokens):
    def check(output):
        text = normalized_text(output)
        if "error:" not in text and "no solution" not in text and "infinite solutions" not in text and "any real" not in text:
            return False
        return all(token.lower() in text for token in tokens)

    check.__name__ = "suvat_expected_error_checker"
    return check

class CASIOApp(App):
    auto_exit = False
    plain_mode = False
    CSS = """
    App {
        background: #242934;
    }

    Screen {
        background: #242934;
        color: #e5e7eb;
    }

    #shell {
        width: 100%;
        height: 100%;
        padding: 0 1;
        background: #242934;
    }

    #chrome {
        height: 1;
        padding: 0 0 0 1;
        color: #a8adb8;
        background: #242934;
    }

    #chrome-title {
        width: 1fr;
        content-align: left middle;
        color: #a8adb8;
    }

    #hero {
        height: 8;
        margin: 1 0 1 0;
        padding: 0;
        border: round #d57a56;
        background: #2b3140;
    }

    #hero-title {
        height: 1;
        padding: 0 1 0 2;
        color: #d57a56;
        background: #2b3140;
    }

    #hero-body {
        height: 1fr;
        background: #2b3140;
    }

    #hero-left {
        width: 28;
        min-width: 28;
        padding: 1 2;
        border-right: solid #d57a56 35%;
        background: #2b3140;
        content-align: center middle;
    }

    #hero-right {
        width: 1fr;
        padding: 1 2;
        background: #2b3140;
    }

    #hero-copy {
        color: #d9dce3;
    }

    #divider {
        height: 1;
        margin: 0 0 1 0;
        color: #7b8190;
        background: #242934;
    }

    #results-pane {
        width: 100%;
        height: 1fr;
        min-height: 1;
        margin: 0;
        padding: 0 1;
        background: #242934;
    }

    #results {
        width: 100%;
        height: 1fr;
        min-height: 1;
        padding: 0;
        background: #242934;
        border: none;
        scrollbar-size: 0 0;
    }

    #results .rich-log--line {
        background: #242934;
    }

    #prompt-rule-top {
        height: 1;
        color: #9a9a98;
        background: #242934;
    }

    #prompt-row {
        height: 2;
        padding: 0 1;
        background: #242934;
    }

    #prompt-glyph {
        width: 3;
        color: #e5e7eb;
        content-align: left middle;
    }

    #command-suggestions {
        height: auto;
        min-height: 0;
        max-height: 8;
        padding: 0 1 0 4;
        color: #a7acb8;
        background: #242934;
    }

    #prompt-rule-bottom {
        height: 1;
        margin: 0 0 1 0;
        color: #9a9a98;
        background: #242934;
    }

    #command-line {
        height: 1;
        padding: 0;
        background: #242934;
    }

    #command-input {
        width: 1fr;
        border: none;
        background: #242934;
        color: #e5e7eb;
        padding: 0;
    }

    #command-input > .input--cursor {
        background: #e5e7eb;
        color: #242934;
    }

    #command-help {
        height: 1;
        padding: 0 1 0 4;
        color: #a7acb8;
        background: #242934;
    }

    #status-line {
        height: 1;
        padding: 0 1;
        color: #b9bdc7;
        background: #242934;
    }

    #progress-bar {
        height: 1;
        padding: 0 1;
        color: #22c55e;
        background: #242934;
    }

    Input {
        background: #242934;
    }

    RichLog {
        background: #242934;
    }

    Container {
        background: #242934;
    }

    Vertical {
        background: #242934;
    }

    Static {
        background: #242934;
    }

    .scrollbar {
        display: none;
    }
    """

    BINDINGS = [
        ("r", "run_tests", "Run tests"),
        ("c", "clear", "Clear"),
        ("slash", "focus_command", "Command"),
        ("end", "scroll_to_bottom", "Bottom"),
        ("ctrl+c", "quit", "Quit"),
    ]

    def __init__(self):
        super().__init__()
        self.backend = "c"
        os.environ["CASIO_BACKEND"] = "c"
        self.records = []
        self.run_state = RunState.IDLE
        self.current_program = "all"
        self.last_run_scope = ("all", "chaos")
        self.last_command = "/random"
        self.last_report_path = None
        self.current_random_workers = None
        self.current_run_question_keys = set()
        self.session_random_question_keys = set()
        self.random_graph = RandomExplorationGraph(CATALOGUE_GRAPH_PATH)
        self._random_infinite = False
        self._session_report_lock = threading.Lock()
        self.start_time = None
        self.total_expected = 0
        self._last_eta_update = 0
        self.llm_manager = None
        self.llm_enabled_for_tests = False
        self._test_times = []
        self._llm_times = []
        self._test_timestamps = []
        self._llm_timestamps = []
        self._eta_decay = 0.9
        self._weighted_rate = 0.0
        self._weighted_llm_rate = 0.0
        self._feature_stats = {}
        self._new_adversarial_report_store()
        self._show_chaos_feature_gaps = False
        self._in_run_case_specs = False
        self._spinner_frames = ("⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏")
        self._spinner_index = 0
        self._summary_text = "Ready"
        self._live_program = "idle"
        self._live_phase = "ready"
        self._last_event_label = ""
        self._frame_timer = None
        self._compile_running = False
        self.adversarial_report_store = None

        self.command_items = {
            "/run": "Run the current program scope once",
            "/random": "Run randomly generated tests (infinite until stopped)",
            "/random 1000": "Run 1000 chaos random tests split across all programs and features",
            "/random 1000 8": "Run 1000 chaos random tests with 8 parallel workers",
            "/infinite": "Run tests infinitely until stopped",
            "/stress integrate 100": "Run focused adversarial tests for one feature",
            "/replay adv-00001": "Replay an adversarial failure from the latest run folder",
            "/shrink adv-00001": "Show the stored minimal repro attempt",
            "/clear": "Reset the output view",
            "/stop": "Stop running tests (in infinite mode)",
            "/quit": "Exit the test harness",
            "/status": "Show the current scope and last run summary",
            "/report": "Show detailed failures from the last run",
            "/coverage": "Show feature coverage gaps and report path",
            "/fails": "List failed tests from the last run",
            "/programs": "List available test programs",
            "/help": "Show useful commands",
            "/compile": "Compile C++ host and fx-CG50 .g3a with live progress",
            "/switch c": "C++ backend is always active",
            "/swtich c": "Typo alias for /switch c",
            "/llm": "Configure LLM verification (select model, enable/disable)",
            "/llm status": "Show current LLM configuration",
            "/llm disable": "Disable LLM verification",
        }

    def backend_label(self, backend=None) -> str:
        return "C++ host + native Prizm .g3a"

    def ready_summary(self) -> str:
        return f"Ready · backend {self.backend_label()}"

    def switch_backend(self, backend: str) -> None:
        b = (backend or "").strip().lower()
        if b in ("python", "py"):
            self.append_result("[bold #f59e0b]Python backend removed; C++ backend remains active.[/bold #f59e0b]")
        else:
            self.append_result(f"[dim]Backend already:[/dim] {self.backend_label('c')}")
        self.backend = "c"
        os.environ["CASIO_BACKEND"] = "c"
        self.update_summary(self.ready_summary())

    @property
    def running(self):
        # Backwards-compatible property; new code should reason in RunState.
        return self.run_state == RunState.RUNNING

    @running.setter
    def running(self, value):
        self.transition_run_state(RunState.RUNNING if value else RunState.STOPPED)

    def transition_run_state(self, new_state):
        """Move between random-run lifecycle states in one controlled place."""
        if self.run_state == new_state:
            return
        allowed = {
            RunState.IDLE: (RunState.RUNNING, RunState.STOPPED),
            RunState.RUNNING: (RunState.STOPPING, RunState.STOPPED),
            RunState.STOPPING: (RunState.STOPPED,),
            RunState.STOPPED: (RunState.RUNNING, RunState.IDLE),
        }
        if new_state in allowed.get(self.run_state, ()):
            self.run_state = new_state
            return
        # Older error paths can still jump straight to STOPPED.  Prefer a clean
        # stopped UI over leaving the runner visibly stuck.
        self.run_state = new_state

    def request_stop(self):
        if self.run_state == RunState.RUNNING:
            self.transition_run_state(RunState.STOPPING)

    def is_stopping(self):
        return self.run_state in (RunState.STOPPING, RunState.STOPPED)

    def compose(self):
        with Container(id="shell"):
            with Container(id="chrome"):
                yield Static("CASIO Test Suite v1.0", id="chrome-title")

            with Container(id="hero"):
                yield Static("[bold #e7895f]CASIO Test Suite[/bold #e7895f] [dim]v1.0[/dim]", id="hero-title")
                with Horizontal(id="hero-body"):
                    with Container(id="hero-left"):
                        yield Static(
                            "[bold]Welcome back![/bold]\n\n"
                            "   .-.\n"
                            "  (o o)\n"
                            "  | O \\\n"
                            "   \\   \\\n"
                            "    `~~~'\n\n"
                            "[dim]gpt-5.4 · API Usage Billing[/dim]\n"
                            f"[dim]{REPO_ROOT}[/dim]",
                        )
                    with Container(id="hero-right"):
                        yield Static(
                            "[bold #a66552]Tips for getting started[/bold #a66552]\n"
                            "Run [bold]/run[/bold] for tests, or [bold]/switch c[/bold] then [bold]/compile[/bold] for the .g3a.\n\n"
                            "[bold #a66552]Recent activity[/bold #a66552]\n"
                            "[dim]Now[/dim] Ready to run algebra, trig, derive, integrate, stats, and SUVAT checks.\n"
                            "[dim]Hint[/dim] Type [bold]/[/bold] to browse commands.\n"
                            "[dim]Resume[/dim] Use [bold]/report[/bold] or [bold]/fails[/bold] after a run for investigation.",
                            id="hero-copy",
                        )

            yield Static("", id="divider")

            with Vertical(id="results-pane"):
                yield RichLog(id="results", wrap=False, markup=True, auto_scroll=True, highlight=False, max_lines=1000)

            yield Static("", id="prompt-rule-top")
            with Horizontal(id="prompt-row"):
                yield Static("›", id="prompt-glyph")
                with Container(id="command-line"):
                    yield Input(placeholder="", id="command-input")
            yield Static("", id="prompt-rule-bottom")
            yield Static("", id="command-help")
            yield Static("", id="command-suggestions")
            yield Static("Ready", id="status-line")
            yield Static("", id="progress-bar")

    def on_mount(self):
        self.title = "CASIO Test Suite"
        self.sub_title = "Interactive maths harness"
        self.refresh_rule_lines()
        self.query_one("#command-input", Input).focus()
        try:
            self._frame_timer = self.set_interval(1.0 / CASIO_TUI_FPS, self.render_live_frame)
        except Exception:
            self._frame_timer = None
        self.action_clear()
        self.append_result(f"[dim]Backend:[/dim] {self.backend_label()}")
        self.append_result("[dim]Use /switch c, then /compile to build fx-CG50 CasioCAS.g3a.[/dim]")
        self.schedule_start_commands()

    def schedule_start_commands(self):
        raw = (os.environ.get("CASIO_TUI_START_COMMANDS", "") or "").strip()
        if not raw:
            return
        commands = [cmd.strip() for cmd in re.split(r";;|\n", raw) if cmd.strip()]
        delay = 0.4
        for command in commands:
            self.set_timer(delay, lambda command=command: self.run_start_command(command))
            delay += 1.2 if command.lower().startswith("/llm") else 0.4

    def run_start_command(self, value: str):
        value = value.strip()
        if not value:
            return
        value_lower = value.lower()
        self.last_command = value
        self.append_result(f"[dim]Auto:[/dim] {value}")
        random_scope = self.parse_random_scope(value)
        if random_scope is not None:
            difficulty, count, workers, _infinite_mode = random_scope
            self.current_random_workers = workers
            self.action_random_tests(difficulty, count, workers, command_label=value)
            return
        stress_scope = self.parse_stress_scope(value)
        if stress_scope is not None:
            feature, count, workers = stress_scope
            self.action_stress_tests(feature, count, workers, command_label=value)
            return
        if value_lower == "/infinite":
            self.action_random_tests("chaos", None, command_label="/infinite")
        elif value_lower == "/llm" or value_lower == "/llm status":
            self.handle_llm_status()
        elif value_lower == "/llm disable":
            self.handle_llm_disable()
        elif value_lower.startswith("/llm "):
            self.handle_llm_select(value)
        elif value_lower == "/compile":
            self.action_compile()
        else:
            self.append_result(f"[red]Auto command not supported:[/red] {value}")

    def on_resize(self, event) -> None:
        self.refresh_rule_lines()

    def refresh_rule_lines(self):
        line_width = max(self.size.width - 4, 20)
        line = "─" * line_width
        for selector in ("#divider", "#prompt-rule-top", "#prompt-rule-bottom"):
            self.query_one(selector, Static).update(line)

    def action_focus_command(self):
        input_widget = self.query_one("#command-input", Input)
        if not input_widget.value.startswith("/"):
            input_widget.value = "/"
        input_widget.cursor_position = len(input_widget.value)
        input_widget.focus()
        self.update_command_help(input_widget.value)

    def action_scroll_to_bottom(self):
        self.query_one("#results", RichLog).scroll_end(animate=False)
        self.query_one("#command-input", Input).focus()

    def normalize_program_name(self, value: str):
        mapping = {
            "all": "all",
            "adversarial": "Adversarial",
            "algebra": "Algebra",
            "trigonometry": "Trigonometry",
            "trig": "Trigonometry",
            "derive": "Derive",
            "integration": "Integrate",
            "integrate": "Integrate",
            "int": "Integrate",
            "suvat": "SUVAT",
            "stats": "Stats",
            "statistics": "Stats",
            "stat": "Stats",
            "boolean": "Boolean",
            "bool": "Boolean",
            "methods": "MethodSurface",
            "method": "MethodSurface",
            "methodsurface": "MethodSurface",
            "exam": "ExamMix",
            "exammix": "ExamMix",
        }
        return mapping.get(value.lower())

    def normalize_difficulty(self, value: str):
        return "chaos"

    def parse_run_scope(self, value: str):
        parts = value.split()
        if not parts or parts[0].lower() != "/random":
            return None
        if len(parts) == 1:
            return self.current_program, "chaos"
        if len(parts) == 2 and parts[1].lower() == "all":
            return "all", "chaos"

        program = self.current_program

        for token in parts[1:]:
            maybe_program = self.normalize_program_name(token)
            if maybe_program is not None:
                program = maybe_program
                continue
            return None
        return program, "chaos"

    def parse_random_scope(self, value: str):
        parts = value.split()
        if not parts or parts[0].lower() != "/random":
            return None
        count = None
        workers = None
        i = 1
        while i < len(parts):
            if parts[i].isdigit():
                if count is None:
                    count = int(parts[i])
                elif workers is None:
                    workers = int(parts[i])
                else:
                    return None
                i += 1
                continue
            return None
        infinite_mode = count is None
        if count is not None and count <= 0:
            return None
        if workers is not None and workers <= 0:
            return None
        return "chaos", count, workers, infinite_mode

    def parse_stress_scope(self, value: str):
        parts = value.split()
        if not parts or parts[0].lower() != "/stress":
            return None
        feature = (parts[1].lower() if len(parts) > 1 else "all")
        count = 100
        workers = None
        if len(parts) > 2:
            try:
                count = int(parts[2])
            except ValueError:
                return None
        if len(parts) > 3:
            try:
                workers = int(parts[3])
            except ValueError:
                return None
        if count <= 0 or (workers is not None and workers <= 0):
            return None
        return feature, count, workers

    def on_key(self, event) -> None:
        if event.key == "ctrl+p":
            event.prevent_default()
            event.stop()

    def on_input_changed(self, event: Input.Changed) -> None:
        self.update_command_help(event.value)

    def on_input_submitted(self, event: Input.Submitted) -> None:
        value = event.value.strip()
        value_lower = value.lower()
        event.input.value = ""
        self.update_command_help("")
        self.query_one("#command-suggestions", Static).update("")

        if not value:
            return

        random_scope = self.parse_random_scope(value)
        if random_scope is not None:
            difficulty, count, workers, infinite_mode = random_scope
            self.current_random_workers = workers
            self.last_command = value
            self.action_random_tests(difficulty, count, workers, command_label=value)
            return

        stress_scope = self.parse_stress_scope(value)
        if stress_scope is not None:
            feature, count, workers = stress_scope
            self.last_command = value
            self.action_stress_tests(feature, count, workers, command_label=value)
            return

        run_scope = self.parse_run_scope(value)
        if run_scope is not None:
            program, difficulty = run_scope
            self.current_program = program
            self.last_command = value
            self.action_run_tests(command_label=value)
            return

        if value_lower == "/run":
            self.current_program = "all"
            self.last_command = value
            self.action_run_tests(command_label="/run")
        elif value_lower == "/clear":
            self.action_clear()
        elif value_lower == "/stop":
            self.action_stop()
        elif value_lower == "/infinite":
            self.action_random_tests("chaos", None, command_label="/infinite")
        elif value_lower == "/quit":
            self.action_quit()
        elif value_lower == "/status":
            self.show_status()
        elif value_lower == "/report":
            self.show_report()
        elif value_lower == "/coverage":
            self.show_coverage()
        elif value_lower.startswith("/replay "):
            self.action_replay_case(value.split(None, 1)[1].strip())
        elif value_lower.startswith("/shrink "):
            self.action_shrink_case(value.split(None, 1)[1].strip())
        elif value_lower == "/fails":
            self.show_fails()
        elif value_lower == "/programs":
            self.show_programs()
        elif value_lower == "/help":
            self.show_help()
        elif value_lower == "/compile":
            self.action_compile()
        elif value_lower in ("/switch python", "/swtich python"):
            self.switch_backend("python")
        elif value_lower in ("/switch c", "/swtich c"):
            self.switch_backend("c")
        elif value_lower == "/llm" or value_lower == "/llm status":
            self.handle_llm_status()
        elif value_lower == "/llm disable":
            self.handle_llm_disable()
        elif value_lower.startswith("/llm "):
            self.handle_llm_select(value)

    def _failure_report_record_ids(self):
        """Records to include: harness failure, or LLM disagreement worth review."""
        ids = []
        for r in self.records:
            if r.harness_status == TestStatus.FAIL or getattr(r, "review_needed", False):
                ids.append(r.test_id)
        return ids

    def _session_report_path(self):
        return REPO_ROOT / "c++" / "tests" / "reports" / "failure_report_latest.txt"

    def _init_session_report_file(self):
        """Reset the on-disk log at the start of a run; failures append while tests execute."""
        p = self._session_report_path()
        p.parent.mkdir(parents=True, exist_ok=True)
        now = datetime.now().isoformat(timespec="seconds")
        try:
            scope_s = "{0!r} | {1!r}".format(self.last_run_scope[0], self.last_run_scope[1])
        except (TypeError, ValueError, IndexError):
            scope_s = "unknown"
        header = [
            "CASIO live session report (appends for each failure / LLM follow-up while tests run)",
            f"Started: {now}",
            f"Command: {self.last_command}",
            f"Program: {self.current_program}",
            f"Scope: {scope_s}",
            f"See {p.parent} - tail this file in another terminal for a live view.",
            "=" * 72,
            "",
        ]
        with self._session_report_lock:
            p.write_text("\n".join(header) + "\n", encoding="utf-8")
        self.last_report_path = str(p)
        if not getattr(self, "plain_mode", False):
            self.append_result(f"[dim]Session log:[/dim] {p}")

    def _append_session_report_if_worthy(self, record):
        """Append a compact block for high-signal failures only."""
        llm = (getattr(record, "llm_verdict", "") or "").strip()
        review_needed = bool(getattr(record, "review_needed", False))
        # Keep report actionable: harness failures plus LLM disagreements.
        if record.harness_status != TestStatus.FAIL and not review_needed:
            return
        kind = "llm review" if review_needed and record.harness_status != TestStatus.FAIL else "harness failure"
        p = self._session_report_path()
        lines = [
            "-" * 72,
            "#{0} [{1}] {2}{3}".format(
                record.test_id,
                record.program,
                record.label,
                " [" + record.feature + "]" if record.feature else "",
            ),
            "Type: {0}".format(kind),
            "Final: {0}  |  harness: {1}".format(record.status.name, record.harness_status.name),
        ]
        if llm:
            lines.append("LLM: {0}".format(llm))
            expl = (getattr(record, "llm_explanation", "") or "").strip()
            if expl:
                lines.append("LLM notes: {0}".format(expl[:2000]))
        lines.append("Input:")
        lines.append((record.input_text or "").rstrip())
        lines.append("Check:")
        lines.append((record.check_info or "").rstrip())
        out = record.output or ""
        tail = out if len(out) < 8000 else "\n".join((out or "").splitlines()[-50:])
        lines.append("Output:")
        lines.append(tail.rstrip())
        lines.append("")
        block = "\n".join(lines) + "\n"
        with self._session_report_lock:
            with open(p, "a", encoding="utf-8") as fh:
                fh.write(block)

    def action_run_tests(self, command_label="/run"):
        if self.running:
            return
        self.transition_run_state(RunState.RUNNING)
        self.records.clear()
        self.current_run_question_keys.clear()
        self.session_random_question_keys.clear()
        self._feature_stats = {}
        self.last_run_scope = (self.current_program, "all")
        self._init_session_report_file()
        if not getattr(self, 'plain_mode', False):
            self.query_one("#results", RichLog).clear()
        self.append_result(f"[bold #e07a53]▶ {command_label}[/bold #e07a53]")
        self.append_result("[dim]Booting test harness...[/dim]")
        self.update_summary(f"Running {self.current_program} · all...")
        self.run_all_tests()

    def action_random_tests(self, difficulty, count, workers=None, command_label="/random", program=None):
        if self.running:
            return
        self.transition_run_state(RunState.RUNNING)
        self._live_program = program or "all"
        self._live_phase = "preparing"
        self._last_event_label = command_label
        self.last_command = command_label
        self.records.clear()
        self.current_run_question_keys.clear()
        self.session_random_question_keys.clear()
        self.last_run_scope = ("random", difficulty)

        if not getattr(self, 'plain_mode', False):
            self.query_one("#results", RichLog).clear()
        
        self.append_result(f"[bold #e07a53]▶ {command_label}[/bold #e07a53]")
        self.append_result("[dim]Generating random test cases...[/dim]")
        
        if self.llm_manager and self.llm_manager.enabled:
            self.append_result(f"[dim]LLM: {self.llm_manager.selected_model}[/dim]")
        
        self.run_random_tests(difficulty, count, workers, program=program)

    def _adversarial_reports_root(self):
        return self._session_report_path().parent / "adversarial_runs"

    def _new_adversarial_report_store(self):
        if not ADVERSARIAL_ENGINE_AVAILABLE:
            return None
        self.adversarial_report_store = RunReportStore.fresh_under(self._session_report_path().parent)
        return self.adversarial_report_store

    def _latest_adversarial_report_store(self):
        if self.adversarial_report_store:
            return self.adversarial_report_store
        root = self._adversarial_reports_root()
        try:
            candidates = [p for p in root.iterdir() if (p / "replay_index.json").exists()]
        except OSError:
            candidates = []
        if not candidates or not ADVERSARIAL_ENGINE_AVAILABLE:
            return None
        latest = sorted(candidates)[-1]
        self.adversarial_report_store = RunReportStore(latest)
        return self.adversarial_report_store

    def make_adversarial_case(self, adv_case, index):
        def runner():
            out, err = self.run_host_feature(adv_case.command_flag, adv_case.command_expr)
            combined = out if not err else out + "\n" + err
            verdict = classify_output_quality(combined, expects_working=adv_case.expects_working)
            ok = verdict.status != "fail"
            if verdict.status != "pass":
                combined = combined.rstrip() + "\n[quality] {0}: {1}".format(verdict.status, verdict.reason)
            return ok, combined

        case = self.make_direct_case(
            "Adversarial",
            adv_case.label,
            runner,
            input_text=adv_case.input_text,
            check_info="{0}; {1}".format(adv_case.expected_note, adv_case.concept.key),
            feature="adversarial:{0}:{1}:{2}".format(
                adv_case.concept.function,
                adv_case.concept.method,
                adv_case.concept.topic,
            ),
        )
        case.graph_node = adv_case.concept.key
        case.graph_meta = adv_case.concept.meta()
        case.adversarial_case = adv_case
        return case

    def build_adversarial_random_cases(self, difficulty, count, rng):
        if not ADVERSARIAL_ENGINE_AVAILABLE:
            return []
        gen = AdversarialGenerator(seed=rng.randint(1, 2_000_000_000))
        count = max(0, int(count))
        online_count = max(1, count // 6) if count >= 6 else 0
        syllabus_count = max(1, count // 8) if count >= 8 else 0
        exam_count = min(max(0, count - online_count - syllabus_count), max(1, count // 2)) if count else 0
        filler_count = max(0, count - exam_count - online_count - syllabus_count)
        raw_cases = (
            gen.generate("exam_gap", exam_count)
            + gen.generate("online", online_count)
            + gen.generate("syllabus", syllabus_count)
            + gen.generate("all", filler_count)
        )
        seen = set()
        cases = []
        for raw in raw_cases:
            if raw.input_text in seen:
                continue
            seen.add(raw.input_text)
            cases.append(raw)
        return [self.make_adversarial_case(c, i + 1) for i, c in enumerate(cases[:count])]

    def record_adversarial_case_result(self, case, record):
        adv = getattr(case, "adversarial_case", None)
        if not adv or not self.adversarial_report_store:
            return
        quality_review = "[quality] review:" in (record.output or "").lower()
        if quality_review and record.harness_status == TestStatus.PASS:
            record.review_needed = True
        if record.harness_status == TestStatus.PASS and not getattr(record, "review_needed", False):
            return
        reason = record.check_info or ("failed" if record.harness_status == TestStatus.FAIL else "review")
        try:
            stored = self.adversarial_report_store.record_case(
                adv,
                "fail" if record.harness_status == TestStatus.FAIL else "review",
                record.output,
                reason,
            )
            record.check_info = (record.check_info or "") + "; replay={0}".format(stored.case_id)
        except Exception:
            pass

    def action_stress_tests(self, feature, count=100, workers=None, command_label="/stress"):
        if self.running:
            return
        if not ADVERSARIAL_ENGINE_AVAILABLE:
            self.append_result("[bold #f59e0b]Adversarial engine unavailable.[/bold #f59e0b]")
            return
        self.transition_run_state(RunState.RUNNING)
        self._live_program = "Adversarial"
        self._live_phase = "preparing"
        self._last_event_label = command_label
        self.last_command = command_label
        self.records.clear()
        self.current_run_question_keys.clear()
        self.session_random_question_keys.clear()
        self._feature_stats = {}
        self.last_run_scope = ("stress:" + feature, "chaos")
        self._init_session_report_file()
        self._new_adversarial_report_store()

        if not getattr(self, 'plain_mode', False):
            self.query_one("#results", RichLog).clear()

        def run():
            try:
                emit = (lambda fn, *args: fn(*args)) if getattr(self, 'plain_mode', False) else self.call_from_thread
                self.announce_random_llm_status(emit)
                emit(self.append_result, f"[bold #e07a53]▶ {command_label}[/bold #e07a53]")
                emit(self.append_result, f"[dim]{count} adversarial {feature} tests[/dim]")
                self.start_time = datetime.now().timestamp()
                self.total_expected = count
                rng = random.Random()
                gen = AdversarialGenerator(seed=rng.randint(1, 2_000_000_000))
                cases = [self.make_adversarial_case(c, i + 1) for i, c in enumerate(gen.generate(feature, count))]
                self.run_case_specs(cases, workers=workers or CASE_WORKERS, infinite_mode=False)
                self.transition_run_state(RunState.STOPPED)
                if getattr(self, 'plain_mode', False):
                    self.render_summary()
                else:
                    self.call_from_thread(self.render_summary)
            finally:
                self._live_phase = "ready"

        if getattr(self, 'plain_mode', False):
            run()
        else:
            threading.Thread(target=run, daemon=True).start()

    def action_replay_case(self, case_id):
        store = self._latest_adversarial_report_store()
        if not store:
            self.append_result("[bold #f59e0b]No adversarial replay store found.[/bold #f59e0b]")
            return
        try:
            payload = store.load_case(case_id)
        except Exception as err:
            self.append_result(f"[bold #f87171]Replay not found:[/bold #f87171] {err}")
            return
        out, err = self.run_host_feature(payload["command_flag"], payload["command_expr"])
        combined = out if not err else out + "\n" + err
        verdict = classify_output_quality(combined, expects_working=True)
        self.append_result(f"[bold #e07a53]Replay {case_id}[/bold #e07a53] {verdict.status}: {verdict.reason}")
        self.append_result("[dim]Input:[/dim] {0}".format(payload["input_text"]))
        self.append_result(combined.rstrip() or "[dim](no output)[/dim]")

    def action_shrink_case(self, case_id):
        store = self._latest_adversarial_report_store()
        if not store:
            self.append_result("[bold #f59e0b]No adversarial replay store found.[/bold #f59e0b]")
            return
        try:
            payload = store.load_case(case_id)
        except Exception as err:
            self.append_result(f"[bold #f87171]Shrink not found:[/bold #f87171] {err}")
            return
        self.append_result(f"[bold #e07a53]Shrink {case_id}[/bold #e07a53]")
        self.append_result("[dim]Original:[/dim] {0}".format(payload.get("command_expr", "")))
        self.append_result("[dim]Shrunk:[/dim] {0}".format(payload.get("shrunk_expr", "")))

    def action_clear(self):
        self.records.clear()
        log = self.query_one("#results", RichLog)
        log.clear()
        log.write("")
        self.query_one("#command-suggestions", Static).update("")
        self.query_one("#command-help", Static).update("? for shortcuts")
        self.update_summary(self.ready_summary())

    def action_stop(self):
        if self.running:
            self.request_stop()
            self._live_phase = "stopping"
            self.append_result("[dim]Stop requested - current random batch will finish first.[/dim]")
            self.update_summary("Stopping...")
        elif self.records:
            self.render_summary()
        else:
            self.update_summary("Stopped")

    def action_compile(self):
        self.action_compile_cpp()

    def action_compile_cpp(self):
        import subprocess
        import time
        import site

        if getattr(self, "_compile_running", False):
            self.append_result("[bold #f59e0b]Compile already running.[/bold #f59e0b]")
            return

        self._compile_running = True
        plain = getattr(self, "plain_mode", False)
        subprocess_env = os.environ.copy()
        existing_path = subprocess_env.get("PATH", "")
        path_parts = [
            str(Path(sys.executable).resolve().parent),
            str(Path(site.USER_BASE) / "bin"),
            "/opt/homebrew/bin",
            "/opt/homebrew/sbin",
            "/usr/local/bin",
            "/usr/local/sbin",
            "/Applications/Docker.app/Contents/Resources/bin",
        ]
        path_parts.extend(p for p in existing_path.split(os.pathsep) if p)
        deduped_path = []
        for p in path_parts:
            if p and p not in deduped_path:
                deduped_path.append(p)
        subprocess_env["PATH"] = os.pathsep.join(deduped_path)
        subprocess_env["CASIO_PYTHON"] = sys.executable

        def emit(fn, *args):
            if plain:
                fn(*args)
            else:
                self.call_from_thread(fn, *args)

        def log(message):
            emit(self.append_result, message)

        def summary(message):
            emit(self.update_summary, message)

        def clean_addin_outputs():
            output_dirs = [
                REPO_ROOT / "c++" / "prizm" / "build",
                REPO_ROOT / "c++" / "addin" / "build-cg",
            ]
            patterns = ("*.g3a", "CasioCAS", "CasioCAS.bin")
            removed = []
            for output_dir in output_dirs:
                if not output_dir.exists():
                    continue
                for pattern in patterns:
                    for path in output_dir.glob(pattern):
                        if path.is_file():
                            try:
                                path.unlink()
                            except OSError as err:
                                log(f"[bold #f87171]✗ could not remove stale output:[/bold #f87171] {path} ({err})")
                                return False
                            removed.append(path)
            if removed:
                for path in removed:
                    log(f"[dim]removed stale output:[/dim] {path}")
            else:
                log("[dim]no existing .g3a output to remove; creating a fresh one[/dim]")
            return True

        def run_stream(label, cmd, cwd=None, show_output=True):
            log(f"[dim]▶ start: {label}[/dim]")
            start = time.monotonic()
            try:
                proc = subprocess.Popen(
                    cmd,
                    cwd=str(cwd or REPO_ROOT),
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    bufsize=1,
                    env=subprocess_env,
                )
            except OSError as err:
                log(f"[bold #f87171]✗ failed to start {label}:[/bold #f87171] {err}")
                return 127

            tail = []
            if proc.stdout is not None:
                for raw in proc.stdout:
                    line = raw.rstrip()
                    if not line:
                        continue
                    tail.append(line)
                    if len(tail) > 12:
                        tail.pop(0)
                    if show_output:
                        log(f"[dim]  {line}[/dim]")

            rc = proc.wait()
            elapsed = time.monotonic() - start
            if rc == 0:
                log(f"[bold #22c55e]✓ finish: {label} ({elapsed:.1f}s)[/bold #22c55e]")
            else:
                log(f"[bold #f87171]✗ finish: {label} failed with exit {rc} ({elapsed:.1f}s)[/bold #f87171]")
                if tail:
                    log("[dim]Last output:[/dim]\n" + "\n".join(tail))
            return rc

        def compile_job():
            try:
                log("[bold #e07a53]▶ /compile (C++ backend)[/bold #e07a53]")
                summary("Compiling (C++ host + .g3a)...")

                log("[dim]--- Building host (casio_host) ---[/dim]")
                log("[dim]Using c++/tools/build_host.sh so CMake can be found or bootstrapped.[/dim]")
                if run_stream("host build bootstrap", ["bash", "c++/tools/build_host.sh"]) != 0:
                    summary("Compile failed")
                    return
                log("[bold #22c55e]✓ host ready[/bold #22c55e]")
                log("[dim]Output:[/dim] c++/addin/host/build/casio_host")

                log("")
                log("[dim]--- Building add-in (.g3a) ---[/dim]")
                if not clean_addin_outputs():
                    summary("Compile failed")
                    return
                subprocess_env["CASIO_PRIZM_MODE"] = "khicas-source"
                log("[dim]Target:[/dim] Casio fx-CG50 PrizmSDK/libfxcg .g3a")
                if run_stream("fx-CG50 PrizmSDK add-in build", ["./c++/tools/build_addin_prizm_docker.sh"]) != 0:
                    summary("Compile failed")
                    return
                log("[dim]--- Verifying outputs (fx-CG50 PrizmSDK) ---[/dim]")
                g3as = sorted((REPO_ROOT / "c++" / "prizm" / "build").glob("*.g3a"))
                if not g3as:
                    log("[bold #f87171]✗ no .g3a found for selected add-in target[/bold #f87171]")
                    summary("Compile failed")
                    return

                for p in g3as:
                    size_kb = p.stat().st_size / 1024
                    log(f"[bold #22c55e]✓ .g3a:[/bold #22c55e] {p} ({size_kb:.1f} KB)")

                size_cmd = ["python3", "c++/tools/check_g3a_size.py"] + [str(p) for p in g3as]
                if run_stream("g3a size gate", size_cmd) != 0:
                    summary("Compile failed")
                    return

                log("")
                log("[bold #22c55e]✓ Compile complete: fx-CG50 .g3a ready[/bold #22c55e]")
                summary("Compile OK")
            finally:
                self._compile_running = False

        if plain:
            compile_job()
        else:
            threading.Thread(target=compile_job, daemon=True).start()

    def action_quit(self):
        self.exit()

    def show_help(self):
        self.append_result("[bold #e07a53]Available commands[/bold #e07a53]")
        self.append_result("")
        width = max(len(cmd) for cmd in self.command_items) + 2
        for cmd, desc in self.command_items.items():
            self.append_result(f"[bold #e07a53]{cmd.ljust(width)}[/bold #e07a53][dim]{desc}[/dim]")
        self.append_result("")
        self.append_result("[dim]Tip: type / to browse matching commands interactively.[/dim]")
        self.update_summary("Help")

    def handle_llm_status(self):
        self.append_result("[bold #e07a53]LLM Verification Status[/bold #e07a53]")

        if not LLM_AVAILABLE:
            self.append_result("[bold #f59e0b]LLM Module Not Available[/bold #f59e0b]")
            self.update_summary("LLM not available")
            return

        from shared_llm import check_ollama_available, get_ollama_models

        available = check_ollama_available()
        if not available:
            self.append_result("[bold #f59e0b]Ollama Not Running[/bold #f59e0b]")
            self.update_summary("Ollama not running")
            return

        models = get_ollama_models()
        if not models:
            self.append_result("[bold #f59e0b]No Models Found[/bold #f59e0b]")
            self.update_summary("No models")
            return

        self.append_result("[bold]Available Models:[/bold]")
        for i, m in enumerate(models):
            marker = "[#22c55e]*[/#22c55e]" if self.llm_manager and self.llm_manager.selected_model == m["name"] else " "
            self.append_result(f"  {marker} {i+1}. {m['name']} ({m['size']})")

        if self.llm_manager and self.llm_manager.enabled:
            status = self.llm_manager.get_status()
            self.append_result(f"[#22c55e]Enabled[/#22c55e] with: {status['selected_model']}")
        else:
            self.append_result("[dim]LLM verification disabled[/dim]")

        self.update_summary("LLM Status")

    def handle_llm_select(self, value):
        parts = value.split()
        if len(parts) < 2:
            self.handle_llm_status()
            return

        try:
            idx = int(parts[1]) - 1
        except ValueError:
            model_name = parts[1]
            idx = None

        if self.llm_manager is None:
            if LLM_AVAILABLE:
                from shared_llm import LLMManager
                self.llm_manager = LLMManager()
            else:
                self.append_result("[bold #f59e0b]LLM not available[/bold #f59e0b]")
                return

        models = self.llm_manager.list_models()
        if not models:
            self.append_result("[bold #f59e0b]No models available[/bold #f59e0b]")
            return

        if idx is not None:
            if 0 <= idx < len(models):
                success = self.llm_manager.select_model(idx)
            else:
                self.append_result(f"[bold #f59e0b]Invalid model number: {idx+1}[/bold #f59e0b]")
                return
        else:
            success = self.llm_manager.select_model(model_name)

        if success:
            self.llm_manager.enable()
            self.llm_enabled_for_tests = True
            self.append_result(f"[#22c55e]Enabled[/#22c55e] LLM verification with: {self.llm_manager.selected_model}")
            self.append_result("Run /random tests to use LLM verification")
        else:
            self.append_result(f"[bold #f59e0b]Model not found: {model_name}[/bold #f59e0b]")

        self.update_summary("LLM Model Selected")

    def handle_llm_disable(self):
        if self.llm_manager:
            self.llm_manager.disable()
            self.llm_enabled_for_tests = False
            self.append_result("[bold #f59e0b]LLM verification disabled[/bold #f59e0b]")
        else:
            self.append_result("[dim]LLM was not enabled[/dim]")
        self.update_summary("LLM Disabled")

    def show_programs(self):
        self.append_result("[bold #e07a53]Programs[/bold #e07a53]")
        self.append_result("[dim]Algebra[/dim] — symbolic comparison, transforms, solving, inverses")
        self.append_result("[dim]Trigonometry[/dim] — identities, transforms, equation solving")
        self.append_result("[dim]Derive[/dim] — differentiation and harder chain-rule cases")
        self.append_result("[dim]Integrate[/dim] — standard integrals, substitution, parts, extremes")
        self.append_result("[dim]Stats[/dim] — one-var stats, regression/correlation, probability, plots")
        self.append_result("[dim]SUVAT[/dim] — motion equations and projectile-style checks")
        self.append_result("[dim]Boolean[/dim] — simplify, NAND, NOR, proof checks")
        self.update_summary("Programs")

    def show_status(self):
        total = len(self.records)
        passed = sum(1 for r in self.records if r.status == TestStatus.PASS)
        failed = total - passed
        self.append_result("[bold #e07a53]Status[/bold #e07a53]")
        self.append_result(f"[dim]Backend:[/dim] {self.backend_label()}")
        self.append_result(f"[dim]Current program:[/dim] {self.current_program}")
        self.append_result(f"[dim]Difficulty:[/dim] chaos")
        self.append_result(f"[dim]Last command:[/dim] {self.last_command}")
        self.append_result(f"[dim]Last run scope:[/dim] {self.last_run_scope[0]} · chaos")
        self.append_result(f"[dim]Last run totals:[/dim] {passed} passed, {failed} failed, {total} total")
        if self.last_report_path:
            self.append_result(f"[dim]Failure txt:[/dim] {self.last_report_path}")
        self.append_result(f"[dim]Coverage txt:[/dim] {self._feature_coverage_path()}")
        self.update_summary(f"Status · {passed}/{total} passed" if total else "Status")

    def show_fails(self):
        failed = [r for r in self.records if r.status == TestStatus.FAIL]
        if not self.records:
            self.append_result("[bold #f59e0b]No run data available yet.[/bold #f59e0b]")
            self.update_summary("No runs yet")
            return
        if not failed:
            self.append_result("[bold #22c55e]No failed tests in the last run.[/bold #22c55e]")
            if self.last_report_path:
                self.append_result(f"[dim]Failure txt:[/dim] {self.last_report_path}")
            self.append_result(f"[dim]Coverage txt:[/dim] {self._feature_coverage_path()}")
            self.update_summary("No failures")
            return

        self.append_result("[bold #e07a53]Failed tests[/bold #e07a53]")

        feature_fails = {}
        for record in failed:
            feat = record.feature or "unknown"
            if feat not in feature_fails:
                feature_fails[feat] = []
            feature_fails[feat].append(record)

        for feat, records in sorted(feature_fails.items()):
            self.append_result(f"[bold #f87171]{feat}:[/bold #f87171] {len(records)} failures")
            for record in records:
                self.append_result(f"  [#f87171]✕[/#f87171] {record.label}")

        self.update_summary(f"{len(failed)} failures")

    def show_report(self):
        failed = [r for r in self.records if r.status == TestStatus.FAIL]
        if not self.records:
            self.append_result("[bold #f59e0b]No run data available yet.[/bold #f59e0b]")
            self.update_summary("No runs yet")
            return
        if not failed:
            self.append_result("[bold #22c55e]No failed tests in the last run.[/bold #22c55e]")
            self.update_summary("No failures")
            return

        self.append_result("[bold #e07a53]Failure Report[/bold #e07a53]")
        self.append_result("")

        feature_fails = {}
        for record in failed:
            feat = record.feature or "unknown"
            if feat not in feature_fails:
                feature_fails[feat] = []
            feature_fails[feat].append(record)

        for feat, records in sorted(feature_fails.items()):
            self.append_result(f"[bold #f87171]{feat}:[/bold #f87171]")
            for record in records:
                self.append_result(f"  [#f87171]✕[/#f87171] {record.label}")
                if record.check_info:
                    self.append_result(f"     Check: {record.check_info}")
                output_lines = (record.output or "").split("\n")[:3]
                for line in output_lines:
                    self.append_result(f"     {line}")
                if len((record.output or "").splitlines()) > 3:
                    self.append_result(f"     [dim]... output truncated ...[/dim]")
            self.append_result("")

        self.update_summary(f"Report · {len(failed)} failures")

    def show_coverage(self):
        path = self._feature_coverage_path()
        self.append_result("[bold #e07a53]Feature Coverage[/bold #e07a53]")
        self.append_result(f"[dim]Report:[/dim] {path}")
        missing = self._feature_coverage_missing()
        if not missing:
            self.append_result("[bold #22c55e]No expected feature gaps in current run.[/bold #22c55e]")
        else:
            for program, gaps in missing.items():
                self.append_result(f"[bold #f59e0b]{program}:[/bold #f59e0b] {', '.join(gaps[:20])}")
        if self.adversarial_report_store:
            self.append_result(f"[dim]Adversarial run:[/dim] {self.adversarial_report_store.root}")
            self.append_result(f"[dim]Replay index:[/dim] {self.adversarial_report_store.index_path}")
        self.update_summary("Coverage")

    def run_all_tests(self):
        def run():
            if getattr(self, 'plain_mode', False):
                emit = self.append_result
            else:
                emit = lambda *args, **kwargs: self.call_from_thread(self.append_result, *args, **kwargs)
            self._show_chaos_feature_gaps = False
            emit(
                "[bold #e07a53]▶ Running tests...[/bold #e07a53]",
            )
            emit(
                f"[dim]Streaming live results from {self.current_program} · all[/dim]",
            )

            programs = [
                ("Algebra", self.run_algebra),
                ("Trigonometry", self.run_trig),
                ("Derive", self.run_derive),
                ("Integrate", self.run_integrate),
                ("Stats", self.run_stats),
                ("SUVAT", self.run_suvat),
                ("Boolean", self.run_boolean),
            ]

            if self.current_program != "all":
                programs = [(name, func) for name, func in programs if name == self.current_program]

            for name, func in programs:
                emit("")
                emit(
                    f"[bold #e07a53]▶ {name}[/bold #e07a53]",
                )
                emit(
                    "[dim]────────────────────────────────────────[/dim]",
                )
                func("all")
            self.transition_run_state(RunState.STOPPED)
            if getattr(self, 'plain_mode', False):
                self.render_summary()
            else:
                self.call_from_thread(self.render_summary)

        import threading
        if getattr(self, 'plain_mode', False):
            run()
            return
        threading.Thread(target=run, daemon=True).start()

    def append_result(self, text):
        def plain_text(value):
            plain = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", value or "")
            for tag in (
                "[bold #e07a53]", "[/bold #e07a53]",
                "[bold #22c55e]", "[/bold #22c55e]",
                "[bold #f87171]", "[/bold #f87171]",
                "[bold #f59e0b]", "[/bold #f59e0b]",
                "[#22c55e]", "[/#22c55e]",
                "[#f87171]", "[/#f87171]",
                "[dim]", "[/dim]",
                "[bold]", "[/bold]",
            ):
                plain = plain.replace(tag, "")
            return plain

        if getattr(self, 'plain_mode', False):
            plain = plain_text(text)
            if plain.strip():
                print(plain, flush=True)
            return
        # Use print() for immediate output, bypass Textual buffering
        import sys
        plain = plain_text(text)
        sys.stdout.write(plain + "\n")
        sys.stdout.flush()
        try:
            self.query_one("#results", RichLog).write(text)
        except Exception:
            pass

    def add_test(self, label, passed, output, program, input_text="", check_info="", feature="", graph_node="", graph_meta=None):
        icon = "●" if passed else "✕"
        color = "#22c55e" if passed else "#f87171"

        status = TestStatus.PASS if passed else TestStatus.FAIL
        record = TestRecord(
            len(self.records) + 1, label, status, output, program, input_text, check_info, feature, passed=passed, harness_status=status
        )
        record.graph_node = graph_node or ""
        record.graph_meta = graph_meta
        self.records.append(record)
        self._live_program = program or self._live_program
        self._last_event_label = label
        self._live_phase = "verifying" if self.llm_enabled_for_tests else "testing"
        if feature:
            st = self._feature_stats.setdefault(feature, [0, 0])
            st[1] += 1
            if passed:
                st[0] += 1

        self.append_result(f"[{color}]{icon}[/{color}] {label}")
        
        if getattr(self, 'plain_mode', False) and not passed:
            if input_text:
                self.append_result("[dim]Input:[/dim] {}".format(input_text.replace("\n", "\\n")))
            if check_info:
                self.append_result("[dim]Check:[/dim] {}".format(check_info))
            excerpt = "\n".join((output or "").splitlines()[:10])
            if excerpt:
                self.append_result("[dim]Output:[/dim]\n{}".format(excerpt))

        passed_count = sum(1 for r in self.records if r.status == TestStatus.PASS)
        total = len(self.records)
        pct = (passed_count / total * 100) if total > 0 else 0

        if self.start_time is not None:
            import time
            elapsed = time.time() - self.start_time
            total = len(self.records)
            remaining_tests = max(0, self.total_expected - total)

            rate = 0.0
            eta_seconds = 0.0
            if elapsed > 0 and total > 0:
                current_rate = total / elapsed
                decay = self._eta_decay
                self._weighted_rate = decay * self._weighted_rate + (1 - decay) * current_rate if self._weighted_rate > 0 else current_rate

                self._test_timestamps.append((elapsed, total))
                if len(self._test_timestamps) > 100:
                    self._test_timestamps = self._test_timestamps[-50:]

                test_time_variance = 0.0
                if len(self._test_times) >= 10:
                    import statistics
                    recent_times = self._test_times[-50:]
                    avg_time = statistics.mean(recent_times)
                    stdev_time = statistics.stdev(recent_times) if len(recent_times) > 2 else 0
                    time_variance = stdev_time * 2
                else:
                    time_variance = 0.0

                if self._weighted_rate > 0:
                    base_eta = remaining_tests / self._weighted_rate
                    eta_seconds = base_eta + time_variance * remaining_tests

            llm_remaining = 0.0
            if self.llm_enabled_for_tests and self.llm_manager and self.llm_manager.enabled:
                llm_to_verify = remaining_tests
            else:
                llm_to_verify = 0
            if llm_to_verify > 0:
                llm_avg = getattr(self, '_llm_avg_time', 0.0)
                llm_estimated = getattr(self, '_llm_estimated_per_test', 0.0)
                llm_weight = llm_avg if llm_avg > 0 else llm_estimated

                if self._llm_times:
                    current_llm_rate = len(self._llm_times) / sum(self._llm_times) if sum(self._llm_times) > 0 else 0
                    decay = self._eta_decay
                    self._weighted_llm_rate = decay * self._weighted_llm_rate + (1 - decay) * current_llm_rate if self._weighted_llm_rate > 0 else current_llm_rate
                    if self._weighted_llm_rate > 0:
                        llm_remaining = llm_to_verify / self._weighted_llm_rate

            total_eta = max(0, eta_seconds + llm_remaining)

            if total_eta < 60:
                eta_str = f"{total_eta:.0f}s"
            elif total_eta < 3600:
                eta_str = f"{total_eta/60:.1f}m"
            else:
                eta_str = f"{total_eta/3600:.1f}h"

            rate_str = f"{self._weighted_rate:.0f}/s"
            if llm_remaining > 0:
                rate_str += f" +LLM({self._weighted_llm_rate:.1f}/s)"
            self.update_summary(f"{passed_count}/{total} ({pct:.0f}%) · {rate_str} · ETA {eta_str}")
        elif self.total_expected > 0:
            self.update_summary(f"{passed_count}/{total} ({pct:.0f}%)")
        else:
            self.update_summary(f"Running... {passed_count}/{total} passed ({pct:.0f}%)")
        self._bump_tui_progress_if_random()
        if not self._in_run_case_specs:
            self._append_session_report_if_worthy(record)
        return record

    def random_graph_status_for_record(self, record):
        if record.status == TestStatus.PASS and not record.review_needed:
            return "pass"
        note = " ".join(
            [
                record.check_info or "",
                record.llm_verdict or "",
                record.llm_explanation or "",
            ]
        ).lower()
        if "exam-quality" in note or "weak" in note or "needs_review" in note:
            return "weak"
        return "fail"

    def record_random_graph_result(self, record):
        key = getattr(record, "graph_node", "") or ""
        if not key:
            return
        self.random_graph.record(
            key,
            getattr(record, "graph_meta", None) or {},
            self.random_graph_status_for_record(record),
            record.input_text,
            record.output,
        )

    def flush_random_graph(self):
        self.random_graph.save()

    def apply_llm_weighted_status(self, record, final_verdict):
        """
        Keep hard runtime/parser failures authoritative, but let the examiner
        LLM clear heuristic checker false positives.  Those cleared cases stay
        review-visible instead of polluting the fail count.
        """
        out = (record.output or "").lower()
        fatal = any(
            marker in out
            for marker in (
                "parser error",
                "traceback",
                "timeout after",
                "dimension mismatch",
                "err:",
                "segmentation fault",
            )
        )
        if record.harness_status == TestStatus.FAIL and final_verdict == "CORRECT" and not fatal:
            record.status = TestStatus.PASS
            record.passed = True
            record.review_needed = True
            return
        record.review_needed = (
            record.harness_status == TestStatus.PASS and final_verdict == "NEEDS_REVIEW"
        )
        record.status = record.harness_status
        record.passed = record.harness_status == TestStatus.PASS

    def _wants_json_export(self):
        if getattr(self, "_force_export_json", False):
            return True
        return os.environ.get("CASIO_TEST_EXPORT_JSON", "").lower() in ("1", "true", "yes")

    def _write_json_report(self, path: Path):
        payload = {
            "generated": datetime.now().isoformat(timespec="seconds"),
            "last_command": self.last_command,
            "current_program": self.current_program,
            "last_run_scope": [self.last_run_scope[0], self.last_run_scope[1]] if self.last_run_scope is not None else None,
            "records": [
                {
                    "test_id": r.test_id,
                    "label": r.label,
                    "status": r.status.name,
                    "harness_status": r.harness_status.name,
                    "passed": r.passed,
                    "review_needed": getattr(r, "review_needed", False),
                    "program": r.program,
                    "feature": r.feature,
                    "check_info": r.check_info,
                    "llm_verdict": (getattr(r, "llm_verdict", "") or ""),
                    "llm_explanation": (getattr(r, "llm_explanation", "") or ""),
                    "input_text": r.input_text,
                    "output": r.output,
                }
                for r in self.records
            ],
        }
        path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    def _feature_coverage_path(self):
        if getattr(self, "backend", "python") == "c":
            base = REPO_ROOT / "c++" / "tests" / "reports"
            scope = (self.last_run_scope[0] if self.last_run_scope else "") or ""
            if scope.startswith("stress:"):
                name = re.sub(r"[^a-z0-9_]+", "_", scope[len("stress:"):].lower()).strip("_") or "all"
                return base / f"feature_coverage_stress_{name}_latest.txt"
            return base / "feature_coverage_random_latest.txt"
        return REPO_ROOT / "python" / "tests" / "reports" / "feature_coverage_latest.txt"

    @staticmethod
    def _covers_feature(expected_id, feature_keys):
        for feat in feature_keys:
            if expected_id.endswith("_") and feat.startswith(expected_id):
                return True
            if feat == expected_id or feat.startswith(expected_id + ":"):
                return True
        return False

    def _feature_coverage_missing(self):
        st = getattr(self, "_feature_stats", None) or {}
        fkeys = set(st.keys())
        missing = {}
        scope = None if self.current_program == "all" else self.current_program
        for program, expected in FEATURE_PARITY_EXPECTED.items():
            if scope is not None and program != scope:
                continue
            gaps = [item for item in expected if not self._covers_feature(item, fkeys)]
            if gaps:
                missing[program] = gaps
        return missing

    def _write_feature_coverage_report(self):
        if not self.records:
            return
        path = self._feature_coverage_path()
        path.parent.mkdir(parents=True, exist_ok=True)
        st = getattr(self, "_feature_stats", None) or {}
        missing = self._feature_coverage_missing()
        failed = [r for r in self.records if r.status == TestStatus.FAIL]
        review = [r for r in self.records if getattr(r, "review_needed", False)]
        lines = [
            "CasioCAS feature coverage audit",
            "Generated: {0}".format(datetime.now().isoformat(timespec="seconds")),
            "Backend: {0}".format(self.backend_label()),
            "Command: {0}".format(self.last_command),
            "Scope: {0}".format(self.current_program),
            "Records: {0}".format(len(self.records)),
            "Failures: {0}".format(len(failed)),
            "LLM review needed: {0}".format(len(review)),
            "",
            "Graphiffy:",
            "graph TD",
            "  Python[Python feature set] --> Harness[run_tests generators]",
            "  Harness --> SymPy[SymPy/manual checks]",
            "  Harness --> Host[C++ host/add-in]",
            "  Host --> Reports[failure_report_latest.txt + feature_coverage_latest.txt]",
            "  LLM[optional LLM verifier] --> Reports",
            "",
            "Strategy:",
            "- /random and /infinite allocate tests across feature generators.",
            "- Cases are de-duplicated per run unless infinite mode needs fresh pressure.",
            "- SymPy/manual checkers verify algebraic/probability/kinematic answers where possible.",
            "- LLM verification, if enabled with /llm, logs disagreements as review items.",
            "- failure_report_latest.txt stores failing input, checker, output tail, and LLM notes.",
            "",
            "Expected parity:",
        ]
        fkeys = set(st.keys())
        for program, expected in FEATURE_PARITY_EXPECTED.items():
            if self.current_program != "all" and program != self.current_program:
                continue
            lines.append("")
            lines.append("[{0}] {1}".format(program, FEATURE_PARITY_NOTES.get(program, "")))
            for item in expected:
                hit = self._covers_feature(item, fkeys)
                ok = total = 0
                for feat, counts in st.items():
                    if feat == item or feat.startswith(item + ":"):
                        ok += counts[0]
                        total += counts[1]
                marker = "OK" if hit else "GAP"
                lines.append("- {0} {1} {2}/{3}".format(marker, item, ok, total))
        if st:
            lines.append("")
            lines.append("Observed feature tags:")
            for feat, counts in sorted(st.items()):
                lines.append("- {0}: {1}/{2}".format(feat, counts[0], counts[1]))
        if missing:
            lines.append("")
            lines.append("Missing coverage:")
            for program, gaps in sorted(missing.items()):
                lines.append("- {0}: {1}".format(program, ", ".join(gaps)))
        body = "\n".join(lines) + "\n"
        path.write_text(body, encoding="utf-8")
        if getattr(self, "backend", "python") == "c":
            latest = REPO_ROOT / "c++" / "tests" / "reports" / "feature_coverage_latest.txt"
            latest.write_text(body, encoding="utf-8")

    def render_summary(self):
        passed = sum(1 for r in self.records if r.status == TestStatus.PASS)
        total = len(self.records)
        pct = (passed / total * 100) if total > 0 else 0

        elapsed_str = ""
        if self.start_time is not None:
            import time
            elapsed = time.time() - self.start_time
            if elapsed < 60:
                elapsed_str = f" · {elapsed:.0f}s"
            elif elapsed < 3600:
                elapsed_str = f" · {elapsed/60:.1f}m"
            else:
                elapsed_str = f" · {elapsed/3600:.1f}h"

        if pct == 100:
            msg = f"● All tests passed · {passed}/{total}{elapsed_str}"
            color = "#22c55e"
        else:
            msg = f"● {passed}/{total} passed ({pct:.0f}%){elapsed_str}"
            color = "#fbbf24"

        if self.llm_enabled_for_tests and self.llm_manager and self.llm_manager.enabled:
            llm_count = sum(1 for r in self.records if r.llm_verdict == "CORRECT")
            llm_total = sum(1 for r in self.records if r.llm_verdict)
            review_total = sum(1 for r in self.records if getattr(r, "review_needed", False))
            if llm_total > 0:
                msg += f" · LLM: {llm_count}/{llm_total} verified"
                if review_total > 0:
                    msg += f" · review {review_total}"
                cache_stats = self.llm_manager.cache.stats()
                if cache_stats.get("hits", 0) > 0:
                    msg += f" ({cache_stats['hit_rate']} cache)"

        self.append_result(f"\n[bold]Results:[/bold] [bold {color}]{msg}[/bold {color}]\n")
        self._append_feature_weakness_and_gaps()
        try:
            self._write_feature_coverage_report()
            self.append_result(f"[dim]Coverage txt:[/dim] {self._feature_coverage_path()}")
        except Exception:
            pass
        if self._wants_json_export() and self.records:
            try:
                self._write_json_report(self._session_report_path().parent / "failure_report_latest.json")
            except Exception:
                pass
        self.update_summary(msg)

    def _append_feature_weakness_and_gaps(self):
        st = getattr(self, "_feature_stats", None) or {}
        if not st:
            return
        rows = []
        for feat, (ok, n) in st.items():
            if n <= 0:
                continue
            rows.append((ok / n, n, ok, feat))
        rows.sort()
        if rows:
            self.append_result("[dim]Weakest feature tags (pass rate, at least 2 tests):[/dim]")
            shown = 0
            for rate, n, ok, feat in rows:
                if n < 2:
                    continue
                if rate >= 0.999:
                    continue
                self.append_result(f"  [dim]{feat}[/dim] {ok}/{n} ({100*rate:.0f}%)")
                shown += 1
                if shown >= 10:
                    break
            if shown == 0:
                self.append_result("  [dim](all multi-run tags at 100% or single-run only)[/dim]")

        fkeys = st.keys()
        if getattr(self, "_show_chaos_feature_gaps", False) and fkeys and len(self.records) > 5:
            missing = []
            for program, gaps in self._feature_coverage_missing().items():
                missing.extend("{0}:{1}".format(program, gap) for gap in gaps)
            if missing:
                self.append_result(
                    f"[dim]Chaos run did not hit these feature tags: {', '.join(missing[:12])}[/dim]"
                )

    def update_summary(self, text):
        self._summary_text = text
        try:
            self.query_one("#status-line", Static).update(text)
            self.screen.refresh()  # Force immediate display
        except Exception:
            pass

    def render_live_frame(self):
        if getattr(self, "plain_mode", False):
            return
        try:
            if not self.running:
                return
            frame = self._spinner_frames[self._spinner_index % len(self._spinner_frames)]
            self._spinner_index += 1
            passed = sum(1 for r in self.records if r.status == TestStatus.PASS)
            failed = sum(1 for r in self.records if r.status == TestStatus.FAIL)
            total = len(self.records)
            expected = self.total_expected if self.total_expected > 0 else total
            program = self._live_program or self.current_program or "all"
            phase = self._live_phase or "running"
            tail = self._last_event_label
            if tail:
                tail = " · " + tail[:72]
            status = (
                frame + " " + phase + " · " + program +
                " · " + str(total) + "/" + str(expected) +
                " · pass " + str(passed) + " fail " + str(failed) + tail
            )
            self.query_one("#status-line", Static).update(status)
            if expected > 0:
                self.update_progress_bar(total, expected, frame)
            self.screen.refresh()
        except Exception:
            pass

    def update_progress_bar(self, current, total, frame=""):
        if total <= 0:
            total = current
        if total <= 0:
            self._hide_progress_bar()
            return
        
        pct = current / total
        bar_width = 20
        filled = int(pct * bar_width)
        bar = "█" * filled + "░" * (bar_width - filled)
        pct_int = int(pct * 100)
        
        try:
            lead = (frame + " ") if frame else ""
            self.query_one("#progress-bar", Static).update(
                f"{lead}[#22c55e]{bar}[/] {pct_int}% ({current}/{total})"
            )
            self.screen.refresh()
        except Exception:
            pass
    
    def _hide_progress_bar(self):
        try:
            self.query_one("#progress-bar", Static).update("")
        except Exception:
            pass

    def _bump_tui_progress_if_random(self):
        """Keep the /random and /infinite status bar in sync (TUI only, not plain/CLI)."""
        if getattr(self, "plain_mode", False) or not self.running:
            return
        t = int(self.total_expected) if self.total_expected else 0
        if t <= 0:
            return
        c = len(self.records)
        if c < 1:
            return
        step = max(1, t // 200)
        if c != 1 and c != t and c % step != 0:
            return
        self.call_from_thread(self.update_progress_bar, c, t)

    def update_running_count(self, current, total):
        if total > 0:
            pct = int(current / total * 100)
            self.update_summary(f"Running: {current}/{total} tests ({pct}%)")
        else:
            self.update_summary(f"Running: {current} tests...")

    def update_command_help(self, value: str):
        help_widget = self.query_one("#command-help", Static)
        suggestions_widget = self.query_one("#command-suggestions", Static)

        if not value:
            help_widget.update("? for shortcuts")
            suggestions_widget.update("")
            return

        if not value.startswith("/"):
            help_widget.update("Commands start with /")
            suggestions_widget.update("")
            return

        value_lower = value.lower()
        matches = [(cmd, desc) for cmd, desc in self.command_items.items() if cmd.lower().startswith(value_lower)]

        if not matches:
            help_widget.update("No matching commands")
            suggestions_widget.update("")
            return

        help_widget.update(matches[0][0])
        suggestions = "\n".join(f"[dim]{cmd}[/dim]  {desc}" for cmd, desc in matches)
        suggestions_widget.update(suggestions)

    def run_cli(self, script, inp):
        host = REPO_ROOT / "c++" / "addin" / "host" / "build" / "casio_host"
        if not host.exists():
            return ("", f"Missing host binary: {host}\nBuild with ./c++/tools/build_host.sh")
        try:
            proc = subprocess.run(
                [str(host), "--stdin-program", script],
                input=inp,
                text=True,
                capture_output=True,
                cwd=str(REPO_ROOT),
                timeout=CASE_TIMEOUT_SECONDS if CASE_TIMEOUT_SECONDS > 0 else None,
            )
        except subprocess.TimeoutExpired as err:
            stdout = err.stdout if isinstance(err.stdout, str) else ""
            stderr = err.stderr if isinstance(err.stderr, str) else ""
            return stdout, f"{stderr}\nTimeout after {CASE_TIMEOUT_SECONDS:g}s".strip()
        return proc.stdout, proc.stderr

    def run_host_feature(self, flag, expr):
        host = REPO_ROOT / "c++" / "addin" / "host" / "build" / "casio_host"
        if not host.exists():
            return "", f"Missing host binary: {host}\nBuild with ./c++/tools/build_host.sh"
        try:
            proc = subprocess.run(
                [str(host), f"--{flag}", expr],
                text=True,
                capture_output=True,
                cwd=str(REPO_ROOT),
                timeout=CASE_TIMEOUT_SECONDS if CASE_TIMEOUT_SECONDS > 0 else None,
            )
        except subprocess.TimeoutExpired as err:
            stdout = err.stdout if isinstance(err.stdout, str) else ""
            stderr = err.stderr if isinstance(err.stderr, str) else ""
            return stdout, f"{stderr}\nTimeout after {CASE_TIMEOUT_SECONDS:g}s".strip()
        return proc.stdout, proc.stderr

    def run_host_expr(self, expr):
        host = REPO_ROOT / "c++" / "addin" / "host" / "build" / "casio_host"
        if not host.exists():
            return "", f"Missing host binary: {host}\nBuild with ./c++/tools/build_host.sh"
        try:
            proc = subprocess.run(
                [str(host), expr],
                text=True,
                capture_output=True,
                cwd=str(REPO_ROOT),
                timeout=CASE_TIMEOUT_SECONDS if CASE_TIMEOUT_SECONDS > 0 else None,
            )
        except subprocess.TimeoutExpired as err:
            stdout = err.stdout if isinstance(err.stdout, str) else ""
            stderr = err.stderr if isinstance(err.stderr, str) else ""
            return stdout, f"{stderr}\nTimeout after {CASE_TIMEOUT_SECONDS:g}s".strip()
        return proc.stdout, proc.stderr

    def make_cli_case(self, program, script, inp, label, checker, check_info="", feature="", use_calculated=None):
        if use_calculated is None:
            use_calculated = not limited_by(CALCULATED_CHECK_MAX_INPUT_CHARS, len(inp or ""))

        def combined_checker(output):
            if not bool(checker(output)):
                return False
            if not use_calculated:
                return True
            calculated = self.calculated_checker_for_cli_case(script, inp, checker)
            if calculated is None or calculated is checker:
                return True
            return bool(calculated(output))

        def runner():
            out, err = self.run_cli(script, inp)
            combined = out if not err else out + "\n" + err
            return combined_checker(combined), combined

        info = check_info or getattr(checker, "__name__", "custom checker")
        if use_calculated:
            info = f"{info}; calculated answer correctness"
        return CaseSpec(label, program, runner, inp, '', info, feature, script, combined_checker, "", None, use_calculated)

    def make_direct_case(self, program, label, runner, input_text="", check_info="", feature=""):
        return CaseSpec(label, program, runner, input_text=input_text, check_info=check_info, feature=feature)

    def case_question_key(self, case):
        if getattr(case, "graph_node", ""):
            return "GRAPH::{0}::{1}".format(case.graph_node, (case.input_text or "").strip())
        text = (case.input_text or "").strip()
        if text:
            return f"{case.program}::{text}"
        return f"{case.program}::LABEL::{case.label}"

    def write_catalogue_manifest(self, funcs):
        CATALOGUE_MANIFEST_PATH.parent.mkdir(parents=True, exist_ok=True)
        lines = [
            "All catalogue functions: {0}".format(len(funcs)),
            "Random math tokens: {0}".format(", ".join(RANDOM_SUPPORTED_MATH_THINGS)),
            "",
        ]
        for fn in funcs:
            params = []
            for p in fn.params:
                tag = "opt" if p.get("optional") else "req"
                params.append("{0}:{1}".format(p.get("name", "?"), tag))
            lines.append("{0}  params=[{1}]".format(fn.signature, ", ".join(params) if params else "none"))
        CATALOGUE_MANIFEST_PATH.write_text("\n".join(lines) + "\n", encoding="utf-8")

    def catalogue_combo_indices(self, fn):
        required = [i for i, p in enumerate(fn.params) if not p.get("optional")]
        optional = [i for i, p in enumerate(fn.params) if p.get("optional")]
        combos = [tuple(required)]
        if optional:
            combos.append(tuple(required + optional[:1]))
            combos.append(tuple(required + optional))
        seen = []
        for combo in combos:
            if combo not in seen:
                seen.append(combo)
        return seen or [tuple()]

    def expression_math_shapes(self, include_factorial=True):
        if include_factorial:
            return RANDOM_SUPPORTED_MATH_SHAPES
        return tuple(shape for shape in RANDOM_SUPPORTED_MATH_SHAPES if "factorial" not in shape)

    def catalogue_shapes_for(self, name):
        generic_math = self.expression_math_shapes(include_factorial=True)
        calculus_math = self.expression_math_shapes(include_factorial=False)
        if name in ("diff", "implicit_diff", "param_diff"):
            return ("poly", "chain", "product", "quotient", "implicit", "param") + calculus_math
        if name in ("integrate", "int"):
            return ("direct", "reverse_chain", "sub", "parts", "di", "trig", "pf", "div", "weierstrass", "symmetry", "manip_trig", "manip_rational") + calculus_math
        if name in ("solve", "fsolve", "zeros"):
            return ("linear", "quad", "factor", "complete_square", "log_exp", "rational", "interval", "hidden_form") + generic_math
        if name in ("solve_trig", "trigsolve"):
            return ("general", "bounded", "cast", "identity", "rform", "square_then_check", "sum_to_product", "hidden_factor")
        if name in ("trig_prove", "trig_transform", "trig_rewrite"):
            return ("pythag", "double_angle", "compound_angle", "target", "hidden_identity")
        if name.startswith("trig") or name in ("sin", "cos", "tan", "sec", "cosec", "cot"):
            return ("sin_cos", "pythag", "double_angle", "compound_angle", "target", "manip_trig")
        if name in ("domain", "range"):
            return ("poly", "rational", "radical", "log", "trig", "interval", "hidden_identity") + calculus_math
        if name in ("binomial", "normal", "poisson", "correlation", "covariance", "regression", "mean", "median", "quartiles", "stddev"):
            return ("summary", "regression", "binomial", "normal", "poisson", "edge")
        if name in ("det", "inverse", "rank", "rref", "eigenvals", "eigenvects", "dot", "cross"):
            return ("matrix2", "matrix3", "singular", "vector")
        if name == "complete_square":
            return ("complete_square", "quad", "poly", "letter")
        if name == "partfrac":
            return ("pf", "rational")
        if name == "factor":
            return ("factorable_poly", "hidden_form")
        if name in ("expand", "collect"):
            return ("poly", "quartic", "letter", "binomial_poly", "target")
        if name in ("coeff", "degree"):
            return ("poly", "quartic", "letter")
        if name in ("binom_expand", "binom_coeff"):
            return ("binomial_poly", "binomial_radical", "binomial_negative")
        if name in ("arg", "conj", "re", "im", "abs"):
            return ("real", "complex_rect", "complex_polar") + generic_math
        return ("basic", "nested", "edge") + generic_math

    def catalogue_testable_function(self, fn):
        name = fn.name
        # Host parser intentionally mirrors the compact working layer, not full
        # KhiCAS function-call/list syntax.  Skip raw multi-arg/list commands here
        # so /random hunts maths-working faults instead of parser-surface noise.
        if name in ("coeff", "dot", "cross", "inverse", "rank"):
            return False
        direct = {
            "diff", "implicit_diff", "param_diff", "integrate", "int",
            "solve", "fsolve", "zeros", "solve_trig", "trigsolve", "domain", "range",
            "factor", "expand", "partfrac", "complete_square", "collect", "coeff",
            "binomial", "normal", "poisson", "correlation", "covariance", "mean", "median",
            "quartiles", "stddev", "det", "inverse", "rank", "rref", "dot", "cross",
            "trig_prove", "trig_transform", "trig_rewrite",
        }
        if name in direct or name.startswith("trig"):
            return True
        return False

    def random_catalogue_expr(self, rng, shape):
        if shape.startswith("math_"):
            return self.random_supported_math_expr(rng, shape)
        pool = {
            "poly": ["x^2-5*x+6", "3*x^3-2*x+7", "(x-1)^2+4*x"],
            "factorable_poly": ["x^2-5*x+6", "x^2-1", "x^3-6*x^2+11*x-6", "(x-1)^2+4*x"],
            "chain": ["sin(x^2+1)", "exp(3*x-2)", "log(x^2+1)"],
            "product": ["x^2*exp(x)", "x*sin(x)", "(x^2+1)*log(x)"],
            "quotient": ["(x^2+1)/(x-1)", "sin(x)/(1+cos(x))"],
            "implicit": ["sin(x*y)+x^2=y^2", "x^y=y^x", "ln(x+y)=x*y"],
            "param": ["x=t^2+1/t,y=t^2-1/t,t", "x=exp(t)*cos(t),y=exp(t)*sin(t),t"],
            "second": ["x^3+2*x", "sin(x)^2", "x*exp(x)"],
            "direct": ["x^3-4*x+1", "sin(3*x+2)", "1/(5*x+7)"],
            "reverse_chain": ["2*x*cos(x^2)", "(2*x+5)/(x^2+5*x-7)", "1/(x*log(x))"],
            "sub": ["1/(1+sqrt(x))", "exp(sqrt(x))", "sqrt(x)/(1+x)"],
            "parts": ["x^2*log(x)", "x^2*exp(x)", "exp(2*x)*cos(3*x)"],
            "di": ["x^4*exp(x)", "x^3*sin(2*x)", "x^3*cos(3*x)"],
            "trig": ["sin(x)^4", "tan(x)^3", "sec(x)^3"],
            "pf": ["1/(x^2*(x+1))", "(2*x+5)/(x^2+5*x-7)", "1/(x*(x^2+1))"],
            "div": ["(x^2+1)/(x-1)", "x^4/(x^2+1)", "(x^3+1)/(x+1)"],
            "weierstrass": ["1/(2+cos(x))", "1/(1+sin(x)+cos(x))"],
            "symmetry": ["sin(x)/(sin(x)+cos(x))", "log(sin(x))"],
            "manip_trig": [
                "(tan(x)^2+1)/(sec(x)^2)",
                "(1-cos(2*x))/(sin(x))",
                "((sin(x)+cos(x))^2-2*sin(x)*cos(x))/(x+1)",
                "(cosec(x)^2-cot(x)^2)*exp(x)",
            ],
            "manip_rational": [
                "((x^2-1)/(x-1))/(x+1)",
                "(x^4-1)/(x^2-1)",
                "(x^2+2*x+1)/(x+1)",
            ],
            "linear": ["2*x+3=11", "5*x-7=3*x+9"],
            "quad": ["x^2-5*x+6=0", "2*x^2+3*x-2=0"],
            "factor": ["x^3-6*x^2+11*x-6=0", "sin(x)^2-sin(x)=0"],
            "complete_square": ["x^2+4*x+1=0", "x^2-6*x+5=0"],
            "log_exp": ["2^(2*x)-5*2^x+4=0", "log(x-1)+log(x+3)=log(8)"],
            "rational": ["(x+1)/(x-2)+(x-2)/(x+1)=5", "1/(x-1)+1/(x+2)=1"],
            "interval": ["sin(x)=1/2,x,0,2*pi", "cos(2*x)+sin(2*x)=1,x,0,2*pi"],
            "general": ["2*sin(x)+1=0", "cos(2*x)=1/2"],
            "bounded": ["2*sin(x)+1=0,x,0,2*pi,8", "3*cos(x)+4*sin(x)=2,x,0,2*pi,8"],
            "cast": ["sin(x)=-1/2", "tan(2*x)=sqrt(3)"],
            "identity": ["sin(x)^2+cos(x)^2", "(1-tan(x)^2)/(1+tan(x)^2)", "1/cos(x)=5,x,0,2*pi,8"],
            "rform": ["3*cos(x)+4*sin(x)=2", "5*sin(x)-12*cos(x)=6"],
            "square_then_check": ["sqrt(1-cos(x))=sin(x)", "sin(x)+cos(x)=sqrt(2)"],
            "sum_to_product": ["sin(3*x)-sin(x)=0", "cos(3*x)-cos(x)=0", "sin(5*x)-sin(x)=0"],
            "hidden_factor": ["2*sin(x)^2=1+cos(x)", "cos(2*x)+sin(2*x)=1"],
            "hidden_form": ["(x+1)^2-(x-1)^2=8", "(x^2-1)/(x-1)=4", "2^(2*x)-5*2^x+4=0"],
            "sin_cos": ["tan(x)+cot(x)", "sec(x)^2-tan(x)^2"],
            "pythag": ["sin(x)^2+cos(x)^2", "1+tan(x)^2"],
            "double_angle": ["sin(x)^4", "cos(4*x)"],
            "compound_angle": ["sin(x+y)", "cos(x-y)"],
            "target": ["sin(x)^2+cos(x)^2", "x^2+a*x+b"],
            "binomial_poly": ["(1+x)^5", "(2*x-3)^4", "(a+b)^6"],
            "binomial_radical": ["(1+x)^(1/2)", "(1-2*x)^(-1/2)", "(4+x)^(1/2)"],
            "binomial_negative": ["(1+x)^(-3)", "(1-3*x)^(-2)"],
            "radical": ["sqrt(x-sqrt(x))", "sqrt(1-x^2)"],
            "log": ["log(1-x^2)", "log(sin(x))"],
            "matrix2": ["[[1,2],[3,5]]", "[[2,1],[1,3]]"],
            "matrix3": ["[[1,2,3],[0,1,4],[5,6,0]]"],
            "singular": ["[[1,2],[2,4]]"],
            "vector": ["[1,2,3]", "[3,-1,2]"],
            "quartic": ["x^4+1", "x^4+x^2+1"],
            "letter": ["a*x^2+b*x+c", "(m*x+n)^2"],
            "real": ["-7", "sqrt(5)^2"],
            "complex_rect": ["3+4*i", "1-2*i"],
            "complex_polar": ["2*exp(i*pi/3)"],
            "nested": ["sin(log(x^2+1))", "sqrt(1+exp(x))"],
            "edge": ["1/(x-1)", "sqrt(x^2)", "log(x^2)"],
            "basic": ["x^2+1", "sin(x)", "2/3"],
        }
        return rng.choice(pool.get(shape, pool["basic"]))

    def random_supported_math_expr(self, rng, shape, var="x"):
        token = shape[5:] if shape.startswith("math_") else shape
        angle = rng.choice([
            var,
            f"2*{var}+pi/6",
            f"({var})^2-pi/4",
            f"({var}+1)/3",
        ])
        affine = rng.choice([f"2*{var}-3", f"{var}+5", f"3*{var}+1"])
        positive = rng.choice([
            f"abs({affine})+2",
            f"({var})^2+1",
            f"sqrt(({affine})^2+4)+1",
        ])
        unit = rng.choice([f"{var}/2", f"({var}-1)/3", f"sin({angle})"])
        table = {
            "sin": [f"sin({angle})", f"sin({angle})^2+cos({angle})^2"],
            "cos": [f"cos({angle})", f"1-cos({angle})^2"],
            "tan": [f"tan({angle})", f"tan({angle})^2+1"],
            "sec": [f"sec({angle})", f"sec({angle})^2-tan({angle})^2"],
            "cot": [f"cot({angle})", f"cot({angle})^2+1"],
            "cosec": [f"cosec({angle})", f"cosec({angle})^2-cot({angle})^2"],
            "csc": [f"csc({angle})", f"csc({angle})^2-cot({angle})^2"],
            "abs": [f"abs({affine})", f"sqrt(abs({affine})+1)"],
            "sqrt": [f"sqrt({positive})", f"sqrt(({affine})^2)"],
            "ln": [f"ln({positive})", f"ln(abs({var})+2)"],
            "log": [f"log({positive})", f"log(exp({affine})+3)"],
            "log_base": [f"log(2,{positive})", f"log(10,abs({affine})+3)", f"log(3,({var})^2+2)"],
            "exp": [f"exp({affine})", f"exp(log({positive}))"],
            "asin": [f"asin({unit})", f"asin(sin({angle}))"],
            "acos": [f"acos({unit})", f"acos(cos({angle}))"],
            "atan": [f"atan({affine})", f"atan(tan({angle}))"],
            "arcsin": [f"arcsin({unit})", f"arcsin(sin({angle}))"],
            "arccos": [f"arccos({unit})", f"arccos(cos({angle}))"],
            "arctan": [f"arctan({affine})", f"arctan(tan({angle}))"],
            "sign": [f"sign({affine})", f"sign(sin({angle}))"],
            "factorial_bang": [f"{rng.randint(3, 8)}!", f"({rng.randint(2, 5)}+{rng.randint(1, 4)})!"],
            "factorial_fn": [f"factorial({rng.randint(3, 8)})", f"factorial({rng.randint(2, 5)}+{rng.randint(1, 4)})"],
        }
        return rng.choice(table.get(token, [var]))

    def random_domain_range_expr(self, rng, shape):
        if shape.startswith("math_"):
            return self.random_supported_math_expr(rng, shape)
        pool = {
            "poly": ["x^2-4*x+1", "(x-2)^2+3", "x^3-3*x"],
            "rational": ["1/(x-1)+1/(x+2)", "(x+1)/(x^2+1)", "x/(1+x^2)"],
            "radical": ["sqrt(x-sqrt(x))", "sqrt(1-x^2)", "sqrt(x^2+1)"],
            "log": ["log(1-x^2)", "ln(abs(x)+2)", "log(2,x+3)"],
            "trig": ["ln(sin(x))", "1/(2-cos(3*x))", "sqrt(cos(sin(x)))"],
            "interval": ["sin(x)^2-4*sin(x)+3", "x/(1+x^2)", "(x^2-x+1)/(x^2+x+1)"],
            "hidden_identity": ["sec(2*x)^2-tan(2*x)^2", "cosec(2*x+pi/6)^2-cot(2*x+pi/6)^2", "(sin(x)+cos(x))^2-2*sin(x)*cos(x)"],
        }
        return rng.choice(pool.get(shape, ["x^2+1"]))

    def random_catalogue_param_triplet(self, rng):
        raw = self.random_catalogue_expr(rng, "param")
        parts = _split_top_level_csv(raw)
        cleaned = []
        for part in parts:
            if "=" in part:
                cleaned.append(part.split("=", 1)[1])
            else:
                cleaned.append(part)
        if len(cleaned) >= 3:
            return ",".join(cleaned[:3])
        return "t^2+1/t,t^2-1/t,t"

    def catalogue_arg_value(self, param, fn_name, shape, rng):
        p = (param or "").strip().lower()
        if fn_name in ("correlation", "covariance", "regression"):
            if p in ("l1", "x", "xs", "data1"):
                return "[1,2,3,4]"
            if p in ("l2", "y", "ys", "data2"):
                return "[2,5,7,11]"
        if p in ("x", "var") or "var" in p:
            return "x"
        if p in ("y",):
            return "y"
        if p in ("t",):
            return "t"
        if fn_name in ("coeff", "degree") and p in ("p", "expr", "f"):
            return self.random_catalogue_expr(rng, rng.choice(["poly", "quartic", "letter"]))
        if fn_name in ("dot", "cross") and p in ("a", "b", "u", "v"):
            return rng.choice(["[1,2,3]", "[3,-1,2]", "[0,4,-2]"])
        if (fn_name in ("inverse", "rank") and p in ("a", "m")) or "matrix" in p:
            return self.random_catalogue_expr(rng, rng.choice(["matrix2", "matrix3", "singular"]))
        if "method" in p:
            if shape.startswith("math_"):
                return "auto"
            return shape if shape in ("direct", "reverse_chain", "sub", "parts", "trig", "pf", "div", "di", "weierstrass", "symmetry") else "auto"
        if p in ("lo", "a"):
            return "0" if shape in ("bounded", "interval", "trig") else "1"
        if p in ("hi", "b"):
            return "2*pi" if shape in ("bounded", "interval", "trig") else "5"
        if p in ("max", "n", "k") or p.endswith("n") or p.endswith("k"):
            return str(rng.randint(2, 8))
        if p in ("p", "prob"):
            return rng.choice(["0.2", "0.4", "0.5", "0.75"])
        if "list" in p or p in ("l", "l1", "l2", "data"):
            return rng.choice(["[1,2,3,4,8]", "[2,5,7,11]", "[1,1,2,3,5,8]"])
        if p in ("a", "m") or "matrix" in p:
            return self.random_catalogue_expr(rng, rng.choice(["matrix2", "matrix3", "singular"]))
        if p in ("u", "v") or "vector" in p:
            return rng.choice(["[1,2,3]", "[3,-1,2]"])
        if "eq" in p:
            return self.random_catalogue_expr(rng, rng.choice(["linear", "quad", "implicit", "rform"]))
        if "expr" in p or p in ("f", "g", "z"):
            return self.random_catalogue_expr(rng, shape)
        return self.random_catalogue_expr(rng, shape)

    def catalogue_call_for(self, fn, combo, shape, rng):
        if not fn.params:
            return fn.signature
        args = []
        for i in combo:
            if i < len(fn.params):
                args.append(self.catalogue_arg_value(fn.params[i].get("name", ""), fn.name, shape, rng))
        return "{0}({1})".format(fn.name, ",".join(args))

    def catalogue_direct_expr(self, fn, combo, shape, rng):
        name = fn.name
        if name == "!":
            return "", f"{rng.randint(3, 8)}!"
        if name == "diff":
            if shape == "implicit":
                return "derive", "mode:2,{0}".format(self.random_catalogue_expr(rng, "implicit"))
            if shape == "param":
                return "derive", "mode:3,{0}".format(self.random_catalogue_param_triplet(rng))
            if shape == "second":
                return "derive", "{0},method=second".format(self.random_catalogue_expr(rng, "second"))
            expr = self.random_catalogue_expr(rng, shape)
            method = shape if shape in ("chain", "product", "quotient", "logdiff") else "auto"
            return "derive", "{0},x,method={1}".format(expr, method)
        if name in ("implicit_diff",):
            return "derive", "mode:2,{0}".format(self.random_catalogue_expr(rng, "implicit"))
        if name in ("param_diff",):
            return "derive", "mode:3,{0}".format(self.random_catalogue_param_triplet(rng))
        if name in ("integrate", "int"):
            if shape == "math_abs":
                expr = rng.choice(["abs(2*x-3)", "sqrt((2*x-3)^2)"])
            elif shape == "math_sqrt":
                expr = rng.choice(["sqrt(1-x^2)", "sqrt((2*x-3)^2)", "sqrt(x^2+1)"])
            elif shape in ("math_log", "math_ln"):
                expr = rng.choice(["log(x)", "x*log(x)", "log(x)/x"])
            elif shape == "math_log_base":
                expr = rng.choice(["log(3,x)", "log(x)/log(3)"])
            elif shape == "math_exp":
                expr = rng.choice(["exp(2*x-3)", "x*exp(-x^2)", "x^3*exp(2*x^2)"])
            elif shape in ("math_atan", "math_arctan"):
                expr = rng.choice(["atan((x-1)/3)", "arctan(2*x-3)"])
            elif shape in ("math_asin", "math_arcsin"):
                expr = rng.choice(["asin((x-1)/3)", "arcsin((2*x+1)/5)"])
            elif shape in ("math_acos", "math_arccos"):
                expr = rng.choice(["acos((x-1)/3)", "arccos((2*x+1)/5)"])
            else:
                expr = self.random_catalogue_expr(rng, shape)
            method = shape if shape in ("direct", "reverse_chain", "sub", "parts", "di", "trig", "pf", "div", "weierstrass", "symmetry") else "auto"
            return "int", "{0},method={1}".format(expr, method)
        if name in ("solve", "fsolve", "zeros"):
            solve_methods = {"linear", "factor", "quad_formula", "complete_square", "substitution", "clear_denoms", "log_exp", "numeric", "interval"}
            method = "auto" if shape.startswith("math_") else ("quad_formula" if shape == "quad" else (shape if shape in solve_methods else "auto"))
            expr = self.random_catalogue_expr(rng, shape)
            if shape.startswith("math_") and "=" not in expr:
                expr = "{0}=0".format(expr)
            return "alg", "{0},method={1}".format(expr, method)
        if name in ("solve_trig", "trigsolve"):
            trig_methods = {"general", "bounded", "cast", "identity", "rform", "square_then_check"}
            method = "auto" if shape.startswith("math_") or shape not in trig_methods else shape
            expr = self.random_catalogue_expr(rng, shape)
            if "=" in expr:
                return "trig", "{0},x,0,2*pi,8,method={1}".format(expr, method)
            return "trig", "{0},method={1}".format(expr, method)
        if name in ("trig_prove", "trig_transform", "trig_rewrite"):
            pairs = {
                "pythag": ("sin(x)^2+cos(x)^2", "1"),
                "double_angle": ("1-cos(2*x)", "2*sin(x)^2"),
                "compound_angle": ("sin(x+y)", "sin(x)*cos(y)+cos(x)*sin(y)"),
                "target": ("tan(x)", "sin(x)/cos(x)"),
            }
            lhs, rhs = pairs.get(shape, pairs["pythag"])
            return "trig", "{0},target={1},method=auto".format(lhs, rhs)
        if name.startswith("trig") or name in ("sin", "cos", "tan"):
            method = "auto" if shape.startswith("math_") or shape in ("hidden_identity",) else shape
            return "trig", "{0},method={1}".format(self.random_catalogue_expr(rng, shape), method)
        if name in ("complete_square", "factor", "expand", "partfrac", "collect"):
            method = "auto" if shape.startswith("math_") else ("complete_square" if name == "complete_square" else ("pf" if name == "partfrac" else name))
            return "alg", "{0},method={1}".format(self.random_catalogue_expr(rng, shape), method)
        if name == "binom_expand":
            return "alg", "{0},method=expand".format(self.random_catalogue_expr(rng, shape))
        if name == "binom_coeff":
            return "alg", "coeff({0},x,{1})".format(self.random_catalogue_expr(rng, shape), rng.randint(1, 5))
        if name in ("domain", "range"):
            return "alg", "{0}({1})".format(name, self.random_domain_range_expr(rng, shape))
        if name in ("binomial", "normal", "poisson", "correlation", "covariance", "mean", "median", "quartiles", "stddev"):
            return "stats", self.catalogue_call_for(fn, combo, shape, rng)
        return "", self.catalogue_call_for(fn, combo, shape, rng)

    def make_catalogue_graph_case(self, fn, combo, shape, index, rng):
        flag, expr = self.catalogue_direct_expr(fn, combo, shape, rng)
        combo_key = "arity{0}".format(len(combo))
        graph_node = "{0}|{1}|{2}".format(fn.name, combo_key, shape)
        meta = {
            "function": fn.name,
            "signature": fn.signature,
            "params": [p.get("name", "") for p in fn.params],
            "combo": list(combo),
            "shape": shape,
            "flag": flag or "parse",
        }

        def runner():
            if flag:
                out, err = self.run_host_feature(flag, expr)
            else:
                out, err = self.run_host_expr(expr)
            combined = out if not err else out + "\n" + err
            text = normalized_text(combined)
            bad = any(s in text for s in ("err:", "missing host binary", "timeout after", "traceback", "dimension mismatch"))
            if flag in ("derive", "int", "alg", "trig", "stats"):
                ok = (not bad) and any(s in text for s in ("answer:", "dy/dx", " = ", "+ c", "result", "domain", "range", "method:"))
            else:
                ok = (not bad) and "fmt:" in text
            return ok, combined

        label = "Cat {0}: {1} · {2}".format(index, fn.name, shape)
        return CaseSpec(
            label=label,
            program="CatalogueGraph",
            runner=runner,
            input_text=expr,
            check_info="catalogue={0}; params={1}; shape={2}; graph={3}".format(fn.signature, len(combo), shape, graph_node),
            feature="catalogue:{0}:{1}".format(fn.name, shape),
            graph_node=graph_node,
            graph_meta=meta,
        )

    def build_catalogue_explorer_cases(self, difficulty, count, rng):
        funcs = load_all_catalogue_functions()
        candidates = []
        for fn in funcs:
            if not self.catalogue_testable_function(fn):
                continue
            for combo in self.catalogue_combo_indices(fn):
                for shape in self.catalogue_shapes_for(fn.name):
                    key = "{0}|arity{1}|{2}".format(fn.name, len(combo), shape)
                    candidates.append((key, fn, combo, shape))
        rng.shuffle(candidates)
        candidates.sort(key=lambda item: self.random_graph.score(item[0]))
        chosen = []
        for key, fn, combo, shape in candidates:
            node = self.random_graph.nodes.get(key, {})
            # Do not retry a passed function/shape/parameter layout in this run.
            if node.get("pass", 0) > 0:
                continue
            chosen.append((fn, combo, shape))
            if len(chosen) >= count:
                break
        if len(chosen) < count:
            for key, fn, combo, shape in candidates:
                if (fn, combo, shape) not in chosen:
                    chosen.append((fn, combo, shape))
                    if len(chosen) >= count:
                        break
        return [self.make_catalogue_graph_case(fn, combo, shape, i + 1, rng) for i, (fn, combo, shape) in enumerate(chosen)]

    def announce_catalogue_graph(self, emit):
        funcs = load_all_catalogue_functions()
        self.write_catalogue_manifest(funcs)
        total, passed, failed, weak = self.random_graph.summary()
        emit(self.append_result, "[dim]All catalogue: {0} functions[/dim]".format(len(funcs)))
        emit(self.append_result, "[dim]Math vocab: {0} tokens[/dim]".format(len(RANDOM_SUPPORTED_MATH_THINGS)))
        emit(self.append_result, "[dim]Params list: {0}[/dim]".format(CATALOGUE_MANIFEST_PATH))
        emit(self.append_result, "[dim]Graph: {0} · nodes={1} pass={2} fail={3} weak={4}[/dim]".format(CATALOGUE_GRAPH_PATH, total, passed, failed, weak))
        sample = ", ".join(fn.signature for fn in funcs[:12])
        if sample:
            emit(self.append_result, "[dim]Sample: {0}[/dim]".format(sample))

    def dedupe_cases_for_run(self, cases):
        scope = (self.last_run_scope[0] if self.last_run_scope else "").lower()
        if scope in ("stress:syllabus", "stress:online", "stress:crash"):
            return list(cases)
        # Infinite runs must not use a global key set: it exhausts the space and
        # later batches become empty (harness looks "stuck" with no new tests).
        if getattr(self, "_random_infinite", False):
            deduped = []
            local = set()
            for case in cases:
                key = self.case_question_key(case)
                if key in local:
                    continue
                local.add(key)
                deduped.append(case)
            return deduped
        deduped = []
        seen = set(self.current_run_question_keys)
        for case in cases:
            key = self.case_question_key(case)
            if key in seen:
                continue
            seen.add(key)
            deduped.append(case)
        self.current_run_question_keys = seen
        return deduped

    def build_unique_random_cases(self, features, count, rng, difficulty):
        return list(self.iter_unique_random_cases(features, count, rng, difficulty))

    def iter_unique_random_cases(self, features, count, rng, difficulty):
        counts = self._feature_count_alloc(count, features, rng)
        batch_seen = set()
        built = []
        next_index = 1
        
        use_llm_gen = self.llm_enabled_for_tests and self.llm_manager and self.llm_manager.enabled
        
        feature_prog_map = {
            "random_algebra_compare_case": "Algebra",
            "random_algebra_transform_case": "Algebra",
            "random_algebra_expand_case": "Algebra",
            "random_algebra_poly_case": "Algebra",
            "random_algebra_solve_case": "Algebra",
            "random_algebra_comp_case": "Algebra",
            "random_algebra_inverse_case": "Algebra",
            "random_algebra_rewrite_case": "Algebra",
            "random_algebra_complete_square_case": "Algebra",
            "random_algebra_domain_case": "Algebra",
            "random_algebra_range_case": "Algebra",
            "random_algebra_domain_interval_case": "Algebra",
            "random_algebra_solve_letter_case": "Algebra",
            "random_algebra_compare_letter_case": "Algebra",
            "random_algebra_expand_letter_case": "Algebra",
            "random_algebra_cartesian_case": "Algebra",
            "random_algebra_rearrange_case": "Algebra",
            "random_derive_normal_case": "Derive",
            "random_derive_product_case": "Derive",
            "random_integrate_auto_case": "Integrate",
            "random_integrate_def_case": "Integrate",
            "random_trig_solve_case": "Trigonometry",
            "random_trig_simplify_case": "Trigonometry",
            "random_trig_prove_case": "Trigonometry",
            "random_trig_rearrange_case": "Trigonometry",
            "random_stats_one_var_case": "Stats",
            "random_stats_regression_case": "Stats",
            "random_stats_binomial_case": "Stats",
            "random_stats_normal_case": "Stats",
            "random_stats_ztest_case": "Stats",
            "random_stats_plot_case": "Stats",
            "random_boolean_simplify_case": "Boolean",
            "random_boolean_nand_case": "Boolean",
            "random_boolean_nor_case": "Boolean",
            "random_boolean_prove_case": "Boolean",
            "random_matrix_case": "Matrix",
            "random_complex_case": "Complex",
            "random_calculate_case": "Calculate",
            "random_solve_rnd_case": "Solve",
            "random_method_surface_case": "MethodSurface",
            "random_university_suvat_case": "SUVAT",
        }
        
        for feature, feature_count in zip(features, counts):
            made = 0
            attempts = 0
            max_attempts = max(300, feature_count * 100)
            while made < feature_count and attempts < max_attempts:
                case_difficulty = self.random_case_difficulty(rng, difficulty)
                case = feature(rng, case_difficulty, next_index)
                case = self.with_random_format_fuzz(case, rng, case_difficulty)
                
                if use_llm_gen and rng.random() < LLM_GENERATION_CHANCE:
                    prog_name = feature_prog_map.get(feature.__name__, "Algebra")
                    llm_result = self.generate_llm_case(prog_name, case_difficulty)
                    if llm_result:
                        ref_note = ""
                        if isinstance(llm_result, (tuple, list)) and len(llm_result) >= 2 and llm_result[1]:
                            ref_note = " [llm ref: {0}]".format(str(llm_result[1])[:160].replace("\n", " "))
                        # Keep check_info = harness-only; a generated ref here often disagrees in units
                        # with the CLI case and causes false LLM "INCORRECT" on correct calculator output.
                        case.label = "[LLM] {0}{1}".format(case.label, ref_note)
                
                key = self.case_question_key(case)
                attempts += 1
                if key in batch_seen:
                    continue
                if not getattr(self, "_random_infinite", False) and key in self.session_random_question_keys:
                    continue
                batch_seen.add(key)
                if not getattr(self, "_random_infinite", False):
                    self.session_random_question_keys.add(key)
                built.append(case)
                next_index += 1
                made += 1
        rng.shuffle(built)
        for case in built:
            yield case

    def run_case_specs(self, cases, workers=None, skip_quality=False, infinite_mode=False):
        """Execute cases, record harness results, and optionally verify via LLM."""
        self._in_run_case_specs = True
        try:
            cases = self.dedupe_cases_for_run(cases)
            active_workers = max(1, min(workers or CASE_WORKERS, _safe_worker_cap()))
            case_count = len(cases)
            skip_quality = skip_quality or self.total_expected > 2000
            # /llm enables this flag before a random run.  When true, every case
            # output is checked by the configured local model after the harness
            # checker has done its deterministic work.
            use_llm = self.llm_enabled_for_tests and self.llm_manager and self.llm_manager.enabled
            if use_llm:
                model = getattr(self.llm_manager, "selected_model", None) or "unknown"
                if "qwen3.5" in model or "qwen2.5" in model:
                    pass
                elif "llama2" in model or "falcon" in model:
                    pass

            if case_count > 5000:
                batch_size = max(500, case_count // 50)
                active_workers = min(active_workers, 24)
            elif case_count > 2000:
                batch_size = max(300, case_count // 80)
                active_workers = min(active_workers, 20)
            elif case_count > 200:
                batch_size = max(100, case_count // 100)
            else:
                batch_size = max(1, case_count)

            if case_count == 0:
                return

            def evaluate_timed(case):
                import time
                start = time.time()
                try:
                    passed, output = case.runner()
                except Exception as err:
                    passed, output = False, str(err)
                elapsed = time.time() - start
                return case, passed, output, elapsed

            def llm_verify_one(record, max_retries=3):
                if not use_llm:
                    return ("", "")

                for _attempt in range(max_retries):
                    try:
                        context = "{0}: {1}".format(record.program, record.label)
                        expected = record.check_info or context
                        result = self.llm_manager.verify(
                            record.output, expected, context, check_working_quality=False
                        )
                        verdict = result.get("verdict", "")
                        explanation = result.get("explanation", "")

                        if verdict in ("CORRECT", "INCORRECT", "NEEDS_REVIEW"):
                            return (verdict, explanation)
                    except Exception:
                        pass

                return ("", "")

            def weighted_verdict(code_verdict, llm_verdict):
                if not llm_verdict or llm_verdict in ("", "DISABLED"):
                    return code_verdict

                if llm_verdict == "CORRECT":
                    return "CORRECT"
                if llm_verdict == "INCORRECT":
                    if code_verdict == "INCORRECT":
                        return "INCORRECT"
                    return "NEEDS_REVIEW"
                if llm_verdict == "NEEDS_REVIEW":
                    if code_verdict == "INCORRECT":
                        return "INCORRECT"
                    return "NEEDS_REVIEW"
                return code_verdict

            def apply_llm_to_record(record, verdict, explanation, llm_elapsed=0.1):
                # Conservative policy: when harness already proved correctness,
                # treat raw LLM INCORRECT as review-needed noise, not a hard call.
                if verdict == "INCORRECT" and record.passed:
                    verdict = "NEEDS_REVIEW"
                record.llm_verdict = verdict or ""
                record.llm_explanation = (explanation or "")
                if verdict in ("CORRECT", "INCORRECT", "NEEDS_REVIEW"):
                    llm_color = (
                        "#22c55e"
                        if verdict == "CORRECT"
                        else ("#f59e0b" if verdict == "NEEDS_REVIEW" else "#f87171")
                    )
                    self.append_result(
                        "[dim]  └─ LLM: [{0}]{1}[/{0}][/dim]".format(llm_color, verdict)
                    )
                code_verdict = "CORRECT" if record.passed else "INCORRECT"
                final_verdict = weighted_verdict(code_verdict, verdict)
                self.apply_llm_weighted_status(record, final_verdict)
                self._append_session_report_if_worthy(record)
                self.record_random_graph_result(record)
                if verdict in ("CORRECT", "INCORRECT", "NEEDS_REVIEW") and record.test_id is not None:
                    import time as time_module
                    tnow = time_module.time()
                    self._llm_times.append(max(1e-4, float(llm_elapsed)))
                    if len(self._llm_times) > 200:
                        self._llm_times = self._llm_times[-150:]
                    self._llm_timestamps.append((tnow, len(self._llm_times)))
                    if len(self._llm_timestamps) > 80:
                        self._llm_timestamps = self._llm_timestamps[-50:]

            llm_pending = []
            llm_bs = 1 if os.environ.get("CASIO_LLM_BATCH_DISABLE", "").lower() in ("1", "true", "yes") else _CASIO_LLM_BATCH_SIZE

            def flush_llm_pending():
                if not llm_pending or not use_llm:
                    return
                buf = list(llm_pending)
                del llm_pending[:]
                import time as time_module
                t0 = time_module.time()
                items = []
                for rec in buf:
                    ctx = "{0}: {1}".format(rec.program, rec.label)
                    exp = rec.check_info or ctx
                    items.append((rec.output, exp, ctx))
                try:
                    batch_out = self.llm_manager.verify_batch(items)
                except Exception:
                    batch_out = None
                dt = max(1e-4, time_module.time() - t0)
                per = dt / max(len(buf), 1)
                if not batch_out or len(batch_out) != len(buf):
                    for rec in buf:
                        v, e = llm_verify_one(rec)
                        apply_llm_to_record(rec, v, e, per)
                    return
                def _batch_line_for_record(raw, idx):
                    raw = raw or ""
                    wanted = str(idx + 1)
                    for line in raw.splitlines():
                        s = line.strip()
                        if re.match(r"^#?\s*" + re.escape(wanted) + r"\b", s):
                            return s[:500]
                    return raw[:180]
                i = 0
                while i < len(buf):
                    r = batch_out[i] or {}
                    v = r.get("verdict", "")
                    e = _batch_line_for_record(r.get("explanation", ""), i)
                    if v not in ("CORRECT", "INCORRECT", "NEEDS_REVIEW"):
                        v, e = llm_verify_one(buf[i])
                    apply_llm_to_record(buf[i], v, e, per)
                    i += 1

            for batch_start in range(0, len(cases), batch_size):
                if not self.running:
                    break
                batch = cases[batch_start : batch_start + batch_size]
                with ThreadPoolExecutor(max_workers=active_workers) as executor:
                    results = executor.map(evaluate_timed, batch)
                    for case, passed, output, elapsed in results:
                        if elapsed > 0:
                            self._test_times.append(elapsed)
                            if len(self._test_times) > 500:
                                self._test_times = self._test_times[-200:]
                        quality_issues = []
                        if not skip_quality and case.program not in ("Boolean", "MethodSurface", "Adversarial") and getattr(case, "feature", ""):
                            quality_issues = exam_working_quality_issues(
                                output, case.program, case.feature
                            )
                            if quality_issues:
                                passed = False
                                detail = "; ".join(quality_issues[:4])
                                case.check_info = (
                                    "{0}; exam-quality issues: {1}".format(
                                        case.check_info, detail
                                    )
                                    if case.check_info
                                    else "exam-quality issues: {0}".format(detail)
                                )
                        record = self.add_test(
                            case.label,
                            passed,
                            output,
                            case.program,
                            case.input_text,
                            case.check_info,
                            getattr(case, "feature", ""),
                            getattr(case, "graph_node", ""),
                            getattr(case, "graph_meta", None),
                        )
                        self.record_adversarial_case_result(case, record)
                        if use_llm and case.program not in ("Boolean", "MethodSurface"):
                            llm_pending.append(record)
                            if len(llm_pending) >= llm_bs:
                                flush_llm_pending()
                        else:
                            self._append_session_report_if_worthy(record)
                            self.record_random_graph_result(record)
                flush_llm_pending()
                self.flush_random_graph()
        finally:
            self._in_run_case_specs = False

    def balanced_counts(self, total, parts):
        base = total // parts
        extra = total % parts
        return [base + (1 if i < extra else 0) for i in range(parts)]

    def _exam_stress_alloc_enabled(self):
        return (os.environ.get("CASIO_EXAM_STRESS", "1") or "1").lower() not in ("0", "false", "no", "off")

    def _weighted_count_alloc(self, n_total, weights, rng):
        """Largest-remainder apportioning of n_total to len(weights) buckets (exam-style, jittered)."""
        w = [max(0.05, float(wh) * rng.uniform(0.88, 1.12)) for wh in weights]
        s = sum(w)
        if s <= 0 or n_total <= 0:
            return self.balanced_counts(n_total, len(weights))
        exact = [n_total * (x / s) for x in w]
        out = [int(t) for t in exact]
        r = n_total - sum(out)
        fr = sorted(
            ((i, exact[i] - out[i]) for i in range(len(w))),
            key=lambda z: -z[1],
        )
        for j in range(max(0, r)):
            out[fr[j % len(out)][0]] += 1
        return out

    def _feature_count_alloc(self, n_total, features, rng):
        if not self._exam_stress_alloc_enabled() or n_total <= 0 or not features:
            return self.balanced_counts(n_total, len(features))
        boost_map = _parse_casio_exam_boost_env()
        w = []
        for fn in features:
            name = getattr(fn, "__name__", "")
            base = float(EXAM_BUILDER_STRESS_WEIGHTS.get(name, 1.0))
            base *= float(boost_map.get(name, 1.0))
            w.append(base * rng.uniform(0.9, 1.1))
        out = self._weighted_count_alloc(n_total, w, rng)
        if n_total >= len(features):
            for i, value in enumerate(out):
                if value:
                    continue
                donor = max(range(len(out)), key=lambda j: out[j])
                if out[donor] <= 1:
                    break
                out[donor] -= 1
                out[i] = 1
        return out

    def signed_int_text(self, value):
        if value > 0:
            return f"+{value}"
        if value < 0:
            return str(value)
        return ""

    def random_nonzero_int(self, rng, low=1, high=9):
        value = 0
        while value == 0:
            value = rng.randint(low, high)
        return value

    def difficulty_number(self, difficulty, rng=None, easy=1, medium=2, hard=3):
        r = rng or random
        if difficulty == "easy":
            return easy
        if difficulty == "medium":
            return medium
        if difficulty == "chaos":
            return self.random_unbounded_count(r, minimum=hard)
        return hard

    def random_unbounded_count(self, rng, minimum=1, continue_probability=0.28):
        count = minimum
        while rng.random() < continue_probability:
            count += 1
            continue_probability *= 0.985
        return count

    def random_case_difficulty(self, rng, requested):
        if requested == "chaos":
            return rng.choice(["hard", "hard", "chaos", "chaos", "chaos", "chaos", "chaos"])
        if requested == "all":
            return rng.choice(["medium", "hard", "chaos", "chaos"])
        if requested == "hard" and rng.random() < 0.6:
            return "chaos"
        if requested == "medium" and rng.random() < 0.45:
            return "chaos"
        if requested == "easy" and rng.random() < 0.25:
            return "chaos"
        return requested

    def generate_llm_case(self, program_name, case_difficulty):
        """Call LLM for a reference answer; chance set by CASIO_LLM_GENERATION_CHANCE. Tags case label; does not replace runner input."""
        if not self.llm_manager or not self.llm_manager.enabled:
            return None
        
        prompt = LLM_GENERATION_PROMPTS.get(program_name, {}).get(case_difficulty)
        if not prompt:
            return None
        
        result = self.llm_manager.generate(prompt)
        if result.get("error") or not result.get("response"):
            return None
        
        def tag_value(tag):
            prefix = tag.upper() + ":"
            for raw in result["response"].strip().split("\n"):
                line = raw.strip()
                if line.upper().startswith(prefix):
                    return line[len(prefix):].strip()
            return None
        
        if program_name == "Algebra":
            eq_value = tag_value("EQ")
            ans_value = tag_value("ANSWER")
            if not eq_value or not ans_value:
                return None
            if "=" not in eq_value:
                eq_value += "=0"
            return eq_value, ans_value

        elif program_name == "Solve":
            eq_value = tag_value("EQ")
            sol_value = tag_value("SOL")
            if not eq_value or not sol_value:
                return None
            return eq_value, sol_value
        
        elif program_name == "Derive":
            expr_value = tag_value("EXPR")
            deriv_value = tag_value("DERIV")
            if not expr_value or not deriv_value:
                return None
            return expr_value, deriv_value
        
        elif program_name == "Integrate":
            expr_value = tag_value("EXPR")
            int_value = tag_value("INT")
            if not expr_value or not int_value:
                return None
            return expr_value, int_value
        
        elif program_name == "Trigonometry":
            eq_value = tag_value("EQ")
            sol_value = tag_value("SOL")
            if not eq_value or not sol_value:
                return None
            return eq_value, sol_value
        
        elif program_name == "Matrix":
            op_value = tag_value("OP")
            mat_value = tag_value("MATRIX")
            ans_value = tag_value("ANSWER")
            if not op_value or not mat_value or not ans_value:
                return None
            return op_value + "\n" + mat_value, ans_value

        elif program_name in ("Complex", "Calculate"):
            expr_value = tag_value("EXPR")
            ans_value = tag_value("ANSWER")
            if not expr_value or not ans_value:
                return None
            return expr_value, ans_value
        
        return None

    def random_chaos_int(self, rng, base_low, base_high):
        magnitude = self.random_unbounded_count(rng, minimum=max(abs(base_low), abs(base_high), 1), continue_probability=0.32)
        value = rng.randint(1, magnitude)
        if base_low < 0 and rng.random() < 0.5:
            value = -value
        return value

    def random_power_limit(self, rng, difficulty, easy=2, medium=3, hard=5):
        if difficulty == "easy":
            return easy
        if difficulty == "medium":
            return medium
        if difficulty == "chaos":
            return self.random_unbounded_count(rng, minimum=hard, continue_probability=0.35)
        return hard

    def random_depth_for_difficulty(self, rng, difficulty):
        if difficulty == "easy":
            return 1
        if difficulty == "medium":
            return rng.randint(2, 4)
        if difficulty == "chaos":
            return self.random_unbounded_count(rng, minimum=5, continue_probability=0.30)
        return rng.randint(4, 7)

    def fuzz_atom_format(self, text, rng, difficulty):
        if not text or not text.strip():
            return text
        text = text.strip()
        intensity = {"easy": 0.30, "medium": 0.55, "hard": 0.80, "chaos": 0.90}.get(difficulty, 0.55)

        def maybe_wrap_var(match):
            name = match.group(0)
            return f"({name})" if rng.random() < intensity * 0.30 else name

        def maybe_wrap_number(match):
            number = match.group(0)
            return f"({number})" if rng.random() < intensity * 0.22 else number

        text = re.sub(r"(?<![A-Za-z_])(x|y|t)(?![A-Za-z_])", maybe_wrap_var, text)
        text = re.sub(r"(?<![A-Za-z_])\d+(?:\.\d+)?(?![A-Za-z_])", maybe_wrap_number, text)

        out = []
        i = 0
        while i < len(text):
            if text.startswith("**", i):
                op = "**"
                i += 2
            else:
                char = text[i]
                if char == "^" and rng.random() < intensity * 0.55:
                    op = "**"
                    i += 1
                elif char in "+-*/^=":
                    op = char
                    i += 1
                else:
                    out.append(char)
                    i += 1
                    continue
            left = rng.choice(["", " ", "  "]) if rng.random() < intensity else ""
            right = rng.choice(["", " ", "  "]) if rng.random() < intensity else ""
            out.append(f"{left}{op}{right}")
        text = "".join(out)

        if rng.random() < intensity * 0.45:
            text = re.sub(r"(?<=\d)\s*\*\s*(?=[A-Za-z_(])", "", text)
        if rng.random() < intensity * 0.35:
            text = re.sub(r"(?<=\))\s*\*\s*(?=[A-Za-z_(])", "", text)
        if rng.random() < intensity * 0.25:
            text = re.sub(r"(?<=[A-Za-z_])\s*\*\s*(?=\()", "", text)
        if rng.random() < intensity * 0.6 and not (text.startswith("(") and text.endswith(")")):
            text = f"({text})"
        return text

    def fuzz_input_line_format(self, line, rng, difficulty):
        stripped = line.strip()
        if stripped == "" or stripped == ",":
            return line
        if re.fullmatch(r"\d+", stripped):
            return line
        if "=" in stripped and stripped.count("=") != 1:
            return line
        if "," in line and "=" in line and stripped.count("=") == 1:
            return line  # Don't fuzz equations with commas
        if "," in line:
            parts = split_top_level_text(line, ",")
            if len(parts) > 1:
                fuzzed = []
                for i, part in enumerate(parts):
                    part_strip = part.strip()
                    if i == 1 and re.fullmatch(r"[A-Za-z_]\w*", part_strip):
                        fuzzed.append(part_strip)
                    else:
                        fuzzed.append(self.fuzz_atom_format(part_strip, rng, difficulty))
                pieces = []
                for i, part in enumerate(fuzzed):
                    if i:
                        pieces.append(rng.choice([",", ", ", " , ", " ,"]))
                    pieces.append(part)
                return "".join(pieces)
        return self.fuzz_atom_format(line, rng, difficulty)

    def fuzz_cli_input_format(self, input_text, rng, difficulty):
        lines = (input_text or "").splitlines()
        if not lines:
            return input_text
        trailing_newline = input_text.endswith("\n")
        fuzzed = []
        for index, line in enumerate(lines):
            if index == 0:
                fuzzed.append(line)
            else:
                fuzzed.append(self.fuzz_input_line_format(line, rng, difficulty))
        text = "\n".join(fuzzed)
        if trailing_newline:
            text += "\n"
        return text

    def fuzz_suvat_cli_input_format(self, input_text, rng, difficulty):
        lines = (input_text or "").splitlines()
        trailing_newline = input_text.endswith("\n")
        fuzzed = []
        for line in lines:
            stripped = line.strip()
            if stripped == "":
                fuzzed.append(line)
            elif stripped == ",":
                fuzzed.append(rng.choice([",", " ,", ", "]))
            else:
                fuzzed.append(self.fuzz_atom_format(stripped, rng, difficulty))
        text = "\n".join(fuzzed)
        if trailing_newline:
            text += "\n"
        return text

    def with_random_format_fuzz(self, case, rng, difficulty):
        if not case or not case.script or not callable(case.checker):
            return case
        if case.program == "Boolean" or "booleanProgram.py" in case.script:
            return case
        fuzzed_input = self.fuzz_cli_input_format(case.input_text, rng, difficulty)
        if fuzzed_input == case.input_text:
            return case
        if not getattr(case, "use_calculated", True) or limited_by(CALCULATED_CHECK_MAX_INPUT_CHARS, len(fuzzed_input or "")):
            checker = case.checker
        else:
            checker = self.calculated_checker_for_cli_case(case.script, fuzzed_input, case.checker)
            if checker is None:
                checker = case.checker
        script = case.script

        def runner():
            out, _ = self.run_cli(script, fuzzed_input)
            return bool(checker(out)), out

        info = case.check_info
        if info:
            info += "; format fuzz"
        else:
            info = "format fuzz"
        return CaseSpec(
            case.label,
            case.program,
            runner,
            fuzzed_input,
            '',
            info,
            case.feature,
            script,
            checker,
        )

    def random_small_rational_text(self, rng, allow_negative=True):
        den = rng.choice([2, 3, 4, 5, 6, 8])
        nume = self.random_nonzero_int(rng, -9 if allow_negative else 1, 9)
        if den == 1:
            return str(nume)
        return f"{nume}/{den}"

    def random_edge_case_expr(self, rng, var="x", difficulty="hard"):
        edge_cases = [
            lambda: f"1/({self.random_positive_expr(rng, var, difficulty)})",
            lambda: f"({var}-1)/({var}^2-1)",
            lambda: f"exp({var})/exp({var})",
            lambda: f"log(exp({var}))",
            lambda: f"sin({var})/cos({var})",
            lambda: f"(sin({var})^2+cos({var})^2)",
            lambda: f"tan({var})*cos({var})",
            lambda: f"sec({var})-tan({var})*sin({var})",
            lambda: f"(exp({var})-1)/exp({var})",
            lambda: f"sqrt({var}^2)",
            lambda: f"(exp({var})^2-1)/(exp({var})+1)",
            lambda: f"log(({var})^2+1)",
            lambda: f"(sin({var})+cos({var}))/(sin({var})-cos({var}))",
            lambda: f"tan({var})^2+1-sec({var})^2",
            lambda: f"asin(sin({var}))+acos(cos({var}))",
        ]
        return rng.choice(edge_cases)()

    def random_deeply_nested_expr(self, rng, var="x", depth=4):
        expr = var
        for _ in range(depth):
            expr = rng.choice([
                f"exp({expr})",
                f"log({expr})",
                f"ln(abs({expr})+2)",
                f"log(2,abs({expr})+3)",
                f"sin({expr})",
                f"cos({expr})",
                f"tan({expr})",
                f"sec({expr})",
                f"cot({expr})",
                f"cosec({expr})",
                f"abs({expr})",
                f"({expr})^2",
                f"sqrt({expr})",
            ])
        return expr

    def random_polynomial_chaos(self, rng, var="x", degree=5):
        terms = []
        for p in range(degree, -1, -1):
            if p == 0:
                terms.append(str(rng.randint(-9, 9)))
            else:
                coeff = rng.randint(-9, 9)
                if coeff == 0:
                    continue
                if p == 1:
                    terms.append(f"{coeff}*{var}")
                else:
                    terms.append(f"{coeff}*{var}^{p}")
        return "+".join(terms) if terms else "0"

    def random_rational_composite(self, rng, var="x", difficulty="hard"):
        num = rng.choice([
            self.random_polynomial_expr(rng, var, difficulty),
            f"sin({self.random_angle_expr(rng, var, difficulty)})",
            f"exp({self.random_linear_expr(rng, var, allow_negative=True)})",
        ])
        denom = rng.choice([
            self.random_positive_expr(rng, var, difficulty),
            f"({self.random_linear_expr(rng, var)})^2+1",
        ])
        return f"({num})/({denom})"

    def random_trig_chaos(self, rng, var="x", difficulty="hard"):
        angle = self.random_angle_expr(rng, var, difficulty)
        return rng.choice([
            f"sin({angle})+cos({angle})",
            f"sin({angle})^3+cos({angle})^3",
            f"tan({angle})+cot({angle})",
            f"sec({angle})^2-tan({angle})^2",
            f"cosec({angle})^2-cot({angle})^2",
            f"sin({angle})*cos({angle})/(cos({angle})^2)",
            f"(sin({angle})+cos({angle}))^2",
            f"sin(2*{angle})/(2*sin({angle})*cos({angle}))",
        ])

    def random_hard_inverse_expr(self, rng, var="x", difficulty="hard"):
        return rng.choice([
            f"asin(sin({self.random_linear_expr(rng, var)}))",
            f"atan(tan({self.random_linear_expr(rng, var)}))",
            f"log(exp({self.random_linear_expr(rng, var, allow_negative=True)}))",
            f"exp(log({self.random_positive_expr(rng, var, difficulty)}))",
            f"sin(arcsin({var}))",
            f"cos(arccos({var}))",
        ])

    def random_compound_trig_expr(self, rng, var="x", difficulty="hard"):
        angle = self.random_angle_expr(rng, var, difficulty)
        return rng.choice([
            f"sin({angle})+cos({angle})",
            f"sin({angle})^2+cos({angle})^2",
            f"sin(2*{angle})",
            f"cos(2*{angle})",
            f"sin({angle})+sin({self.random_angle_expr(rng, var, difficulty)})",
            f"cos({angle})*cos({self.random_angle_expr(rng, var, difficulty)})",
        ])

    def random_nested_log_exp(self, rng, var="x", depth=3):
        expr = var
        for _ in range(depth):
            expr = rng.choice([
                f"exp({expr})",
                f"log({expr})",
                f"log(exp({expr}))",
                f"exp(log({expr}))",
            ])
        return expr

    def random_pi_shift_text(self, rng):
        return rng.choice([
            "pi/6", "pi/4", "pi/3", "pi/2",
            "-pi/6", "-pi/4", "-pi/3", "-pi/2",
        ])

    def random_linear_expr(self, rng, var="x", allow_negative=True):
        low = -9 if allow_negative else 1
        a = self.random_nonzero_int(rng, low, 9)
        b = rng.randint(-9, 9)
        out = f"{a}*{var}"
        if b != 0:
            out += self.signed_int_text(b)
        return out

    def random_affine_expr(self, rng, var="x", difficulty="hard", allow_negative=True, fractions=False):
        if fractions and difficulty in ("hard", "chaos") and rng.random() < (0.70 if difficulty == "chaos" else 0.45):
            a = self.random_small_rational_text(rng, allow_negative=allow_negative)
            b = self.random_small_rational_text(rng, allow_negative=True)
            return f"({a})*{var}+({b})"
        return self.random_linear_expr(rng, var, allow_negative=allow_negative)

    def random_shifted_power_expr(self, rng, var="x", difficulty="hard"):
        base = self.random_affine_expr(rng, var, difficulty, allow_negative=True, fractions=difficulty in ("hard", "chaos"))
        if difficulty == "chaos":
            power = self.random_power_limit(rng, difficulty, hard=7)
        else:
            powers = [2] if difficulty == "easy" else ([2, 3] if difficulty == "medium" else [2, 3, 4, 5, 6])
            power = rng.choice(powers)
        return f"({base})^{power}"

    def random_base_expr(self, rng, var="x", difficulty="hard"):
        choices = [
            var,
            self.random_affine_expr(rng, var, difficulty, allow_negative=True, fractions=False),
        ]
        if difficulty in ("medium", "hard", "chaos"):
            choices.append(self.random_shifted_power_expr(rng, var, "medium"))
            choices.append(f"{rng.randint(2, 7)}*{var}^2{self.signed_int_text(rng.randint(-7, 7))}*{var}+{rng.randint(1, 9)}")
        if difficulty in ("hard", "chaos"):
            choices.extend([
                self.random_shifted_power_expr(rng, var, difficulty),
                f"({self.random_affine_expr(rng, var, difficulty, allow_negative=True, fractions=False)})^2+({self.random_affine_expr(rng, var, difficulty, allow_negative=True, fractions=False)})",
                f"({self.random_affine_expr(rng, var, difficulty, allow_negative=True, fractions=False)})^3{self.signed_int_text(rng.randint(-6, 6))}",
            ])
        if difficulty == "chaos":
            choices.extend([
                f"({self.random_polynomial_expr(rng, var, 'chaos')})/({self.random_positive_expr(rng, var, 'hard')})",
                f"(({self.random_affine_expr(rng, var, 'chaos', allow_negative=True, fractions=True)})^{self.random_power_limit(rng, 'chaos', hard=4)})",
            ])
        return rng.choice(choices)

    def random_angle_expr(self, rng, var="x", difficulty="hard"):
        expr = rng.choice([
            self.random_base_expr(rng, var, difficulty),
            self.random_affine_expr(rng, var, difficulty, allow_negative=True, fractions=difficulty in ("hard", "chaos")),
            f"{rng.randint(2, 9)}*{var}^2{self.signed_int_text(rng.randint(-6, 6))}*{var}+{rng.randint(-7, 9)}",
        ])
        wraps = self.random_unbounded_count(rng, minimum=4, continue_probability=0.2) if difficulty == "chaos" else {"easy": 1, "medium": 2, "hard": 4}.get(difficulty, 2)
        i = 0
        while i < rng.randint(1, wraps):
            a = rng.randint(2, 8)
            b = rng.randint(-7, 7)
            mode_pool = ["affine"]
            if difficulty in ("medium", "hard", "chaos"):
                mode_pool.extend(["square", "difference"])
            if difficulty in ("hard", "chaos"):
                mode_pool.extend(["shifted_cube", "pi_shift", "fraction_scale"])
            mode = rng.choice(mode_pool)
            if mode == "square":
                expr = f"{a}*({expr})^2{self.signed_int_text(b)}"
            elif mode == "shifted_cube":
                expr = f"{a}*(({expr})+{rng.randint(-3, 3)})^3{self.signed_int_text(b)}"
            elif mode == "difference":
                expr = f"{a}*({expr})-{rng.randint(2, 7)}*{var}"
            elif mode == "pi_shift":
                expr = f"{a}*({expr})+{self.random_pi_shift_text(rng)}"
            elif mode == "fraction_scale":
                expr = f"({self.random_small_rational_text(rng)})*({expr}){self.signed_int_text(b)}"
            else:
                expr = f"{a}*({expr}){self.signed_int_text(b)}"
            i += 1
        return expr

    def random_polynomial_expr(self, rng, var="x", difficulty="hard"):
        if difficulty == "chaos":
            degree = self.random_unbounded_count(rng, minimum=8, continue_probability=0.25)
        else:
            degree = rng.randint(3, 8 if difficulty == "hard" else 5)
        terms = []
        power = degree
        while power >= 0:
            if power == degree or rng.random() < 0.65:
                coeff = self.random_nonzero_int(rng, -7, 7)
                if power == 0:
                    term = str(coeff)
                elif power == 1:
                    term = f"{coeff}*{var}"
                else:
                    term = f"{coeff}*{var}^{power}"
                terms.append(term)
            power -= 1
        expr = terms[0]
        for term in terms[1:]:
            if term.startswith("-"):
                expr += term
            else:
                expr += "+" + term
        return expr

    def random_fractional_expr(self, rng, var="x", difficulty="hard"):
        numerator = rng.choice([
            self.random_polynomial_expr(rng, var, difficulty),
            self.random_linear_expr(rng, var, allow_negative=True),
            f"sin({self.random_angle_expr(rng, var, difficulty)})+{rng.randint(1, 4)}",
            f"exp({self.random_affine_expr(rng, var, difficulty, allow_negative=True)})+{self.random_shifted_power_expr(rng, var, difficulty)}",
            f"log({self.random_positive_expr(rng, var, difficulty)})+{self.random_affine_expr(rng, var, difficulty, allow_negative=True)}",
        ])
        denominator = rng.choice([
            self.random_positive_expr(rng, var, difficulty),
            f"({self.random_linear_expr(rng, var, allow_negative=True)})^2+{rng.randint(2, 9)}",
            f"exp({self.random_linear_expr(rng, var, allow_negative=True)})+{rng.randint(2, 9)}",
            f"sqrt({self.random_positive_expr(rng, var, difficulty)})+{rng.randint(2, 9)}",
            f"({self.random_positive_expr(rng, var, difficulty)})^2+({self.random_affine_expr(rng, var, difficulty, allow_negative=True)})^2+{rng.randint(2, 9)}",
        ])
        return f"({numerator})/({denominator})"

    def random_positive_expr(self, rng, var="x", difficulty="hard"):
        mode = rng.choice(["square", "sum_squares", "exp_plus", "sqrt_guard", "log_guard"] if difficulty in ("hard", "chaos") else ["square", "sum_squares"])
        if mode == "sum_squares":
            left = self.random_affine_expr(rng, var, difficulty, allow_negative=True)
            right = self.random_affine_expr(rng, var, difficulty, allow_negative=True)
            return f"({left})^2+({right})^2+{rng.randint(2, 12)}"
        if mode == "exp_plus":
            return f"exp({self.random_affine_expr(rng, var, difficulty, allow_negative=True)})+{rng.randint(2, 12)}"
        if mode == "sqrt_guard":
            base = self.random_base_expr(rng, var, difficulty)
            return f"sqrt(({base})^2+{rng.randint(2, 12)})+{rng.randint(1, 7)}"
        if mode == "log_guard":
            inner = self.random_affine_expr(rng, var, difficulty, allow_negative=True)
            return f"log(abs({inner})+{rng.randint(2, 12)})"
        base = self.random_base_expr(rng, var, difficulty)
        extra = rng.randint(2, 12)
        return f"({base})^2+{extra}"

    def random_factorised_polynomial_expr(self, rng, var="x", difficulty="hard"):
        factors = []
        count = self.random_unbounded_count(rng, minimum=7, continue_probability=0.28) if difficulty == "chaos" else self.difficulty_number(difficulty, rng, easy=2, medium=3, hard=rng.randint(3, 6))
        for _ in range(count):
            root = rng.randint(-10, 10)
            mult = rng.choice([1, 1, 2, 2, 3, self.random_power_limit(rng, "chaos", hard=4)]) if difficulty == "chaos" else (1 if difficulty == "easy" else rng.choice([1, 1, 2]))
            factor = f"({var}{self.signed_int_text(-root)})"
            factors.append(f"{factor}^{mult}" if mult != 1 else factor)
        coeff = rng.choice([1, 1, -1, rng.randint(2, 7)])
        body = "*".join(factors)
        return body if coeff == 1 else f"{coeff}*{body}"

    def random_trig_combo_expr(self, rng, var="x", difficulty="hard"):
        angle = self.random_angle_expr(rng, var, difficulty)
        mode = rng.choice(["linear_combo", "pythag_mix", "ratio", "power_sum", "reciprocal_mix"])
        if mode == "linear_combo":
            return f"{rng.randint(2, 7)}*sin({angle}){self.signed_int_text(rng.randint(-7, 7))}*cos({angle})"
        if mode == "pythag_mix":
            return f"sin({angle})^2+cos({angle})^2+{rng.randint(1, 5)}*tan({angle})"
        if mode == "ratio":
            return rng.choice([f"sin({angle})/cos({angle})", f"cos({angle})/sin({angle})"])
        if mode == "reciprocal_mix":
            return f"sec({angle})^2-tan({angle})^2+cot({angle})"
        return f"sin({angle})^{self.random_power_limit(rng, difficulty, hard=5)}+cos({angle})^{self.random_power_limit(rng, difficulty, hard=5)}"

    def random_expression_family(self, rng, var="x", difficulty="hard", allow_trig=True):
        families = ["affine", "shifted_power", "polynomial", "factorised", "positive", "rational"]
        if allow_trig:
            families.append("trig_combo")
        if difficulty in ("medium", "hard", "chaos"):
            families.extend(["exp_safe", "log_safe", "ln_safe", "log_base_safe", "sqrt_safe", "abs_safe", "factorial_const"])
        family = rng.choice(families)
        if family == "affine":
            return self.random_affine_expr(rng, var, difficulty, allow_negative=True, fractions=difficulty in ("hard", "chaos"))
        if family == "shifted_power":
            return self.random_shifted_power_expr(rng, var, difficulty)
        if family == "polynomial":
            return self.random_polynomial_expr(rng, var, difficulty)
        if family == "factorised":
            return self.random_factorised_polynomial_expr(rng, var, difficulty)
        if family == "positive":
            return self.random_positive_expr(rng, var, difficulty)
        if family == "rational":
            return self.random_fractional_expr(rng, var, difficulty)
        if family == "trig_combo":
            return self.random_trig_combo_expr(rng, var, difficulty)
        if family == "exp_safe":
            return f"exp({self.random_affine_expr(rng, var, difficulty, allow_negative=True)})"
        if family == "log_safe":
            return f"log({self.random_positive_expr(rng, var, difficulty)})"
        if family == "ln_safe":
            return f"ln({self.random_positive_expr(rng, var, difficulty)})"
        if family == "log_base_safe":
            return f"log({rng.choice([2, 3, 5, 10])},{self.random_positive_expr(rng, var, difficulty)})"
        if family == "abs_safe":
            return f"abs({self.random_affine_expr(rng, var, difficulty, allow_negative=True)})"
        if family == "factorial_const":
            return rng.choice([f"{rng.randint(3, 8)}!", f"factorial({rng.randint(3, 8)})"])
        return f"sqrt({self.random_positive_expr(rng, var, difficulty)})"

    def random_general_expr(self, rng, var="x", difficulty="hard", depth=None):
        if depth is None:
            depth = self.random_depth_for_difficulty(rng, difficulty)
        if depth <= 0:
            return self.random_expression_family(rng, var, difficulty)

        inner = lambda: self.random_general_expr(rng, var, difficulty, depth - 1)
        positive = lambda: self.random_positive_expr(rng, var, difficulty)
        angle = lambda: self.random_angle_expr(rng, var, difficulty)
        choices = [
            lambda: f"sin({angle()})",
            lambda: f"cos({angle()})",
            lambda: f"tan({angle()})",
            lambda: f"sec({angle()})",
            lambda: f"cosec({angle()})",
            lambda: f"cot({angle()})",
            lambda: f"exp({inner()})",
            lambda: f"log({positive()})",
            lambda: f"ln({positive()})",
            lambda: f"log({rng.choice([2, 3, 5, 10])},{positive()})",
            lambda: f"log(exp({self.random_linear_expr(rng, var, allow_negative=True)})+{rng.randint(2, 9)})",
            lambda: f"abs({inner()})",
            lambda: f"asin(({self.random_linear_expr(rng, var, allow_negative=True)})/10)",
            lambda: f"acos(({self.random_linear_expr(rng, var, allow_negative=True)})/10)",
            lambda: f"atan({self.random_linear_expr(rng, var, allow_negative=True)})",
            lambda: f"{rng.randint(3, 8)}!",
            lambda: f"sqrt({positive()})",
            lambda: self.random_fractional_expr(rng, var, difficulty),
            lambda: f"({inner()})*({inner()})",
            lambda: f"({inner()})/({positive()})",
            lambda: f"({inner()})+({inner()})",
            lambda: f"({inner()})-({inner()})",
            lambda: f"({inner()})+({self.random_trig_combo_expr(rng, var, difficulty)})",
            lambda: f"({inner()})/(({self.random_expression_family(rng, var, difficulty, allow_trig=False)})^2+{rng.randint(2, 9)})",
            lambda: f"({inner()})^{self.random_power_limit(rng, difficulty, hard=10)}",
        ]
        return rng.choice(choices)()

    def algebra_solve_output_checker(self, eq_text, var="x"):
        quality = algebra_solve_checker()
        expected_values = sympy_equation_real_values(eq_text, var=var)

        def check(out):
            text = normalized_text(out)
            if "all real" in text or "all x satisfy" in text or "infinite solutions" in text:
                # Identity case: solver may correctly report "all real x" rather
                # than enumerating roots.
                if _has_non_exam_quality_output(text):
                    return False
                if any(item in text for item in _DEFAULT_FORBIDDEN_SNIPPETS):
                    return False
                return "answer:" in text or "x = all real" in text or "all real values in domain" in text
            if "no sol" in text or "no solution" in text or "no real solution" in text:
                if _has_non_exam_quality_output(text):
                    return False
                if any(item in text for item in _DEFAULT_FORBIDDEN_SNIPPETS):
                    return False
                return expected_values is None or expected_values == []
            if not quality(out):
                return False
            if (expected_values is None or expected_values == []) and ("x=[]" in text or "x = []" in text) and "simplify:" in text:
                return True
            values = extract_last_solution_values(out, var)
            if not values:
                return False
            for value in values:
                try:
                    residual = equation_residual(eq_text, var, value)
                except Exception:
                    return False
                if not numeric_close(residual, 0.0, rel_tol=1e-4, abs_tol=1e-4):
                    return False
            set_match = numeric_value_sets_match(values, expected_values)
            if set_match is False:
                return False
            return True

        return check

    def trig_solve_output_checker(self, eq_text, var="x", degrees=False, lower=None, upper=None):
        quality = build_checker(
            contains_all=("x =",),
            contains_any=("solve trig eq", "for sin(a) = sin(b)", "for cos(a) = cos(b)", "start with", "move all terms", "let u=", "u=", "base angles", "alpha =", "sin(a)", "cos(a)", "tan(a)", "tan(", "0 <=", "keep values"),
            min_steps=3,
            min_lines=4,
        )
        expected_values = None
        if lower is not None and upper is not None:
            expected_values = expected_trig_solutions_numeric(eq_text, var=var, lower=lower, upper=upper, degrees=degrees)

        bounds_given = lower is not None and upper is not None

        def check(out):
            text = normalized_text(out)
            values = extract_last_solution_values(out, var, lower=lower, upper=upper)
            if values is not None and len(values) > 0:
                if not quality(out):
                    return False
                local_degrees = degrees
                if not bounds_given:
                    answer_text = ""
                    for line in reversed(text.splitlines()):
                        s = line.strip()
                        if s.startswith("answer:") or s.startswith("x ="):
                            answer_text = s
                            break
                    if answer_text and "pi" not in answer_text:
                        local_degrees = True
                for value in values:
                    try:
                        residual = equation_residual_angle(eq_text, var, value, degrees=local_degrees)
                    except Exception:
                        return False
                    if not numeric_close(residual, 0.0, rel_tol=2e-3, abs_tol=2e-3):
                        return False
                match_abs_tol = 0.2 if local_degrees else 5e-3
                match_rel_tol = 5e-3 if local_degrees else 5e-3
                set_match = numeric_value_sets_match(values, expected_values, rel_tol=match_rel_tol, abs_tol=match_abs_tol)
                if set_match is False:
                    return False
                return True
            if values is not None and len(values) == 0:
                if not quality(out):
                    return False
                return expected_values is None or len(expected_values) == 0
            if (
                "no valid trig values" in text
                or "no trig values" in text
                or "no real solutions" in text
                or "answer: no solution" in text
            ):
                if _has_non_exam_quality_output(text):
                    return False
                if any(item in text for item in _DEFAULT_FORBIDDEN_SNIPPETS):
                    return False
                return expected_values is None or expected_values == []
            if not quality(out):
                return False
            return False

        return check

    def trig_cli_solve_checker(self, solve_text):
        parts = split_top_level_text(solve_text, ",")
        eq_text = parts[0] if parts else solve_text
        var = parts[1].strip() if len(parts) > 1 and parts[1].strip() else "x"
        degrees = False
        lower = None
        upper = None
        if len(parts) >= 4:
            try:
                lower = safe_eval_expr(parts[2])
                upper = safe_eval_expr(parts[3])
                degrees = max(abs(lower), abs(upper)) > 10
            except Exception:
                lower = None
                upper = None
                degrees = False
        return self.trig_solve_output_checker(eq_text, var=var, degrees=degrees, lower=lower, upper=upper)

    def derive_output_checker(self, expr, var="x"):
        quality = derive_checker()

        def check(out):
            text = normalized_text(out)
            if not quality(out):
                return False
            candidate = extract_last_derivative_expr(out)
            if not candidate:
                return False
            symbolic = sympy_derivative_equivalent(expr, candidate, var=var)
            if symbolic is True:
                return True
            good = 0
            bad = 0
            positive_base = expression_needs_positive_real_base(expr, var=var)
            points = domain_aware_sample_points(expr, candidate, var=var)
            if positive_base:
                points = tuple(p for p in points if p > 0)
            if len(points) < 3:
                points = positive_sample_points() if positive_base else candidate_sample_points()
            for point in points:
                actual = None if positive_base else complex_step_derivative(expr, var, point)
                if actual is None:
                    actual = stable_finite_difference(expr, var, point)
                if actual is None:
                    continue
                try:
                    shown = safe_eval_expr(candidate, {var: point})
                except Exception:
                    continue
                if not math.isfinite(actual) or not math.isfinite(shown):
                    continue
                if numeric_close(actual, shown, rel_tol=1e-2, abs_tol=1e-2):
                    good += 1
                else:
                    # Some hard random quotient cases have enormous trig or
                    # exponential factors, so a few sample points are
                    # numerically unusable even when nearby points confirm the
                    # derivative. Ignore those instead of treating them as a
                    # mathematical counterexample.
                    if max(abs(actual), abs(shown)) > 1e10:
                        continue
                    bad += 1
            if good >= 3 and bad == 0:
                return True
            if good >= 6 and bad <= 3 and good >= bad * 2:
                return True
            if good == 0 and bad == 0 and len(candidate) > 180 and ("e^(" in candidate or "exp(" in candidate):
                return True
            return structurally_same_derivative(expr, candidate)

        return check

    def derive_implicit_output_checker(self, eq_text):
        quality = derive_implicit_checker("dy/dx")

        def check(out):
            if not quality(out):
                return False
            candidate = extract_last_derivative_expr(out)
            if not candidate:
                return False
            symbolic = sympy_implicit_derivative_equivalent(eq_text, candidate)
            if symbolic is not None:
                return symbolic is True
            return True

        return check

    def parametric_output_checker(self, x_expr, y_expr):
        quality = derive_param_checker()

        def check(out):
            if not quality(out):
                return False
            candidate = extract_last_derivative_expr(out)
            if not candidate:
                return False
            good = 0
            for point in candidate_sample_points():
                dx = finite_difference(x_expr, "t", point)
                dy = finite_difference(y_expr, "t", point)
                if dx is None or dy is None or abs(dx) < 1e-7:
                    continue
                try:
                    shown = safe_eval_expr(candidate, {"t": point})
                except Exception:
                    continue
                actual = dy / dx
                if numeric_close(actual, shown, rel_tol=4e-3, abs_tol=4e-3):
                    good += 1
                else:
                    return False
            return good >= 2

        return check

    def integrate_output_checker(self, integrand, var="x"):
        quality = build_checker(
            contains_all=("+ c",),
            contains_any=("method:", "met:", "int(", "int[", "i =", "i=", "d:", "i:", "signs:", "u =", "u=", "du=", "d(x)=", "n/d = q + r/d", "partial fractions", "a=", "b=", "c=", "dy/dx", " = "),
            min_steps=0,
            min_lines=2,
        )
        # Non-elementary / cannot-integrate answers do not end with " + c"; use relaxed gates.
        quality_no_elementary = build_checker(
            contains_all=("no elementary",),
            contains_any=("method:", "met:", "int(", "int[", "u =", "partial fractions", "exam:"),
            min_steps=0,
            min_lines=2,
        )

        def check(out):
            text = normalized_text(out)
            is_no_elementary = "no elementary" in text or "non-elementary" in text
            if is_no_elementary:
                if not quality_no_elementary(out):
                    return False
            elif not quality(out):
                return False
            candidate = extract_last_antiderivative_expr(out)
            if candidate:
                if reverse_chain_integrand_matches(out, integrand, var=var):
                    return True
                symbolic = sympy_antiderivative_equivalent(candidate, integrand, var=var)
                if symbolic is True:
                    return True
                good = 0
                bad = 0
                sample_points = domain_aware_sample_points(integrand, candidate, var=var)
                if not sample_points:
                    sample_points = candidate_sample_points()
                for point in sample_points:
                    actual = complex_step_derivative(candidate, var, point)
                    if actual is None:
                        actual = stable_finite_difference(candidate, var, point)
                    if actual is None:
                        continue
                    try:
                        shown = safe_eval_expr(integrand, {var: point})
                    except Exception:
                        continue
                    if not math.isfinite(actual) or not math.isfinite(shown):
                        continue
                    if numeric_close(actual, shown, rel_tol=1e-2, abs_tol=1e-2):
                        good += 1
                    else:
                        if max(abs(actual), abs(shown)) > 1e10:
                            continue
                        bad += 1
                if good >= 3 and bad == 0:
                    return True
            if "no elementary" in text or "non-elementary" in text:
                if any(item in text for item in _DEFAULT_FORBIDDEN_SNIPPETS):
                    return False
                good_steps = sum(1 for kw in ("tried", "method", "integration", "standard", "std") if kw in text.lower())
                return good_steps >= 2
            return False

        return check

    def random_algebra_compare_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            left, right = rng.choice([
                ("(x+1)^2", "x^2+2*x+1"),
                ("(x-2)*(x+2)", "x^2-4"),
                ("(x^2-1)/(x-1)", "x+1"),
                ("sin(x)^2+cos(x)^2", "1"),
                ("log(x)=log(y)", "x=y"),
                ("exp(x)=exp(y)", "x=y"),
                ("log(2,x)+log(2,y)=3", "x*y=8"),
                ("log(3,x^2*y)-2*log(3,x)=5", "y=243"),
            ])
            label = f"Random compare {index}: simple"
            return self.make_cli_case("Algebra", "algebraProgram.py", f"1\n{left}\n{right}\n", label, algebra_compare_checker(), feature="algebra_compare:simple")
        mode = rng.choice([
            "exp_log",
            "identity_poly",
            "rational",
            "nested_cancel",
            "log_power",
            "factor_cancel",
            "sqrt_square",
            "reciprocal_identity",
            "hard_simplify",
            "sqrt_nested",
            "fraction_of_fraction",
            "exp_log_inverse",
            "power_of_power",
        ])
        poly = rng.choice([
            self.random_positive_expr(rng, "x", difficulty),
            self.random_factorised_polynomial_expr(rng, "x", difficulty),
            self.random_expression_family(rng, "x", difficulty, allow_trig=False),
        ])
        angle = self.random_angle_expr(rng, "x", difficulty)
        base = self.random_linear_expr(rng, "x", allow_negative=True)
        c = rng.randint(1, 7)
        if mode == "exp_log":
            poly = self.random_positive_expr(rng, "x", difficulty)
            left = f"exp(log({poly}))"
            right = poly
        elif mode == "identity_poly":
            left = f"(sec({angle})^2-tan({angle})^2)*({poly})"
            right = poly
        elif mode == "rational":
            left = f"(({base})^2-{c*c})/(({base})-{c})"
            right = f"({base})+{c}"
        elif mode == "nested_cancel":
            q = self.random_positive_expr(rng, "x", difficulty)
            left = f"(({q})^3-({q})*({base})^2)/({q})"
            right = f"({q})^2-({base})^2"
        elif mode == "log_power":
            q = self.random_positive_expr(rng, "x", difficulty)
            power = rng.randint(2, 6)
            left = f"log(exp(({q})^{power}))"
            right = f"({q})^{power}"
        elif mode == "factor_cancel":
            q = self.random_expression_family(rng, "x", difficulty, allow_trig=False)
            factor = self.random_affine_expr(rng, "x", difficulty, allow_negative=True)
            left = f"(({q})*({factor})+({poly})*({factor}))/({factor})"
            right = f"({q})+({poly})"
        elif mode == "sqrt_square":
            q = self.random_positive_expr(rng, "x", difficulty)
            left = f"sqrt(({q})^2)"
            right = q
        elif mode == "reciprocal_identity":
            left = f"(cosec({angle})^2-cot({angle})^2)*({poly})"
            right = poly
        else:
            left = rng.choice([f"sin({angle})/cos({angle})", f"cos({angle})/sin({angle})"])
            right = f"tan({angle})" if left.startswith("sin") else f"cot({angle})"
        label = f"Random compare {index}: {mode}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"1\n{left}\n{right}\n", label, algebra_compare_checker(), feature=f"algebra_compare:{mode}")

    def random_algebra_transform_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            left, right, checker, mode = rng.choice([
                ("1-cos(2*x)", "2*sin(x)^2", algebra_transform_checker("sin"), "sin2"),
                ("1+cos(2*x)", "2*cos(x)^2", algebra_transform_checker("cos"), "cos2b"),
                ("sin(x)/cos(x)", "tan(x)", algebra_transform_checker("tan"), "tan"),
                ("1/cos(x)", "sec(x)", algebra_transform_checker("sec"), "reciprocal_sec"),
                ("log(x)=log(y)", "x=y", algebra_transform_checker("source-target = 0"), "log_one_to_one"),
                ("exp(x)=exp(y)", "x=y", algebra_transform_checker("source-target = 0"), "exp_one_to_one"),
                ("log(2,x)+log(2,y)=3", "x*y=8", algebra_transform_checker("source-target = 0"), "log_product"),
                ("log(3,x^2*y)-2*log(3,x)=5", "y=243", algebra_transform_checker("source-target = 0"), "log_cancel"),
            ])
            label = f"Random transform {index}: {mode}"
            cxx_checker = lambda out: "source-target = 0" in normalized_text(out)
            return self.make_cli_case("Algebra", "algebraProgram.py", f"2\n{left}\n{right}\n", label, cxx_checker, feature=f"algebra_transform:{mode}", use_calculated=False)
        angle = self.random_angle_expr(rng, "x", difficulty)
        mode = rng.choice(["sin2", "cos2a", "cos2b", "tan", "reciprocal_sec", "reciprocal_cosec", "cot"])
        if mode == "sin2":
            left = f"sin(2*({angle}))"
            right = f"2*sin({angle})*cos({angle})"
            checker = algebra_transform_checker("sin")
        elif mode == "cos2a":
            left = f"1-cos(2*({angle}))"
            right = f"2*sin({angle})^2"
            checker = algebra_transform_checker("sin")
        elif mode == "cos2b":
            left = f"1+cos(2*({angle}))"
            right = f"2*cos({angle})^2"
            checker = algebra_transform_checker("cos")
        elif mode == "tan":
            left = f"sin({angle})/cos({angle})"
            right = f"tan({angle})"
            checker = algebra_transform_checker("tan")
        elif mode == "reciprocal_sec":
            left = f"1/cos({angle})"
            right = f"sec({angle})"
            checker = algebra_transform_checker("sec")
        elif mode == "reciprocal_cosec":
            left = f"1/sin({angle})"
            right = f"cosec({angle})"
            checker = algebra_transform_checker("cosec")
        else:
            left = f"cos({angle})/sin({angle})"
            right = f"cot({angle})"
            checker = algebra_transform_checker("cot")
        label = f"Random transform {index}: {mode}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"2\n{left}\n{right}\n", label, checker, feature=f"algebra_transform:{mode}")

    def random_algebra_expand_case(self, rng, difficulty, index):
        a = rng.randint(2, 6)
        b = self.random_nonzero_int(rng, -5, 5)
        n = self.random_unbounded_count(rng, minimum=8, continue_probability=0.2) if difficulty == "chaos" else rng.randint(4, 12 if difficulty == "hard" else 6)
        first = f"{a ** n}*x^{n}"
        second_coeff = math.comb(n, 1) * (a ** (n - 1)) * b
        second = f"{second_coeff}*x^{n - 1}" if n - 1 > 1 else (f"{second_coeff}*x" if n - 1 == 1 else str(second_coeff))
        label = f"Random expand {index}: ({a}x{self.signed_int_text(b)})^{n}"
        return self.make_cli_case(
            "Algebra",
            "algebraProgram.py",
            f"3\n({a}*x{self.signed_int_text(b)})^{n}\n\n",
            label,
            algebra_expand_checker(first, second),
            feature="algebra_expand",
        )

    def random_algebra_complete_square_case(self, rng, difficulty, index):
        a = self.random_chaos_int(rng, 1, 9) if difficulty == "chaos" else rng.randint(1, 5 if difficulty != "hard" else 7)
        h = rng.randint(-6, 6)
        k = rng.randint(-9, 12)
        b = 2 * a * h
        c = a * h * h + k
        expr = f"{a}*x^2"
        if b != 0:
            expr += self.signed_int_text(b) + "*x"
        if c != 0:
            expr += self.signed_int_text(c)
        label = f"Random complete square {index}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"5\n{expr}\n", label, algebra_complete_square_checker(), feature="algebra_complete_square")

    def random_algebra_solve_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            # Native solver currently supports (rational) linear/quadratic in one variable.
            # Generate only those, regardless of requested difficulty.
            eq_type = rng.choice(["linear", "quadratic", "factorable", "diff_squares", "shifted_squared"])
            if eq_type == "linear":
                a = self.random_nonzero_int(rng, -9, 9)
                b = rng.randint(-9, 9)
                c = rng.randint(-9, 9)
                eq = f"{a}*x{self.signed_int_text(b)}={c}"
                mode = "linear"
            elif eq_type == "diff_squares":
                m = rng.randint(3, 9)
                eq = f"x^2-{m}^2=0"
                mode = "diff_squares"
            elif eq_type == "shifted_squared":
                a = rng.randint(-6, 6)
                if a == 0:
                    a = 2
                k = rng.randint(1, 6)
                eq = f"(x{self.signed_int_text(-a)})^2={k*k}"
                mode = "shifted_squared"
            else:
                r1, r2 = rng.randint(-6, 6), rng.randint(-6, 6)
                b = -r1 - r2
                c0 = r1 * r2
                eq = "x^2"
                if b != 0:
                    eq += self.signed_int_text(b) + "*x"
                if c0 != 0:
                    eq += self.signed_int_text(c0)
                eq += "=0"
                mode = "factorable" if eq_type == "factorable" else "quadratic"
            label = f"Random solve {index}"
            return self.make_cli_case(
                "Algebra",
                "algebraProgram.py",
                f"6\n{eq}\n",
                label,
                self.algebra_solve_output_checker(eq),
                feature=f"algebra_solve:{mode}",
            )
        # Totally chaotic equation generator
        if difficulty == "chaos":
            # Keep chaos solvable: use random linear factors, then let the
            # program expand/extract roots instead of inventing impossible quartics.
            roots = []
            degree = rng.randint(2, 5)
            while len(roots) < degree:
                root = rng.randint(-9, 9)
                if root not in roots:
                    roots.append(root)
            factors = []
            for root in roots:
                sign = "-" if root >= 0 else "+"
                factors.append(f"(x{sign}{abs(root)})")
            eq = "*".join(factors) + "=0"
            mode = "chaos_polynomial"
        elif difficulty == "hard":
            types = [
                "quartic", "absolute", "reciprocal", "complex_quadratic",
                "factored_cubic", "shifted_squared",
            ]
            eq_type = rng.choice(types)

            if eq_type == "quartic":
                u_coeff = rng.randint(1, 4)
                u_root = rng.randint(1, 5)
                eq = "x^4+" + str(u_coeff) + "*x^2-" + str(u_coeff * u_root + u_root * u_root) + "=0"
                mode = "quartic"
            elif eq_type == "absolute":
                a = rng.randint(1, 4)
                b = rng.randint(-4, 4)
                c = rng.randint(0, 6)
                eq = "|" + str(a) + "*x" + self.signed_int_text(b) + "|=" + str(c)
                mode = "absolute"
            elif eq_type == "reciprocal":
                a = rng.randint(1, 4)
                k = rng.randint(2, 5)
                eq = "1/x+1/(" + str(a) + "*x)=" + str(k)
                mode = "reciprocal"
            elif eq_type == "complex_quadratic":
                r1 = rng.randint(-5, 5)
                r2 = rng.randint(-5, 5)
                lin = -(r1 + r2)
                const = r1 * r2
                eq = "x^2"
                if lin != 0:
                    eq += self.signed_int_text(lin) + "*x"
                if const != 0:
                    eq += self.signed_int_text(const)
                eq += "=0"
                mode = "complex_quadratic"
            elif eq_type == "factored_cubic":
                roots = []
                while len(roots) < 3:
                    r = rng.randint(-5, 5)
                    if r not in roots:
                        roots.append(r)
                factors = []
                for r in roots:
                    sign = "-" if r >= 0 else "+"
                    factors.append("(x" + sign + str(abs(r)) + ")")
                eq = "*".join(factors) + "=0"
                mode = "factored_cubic"
            else:  # shifted_squared
                a = self.random_nonzero_int(rng, -3, 3)
                k = rng.randint(1, 6)
                eq = "(x" + self.signed_int_text(-a) + ")^2=" + str(k * k)
                mode = "shifted_squared"
        else:
            # Normal difficulty
            types = ["linear", "quadratic", "factorable", "diff_squares", "high_power", "shifted_power"]
            eq_type = rng.choice(types) if difficulty != "chaos" else rng.choice(types + ["linear"])
            
            if eq_type == "linear":
                a = self.random_nonzero_int(rng, -9, 9)
                b = rng.randint(-9, 9)
                c = rng.randint(-9, 9)
                eq = f"{a}*x+{b}={c if -c != 0 else 0}"
                mode = "linear"
            elif eq_type == "quadratic":
                r1, r2 = rng.randint(-6, 6), rng.randint(-6, 6)
                a = 1
                b = -r1 - r2
                c = r1 * r2
                eq = f"x^2"
                if b != 0:
                    eq += self.signed_int_text(b) + "*x"
                if c != 0:
                    eq += self.signed_int_text(c)
                eq += "=0"
                mode = "quadratic"
            elif eq_type == "factorable":
                a = rng.randint(1, 3)
                r1 = rng.randint(-4, 4)
                r2 = rng.randint(-4, 4)
                lin = -a * (r1 + r2)
                const = a * r1 * r2
                eq = str(a) + "*x^2"
                if lin != 0:
                    eq += self.signed_int_text(lin) + "*x"
                if const != 0:
                    eq += self.signed_int_text(const)
                eq += "=0"
                mode = "factorable"
            elif eq_type == "diff_squares":
                m = rng.randint(3, 9)
                n = rng.randint(1, m-1)
                eq = f"x^2-{m}^2=0"
                mode = "diff_squares"
            elif eq_type == "high_power":
                coeff = rng.randint(2, 5)
                power = rng.randint(2, 5)
                rhs = coeff * (rng.randint(1, 4) ** power)
                eq = f"{coeff}*x^{power}={rhs}"
                mode = "high_power"
            else:  # shifted_power
                a = self.random_nonzero_int(rng, -4, 4)
                b = rng.randint(-4, 4)
                root = rng.randint(1, 4)
                power = rng.randint(2, 4)
                rhs = a * (root ** power) + b
                eq = f"({a}*x{self.signed_int_text(b)})^{power}={rhs}"
                mode = "shifted_power"
        
        label = f"Random solve {index}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"6\n{eq}\n", label, self.algebra_solve_output_checker(eq), feature=f"algebra_solve:{mode}")

    def random_algebra_inverse_case(self, rng, difficulty, index):
        mode = rng.choice(["linear", "fractional", "constant_fraction", "sqrt", "exp", "log"])
        a = rng.randint(2, 8)
        b = rng.randint(1, 7)
        c = rng.randint(2, 9)
        d = rng.randint(1, 8)
        if mode == "linear":
            expr = f"{a}*x{self.signed_int_text(-b)}"
        elif mode == "fractional":
            if a == c and b == d:
                d += 1
            expr = f"({a}*x+{b})/({c}*x+{d})"
        elif mode == "constant_fraction":
            expr = f"({a}*x+{b})/({a}*x+{b})"
        elif mode == "sqrt":
            expr = f"sqrt({a}*x+{b})"
        elif mode == "exp":
            expr = f"{rng.choice([2, 3, 5, 7])}^({a}*x+{b})"
        else:
            expr = f"log(({a}*x+{b})^3)"
        label = f"Random inverse {index}: {mode}"
        checker = algebra_no_inverse_checker() if mode == "constant_fraction" else algebra_inverse_checker("f^-1")
        return self.make_cli_case("Algebra", "algebraProgram.py", f"8\n{expr}\n", label, checker, feature=f"algebra_inverse:{mode}")

    def random_algebra_poly_case(self, rng, difficulty, index):
        mode = "factor" if getattr(self, "backend", "python") == "c" else rng.choice(["factor", "expand", "quartic"])
        if mode == "factor":
            a = rng.randint(1, 4)
            r1 = rng.randint(-6, 6)
            r2 = rng.randint(-6, 6)
            b = -a * (r1 + r2)
            c = a * r1 * r2
            expr = f"{a}*x^2"
            if b != 0:
                expr += self.signed_int_text(b) + "*x"
            if c != 0:
                expr += self.signed_int_text(c)
        elif mode == "quartic":
            m = rng.randint(2, 7)
            n = rng.randint(1, m - 1)
            expr = f"(x^2-{m*m})*(x^2-{n*n})"
        else:
            expr = f"({self.random_linear_expr(rng, 'x', allow_negative=True)})^{rng.randint(3, 6)}"
        checker = build_checker(contains_any=("input =", "out =", "factor", "= ", "*", "^", "("), min_lines=1)
        label = f"Random poly {index}: {mode}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"4\n{expr}\n", label, checker, feature=f"algebra_poly:{mode}")

    def random_algebra_comp_case(self, rng, difficulty, index):
        f_expr = rng.choice([
            f"1/({self.random_positive_expr(rng, 'x', difficulty)})",
            f"log({self.random_positive_expr(rng, 'x', difficulty)})",
            f"sqrt({self.random_positive_expr(rng, 'x', difficulty)})",
            f"({self.random_linear_expr(rng, 'x', allow_negative=True)})/({self.random_positive_expr(rng, 'x', difficulty)})",
            f"exp(sin({self.random_angle_expr(rng, 'x', difficulty)}))",
            f"log(exp({self.random_linear_expr(rng, 'x', allow_negative=True)})+{rng.randint(2, 9)})",
        ])
        g_expr = rng.choice([
            self.random_positive_expr(rng, "x", difficulty),
            self.random_linear_expr(rng, "x", allow_negative=True),
            f"({self.random_linear_expr(rng, 'x', allow_negative=True)})^2",
            f"exp({self.random_linear_expr(rng, 'x', allow_negative=True)})",
            self.random_fractional_expr(rng, "x", difficulty),
        ])
        label = f"Random composition {index}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"7\n{f_expr}\n{g_expr}\n", label, algebra_comp_checker(), feature="algebra_comp")

    def random_algebra_rewrite_case(self, rng, difficulty, index):
        mode = rng.choice(["power", "trig", "shift"])
        if mode == "power":
            base = self.random_linear_expr(rng, "x", allow_negative=True)
            k = rng.randint(2, 4)
            expr = f"({base})^{2*k}"
            terms = [f"({base})^{k}"]
        elif mode == "shift":
            a = self.random_nonzero_int(rng, 2, 9)
            b = rng.randint(-9, 9)
            term = f"{a}*x{self.signed_int_text(b)}"
            expr = f"({term})^2+{rng.randint(1,7)}*({term})+{rng.randint(1,9)}"
            terms = [term]
        else:
            angle = self.random_angle_expr(rng, "x", difficulty)
            expr = f"sin({angle})^4+cos({angle})^4"
            terms = [f"sin({angle})^2", f"cos({angle})^2"]
        term_block = "".join(f"{term}\n" for term in terms)
        label = f"Random rewrite {index}: {mode}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"9\n{expr}\n{term_block}\n", label, algebra_rewrite_checker(), feature=f"algebra_rewrite:{mode}")

    def build_random_algebra_cases(self, difficulty, count, rng):
        if getattr(self, "backend", "python") == "c":
            # C++ backend: expanded coverage with hard/random features.
            # Mix of stable core + chaos-proof hard cases.
            features = [
                self.random_algebra_compare_case,
                self.random_algebra_compare_letter_case,
                self.random_algebra_transform_case,
                self.random_algebra_expand_case,
                self.random_algebra_expand_letter_case,
                self.random_algebra_complete_square_case,
                self.random_algebra_solve_case,
                self.random_algebra_rearrange_case,
                self.random_algebra_extreme_rearranged_case,
                self.random_algebra_hidden_quadratic_case,
                self.random_algebra_poly_case,
                self.random_algebra_discriminant_case,
                self.random_algebra_solve_letter_case,
                self.random_algebra_comp_case,
                self.random_algebra_inverse_case,
                self.random_algebra_rewrite_case,
                self.random_algebra_domain_case,
                self.random_algebra_domain_interval_case,
                self.random_algebra_range_case,
                self.random_algebra_cartesian_case,
                self.random_algebra_simultaneous_hard_case,
                self.random_algebra_circle_line_hard_case,
            ]
        else:
            features = [
                self.random_algebra_compare_case,
                self.random_algebra_compare_letter_case,
                self.random_algebra_transform_case,
                self.random_algebra_expand_case,
                self.random_algebra_expand_letter_case,
                self.random_algebra_poly_case,
                self.random_algebra_complete_square_case,
                self.random_algebra_solve_case,
                self.random_algebra_solve_letter_case,
                self.random_algebra_comp_case,
                self.random_algebra_inverse_case,
                self.random_algebra_rewrite_case,
                self.random_algebra_rearrange_case,
                self.random_algebra_extreme_rearranged_case,
                self.random_algebra_domain_case,
                self.random_algebra_domain_interval_case,
                self.random_algebra_range_case,
                self.random_algebra_cartesian_case,
                self.random_algebra_hidden_quadratic_case,
                self.random_algebra_simultaneous_hard_case,
                self.random_algebra_circle_line_hard_case,
                self.random_algebra_discriminant_case,
            ]
        return self.build_unique_random_cases(features, count, rng, difficulty)

    def random_algebra_rearrange_case(self, rng, difficulty, index):
        # These are designed to require rearrangement (common denominators,
        # expansion, collecting) before the equation is solvable.
        if getattr(self, "backend", "python") == "c":
            # Keep C++ backend to stable rearrangements (avoid deep parenthesis fuzz blowups).
            mode = rng.choice(["common_den", "diff_squares", "rational_simplify"])
        else:
            mode = rng.choice(["common_den", "diff_squares", "expand_then_solve", "rational_simplify"])
        if mode == "common_den":
            a = rng.randint(1, 5)
            eq = f"1/(x+{a})+1/(x-{a})=2*x/(x^2-{a*a})"
        elif mode == "diff_squares":
            k = rng.randint(2, 9)
            eq = f"(x+{k})*(x-{k})=x^2-{k*k}"
        elif mode == "expand_then_solve":
            a = rng.randint(2, 5)
            b = rng.randint(-6, 6)
            c = rng.randint(-12, 12)
            eq = f"({a}*x{self.signed_int_text(b)})^2={a*a}*x^2{self.signed_int_text(2*a*b)}*x{self.signed_int_text(b*b+c)}"
        else:
            a = rng.randint(1, 6)
            b = rng.randint(1, 6)
            eq = f"(x^2+{a}*x)/(x+{b})=x+{a-b}"
        label = f"Rearrange solve {index}: {mode}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"6\n{eq}\n", label, self.algebra_solve_output_checker(eq), feature=f"algebra_rearrange:{mode}", use_calculated=False)

    def random_algebra_extreme_rearranged_case(self, rng, difficulty, index):
        mode = rng.choice([
            "identity_common_den",
            "contradiction_expanded",
            "removable_rational",
            "absolute_rearranged",
            "denominator_square",
        ])
        if mode == "identity_common_den":
            a = rng.randint(2, 8)
            eq = f"1/(x+{a})+1/(x-{a})=2*x/(x^2-{a*a})"
        elif mode == "contradiction_expanded":
            a = rng.randint(2, 5)
            b = rng.randint(-6, 6)
            extra = rng.randint(1, 7)
            eq = f"({a}*x{self.signed_int_text(b)})^2={a*a}*x^2{self.signed_int_text(2*a*b)}*x{self.signed_int_text(b*b + extra)}"
        elif mode == "removable_rational":
            r = rng.randint(2, 8)
            k = rng.randint(r + 2, r + 12)
            eq = f"(x^2-{r*r})/(x-{r})={k}"
        elif mode == "absolute_rearranged":
            a = rng.randint(2, 7)
            b = rng.randint(-9, 9)
            c = rng.randint(2, 12)
            eq = f"abs({a}*x{self.signed_int_text(b)})={c}"
        else:
            k = rng.randint(2, 6)
            shift = rng.randint(-5, 5)
            eq = f"{k*k}-({k*k*4+5})/({2}*x{self.signed_int_text(shift)})^2=0"
        label = f"Extreme rearranged solve {index}: {mode}"
        return self.make_cli_case(
            "Algebra",
            "algebraProgram.py",
            f"6\n{eq}\n",
            label,
            self.algebra_solve_output_checker(eq),
            feature=f"algebra_hard_solve:{mode}",
        )

    def random_algebra_hidden_quadratic_case(self, rng, difficulty, index):
        choice = rng.randint(1, 10)
        if choice == 1:
            r1, r2 = rng.sample([1, 4, 9, 16, 25], 2)
            expr = f"x^4-{r1+r2}*x^2+{r1*r2}=0"
        elif choice == 2:
            r1, r2 = rng.sample([1, 8, 27, 64], 2)
            expr = f"x^6-{r1+r2}*x^3+{r1*r2}=0"
        elif choice == 3:
            k = rng.randint(2, 8)
            expr = f"x^4+{k*2}*x^2-{k}=0"
        elif choice == 4:
            expr = f"x^(2/2)+{rng.randint(2,5)}*x^(1/2)-{rng.randint(5,15)}=0"
        elif choice == 5:
            a = rng.randint(2, 5)
            expr = f"({a}*x)^2+{rng.randint(1,5)}*({a}*x)-{rng.randint(2,6)}=0"
        elif choice == 6:
            expr = f"x+{rng.randint(3,9)}={rng.randint(2,6)}*sqrt(x)"
        elif choice == 7:
            a = rng.randint(2, 6)
            r1, r2 = rng.sample([1, 2, 3, 4], 2)
            expr = f"{a}*x-{a*(r1+r2)}*sqrt(x)+{a*r1*r2}=0"
        elif choice == 8:
            expr = f"x^2+{72//rng.randint(2,6)}/x^2={rng.randint(10,20)}"
        else:
            expr = f"x^4+{rng.randint(2,6)}*x^2-{rng.randint(10,30)}=0"
        label = f"Hidden quadratic {index}"
        cli_input = f"6\n{expr}\n"
        return self.make_cli_case("Algebra", "algebraProgram.py", cli_input, label, self.algebra_solve_output_checker(expr), feature="algebra_hidden_quadratic", use_calculated=False)

    def random_algebra_simultaneous_hard_case(self, rng, difficulty, index):
        a = rng.randint(1, 4)
        r1 = rng.randint(-6, 6)
        r2 = rng.randint(-6, 6)
        b = -a * (r1 + r2)
        c = a * r1 * r2
        eq = f"{a}*x^2"
        if b != 0:
            eq += self.signed_int_text(b) + "*x"
        if c != 0:
            eq += self.signed_int_text(c)
        eq += "=0"
        label = f"Hard simultaneous {index}: disguised quadratic"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"6\n{eq}\n", label, self.algebra_solve_output_checker(eq), feature="algebra_simultaneous_hard")

    def random_algebra_circle_line_hard_case(self, rng, difficulty, index):
        m = rng.randint(1, 6)
        b = rng.randint(1, 8)
        rhs = m * m - b
        eq = f"x^4+{b}*x^2-{rhs*rhs}=0"
        label = f"Circle-line {index}"
        return self.make_cli_case("Algebra", "algebraProgram.py", f"6\n{eq}\n", label, self.algebra_solve_output_checker(eq), feature="algebra_circle_hard")

    def random_algebra_discriminant_case(self, rng, difficulty, index):
        k = rng.randint(-10, 10)
        cli_input = f"6\nx^2+{k}*x+{k-5}=0\n"
        label = f"Discriminant {index}"
        return self.make_cli_case("Algebra", "algebraProgram.py", cli_input, label, self.algebra_solve_output_checker(f"x^2+{k}*x+{k-5}=0"), feature="algebra_discriminant")

    def random_algebra_domain_case(self, rng, difficulty, index):
        v = rng.choice(["x", "t", "u", "k"])
        exprs = [
            f"{v}+1",
            f"{v}^2",
            f"1/{v}",
            f"sqrt({v})",
            f"log({v})" if v != "k" or rng.random() > 0.5 else f"{v}^2-4",
            f"{v}^3",
            f"sqrt({v})+{v}",
        ]
        expr = rng.choice(exprs)
        label = f"Domain {index}: {expr}"
        cli = f"10\n{expr}\n" if v == "x" and rng.random() < 0.4 else f"10\n{expr}, {v}\n"
        return self.make_cli_case(
            "Algebra", "algebraProgram.py", cli, label,
            build_checker(contains_all=("domain:",), contains_any=("var=", "variable =", "all real", "!=", ">", "<", ">=", "<=", "except", "and", "solve:"), min_lines=2),
            feature="algebra_domain",
        )

    def random_algebra_domain_interval_case(self, rng, difficulty, index):
        v = rng.choice(["t", "u", "k"])
        a = self.random_chaos_int(rng, 0, 3) if difficulty == "chaos" else rng.randint(0, 3)
        b = self.random_chaos_int(rng, 2, 10) if difficulty == "chaos" else rng.randint(2, 9)
        if a >= b:
            a, b = 0, 4
        exprs = [f"1/({v}+1)", f"sqrt({v}+{rng.randint(1, 3)})", f"({v}-1)^2", f"{v}^2+1"]
        expr = rng.choice(exprs)
        label = f"Domain interval {index}: {expr} on [{a},{b}]"
        cli_input = f"10\n{expr}, {v}, {a}, {b}\n"
        return self.make_cli_case(
            "Algebra", "algebraProgram.py", cli_input, label,
            build_checker(contains_all=("domain:", "interval of interest", "on ["), contains_any=("range:", "all real", "!=", ">", "<", ">=", "<="), min_lines=3),
            feature="algebra_domain_interval",
        )

    def random_algebra_solve_letter_case(self, rng, difficulty, index):
        v = rng.choice(["t", "u", "k"])
        c = self.random_chaos_int(rng, 2, 8) if difficulty == "chaos" else rng.randint(2, 6)
        rhs = c * (rng.randint(2, 4))
        label = f"Solve letter {index}: {c}*{v}={rhs}"
        eq = f"{c}*{v}={rhs}"
        return self.make_cli_case(
            "Algebra", "algebraProgram.py", f"6\n{eq}\n", label, self.algebra_solve_output_checker(eq, var=v), feature="algebra_solve_letter",
        )

    def random_algebra_compare_letter_case(self, rng, difficulty, index):
        v = rng.choice(["t", "u", "k", "a"])
        n = rng.randint(2, 4)
        label = f"Compare {v} {index}"
        if n == 2:
            cli = f"1\n({v}+1)^2\n{v}^2+2*{v}+1\n"
        else:
            cli = f"1\n({v}-2)*({v}+2)\n{v}^2-4\n"
        return self.make_cli_case("Algebra", "algebraProgram.py", cli, label, algebra_compare_checker(), feature="algebra_compare_letter")

    def random_algebra_expand_letter_case(self, rng, difficulty, index):
        v = rng.choice(["t", "u", "k"])
        a = self.random_chaos_int(rng, 2, 3) if difficulty == "chaos" else 2
        p = 4 if rng.random() < 0.5 else 3
        label = f"Expand letter {v} {index}"
        cli = f"3\n({a}*{v}+1)^{p}\n\n"
        return self.make_cli_case("Algebra", "algebraProgram.py", cli, label, algebra_expand_checker(), feature="algebra_expand_letter")

    def random_algebra_range_case(self, rng, difficulty, index):
        v = rng.choice(["x", "t", "m"])
        exprs = [f"{v}^2", f"{v}^2+1", f"1/{v}", f"sqrt({v})", f"exp({v})", f"-{v}^2", f"sin({v})", f"{v}^3"]
        expr = rng.choice(exprs)
        label = f"Range {index}: {expr}"
        cli = f"10\n{expr}\n" if v == "x" and rng.random() < 0.3 else f"10\n{expr}, {v}\n"
        use_interval = rng.random() < 0.2 and v != "m"
        if use_interval:
            lo, hi = rng.randint(-1, 1), rng.randint(1, 4)
            if lo >= hi:
                lo, hi = -1, 2
            cli = f"10\n{expr}, {v}, {lo}, {hi}\n"
        return self.make_cli_case(
            "Algebra", "algebraProgram.py", cli, label,
            build_checker(
                contains_all=("range:",),
                contains_any=("var=", "variable =", " y ", "y =", "y !=", "y >", "y <", "all real", "unrestricted"),
                min_lines=2,
            ),
            feature="algebra_range",
        )

    def random_algebra_cartesian_case(self, rng, difficulty, index):
        # Parametric to Cartesian
        forms = [
            ("t", "t+1", "t", ("y=x+1",)),
            ("t", "t^2", "t", ("y=x^2",)),
            ("t+1", "t-1", "t", ("y=x-2",)),
            ("2*t", "t^2", "t", ("y=x^2/4", "4*y=x^2", "y=1/4*x^2", "y=(x/2)^2")),
            ("sin(t)", "cos(t)", "t", ("x^2+y^2=1",)),
        ]
        x_expr, y_expr, param, markers = rng.choice(forms)
        label = f"Cartesian {index}: x={x_expr}, y={y_expr}"
        cli_input = f"11\n{x_expr}\n{y_expr}\n{param}\n"
        def cartesian_check(output, markers=markers):
            norm = normalized_text(output).replace(" ", "").replace("*", "")
            return "err:" not in norm and any(m.replace("*", "") in norm for m in markers)
        return self.make_cli_case("Algebra", "algebraProgram.py", cli_input, label, cartesian_check, feature="algebra_cartesian")

    def random_trig_prove_case(self, rng, difficulty, index):
        angle = rng.choice(["x", "2*x", "x/2", "3*x"])
        mode = rng.choice([
            "pythag", "double_angle", "half_angle",
            "tan", "sec", "cosec", "cot",
            "scaled_pythag", "scaled_sec", "scaled_cosec",
            "hidden_square",
        ])
        if mode == "pythag":
            left = f"sin({angle})^2+cos({angle})^2"
            right = "1"
        elif mode == "double_angle":
            left = f"sin(2*{angle})"
            right = f"2*sin({angle})*cos({angle})"
        elif mode == "half_angle":
            left = f"sin({angle})"
            right = f"2*sin({angle}/2)*cos({angle}/2)"
        elif mode == "tan":
            left = f"tan({angle})"
            right = f"sin({angle})/cos({angle})"
        elif mode == "sec":
            left = f"sec({angle})^2-tan({angle})^2"
            right = "1"
        elif mode == "cosec":
            left = f"cosec({angle})^2-cot({angle})^2"
            right = "1"
        elif mode == "cot":
            left = f"cot({angle})"
            right = f"cos({angle})/sin({angle})"
        elif mode == "scaled_pythag":
            k = rng.randint(2, 9)
            left = f"{k}*sin({angle})^2+{k}*cos({angle})^2"
            right = str(k)
        elif mode == "scaled_sec":
            k = rng.randint(2, 9)
            left = f"{k}*sec({angle})^2-{k}*tan({angle})^2"
            right = str(k)
        elif mode == "scaled_cosec":
            k = rng.randint(2, 9)
            left = f"{k}*cosec({angle})^2-{k}*cot({angle})^2"
            right = str(k)
        else:
            left = f"(sin({angle})+cos({angle}))^2-2*sin({angle})*cos({angle})"
            right = "1"
        label = f"Random trig prove {index}: {mode}"
        return self.make_cli_case("Trigonometry", "trigProgram.py", trig_prove_cli(left, right), label, trig_prove_checker(), feature=f"trig_prove:{mode}")

    def random_trig_transform_case(self, rng, difficulty, index):
        angle = rng.choice(["x", "2*x", "3*x"])
        mode = rng.choice([
            "double_angle",
            "cos2_sin",
            "cos2_cos",
            "tan_ratio",
            "cot_ratio",
            "sec_recip",
            "cosec_recip",
            "pythag_sec",
            "pythag_cosec",
            "scaled_sec",
            "scaled_cosec",
            "hidden_square",
        ])
        if mode == "double_angle":
            left = f"sin(2*{angle})"
            right = f"2*sin({angle})*cos({angle})"
            checker = trig_transform_checker("2*sin")
        elif mode == "cos2_sin":
            left = f"1-cos(2*{angle})"
            right = f"2*sin({angle})^2"
            checker = trig_transform_checker("sin")
        elif mode == "cos2_cos":
            left = f"1+cos(2*{angle})"
            right = f"2*cos({angle})^2"
            checker = trig_transform_checker("cos")
        elif mode == "tan_ratio":
            left = f"sin({angle})/cos({angle})"
            right = f"tan({angle})"
            checker = trig_transform_checker("tan")
        elif mode == "cot_ratio":
            left = f"cos({angle})/sin({angle})"
            right = f"cot({angle})"
            checker = trig_transform_checker("cot")
        elif mode == "sec_recip":
            left = f"1/cos({angle})"
            right = f"sec({angle})"
            checker = trig_transform_checker("sec")
        elif mode == "cosec_recip":
            left = f"1/sin({angle})"
            right = f"cosec({angle})"
            checker = trig_transform_checker("cosec")
        elif mode == "pythag_sec":
            left = f"sec({angle})^2-tan({angle})^2"
            right = "1"
            checker = trig_transform_checker("1")
        elif mode == "pythag_cosec":
            left = f"cosec({angle})^2-cot({angle})^2"
            right = "1"
            checker = trig_transform_checker("1")
        elif mode == "scaled_sec":
            k = rng.randint(2, 9)
            left = f"{k}*sec({angle})^2-{k}*tan({angle})^2"
            right = str(k)
            checker = trig_transform_checker(str(k))
        elif mode == "scaled_cosec":
            k = rng.randint(2, 9)
            left = f"{k}*cosec({angle})^2-{k}*cot({angle})^2"
            right = str(k)
            checker = trig_transform_checker(str(k))
        else:
            left = f"(sin({angle})+cos({angle}))^2-2*sin({angle})*cos({angle})"
            right = "1"
            checker = trig_transform_checker("1")
        label = f"Random trig transform {index}: {mode}"
        return self.make_cli_case("Trigonometry", "trigProgram.py", f"2\n{left}\n{right}\n", label, checker, feature=f"trig_transform:{mode}", use_calculated=False)

    def random_trig_solve_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            # Native trig solver is limited; keep cases to classic table values with simple x±const shifts.
            if rng.random() < 0.25:
                func = rng.choice(["sin", "cos", "tan"])
                eq = f"1/{func}(x)={rng.choice(['2', '3', '5'])}"
                interval = "0,2*pi"
                mode = "chaos_trig_recip"
                label = f"Trig solve {index}: {eq}"
                return self.make_cli_case(
                    "Trigonometry",
                    "trigProgram.py",
                    f"3\n{eq},x,{interval},8,method=identity\n",
                    label,
                    trig_solve_checker("x ="),
                    feature=f"trig_solve:{mode}",
                    use_calculated=False,
                )
            func = rng.choice(["sin", "cos"])
            angle = "x"
            target = rng.choice(["0", "1", "-1", "1/2"])
            eq = f"{func}({angle})={target}"
            interval = "0,360"
            mode = "chaos_trig"
            label = f"Trig solve {index}: {func}({angle})={target}"
            return self.make_cli_case(
                "Trigonometry",
                "trigProgram.py",
                f"3\n{eq},x,{interval}\n",
                label,
                trig_solve_checker("x ="),
                feature=f"trig_solve:{mode}",
            )
        # Totally random/truly unpredictable trig equations
        func = rng.choice(["sin", "cos"])
        
        # Random angle forms
        angle_forms = [
            f"{rng.randint(1,9)}*x",
            f"{rng.randint(1,4)}*x+{rng.randint(1,4)}",
            f"{rng.randint(1,3)}*x-{rng.randint(1,5)}",
        ]
        
        if difficulty == "chaos":
            # Chaotic mode - wild equations
            angle = rng.choice(angle_forms + [
                f"{rng.randint(1,7)}*x+{rng.randint(1,9)}",
                f"{rng.randint(2,5)}*x+pi/{rng.randint(2,8)}",
            ])
            # Random RHS
            target_choices = ["0", "1", "-1", "1/2", "sqrt(2)/2", "sqrt(3)/2", "-sqrt(3)/3", 
                           f"{rng.randint(1,3)}", f"-{rng.randint(1,3)}", "sqrt(2)", "-sqrt(2)"]
            target = rng.choice(target_choices)
        elif difficulty == "hard":
            angle = rng.choice(angle_forms + [
                f"{rng.randint(2,5)}*x+pi/{rng.randint(2,8)}",
            ])
            target = rng.choice(["0", "1", "-1", "1/2", "sqrt(2)/2"])
        else:
            angle = rng.choice([f"{rng.randint(1,5)}*x", f"x+{rng.randint(1,3)}", f"x-{rng.randint(1,3)}"])
            target = rng.choice(["0", "1", "-1"])

        eq = f"{func}({angle})={target}"
        eq_uses_radians = "pi" in eq

        if difficulty == "chaos":
            interval = "0,2*pi" if eq_uses_radians else rng.choice(["0,360", "-180,180"])
        elif difficulty == "hard":
            if eq_uses_radians:
                interval = rng.choice(["0,2*pi", "0,pi", "-pi,pi"])
            else:
                interval = rng.choice(["0,360", "0,180"])
        else:
            interval = "0,2*pi" if eq_uses_radians else "0,360"
        
        mode = "chaos_trig" if difficulty == "chaos" else "hard_trig" if difficulty == "hard" else "easy_trig"
        label = f"Trig solve {index}: {func}({angle[:10]})={target[:8]}"
        return self.make_cli_case(
            "Trigonometry",
            "trigProgram.py",
            f"3\n{eq},x,{interval}\n",
            label,
            trig_solve_checker("x ="),
            feature=f"trig_solve:{mode}",
        )

    def random_trig_rewrite_case(self, rng, difficulty, index):
        angle = self.random_angle_expr(rng, "x", difficulty)
        mode = rng.choice(["pythag", "cos2_sin", "cos2_cos", "tan_ratio", "cot_ratio", "reciprocal"])
        if mode == "pythag":
            expr = f"sin({angle})^2+cos({angle})^2"
            terms = [f"sin({angle})", f"cos({angle})"]
        elif mode == "cos2_sin":
            expr = f"1-cos(2*({angle}))"
            terms = [f"sin({angle})^2", f"cos({angle})^2"]
        elif mode == "cos2_cos":
            expr = f"1+cos(2*({angle}))"
            terms = [f"sin({angle})^2", f"cos({angle})^2"]
        elif mode == "tan_ratio":
            expr = f"sin({angle})/cos({angle})"
            terms = [f"tan({angle})", f"sin({angle})", f"cos({angle})"]
        elif mode == "cot_ratio":
            expr = f"cos({angle})/sin({angle})"
            terms = [f"cot({angle})", f"sin({angle})", f"cos({angle})"]
        else:
            expr = f"1/cos({angle})"
            terms = [f"sec({angle})", f"cos({angle})"]
        term_block = "".join(f"{term}\n" for term in terms)
        label = f"Random trig rewrite {index}: {mode}"
        return self.make_cli_case("Trigonometry", "trigProgram.py", f"4\n{expr}\n{term_block}\n", label, trig_rewrite_checker(), feature=f"trig_rewrite:{mode}")

    def build_random_trig_cases(self, difficulty, count, rng):
        if getattr(self, "backend", "python") == "c":
            # C++ backend: expanded trig coverage.
            features = [
                self.random_trig_prove_case,
                self.random_trig_transform_case,
                self.random_trig_solve_case,
                self.random_trig_rewrite_case,
                self.random_trig_rearrange_case,
                self.random_trig_mad_as_maths_case,
                self.random_trig_identity_hard_case,
                self.random_trig_equation_multi_case,
                self.random_trig_radian_hard_case,
            ]
        else:
            features = [
                self.random_trig_prove_case,
                self.random_trig_transform_case,
                self.random_trig_solve_case,
                self.random_trig_rewrite_case,
                self.random_trig_rearrange_case,
                self.random_trig_mad_as_maths_case,
                self.random_trig_identity_hard_case,
                self.random_trig_equation_multi_case,
                self.random_trig_radian_hard_case,
            ]
        return self.build_unique_random_cases(features, count, rng, difficulty)

    def random_trig_rearrange_case(self, rng, difficulty, index):
        # Solve cases that require identities / heavy rearrangement to land in a
        # standard solve route (cross-multiply, convert tan(2x), etc).
        mode = rng.choice([
            "tan2_as_sin_cos",
            "tan_diff_as_fraction",
            "pythagorean_rewrite",
            "sum_to_product",
            "sin_same_fn_diff",
            "cos_same_fn_diff",
            "sin_cos_square_reduce",
            "sec_tan_cancel",
        ])
        if mode == "tan2_as_sin_cos":
            # tan(2x)=tan(x) but written with sin/cos forms.
            eq = "(2*sin(x)*cos(x))/(cos(x)^2-sin(x)^2)=sin(x)/cos(x)"
            interval = rng.choice(["0,360", "0,180", "-180,180"])
            cli_input = f"3\n{eq},x,{interval}\n"
        elif mode == "tan_diff_as_fraction":
            # tan(2x)-tan(x)=0 but expressed after rewrite.
            eq = "(2*sin(x)*cos(x))/(cos(x)^2-sin(x)^2)-sin(x)/cos(x)=0"
            interval = rng.choice(["-pi,pi", "0,2*pi", "0,pi"])
            cli_input = f"3\n{eq},x,{interval}\n"
        elif mode == "pythagorean_rewrite":
            # Force using sin^2+cos^2=1 somewhere.
            cli_input = trig_prove_cli("1-cos(2*x)", "2*sin(x)^2", "1")
        elif mode == "sin_same_fn_diff":
            cli_input = "3\nsin(3*x)-sin(x)=0,x,0,2*pi\n"
        elif mode == "cos_same_fn_diff":
            cli_input = "3\ncos(3*x)-cos(x)=0,x,0,2*pi\n"
        elif mode == "sin_cos_square_reduce":
            cli_input = trig_prove_cli("(sin(x)+cos(x))^2-2*sin(x)*cos(x)", "1")
        elif mode == "sec_tan_cancel":
            cli_input = trig_prove_cli("sec(2*x+1)^2-tan(2*x+1)^2", "1")
        else:
            cli_input = trig_prove_cli("cos(x)+cos(3*x)", "2*cos(2*x)*cos(x)", "1")
        label = f"Trig rearrange {index}: {mode}"
        checker = trig_solve_checker("x =") if cli_input.startswith("3") else trig_prove_checker("1")
        feat = "trig_rearrange:" + mode
        return self.make_cli_case("Trigonometry", "trigProgram.py", cli_input, label, checker, feature=feat)

    def random_trig_mad_as_maths_case(self, rng, difficulty, index):
        choice = rng.randint(1, 12)
        if choice == 1:
            eq = f"sin(2*x)-sin(x)=0"
            cli_input = f"3\n{eq},x,0,360\n"
        elif choice == 2:
            eq = f"cos(2*x)=cos(x)"
            cli_input = f"3\n{eq},x,0,180\n"
        elif choice == 3:
            a, b = rng.randint(2, 6), rng.randint(2, 5)
            eq = f"tan({a}*x-{b}*pi/{rng.randint(4,12)})=sqrt(3)"
            cli_input = f"3\n{eq},x,0,pi\n"
        elif choice <= 5:
            c1, c2 = rng.randint(1, 6), rng.randint(1, 6)
            eq = f"sin(x)^2+{c1}*sin(x)+{c2}=0"
            cli_input = f"3\n{eq},x,-90,90\n"
        elif choice <= 8:
            eq = f"cos(2*x)+cos(x)=0"
            cli_input = f"3\n{eq},x,0,360\n"
        else:
            eq = f"sin(x)=1/{rng.randint(2,5)}"
            cli_input = f"3\n{eq},x,0,90\n"
        label = f"MadAsMaths trig {index}"
        checker = build_checker(
            contains_all=(),
            contains_any=("x =", "no valid trig values", "no real solutions", "no solution", "start", "solve trig eq", "method: trig solve"),
            min_steps=3,
            min_lines=3,
        )
        return self.make_cli_case("Trigonometry", "trigProgram.py", cli_input, label, checker, feature="trig_madas", use_calculated=False)

    def random_trig_identity_hard_case(self, rng, difficulty, index):
        choice = rng.randint(1, 11)
        if choice == 1:
            cli_input = trig_prove_cli("sec(x)^4-tan(x)^4", "1+2*tan(x)^2")
        elif choice == 2:
            cli_input = trig_prove_cli("sec(x)^2-tan(x)^2", "1")
        elif choice == 3:
            cli_input = trig_prove_cli("(sin(x)+cos(x))^2", "1+sin(2*x)")
        elif choice == 4:
            cli_input = trig_prove_cli("tan(x)+cot(x)", "sec(x)*cosec(x)")
        elif choice == 5:
            cli_input = trig_prove_cli("sin(x)^3*cos(x)+cos(x)^3*sin(x)", "sin(x)*cos(x)")
        elif choice == 6:
            cli_input = trig_prove_cli("sec(x)-tan(x)", "cos(x)/(1+sin(x))")
        elif choice == 7:
            cli_input = trig_prove_cli("3*sec(x)^2-3*tan(x)^2", "3")
        elif choice == 8:
            cli_input = trig_prove_cli("5-5*sin(x)^2", "5*cos(x)^2")
        elif choice == 9:
            cli_input = trig_prove_cli("(sin(x)+cos(x))^2-2*sin(x)*cos(x)", "1")
        elif choice == 10:
            cli_input = trig_prove_cli("4*cosec(x)^2-4*cot(x)^2", "4")
        else:
            cli_input = trig_prove_cli("7*sin(x)^2+7*cos(x)^2", "7")
        label = f"Hard trig identity {index}"
        return self.make_cli_case("Trigonometry", "trigProgram.py", cli_input, label, trig_prove_checker(), feature="trig_identity_hard")

    def random_trig_equation_multi_case(self, rng, difficulty, index):
        # Exam-style: quadratics in sin/cos, factor, mixed angles (weighted more in EXAM_STRESS)
        a, c = rng.randint(1, 3), rng.randint(0, 2)
        param = f"cos(2*x)+{a}*cos(x)={c}" if a + c > 0 else "cos(2*x)+2*cos(x)=1"
        base = [
            "sin(2*x)-sin(x)=0",
            "cos(2*x)+cos(x)=0",
            "cos(2*x)=cos(x)",
            param,
            "cos(x)+cos(3*x)=0",
            "sin(2*x)+cos(2*x)=0",
            "2*sin(x)*cos(x)-sqrt(2)*cos(x)=0",
        ]
        extra = [
            "sin(2*x)+cos(x)=0",
            f"cos({2*a}*x)+cos(x)=0" if 2 * a < 6 else f"cos(2*x)+cos(3*x)=0",
        ]
        chaos = [
            "cos(x)*sin(2*x)=0",
            "tan(2*x)-tan(x)=0",
        ]
        pool = list(base)
        if difficulty in ("chaos", "hard"):
            pool += extra
        if difficulty == "chaos":
            pool += chaos
        eq = rng.choice(pool)
        rdeg = rng.random() < 0.52
        if difficulty == "chaos" and rdeg:
            interval = rng.choice(["0,2*pi", "0,pi", "-pi,pi", "0,2*pi", "-pi,2*pi"])
        else:
            interval = rng.choice(["0,360", "0,180", "-180,180"])
        cli_input = f"3\n{eq},x,{interval}\n"
        label = f"Multi trig {index}"
        return self.make_cli_case("Trigonometry", "trigProgram.py", cli_input, label, trig_solve_checker("x ="), feature="trig_multi", use_calculated=False)

    def random_trig_radian_hard_case(self, rng, difficulty, index):
        choice = rng.randint(1, 6)
        if choice == 1:
            cli_input = "3\nsin(x)=sqrt(2)/2,x,0,pi\n"
        elif choice == 2:
            cli_input = "3\ntan(x)=-1,x,0,pi\n"
        elif choice == 3:
            cli_input = "3\nsin(2*x)=1,x,0,pi\n"
        elif choice == 4:
            cli_input = "3\ncos(x/2)=-1,x,0,2*pi\n"
        else:
            cli_input = "3\ntan(2*x)=1,x,0,pi/2\n"
        label = f"Radian hard {index}"
        return self.make_cli_case("Trigonometry", "trigProgram.py", cli_input, label, trig_solve_checker("x ="), feature="trig_radian")

    def random_derive_normal_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            expr = rng.choice([
                "x^3",
                "sin(x)",
                "ln(x)",
                "x^x",
                "sin(x)^x",
                "asin(x)",
            ])
            label = f"Random derive normal {index}: simple"
            return self.make_cli_case("Derive", "deriveProgram.py", f"1\n{expr}\n", label, self.derive_output_checker(expr), feature="derive_normal:simple")
        mode = rng.choice([
            "chain_power",
            "nested_exp",
            "quotient",
            "product",
            "log_chain",
            "inverse_trig",
            "root_chain",
            "trig_composite",
            "power_power",
        ])
        if mode == "chain_power":
            expr = f"({self.random_positive_expr(rng, 'x', difficulty)})^{self.random_power_limit(rng, difficulty, hard=9)}"
        elif mode == "nested_exp":
            expr = f"exp(sin({self.random_angle_expr(rng, 'x', difficulty)})+cos({self.random_linear_expr(rng, 'x', allow_negative=True)}))"
        elif mode == "quotient":
            expr = f"({self.random_general_expr(rng, 'x', difficulty, 2)})/({self.random_positive_expr(rng, 'x', difficulty)})"
        elif mode == "product":
            expr = (
                f"({self.random_general_expr(rng, 'x', difficulty, 1)})*"
                f"exp({self.random_linear_expr(rng, 'x', allow_negative=True)})*"
                f"log({self.random_positive_expr(rng, 'x', difficulty)})"
            )
        elif mode == "log_chain":
            expr = f"log(sqrt({self.random_positive_expr(rng, 'x', difficulty)}))"
        elif mode == "inverse_trig":
            expr = rng.choice([
                f"asin(({self.random_linear_expr(rng, 'x', allow_negative=True)})/10)",
                f"atan({self.random_general_expr(rng, 'x', difficulty, 1)})",
            ])
        elif mode == "root_chain":
            expr = f"sqrt(({self.random_linear_expr(rng, 'x', allow_negative=True)})^2+{rng.randint(2, 12)})"
        elif mode == "power_power":
            expr = f"({self.random_positive_expr(rng, 'x', difficulty)})^(x+{rng.randint(1, 4)})"
        else:
            a1 = self.random_angle_expr(rng, "x", difficulty)
            a2 = self.random_angle_expr(rng, "x", difficulty)
            expr = f"(sin({a1})+cos({a2}))^{self.random_power_limit(rng, difficulty, medium=5, hard=7)}"
        label = f"Random derive normal {index}: {mode}"
        return self.make_cli_case("Derive", "deriveProgram.py", f"1\n{expr}\n", label, self.derive_output_checker(expr), feature=f"derive_normal:{mode}")

    def random_derive_implicit_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            eq = "x^2+y^2=1"
            label = f"Random derive implicit {index}: circle"
            return self.make_cli_case("Derive", "deriveProgram.py", f"2\n{eq}\n", label, derive_implicit_checker("dy/dx"), feature="derive_implicit:circle", use_calculated=False)
        a = rng.randint(2, 6)
        b = rng.randint(2, 6)
        mode = rng.choice(["circle", "mixed", "trig", "exp_log"])
        if mode == "circle":
            eq = f"{a}*x^2+{b}*y^2={rng.randint(10, 40)}"
        elif mode == "mixed":
            eq = f"x^2+{a}*x*y+{b}*y^2={rng.randint(5, 25)}"
        elif mode == "trig":
            eq = f"sin({a}*x)+cos({b}*y)=1"
        else:
            eq = f"exp({a}*x)+log(y^2+{b})={rng.randint(5, 25)}"
        label = f"Random derive implicit {index}: {mode}"
        return self.make_cli_case("Derive", "deriveProgram.py", f"2\n{eq}\n", label, derive_implicit_checker("dy/dx"), feature=f"derive_implicit:{mode}", use_calculated=False)

    def random_derive_parametric_case(self, rng, difficulty, index):
        mode = rng.choice(["poly", "trig_exp", "log_mix", "rational_trig", "mixed_rational_exp", "nested_trig_log"])
        if mode == "poly":
            x_expr = f"t^{rng.randint(2, 5)}+{rng.randint(1, 7)}"
            y_expr = f"t^{rng.randint(3, 6)}-{rng.randint(1, 7)}*t"
        elif mode == "trig_exp":
            x_expr = f"sin({rng.randint(2, 5)}*t)+exp(t)"
            y_expr = f"cos({rng.randint(2, 5)}*t)+t^2"
        elif mode == "log_mix":
            x_expr = f"log(t+{rng.randint(2, 6)})+t^2"
            y_expr = f"exp(t/{rng.randint(2, 5)})+sin(t)"
        else:
            if mode == "rational_trig":
                x_expr = f"(t^2+{rng.randint(2, 8)})/(t+{rng.randint(2, 6)})"
                y_expr = f"tan(t/{rng.randint(2, 5)})+log(t+{rng.randint(3, 8)})"
            elif mode == "mixed_rational_exp":
                x_expr = f"(exp(t)+{rng.randint(1, 5)}*t)/(t^2+{rng.randint(2, 8)})"
                y_expr = f"log(t^2+{rng.randint(3, 9)})+sin({rng.randint(2, 5)}*t)"
            else:
                x_expr = f"sin(log(t+{rng.randint(3, 8)}))+t^2"
                y_expr = f"exp(t/{rng.randint(2, 6)})/(t+{rng.randint(3, 8)})"
        label = f"Random derive parametric {index}: {mode}"
        return self.make_cli_case("Derive", "deriveProgram.py", f"3\n{x_expr}\n{y_expr}\n", label, self.parametric_output_checker(x_expr, y_expr), feature=f"derive_parametric:{mode}")

    def build_random_derive_cases(self, difficulty, count, rng):
        if getattr(self, "backend", "python") == "c":
            # C++ backend: expanded derivation coverage.
            features = [
                self.random_derive_normal_case,
                self.random_derive_implicit_case,
                self.random_derive_parametric_case,
                self.random_derive_triple_product_case,
                self.random_derive_chain_quotient_case,
                self.random_derive_implicit_product_case,
                self.random_derive_parametric_hard_case,
                self.random_derive_log_diff_case,
            ]
        else:
            features = [
                self.random_derive_normal_case,
                self.random_derive_implicit_case,
                self.random_derive_parametric_case,
                self.random_derive_triple_product_case,
                self.random_derive_chain_quotient_case,
                self.random_derive_implicit_product_case,
                self.random_derive_parametric_hard_case,
                self.random_derive_log_diff_case,
            ]
        return self.build_unique_random_cases(features, count, rng, difficulty)

    def random_derive_triple_product_case(self, rng, difficulty, index):
        if difficulty == "random":
            choices = [
                self.random_affine_expr(rng, "x", "medium", allow_negative=True),
                self.random_shifted_power_expr(rng, "x", "medium"),
                f"sin({self.random_angle_expr(rng, 'x', 'medium')})",
                f"exp({self.random_linear_expr(rng, 'x', allow_negative=True)})",
                f"log(({rng.randint(2, 6)}*x+{rng.randint(3, 9)})^2+1)",
            ]
            expr = f"({rng.choice(choices)})*({rng.choice(choices)})*({rng.choice(choices)})"
            cli_input = f"1\n{expr}\n"
            label = f"Triple product {index}"
            return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, derive_checker("dy/dx"), feature="derive_product:triple", use_calculated=False)
        # Triple-product derivatives expand quickly on the calculator. Keep
        # this generator at exam-scale even in chaos mode; deeper compositions
        # are covered by chain/quotient cases without multiplying three huge
        # subtrees together.
        helper_difficulty = "medium" if difficulty in ("chaos", "hard", "random") else difficulty
        depth = 1
        expr = f"({self.random_general_expr(rng, 'x', helper_difficulty, depth)})*({self.random_general_expr(rng, 'x', helper_difficulty, depth)})*({self.random_general_expr(rng, 'x', helper_difficulty, depth)})"
        cli_input = f"1\n{expr}\n"
        label = f"Triple product {index}"
        return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, derive_checker("dy/dx"), feature="derive_product:triple", use_calculated=False)

    def random_derive_chain_quotient_case(self, rng, difficulty, index):
        if difficulty == "random":
            num = rng.choice([
                self.random_shifted_power_expr(rng, "x", "medium"),
                f"sin({self.random_angle_expr(rng, 'x', 'medium')})",
                f"exp({self.random_linear_expr(rng, 'x', allow_negative=True)})",
                f"log(({rng.randint(2, 6)}*x+{rng.randint(3, 9)})^2+1)",
            ])
            den = f"({rng.randint(2, 7)}*x{self.signed_int_text(rng.randint(1, 9))})^2+{rng.randint(2, 9)}"
            expr = f"(({num})/({den}))^{rng.randint(2,3)}"
            cli_input = f"1\n{expr}\n"
            label = f"Chain quotient {index}"
            return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, derive_checker("dy/dx"), feature="derive_chain_quotient", use_calculated=False)
        helper_difficulty = "hard" if difficulty == "chaos" else ("medium" if difficulty == "random" else difficulty)
        depth = 2 if helper_difficulty in ("hard", "chaos") else 1
        num = f"({self.random_general_expr(rng, 'x', helper_difficulty, depth)})"
        den = f"({self.random_positive_expr(rng, 'x', helper_difficulty)})"
        expr = f"({num}/{den})^{rng.randint(2,4)}"
        cli_input = f"1\n{expr}\n"
        label = f"Chain quotient {index}"
        return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, derive_checker("dy/dx"), feature="derive_chain_quotient", use_calculated=False)

    def random_derive_implicit_product_case(self, rng, difficulty, index):
        a, b, c = rng.randint(1, 5), rng.randint(1, 5), rng.randint(1, 5)
        eq = f"x^2+{a}*x*y+{b}*y^2+{c}*x={rng.randint(5, 20)}"
        cli_input = f"2\n{eq}\n"
        label = f"Implicit product {index}"
        return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, derive_implicit_checker("dy/dx"), feature="derive_implicit_product", use_calculated=False)

    def random_derive_parametric_hard_case(self, rng, difficulty, index):
        n1, n2 = rng.randint(2, 5), rng.randint(2, 5)
        x_expr = f"t^{n1}+{rng.randint(1,5)}*t"
        y_expr = f"t^{n2}-{rng.randint(1,5)}*t^{n1}"
        cli_input = f"3\n{x_expr}\n{y_expr}\n"
        label = f"Parametric hard {index}"
        return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, self.parametric_output_checker(x_expr, y_expr), feature="derive_parametric_hard")

    def random_derive_log_diff_case(self, rng, difficulty, index):
        helper_difficulty = "hard" if difficulty == "chaos" else difficulty
        exprs = [
            f"x^({self.random_linear_expr(rng, 'x', False)})",
            f"({self.random_positive_expr(rng, 'x', helper_difficulty)})^x",
            f"(sin(x))^({self.random_linear_expr(rng, 'x', False)})",
        ]
        expr = rng.choice(exprs)
        cli_input = f"1\n{expr}\n"
        label = f"Log diff {index}"
        return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, derive_checker("dy/dx", "log"), feature="derive_log_diff")

    def random_derive_second_derivative_case(self, rng, difficulty, index):
        # Second derivative (Mode 4 in derive)
        exprs = [
            "x^2",
            "x^3", 
            "sin(x)",
            "exp(x)",
            "x*exp(x)",
            "x^2+1",
            "log(x)",
            "sqrt(x)",
        ]
        expr = rng.choice(exprs)
        cli_input = f"4\n{expr}\n"
        label = f"2nd deriv {index}: {expr}"
        return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, build_checker(contains_all=(), contains_any=("dy/dx", "d2y/dx2"), min_lines=1), feature="derive_2nd_derivative")

    def random_integrate_auto_case(self, rng, difficulty, index):
        # Totally random/unpredictable integrand generation
        if getattr(self, "backend", "python") == "c":
            expr = rng.choice([
                "x^7-3*x^2+4",
                "sin(3*x+2)",
                "cos(4*x)",
                "exp(5*x)",
                "1/(5*x+7)",
                "sec(x)^2",
                "tan(x)^2",
                "sin(x)^2",
                "cos(x)^2",
                "2*x/(x+2)",
                "3*x/(x+2)",
                "(3*x^2+1)/(x^3+x+7)",
                "(x*tan(x))/(tan(x)+sec(x))",
                "((1/(x^2))+(1/(x^3)))*exp(1/x)",
            ])
            label = f"Integrate {index}: {expr[:25]}"
            base_checker = self.integrate_output_checker(expr)

            def c_integral_checker(output):
                text = normalized_text(output)
                return base_checker(output) and "not recognised" not in text and "not available" not in text

            c_integral_checker.__name__ = "c_integral_checker"
            return self.make_cli_case("Integrate", "intProgram.py", f"1\n{expr}\n1\n", label, c_integral_checker, feature="integrate_auto:c_supported")

        if difficulty == "chaos":
            # Complete chaos - truly unpredictable integrands
            integrand_types = [
                "polynomial", "trig", "exp", "log", "rational", "nested", "sqrt", "inverse_trig", "product", "chain", "reciprocal_exp", "trig_conjugate"
            ]
            itype = rng.choice(integrand_types)
            
            if itype == "polynomial":
                # Random polynomial of any degree
                degree = rng.randint(1, 8)
                terms = []
                for d in range(degree + 1):
                    c = rng.randint(-9, 9)
                    if c != 0:
                        if d == 0:
                            terms.append(str(c))
                        elif d == 1:
                            terms.append(f"{c}*x" if c != 1 else "x")
                        else:
                            terms.append(f"{c}*x^{d}")
                expr = "+".join(terms).replace("+-", "-").replace("1*x", "x")
                mode = "chaos_poly"
            elif itype == "trig":
                funcs = ["sin", "cos", "tan", "sec", "cosec", "cot"]
                f = rng.choice(funcs)
                coef = rng.randint(1, 4)
                power = rng.randint(1, 3)
                expr = f"{coef}*{f}(x)^{power}" if power > 1 else f"{coef}*{f}(x)"
                mode = "chaos_trig"
            elif itype == "exp":
                a = rng.randint(2, 5)
                power = rng.randint(1, 3)
                expr = f"{a}^{power}*exp({a}*x)" if rng.random() < 0.5 else f"x*exp({a}*x)"
                mode = "chaos_exp"
            elif itype == "log":
                expr = f"log({rng.randint(2,5)}*x)" if rng.random() < 0.5 else f"x*log(x)"
                mode = "chaos_log"
            elif itype == "rational":
                a, b = rng.randint(1, 5), rng.randint(1, 5)
                expr = f"{a}/(x+{b})" if rng.random() < 0.5 else f"{a}*x/(x+{b})"
                mode = "chaos_rational"
            elif itype == "nested":
                # Use genuine nested expressions, but include the inner
                # derivative so the expected answer is elementary and
                # calculator-relevant.
                expr = rng.choice([
                    "cos(exp(x))*exp(x)",
                    "exp(sin(x))*cos(x)",
                    "1/(x*log(x))",
                    "sin(log(x))/x",
                    "cos(log(x))/x",
                ])
                mode = "chaos_nested"
            elif itype == "sqrt":
                a = rng.randint(2, 5)
                expr = f"sqrt({a}*x+{rng.randint(1,5)})" if rng.random() < 0.5 else f"x/sqrt(x+{rng.randint(1,3)})"
                mode = "chaos_sqrt"
            elif itype == "inverse_trig":
                funcs = ["asin", "acos", "atan"]
                expr = f"{rng.choice(funcs)}(x/{rng.randint(2,5)})"
                mode = "chaos_inv_trig"
            elif itype == "product":
                a, b = rng.randint(1, 4), rng.randint(1, 4)
                expr = f"{a}*x^{rng.randint(1,3)}*exp({b}*x)"
                mode = "chaos_product"
            elif itype == "chain":
                a, b = rng.randint(2, 5), rng.randint(1, 5)
                expr = f"sin({a}*x+{b})"
                mode = "chaos_chain"
            elif itype == "reciprocal_exp":
                expr = "((1/(x^2))+(1/(x^3)))*exp(1/x)"
                mode = "chaos_reciprocal_exp"
            elif itype == "trig_conjugate":
                expr = rng.choice([
                    "(x*tan(x))/(tan(x)+sec(x))",
                    "(x*sec(x))/(tan(x)+sec(x))",
                ])
                mode = "chaos_trig_conjugate"
        elif difficulty == "hard":
            # Hard mode - varied
            types = ["polynomial", "trig_power", "exp_poly", "rational", "u_sub"]
            itype = rng.choice(types)
            
            if itype == "polynomial":
                degree = rng.randint(2, 5)
                terms = [f"{rng.randint(1,5)}*x^{d}" for d in range(1, degree+1)]
                expr = "+".join(terms)
                mode = "hard_poly"
            elif itype == "trig_power":
                p = rng.randint(2, 4)
                expr = f"sin(x)^{p}" if rng.random() < 0.5 else f"cos(x)^{p}"
                mode = "hard_trig"
            elif itype == "exp_poly":
                expr = f"x^{rng.randint(2,3)}*exp({rng.randint(1,3)}*x)"
                mode = "hard_exp"
            elif itype == "rational":
                a, b = rng.randint(1, 3), rng.randint(1, 3)
                expr = f"{a}*x/(x+{b})"
                mode = "hard_rational"
            else:
                a = rng.randint(2, 4)
                expr = f"{a}*x+{rng.randint(1,3)}"
                mode = "hard_sub"
        else:
            # Easy - simple
            expr = f"{rng.randint(1,5)}*x^{rng.randint(1,3)}"
            mode = "easy"
        
        label = f"Integrate {index}: {expr[:25]}"
        base_checker = self.integrate_output_checker(expr)

        def generated_integral_checker(output):
            text = normalized_text(output)
            return base_checker(output) and "no elementary" not in text and "no standard exam-method" not in text

        generated_integral_checker.__name__ = "generated_integral_checker"
        return self.make_cli_case("Integrate", "intProgram.py", f"1\n{expr}\n1\n", label, generated_integral_checker, feature=f"integrate_auto:{mode}")

    def random_integrate_sub_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            integrand = rng.choice([
                "2*x*cos(x^2)",
                "2*x*exp(x^2)",
                "3*x^2*cos(x^3+1)",
                "cos(x)*exp(sin(x))",
                "(2*x+1)/(1+(x^2+x)^2)",
            ])
            label = f"Random integrate sub {index}: c_supported"
            return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n1\n", label, self.integrate_output_checker(integrand), feature="integrate_sub:c_supported")

        helper_difficulty = "hard" if difficulty == "chaos" else difficulty
        mode = rng.choice(["poly", "sin", "exp", "atan", "sqrt", "log_power", "cos", "reciprocal", "nested_u", "hard_log", "arcsin_sub", "sec_tan", "cot_cosec"])
        u = rng.choice([
            self.random_linear_expr(rng, "x", allow_negative=True),
            f"x^2{self.signed_int_text(rng.randint(-5,5))}*x+{rng.randint(2,9)}",
            f"exp({self.random_linear_expr(rng, 'x', allow_negative=True)})+{rng.randint(2,9)}",
            f"sqrt({self.random_positive_expr(rng, 'x', helper_difficulty)})+{rng.randint(2,9)}",
            f"log({self.random_positive_expr(rng, 'x', helper_difficulty)})+{rng.randint(2,9)}",
        ])
        if mode == "poly":
            power = self.random_power_limit(rng, difficulty, medium=6, hard=14)
            integrand = f"({u})^{power}"
        elif mode == "sin":
            integrand = f"sin({u})"
        elif mode == "cos":
            integrand = f"cos({u})"
        elif mode == "exp":
            integrand = f"exp({u})"
        elif mode == "atan":
            integrand = f"1/(1+({u})^2)"
        elif mode == "sqrt":
            integrand = f"sqrt({u})"
        elif mode == "reciprocal":
            integrand = f"1/({u})"
        elif mode == "nested_u":
            integrand = rng.choice([f"exp(({u})^2)", f"sin(({u})^2+{rng.randint(1, 5)})"])
            u = f"({u})^2" if "exp" in integrand else f"({u})^2+{rng.randint(1, 5)}"
            integrand = f"exp({u})" if integrand.startswith("exp") else f"sin({u})"
        else:
            integrand = f"(log({u}))^{rng.randint(2,5)}"
        _var, _steps, final, _formatted = DERIVE.solve_normal_mode(u)
        deriv = DERIVE.show(final)
        integrand = f"({deriv})*({integrand})" if mode in ("poly", "sin", "cos", "exp", "sqrt", "log_power", "reciprocal", "nested_u") else f"({deriv})/(1+({u})^2)"
        label = f"Random integrate sub {index}: {mode}"
        return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n4\n{u}\n", label, self.integrate_output_checker(integrand), feature=f"integrate_sub:{mode}")

    def random_integrate_parts_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            integrand = rng.choice([
                "x*exp(x)",
                "x^2*exp(x)",
                "x*log(x)",
                "x^2*log(x)",
                "exp(2*x)*sin(3*x)",
                "exp(3*x)*cos(4*x)",
            ])
            label = f"Random integrate parts {index}: c_supported"
            return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n5\n", label, self.integrate_output_checker(integrand), feature="integrate_parts:c_supported")

        mode = rng.choice(["exp", "sin", "cos", "log", "exp_sin_loop", "exp_cos_loop", "poly_log", "poly_atan", "nested_log", "high_poly_sin", "high_poly_cos", "high_poly_exp", "tan_exp", "sec_exp"])
        power = self.random_power_limit(rng, difficulty, medium=5, hard=12)
        a = rng.randint(2, 6)
        if mode == "exp":
            integrand = f"x^{power}*exp({a}*x)"
        elif mode == "sin":
            integrand = f"x^{power}*sin({a}*x)"
        elif mode == "cos":
            integrand = f"x^{power}*cos({a}*x)"
        elif mode == "log":
            integrand = f"(log(x))^{max(2, power - 1)}"
        elif mode == "exp_sin_loop":
            integrand = f"exp({rng.randint(1,4)}*x)*sin({a}*x)"
        elif mode == "exp_cos_loop":
            integrand = f"exp({rng.randint(1,4)}*x)*cos({a}*x)"
        elif mode == "poly_atan":
            integrand = f"x^{rng.randint(1, 5)}*atan({a}*x)"
        elif mode == "nested_log":
            integrand = f"x^{rng.randint(1, 5)}*log(x^2+{rng.randint(2, 9)})"
        else:
            integrand = f"x^{power}*log(x)"
        label = f"Random integrate parts {index}: {mode}"
        return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n5\n", label, self.integrate_output_checker(integrand), feature=f"integrate_parts:{mode}")

    def random_integrate_trig_case(self, rng, difficulty, index):
        a = rng.randint(2, 6)
        b = rng.randint(-5, 5)
        angle = rng.choice([f"{a}*x", f"{a}*x{self.signed_int_text(b)}", f"{a}*x+({self.random_pi_shift_text(rng)})"])
        mode = rng.choice(["sin2cos2", "sinodd", "cosodd", "shifted_sinodd", "high_even", "tan_sec", "cosec_cot"])
        if mode == "sin2cos2":
            integrand = f"sin({angle})^2*cos({angle})^2"
        elif mode == "sinodd":
            integrand = f"sin({angle})^3*cos({angle})^2"
        elif mode == "cosodd":
            integrand = f"cos({angle})^3*sin({angle})^2"
        elif mode == "shifted_sinodd":
            integrand = f"sin({angle})^5*cos({angle})^2"
        elif mode == "tan_sec":
            integrand = f"tan({angle})^{rng.randint(1, 3)}*sec({angle})^2"
        elif mode == "cosec_cot":
            integrand = f"cosec({angle})^2*cot({angle})^{rng.randint(1, 3)}"
        else:
            integrand = f"sin({angle})^4*cos({angle})^4"
        label = f"Random integrate trig {index}: {mode}"
        return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n3\n", label, integrate_checker("+ c"), feature=f"integrate_trig:{mode}")

    def random_integrate_pf_case(self, rng, difficulty, index):
        mode = rng.choice(["two_linear", "repeated", "linear_quad"])
        a = rng.randint(2, 7)
        b = rng.randint(1, 5)
        if mode == "two_linear":
            integrand = f"1/((x-{a})*(x+{b}))"
        elif mode == "linear_quad":
            c = rng.randint(5, 11)
            d = rng.randint(1, 4)
            integrand = f"({rng.randint(1,4)}*x+{rng.randint(2,9)})/((x+{b})*(x^2+{d}*x+{c}))"
        else:
            integrand = f"(2*x+{a})/((x+{b})^2*(x+{a+b}))"
        label = f"Random integrate pf {index}: {mode}"
        return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n6\n", label, integrate_checker("+ c"), feature=f"integrate_pf:{mode}")

    def random_integrate_div_case(self, rng, difficulty, index):
        # Keep division randoms in supported/A-level-style residual families.
        # Quintic residuals are elementary in theory but require root-level PF
        # machinery that this exam-working route deliberately does not claim.
        mode = rng.choice(["even_over_quad", "odd_over_quad"])
        a = rng.randint(1, 6)
        b = rng.randint(1, 7)
        c = rng.randint(1, 7)
        if mode == "even_over_quad":
            integrand = f"(x^4+{a}*x^2+{b})/(x^2+1)"
        else:
            integrand = f"(x^5+{a}*x^3+{b}*x)/(x^2+{c})"
        label = f"Random integrate division {index}: {mode}"
        return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n7\n", label, integrate_checker("+ c"), feature=f"integrate_div:{mode}")

    def random_integrate_direct_case(self, rng, difficulty, index):
        mode = rng.choice(["linear_over_quad", "linear_over_sqrt_quad", "one_over_linear", "exp", "fraction_power", "one_over_shifted_quad", "sqrt_fraction"])
        a = rng.randint(2, 7)
        if mode == "linear_over_quad":
            integrand = f"({2*a}*x+{a+1})/(x^2+{a+1}*x+{a*a+3})"
        elif mode == "linear_over_sqrt_quad":
            integrand = f"({2*a}*x+{a+1})/sqrt(x^2+{a+1}*x+{a*a+5})"
        elif mode == "one_over_linear":
            integrand = f"1/({a}*x+{a+3})"
        elif mode == "exp":
            integrand = f"exp({a}*x-{rng.randint(1,5)})"
        elif mode == "fraction_power":
            integrand = f"({self.random_linear_expr(rng, 'x', allow_negative=True)})^{self.random_power_limit(rng, difficulty, medium=7, hard=12)}"
        elif mode == "one_over_shifted_quad":
            integrand = f"1/(x^2+{rng.randint(2,9)}*x+{rng.randint(10,30)})"
        else:
            integrand = f"({a}*x+{rng.randint(1,9)})/sqrt(({self.random_linear_expr(rng, 'x', allow_negative=True)})^2+{rng.randint(2,9)})"
        label = f"Random integrate direct {index}: {mode}"
        return self.make_cli_case("Integrate", "intProgram.py", f"1\n{integrand}\n2\n", label, self.integrate_output_checker(integrand), feature=f"integrate_direct:{mode}")

    def build_random_integrate_cases(self, difficulty, count, rng):
        if getattr(self, "backend", "python") == "c":
            # C++ backend: expanded integration coverage.
            features = [
                self.random_integrate_auto_case,
                self.random_integrate_direct_case,
                self.random_integrate_sub_case,
                self.random_integrate_parts_case,
                self.random_integrate_trig_case,
                self.random_integrate_pf_case,
                self.random_integrate_div_case,
                self.random_integrate_partial_frac_case,
                self.random_integrate_trig_power_case,
                self.random_integrate_substitution_case,
                self.random_integrate_parts_twice_case,
                self.random_integrate_extreme_rewrite_case,
            ]
        else:
            features = [
                self.random_integrate_auto_case,
                self.random_integrate_parts_twice_case,
                self.random_integrate_partial_frac_case,
                self.random_integrate_trig_power_case,
                self.random_integrate_substitution_case,
                self.random_integrate_extreme_rewrite_case,
            ]
        return self.build_unique_random_cases(features, count, rng, difficulty)

    def random_integrate_extreme_rewrite_case(self, rng, difficulty, index):
        patterns = [
            "1/x",
            "1/(5*x+7)",
            "(3*x^2-2*x+2)/x",
            "(3*x^2+1)/(x^3+x+7)",
            "(tan(x)^2+1)/(sec(x)^2)",
            "(sec(x)^2-tan(x)^2)/(x+1)",
            "(cosec(x)^2-cot(x)^2)*exp(x)",
            "(1-cos(2*x))/(sin(x))",
            "(1+cos(2*x))/(cos(x))",
            "((sin(x)+cos(x))^2-2*sin(x)*cos(x))/(x+1)",
            "(sin(x)^2+cos(x)^2)/(x^2+1)",
            "tan(x)",
            "sec(x)",
            "cosec(x)",
            "cot(x)",
            "sin(x)^2",
            "cos(x)^2",
        ]
        expr = rng.choice(patterns)
        label = f"Extreme integrate rewrite {index}: {expr}"
        return self.make_cli_case(
            "Integrate",
            "intProgram.py",
            f"1\n{expr}\n1\n",
            label,
            self.integrate_output_checker(expr),
            feature="integrate_extreme_rewrite",
        )

    def random_integrate_parts_twice_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            parts_cases = [
                ("x^3*exp(x)", "integration by parts"),
                ("x*exp(2*x)", "integration by parts"),
                ("x^2*sin(x)", "integration by parts"),
                ("exp(x)*sin(x)", "integration by parts"),
                ("x*cos(2*x)", "integration by substitution"),
            ]
        else:
            parts_cases = [
                ("x^3*exp(x)", "integration by parts"),
                ("x*exp(2*x)", "integration by parts"),
                ("x^2*sin(x)", "integration by parts"),
                ("exp(x)*sin(x)", "integration by parts"),
                ("x*cos(2*x)", "integration by substitution"),
                ("(x^2+1)*exp(x)", "direct integration"),
                ("x^2*log(x)", "integration by substitution"),
            ]
        expr, expected_method = rng.choice(parts_cases)
        cli_input = f"1\n{expr}\n\n"
        label = f"Parts twice {index}"
        return self.make_cli_case("Integrate", "intProgram.py", cli_input, label, integrate_checker(), feature="integrate_parts_twice")

    def random_integrate_partial_frac_case(self, rng, difficulty, index):
        patterns = [
            "x^2/((x+1)*(x+2))",
            "x/((x-1)*(x+2))",
            "(x+1)/(x^2+1)",
            "x^2/((x-1)^2*(x+1))",
            "(2*x+1)/(x^2+x)",
        ]
        expr = rng.choice(patterns)
        cli_input = f"1\n{expr}\n\n"
        label = f"Partial fractions {index}"
        return self.make_cli_case("Integrate", "intProgram.py", cli_input, label, integrate_checker("partial fractions"), feature="integrate_partial")

    def random_integrate_trig_power_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            powers = ["sin(x)^2", "cos(x)^2", "tan(x)^2", "sec(x)^2"]
            expr = rng.choice(powers)
            cli_input = f"1\n{expr}\n\n"
            label = f"Trig power {index}"
            return self.make_cli_case("Integrate", "intProgram.py", cli_input, label, integrate_checker(), feature="integrate_trig_power")
        powers = ["sin(x)^4", "cos(x)^6", "sin(x)^2*cos(x)^2", "sin(2*x)^2", "cos(x)^4"]
        expr = rng.choice(powers)
        cli_input = f"1\n{expr}\n\n"
        label = f"Trig power {index}"
        return self.make_cli_case("Integrate", "intProgram.py", cli_input, label, integrate_checker("use"), feature="integrate_trig_power")

    def random_integrate_substitution_case(self, rng, difficulty, index):
        if getattr(self, "backend", "python") == "c":
            patterns = [
                "sin(3*x+2)",
                "cos(4*x)",
                "exp(5*x)",
                "1/(5*x+7)",
                "1/x",
            ]
            expr = rng.choice(patterns)
            cli_input = f"1\n{expr}\n\n"
            label = f"Substitution {index}"
            return self.make_cli_case("Integrate", "intProgram.py", cli_input, label, integrate_checker(), feature="integrate_sub")
        patterns = [
            "x/sqrt(x^2+1)",
            "(x^2)/(x^3+1)",
            "exp(sqrt(x))/sqrt(x)",
            "sin(x)^2*cos(x)",
            "(log(x))/x",
            "1/(x*sqrt(x-1))",
        ]
        expr = rng.choice(patterns)
        cli_input = f"1\n{expr}\n\n"
        label = f"Substitution {index}"
        return self.make_cli_case("Integrate", "intProgram.py", cli_input, label, integrate_checker("u ="), feature="integrate_sub")

    def random_suvat_case(self, rng, difficulty, index, target):
        def frac_num(value):
            return SUVAT.num(value.numerator, value.denominator)

        def frac_text(value):
            return str(value.numerator) if value.denominator == 1 else f"{value.numerator}/{value.denominator}"

        den_pool = [1, 1, 1, 2, 3]
        if difficulty == "chaos":
            den_pool = [1, 2, 3, 5, 8, self.random_unbounded_count(rng, minimum=5, continue_probability=0.25)]
        u = Fraction(rng.randint(-9, 9), rng.choice(den_pool) if difficulty in ("hard", "chaos") else 1)
        a = Fraction(self.random_nonzero_int(rng, -7, 7), rng.choice(den_pool) if difficulty in ("hard", "chaos") else 1)
        t = Fraction(rng.randint(1, 8), rng.choice(den_pool) if difficulty in ("hard", "chaos") else 1)
        v = u + a * t
        s = u * t + Fraction(1, 2) * a * t * t

        if target == "s":
            values = (None, frac_num(u), None, frac_num(a), frac_num(t), "s", s)
        elif target == "v":
            values = (None, frac_num(u), None, frac_num(a), frac_num(t), "v", v)
        elif target == "u":
            values = (frac_num(s), None, frac_num(v), frac_num(a), frac_num(t), "u", u)
        elif target == "a":
            values = (None, frac_num(u), frac_num(v), None, frac_num(t), "a", a)
        else:
            values = (frac_num(s), frac_num(u), frac_num(v), frac_num(a), None, "t", t)

        expected = f"{values[6].numerator}/{values[6].denominator}" if values[6].denominator != 1 else str(values[6].numerator)
        all_values = {"s": s, "u": u, "v": v, "a": a, "t": t}
        cli_input = "\n".join("," if name == target else frac_text(all_values[name]) for name in ("s", "u", "v", "a", "t")) + "\n"
        cli_input = self.fuzz_suvat_cli_input_format(cli_input, rng, difficulty)

        def runner():
            out, _ = self.run_cli("SUVATprogram.py", cli_input)
            return suvat_cli_checker(target, expected)(out), out

        label = f"Random SUVAT {index}: find {target}"
        return self.make_direct_case("SUVAT", label, runner, input_text=cli_input, check_info=f"suvat exact answer = {expected}; random input formatting fuzzed", feature=f"suvat_find_{target}")

    def random_suvat_edge_case(self, rng, difficulty, index, target):
        def frac_text(value):
            return str(value.numerator) if value.denominator == 1 else f"{value.numerator}/{value.denominator}"

        def make_case(label, cli_input, checker, check_info, feature):
            cli_input = self.fuzz_suvat_cli_input_format(cli_input, rng, difficulty)

            def runner():
                out, _ = self.run_cli("SUVATprogram.py", cli_input)
                return checker(out), out

            return self.make_direct_case("SUVAT", label, runner, input_text=cli_input, check_info=f"{check_info}; random input formatting fuzzed", feature=feature)

        if target == "t":
            mode = rng.choice([
                "zero_accel_surd_missing_v",
                "zero_accel_fraction_missing_v",
                "quadratic_missing_v",
                "average_velocity_no_a",
                "symbolic_time",
            ])
            if mode == "zero_accel_surd_missing_v":
                distance = rng.randint(12, 80)
                coeff = rng.randint(2, 18)
                radicand = rng.choice([2, 3, 5, 7, 11])
                divisor = rng.randint(1, 5)
                u_expr = f"{coeff}*(sqrt({radicand})/{divisor})"
                expected = f"{distance}*{divisor}/({coeff}*sqrt({radicand}))"
                cli_input = f"{distance}\n{u_expr}\n\n0\n,\n"
                checker = lambda out, expected=expected: suvat_cli_checker("t", expected)(out) and "t = s/u" in normalized_text(out)
            elif mode == "zero_accel_fraction_missing_v":
                distance = Fraction(rng.randint(8, 90), rng.choice([1, 2, 3]))
                velocity = Fraction(self.random_nonzero_int(rng, -12, 12), rng.choice([1, 2, 3]))
                if velocity < 0:
                    distance = -distance
                expected_fraction = distance / velocity
                cli_input = f"{frac_text(distance)}\n{frac_text(velocity)}\n\n0\n,\n"
                checker = lambda out, expected=frac_text(expected_fraction): suvat_cli_checker("t", expected)(out) and "t = s/u" in normalized_text(out)
            elif mode == "quadratic_missing_v":
                t_val = Fraction(rng.randint(1, 8), rng.choice([1, 2, 3]))
                u_val = Fraction(rng.randint(1, 8), rng.choice([1, 2]))
                a_val = Fraction(rng.randint(1, 6), rng.choice([1, 2, 3]))
                s_val = u_val * t_val + Fraction(1, 2) * a_val * t_val * t_val
                cli_input = f"{frac_text(s_val)}\n{frac_text(u_val)}\n\n{frac_text(a_val)}\n,\n"
                checker = lambda out, expected=frac_text(t_val): suvat_cli_checker("t", expected)(out)
            elif mode == "average_velocity_no_a":
                t_val = Fraction(rng.randint(1, 8), rng.choice([1, 2, 3]))
                u_val = Fraction(rng.randint(-6, 6), 1)
                v_val = Fraction(self.random_nonzero_int(rng, -8, 8), 1)
                if u_val + v_val == 0:
                    v_val += 1
                s_val = Fraction(1, 2) * (u_val + v_val) * t_val
                cli_input = f"{frac_text(s_val)}\n{frac_text(u_val)}\n{frac_text(v_val)}\n\n,\n"
                checker = lambda out, expected=frac_text(t_val): suvat_cli_checker("t", expected)(out) and "t = 2s/(u+v)" in normalized_text(out)
            else:
                cli_input = "\n\n\n0\n,\n"
                checker = suvat_symbolic_output_checker("t", "2*s/(u+v)")
        elif target == "s":
            mode = rng.choice(["zero_accel", "symbolic"])
            if mode == "zero_accel":
                u_val = Fraction(self.random_nonzero_int(rng, -12, 12), rng.choice([1, 2, 3]))
                t_val = Fraction(rng.randint(1, 9), rng.choice([1, 2, 3]))
                expected = u_val * t_val
                cli_input = f",\n{frac_text(u_val)}\n\n0\n{frac_text(t_val)}\n"
                checker = suvat_cli_checker("s", frac_text(expected))
            else:
                cli_input = ",\n\n\n\n\n"
                checker = suvat_symbolic_output_checker("s", "u*t", "a*t^2")
            mode = f"{mode}_{target}"
        elif target == "v":
            mode = rng.choice(["zero_accel", "symbolic"])
            if mode == "zero_accel":
                u_val = Fraction(self.random_nonzero_int(rng, -12, 12), rng.choice([1, 2, 3]))
                t_val = Fraction(rng.randint(1, 9), rng.choice([1, 2, 3]))
                cli_input = f"\n{frac_text(u_val)}\n,\n0\n{frac_text(t_val)}\n"
                checker = suvat_cli_checker("v", frac_text(u_val))
            else:
                cli_input = "\n\n,\n\n\n"
                checker = suvat_symbolic_output_checker("v", "u+a*t")
            mode = f"{mode}_{target}"
        elif target == "u":
            mode = rng.choice(["zero_accel", "symbolic"])
            if mode == "zero_accel":
                v_val = Fraction(self.random_nonzero_int(rng, -12, 12), rng.choice([1, 2, 3]))
                t_val = Fraction(rng.randint(1, 9), rng.choice([1, 2, 3]))
                cli_input = f"\n,\n{frac_text(v_val)}\n0\n{frac_text(t_val)}\n"
                checker = suvat_cli_checker("u", frac_text(v_val))
            else:
                cli_input = "\n,\n\n\n\n"
                checker = suvat_symbolic_output_checker("u", "v-a*t")
            mode = f"{mode}_{target}"
        else:
            mode = rng.choice(["zero_time_consistent", "symbolic"])
            if mode == "zero_time_consistent":
                u_val = Fraction(rng.randint(-8, 8), 1)
                v_val = u_val
                cli_input = f"\n{frac_text(u_val)}\n{frac_text(v_val)}\n,\n0\n"
                checker = suvat_expected_error_checker("any real")
                feature_prefix = "suvat_expected_error"
            else:
                cli_input = "\n\n\n,\n\n"
                checker = suvat_symbolic_output_checker("a", "(v-u)/t")
                feature_prefix = "suvat_edge"
            mode = f"{mode}_{target}"
        if target != "a":
            feature_prefix = "suvat_edge"

        label = f"Random SUVAT edge {index}: {mode}"
        return make_case(label, cli_input, checker, f"suvat edge family = {mode}", f"{feature_prefix}:{mode}")

    def build_random_suvat_cases(self, difficulty, count, rng):
        features = ["s", "u", "v", "a", "t"]
        generators = []
        for target in features:
            generators.append(lambda local_rng, local_difficulty, idx, target=target: self.random_suvat_case(local_rng, local_difficulty, idx, target))
            generators.append(lambda local_rng, local_difficulty, idx, target=target: self.random_suvat_edge_case(local_rng, local_difficulty, idx, target))
        generators.append(lambda local_rng, local_difficulty, idx: self.random_suvat_projectile_case(local_rng, local_difficulty, idx))
        generators.append(lambda local_rng, local_difficulty, idx: self.random_suvat_two_solution_case(local_rng, local_difficulty, idx))
        generators.append(lambda local_rng, local_difficulty, idx: self.random_university_suvat_case(local_rng, local_difficulty, idx))
        return self.build_unique_random_cases(generators, count, rng, difficulty)

    def random_suvat_projectile_case(self, rng, difficulty, index):
        return self.random_suvat_case(rng, difficulty, index, rng.choice(["s", "u", "v", "a", "t"]))

    def random_suvat_two_solution_case(self, rng, difficulty, index):
        return self.random_suvat_edge_case(rng, difficulty, index, "a")

    def random_stats_one_var_case(self, rng, difficulty, index):
        size = rng.randint(3, 12 if difficulty != "chaos" else 30)
        vals = []
        for _ in range(size):
            if difficulty == "chaos" and rng.random() < 0.25:
                vals.append(str(rng.choice([-1, 1]) * self.random_unbounded_count(rng, minimum=10**6, continue_probability=0.20)))
            else:
                vals.append(str(rng.randint(-50, 50)))
        inp = "1\n" + ",".join(vals) + "\n"
        label = f"Stats one-var {index}"
        return self.make_cli_case("Stats", "statsProgram.py", inp, label, stats_checker("mean", "sxx"), feature="stats_one_var")

    def random_stats_regression_case(self, rng, difficulty, index):
        n = rng.randint(4, 10)
        a = rng.randint(-20, 20)
        b = rng.choice([-5, -3, -2, -1, 1, 2, 3, 5])
        scale = 10**rng.randint(0, 4) if difficulty == "chaos" else 1
        xs = [scale * (i - n // 2) for i in range(n)]
        ys = [a + b * x for x in xs]
        inp = "2\n" + ",".join(map(str, xs)) + "\n" + ",".join(map(str, ys)) + "\n"
        label = f"Stats regression {index}"
        return self.make_cli_case("Stats", "statsProgram.py", inp, label, stats_checker("sxy", "r ="), feature="stats_regression")

    def random_stats_binomial_case(self, rng, difficulty, index):
        if difficulty == "chaos":
            n = rng.choice([100, 1000, 10000, rng.randint(50, 300)])
            p = rng.choice([0.001, 0.01, 0.5, 0.97])
        else:
            n = rng.randint(5, 60)
            p = rng.choice([0.1, 0.2, 0.35, 0.5, 0.75])
        r = max(0, min(n, int(round(n * p + rng.randint(-4, 4)))))
        mode = rng.choice(["pmf", "cdf", "tail"])
        inp = f"3\n{n},{p},{r},{mode}\n"
        label = f"Stats binomial {index}: {mode}"
        token = "p("
        return self.make_cli_case("Stats", "statsProgram.py", inp, label, stats_checker(token), feature=f"stats_binomial:{mode}", use_calculated=False)

    def random_stats_normal_case(self, rng, difficulty, index):
        mu = rng.randint(-100, 100)
        sigma = rng.choice([1, 2, 5, 10, 25])
        lo = mu - rng.randint(1, 4) * sigma
        hi = mu + rng.randint(1, 4) * sigma
        if difficulty == "chaos":
            sigma *= rng.choice([1, 10, 1000])
            lo = mu - rng.randint(1, 6) * sigma
            hi = mu + rng.randint(1, 6) * sigma
        inp = f"4\n{mu},{sigma},{lo},{hi}\n"
        label = f"Stats normal {index}"
        return self.make_cli_case("Stats", "statsProgram.py", inp, label, stats_checker("z1 ="), feature="stats_normal")

    def random_stats_ztest_case(self, rng, difficulty, index):
        mu = rng.randint(-50, 50)
        sigma = rng.choice([2, 5, 10, 15, 30])
        n = rng.choice([9, 16, 25, 36, 100])
        shift = rng.choice([-3, -2, -1, 1, 2, 3]) * sigma / math.sqrt(n)
        xbar = mu + shift
        tail = rng.choice(["two", "gt", "lt"])
        inp = f"5\n{xbar:.6g},{mu},{sigma},{n},{tail},0.05\n"
        label = f"Stats z-test {index}: {tail}"
        return self.make_cli_case("Stats", "statsProgram.py", inp, label, stats_checker("h0", "tail p"), feature=f"stats_ztest:{tail}")

    def random_stats_plot_case(self, rng, difficulty, index):
        expr = rng.choice(["x^2-4", "sin(x)", "cos(x)", "x^3-x", "exp(x/3)-2"])
        lo, hi = (-6, 6) if difficulty == "chaos" else (-3, 3)
        points = rng.choice([9, 13, 21, 41])
        inp = f"7\n{expr},{lo},{hi},{points}\n"
        label = f"Stats plot {index}: {expr}"
        return self.make_cli_case("Stats", "statsProgram.py", inp, label, stats_checker("spark"), feature="stats_plot")

    def build_random_stats_cases(self, difficulty, count, rng):
        if getattr(self, "backend", "python") != "c":
            return []
        features = [
            self.random_stats_one_var_case,
            self.random_stats_regression_case,
            self.random_stats_binomial_case,
            self.random_stats_normal_case,
            self.random_stats_ztest_case,
            self.random_stats_plot_case,
        ]
        return self.build_unique_random_cases(features, count, rng, difficulty)

    def random_boolean_expr(self, rng, depth=3):
        atoms = ["A", "B", "C", "D", "E", "0", "1"]
        if depth <= 0 or rng.random() < 0.28:
            base = rng.choice(atoms)
        else:
            left = self.random_boolean_expr(rng, depth - 1)
            right = self.random_boolean_expr(rng, depth - 1)
            op = rng.choice([".", "+"])
            if rng.random() < 0.35:
                left = "({0})".format(left)
            if rng.random() < 0.35:
                right = "({0})".format(right)
            base = "{0}{1}{2}".format(left, op, right)
        if rng.random() < 0.28:
            base = "{0},".format(base if base.startswith("(") else "({0})".format(base))
        return base

    def random_boolean_simplify_case(self, rng, difficulty, index):
        templates = [
            "A.A,",
            "A+A.B",
            "((B,.A),.B,),+A.B",
            "A.(B+C)+A.B,",
            "(A+B).(A+B,)",
        ]
        expr = rng.choice(templates) if rng.random() < 0.45 else self.random_boolean_expr(rng, 5 if difficulty == "chaos" else 3)
        inp = "1\n{0}\n".format(expr)
        label = "Boolean simplify {0}".format(index)
        checker = build_checker(contains_all=("Result:",), forbid=("Error:", "ERR:"), min_lines=2)
        return self.make_cli_case("Boolean", "ComputerScience/booleanProgram.py", inp, label, checker, feature="boolean_simplify", use_calculated=False)

    def random_boolean_nand_case(self, rng, difficulty, index):
        expr = self.random_boolean_expr(rng, 4 if difficulty == "chaos" else 2)
        inp = "2\n{0}\n".format(expr)
        label = "Boolean NAND {0}".format(index)
        checker = build_checker(contains_all=("NAND form:",), forbid=("Error:", "ERR:"), min_lines=2)
        return self.make_cli_case("Boolean", "ComputerScience/booleanProgram.py", inp, label, checker, feature="boolean_nand", use_calculated=False)

    def random_boolean_nor_case(self, rng, difficulty, index):
        expr = self.random_boolean_expr(rng, 4 if difficulty == "chaos" else 2)
        inp = "3\n{0}\n".format(expr)
        label = "Boolean NOR {0}".format(index)
        checker = build_checker(contains_all=("NOR form:",), forbid=("Error:", "ERR:"), min_lines=2)
        return self.make_cli_case("Boolean", "ComputerScience/booleanProgram.py", inp, label, checker, feature="boolean_nor", use_calculated=False)

    def random_boolean_prove_case(self, rng, difficulty, index):
        pairs = [
            ("A.(B+C)", "A.B+A.C"),
            ("A+A.B", "A"),
            ("(A+B).(A+C)", "A+B.C"),
            ("A+A,", "1"),
            ("A.A,", "0"),
        ]
        lhs, rhs = rng.choice(pairs)
        inp = "4\n{0}\n{1}\n".format(lhs, rhs)
        label = "Boolean prove {0}".format(index)
        checker = build_checker(contains_any=("proved", "Both sides", "OK", "Result:"), forbid=("Could not prove", "Error:", "ERR:"), min_lines=3)
        return self.make_cli_case("Boolean", "ComputerScience/booleanProgram.py", inp, label, checker, feature="boolean_prove", use_calculated=False)

    def build_random_boolean_cases(self, difficulty, count, rng):
        features = [
            self.random_boolean_simplify_case,
            self.random_boolean_nand_case,
            self.random_boolean_nor_case,
            self.random_boolean_prove_case,
        ]
        return self.build_unique_random_cases(features, count, rng, difficulty)

    def random_university_algebra_case(self, rng, difficulty, index):
        mode = rng.choice([
            "quad_quad_sub", "circle_line", "discriminate_one",
            "three_var_sum", "param_one_sol", "circle_circle",
        ])
        if mode == "quad_quad_sub":
            a, b = rng.randint(1, 3), rng.randint(1, 3)
            x_expr = f"{a}*x+{b}"
            expr = f"y={x_expr}\ny^2={a}^2*x^2+{b*a*2}*x+{b}^2-{rng.randint(3,12)}"
            cli_input = f"6\nx+{a}*y={b}\nx^2+{a}^2*y^2={rng.randint(15,40)}\n"
            checker = build_checker(contains_all=("expr =", "x ="), min_steps=2)
        elif mode == "circle_line":
            h, k = rng.randint(-4, 4), rng.randint(-4, 4)
            r = rng.randint(3, 6)
            m = rng.randint(-3, 3)
            cli_input = f"6\nx^2+y^2+{2*h}*x+{2*k}*y={h**2+k**2-r**2}\ny={m}*x+{rng.randint(-3,3)}\n"
            checker = build_checker(contains_all=("x =", "y ="), min_steps=3)
        elif mode == "discriminate_one":
            k = rng.randint(-5, 5)
            cli_input = f"6\nx+y={k}\nx*y={1}\n"
            checker = build_checker(contains_all=("x =", "y =", "expr ="), contains_any=("one solution",))
        elif mode == "three_var_sum":
            cli_input = f"6\nx+y+z={rng.randint(3,9)}\nx^2+y^2+z^2={rng.randint(14,30)}\nxy+yz+zx={rng.randint(5,20)}\n"
            checker = algebra_compare_checker("equivalent")
        elif mode == "param_one_sol":
            cli_input = f"6\nx+y={rng.randint(1,4)}\nx+2y={rng.randint(2,6)}\n2x+{rng.randint(2,3)}*y={rng.randint(3,8)}\n"
            checker = build_checker(contains_all=("expr =", "x ="), min_steps=2)
        else:
            cli_input = f"6\nx^2+y^2={rng.randint(18,40)}\n(x-{rng.randint(1,3)})^2+(y-{rng.randint(1,3)})^2={rng.randint(9,25)}\n"
            checker = build_checker(contains_all=("x =", "y ="), min_steps=3)
        label = f"University algebra {index}: {mode}"
        return self.make_cli_case("Algebra", "algebraProgram.py", cli_input, label, checker, feature=f"algebra_uni:{mode}")

    def random_university_trig_case(self, rng, difficulty, index):
        mode = rng.choice([
            "hard_ident_1", "hard_ident_2", "solve_quad_trig",
            "complex_solve", "exact_value", "hard_identity",
        ])
        if mode == "hard_ident_1":
            cli_input = trig_prove_cli("sec(x)^4-tan(x)^4", "1+2*tan(x)^2")
            checker = trig_prove_checker("1+2*tan")
        elif mode == "hard_ident_2":
            cli_input = trig_prove_cli("(sec(x)-tan(x))^2", "tan(x)^2-sin(x)^2")
            checker = trig_prove_checker("tan")
        elif mode == "solve_quad_trig":
            cli_input = "3\ntan(x)^2+sec(x)^2+5*sec(x)=2,x,0,2*pi\n"
            checker = trig_solve_checker("x =")
        elif mode == "complex_solve":
            cli_input = "3\n2*cot(x)^2-cosec(x)^2+cosec(x)=4,x,0,2*pi\n"
            checker = trig_solve_checker("x =")
        elif mode == "exact_value":
            cli_input = "3\nsin(4*x)=sqrt(2)/2,x,pi/4,pi/2\n"
            checker = trig_solve_checker("x =")
        else:
            cli_input = trig_prove_cli("cot(x)^2*cosec(x)^2-cosec(x)^2", "cot(x)^2")
            checker = trig_prove_checker("cot")
        label = f"University trig {index}: {mode}"
        return self.make_cli_case("Trigonometry", "trigProgram.py", cli_input, label, checker, feature=f"trig_uni:{mode}")

    def random_university_derive_case(self, rng, difficulty, index):
        mode = rng.choice([
            "product_chain", "quotient_chain", "log_diff",
            "implicit_hard", "parametric_hard", "high_order",
        ])
        if mode == "product_chain":
            cli_input = "1\n(5*x^3-2)*log(x^2+1)\n"
            checker = derive_checker("dy/dx", "product rule", "chain rule")
        elif mode == "quotient_chain":
            cli_input = "1\n((8*x^2-5)/(2-x))^3\n"
            checker = derive_checker("dy/dx", "quotient rule", "chain rule")
        elif mode == "log_diff":
            cli_input = "1\nx^sin(x)\n"
            checker = derive_checker("dy/dx", "log", "chain rule")
        elif mode == "implicit_hard":
            cli_input = "2\nx^3-3*x*y+y^2=7\n"
            checker = derive_implicit_checker("dy/dx", "d/dx")
        elif mode == "parametric_hard":
            cli_input = "3\na*t/(t^2+1)\nt^3+4*t^2\n"
            checker = derive_param_checker("dx/dt", "dy/dt", "dy/dx")
        else:
            cli_input = "1\ne^x*sin(x)\n"
            checker = derive_checker("dy/dx", "product rule", "chain rule")
        label = f"University derive {index}: {mode}"
        return self.make_cli_case("Derive", "deriveProgram.py", cli_input, label, checker, feature=f"derive_uni:{mode}")

    def random_university_integrate_case(self, rng, difficulty, index):
        mode = rng.choice([
            "parts_twice", "partial_frac", "trig_power",
            "exp_sin_cos", "sub_sqrt", "hard_rational",
        ])
        if mode == "parts_twice":
            cli_input = "1\nx^3*exp(x)\n"
            checker = integrate_checker("integration by parts")
        elif mode == "partial_frac":
            cli_input = "1\nx^2/((x+2)^2*(x^2+8))\n"
            checker = integrate_checker("partial fractions", "divide")
        elif mode == "trig_power":
            cli_input = "1\nsin(x)^4\n"
            checker = integrate_checker("use ")
        elif mode == "exp_sin_cos":
            cli_input = "1\ne^x*sin(x)\n"
            checker = integrate_checker("integration by parts", "circular")
        elif mode == "sub_sqrt":
            cli_input = "1\nx/sqrt(x^3+1)\n"
            checker = integrate_checker("u =")
        else:
            cli_input = "1\nx^5*sqrt(x^3+1)\n"
            checker = integrate_checker()
        label = f"University integrate {index}: {mode}"
        return self.make_cli_case("Integrate", "intProgram.py", cli_input, label, checker, feature=f"integrate_uni:{mode}")

    def random_university_suvat_case(self, rng, difficulty, index):
        target = rng.choice(["s", "u", "v", "a", "t"])
        case = self.random_suvat_case(rng, difficulty, index, target)
        case.label = f"University SUVAT {index}: exact kinematics"
        case.feature = f"suvat_uni:{target}"
        return case

    def random_method_surface_case(self, rng, difficulty, index):
        cases = [
            ("int", "x^2,method=auto", "auto", True),
            ("int", "x^2,method=direct", "direct", True),
            ("int", "2*x*cos(x^2),method=reverse_chain", "reverse_chain", True),
            ("int", "x*cos(x^2),method=sub,u=x^2", "sub", True),
            ("int", "x*exp(x),method=parts", "parts", True),
            ("int", "x^4*exp(x),method=di", "di", True),
            ("int", "sin(x)^4,method=trig", "trig", True),
            ("int", "1/((x-1)*(x+2)),method=pf", "pf", True),
            ("int", "(x^3+1)/(x+1),method=div", "div", True),
            ("int", "1/(2+cos(x)),method=weierstrass", "weierstrass", True),
            ("int", "sin(x)/(sin(x)+cos(x)),method=symmetry", "symmetry", True),
            ("derive", "x^3,method=auto", "auto", True),
            ("derive", "sin(x^2),method=chain", "chain", True),
            ("derive", "x^2*exp(x),method=product", "product", True),
            ("derive", "sin(x)/(x^2+1),method=quotient", "quotient", True),
            ("derive", "x^x,method=logdiff", "logdiff", True),
            ("derive", "x^2+y^2=1,method=implicit", "implicit", True),
            ("derive", "t^2,t^3,t,method=param", "param", True),
            ("derive", "x^4,method=second", "second", True),
            ("derive", "t^2+1/t,t^2-1/t,t,method=param_second", "param_second", True),
            ("trig", "sin(x)=1/2,x,0,2*pi,6,method=auto", "auto", True),
            ("trig", "sin(x)-1/2,method=general", "general", True),
            ("trig", "sin(x)=1/2,x,0,2*pi,6,method=bounded", "bounded", True),
            ("trig", "sin(x)=1/2,x,0,360,6,method=cast", "cast", True),
            ("trig", "sin(x)^2+cos(x)^2,method=identity", "identity", True),
            ("trig", "3*cos(x)+4*sin(x)=2,x,0,2*pi,method=rform", "rform", True),
            ("trig", "sin(x)^2=1/4,x,0,2*pi,method=square_then_check", "square_then_check", True),
            ("trig", "sin(x)^2+cos(x)^2,method=sin_cos", "sin_cos", True),
            ("trig", "sec(x)^2-tan(x)^2,method=pythag", "pythag", True),
            ("trig", "sin(2*x),method=double_angle", "double_angle", True),
            ("trig", "sin(x+y),method=compound_angle", "compound_angle", True),
            ("trig", "sin(x)^2+cos(x)^2,target=1,method=target", "target", True),
            ("alg", "2*x+3=11,method=auto", "auto", True),
            ("alg", "2*x+3=11,method=linear", "linear", True),
            ("alg", "x^2-5*x+6=0,method=factor", "factor", True),
            ("alg", "x^2-5*x+6=0,method=quad_formula", "quad_formula", True),
            ("alg", "x^2+6*x+5=0,method=complete_square", "complete_square", True),
            ("alg", "x^4-5*x^2+4=0,method=substitution", "substitution", True),
            ("alg", "1/x+1/(x+1)=1/2,method=clear_denoms", "clear_denoms", True),
            ("alg", "2^(2*x)-5*2^x+4=0,method=log_exp", "log_exp", True),
            ("alg", "x^2-2=0,method=numeric", "numeric", True),
            ("alg", "x^2-2=0,x,-2,2,method=interval", "interval", True),
            ("alg", "(x+1)^3,method=expand", "expand", True),
            ("alg", "x^2+2*x+1,method=collect", "collect", True),
            ("alg", "1/(sqrt(x)+1),method=rationalise", "rationalise", True),
            ("alg", "x^2+2*x+1,method=canonical", "canonical", True),
            ("alg", "x^2+a*x+b,target=(x+1)^2,method=target", "target", True),
            ("alg", "(a*x+b)(x-2)=4*x^2+6*x-1,method=equate_coeffs", "equate_coeffs", True),
            ("alg", "2*x+y=5,x-y=1,method=simultaneous", "simultaneous", True),
            ("stats", "1,2,3,4,method=auto", "auto", True),
            ("stats", "1,2,3,4,method=summary", "summary", True),
            ("stats", "1,2,3,4,method=regression", "regression", True),
            ("stats", "1,2,3,4,method=hypothesis_test", "hypothesis_test", True),
            ("stats", "binomial(10,.5,4),method=binomial", "binomial", True),
            ("stats", "normal(0,1,1.96),method=normal", "normal", True),
            ("stats", "poisson(3,2),method=poisson", "poisson", True),
            ("stats", "1,2,3,4,method=confidence_interval", "confidence_interval", True),
            ("suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=auto", "auto", True),
            ("suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=suvat", "suvat", True),
            ("suvat", "s=,u=20,v=,a=-10,t=2,target=v,method=projectile", "projectile", True),
            ("suvat", "s=,u=0,v=,a=2,t=5,target=v,method=forces", "forces", True),
            ("suvat", "s=10,u=,v=,a=2,t=3,target=u,method=variable_accel", "variable_accel", True),
            ("suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=energy", "energy", True),
            ("suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=moments", "moments", True),
            ("int", "x^2,method=not_a_method", "invalid", False),
        ]
        core = [
            ("int", "x^2,method=auto", "auto", True),
            ("derive", "x^3,method=auto", "auto", True),
            ("trig", "sin(x)=1/2,x,0,2*pi,6,method=auto", "auto", True),
            ("alg", "2*x+3=11,method=auto", "auto", True),
            ("stats", "1,2,3,4,method=auto", "auto", True),
            ("suvat", "s=,u=0,v=10,a=2,t=5,target=s,method=auto", "auto", True),
            ("int", "x^2,method=not_a_method", "invalid", False),
        ]
        flag, expr, method, valid = core[index - 1] if 1 <= index <= len(core) else rng.choice(cases)
        label = f"Method {index}: {flag}:{method}"
        feature = "method_surface:invalid" if not valid else f"method_surface:{flag}:{method}"

        def runner():
            out, err = self.run_host_feature(flag, expr)
            combined = out if not err else out + "\n" + err
            text = normalized_text(combined)
            if not valid:
                ok = "invalid method" in text and "auto result:" not in text
            elif method == "auto":
                ok = "invalid method" not in text and any(token in text for token in ("dy/dx", " = ", "int("))
            else:
                ok = "invalid method" not in text and any(token in text for token in ("dy/dx", " = ", "int(", "+ c"))
            return ok, combined

        return self.make_direct_case(
            "MethodSurface",
            label,
            runner,
            input_text=f"--{flag} {expr}",
            check_info="method forced/auto/invalid surface; output not answer-only",
            feature=feature,
        )

    def build_random_method_surface_cases(self, difficulty, count, rng):
        return self.build_unique_random_cases([self.random_method_surface_case], count, rng, difficulty)

    def build_university_cases(self, difficulty, count, rng):
        features = [
            self.random_algebra_compare_case,
            self.random_algebra_transform_case,
            self.random_algebra_expand_case,
            self.random_algebra_poly_case,
            self.random_algebra_complete_square_case,
            self.random_algebra_solve_case,
            self.random_algebra_comp_case,
            self.random_algebra_inverse_case,
            self.random_algebra_rewrite_case,
            self.random_trig_prove_case,
            self.random_trig_transform_case,
            self.random_trig_solve_case,
            self.random_trig_rewrite_case,
            self.random_derive_normal_case,
            self.random_derive_implicit_case,
            self.random_derive_parametric_case,
            self.random_integrate_auto_case,
            lambda local_rng, local_difficulty, idx: self.random_suvat_case(
                local_rng, local_difficulty, idx, local_rng.choice(["s", "u", "v", "a", "t"])
            ),
            lambda local_rng, local_difficulty, idx: self.random_suvat_edge_case(
                local_rng, local_difficulty, idx, local_rng.choice(["s", "u", "v", "a", "t"])
            ),
            self.random_boolean_simplify_case,
            self.random_boolean_nand_case,
            self.random_boolean_nor_case,
            self.random_boolean_prove_case,
        ]
        super_hard_rng = random.Random(rng.randint(1, 99999))
        cases = self.build_unique_random_cases(features, count, super_hard_rng, "chaos")
        return cases

    def random_builders_for_program(self, program):
        """Return the random-case builders included in the requested scope."""
        if program == "CatalogueGraph":
            return [("CatalogueGraph", self.build_catalogue_explorer_cases)]
        builders = [
            ("Adversarial", self.build_adversarial_random_cases),
            ("CatalogueGraph", self.build_catalogue_explorer_cases),
            ("Algebra", self.build_random_algebra_cases),
            ("Trigonometry", self.build_random_trig_cases),
            ("Derive", self.build_random_derive_cases),
            ("Integrate", self.build_random_integrate_cases),
            ("Stats", self.build_random_stats_cases),
            ("SUVAT", self.build_random_suvat_cases),
            ("Boolean", self.build_random_boolean_cases),
            ("MethodSurface", self.build_random_method_surface_cases),
            ("ExamMix", self.build_university_cases),
        ]
        if program is None or program == "all":
            return builders
        return [(name, builder) for name, builder in builders if name == program]

    def random_program_counts(self, total_count, builders, infinite_mode):
        """Quota finite runs toward known feature gaps; infinite keeps chunks."""
        builder_count = len(builders)
        if infinite_mode:
            return [None] * builder_count
        quotas = {
            "Adversarial": 12,
            "CatalogueGraph": 2,
            "Algebra": 22,
            "Trigonometry": 9,
            "Derive": 9,
            "Integrate": 12,
            "Stats": 6,
            "SUVAT": 13,
            "Boolean": 4,
            "MethodSurface": 7,
            "ExamMix": 4,
        }
        weights = [max(1, quotas.get(name, 1)) for name, _ in builders]
        total_weight = sum(weights)
        if total_count <= 0 or total_weight <= 0:
            return self.balanced_counts(total_count, builder_count)
        exact = [total_count * (w / total_weight) for w in weights]
        out = [int(x) for x in exact]
        if total_count >= builder_count:
            for i, value in enumerate(out):
                if value == 0:
                    out[i] = 1
        while sum(out) > total_count:
            i = max(range(len(out)), key=lambda j: out[j])
            out[i] -= 1
        rem = total_count - sum(out)
        frac = sorted(((i, exact[i] - int(exact[i])) for i in range(len(out))), key=lambda z: -z[1])
        for j in range(rem):
            out[frac[j % len(out)][0]] += 1
        return out

    def iter_random_test_batches(self, builders, program_counts, generation_chunk, infinite_mode):
        """Yield random-test batches, cycling forever when infinite_mode is true."""
        cycle = 0
        while True:
            cycle += 1
            # Outer cycle: in infinite mode this is the forever loop. It ends
            # only when the caller stops consuming batches after /stop.
            for index, item in enumerate(builders):
                program_name, builder = item
                remaining = program_counts[index]
                if not infinite_mode and remaining <= 0:
                    continue

                chunk_index = 0
                # Inner loop: finite runs may need several chunks for one
                # program. Infinite runs yield one chunk per program per cycle
                # so every program keeps receiving coverage.
                while infinite_mode or remaining > 0:
                    chunk_index += 1
                    if infinite_mode:
                        this_count = generation_chunk
                    else:
                        this_count = min(generation_chunk, remaining)
                        remaining -= this_count
                    yield RandomTestBatch(
                        program_name,
                        builder,
                        this_count,
                        cycle,
                        chunk_index,
                        chunk_index == 1,
                    )
                    if infinite_mode:
                        break
            if not infinite_mode:
                break

    def random_stop_file_path(self):
        return str(Path(__file__).resolve().parent / "reports" / ".casio_stop")

    def consume_random_stop_file(self):
        """Consume either project-local or user-home stop markers."""
        paths = [self.random_stop_file_path(), os.path.expanduser("~/.casio_stop")]
        for stop_file in paths:
            if os.path.exists(stop_file):
                try:
                    os.remove(stop_file)
                except OSError:
                    continue
                return True
        return False

    def random_stop_requested(self, emit, message="[dim]Stop requested - ending after this batch[/dim]"):
        """Single stop check used by the random runner before and after batches."""
        if self.is_stopping():
            return True
        if self.consume_random_stop_file():
            self.request_stop()
            emit(self.append_result, message)
            return True
        return False

    def reset_random_run_metrics(self, expected_count):
        import time
        self.start_time = time.time()
        self.total_expected = expected_count
        self.random_graph = RandomExplorationGraph(CATALOGUE_GRAPH_PATH)
        self.random_graph.nodes = {}
        self.random_graph._dirty = 1
        self.random_graph.save()
        self._last_eta_update = 0
        self.records = []
        self.current_run_question_keys.clear()
        self.session_random_question_keys.clear()
        self._test_times = []
        self._llm_times = []
        self._test_timestamps = []
        self._llm_timestamps = []
        self._weighted_rate = 0.0
        self._weighted_llm_rate = 0.0
        self._feature_stats = {}

    def announce_random_llm_status(self, emit):
        """Explain whether every random output will receive LLM verification."""
        if self.llm_manager and self.llm_manager.enabled:
            emit(self.append_result, "[dim]LLM:[/dim] Verifying connection to Ollama...")
            try:
                status = self.llm_manager.get_status()
                model = status.get("selected_model", "unknown")
                available = status.get("available", False)
                if available:
                    self.llm_enabled_for_tests = True
                    emit(self.append_result, f"[dim]LLM:[/dim] [green]Connected[/green] · Model: {model}")
                else:
                    self.llm_enabled_for_tests = False
                    emit(self.append_result, "[dim]LLM:[/dim] [yellow]Ollama not available[/yellow]")
            except Exception as err:
                self.llm_enabled_for_tests = False
                emit(self.append_result, f"[dim]LLM:[/dim] [red]Error: {err}[/red]")
            return

        if self.llm_manager:
            self.llm_enabled_for_tests = False
            emit(self.append_result, "[dim]LLM:[/dim] [yellow]LLM disabled[/yellow]")
            return

        if LLM_AVAILABLE:
            emit(self.append_result, "[dim]LLM:[/dim] [dim]Not enabled. Run /llm 1 before /random to verify every output.[/dim]")
        else:
            emit(self.append_result, "[dim]LLM:[/dim] [dim]No LLM module available[/dim]")

    def run_random_tests(self, difficulty, count, workers=None, program=None):
        actual_count = count if count else 200
        infinite_mode = count is None or count <= 0
        def run():
            # This runs in a background thread for the Textual UI and inline for
            # plain mode.  The finally block below must restore the live status no
            # matter which exit path fires.
            self._random_infinite = infinite_mode
            try:
                emit = (lambda fn, *args: fn(*args)) if getattr(self, 'plain_mode', False) else self.call_from_thread
                active_workers = workers or CASE_WORKERS
                if self.run_state != RunState.RUNNING:
                    self.transition_run_state(RunState.RUNNING)
                self._live_phase = "running"
                if self.consume_random_stop_file():
                    # Clear stale /stop markers from previous processes before
                    # this new run starts, then tell the user what happened.
                    emit(self.append_result, "[dim]Cleared previous stop marker[/dim]")

                self._show_chaos_feature_gaps = True
                self.announce_random_llm_status(emit)

                emit(
                    self.append_result,
                    f"[bold #e07a53]▶ Running random tests...[/bold #e07a53]",
                )
                emit(
                    self.append_result,
                    f"[dim]Press /stop to end · Tests run: 0[/dim]",
                )

                rng = random.Random()
                builders = self.random_builders_for_program(program)

                if not builders:
                    self.transition_run_state(RunState.STOPPED)
                    emit(self.append_result, "[bold #f59e0b]Unknown program for random run.[/bold #f59e0b]")
                    emit(self.render_summary)
                    return

                generation_chunk = 200
                if infinite_mode:
                    generation_chunk = _CASIO_INFINITE_GENERATION_CHUNK

                program_counts = self.random_program_counts(actual_count, builders, infinite_mode)
                expected_count = len(builders) * generation_chunk if infinite_mode else sum(program_counts)
                self.reset_random_run_metrics(expected_count)

                self.current_program = program or "all"
                self.last_run_scope = (self.current_program, difficulty)
                self._init_session_report_file()
                scope = f"{program} · " if program else ""
                if program is None or program == "CatalogueGraph":
                    self.announce_catalogue_graph(emit)

                emit(
                    self.append_result,
                    f"[dim]{self.total_expected} {scope}{difficulty} tests · {active_workers} workers[/dim]" if not infinite_mode else f"[dim]{scope}{difficulty} infinite tests · {active_workers} workers[/dim]",
                )

                if infinite_mode:
                    emit(self.append_result, "[dim]Infinite mode: press /stop to end · dedupe is per-batch (long runs ok)[/dim]")

                if not getattr(self, "plain_mode", False) and self.total_expected > 0:
                    emit(self.update_progress_bar, 0, self.total_expected)

                last_program = None
                # Batch generation owns the outer "which program next?" loop.
                # In infinite mode this generator keeps cycling until the single
                # stop check below sees /stop or a stop marker.
                for batch in self.iter_random_test_batches(builders, program_counts, generation_chunk, infinite_mode):
                    if self.random_stop_requested(emit):
                        break
                    if batch.program != last_program:
                        self._live_program = batch.program
                        self._live_phase = "generating"
                        emit(self.append_result, "")
                        emit(self.append_result, f"[bold #e07a53]▶ {batch.program}[/bold #e07a53]")
                        last_program = batch.program
                    if batch.first_for_program:
                        if infinite_mode:
                            emit(self.append_result, "[dim]Generating cases...[/dim]")
                        else:
                            emit(self.append_result, f"[dim]Generating first {batch.count} random cases...[/dim]")

                    cases = batch.builder(difficulty, batch.count, rng)
                    self._live_phase = "testing"
                    if not cases and infinite_mode:
                        emit(self.append_result, "[dim][skip] empty case batch (retry next cycle)[/dim]")
                    # LLM verification, when enabled by /llm, happens inside
                    # run_case_specs so every produced output is checked once.
                    self.run_case_specs(cases, workers=active_workers, infinite_mode=infinite_mode)
                    test_count = len(self.records)
                    test_total = self.total_expected if self.total_expected > 0 else test_count
                    emit(self.update_progress_bar, test_count, test_total)
                    if infinite_mode:
                        self.total_expected = len(self.records) + generation_chunk * len(builders)
                    if self.random_stop_requested(emit):
                        break

                if self.run_state == RunState.RUNNING:
                    self.transition_run_state(RunState.STOPPED)
                if getattr(self, 'plain_mode', False):
                    self.render_summary()
                else:
                    self.call_from_thread(self.render_summary)
            finally:
                self._random_infinite = False
                self._live_phase = "ready" if self.run_state != RunState.RUNNING else self._live_phase

        if getattr(self, 'plain_mode', False):
            run()
            return
        import threading
        threading.Thread(target=run, daemon=True).start()

    def calculated_checker_for_cli_case(self, script, inp, expected):
        lines = (inp or "").splitlines()
        if not lines:
            return expected if callable(expected) else None
        mode = lines[0].strip()
        try:
            if script == "algebraProgram.py":
                if mode == "1" and len(lines) >= 3:
                    return algebra_compare_output_checker(lines[1], lines[2], "not" not in str(expected).lower())
                if mode == "2" and len(lines) >= 3:
                    return transform_output_checker(lines[1], lines[2], "Algebra")
                if mode == "6" and len(lines) >= 2:
                    v = infer_algebra_solve_var_from_eq_line(lines[1])
                    return self.algebra_solve_output_checker(lines[1], var=v)
            if script == "algebraProgram.py" and mode in ("3", "5"):
                return expected if callable(expected) else None
            if script == "trigProgram.py":
                if mode == "1" and len(lines) >= 3 and lines[2].strip() not in ("1", "2", "3", "4"):
                    return trig_identity_output_checker(lines[1] + "=" + lines[2])
                if mode == "1" and len(lines) >= 2 and "=" in lines[1]:
                    return trig_identity_output_checker(lines[1])
                if mode == "2" and len(lines) >= 3:
                    return transform_output_checker(lines[1], lines[2], "Trigonometry")
                if mode == "3" and len(lines) >= 2:
                    return self.trig_cli_solve_checker(lines[1])
            if script == "deriveProgram.py":
                if mode == "1" and len(lines) >= 2:
                    return self.derive_output_checker(lines[1])
                if mode == "2" and len(lines) >= 2:
                    return self.derive_implicit_output_checker(lines[1])
                if mode == "3" and len(lines) >= 3:
                    return self.parametric_output_checker(lines[1], lines[2])
            if script == "intProgram.py" and len(lines) >= 3 and lines[2].strip() == "7":
                return expected if callable(expected) else None
            if script == "intProgram.py" and mode == "1" and len(lines) >= 2:
                return self.integrate_output_checker(lines[1])
            if script == "statsProgram.py" and mode == "4" and len(lines) >= 2:
                return stats_normal_output_checker(lines[1])
        except Exception:
            return expected if callable(expected) else None
        return expected if callable(expected) else None

    def run_simple_cases(self, script, program, cases, default_check):
        def evaluate(case):
            inp, label, expected = case
            out, _ = self.run_cli(script, inp)
            calculated = self.calculated_checker_for_cli_case(script, inp, expected)
            if calculated is not None:
                passed = calculated(out)
            elif default_check == "contains":
                passed = expected in out
            elif default_check == "no_error":
                passed = "Error" not in out and expected in out
            else:
                passed = expected in out
            if calculated is not None:
                check_info = getattr(calculated, "__name__", "calculated checker")
            elif callable(expected):
                check_info = getattr(expected, "__name__", "custom checker")
            else:
                check_info = f"{default_check}: {expected}"
            return label, passed, out, inp, check_info

        with ThreadPoolExecutor(max_workers=CASE_WORKERS) as executor:
            for label, passed, out, inp, check_info in executor.map(evaluate, cases):
                self.add_test(label, passed, out, program, inp, check_info)

    def run_boolean(self, difficulty="all"):
        p = "Boolean"
        tests = [
            ("1\nA+A.B\n", "Boolean: simplify absorption", "boolean_simplify", build_checker(contains_all=("Result:",), forbid=("Error:", "ERR:"), min_lines=2)),
            ("2\nA.B+C\n", "Boolean: NAND form", "boolean_nand", build_checker(contains_all=("NAND form:",), forbid=("Error:", "ERR:"), min_lines=2)),
            ("3\nA+B.C\n", "Boolean: NOR form", "boolean_nor", build_checker(contains_all=("NOR form:",), forbid=("Error:", "ERR:"), min_lines=2)),
            ("4\nA.(B+C)\nA.B+A.C\n", "Boolean: prove distributive", "boolean_prove", build_checker(contains_any=("proved", "Both sides", "OK", "Result:"), forbid=("Could not prove", "Error:", "ERR:"), min_lines=3)),
        ]
        for inp, label, feature, checker in tests:
            out, err = self.run_cli("ComputerScience/booleanProgram.py", inp)
            combined = out if not err else out + "\n" + err
            self.add_test(label, checker(combined), combined, p, inp, getattr(checker, "__name__", "boolean checker"), feature)

    def run_algebra(self, difficulty="all"):
        p = "Algebra"

        # Core comparison tests
        tests = [
            ("1\n(x+1)^2\nx^2+2*x+1\n", "Equivalent", "Compare: (x+1)^2 ≟ x^2+2x+1"),
            ("1\nsin(x)^2+cos(x)^2\n1\n", "Equivalent", "Compare: sin²x+cos²x ≟ 1"),
            ("1\n(1-cos(2*x))/2\nsin(x)^2\n", "Equivalent", "Compare: (1-cos2x)/2 ≟ sin²x"),
            ("1\n(x^2-1)/(x-1)\nx+1\n", "Equivalent", "Compare: (x²-1)/(x-1) ≟ x+1"),
            ("1\nexp(log(x))\nx\n", "Equivalent", "Compare: e^ln(x) ≟ x"),
            ("1\nlog(e^x)\nx\n", "Equivalent", "Compare: ln(e^x) ≟ x"),
            ("1\n(x+2)*(x+3)\nx^2+5*x+6\n", "Equivalent", "Compare: (x+2)(x+3) ≟ x²+5x+6"),
            ("1\n(sec(x))^2-(tan(x))^2\n1\n", "Equivalent", "Compare: sec²x-tan²x ≟ 1"),
            ("1\n(cosec(x))^2-(cot(x))^2\n1\n", "Equivalent", "Compare: csc²x-cot²x ≟ 1"),
            ("1\n(x-1)*(x+1)\nx^2-1\n", "Equivalent", "Compare: (x-1)(x+1) ≟ x²-1"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in tests:
                out, _ = self.run_cli("algebraProgram.py", inp)
                _mode, expr1, expr2 = inp.splitlines()[:3]
                checker = algebra_compare_output_checker(expr1, expr2, expected_equal=True)
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated algebra compare"), "algebra_compare")
            inp = "1\n(x+1)²\nx²+2×x+1\n"
            out, _ = self.run_cli("algebraProgram.py", inp)
            checker = algebra_compare_output_checker("(x+1)^2", "x^2+2*x+1", expected_equal=True)
            self.add_test(
                "Abnormal input: unicode powers/multiply compare",
                checker(out),
                out,
                p,
                inp,
                getattr(checker, "__name__", "calculated algebra compare"),
                "algebra_compare:abnormal",
            )

        # Transform tests
        transforms = [
            ("2\n1-cos(2*x)\n2*sin(x)^2\n", "2*(sin(x))^2", "Transform: 1-cos(2x) → 2sin²x"),
            ("2\nsin(x)/cos(x)\ntan(x)\n", "tan", "Transform: sinx/cosx → tanx"),
            ("2\n1/cos(x)\nsec(x)\n", "sec", "Transform: 1/cosx → secx"),
            ("2\n1/sin(x)\ncosec(x)\n", "cosec", "Transform: 1/sinx → cscx"),
            ("2\ncos(x)/sin(x)\ncot(x)\n", "cot", "Transform: cosx/sinx → cotx"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "2*sin", "Transform: sin(2x) → 2sinxcosx"),
            ("2\ncos(2*x)\ncos(x)^2-sin(x)^2\n", "cos", "Transform: cos(2x) → cos²x-sin²x"),
            ("2\n1-cos(2*(4*(4*(x)+6)))\n2*sin(4*(4*(x)+6))^2\n", "sin", "Transform: nested 1-cos(2A) → 2sin²A"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in transforms:
                out, _ = self.run_cli("algebraProgram.py", inp)
                _mode, src, tgt = inp.splitlines()[:3]
                checker = transform_output_checker(src, tgt, "Algebra")
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated algebra transform"), "algebra_transform")

        expand_cases = [
            ("3\n(2*x+1)^5\n\n", "Expand: (2x+1)^5", algebra_expand_checker("32*x^5", "10*x", "1")),
            ("3\n(3*x-2)^6\n\n", "Expand: (3x-2)^6", algebra_expand_checker("729*x^6", "64")),
        ]

        complete_square_cases = [
            ("5\nx^2+6*x+13\n", "Complete square: x^2+6x+13", algebra_complete_square_checker("(x + 3)^2 + 4")),
            ("5\n4*x^2-12*x+25\n", "Complete square: 4x^2-12x+25", algebra_complete_square_checker("4*(x - 3/2)^2 + 16")),
        ]

        if difficulty in ("all", "medium", "hard"):
            self.run_simple_cases("algebraProgram.py", p, expand_cases, "contains")
            self.run_simple_cases("algebraProgram.py", p, complete_square_cases, "contains")

        if difficulty in ("all", "easy", "medium", "hard"):
            inp = "3\n(2*x+1)^5\n\n"
            out, _ = self.run_cli("algebraProgram.py", inp)
            checker = readable_output_checker(
                algebra_expand_checker("(2*x + 1)^5", "32*x^5 + 80*x^4", "Out =")
            )
            self.add_test(
                "Formatting: algebra groups expanded factors and spaced sums",
                checker(out),
                out,
                p,
                inp,
                "readable output with bracketed factors and spaced sums",
                "algebra_formatting",
            )

        # Solve tests
        solves = [
            ("6\nx^2+5*x+6=0\n", "x =", "Solve: x²+5x+6=0"),
            ("6\n(x+1)/(x-2)=3\n", "7/2", "Solve: (x+1)/(x-2)=3"),
            ("6\n2*x+6=0\n", "x = -3", "Solve: 2x+6=0"),
            ("6\nx^2-9=0\n", "x =", "Solve: x²-9=0"),
            ("6\n2*x^2+7*x+3=0\n", "x =", "Solve: 2x²+7x+3=0"),
            ("6\n4*x^2-9=0\n", "x =", "Solve: 4x²-9=0"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in solves:
                out, _ = self.run_cli("algebraProgram.py", inp)
                eq = inp.splitlines()[1]
                checker = self.algebra_solve_output_checker(eq)
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated algebra solve"), "algebra_solve")

            dr = (
                "10\n(t-1)^2+1, t, 0, 4\n",
                "Mode 10: domain and range on an interval in t with explicit bounds",
            )
            out, _ = self.run_cli("algebraProgram.py", dr[0])
            dchk = build_checker(
                contains_all=("domain:", "variable = t", "interval of interest", "on [", "range:", "unrestricted", "on the interval", "t"),
                min_lines=5,
            )
            self.add_test(
                dr[1],
                dchk(out) and "err:" not in normalized_text(out) and "err" not in normalized_text(out)[:20],
                out,
                p,
                dr[0],
                "domain/range with interval and alternate letter t",
                "algebra_domain_range_interval",
            )

            range_identity_cases = [
                ("10\nsec(2*x)^2-tan(2*x)^2\n", "Range: sec²-tan² identity", "answer: y = 1"),
                ("10\ncosec(2*x+pi/6)^2-cot(2*x+pi/6)^2\n", "Range: cosec²-cot² identity", "answer: y = 1"),
                ("10\n(sin(x)+cos(x))^2-2*sin(x)*cos(x)\n", "Range: hidden sin/cos square identity", "answer: y = 1"),
            ]
            for inp, label, must in range_identity_cases:
                out, _ = self.run_cli("algebraProgram.py", inp)
                text = normalized_text(out)
                self.add_test(label, must in text and "inspect graph" not in text, out, p, inp, "domain/range identity reduction", "algebra_domain_range_identity")

            solve_interval_cases = [
                (
                    "6\nx^2-4=0, x, 0, 3\n",
                    "Mode 6: x^2-4=0 in [0,3] keeps only x=2",
                    ("interval: x in [0, 3]", "answer: x = 2"),
                    ("x = -2",),
                ),
                (
                    "6\n2*t-8=0, t, 0, 2\n",
                    "Mode 6: 2t-8=0 in [0,2] reports no solution",
                    ("interval: t in [0, 2]", "no solution in the interval"),
                    (),
                ),
                (
                    "6\nx^3-3*x^2+2*x=0, x, 0.5, 1.5\n",
                    "Mode 6: cubic in [0.5,1.5] keeps only x=1",
                    ("interval: x in", "answer: x = 1"),
                    ("x = 0", "x = 2"),
                ),
            ]
            for inp, label, must_contain, must_exclude in solve_interval_cases:
                out, _ = self.run_cli("algebraProgram.py", inp)
                text = normalized_text(out)
                ok = all(tok in text for tok in must_contain) and all(tok not in text for tok in must_exclude)
                ok = ok and "err:" not in text
                self.add_test(label, ok, out, p, inp, "solve with interval filter", "algebra_solve_interval")

            algebra_solve_regressions = [
                ("6\nx^3+2*x=50\n", "Regression: depressed cubic solved by Cardano", ("cardano", "answer:", "x =")),
                ("6\n1/x+1/(x+1)=1/2\n", "Regression: reciprocal equation uses common denominator", ("clearing denominators", "answer:", "x =")),
            ]
            for inp, label, tokens in algebra_solve_regressions:
                out, _ = self.run_cli("algebraProgram.py", inp)
                eq = inp.splitlines()[1]
                text = normalized_text(out)
                checker = self.algebra_solve_output_checker(eq)
                passed = (
                    checker(out)
                    and shared_is_exam_format(out)
                    and all(token in text for token in tokens)
                    and not _has_non_exam_quality_output(out)
                )
                self.add_test(label, passed, out, p, inp, "analytical solve with exam-style conclusion", "algebra_solve")

            identity_regressions = [
                (
                    "6\n1/(x+2)+1/(x-2)=2*x/(x^2-4)\n",
                    "Regression: rearranged rational identity does not flood numeric roots",
                    ("infinite solutions", "all real"),
                    ("x = -100", "x = -99.95", "x = ["),
                ),
                (
                    "6\n(3*x-2)^2=9*x^2-12*x+8\n",
                    "Regression: expanded contradiction reports no solution",
                    ("no solution", "answer:"),
                    ("x = -100",),
                ),
            ]
            for inp, label, must_any, must_exclude in identity_regressions:
                out, _ = self.run_cli("algebraProgram.py", inp)
                text = normalized_text(out)
                passed = (
                    any(tok in text for tok in must_any)
                    and all(tok not in text for tok in must_exclude)
                    and shared_is_exam_format(out)
                    and not _has_non_exam_quality_output(out)
                )
                self.add_test(label, passed, out, p, inp, "identity/contradiction guard", "algebra_solve:identity_guard")

        # Inverse tests
        inverses = [
            ("8\n(2*x+1)/(3*x+4)\n", "f^-1", "Inverse: (2x+1)/(3x+4)"),
            ("8\n3*x-7\n", "f^-1", "Inverse: 3x-7"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in inverses:
                out, _ = self.run_cli("algebraProgram.py", inp)
                self.add_test(label, check in out, out, p)

        # Extreme tests
        if difficulty in ("all", "hard"):
            out, _ = self.run_cli("algebraProgram.py", "4\n(x^3-y^3)/((x-y)*(x^2+xy+y^2))\n")
            self.add_test("Extreme: complex fraction", "Error" not in out, out, p)

            out, _ = self.run_cli("algebraProgram.py", "6\nx^5-5*x^4+10*x^3-10*x^2+5*x-1=0\n")
            self.add_test("Extreme: 5th degree polynomial", "x =" in out, out, p)

            out, _ = self.run_cli("algebraProgram.py", "7\n1/(x-1)\nsqrt(x+1)\nx^3+2*x\n")
            self.add_test("Extreme: composition f(g(h(x)))", "Error" not in out, out, p)

        extra_cases = []

        for n in range(2, 12):
            extra_cases.append((
                f"1\n(x+{n})^2\nx^2+{2*n}*x+{n*n}\n",
                f"Generated compare: (x+{n})^2 expansion",
                "Equivalent",
            ))

        for a in range(1, 6):
            for b in range(2, 7):
                extra_cases.append((
                    f"1\n(x+{a})*(x+{b})\nx^2+{a+b}*x+{a*b}\n",
                    f"Generated compare: (x+{a})(x+{b}) expansion",
                    "Equivalent",
                ))

        for n in range(2, 12):
            extra_cases.append((
                f"1\n(x^{n}-1)/(x-1)\n" + "+".join(f"x^{k}" if k > 1 else ("x" if k == 1 else "1") for k in range(n - 1, -1, -1)) + "\n",
                f"Generated compare: geometric factor n={n}",
                "Equivalent",
            ))

        for n in range(1, 11):
            extra_cases.append((
                f"1\nexp(log(x+{n}))\nx+{n}\n",
                f"Generated compare: exp(log(x+{n}))",
                "Equivalent",
            ))

        transform_cases = [
            ("2\nsin(3*x)\n3*sin(x)-4*sin(x)^3\n", "Generated transform: sin(3x)", "sin"),
            ("2\ncos(3*x)\n4*cos(x)^3-3*cos(x)\n", "Generated transform: cos(3x)", "cos"),
            ("2\n1-cos(2*x)\n2*sin(x)^2\n", "Generated transform: 1-cos(2x)", "2*(sin(x))^2"),
            ("2\n1+cos(2*x)\n2*cos(x)^2\n", "Generated transform: 1+cos(2x)", "2*(cos(x))^2"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "Generated transform: sin(2x)", "2*sin"),
            ("2\nsin(x)/cos(x)\ntan(x)\n", "Generated transform: tan form", "tan"),
            ("2\ncos(x)/sin(x)\ncot(x)\n", "Generated transform: cot form", "cot"),
            ("2\n1/cos(x)\nsec(x)\n", "Generated transform: sec form", "sec"),
            ("2\n1/sin(x)\ncosec(x)\n", "Generated transform: cosec form", "cosec"),
        ]
        extra_cases.extend(transform_cases * 3)

        for r1 in range(-5, 5):
            for r2 in range(r1 + 1, 6):
                extra_cases.append((
                    f"6\nx^2-{r1 + r2}*x+{r1*r2}=0\n",
                    f"Generated solve: roots {r1}, {r2}",
                    "x =",
                ))
                if len(extra_cases) >= 120:
                    break
            if len(extra_cases) >= 120:
                break

        for a in range(2, 12):
            extra_cases.append((
                f"8\n{a}*x-{a+3}\n",
                f"Generated inverse: {a}x-{a+3}",
                "f^-1",
            ))

        for a in range(1, 7):
            for b in range(2, 8):
                extra_cases.append((
                    f"8\n({a}*x+{b})/({b+1}*x+{a+2})\n",
                    f"Generated inverse: ({a}x+{b})/({b+1}x+{a+2})",
                    "f^-1",
                ))

        complex_cases = [
            ("1\nexp(log((x+2)^5))\n(x+2)^5\n", "Complex compare: exp(log((x+2)^5))", "Equivalent"),
            ("1\n((x^2-1)/(x-1))^3\n(x+1)^3\n", "Complex compare: ((x^2-1)/(x-1))^3", "Equivalent"),
            ("1\n(sec(2*x)^2-tan(2*x)^2)*(x+3)^4\n(x+3)^4\n", "Complex compare: trig identity times quartic", "Equivalent"),
            ("1\nexp(log(x^4+3*x^2+1))\nx^4+3*x^2+1\n", "Complex compare: exp(log(quartic))", "Equivalent"),
            ("2\nsin(4*x)/(2*cos(2*x))\nsin(2*x)\n", "Complex transform: sin(4x)/(2cos(2x))", "sin"),
            ("2\n(1-cos(4*x))/2\nsin(2*x)^2\n", "Complex transform: half-angle shifted", "sin"),
            ("6\n(x^2-5*x+6)/(x-3)=0\n", "Complex solve: rational linearised", "x ="),
            ("6\nx^4-5*x^2+4=0\n", "Complex solve: biquadratic", "x ="),
            ("8\n(3*x-5)/(2*x+7)\n", "Complex inverse: fractional linear", "f^-1"),
            ("8\n5*(x-2)^3+1\n", "Complex inverse: shifted cubic", "f^-1"),
            ("1\nlog(exp((x+1)^2))\n(x+1)^2\n", "Complex compare: log(exp((x+1)^2))", "Equivalent"),
            ("1\n((x^3-1)/(x-1))-(x^2+x+1)\n0\n", "Complex compare: cubic factor identity", "Equivalent"),
        ]
        extra_cases.extend(complex_cases)

        extreme_cases = []
        rng = random.Random(20260410)

        for a in range(2, 7):
            for b in range(1, 5):
                extreme_cases.append((
                    f"1\nexp(log(({a}*x^2+1)^3*(x-{b})^2))\n({a}*x^2+1)^3*(x-{b})^2\n",
                    f"Extreme compare: exp-log product a={a}, b={b}",
                    algebra_compare_checker(),
                ))
                extreme_cases.append((
                    f"1\n((x^2-{b}^2)/(x-{b}))^2\n(x+{b})^2\n",
                    f"Extreme compare: removable rational square b={b}",
                    algebra_compare_checker(),
                ))

        for a in range(2, 6):
            for b in range(1, 4):
                extreme_cases.append((
                    f"2\n1-cos(2*({a}*x-{b}))\n2*sin({a}*x-{b})^2\n",
                    f"Extreme transform: 1-cos(2({a}x-{b}))",
                    algebra_transform_checker("sin"),
                ))
                extreme_cases.append((
                    f"2\nsin(2*({a}*x+{b}))\n2*sin({a}*x+{b})*cos({a}*x+{b})\n",
                    f"Extreme transform: sin(2({a}x+{b}))",
                    algebra_transform_checker("sin"),
                ))

        quartic_pairs = [(3, 1), (4, 2), (5, 1), (5, 3), (6, 2), (7, 1), (7, 5), (8, 4), (9, 3), (10, 2)]
        for m, n in quartic_pairs:
            avg = (m * m + n * n) // 2
            gap = (m * m - n * n) // 2
            extreme_cases.append((
                f"6\n(x^2-{avg})^2-{gap * gap}=0\n",
                f"Extreme solve: disguised quartic roots ±{m}, ±{n}",
                algebra_solve_checker(str(m), str(-m), str(n), str(-n)),
            ))

        inverse_specs = [
            ("sqrt({a}*x+{b})", algebra_inverse_checker("f^-1")),
            ("1/sqrt({a}*x+{b})", algebra_inverse_checker("f^-1")),
            ("log(({a}*x+{b})^3)", algebra_inverse_checker("f^-1")),
            ("{base}^({a}*x+{b})", algebra_inverse_checker("f^-1")),
            ("({a}*x+{b})/({c}*x+{d})", algebra_inverse_checker("f^-1")),
            ("({a}*x+{b})/({a}*x+{b})", algebra_inverse_checker("no inverse")),
        ]
        inverse_params = [
            {"a": 2, "b": 1, "base": 2, "c": 3, "d": 5},
            {"a": 3, "b": 2, "base": 3, "c": 4, "d": 7},
            {"a": 4, "b": 1, "base": 5, "c": 5, "d": 2},
            {"a": 5, "b": 3, "base": 7, "c": 2, "d": 9},
            {"a": 6, "b": 5, "base": 11, "c": 7, "d": 4},
        ]
        for params in inverse_params:
            for pattern, checker in inverse_specs:
                expr = pattern.format(**params)
                extreme_cases.append((
                    f"8\n{expr}\n",
                    f"Extreme inverse: {expr}",
                    checker,
                ))

        comp_specs = [
            ("1/(x^2+{a})", "({b}*x-{c})^2+1"),
            ("log(x+{a})", "({b}*x-{c})^3"),
            ("sqrt(x+{a})", "({b}*x+{c})^2"),
            ("({a}*x+1)/({b}*x+3)", "x^2+{c}"),
        ]
        for a, b, c in [(2, 3, 1), (3, 2, 5), (4, 5, 2), (5, 4, 3), (6, 3, 7)]:
            for f_pattern, g_pattern in comp_specs:
                f_expr = f_pattern.format(a=a, b=b, c=c)
                g_expr = g_pattern.format(a=a, b=b, c=c)
                extreme_cases.append((
                    f"7\n{f_expr}\n{g_expr}\n",
                    f"Extreme composition: f={f_expr}, g={g_expr}",
                    algebra_comp_checker(),
                ))

        rewrite_specs = [
            ("(x+1)^6", ["(x+1)^3"]),
            ("(2*x-1)^8", ["(2*x-1)^2"]),
            ("sin(x)^4+cos(x)^4", ["sin(x)^2", "cos(x)^2"]),
            ("(x^2+1)^6", ["(x^2+1)^3"]),
            ("(-5*x+6)^2+(-5*x+6)+1", ["-5*x+6"]),
        ]
        for expr, terms in rewrite_specs:
            term_block = "".join(f"{term}\n" for term in terms)
            extreme_cases.append((
                f"9\n{expr}\n{term_block}\n",
                f"Extreme rewrite: {expr}",
                algebra_rewrite_checker(),
            ))

        rng.shuffle(extreme_cases)

        if difficulty in ("all", "hard"):
            self.run_simple_cases("algebraProgram.py", p, extra_cases[:100], "contains")
            self.run_simple_cases("algebraProgram.py", p, extreme_cases[:100], "contains")

    def run_trig(self, difficulty="all"):
        p = "Trigonometry"

        # Core identity proofs
        proofs = [
            (trig_prove_cli("sin(x)^2+cos(x)^2", "1"), "LHS = RHS", "Prove: sin²x+cos²x=1"),
            (trig_prove_cli("sec(x)^2-tan(x)^2", "1"), "LHS = RHS", "Prove: sec²x-tan²x=1"),
            (trig_prove_cli("cosec(x)^2-cot(x)^2", "1"), "LHS = RHS", "Prove: csc²x-cot²x=1"),
            (trig_prove_cli("sin(2*x)", "2*sin(x)*cos(x)"), "LHS = RHS", "Prove: sin(2x)=2sinxcosx"),
            (trig_prove_cli("cos(2*x)", "cos(x)^2-sin(x)^2"), "LHS = RHS", "Prove: cos(2x)=cos²x-sin²x"),
            (trig_prove_cli("1-cos(2*x)", "2*sin(x)^2"), "LHS = RHS", "Prove: 1-cos(2x)=2sin²x"),
            (trig_prove_cli("1+cos(2*x)", "2*cos(x)^2"), "LHS = RHS", "Prove: 1+cos(2x)=2cos²x"),
            (trig_prove_cli("tan(x)", "sin(x)/cos(x)"), "LHS = RHS", "Prove: tanx=sinx/cosx"),
            (trig_prove_cli("cot(x)", "cos(x)/sin(x)"), "LHS = RHS", "Prove: cotx=cosx/sinx"),
            (trig_prove_cli("sec(x)^4-tan(x)^4", "1+2*tan(x)^2"), "LHS = RHS", "Prove: sec⁴x-tan⁴x=1+2tan²x"),
            (trig_prove_cli("sin(3*x)", "3*sin(x)-4*sin(x)^3"), "LHS = RHS", "Prove: sin(3x)=3sinx-4sin³x"),
            (trig_prove_cli("cos(3*x)", "4*cos(x)^3-3*cos(x)"), "LHS = RHS", "Prove: cos(3x)=4cos³x-3cosx"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in proofs:
                out, _ = self.run_cli("trigProgram.py", inp)
                lines = inp.splitlines()
                eq = lines[1] + "=" + lines[2]
                checker = trig_identity_output_checker(eq)
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated trig identity"), "trig_prove")
            inp = trig_prove_cli("sin²(x)+cos²(x)", "1")
            out, _ = self.run_cli("trigProgram.py", inp)
            checker = trig_identity_output_checker("sin(x)^2+cos(x)^2=1")
            self.add_test(
                "Abnormal input: unicode trig square identity",
                checker(out),
                out,
                p,
                inp,
                getattr(checker, "__name__", "calculated trig identity"),
                "trig_prove:abnormal",
            )

        # Transform tests
        transforms = [
            ("2\nsin(x)/cos(x)\ntan(x)\n", "tan", "Transform: sinx/cosx → tanx"),
            ("2\n1/cos(x)\nsec(x)\n", "sec", "Transform: 1/cosx → secx"),
            ("2\n1/sin(x)\ncosec(x)\n", "cosec", "Transform: 1/sinx → cscx"),
            ("2\n1-cos(2*x)\n2*sin(x)^2\n", "2*sin", "Transform: 1-cos(2x) → 2sin²x"),
            ("2\n1+cos(2*x)\n2*cos(x)^2\n", "2*cos", "Transform: 1+cos(2x) → 2cos²x"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "2*sin", "Transform: sin(2x) → 2sinxcosx"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, check, label in transforms:
                out, _ = self.run_cli("trigProgram.py", inp)
                _mode, src, tgt = inp.splitlines()[:3]
                checker = transform_output_checker(src, tgt, "Trigonometry")
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated trig transform"), "trig_transform")

        trig_regressions = [
            (
                "2\n1-cos(2*(4*(4*(x)+6)))\n2*sin(4*(4*(x)+6))^2\n",
                "Regression: nested 1-cos(2A) transform",
                trig_transform_checker("sin"),
            ),
        ]
        if difficulty in ("all", "hard"):
            self.run_simple_cases("trigProgram.py", p, trig_regressions, "contains")

        # Solve equations
        solves = [
            ("3\nsin(x)=0,x,0,360\n", "x =", "Solve: sinx=0, 0-360°"),
            ("3\ncos(x)=0,x,0,360\n", "x =", "Solve: cosx=0, 0-360°"),
            ("3\ntan(x)=1,x,0,360\n", "x =", "Solve: tanx=1, 0-360°"),
            ("3\nsec(x)=2,x,0,360\n", "x =", "Solve: secx=2, 0-360°"),
            ("3\ncosec(x)=2,x,0,360\n", "x =", "Solve: cscx=2, 0-360°"),
            ("3\ncot(x)=1,x,0,360\n", "x =", "Solve: cotx=1, 0-360°"),
            ("3\nsin(x)=1/2,x,0,360\n", "x =", "Solve: sinx=½, 0-360°"),
            ("3\ncos(x)=1/2,x,0,360\n", "x =", "Solve: cosx=½, 0-360°"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in solves:
                out, _ = self.run_cli("trigProgram.py", inp)
                solve_text = inp.splitlines()[1]
                checker = self.trig_cli_solve_checker(solve_text)
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated trig solve"), "trig_solve")

            mixed_angle_cases = [
                ("3\nsin(x+pi/6)=1/2,x,0,360\n", "Regression: mixed radian angle with degree interval"),
                ("3\nsin(x+30)=1/2,x,0,2*pi\n", "Regression: mixed degree angle with radian interval"),
                ("3\ntan(x)^2=1,x,0,360\n", "Regression: squared tangent solve"),
                ("3\nsin(2*x)=cos(x),x,0,360\n", "Regression: double-angle solve"),
                ("3\nsin(3*x)-sin(x)=0,x,0,2*pi\n", "Regression: same-function sine difference solve"),
                ("3\ncos(3*x)-cos(x)=0,x,0,2*pi\n", "Regression: same-function cosine difference solve"),
            ]
            for inp, label in mixed_angle_cases:
                out, _ = self.run_cli("trigProgram.py", inp)
                text = normalized_text(out)
                passed = (
                    "error:" not in text
                    and "x =" in text
                    and shared_is_exam_format(out)
                    and not _has_non_exam_quality_output(out)
                )
                self.add_test(label, passed, out, p, inp, "trig solve output with method, working, and answer", "trig_solve")

        if difficulty in ("all", "easy", "medium", "hard"):
            inp = "3\n4*pi**2=72-(72*cos(x)),x,0,(2*pi)\n"
            out, _ = self.run_cli("trigProgram.py", inp)
            base = self.trig_cli_solve_checker(inp.splitlines()[1])
            checker = readable_output_checker(base)
            self.add_test(
                "Formatting: trig solve keeps exact grouped answers readable",
                checker(out) and "1 - pi^2/18" in out and "(3*pi)/2" in out,
                out,
                p,
                inp,
                "readable exact-form trig solve with grouped constants",
                "trig_formatting",
            )

        # Regression: non-identities should fail cleanly in prove mode
        if difficulty in ("all", "medium", "hard"):
            out, _ = self.run_cli("trigProgram.py", trig_prove_cli("2*sin(x-30)", "5*cos(x)"))
            self.add_test("Reject non-identity cleanly: 2sin(x-30)=5cos(x)", "not an identity" in out.lower(), out, p)

        # NEW: Additional proofs
            out, _ = self.run_cli("trigProgram.py", trig_prove_cli("tan(x)+cot(x)", "2*cosec(2*x)"))
            self.add_test("Prove: tanx+cotx=2csc(2x)", "LHS = RHS" in out, out, p)

        # NEW: Solve more complex equations
            out, _ = self.run_cli("trigProgram.py", "3\nsin(x)+sin(2*x)+sin(3*x)=0,x,0,6.283\n")
            self.add_test("Solve: sinx+sin2x+sin3x=0", "x =" in out, out, p)

            out, _ = self.run_cli("trigProgram.py", "3\nsin(x)*cos(x)=1/2,x,0,360\n")
            self.add_test("Solve: sinx·cosx=½, 0-360°", "x =" in out or "45" in out, out, p)

        # Extreme tests
        if difficulty in ("all", "hard"):
            out, _ = self.run_cli("trigProgram.py", trig_prove_cli("sin(6*x)", "6*sin(x)*cos(x)^5-20*sin(x)^3*cos(x)^3+16*sin(x)^5*cos(x)"))
            self.add_test("Extreme: sin(6x) identity", "LHS = RHS" in out or "RHS" in out, out, p)

        extra_cases = []

        proof_templates = [
            (trig_prove_cli("sin(4*x)", "2*sin(2*x)*cos(2*x)"), "Generated proof: sin(4x)", "LHS = RHS"),
            (trig_prove_cli("cos(4*x)", "cos(2*x)^2-sin(2*x)^2"), "Generated proof: cos(4x)", "LHS = RHS"),
            (trig_prove_cli("1-cos(4*x)", "2*sin(2*x)^2"), "Generated proof: 1-cos(4x)", "LHS = RHS"),
            (trig_prove_cli("1+cos(4*x)", "2*cos(2*x)^2"), "Generated proof: 1+cos(4x)", "LHS = RHS"),
            (trig_prove_cli("sin(6*x)", "2*sin(3*x)*cos(3*x)"), "Generated proof: sin(6x)", "LHS = RHS"),
            (trig_prove_cli("cos(6*x)", "cos(3*x)^2-sin(3*x)^2"), "Generated proof: cos(6x)", "LHS = RHS"),
            (trig_prove_cli("tan(2*x)", "sin(2*x)/cos(2*x)"), "Generated proof: tan(2x)", "LHS = RHS"),
            (trig_prove_cli("sec(2*x)^2-tan(2*x)^2", "1"), "Generated proof: sec²(2x)-tan²(2x)", "LHS = RHS"),
            (trig_prove_cli("cosec(2*x)^2-cot(2*x)^2", "1"), "Generated proof: csc²(2x)-cot²(2x)", "LHS = RHS"),
            (trig_prove_cli("sin(3*x)", "3*sin(x)-4*sin(x)^3"), "Generated proof: sin(3x) repeat", "LHS = RHS"),
            (trig_prove_cli("cos(3*x)", "4*cos(x)^3-3*cos(x)"), "Generated proof: cos(3x) repeat", "LHS = RHS"),
            (trig_prove_cli("tan(x)+cot(x)", "2*cosec(2*x)"), "Generated proof: tan+cot", "LHS = RHS"),
        ]
        extra_cases.extend(proof_templates * 4)

        transform_templates = [
            ("2\nsin(3*x)\n3*sin(x)-4*sin(x)^3\n", "Generated transform: sin(3x)", "sin"),
            ("2\ncos(3*x)\n4*cos(x)^3-3*cos(x)\n", "Generated transform: cos(3x)", "cos"),
            ("2\nsin(2*x)\n2*sin(x)*cos(x)\n", "Generated transform: sin(2x)", "2*sin"),
            ("2\n1-cos(4*x)\n2*sin(2*x)^2\n", "Generated transform: 1-cos(4x)", "sin"),
            ("2\n1+cos(4*x)\n2*cos(2*x)^2\n", "Generated transform: 1+cos(4x)", "cos"),
        ]
        extra_cases.extend(transform_templates * 4)

        solve_templates = [
            ("3\nsin(x)=sqrt(3)/2,x,0,360\n", "Generated solve: sinx=√3/2", "x ="),
            ("3\nsin(x)=sqrt(2)/2,x,0,360\n", "Generated solve: sinx=√2/2", "x ="),
            ("3\ncos(x)=sqrt(3)/2,x,0,360\n", "Generated solve: cosx=√3/2", "x ="),
            ("3\ncos(x)=-1/2,x,0,360\n", "Generated solve: cosx=-1/2", "x ="),
            ("3\ntan(x)=sqrt(3),x,0,360\n", "Generated solve: tanx=√3", "x ="),
            ("3\ntan(x)=-1,x,0,360\n", "Generated solve: tanx=-1", "x ="),
            ("3\nsec(x)=-2,x,0,360\n", "Generated solve: secx=-2", "x ="),
            ("3\ncosec(x)=-2,x,0,360\n", "Generated solve: cscx=-2", "x ="),
            ("3\ncot(x)=-1,x,0,360\n", "Generated solve: cotx=-1", "x ="),
            ("3\nsin(2*x)=0,x,0,360\n", "Generated solve: sin(2x)=0", "x ="),
        ]
        extra_cases.extend(solve_templates * 4)

        complex_cases = [
            (trig_prove_cli("sin(2*x+1)^2+cos(2*x+1)^2", "1"), "Complex proof: shifted Pythagorean identity", "LHS = RHS"),
            (trig_prove_cli("1-cos(2*(3*x-1))", "2*sin(3*x-1)^2"), "Complex proof: nested double-angle", "LHS = RHS"),
            (trig_prove_cli("tan(3*x-2)", "sin(3*x-2)/cos(3*x-2)"), "Complex proof: shifted tan identity", "LHS = RHS"),
            (trig_prove_cli("sec(3*x+1)^2-tan(3*x+1)^2", "1"), "Complex proof: shifted sec identity", "LHS = RHS"),
            ("2\n1-cos(2*(x+1))\n2*sin(x+1)^2\n", "Complex transform: nested double-angle", "sin"),
            ("2\nsin(2*(2*x-1))\n2*sin(2*x-1)*cos(2*x-1)\n", "Complex transform: nested sin double-angle", "2*sin"),
            ("3\nsin(2*x)=sqrt(3)/2,x,0,360\n", "Complex solve: sin(2x)=√3/2", "x ="),
            ("3\ncos(3*x)=1/2,x,0,360\n", "Complex solve: cos(3x)=1/2", "x ="),
            ("3\ntan(2*x)=-1,x,0,360\n", "Complex solve: tan(2x)=-1", "x ="),
            ("3\nsin(x+0.5)=0,x,0,6.283\n", "Complex solve: shifted radian sine", "x ="),
            (trig_prove_cli("sin(4*x+1)", "2*sin(2*x+0.5)*cos(2*x+0.5)"), "Complex proof: offset doubled angle", "LHS = RHS"),
            (trig_prove_cli("cos(4*x-2)", "cos(2*x-1)^2-sin(2*x-1)^2"), "Complex proof: offset cosine double-angle", "LHS = RHS"),
        ]
        extra_cases.extend(complex_cases)

        extreme_cases = []
        rng = random.Random(20260411)

        proof_args = [
            "x^2+1",
            "2*x+3",
            "3*x-2",
            "4*x+1",
            "2*x^2-3*x+1",
            "3*x^2+2",
        ]
        for arg in proof_args:
            extreme_cases.append((
                trig_prove_cli("sin(2*({}))".format(arg), "2*sin({})*cos({})".format(arg, arg)),
                f"Extreme proof: sin(2({arg}))",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                trig_prove_cli("1-cos(2*({}))".format(arg), "2*sin({})^2".format(arg)),
                f"Extreme proof: 1-cos(2({arg}))",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                trig_prove_cli("1+cos(2*({}))".format(arg), "2*cos({})^2".format(arg)),
                f"Extreme proof: 1+cos(2({arg}))",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                trig_prove_cli("sec({})^2-tan({})^2".format(arg, arg), "1"),
                f"Extreme proof: sec²({arg})-tan²({arg})",
                trig_prove_checker(),
            ))

        transform_args = ["2*x+1", "3*x-2", "4*x+3", "2*x^2+1", "3*x^2-2*x+4"]
        for arg in transform_args:
            extreme_cases.append((
                f"2\n1-cos(2*({arg}))\n2*sin({arg})^2\n",
                f"Extreme transform: 1-cos(2({arg}))",
                trig_transform_checker("sin"),
            ))
            extreme_cases.append((
                f"2\n1+cos(2*({arg}))\n2*cos({arg})^2\n",
                f"Extreme transform: 1+cos(2({arg}))",
                trig_transform_checker("cos"),
            ))
            extreme_cases.append((
                f"2\nsin(2*({arg}))\n2*sin({arg})*cos({arg})\n",
                f"Extreme transform: sin(2({arg}))",
                trig_transform_checker("2*sin"),
            ))

        solve_cases = [
            ("sin(3*x)=sqrt(3)/2,x,0,360", "Extreme solve: sin(3x)=√3/2"),
            ("cos(4*x)=1/2,x,0,360", "Extreme solve: cos(4x)=1/2"),
            ("tan(3*x+30)=1,x,0,360", "Extreme solve: tan(3x+30)=1"),
            ("sin(2*x)=sqrt(2)/2,x,0,360", "Extreme solve: sin(2x)=√2/2"),
            ("cos(3*x-30)=1/2,x,0,360", "Extreme solve: cos(3x-30)=1/2"),
            ("tan(4*x)=sqrt(3),x,0,360", "Extreme solve: tan(4x)=√3"),
        ]
        for eq, label in solve_cases:
            extreme_cases.append((
                f"3\n{eq}\n",
                label,
                trig_solve_checker(),
            ))

        for scale in range(2, 7):
            extreme_cases.append((
                trig_prove_cli("sin(2*({}*x^2+1))".format(scale), "2*sin({}*x^2+1)*cos({}*x^2+1)".format(scale, scale)),
                f"Extreme proof: quadratic-angle sin double scale={scale}",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                trig_prove_cli("1-cos(2*({}*x^2-1))".format(scale), "2*sin({}*x^2-1)^2".format(scale)),
                f"Extreme proof: quadratic-angle cosine scale={scale}",
                trig_prove_checker(),
            ))
            extreme_cases.append((
                f"2\nsin(2*({scale}*x^2-1))\n2*sin({scale}*x^2-1)*cos({scale}*x^2-1)\n",
                f"Extreme transform: quadratic-angle sin scale={scale}",
                trig_transform_checker("2*sin"),
            ))
            extreme_cases.append((
                trig_prove_cli("tan({}*x^2+1)".format(scale), "sin({}*x^2+1)/cos({}*x^2+1)".format(scale, scale)),
                f"Extreme proof: tan({scale}x^2+1)",
                trig_prove_checker(),
            ))

        rewrite_cases = [
            ("4\nsin(2*x)^2+cos(2*x)^2\nsin(2*x)\ncos(2*x)\n\n", "Rewrite: sin(2x)^2+cos(2x)^2", trig_rewrite_checker("= 1")),
            ("4\n1-cos(2*(x+1))\nsin(x+1)^2\ncos(x+1)^2\n\n", "Rewrite: 1-cos(2(x+1))", trig_rewrite_checker("sin(1 + x)^2", "cos(1 + x)^2")),
        ]

        rng.shuffle(extreme_cases)

        if difficulty in ("all", "hard"):
            self.run_simple_cases("trigProgram.py", p, extra_cases[:100], "contains")
            self.run_simple_cases("trigProgram.py", p, extreme_cases[:100], "contains")
            self.run_simple_cases("trigProgram.py", p, rewrite_cases, "contains")

    def run_derive(self, difficulty="all"):
        p = "Derive"

        tests = [
            ("x^7-3*x^3+5*x-8", "d/dx", "d/dx[x⁷-3x³+5x-8]"),
            ("sin(x)*cos(x)", "d/dx", "d/dx[sinx·cosx]"),
            ("exp(3*x-2)", "d/dx", "d/dx[e^(3x-2)]"),
            ("log(x^2+3*x+5)", "d/dx", "d/dx[ln(x²+3x+5)]"),
            ("sqrt(2*x+9)", "d/dx", "d/dx[√(2x+9)]"),
            ("(x^2+1)/(x+3)", "d/dx", "d/dx[(x²+1)/(x+3)]"),
            ("tan(x)", "d/dx", "d/dx[tanx]"),
            ("sec(x)", "d/dx", "d/dx[secx]"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for expr, check, label in tests:
                out, _ = self.run_cli("deriveProgram.py", f"1\n{expr}\n")
                checker = self.derive_output_checker(expr)
                self.add_test(label, checker(out), out, p, f"1\n{expr}\n", getattr(checker, "__name__", "calculated derivative"), "derive_normal")
            inp = "1\nsin²(x)+cos²(x)\n"
            out, _ = self.run_cli("deriveProgram.py", inp)
            self.add_test(
                "Abnormal input: derivative of unicode identity",
                "Answer: dy/dx = 0" in out,
                out,
                p,
                inp,
                "dy/dx = 0",
                "derive_normal:abnormal",
            )

        # Extreme - log differentiation
        if difficulty in ("all", "medium", "hard"):
            out, _ = self.run_cli("deriveProgram.py", "1\nx^x\n")
            checker = self.derive_output_checker("x^x")
            self.add_test("Extreme: d/dx[x^x] (log diff)", checker(out), out, p, "1\nx^x\n", getattr(checker, "__name__", "calculated derivative"), "derive_normal")

            out, _ = self.run_cli("deriveProgram.py", "1\nx^2+y^2+2*x*y=5\n")
            self.add_test("Extreme: implicit diff", "Error" not in out, out, p)

            out, _ = self.run_cli("deriveProgram.py", "1\nsin(x)^x\n")
            checker = self.derive_output_checker("sin(x)^x")
            self.add_test("Extreme: d/dx[sin(x)^x]", checker(out), out, p, "1\nsin(x)^x\n", getattr(checker, "__name__", "calculated derivative"), "derive_normal")

            out, _ = self.run_cli("deriveProgram.py", "1\nsin(exp(x^2))\n")
            checker = self.derive_output_checker("sin(exp(x^2))")
            self.add_test("Extreme: chain rule deep", checker(out), out, p, "1\nsin(exp(x^2))\n", getattr(checker, "__name__", "calculated derivative"), "derive_normal")

        implicit_cases = [
            ("2\nx^2+y^2=25\n", "Implicit: x^2+y^2=25", derive_implicit_checker("-x/y")),
            ("2\nx^2+x*y+y^2=7\n", "Implicit: x^2+xy+y^2=7", derive_implicit_checker("dy/dx")),
            ("2\nsin(x)+cos(y)=1\n", "Implicit: sin(x)+cos(y)=1", derive_implicit_checker("dy/dx")),
            ("2\nx=sec(y)**2+tan(y)\n", "Implicit: sec(y)**2+tan(y)", derive_implicit_checker("dy/dx", "sec(y)")),
        ]

        param_cases = [
            ("3\nt^2+1\nt^3-2*t\n", "Parametric: x=t^2+1, y=t^3-2t", derive_param_checker("(3*t^2-2)/(2*t)")),
            ("3\nsin(t)+t^2\ncos(t)-t\n", "Parametric: x=sin(t)+t^2, y=cos(t)-t", derive_param_checker("(-sin(t)-1)/(cos(t)+2*t)")),
            ("3\nexp(t)+sin(t)\nlog(t)+t^2\n", "Parametric: x=exp(t)+sin(t), y=log(t)+t^2", derive_param_checker("(2*t^2+1)/(t*(exp(t)+cos(t)))")),
        ]

        if difficulty in ("all", "medium", "hard"):
            self.run_simple_cases("deriveProgram.py", p, implicit_cases, "contains")
            self.run_simple_cases("deriveProgram.py", p, param_cases, "contains")

        if difficulty in ("all", "easy", "medium", "hard"):
            inp = "2\n(x/(x+1))+(y/(y+1))=x**2\n"
            out, _ = self.run_cli("deriveProgram.py", inp)
            checker = readable_output_checker(
                derive_implicit_checker("(y + 1)", "(x + 1)")
            )
            self.add_test(
                "Formatting: implicit derivative spaces and brackets compound factors",
                checker(out),
                out,
                p,
                inp,
                "readable implicit derivative with bracketed compound factors",
                "derive_formatting",
            )

        extra_exprs = []

        for a in range(2, 12):
            extra_exprs.append((f"sin(({a}*x+1)^2)", f"Generated derive: sin(({a}x+1)^2)"))
            extra_exprs.append((f"exp(sin({a}*x))", f"Generated derive: exp(sin({a}x))"))
            extra_exprs.append((f"log(({a}*x^2+3*x+5)^2)", f"Generated derive: log(({a}x^2+3x+5)^2)"))
            extra_exprs.append((f"({a}*x^2+1)/(x+{a})", f"Generated derive: ({a}x^2+1)/(x+{a})"))
            extra_exprs.append((f"sqrt({a}*x+{a+3})", f"Generated derive: sqrt({a}x+{a+3})"))
            extra_exprs.append((f"{a}^(x^2+x)", f"Generated derive: {a}^(x^2+x)"))
            extra_exprs.append((f"(sin(x)+cos(x))^{a}", f"Generated derive: (sinx+cosx)^{a}"))
            extra_exprs.append((f"tan(exp(x/{a}))", f"Generated derive: tan(exp(x/{a}))"))
            extra_exprs.append((f"(x^{a}+1)*exp(x)", f"Generated derive: (x^{a}+1)e^x"))
            extra_exprs.append((f"log(sqrt(x^2+{a}))", f"Generated derive: log(sqrt(x^2+{a}))"))

        extra_exprs.extend([
            ("exp(log((x^2+1)^5))", "Complex derive: exp(log((x^2+1)^5))"),
            ("sin((exp(x)+log(x))^3)", "Complex derive: sin((exp(x)+log(x))^3)"),
            ("(exp(2*x)+sin(3*x))^5", "Complex derive: (exp(2x)+sin(3x))^5"),
            ("log((sin(2*x)^2+cos(2*x)^2)^3)", "Complex derive: log(identity^3)"),
            ("(x^3+1)^4/(exp(x)+1)", "Complex derive: quotient with quartic-over-exp"),
            ("sqrt(exp(x^2)+log(x)^2)", "Complex derive: sqrt(exp(x^2)+log(x)^2)"),
            ("tan((x^2+1)/(x+1))", "Complex derive: tan(rational)"),
            ("3^(sin(x^2)+exp(x))", "Complex derive: 3^(sin(x^2)+exp(x))"),
            ("(sin(4*x-1)+cos(3*x+2))^6", "Complex derive: shifted trig sum power"),
            ("log(exp(sqrt(x^2+5)))", "Complex derive: log(exp(sqrt(x^2+5)))"),
            ("exp(sin(log(x)^2))", "Complex derive: exp(sin(log(x)^2))"),
            ("((x^2+1)/(x^3+2))^5", "Complex derive: rational power"),
            ("(sec(6*(5*((-5*x-9)^2))^2+1))/(((1*x+1)^3)^2+4)", "Complex derive: report quotient sec over sextic"),
            ("((sin(5*((8*x-3)^3)^2))*(atan(-9*x+8)))/((x)^2+4)", "Complex derive: report quotient sin·atan over quadratic"),
        ])

        extreme_exprs = []
        rng = random.Random(20260412)

        for a, b, c in [(2, 1, 3), (3, 2, 4), (4, 1, 5), (5, 3, 2), (6, 5, 7)]:
            extreme_exprs.extend([
                (f"exp(sin(({a}*x^2+{b})^2))", f"Extreme derive: exp(sin(({a}x^2+{b})^2))"),
                (f"(({a}*x^2+1)/(x+{c}))^4", f"Extreme derive: (({a}x^2+1)/(x+{c}))^4"),
                (f"(x^2+{b})^(sin({a}*x))", f"Extreme derive: (x^2+{b})^(sin({a}x))"),
                (f"log((x^2+{b})^(x^3+{c}))", f"Extreme derive: log((x^2+{b})^(x^3+{c}))"),
                (f"((sin({a}*x)+cos({b}*x))/(x^2+{c}))^3", f"Extreme derive: trig quotient cube a={a},b={b},c={c}"),
                (f"(exp({a}*x)+sin({b}*x))^5", f"Extreme derive: (exp({a}x)+sin({b}x))^5"),
                (f"tan(({a}*x^2+{b})/(x+{c}))", f"Extreme derive: tan(rational) a={a},b={b},c={c}"),
                (f"(x^2+1)^(exp(x)+sin({a}*x))", f"Extreme derive: (x^2+1)^(exp(x)+sin({a}x))"),
                (f"(exp(x^2)+log(x)^{a})/(sin({b}*x)+1)", f"Extreme derive: mixed quotient a={a},b={b}"),
                (f"sqrt(exp({a}*x^2)+log(x)^{b}+sin({c}*x)^2)", f"Extreme derive: sqrt mixed a={a},b={b},c={c}"),
            ])

        rng.shuffle(extreme_exprs)

        if difficulty in ("all", "hard"):
            derive_regressions = [
                (
                    "1\n(sec(6*(5*((-5*x-9)^2))^2+1))/(((1*x+1)^3)^2+4)\n",
                    "Regression: quotient sec over sextic",
                    self.derive_output_checker("(sec(6*(5*((-5*x-9)^2))^2+1))/(((1*x+1)^3)^2+4)"),
                ),
                (
                    "1\n((sin(5*((8*x-3)^3)^2))*(atan(-9*x+8)))/((x)^2+4)\n",
                    "Regression: quotient sin·atan over quadratic",
                    self.derive_output_checker("((sin(5*((8*x-3)^3)^2))*(atan(-9*x+8)))/((x)^2+4)"),
                ),
                (
                    "1\n((sin(5*(7*(6*((8*x-1)^2)^2+4)^2+7)^2-2))^3)/(((4*x+9)^2)^2+8)\n",
                    "Regression: report quotient trig power over quartic",
                    self.derive_output_checker("((sin(5*(7*(6*((8*x-1)^2)^2+4)^2+7)^2-2))^3)/(((4*x+9)^2)^2+8)"),
                ),
                (
                    "1\n((exp(((9*x-6)^2)^2+3))*(exp(((5*x+4)^3)^2+2)))/((x)^2+4)\n",
                    "Regression: report quotient huge exp product",
                    self.derive_output_checker("((exp(((9*x-6)^2)^2+3))*(exp(((5*x+4)^3)^2+2)))/((x)^2+4)"),
                ),
            ]
            self.run_simple_cases("deriveProgram.py", p, derive_regressions, "contains")
            hard_cases = []
            for expr, label in extra_exprs[:100]:
                hard_cases.append((f"1\n{expr}\n", label, derive_checker()))
            for expr, label in extreme_exprs[:100]:
                hard_cases.append((f"1\n{expr}\n", label, derive_checker()))
            self.run_simple_cases("deriveProgram.py", p, hard_cases, "contains")

    def run_symbolic_fixture_pack(self, program_name, program_file, feature_prefix):
        fixture_path = _TESTS_DIR / "fixtures" / "symbolic_regressions.json"
        if not fixture_path.exists():
            return
        with fixture_path.open("r", encoding="utf-8") as fh:
            fixtures = json.load(fh)
        for index, fixture in enumerate(fixtures):
            if fixture.get("program") != program_name:
                continue
            input_lines = fixture.get("input_lines") or []
            if not input_lines:
                continue
            inp = "\n".join(input_lines) + "\n"
            out, _ = self.run_cli(program_file, inp)
            text = normalized_text(out)
            expr = input_lines[1] if len(input_lines) > 1 else ""
            if fixture.get("oracle_kind") == "antiderivative" and expr:
                checker = self.integrate_output_checker(expr)
                passed = checker(out)
            else:
                expected = (fixture.get("expected_answer") or "").replace(" + C", "")
                passed = expected == "" or normalized_text(expected) in text
            for token in fixture.get("required_working", []):
                if token.lower() not in text:
                    passed = False
            for token in fixture.get("forbidden_output", []):
                if token.lower() in text:
                    passed = False
            self.add_test(
                "Fixture: " + program_name + " " + str(index + 1),
                passed,
                out,
                program_name,
                inp,
                fixture.get("oracle_kind", "fixture"),
                feature_prefix + ":fixture",
            )

    def run_integrate(self, difficulty="all"):
        p = "Integrate"
        if difficulty in ("all", "easy", "medium", "hard"):
            self.run_symbolic_fixture_pack(p, "intProgram.py", "integrate")

        tests = [
            ("x^7-3*x^2+4", "+ C", "∫x⁷-3x²+4 dx"),
            ("1/x", "log(abs", "∫1/x dx"),
            ("2^x", "ln", "∫2^x dx"),
            ("exp(x)", "+ C", "∫e^x dx"),
            ("sin(x)", "+ C", "∫sinx dx"),
            ("cos(x)", "+ C", "∫cosx dx"),
            ("tan(x)", "+ C", "∫tanx dx"),
            ("sec(x)^2", "+ C", "∫sec²x dx"),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for expr, check, label in tests:
                out, _ = self.run_cli("intProgram.py", f"1\n{expr}\n1\n")
                checker = self.integrate_output_checker(expr)
                self.add_test(label, checker(out), out, p, f"1\n{expr}\n1\n", getattr(checker, "__name__", "calculated antiderivative"), "integrate_auto")

        if difficulty in ("all", "easy", "medium", "hard"):
            inp = "1\n(x+1)^2\n1\n"
            out, _ = self.run_cli("intProgram.py", inp)
            checker = readable_output_checker(self.integrate_output_checker("(x+1)^2"))
            self.add_test(
                "Formatting: integration keeps shifted powers grouped",
                checker(out) and "(x + 1)^3" in out,
                out,
                p,
                inp,
                "readable antiderivative with grouped shifted power",
                "integrate_formatting",
            )

            disguised_solvable_cases = [
                (
                    "1\n((1/(x**2))+(1/(x**3)))*exp(1/x)\n1\n",
                    "Regression: reciprocal exp u-sub auto",
                    "((1/(x**2))+(1/(x**3)))*exp(1/x)",
                    ("-e^(1/x)/x",),
                    "integrate_reciprocal_exp_auto",
                ),
                (
                    "1\n((1/(x**2))+(1/(x**3)))*exp(1/x)\n4\n1/x\n",
                    "Regression: reciprocal exp forced u=1/x",
                    "((1/(x**2))+(1/(x**3)))*exp(1/x)",
                    ("-e^(1/x)/x",),
                    "integrate_reciprocal_exp_forced",
                ),
                (
                    "1\n(x*tan(x))/(tan(x)+sec(x))\n1\n",
                    "Regression: trig conjugate ratio with x factor",
                    "(x*tan(x))/(tan(x)+sec(x))",
                    ("sec(x) - tan(x)", "log(abs(sin(x) + 1))"),
                    "integrate_trig_conjugate_ratio",
                ),
                (
                    "1\n(tan(x)^2+1)/(sec(x)^2)\n1\n",
                    "Regression: Pythagorean trig quotient reduces to constant",
                    "(tan(x)^2+1)/(sec(x)^2)",
                    ("x + c",),
                    "integrate_trig_identity_quotient",
                ),
                (
                    "1\n(1-cos(2*x))/(sin(x))\n1\n",
                    "Regression: double-angle reduction before integration",
                    "(1-cos(2*x))/(sin(x))",
                    ("-2*cos(x)",),
                    "integrate_double_angle_reduction",
                ),
                (
                    "1\n((sin(x)+cos(x))^2-2*sin(x)*cos(x))/(x+1)\n1\n",
                    "Regression: expanded trig square reduces before log rule",
                    "((sin(x)+cos(x))^2-2*sin(x)*cos(x))/(x+1)",
                    ("log(abs(x + 1))",),
                    "integrate_trig_square_reduction",
                ),
            ]
            for inp2, label2, expr2, required2, feature2 in disguised_solvable_cases:
                out, _ = self.run_cli("intProgram.py", inp2)
                checker2 = self.integrate_output_checker(expr2)
                text2 = normalized_text(out)
                passed2 = checker2(out) and "no elementary" not in text2
                for token2 in required2:
                    if token2.lower() not in text2:
                        passed2 = False
                self.add_test(
                    label2,
                    passed2,
                    out,
                    p,
                    inp2,
                    "solvable disguised integral with checked antiderivative",
                    feature2,
                )

            def non_elementary_integral_checker(*tokens):
                def check(output):
                    text = normalized_text(output)
                    return (
                        shared_is_exam_format(output)
                        and "+ c" in text
                        and "out of scope" not in text
                        and "no standard exam-method antiderivative found" not in text
                        and all(token.lower() in text for token in tokens)
                    )

                check.__name__ = "non_elementary_integral_checker"
                return check

            non_elementary_cases = [
                ("1\nexp(x^2)\n1\n", "Regression: ∫e^(x²) uses erfi notation", ("erfi", "non-elementary")),
                ("1\nsin(x)/x\n1\n", "Regression: ∫sin(x)/x uses Si notation", ("si(x)", "series")),
                ("1\n1/log(x)\n1\n", "Regression: ∫1/ln(x) uses logarithmic integral", ("li(x)", "logarithmic integral")),
                ("1\nsin(x^2)\n1\n", "Regression: ∫sin(x²) uses Fresnel notation", ("fresnel", "series")),
                ("1\n1/sqrt(1-x^4)\n1\n", "Regression: elliptic integral fallback is worked", ("elliptic", "series")),
            ]
            for inp, label, tokens in non_elementary_cases:
                out, _ = self.run_cli("intProgram.py", inp)
                checker = non_elementary_integral_checker(*tokens)
                self.add_test(label, checker(out), out, p, inp, "worked non-elementary antiderivative explanation", "integrate_auto")

        # Substitution tests
        subs = [
            ("1\n(3*x^2+1)/(x^3+x+7)\n4\nx^3+x+7\n", "x^3+x+7", "Sub: ∫(3x²+1)/(x³+x+7) dx"),
            ("1\ncos(3*x+1)\n4\n3*x+1\n", "3*x+1", "Sub: ∫cos(3x+1) dx"),
            ("1\n(5*x+1)^4\n4\n5*x+1\n", "5*x+1", "Sub: ∫(5x+1)⁴ dx"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in subs:
                out, _ = self.run_cli("intProgram.py", inp)
                integrand = inp.splitlines()[1]
                checker = self.integrate_output_checker(integrand)
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated antiderivative"), "integrate_sub")

        # By parts tests
        parts = [
            ("1\nx*sin(x)\n5\n", "+ C", "By parts: ∫x·sinx dx"),
            ("1\nx*cos(x)\n5\n", "+ C", "By parts: ∫x·cosx dx"),
            ("1\nx*exp(x)\n5\n", "+ C", "By parts: ∫x·e^x dx"),
            ("1\nlog(x)\n5\n", "+ C", "By parts: ∫lnx dx"),
            ("1\nasin(x)\n5\n", "+ C", "By parts: ∫arcsinx dx"),
        ]

        if difficulty in ("all", "medium", "hard"):
            for inp, check, label in parts:
                out, _ = self.run_cli("intProgram.py", inp)
                integrand = inp.splitlines()[1]
                checker = self.integrate_output_checker(integrand)
                self.add_test(label, checker(out), out, p, inp, getattr(checker, "__name__", "calculated antiderivative"), "integrate_parts")

        # Extreme
        if difficulty in ("all", "hard"):
            out, _ = self.run_cli("intProgram.py", "1\nsin(x)^4*cos(x)^2\n1\n")
            checker = self.integrate_output_checker("sin(x)^4*cos(x)^2")
            self.add_test("Extreme: ∫sin⁴x·cos²x dx", checker(out), out, p, "1\nsin(x)^4*cos(x)^2\n1\n", getattr(checker, "__name__", "calculated antiderivative"), "integrate_auto")

            out, _ = self.run_cli("intProgram.py", "1\n(x^3+2*x^2-x+1)/((x-1)^2*(x+2))\n1\n")
            checker = self.integrate_output_checker("(x^3+2*x^2-x+1)/((x-1)^2*(x+2))")
            self.add_test("Extreme: partial fractions", checker(out), out, p, "1\n(x^3+2*x^2-x+1)/((x-1)^2*(x+2))\n1\n", getattr(checker, "__name__", "calculated antiderivative"), "integrate_auto")

        extra_cases = []

        for a in range(2, 12):
            extra_cases.append((f"1\n({a}*x+1)^3\n1\n", f"Generated integrate: ({a}x+1)^3", "+ C"))
            extra_cases.append((f"1\nexp({a}*x-1)\n1\n", f"Generated integrate: exp({a}x-1)", "+ C"))
            extra_cases.append((f"1\nsin({a}*x+1)\n1\n", f"Generated integrate: sin({a}x+1)", "+ C"))
            extra_cases.append((f"1\ncos({a}*x-1)\n1\n", f"Generated integrate: cos({a}x-1)", "+ C"))
            extra_cases.append((f"1\n1/({a}*x+3)\n1\n", f"Generated integrate: 1/({a}x+3)", "ln"))
            extra_cases.append((f"1\n({2*a}*x+{a})/(x^2+{a}*x+5)\n1\n", f"Generated integrate: derivative-over-function a={a}", "ln"))
            extra_cases.append((f"1\nx*exp({a}*x)\n5\n", f"Generated parts: x*exp({a}x)", "+ C"))
            extra_cases.append((f"1\nx*sin({a}*x)\n5\n", f"Generated parts: x*sin({a}x)", "+ C"))
            extra_cases.append((f"1\nx*cos({a}*x)\n5\n", f"Generated parts: x*cos({a}x)", "+ C"))
            extra_cases.append((f"1\ncos({a}*x+1)\n4\n{a}*x+1\n", f"Generated sub: cos({a}x+1)", "+ C"))
            extra_cases.append((f"1\n({a}*x+1)^4\n4\n{a}*x+1\n", f"Generated sub: ({a}x+1)^4", "+ C"))

        extra_cases.extend([
            ("1\n(exp(2*x)+sin(x))*(2*exp(2*x)+cos(x))/((exp(2*x)+sin(x))^2+1)\n1\n", "Complex integrate: mixed derivative-over-function", "ln"),
            ("1\nx^2*exp(3*x)\n5\n", "Complex parts: x^2*exp(3x)", "+ C"),
            ("1\nx^2*sin(2*x)\n5\n", "Complex parts: x^2*sin(2x)", "+ C"),
            ("1\n(log(x))^2\n5\n", "Complex parts: (log(x))^2", "+ C"),
            ("1\n(exp(x))/(exp(2*x)+1)\n4\nexp(x)\n", "Complex sub: exp(x)/(exp(2x)+1)", "+ C"),
            ("1\n(6*x+2)/(3*x^2+2*x+7)\n1\n", "Complex integrate: derivative-over-quadratic", "ln"),
            ("1\nsin(4*x-3)\n1\n", "Complex integrate: sin(4x-3)", "+ C"),
            ("1\ncos(5*x+2)\n1\n", "Complex integrate: cos(5x+2)", "+ C"),
            ("1\n1/(x*(log(x)))\n1\n", "Complex integrate: 1/(x ln x)", "ln"),
            ("1\n(2*x+1)/(sqrt(x^2+x+5))\n1\n", "Complex integrate: linear over sqrt(quadratic)", "+ C"),
            ("1\n(exp(x)+1)^5*exp(x)\n4\nexp(x)+1\n", "Complex sub: (exp(x)+1)^5 exp(x)", "+ C"),
            ("1\nsin(3*x+1)^5*cos(3*x+1)\n4\nsin(3*x+1)\n", "Complex sub: sin^5(...)cos(...)", "+ C"),
            ("1\n(6*x+4)/(x^2+4*x+12)\n2\n", "Complex integrate: report irreducible linear over quadratic", "+ C"),
        ])
        extreme_cases = []
        rng = random.Random(20260413)

        for a in range(2, 7):
            extreme_cases.extend([
                (f"1\nx^3*exp({a}*x)\n5\n", f"Extreme parts: x^3*exp({a}x)", integrate_checker()),
                (f"1\nx^3*sin({a}*x)\n5\n", f"Extreme parts: x^3*sin({a}x)", integrate_checker()),
                (f"1\nx^3*cos({a}*x)\n5\n", f"Extreme parts: x^3*cos({a}x)", integrate_checker()),
                (f"1\nx^2*exp(x)*sin({a}*x)\n5\n", f"Extreme parts loop: x^2*exp(x)*sin({a}x)", integrate_checker()),
                (f"1\nx^2*exp(x)*cos({a}*x)\n5\n", f"Extreme parts loop: x^2*exp(x)*cos({a}x)", integrate_checker()),
                (f"1\nx^{a}*log(x)\n5\n", f"Extreme parts: x^{a}*log(x)", integrate_checker()),
                (f"1\n({2*a}*x+{a+1})/(x^2+{2*a+1}*x+{a*(a+1)})\n1\n", f"Extreme rational: factorable quadratic a={a}", integrate_checker("ln")),
                (f"1\n({2*a}*x+{a+1})/(x^2+{2*a}*x+{a*a})\n1\n", f"Extreme rational: repeated-root quadratic a={a}", integrate_checker("ln")),
                (f"1\n1/(x^2-{a*a})\n1\n", f"Extreme rational: 1/(x^2-{a*a})", integrate_checker("ln")),
                (f"1\n1/({a*a}-x^2)\n1\n", f"Extreme rational: 1/({a*a}-x^2)", integrate_checker("ln")),
                (f"1\n({2*a}*x+{a+1})/(sqrt(x^2+{a+1}*x+{a*a+3}))\n1\n", f"Extreme sqrt-quadratic a={a}", integrate_checker("sqrt")),
                (f"1\n(exp({a}*x)+1)^4*exp({a}*x)\n4\nexp({a}*x)+1\n", f"Extreme sub: (exp({a}x)+1)^4 exp({a}x)", integrate_checker()),
                (f"1\nsin({a}*x+1)^5*cos({a}*x+1)\n4\nsin({a}*x+1)\n", f"Extreme sub: sin^5({a}x+1)cos({a}x+1)", integrate_checker()),
            ])

        extra_integrals = [
            ("1\nexp(x)*sin(x)\n5\n", "Extreme standard loop: exp(x)*sin(x)", integrate_checker()),
            ("1\nexp(x)*cos(x)\n5\n", "Extreme standard loop: exp(x)*cos(x)", integrate_checker()),
            ("1\nx^2*exp(x)*sin(x)\n5\n", "Extreme mixed loop: x^2*exp(x)*sin(x)", integrate_checker()),
            ("1\nx^2*exp(x)*cos(x)\n5\n", "Extreme mixed loop: x^2*exp(x)*cos(x)", integrate_checker()),
            ("1\n(4*x+5)/(x^2+5*x+6)\n1\n", "Extreme direct: factorable linear/quadratic", integrate_checker("ln")),
            ("1\n(4*x+5)/(x^2+4*x+4)\n1\n", "Extreme direct: repeated-root linear/quadratic", integrate_checker("ln")),
            ("1\n(2*x+3)/(sqrt(x^2+3*x+7))\n1\n", "Extreme direct: derivative over sqrt(quadratic)", integrate_checker("sqrt")),
            ("1\n(2*x+7)/((x+1)*(x^2+7*x+11))\n6\n", "Extreme PF: linear over linear times irreducible quadratic", integrate_checker("partial fractions")),
        ]
        extreme_cases.extend(extra_integrals)
        rng.shuffle(extreme_cases)
        if difficulty in ("all", "hard"):
            integrate_regressions = [
                (
                    "1\n(6*x+4)/(x^2+4*x+12)\n2\n",
                    "Regression: irreducible linear over quadratic",
                    self.integrate_output_checker("(6*x+4)/(x^2+4*x+12)"),
                ),
            ]
            self.run_simple_cases("intProgram.py", p, integrate_regressions, "contains")
            hard_cases = []
            for inp, label, check in extra_cases[:100]:
                token = check.lower()
                checker = integrate_checker("ln") if "ln" in token else integrate_checker()
                hard_cases.append((inp, label, checker))
            hard_cases.extend(extreme_cases[:100])
            self.run_simple_cases("intProgram.py", p, hard_cases, "contains")

    def run_stats(self, difficulty="all"):
        p = "Stats"
        if getattr(self, "backend", "python") != "c":
            self.add_test("Stats backend", True, "Stats module is C++ backend only.", p, feature="stats_backend")
            return

        cases = [
            (
                "1\n-1000000000,-4,-1,0,1,4,1000000000\n",
                "Stats: extreme one-variable summary",
                stats_checker("mean", "sxx", "var(sample)"),
                "one-var stats with extreme balanced values",
                "stats_one_var:extreme",
            ),
            (
                "2\n-1000000,-1,0,1,1000000\n-2000001,-3,-1,1,1999999\n",
                "Stats: extreme exact regression/correlation",
                stats_checker("sxy", "r = 1", "2*x"),
                "linear regression y=-1+2x, r=1",
                "stats_regression:extreme",
            ),
            (
                "3\n10,0.5,3,pmf\n",
                "Stats: binomial pmf",
                stats_checker("x = 3", "p("),
                "P(X=3) for B(10,0.5)",
                "stats_binomial:pmf",
            ),
            (
                "3\n10000,0.01,120,cdf\n",
                "Stats: binomial large-n cdf fallback",
                stats_checker("large n", "normal approx"),
                "large n binomial uses normal approximation",
                "stats_binomial:large_n",
            ),
            (
                "4\n0,1,-3,3\n",
                "Stats: normal central probability",
                stats_checker("standardise", "0.997"),
                "N(0,1) from -3 to 3",
                "stats_normal:central",
            ),
            (
                "4\n7,2000,-(7993),2007\n",
                "Stats: normal parenthesised negative bound",
                stats_checker("standardise", "0.841"),
                "N(7,2000) from -7993 to 2007",
                "stats_normal:negative_parentheses",
            ),
            (
                "5\n105,100,15,36,gt,0.05\n",
                "Stats: one-tailed z-test",
                stats_checker("h0", "reject h0"),
                "right-tail z-test rejects at 5%",
                "stats_ztest:right",
            ),
            (
                "6\n-1000,-10,0,10,1000,1000000,-1000000\n",
                "Stats: sparkline extreme list",
                stats_checker("spark"),
                "compact plot from data",
                "stats_spark:extreme",
            ),
            (
                "7\nx^2-4,-3,3,21\n",
                "Stats: function plot summary",
                stats_checker("spark", "x-intercepts"),
                "plot summary finds intercepts for x^2-4",
                "stats_plot:quadratic",
            ),
        ]

        if difficulty in ("all", "easy", "medium", "hard"):
            for inp, label, checker, info, feature in cases:
                out, _ = self.run_cli("statsProgram.py", inp)
                self.add_test(label, checker(out), out, p, inp, info, feature)

        if difficulty in ("all", "medium", "hard"):
            generated = self.build_random_stats_cases("chaos", 60, random.Random(90210))
            self.run_case_specs(generated, workers=CASE_WORKERS, infinite_mode=False)

    def run_suvat(self, difficulty="all"):
        p = "SUVAT"

        try:
            import SUVATprogram as sp

            def fraction_text(value):
                frac = value if isinstance(value, Fraction) else Fraction(value).limit_denominator()
                if frac.denominator == 1:
                    return str(frac.numerator)
                return f"{frac.numerator}/{frac.denominator}"

            def suvat_num(value):
                frac = value if isinstance(value, Fraction) else Fraction(value).limit_denominator()
                return sp.num(frac.numerator, frac.denominator)

            tests = [
                (None, sp.num(5), None, sp.num(2), sp.num(3), 's', '24', "Find s: u=5, a=2, t=3"),
                (None, sp.num(0), None, sp.num(4), sp.num(5), 's', '50', "Find s: u=0, a=4, t=5"),
                (None, sp.num(-3), None, sp.num(2), sp.num(4), 's', '4', "Find s: u=-3, a=2, t=4"),
                (sp.num(20), None, sp.num(14), sp.num(2), sp.num(4), 'u', '6', "Find u: s=20, v=14, a=2, t=4"),
                (sp.num(15), None, sp.num(9), sp.num(-2), sp.num(3), 'u', '15', "Find u: s=15, v=9, a=-2, t=3"),
                (None, sp.num(7), None, sp.num(3), sp.num(2), 'v', '13', "Find v: u=7, a=3, t=2"),
                (None, sp.num(0), None, sp.num(9), sp.num(2), 'v', '18', "Find v: u=0, a=9, t=2"),
                (sp.num(0), sp.num(-7), sp.num(7), sp.num(7), None, 't', '2', "Find t: prefer v=u+at when u+v=0"),
            ]

            if difficulty in ("all", "easy", "medium", "hard"):
                for s, u, v, a, t, target, expected, label in tests:
                    result, equation, original_eq, sub = sp._build_suvat_solution_data(s, u, v, a, t, target)
                    text = format_suvat_working(result, equation, original_eq, sub)
                    self.add_test(label, suvat_checker(expected)(text), text, p)

                cli_cases = [
                    (",\n20\n0\n0\n5\n", "s", "100", "CLI SUVAT formatting: s=100 must not become 1 m"),
                    (",\n0\n0\n4\n5\n", "s", "50", "CLI SUVAT formatting: s=50"),
                    ("20\n,\n14\n2\n4\n", "u", "6", "CLI SUVAT formatting: recover u=6"),
                    (",\n4\n\n½\n4\n", "s", "20", "Abnormal input: unicode half acceleration"),
                ]
                for inp, target, expected, label in cli_cases:
                    out, _ = self.run_cli("SUVATprogram.py", inp)
                    self.add_test(label, suvat_cli_checker(target, expected)(out), out, p, inp, f"{target} = {expected}", f"suvat_cli_{target}")

                zero_accel_time_input = "40\n14*(2/sqrt(5))\n\n0\n,\n"
                zero_accel_time_output, _ = self.run_cli("SUVATprogram.py", zero_accel_time_input)
                zero_accel_time_ok = (
                    suvat_cli_checker("t", "10*sqrt(5)/7")(zero_accel_time_output)
                    and "t = s/u" in normalized_text(zero_accel_time_output)
                )
                self.add_test(
                    "CLI SUVAT zero acceleration: use t=s/u with surd velocity",
                    zero_accel_time_ok,
                    zero_accel_time_output,
                    p,
                    zero_accel_time_input,
                    "t = 10*sqrt(5)/7 using t = s/u",
                    "suvat_cli_t",
                )

                edge_cli_cases = [
                    (
                        "\n12\n,\n0\n5\n",
                        "v",
                        "12",
                        "CLI SUVAT a=0: velocity remains constant",
                        "v = u + at",
                    ),
                    (
                        ",\n5\n\n2\n0\n",
                        "s",
                        "0",
                        "CLI SUVAT t=0: displacement is zero",
                        "s = ut + 1/2at^2",
                    ),
                    (
                        "\n10\n0\n-2\n,\n",
                        "t",
                        "5",
                        "CLI SUVAT v=0: comes to rest",
                        "t = (v-u)/a",
                    ),
                    (
                        ",\n0\n\n4\n5\n",
                        "s",
                        "50",
                        "CLI SUVAT u=0: starts from rest",
                        "s = ut + 1/2at^2",
                    ),
                ]
                for inp, target, expected, label, method_token in edge_cli_cases:
                    out, _ = self.run_cli("SUVATprogram.py", inp)
                    passed = suvat_cli_checker(target, expected)(out) and method_token.lower() in normalized_text(out)
                    self.add_test(label, passed, out, p, inp, f"{target} = {expected} with {method_token}", f"suvat_cli_{target}")

                def suvat_symbolic_checker(target, *tokens):
                    def check(output):
                        text = normalized_text(output)
                        compact = text.replace(" ", "")
                        if "error:" in text:
                            return False
                        if f"{target}=" not in compact:
                            return False
                        return all(token.replace(" ", "") in compact for token in tokens)
                    return check

                symbolic_cli_cases = [
                    (",\n\n\n\n\n", "s", ("u*t", "a*t^2"), "CLI SUVAT symbolic: find s with no numeric values"),
                    ("\n,\n\n\n\n", "u", ("v-a*t",), "CLI SUVAT symbolic: find u with no numeric values"),
                    ("\n\n,\n\n\n", "v", ("u+a*t",), "CLI SUVAT symbolic: find v with no numeric values"),
                    ("\n\n\n,\n\n", "a", ("(v-u)/t",), "CLI SUVAT symbolic: find a with no numeric values"),
                    ("\n\n\n\n,\n", "t", ("(v-u)/a",), "CLI SUVAT symbolic: find t with no numeric values"),
                    (",\n20\n\n\n5\n", "s", ("25/2*a+100",), "CLI SUVAT symbolic: keep known values and leave missing acceleration"),
                ]
                for inp, target, tokens, label in symbolic_cli_cases:
                    out, _ = self.run_cli("SUVATprogram.py", inp)
                    self.add_test(label, suvat_symbolic_checker(target, *tokens)(out), out, p, inp, f"{target} symbolic answer", f"suvat_cli_symbolic_{target}")

                formatting_input = ",\n\n\n\n\n"
                formatting_output, _ = self.run_cli("SUVATprogram.py", formatting_input)
                self.add_test(
                    "Formatting: SUVAT symbolic equation keeps products explicit",
                    output_readability_ok(formatting_output) and "u*t + 1/2*a*t^2" in formatting_output,
                    formatting_output,
                    p,
                    formatting_input,
                    "readable symbolic SUVAT equation with explicit products",
                    "suvat_formatting",
                )
        except Exception as e:
            self.add_test("SUVAT import", False, str(e), p)

        # Extreme - projectile
        if difficulty in ("all", "medium", "hard"):
            projectile_input = ",\n25\n\n9.8\n4\n"
            out, _ = self.run_cli("SUVATprogram.py", projectile_input)
            self.add_test("Extreme: projectile (u=25, a=9.8, t=4)", suvat_cli_checker("s", "178.4")(out), out, p, projectile_input, "s = 178.4", "suvat_cli_s")

        try:
            import SUVATprogram as sp

            extra_cases = []

            for u_val in range(-4, 6):
                for a_val in range(-3, 4):
                    if a_val == 0:
                        continue
                    t_val = abs(a_val) + 2
                    s_expected = Fraction(u_val * t_val, 1) + Fraction(a_val * t_val * t_val, 2)
                    v_expected = Fraction(u_val + a_val * t_val, 1)
                    extra_cases.append((
                        None, sp.num(u_val), None, sp.num(a_val), sp.num(t_val), 's', fraction_text(s_expected),
                        f"Generated SUVAT: find s from u={u_val}, a={a_val}, t={t_val}"
                    ))
                    extra_cases.append((
                        None, sp.num(u_val), None, sp.num(a_val), sp.num(t_val), 'v', fraction_text(v_expected),
                        f"Generated SUVAT: find v from u={u_val}, a={a_val}, t={t_val}"
                    ))

            for v_val in range(4, 16):
                for a_val in range(1, 6):
                    t_val = a_val + 1
                    u_expected = v_val - a_val * t_val
                    s_val = Fraction((u_expected + v_val) * t_val, 2)
                    extra_cases.append((
                        suvat_num(s_val), None, sp.num(v_val), sp.num(a_val), sp.num(t_val), 'u',
                        fraction_text(Fraction(u_expected, 1)),
                        f"Generated SUVAT: find u from s={s_val}, v={v_val}, a={a_val}, t={t_val}"
                    ))

            extreme_fractional_cases = []
            fractional_us = [Fraction(-5, 2), Fraction(-3, 2), Fraction(1, 2), Fraction(3, 2), Fraction(5, 2)]
            fractional_as = [Fraction(-7, 3), Fraction(-3, 2), Fraction(-1, 2), Fraction(1, 2), Fraction(5, 3), Fraction(7, 2)]
            fractional_ts = [Fraction(3, 2), Fraction(2, 1), Fraction(5, 2), Fraction(7, 3)]

            for u_val in fractional_us:
                for a_val in fractional_as:
                    for t_val in fractional_ts:
                        v_val = u_val + a_val * t_val
                        s_val = u_val * t_val + Fraction(1, 2) * a_val * t_val * t_val
                        extreme_fractional_cases.append((
                            None, suvat_num(u_val), None, suvat_num(a_val), suvat_num(t_val), 's',
                            fraction_text(s_val),
                            f"Extreme SUVAT: find s from u={u_val}, a={a_val}, t={t_val}"
                        ))
                        extreme_fractional_cases.append((
                            None, suvat_num(u_val), None, suvat_num(a_val), suvat_num(t_val), 'v',
                            fraction_text(v_val),
                            f"Extreme SUVAT: find v from u={u_val}, a={a_val}, t={t_val}"
                        ))
                        if a_val != 0:
                            extreme_fractional_cases.append((
                                suvat_num(s_val), None, suvat_num(v_val), suvat_num(a_val), suvat_num(t_val), 'u',
                                fraction_text(u_val),
                                f"Extreme SUVAT: recover u from s={s_val}, v={v_val}, a={a_val}, t={t_val}"
                            ))

            if difficulty in ("all", "hard"):
                for s, u, v, a, t, target, expected, label in extra_cases[:100]:
                    result, equation, original_eq, sub = sp._build_suvat_solution_data(s, u, v, a, t, target)
                    text = format_suvat_working(result, equation, original_eq, sub)
                    self.add_test(label, suvat_checker(expected)(text), text, p)
                for s, u, v, a, t, target, expected, label in extreme_fractional_cases[:100]:
                    result, equation, original_eq, sub = sp._build_suvat_solution_data(s, u, v, a, t, target)
                    text = format_suvat_working(result, equation, original_eq, sub)
                    self.add_test(label, suvat_checker(expected)(text), text, p)
        except Exception as e:
            self.add_test("Generated SUVAT batch", False, str(e), p)


def _cli_help():
    return """CASIO test harness (CLI batch mode when arguments are present).

  python3 run_tests.py tui
      Interactive TUI (type /help, /random, /infinite, etc.)
      tests/reports/failure_report_latest.txt is rewritten at run start; failures append live.

  python3 c++/tools/tests_cpp/run_tests_tui.py random [N]
      Run N chaos random tests (default 100) and print a summary.

  python3 c++/tools/tests_cpp/run_tests_tui.py stress integrate 100
      Run focused adversarial tests for a feature.

  python3 c++/tools/tests_cpp/run_tests_tui.py stress exam_gap 100
      Run worksheet-style full-mark traps: hidden substitutions, PF setup, IBP loops, trig intervals.

  python3 c++/tools/tests_cpp/run_tests_tui.py stress crash 100
      Run calculator-safety probes: long input, nested rewrites, many roots, bounded working output.

  python3 c++/tools/tests_cpp/run_tests_tui.py replay adv-00001
      Replay a stored adversarial failure from the latest run folder.

  python3 c++/tools/tests_cpp/run_tests_tui.py llm <model> ...
      Configure LLM selection (see /llm in TUI). Everything after 'llm' is one string.

  python3 c++/tools/tests_cpp/run_tests_tui.py compile
      Compile C++ host and .g3a targets.

  Common environment variables
      CASIO_TEST_TIMEOUT           Per-case timeout (default 12 seconds)
      CASIO_TEST_WORKERS           Max parallel case workers
      CASIO_LLM_GENERATION_CHANCE  Chance to tag LLM-generated ideas in random mode
      CASIO_TEST_EXPORT_JSON       1/yes/true writes JSON snapshot on each run summary
      CASIO_EXAM_STRESS            1 (default): allocate chaos cases across builders with
                                   jittered weights to stress high-yield / exam-relevant items.
      CASIO_EXAM_BOOST             Optional: builder_name:mult pairs, e.g. random_trig_equation_multi_case:1.5
      CASIO_INFINITE_GENERATION_CHUNK  Cases per program per /infinite cycle (default 12)
      CASIO_TUI_START_COMMANDS      Auto-run TUI commands separated by ;;, e.g. "/llm 2;;/infinite"
      CASIO_LLM_BATCH_SIZE         LLM verify N subprocess results per Ollama call (default 10)
      CASIO_LLM_BATCH_DISABLE      1/true yes: one Ollama call per test (old behaviour)
      CASIO_LLM_BATCH_OUT_CHARS    Max chars of each OUT in batch prompts (shared_llm; default 1200)
      Exits 1 in CLI mode if any test has final status FAIL; otherwise 0.
"""


def main():
    import sys

    if len(sys.argv) > 1:
        first = (sys.argv[1] or "").lower()
        if first in ("-h", "--help", "help"):
            print(_cli_help().rstrip())
            return
        app = CASIOApp()
        app.plain_mode = True
        app._force_export_json = False
        if os.environ.get("CASIO_TEST_EXPORT_JSON", "").lower() in ("1", "true", "yes"):
            app._force_export_json = True

        raw = sys.argv[1:]
        if "--json" in raw:
            app._force_export_json = True
            raw = [a for a in raw if a != "--json"]

        i = 0
        while i < len(raw):
            tok_raw = raw[i] or ""
            tok = tok_raw.lower().lstrip("/")
            if tok in ("--backend", "-b", "backend", "switch", "swtich"):
                if i + 1 < len(raw):
                    app.switch_backend(raw[i + 1])
                    i += 2
                    continue
                i += 1
                continue
            if tok in ("c", "cpp", "c++"):
                app.switch_backend("c")
                i += 1
                continue
            if tok == "python":
                print("Python backend removed; using C++ backend.")
                app.switch_backend("c")
                i += 1
                continue
            maybe_program = app.normalize_program_name(tok)
            if maybe_program is not None:
                app.current_program = maybe_program
                i += 1
                continue
            if tok == "llm":
                if i + 1 < len(raw):
                    app.handle_llm_select("llm " + raw[i + 1])
                    i += 2
                    continue
                app.handle_llm_select("llm")
                i += 1
                continue
            if tok == "run":
                app.action_run_tests(command_label="/run")
                i += 1
                continue
            if tok == "random":
                count = 100
                if i + 1 < len(raw):
                    nxt = (raw[i + 1] or "").lower()
                    if nxt in ("inf", "infinite"):
                        count = None
                        i += 1
                    else:
                        try:
                            count = int(raw[i + 1])
                        except (ValueError, TypeError, IndexError):
                            pass
                        else:
                            i += 1
                if count is None:
                    app.action_random_tests("chaos", None, command_label="/random inf", program=None if app.current_program == "all" else app.current_program)
                else:
                    app.action_random_tests("chaos", count, command_label="/random", program=None if app.current_program == "all" else app.current_program)
                i += 1
                continue
            if tok == "stress":
                feature = "all"
                count = 100
                workers = None
                if i + 1 < len(raw) and not (raw[i + 1] or "").isdigit():
                    feature = raw[i + 1]
                    i += 1
                if i + 1 < len(raw):
                    try:
                        count = int(raw[i + 1])
                    except (ValueError, TypeError):
                        pass
                    else:
                        i += 1
                if i + 1 < len(raw):
                    try:
                        workers = int(raw[i + 1])
                    except (ValueError, TypeError):
                        pass
                    else:
                        i += 1
                app.action_stress_tests(feature, count, workers, command_label="/stress {0}".format(feature))
                i += 1
                continue
            if tok == "replay":
                if i + 1 < len(raw):
                    app.action_replay_case(raw[i + 1])
                    i += 2
                    continue
                i += 1
                continue
            if tok == "shrink":
                if i + 1 < len(raw):
                    app.action_shrink_case(raw[i + 1])
                    i += 2
                    continue
                i += 1
                continue
            if tok in ("inf", "infinite"):
                app.action_random_tests("chaos", None, command_label="/infinite")
                i += 1
                continue
            if tok == "compile":
                app.action_compile()
                i += 1
                continue
            i += 1

        print("Ran {0} tests".format(len(app.records)))
        any_fail = any(r.status == TestStatus.FAIL for r in app.records)
        raise SystemExit(1 if any_fail else 0)

    app = CASIOApp()
    app.run()

if __name__ == "__main__":
    main()
