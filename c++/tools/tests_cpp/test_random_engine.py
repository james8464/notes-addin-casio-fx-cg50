#!/usr/bin/env python3
import tempfile
import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from random_engine.concepts import ConceptGraph, ConceptNode
from random_engine.generators import AdversarialGenerator, EXAM_GAP_TOPICS
from random_engine.oracles import classify_output_quality
from random_engine.reports import RunReportStore
from random_engine.shrinker import shrink_expression_text


class RandomEngineTests(unittest.TestCase):
    def test_graph_scores_unseen_before_passed(self):
        graph = ConceptGraph()
        unseen = ConceptNode("integrate", "method", "parts", "exp_trig", ("expand",), 4, "sympy")
        seen = ConceptNode("integrate", "method", "direct", "poly", (), 1, "sympy")
        graph.record(seen, "pass")

        ordered = graph.prioritize([seen, unseen])

        self.assertEqual(ordered[0].key, unseen.key)

    def test_generator_emits_hard_transform_metadata(self):
        gen = AdversarialGenerator(seed=123)

        cases = gen.generate("integrate", 12)

        self.assertGreaterEqual(len(cases), 12)
        self.assertTrue(any(case.concept.difficulty >= 4 for case in cases))
        self.assertTrue(any(case.concept.transforms for case in cases))
        self.assertTrue(all(case.command_flag for case in cases))

    def test_all_scope_includes_general_non_exam_cases(self):
        gen = AdversarialGenerator(seed=321)

        cases = gen.generate("all", 48)

        functions = {case.concept.function for case in cases}
        topics = {case.concept.topic for case in cases}
        notes = " ".join(case.expected_note.lower() for case in cases)
        self.assertIn("general", functions)
        self.assertTrue({"non_elementary", "branch", "special_function"} & topics)
        self.assertIn("advanced", notes)

    def test_exam_gap_scope_targets_full_mark_working_patterns(self):
        gen = AdversarialGenerator(seed=777)

        cases = gen.generate("exam_gap", 40)

        topics = {case.concept.topic for case in cases}
        notes = " ".join(case.expected_note.lower() for case in cases)
        self.assertTrue(set(EXAM_GAP_TOPICS).issubset(topics))
        self.assertIn("exam", notes)
        self.assertIn("show", notes)

    def test_syllabus_scope_targets_edexcel_capability_matrix(self):
        gen = AdversarialGenerator(seed=901)

        cases = gen.generate("syllabus", 60)

        topics = {case.concept.topic for case in cases}
        self.assertIn("pure_algebra", topics)
        self.assertIn("pure_integration", topics)
        self.assertIn("stats_probability", topics)
        self.assertIn("mechanics_kinematics", topics)
        self.assertTrue(all("syllabus" in case.expected_note.lower() for case in cases))

    def test_online_scope_targets_source_anchored_hard_cases(self):
        gen = AdversarialGenerator(seed=902)

        cases = gen.generate("online", 30)

        sources = " ".join(case.source_kernel.lower() for case in cases)
        self.assertIn("source:", sources)
        self.assertTrue(any(case.concept.difficulty >= 5 for case in cases))

    def test_general_scope_only_emits_general_cases(self):
        gen = AdversarialGenerator(seed=222)

        cases = gen.generate("general", 12)

        self.assertTrue(cases)
        self.assertEqual({case.concept.function for case in cases}, {"general"})

    def test_quality_classifier_flags_unsupported_supported_case(self):
        verdict = classify_output_quality("Answer: int(exp(-x^2),x)", expects_working=True)

        self.assertEqual(verdict.status, "fail")
        self.assertIn("unevaluated", verdict.reason)

    def test_quality_classifier_allows_honest_non_elementary_working(self):
        verdict = classify_output_quality(
            "Start with exp(-x^2).\n"
            "No elementary primitive found; use special-function form.\n"
            "Answer: sqrt(pi)/2*erf(x)+C",
            expects_working=True,
        )

        self.assertNotEqual(verdict.status, "fail")

    def test_quality_classifier_flags_ibp_without_choices(self):
        verdict = classify_output_quality(
            "Use integration by parts.\n"
            "e^(2*x)*cos(3*x) -> e^(2*x)*(2*cos(3*x)+3*sin(3*x))/13",
            expects_working=True,
        )

        self.assertIn(verdict.status, {"review", "fail"})
        self.assertIn("ibp", verdict.reason.lower())

    def test_quality_classifier_accepts_full_ibp_choices(self):
        verdict = classify_output_quality(
            "Let I=int(e^(2*x)*cos(3*x),x)\n"
            "u=cos(3*x), dv=e^(2*x) dx\n"
            "du=-3*sin(3*x) dx, v=1/2*e^(2*x)\n"
            "I=1/2*e^(2*x)*cos(3*x)+3/2*int(e^(2*x)*sin(3*x),x)\n"
            "Then solve the repeated I equation.\n"
            "e^(2*x)*(2*cos(3*x)+3*sin(3*x))/13",
            expects_working=True,
        )

        self.assertEqual(verdict.status, "pass")

    def test_quality_classifier_flags_pf_without_coefficients(self):
        verdict = classify_output_quality(
            "Use partial fractions.\n-ln(abs(x))-1/x+ln(abs(x+1))",
            expects_working=True,
        )

        self.assertIn(verdict.status, {"review", "fail"})
        self.assertIn("partial fraction", verdict.reason.lower())

    def test_quality_classifier_does_not_treat_partial_fractions_as_ibp(self):
        verdict = classify_output_quality(
            "Route: partial fractions\n"
            "Set (x^2+1)/(x^4+1)=(Ax+B)/(x^2+sqrt(2)*x+1)+(Cx+D)/(x^2-sqrt(2)*x+1).\n"
            "Multiply through and equate coefficients.\n"
            "A=1/2, B=1/2, C=-1/2, D=1/2.\n"
            "Answer: atan((x - 1/x)/sqrt(2))/sqrt(2) + C",
            expects_working=True,
        )

        self.assertNotEqual(verdict.reason, "IBP missing u/dv/du/v choices")
        self.assertEqual(verdict.status, "pass")

    def test_quality_classifier_flags_substitution_without_differential(self):
        verdict = classify_output_quality(
            "Let u=x^2+1.\n1/2*ln(abs(x^2+1))",
            expects_working=True,
        )

        self.assertIn(verdict.status, {"review", "fail"})
        self.assertIn("differential", verdict.reason.lower())

    def test_quality_classifier_allows_non_calculus_substitution(self):
        verdict = classify_output_quality(
            "Use sin(A)^2=1-cos(A)^2.\n"
            "Let u=cos(x).\n"
            "2u^2+u-1=0.\n"
            "Answer: x = [pi/3, pi, 5*pi/3]",
            expects_working=True,
        )

        self.assertNotEqual(verdict.status, "review")

    def test_quality_classifier_accepts_spaced_dx_du(self):
        verdict = classify_output_quality(
            "Let u=2x, du=2 dx.\n"
            "dx = du/2.\n"
            "Integral becomes Integral(sin(u))/2 du.\n"
            "Answer: -cos(2*x)/2 + C",
            expects_working=True,
        )

        self.assertEqual(verdict.status, "pass")

    def test_report_store_records_replayable_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            store = RunReportStore(Path(tmp))
            gen = AdversarialGenerator(seed=5)
            case = gen.generate("solve_trig", 1)[0]

            record = store.record_case(case, "fail", "bad output", "missing CAST")

            self.assertTrue(record.case_id)
            self.assertTrue((Path(tmp) / "replay_index.json").exists())
            self.assertEqual(store.load_case(record.case_id)["status"], "fail")

    def test_shrinker_removes_safe_wrapper_noise(self):
        shrunk = shrink_expression_text("((sin(x)^2+cos(x)^2))+0")

        self.assertIn("sin(x)^2+cos(x)^2", shrunk)
        self.assertNotIn("+0", shrunk)


if __name__ == "__main__":
    unittest.main()
