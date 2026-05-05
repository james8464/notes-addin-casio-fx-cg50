#!/usr/bin/env python3
import tempfile
import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from random_engine.concepts import ConceptGraph, ConceptNode
from random_engine.generators import AdversarialGenerator
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

    def test_quality_classifier_flags_unsupported_supported_case(self):
        verdict = classify_output_quality("Answer: int(exp(-x^2),x)", expects_working=True)

        self.assertEqual(verdict.status, "fail")
        self.assertIn("unevaluated", verdict.reason)

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
