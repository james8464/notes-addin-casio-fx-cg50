"""Coverage-directed adversarial generators for the Casio C++ tester."""

from .concepts import ConceptGraph, ConceptNode
from .generators import AdversarialCase, AdversarialGenerator, EXAM_GAP_TOPICS
from .oracles import OutputQuality, classify_output_quality
from .reports import RunReportStore
from .shrinker import shrink_expression_text

__all__ = [
    "AdversarialCase",
    "AdversarialGenerator",
    "EXAM_GAP_TOPICS",
    "ConceptGraph",
    "ConceptNode",
    "OutputQuality",
    "RunReportStore",
    "classify_output_quality",
    "shrink_expression_text",
]
