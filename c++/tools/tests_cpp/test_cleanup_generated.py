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
            cache = root / "c++" / "tools" / "tests_cpp" / "__pycache__"
            ds = root / ".DS_Store"
            report.parent.mkdir(parents=True)
            cache.mkdir(parents=True)
            report.write_text("old report\n", encoding="utf-8")
            (cache / "x.pyc").write_bytes(b"pyc")
            ds.write_text("mac\n", encoding="utf-8")

            removed = CLEAN.cleanup(root, dry_run=True)

            self.assertEqual({p.relative_to(root).as_posix() for p in removed}, {
                ".DS_Store",
                "c++/tests/reports/failure_report_latest.txt",
                "c++/tools/tests_cpp/__pycache__",
            })
            self.assertTrue(report.exists())
            self.assertTrue(cache.exists())
            self.assertTrue(ds.exists())


if __name__ == "__main__":
    unittest.main()
