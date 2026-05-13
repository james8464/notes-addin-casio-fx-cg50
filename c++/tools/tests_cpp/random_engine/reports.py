from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
import json
from pathlib import Path
from typing import Dict

from .generators import AdversarialCase
from .shrinker import shrink_expression_text


@dataclass
class StoredCase:
    case_id: str
    status: str
    path: Path


class RunReportStore:
    def __init__(self, root: Path):
        self.root = Path(root)
        self.root.mkdir(parents=True, exist_ok=True)
        self.index_path = self.root / "replay_index.json"
        self.index: Dict[str, dict] = {}
        self._load()

    @classmethod
    def fresh_under(cls, reports_root: Path) -> "RunReportStore":
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        return cls(Path(reports_root) / "adversarial_runs" / stamp)

    def _load(self) -> None:
        try:
            self.index = json.loads(self.index_path.read_text(encoding="utf-8"))
        except Exception:
            self.index = {}

    def _save(self) -> None:
        tmp = self.index_path.with_suffix(".tmp")
        tmp.write_text(json.dumps(self.index, separators=(",", ":"), sort_keys=True), encoding="utf-8")
        tmp.replace(self.index_path)

    def record_case(self, case: AdversarialCase, status: str, output: str, reason: str) -> StoredCase:
        n = len(self.index) + 1
        case_id = "adv-{0:05d}".format(n)
        path = self.root / (case_id + ".json")
        payload = {
            "case_id": case_id,
            "status": status,
            "reason": reason,
            "label": case.label,
            "command_flag": case.command_flag,
            "command_expr": case.command_expr,
            "input_text": case.input_text,
            "source_kernel": case.source_kernel,
            "shrunk_expr": shrink_expression_text(case.command_expr),
            "concept": case.concept.meta(),
            "expected_note": case.expected_note,
            "output": output,
        }
        path.write_text(json.dumps(payload, separators=(",", ":"), sort_keys=True), encoding="utf-8")
        self.index[case_id] = {
            "status": status,
            "reason": reason,
            "path": str(path),
            "input_text": case.input_text,
            "concept": case.concept.key,
        }
        self._save()
        return StoredCase(case_id, status, path)

    def load_case(self, case_id: str) -> dict:
        item = self.index.get(case_id)
        if not item:
            raise KeyError(case_id)
        return json.loads(Path(item["path"]).read_text(encoding="utf-8"))
