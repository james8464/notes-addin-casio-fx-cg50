#!/usr/bin/env python3
"""Meta tests for the CASIO test runner (no calculator subprocess)."""

import sys
import unittest
from pathlib import Path

_TESTS = Path(__file__).resolve().parent
if str(_TESTS) not in sys.path:
    sys.path.insert(0, str(_TESTS))

from run_tests import CASIOApp, TestStatus, TestRecord  # noqa: E402


class RuntimeSourceGuardTests(unittest.TestCase):
    def test_calculator_runtime_does_not_import_sympy(self):
        root = Path(__file__).resolve().parents[1]
        bad = []
        for path in (root / "src").rglob("*.py"):
            if "legacy" in path.parts:
                continue
            text = path.read_text(encoding="utf-8", errors="replace").lower()
            if "import sympy" in text or "from sympy" in text:
                bad.append(str(path.relative_to(root)))
        self.assertEqual(bad, [])


class HarnessLlmStatusTests(unittest.TestCase):
    def test_apply_needs_review_marks_review_only(self):
        app = CASIOApp()
        app._feature_stats = {"trig_solve": [1, 1]}
        r = TestRecord(
            1, "a", TestStatus.PASS, "o", "Trigonometry", feature="trig_solve", passed=True, harness_status=TestStatus.PASS
        )
        app.apply_llm_weighted_status(r, "NEEDS_REVIEW")
        self.assertEqual(r.status, TestStatus.PASS)
        self.assertIs(r.passed, True)
        self.assertEqual(r.harness_status, TestStatus.PASS)
        self.assertIs(r.review_needed, True)
        self.assertEqual(app._feature_stats["trig_solve"], [1, 1])

    def test_apply_correct_does_not_save_code_fail(self):
        app = CASIOApp()
        app._feature_stats = {"a": [0, 1]}
        r = TestRecord(
            1, "a", TestStatus.FAIL, "o", "Algebra", feature="a", passed=False, harness_status=TestStatus.FAIL
        )
        app.apply_llm_weighted_status(r, "CORRECT")
        self.assertEqual(r.status, TestStatus.FAIL)
        self.assertIs(r.passed, False)
        self.assertEqual(app._feature_stats["a"], [0, 1])
        self.assertIs(r.review_needed, False)

    def test_failure_report_ids_include_review_needed(self):
        app = CASIOApp()
        app.records = [
            TestRecord(1, "ok", TestStatus.PASS, "o", "A", passed=True, harness_status=TestStatus.PASS),
            TestRecord(2, "review", TestStatus.PASS, "o", "A", passed=True, harness_status=TestStatus.PASS, review_needed=True),
            TestRecord(
                3, "bad", TestStatus.FAIL, "e", "A", passed=False, llm_verdict="INCORRECT", harness_status=TestStatus.FAIL
            ),
        ]
        self.assertEqual(app._failure_report_record_ids(), [2, 3])


if __name__ == "__main__":
    unittest.main()
