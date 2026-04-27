#!/usr/bin/env python3
"""Focused regressions for trig/algebra transform and prove flows."""

import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PY = sys.executable
MATH_ROOT = ROOT / "src" / "Math"
if str(MATH_ROOT) not in sys.path:
    sys.path.insert(0, str(MATH_ROOT))
sys._trig_no_autorun = True
sys._algebra_no_autorun = True
import trigProgram as trig_program
import algebraProgram as algebra_program


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

    def test_trig_prove_tan_add_uses_sum_formula(self):
        output = run_cli("trigProgram.py", "1\ntan(x+y)\n(tan(x)+tan(y))/(1-tan(x)*tan(y))\n1\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Use tan(A+B)", output)
        self.assertNotIn("Use tan(2A)", output)

    def test_trig_prove_exact_pi_value(self):
        output = run_cli("trigProgram.py", "1\nsin(pi/6)\n1/2\n1\n")
        self.assertNotIn("Err:", output)
        self.assertIn("sin(pi/6) = 1/2", output)
        self.assertIn("LHS = RHS", output)

    def test_trig_prove_inverse_trig_domain_message(self):
        output = run_cli("trigProgram.py", "1\nasin(sin(x))\nx\n1\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Principal value restriction", output)
        self.assertIn("not an identity for all values", output)

    def test_trig_solve_accepts_plain_equation(self):
        output = run_cli("trigProgram.py", "3\nsin(x)=0\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Answer:", output)
        self.assertTrue("x = [" in output or "n any integer" in output, msg=output)

    def test_pipe_absolute_value_normalization(self):
        trig_lhs, trig_rhs = trig_program.parse_equation_or_zero("|x|=1")
        alg_lhs, alg_rhs = algebra_program.parse_equation_or_zero("|x|=1")
        self.assertEqual(trig_program.show(trig_lhs), "abs(x)")
        self.assertEqual(trig_program.show(trig_rhs), "1")
        self.assertEqual(algebra_program.show(alg_lhs), "abs(x)")
        self.assertEqual(algebra_program.show(alg_rhs), "1")

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
