#!/usr/bin/env python3
import unittest

import run_tests_tui as TUI


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


if __name__ == "__main__":
    unittest.main()
