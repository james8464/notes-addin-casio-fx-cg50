#!/usr/bin/env python3
"""Focused regressions for trig/algebra transform and prove flows."""

import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PY = sys.executable
MATH_ROOT = ROOT / "src" / "Math"


def run_cli(script_name, cli_input):
    result = subprocess.run(
        [PY, str(MATH_ROOT / script_name)],
        input=cli_input,
        text=True,
        capture_output=True,
        timeout=20,
    )
    return result.stdout


class TransformRegressionTests(unittest.TestCase):
    def test_trig_transform_handles_obtuse_branch(self):
        output = run_cli("trigProgram.py", "2\nsin(x)=12/13\ncot(2*x)=119/120\n")
        self.assertNotIn("Err:", output)
        self.assertIn("obtuse branch", output.lower())
        self.assertIn("tan(x) = -12/5", output)
        self.assertIn("Answer: cot(2*x)=119/120", output)

    def test_trig_transform_handles_acute_branch(self):
        output = run_cli("trigProgram.py", "2\nsin(x)=12/13\ncot(2*x)=-119/120\n")
        self.assertNotIn("Err:", output)
        self.assertIn("acute branch", output.lower())
        self.assertIn("Answer: cot(2*x)=-119/120", output)

    def test_trig_prove_uses_two_inputs(self):
        output = run_cli("trigProgram.py", "1\ntan(x)\nsin(x)/cos(x)\n1\n")
        self.assertNotIn("Err:", output)
        self.assertIn("LHS = RHS", output)

    def test_algebra_transform_accepts_equation_inputs(self):
        output = run_cli("algebraProgram.py", "2\nx^2-1=0\n(x-1)*(x+1)=0\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Method: Transform equation to target form", output)
        self.assertIn("Answer:(x-1)*(x+1)=0", output.replace(" ", ""))

    def test_algebra_transform_delegates_trig_equations(self):
        output = run_cli("algebraProgram.py", "2\nsin(x)=12/13\ncot(2*x)=119/120\n")
        self.assertNotIn("Err:", output)
        self.assertIn("obtuse branch", output.lower())
        self.assertIn("Answer: cot(2*x)=119/120", output)


if __name__ == "__main__":
    unittest.main()
