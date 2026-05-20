#!/usr/bin/env python3
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import cleanup_generated as CLEAN


class CleanupGeneratedTests(unittest.TestCase):
    def test_dry_run_lists_ignored_generated_files_without_deleting(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            report = root / "c++" / "tests" / "reports" / "failure_report_latest.txt"
            nested_report = root / "c++" / "tests" / "reports" / "adversarial_runs" / "latest.txt"
            cache = root / "c++" / "tools" / "tests_cpp" / "__pycache__"
            extra_cache = root / "c++" / "tools" / "__pycache__"
            fuzz_latest = root / "c++" / "tools" / "fuzz" / "random_exploration_graph.json"
            ds = root / ".DS_Store"
            report.parent.mkdir(parents=True)
            nested_report.parent.mkdir(parents=True)
            cache.mkdir(parents=True)
            extra_cache.mkdir(parents=True)
            fuzz_latest.parent.mkdir(parents=True)
            report.write_text("old report\n", encoding="utf-8")
            nested_report.write_text("old nested report\n", encoding="utf-8")
            (cache / "x.pyc").write_bytes(b"pyc")
            (extra_cache / "y.pyc").write_bytes(b"pyc")
            fuzz_latest.write_text("{}\n", encoding="utf-8")
            ds.write_text("mac\n", encoding="utf-8")

            removed = CLEAN.cleanup(root, dry_run=True)

            self.assertEqual({p.relative_to(root).as_posix() for p in removed}, {
                ".DS_Store",
                "c++/tools/__pycache__",
                "c++/tools/__pycache__/y.pyc",
                "c++/tools/fuzz/random_exploration_graph.json",
                "c++/tools/tests_cpp/__pycache__",
                "c++/tools/tests_cpp/__pycache__/x.pyc",
            })
            self.assertTrue(report.exists())
            self.assertTrue(nested_report.exists())
            self.assertTrue(cache.exists())
            self.assertTrue(extra_cache.exists())
            self.assertTrue(fuzz_latest.exists())
            self.assertTrue(ds.exists())

    def test_reports_are_explicit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            report = root / "c++" / "tests" / "reports" / "failure_report_latest.txt"
            empty = root / "c++" / "tests" / "reports" / "adversarial_runs" / "empty"
            report.parent.mkdir(parents=True)
            empty.mkdir(parents=True)
            report.write_text("old report\n", encoding="utf-8")

            default = CLEAN.cleanup(root, dry_run=True)
            explicit = CLEAN.cleanup(root, dry_run=True, include_reports=True)

            self.assertEqual({p.relative_to(root).as_posix() for p in default}, {
                "c++/tests/reports/adversarial_runs/empty",
            })
            self.assertIn("c++/tests/reports/failure_report_latest.txt", {p.relative_to(root).as_posix() for p in explicit})
            self.assertIn("c++/tests/reports", {p.relative_to(root).as_posix() for p in explicit})


if __name__ == "__main__":
    unittest.main()
