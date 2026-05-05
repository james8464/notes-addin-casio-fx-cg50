from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, Iterable, List, Tuple


@dataclass(frozen=True)
class ConceptNode:
    function: str
    parameter_signature: str
    method: str
    topic: str
    transforms: Tuple[str, ...] = ()
    difficulty: int = 1
    oracle: str = "heuristic"

    @property
    def key(self) -> str:
        chain = ">".join(self.transforms) if self.transforms else "plain"
        return "|".join(
            [
                self.function,
                self.parameter_signature,
                self.method,
                self.topic,
                chain,
                str(int(self.difficulty)),
                self.oracle,
            ]
        )

    def meta(self) -> dict:
        return {
            "function": self.function,
            "parameter_signature": self.parameter_signature,
            "method": self.method,
            "topic": self.topic,
            "transforms": list(self.transforms),
            "difficulty": int(self.difficulty),
            "oracle": self.oracle,
        }


@dataclass
class ConceptStats:
    attempts: int = 0
    passed: int = 0
    failed: int = 0
    review: int = 0
    last_status: str = "new"


@dataclass
class ConceptGraph:
    stats: Dict[str, ConceptStats] = field(default_factory=dict)

    def reset(self) -> None:
        self.stats.clear()

    def record(self, node: ConceptNode, status: str) -> None:
        item = self.stats.setdefault(node.key, ConceptStats())
        item.attempts += 1
        item.last_status = status
        if status == "pass":
            item.passed += 1
        elif status == "review":
            item.review += 1
        else:
            item.failed += 1

    def score(self, node: ConceptNode) -> tuple:
        item = self.stats.get(node.key)
        if item is None:
            return (0, -node.difficulty, node.key)
        if item.failed or item.review:
            return (1, item.attempts, -node.difficulty, node.key)
        if item.passed:
            return (3, item.attempts, -node.difficulty, node.key)
        return (2, item.attempts, -node.difficulty, node.key)

    def prioritize(self, nodes: Iterable[ConceptNode]) -> List[ConceptNode]:
        return sorted(nodes, key=self.score)

    def summary_lines(self) -> List[str]:
        total = len(self.stats)
        passed = sum(1 for s in self.stats.values() if s.passed)
        failed = sum(1 for s in self.stats.values() if s.failed)
        review = sum(1 for s in self.stats.values() if s.review)
        return [
            "Adversarial graph: nodes={0} pass={1} fail={2} review={3}".format(
                total, passed, failed, review
            )
        ]
