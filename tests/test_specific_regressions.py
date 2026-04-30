#!/usr/bin/env python3
"""Targeted regressions for the CASIO harness and math program flows."""

import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TESTS = Path(__file__).resolve().parent
PY = sys.executable
MATH_ROOT = ROOT / "src" / "Math"

for path in (TESTS, ROOT, MATH_ROOT):
    path_text = str(path)
    if path_text not in sys.path:
        sys.path.insert(0, path_text)

from run_tests import CASIOApp, TestStatus, TestRecord  # noqa: E402
import algebraProgram as algebra_program  # noqa: E402
import casio_core  # noqa: E402
import trigProgram as trig_program  # noqa: E402
from src.shared_llm import LLMManager  # noqa: E402
from src.shared_cache import cache_store  # noqa: E402


def run_cli(script_name, cli_input):
    result = subprocess.run(
        [PY, str(MATH_ROOT / script_name)],
        input=cli_input,
        text=True,
        capture_output=True,
        timeout=20,
    )
    return result.stdout


class RuntimeSourceGuardTests(unittest.TestCase):
    def test_calculator_runtime_does_not_import_sympy(self):
        bad = []
        for path in (ROOT / "src").rglob("*.py"):
            if "legacy" in path.parts:
                continue
            text = path.read_text(encoding="utf-8", errors="replace").lower()
            if "import sympy" in text or "from sympy" in text:
                bad.append(str(path.relative_to(ROOT)))
        self.assertEqual(bad, [])


class HarnessLlmStatusTests(unittest.TestCase):
    def test_apply_needs_review_marks_review_only(self):
        app = CASIOApp()
        app._feature_stats = {"trig_solve": [1, 1]}
        record = TestRecord(
            1, "a", TestStatus.PASS, "o", "Trigonometry", feature="trig_solve", passed=True, harness_status=TestStatus.PASS
        )
        app.apply_llm_weighted_status(record, "NEEDS_REVIEW")
        self.assertEqual(record.status, TestStatus.PASS)
        self.assertIs(record.passed, True)
        self.assertEqual(record.harness_status, TestStatus.PASS)
        self.assertIs(record.review_needed, True)


class SharedCoreRegressionTests(unittest.TestCase):
    def test_core_subtraction_is_not_implicit_multiplication(self):
        parsed = casio_core.parse_expr("3*x-3*y")
        self.assertEqual(casio_core.format_expr(parsed), "3*x - 3*y")

    def test_core_scaled_pythagorean_reduction(self):
        parsed = casio_core.parse_expr("3*sec(x)^2-3*tan(x)^2")
        reduced = casio_core.trig_reduce(parsed)
        self.assertEqual(casio_core.format_expr(reduced), "3")

    def test_algebra_cardano_uses_real_cube_root_branch(self):
        roots = algebra_program.depressed_cubic_cardano_roots([
            algebra_program.num(2),
            algebra_program.num(3),
            algebra_program.num(0),
            algebra_program.num(1),
        ])
        self.assertIsNotNone(roots)
        self.assertNotIn("(-", algebra_program.show(roots[0]))

    def test_apply_correct_does_not_save_code_fail(self):
        app = CASIOApp()
        app._feature_stats = {"a": [0, 1]}
        record = TestRecord(
            1, "a", TestStatus.FAIL, "o", "Algebra", feature="a", passed=False, harness_status=TestStatus.FAIL
        )
        app.apply_llm_weighted_status(record, "CORRECT")
        self.assertEqual(record.status, TestStatus.FAIL)
        self.assertIs(record.passed, False)
        self.assertEqual(app._feature_stats["a"], [0, 1])
        self.assertIs(record.review_needed, False)

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

    def test_trig_rejects_excessive_nesting_cleanly(self):
        nested = "(" * 205 + "x" + ")" * 205
        output = run_cli("trigProgram.py", "1\n" + nested + "\nx\n1\n")
        self.assertIn("Err: Expression too nested", output)

    def test_trig_solve_accepts_plain_equation(self):
        output = run_cli("trigProgram.py", "3\nsin(x)=0\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Answer:", output)
        self.assertTrue("x = [" in output or "n any integer" in output, msg=output)

    def test_trig_tan_equality_uses_degree_period(self):
        output = run_cli("trigProgram.py", "3\ntan(2*x)=tan(x),x,0,360\n")
        self.assertNotIn("Err:", output)
        self.assertIn("n*180", output)
        self.assertNotIn("n*pi", output)

    def test_trig_interval_solver_lists_roots_not_general_family(self):
        output = run_cli("trigProgram.py", "3\ntan(2*x)-tan(x)=0,x,-pi,pi\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Answer: x = [-pi, 0, pi]", output)

    def test_trig_solver_accepts_compact_function_syntax(self):
        output = run_cli("trigProgram.py", "3\nsinx=0,x,0,360\n")
        self.assertNotIn("Err:", output)
        self.assertIn("sin(x) = 0", output)
        self.assertIn("Answer: x = [0, 180, 360]", output)

    def test_trig_solver_reciprocal_working_uses_right_family(self):
        output = run_cli("trigProgram.py", "3\ncosecx=2,x,0,360\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Use cosec(A) = 1/sin(A), so sin(A) = 1/2.", output)
        self.assertIn("Answer: x = [30, 150]", output)

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
        self.assertNotIn("Method:", output)
        self.assertNotIn("Step 1:", output)
        self.assertIn("Factor as a difference of squares", output)
        self.assertIn("Answer:(x-1)*(x+1)=0", output.replace(" ", ""))

    def test_algebra_transform_delegates_trig_equations(self):
        output = run_cli("algebraProgram.py", "2\nsin(x)=12/13\ncot(2*x)=119/120\n")
        self.assertNotIn("Err:", output)
        self.assertIn("obtuse branch", output.lower())
        self.assertIn("Answer: cot(2*x)=119/120", output)

    def test_derive_accepts_compact_inverse_trig_syntax(self):
        output = run_cli("deriveProgram.py", "1\nsin^-1x\n")
        self.assertNotIn("Err:", output)
        self.assertNotIn("Method:", output)
        self.assertNotIn("Using asin rule", output)
        self.assertNotIn("Answer: dy/dx", output)
        self.assertIn("dy/dx = 1/sqrt(1 - x^2)", output)

    def test_integrate_accepts_compact_trig_syntax(self):
        output = run_cli("intProgram.py", "1\nsinx\n1\n\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Use the standard integral for sin(x).", output)
        self.assertIn("Answer: -cos(x) + C", output)

    def test_derive_triple_product_keeps_three_factor_working(self):
        expr = "(log(exp(9x-7)+8))(cosec(8(9x^2-(5)(x)+-6)+1))((exp(4x-6))+(6x-5))"
        output = run_cli("deriveProgram.py", "1\n" + expr + "\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Let u = ln(exp(9*x - 7) + 8), v = cosec(8*(9*x^2 - 5*x - 6) + 1), w = exp(4*x - 6) + 6*x - 5", output)
        self.assertIn("dy/dx = u'*v*w + u*v'*w + u*v*w'", output)

    def test_derive_product_rule_final_answer_prefers_factored_common_exp(self):
        out = run_cli("deriveProgram.py", "1\n(4*x-k)*exp(-x^2)\n")
        self.assertNotIn("Err:", out)
        # Prefer factored form.
        self.assertNotIn("Answer: dy/dx", out)
        self.assertIn("dy/dx=e^(-x^2)*(4-2*x*(4*x-k))", out.replace(" ", ""))

    def test_derive_abs_notes_undefined_point(self):
        output = run_cli("deriveProgram.py", "1\n|x|\n")
        self.assertNotIn("Err:", output)
        self.assertIn("d/dx|u| = (u/abs(u))*du/dx", output)
        self.assertIn("undefined where x = 0", output)
        self.assertIn("dy/dx = x/abs(x)", output)

    def test_derive_mark_scheme_factored_forms_from_exam_pack(self):
        out = run_cli("deriveProgram.py", "1\n(2*x+ln(x))^3\n")
        self.assertNotIn("Err:", out)
        self.assertNotIn("Answer: dy/dx", out)
        self.assertIn("dy/dx = 3*(2 + 1/x)*(2*x + ln(x))^2", out)

        out = run_cli("deriveProgram.py", "1\n(x+1)^2*exp(2*x)\n")
        self.assertNotIn("Err:", out)
        self.assertIn("dy/dx = 2*e^(2*x)*(x + 1)*(x + 2)", out)

        out = run_cli("deriveProgram.py", "4\n(x+1)^2*exp(2*x)\n")
        self.assertNotIn("Err:", out)
        self.assertNotIn("Answer: d2y/dx2", out)
        self.assertIn("d2y/dx2 = 2*exp(2*x)*(2*x^2 + 8*x + 7)", out)

        out = run_cli("deriveProgram.py", "1\ncos(x)/(3-sin(x))\n")
        self.assertNotIn("Err:", out)
        self.assertIn("dy/dx = (1 - 3*sin(x))/(3 - sin(x))^2", out)

        out = run_cli("deriveProgram.py", "1\nx*asin(2*x)\n")
        self.assertNotIn("Err:", out)
        self.assertIn("dy/dx = asin(2*x) + 2*x/sqrt(1 - 4*x^2)", out)

    def test_parametric_vertical_tangent_is_answer_not_error(self):
        output = run_cli("deriveProgram.py", "3\n5\nt^2\n")
        self.assertNotIn("Err:", output)
        self.assertIn("dx/dt = 0", output)
        self.assertIn("Answer: dy/dx undefined (vertical tangent)", output)

    def test_cartesian_parametric_linear_offsets(self):
        output = run_cli("algebraProgram.py", "11\nt+1\nt-1\n\n")
        self.assertNotIn("Err:", output)
        self.assertNotIn("Method:", output)
        self.assertIn("Cartesian: y = x - 2", output)
        self.assertIn("Answer: y = x - 2", output)

    def test_exam_pack_parametric_cartesian_trig_reciprocal_pair(self):
        output = run_cli("algebraProgram.py", "11\ntan(t)-sec(t)\ncot(t)-cosec(t)\nt\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Use tan=sin/cos", output)
        self.assertIn("Answer: (x^2 - 1)*(y^2 - 1) = 4*x*y", output)

    def test_exam_pack_binomial_series_forms(self):
        output = run_cli("algebraProgram.py", "3\n(1-3*x)^(-1/3)\n3\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Out = 1 + x + 2*x^2 + ...", output)

        output = run_cli("algebraProgram.py", "3\n(8-3*x)^(1/3)\n3\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Out = 2 - (1/4)*x - (1/32)*x^2 + ...", output)

        output = run_cli("algebraProgram.py", "3\nsqrt(225+15*x)\n3\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Out = 15 + (1/2)*x - (1/120)*x^2 + ...", output)

    def test_integral_reciprocal_exp_substitution_shows_algebra(self):
        output = run_cli("intProgram.py", "1\n((1/x**2)+(1/x**3))*exp(1/x)\n5\n")
        self.assertNotIn("Method:", output)
        self.assertNotIn("Attempt", output)
        self.assertIn("du/dx = -1/x^2, so dx = -x^2 du.", output)
        self.assertIn("x = 1/u, so 1/x^2 = u^2 and 1/x^3 = u^3.", output)
        self.assertIn("I = Int[(- u - 1)*e^(u)] du.", output)
        self.assertIn("= -u*e^(u) + C.", output)
        self.assertIn("Answer: -e^(1/x)/x + C", output)

    def test_exam_pack_trig_integrals_prefer_mark_scheme_rewrites(self):
        out = run_cli("intProgram.py", "1\n1/(1+cos(2*x))\n1\n\n")
        self.assertNotIn("Err:", out)
        self.assertIn("Use 1+cos(2A) = 2cos^2 A.", out)
        self.assertIn("Answer: 1/2*tan(x) + C", out)

        out = run_cli("intProgram.py", "1\ncos(x)^3/((1+sin(x)^2)*sin(x))\n1\n\n")
        self.assertNotIn("Err:", out)
        self.assertIn("(1-u^2)/(u(1+u^2)) = 1/u - 2u/(1+u^2).", out)
        self.assertIn("Answer: ln|sin(x)/(sin(x)^2 + 1)| + C", out)

    def test_integration_handles_cosec_cubed(self):
        output = run_cli("intProgram.py", "1\ncosec(x)^3\n1\n")
        self.assertNotIn("no elementary", output.lower())
        self.assertIn("I = Int[cosec^3 A] dA.", output)
        self.assertIn("Int[cosec^3 A] dA", output)
        self.assertIn("Answer: -1/2*cosec(x)*cot(x) - 1/2*ln|cosec(x) + cot(x)| + C", output)

    def test_harness_accepts_scaled_cosec_cubed_integral(self):
        output = run_cli("intProgram.py", "1\n2*cosec(x)^3\n1\n")
        app = CASIOApp()
        self.assertIn("I = 2 Int[cosec^3 A] dA.", output)
        self.assertTrue(app.integrate_output_checker("2*cosec(x)^3")(output), output)

    def test_algebra_cubic_numeric_fallback_after_rearrange(self):
        output = run_cli("algebraProgram.py", "6\n((4*x)**2=16*x^2*x-10)\n")
        self.assertNotIn("no closed-form solution found", output)
        self.assertIn("bounded real-root scan", output)
        self.assertIn("Answer: x = 1.345323724", output)

    def test_algebra_range_exp_is_positive(self):
        output = run_cli("algebraProgram.py", "10\nexp(m)\n")
        self.assertIn("Since exp(u) is always positive.", output)
        self.assertIn("Range: y > 0", output)

    def test_algebra_inverse_restricted_quadratic_and_exp_range(self):
        inv = run_cli("algebraProgram.py", "8\n3*(x-3)^2-6,x>=3\n\n")
        self.assertNotIn("Err:", inv)
        self.assertIn("Use restricted branch y - 3 >= 0", inv)
        self.assertIn("Answer: f^-1(x) = ((1/3)*(x + 6))^(1/2) + 3", inv)
        self.assertIn("Domain of f^-1 : x >= -6", inv)
        self.assertIn("Range of f^-1 : x>=3", inv)

        inv = run_cli("algebraProgram.py", "8\nln(4-2*x),x<2\n\n")
        self.assertNotIn("Err:", inv)
        self.assertIn("Answer: f^-1(x) = (4 - e^x)/2", inv)
        self.assertIn("Range of f^-1 : x<2", inv)

        rng = run_cli("algebraProgram.py", "10\n(4-exp(x))/2\n")
        self.assertNotIn("Err:", rng)
        self.assertIn("Since exp(u) is always positive.", rng)
        self.assertIn("Range: y < 2", rng)

    def test_algebra_abs_linear_pair_and_reciprocal_inverse(self):
        out = run_cli("algebraProgram.py", "6\nabs(x-sqrt(2))=abs(x+5*sqrt(2)),x\n")
        self.assertNotIn("Err:", out)
        self.assertIn("Square both sides: |u|=|v| gives u=v or u=-v", out)
        self.assertIn("Answer: x = -2*sqrt(2)", out)

        inv = run_cli("algebraProgram.py", "8\n4-1/(x-1),x>1\n\n")
        self.assertNotIn("Err:", inv)
        self.assertIn("Answer: f^-1(x) = 1 - 1/(x - 4)", inv)
        self.assertIn("Domain of f^-1 : x < 4", inv)
        self.assertIn("Range of f^-1 : x>1", inv)

        rng = run_cli("algebraProgram.py", "10\n1-1/(x-4)\n")
        self.assertNotIn("Err:", rng)
        self.assertIn("Range: y != 1", rng)

    def test_trig_prove_scaled_sec_tan_reduces_directly(self):
        output = run_cli("trigProgram.py", "1\n9*sec(2*x)^2-9*tan(2*x)^2\n9\n1\n")
        self.assertIn("Use Pythagorean", output)
        self.assertIn("= 9", output)
        self.assertNotIn("cos(x)^2 - sin(x)^2", output)

    def test_exam_pack_trig_product_to_sum_solver(self):
        output = run_cli("trigProgram.py", "3\n2*cos(x)*cos(2*x)=cos(x)+sin(x),x,0,2*pi\n")
        self.assertNotIn("Err:", output)
        self.assertIn("Use 2cos(A)cos(B) = cos(A+B)+cos(A-B)", output)
        self.assertIn("cos(3x) = sin(x)", output)
        self.assertIn("pi/8", output)

    def test_short_chain_quotient_keeps_calculable_answer(self):
        expr = "((atan(-3*x+4))/(sqrt(((9*x+3)^2+(4*x-7))^2+8)+1))^2"
        output = run_cli("deriveProgram.py", "1\n" + expr + "\n")
        self.assertNotIn("Answer: dy/dx = (D*dN - N*dD)/D^2", output)
        self.assertIn("Quotient rule", output)
        self.assertIn("atan(4 - 3*x)", output)

    def test_long_chain_quotient_uses_compact_exam_symbols(self):
        expr = "((sin((3)*(2*(9*x^2-4*x+4)-3)^2-7)^5+cos(3*(2*(9*x^2-4*x+4)-3)^2-7)^5)/(exp(5*x+5)+8))^4"
        output = run_cli("deriveProgram.py", "1\n" + expr + "\n")
        self.assertIn("Write y = N/D.", output)
        self.assertIn("Let u = N and v = D.", output)
        self.assertNotIn("Answer: dy/dx = (D*dN - N*dD)/D^2", output)
        self.assertIn("dy/dx =", output)

    def test_product_derivative_keeps_nested_trig_argument_factored(self):
        expr = "(cot(2*((2)*(3*(7*(x)**2+2*x+-7)**2+6)+pi/(3))^2+2))*exp(-7*x-5)*log(log(abs(2*(x)+(5))+10))"
        output = run_cli("deriveProgram.py", "1\n" + expr + "\n")
        self.assertNotIn("1555848*x^8", output)
        self.assertIn("dy/dx = u'*v*w + u*v'*w + u*v*w'", output)
        self.assertIn("dy/dx =", output)

    def test_trig_grouped_general_solution_detection(self):
        result = trig_program.detect_general_solution([30.0, 150.0, 390.0, 510.0], True)
        self.assertEqual(result, "30 + n*360 or 150 + n*360 (n any integer)")

    def test_trig_special_identity_rewrites_pythagorean_forms(self):
        node = trig_program.parse("1-sin(x)^2")
        reduced, note = trig_program.special_trig_identity_once(node)
        self.assertIsNotNone(reduced)
        self.assertEqual(trig_program.show(reduced), "cos(x)^2")
        self.assertIn("1 - sin^2", note)

    def test_trig_numeric_eval_overflow_returns_none(self):
        node = trig_program.parse("exp(1000)")
        self.assertIsNone(trig_program.numeric_eval(node, {}, False))

    def test_suvat_zero_average_velocity_reports_no_solution(self):
        output = run_cli("SUVATprogram.py", "10\n5\n-5\n\n,\n")
        self.assertIn("No solution: division by zero in t formula", output)

    def test_integrate_trig_odd_power_5_simplifies_correctly(self):
        out1 = run_cli("intProgram.py", "1\ncos(x)^5\n1\n")
        self.assertNotIn("no elementary antideriv", out1.lower())
        self.assertIn("Answer: 1/5*sin(x)^5 - 2/3*sin(x)^3 + sin(x) + C", out1)
        out2 = run_cli("intProgram.py", "1\nsin(x)^5\n1\n")
        self.assertNotIn("no elementary antideriv", out2.lower())
        self.assertIn("Answer: -1/5*cos(x)^5 + 2/3*cos(x)^3 - cos(x) + C", out2)

    def test_trig_factor_branch_none_in_interval_not_global_no_solution(self):
        out = run_cli("trigProgram.py", "3\nsin(2*x)+cos(x)=0,x,0,180\n\n")
        self.assertNotIn("No solutions in the interval", out)
        self.assertIn("None in interval", out)
        self.assertIn("Answer: x = [90]", out)

    def test_algebra_hidden_substitution_filters_negative_u_roots(self):
        out = run_cli("algebraProgram.py", "6\nx^4+6*x^2-361=0\n")
        self.assertNotIn("Err:", out)
        # Should not emit sqrt of negative u (no complex roots in real solve mode).
        self.assertNotIn("sqrt(-", out.replace(" ", ""))
        self.assertIn("Answer:", out)

    def test_algebra_expand_expands_simple_power_denominator(self):
        out = run_cli("algebraProgram.py", "3\n4/((2+3x)^2)\n5\n")
        self.assertNotIn("Err:", out)
        self.assertIn("Out =", out)
        # Expect expanded quadratic denominator (form may be reordered by canonicalisation).
        compact = out.replace(" ", "")
        self.assertTrue(
            "4/(9*x^2+12*x+4)" in compact
            or "4/(4+12*x+9*x^2)" in compact
            or "4/(9*x^2+4+12*x)" in compact,
            msg=out,
        )

    def test_exam_trig_phase_range_and_minimum_fixture(self):
        out = run_cli("trigProgram.py", "2\n8+3*sin(x)*(cos(x)-2*sin(x))\nP+R*sin(2*x+a)\n")
        self.assertNotIn("Err:", out)
        self.assertIn("P = 5", out)
        self.assertIn("R = 3/2*sqrt(5)", out)
        self.assertIn("a = 1.107 rad", out)
        rng = run_cli("algebraProgram.py", "10\n8+3*sin(x)*(cos(x)-2*sin(x)),x,0,2*pi\n")
        self.assertNotIn("Err:", rng)
        self.assertIn("Range: 5 - 3/2*sqrt(5) <= y <= 5 + 3/2*sqrt(5)", rng)

    def test_exam_inverse_trig_equation_fixture(self):
        out = run_cli("trigProgram.py", "3\n3*asin(y)=2*acos(y),y,-1,1\n")
        self.assertNotIn("Err:", out)
        self.assertIn("Use acos(u) = pi/2 - asin(u)", out)
        self.assertIn("5*asin(y) = pi", out)
        self.assertIn("Answer: y = [sin(pi/5)]", out)

    def test_exam_definite_integral_trig_conjugate_fixture(self):
        out = run_cli("intProgram.py", "1\n(x*tan(x))/(tan(x)+sec(x)),x,0,pi\n1\n")
        self.assertNotIn("Err:", out)
        self.assertIn("tan(x)/(tan(x)+sec(x)) = sin(x)/(1+sin(x)).", out)
        self.assertIn("[x - tan(x) + sec(x)]_0^pi = pi - 2.", out)
        self.assertIn("Answer: 1/2*pi*(pi - 2)", out)

    def test_exam_definite_integral_double_angle_fraction_fixture(self):
        out = run_cli("intProgram.py", "1\n(sin(2*x))/(1+cos(x)),x,0,pi/2\n1\n")
        self.assertNotIn("Err:", out)
        self.assertIn("Use sin(2A) = 2sin A cos A.", out)
        self.assertIn("F(0) = 2*ln(2) - 2.", out)
        self.assertIn("Answer: 2 - 2*ln(2)", out)

    def test_exam_log_curve_tangent_fixture(self):
        out = run_cli("deriveProgram.py", "1\nlog(2*x)/(2*x),e^2/2\n")
        self.assertNotIn("Err:", out)
        self.assertIn("Gradient m = -2/e^4.", out)
        self.assertIn("y = -2/e^4*x + 3/e^2", out)

    def test_exam_parametric_cartesian_fixtures(self):
        loop = run_cli("algebraProgram.py", "11\n2*sin(t)\n3*sin(2*t)\nt\n")
        self.assertNotIn("Err:", loop)
        self.assertIn("k = 3/2.", loop)
        self.assertIn("Maximum radius = 10/3.", loop)
        self.assertIn("Answer: y^2 = (9/4)*x^2*(-x^2 + 4)", loop)
        circle = run_cli("algebraProgram.py", "11\n(t^2+9)/(t^2+3)\n6*sqrt(3)*t/(t^2+3)\nt\n")
        self.assertNotIn("Err:", circle)
        self.assertIn("9*(x - 2)^2 + y^2 simplifies to 9.", circle)
        self.assertIn("Answer: 9*(x - 2)^2 + y^2 = 9", circle)

    def test_shared_cache_eviction_uses_snapshot_keys(self):
        cache = {}
        for i in range(10):
            cache_store(cache, i, i, 4)
        self.assertLessEqual(len(cache), 4)

    def test_llm_response_parser_refusal_goes_to_needs_review(self):
        manager = LLMManager()
        parsed = manager._parse_response(
            "I'm sorry, but as an AI language model...\n#1 CORRECT\n#2 INCORRECT"
        )
        self.assertEqual(parsed.get("verdict"), "NEEDS_REVIEW")


if __name__ == "__main__":
    unittest.main()
