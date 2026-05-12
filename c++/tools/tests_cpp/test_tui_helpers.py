#!/usr/bin/env python3
import sys
import subprocess
import unittest
import random
from pathlib import Path

import run_tests_tui as TUI


REPO_ROOT = Path(__file__).resolve().parents[3]
HOST = REPO_ROOT / "c++" / "addin" / "host" / "build" / "casio_host"


class TuiHelperTests(unittest.TestCase):
    def test_question_preview_keeps_command_visible_on_one_line(self):
        text = "--int e^(2*x)*cos(3*x),method=parts\nunused"

        preview = TUI.format_question_preview(text, limit=80)

        self.assertEqual(preview, "--int e^(2*x)*cos(3*x),method=parts \\n unused")

    def test_question_preview_truncates_long_questions(self):
        text = "--alg " + "x+" * 80 + "0"

        preview = TUI.format_question_preview(text, limit=40)

        self.assertLessEqual(len(preview), 40)
        self.assertTrue(preview.endswith("..."))

    def test_host_command_fuzzer_keeps_method_option_parseable(self):
        app = TUI.CASIOApp()
        expr = app.fuzz_host_command_expr(
            "exp(2*x)*sin(3*x),method=parts",
            random.Random(4),
            "chaos",
        )

        self.assertIn("method", expr)
        self.assertIn("parts", expr)
        self.assertRegex(expr, r"method\s*=\s*parts")

    def test_cli_llm_status_keyword_is_not_model_name(self):
        out = subprocess.run(
            [sys.executable, str(REPO_ROOT / "c++" / "tools" / "tests_cpp" / "run_tests_tui.py"), "llm", "status"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            timeout=20,
        )

        self.assertEqual(out.returncode, 0, out.stdout + out.stderr)
        self.assertIn("LLM Verification Status", out.stdout)
        self.assertNotIn("Model not found: status", out.stdout)

    def test_llm_generation_ref_not_added_to_label(self):
        app = TUI.CASIOApp()

        class FakeLLM:
            enabled = True

        def feature(_rng, _difficulty, _idx):
            return TUI.CaseSpec(
                "Trig solve: cos(x)=-1",
                "Trigonometry",
                lambda: (True, "x = [180]"),
                "3\ncos(x)=-1,x,0,360\n",
                check_info="x = [180]",
                feature="trig_solve",
            )

        old_chance = TUI.LLM_GENERATION_CHANCE
        app.llm_enabled_for_tests = True
        app.llm_manager = FakeLLM()
        app.generate_llm_case = lambda _program, _difficulty: ("cos(x)=-1", "x = pi/4, 5*pi/4")
        try:
            TUI.LLM_GENERATION_CHANCE = 1.0
            cases = list(app.iter_unique_random_cases([feature], 1, random.Random(7), "medium"))
        finally:
            TUI.LLM_GENERATION_CHANCE = old_chance

        self.assertEqual(len(cases), 1)
        self.assertIn("[LLM]", cases[0].label)
        self.assertNotIn("llm ref", cases[0].label)
        self.assertNotIn("pi/4", cases[0].label)

    @unittest.skipUnless(HOST.exists(), "host binary not built")
    def test_host_accepts_spaced_method_option(self):
        out = subprocess.run(
            [str(HOST), "--int", "exp(2*x)*sin(3*x), method = parts"],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            timeout=12,
        )

        self.assertEqual(out.returncode, 0, out.stdout + out.stderr)
        self.assertIn("I = Int(e^(2*x)*sin(3*x)) dx", out.stdout)
        self.assertIn("+ C", out.stdout)

    @unittest.skipUnless(HOST.exists(), "host binary not built")
    def test_host_accepts_spaced_algebra_wrapper(self):
        out = subprocess.run(
            [str(HOST), "--alg", " solve ( log(2,x^2+4*x+3) = 4 + log(2,x^2+x), x ) "],
            cwd=REPO_ROOT,
            text=True,
            capture_output=True,
            timeout=12,
        )

        self.assertEqual(out.returncode, 0, out.stdout + out.stderr)
        self.assertIn("x = [1/5]", out.stdout)


if __name__ == "__main__":
    unittest.main()
