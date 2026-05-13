# Past Paper Audit

Sources:
- PMT A Level Edexcel papers: https://www.physicsandmathstutor.com/maths-revision/a-level-edexcel/papers/
- PMT Further Maths Edexcel papers: https://www.physicsandmathstutor.com/maths-revision/a-level-edexcel/papers-further/

Corpus:
- PDFs/text are ignored under `c++/tests/reports/pmt_pdf_corpus/`.
- Audit rows live in `past_paper_audit.csv`.

Status:
- 217 PDFs downloaded/extracted.
- 8 question-parts manually checked against mark-scheme text.
- This is not a complete 217-document audit yet.

Current Findings:

| id | status | result |
|---|---|---|
| PMT-CP1-2020-Q7 | PASS-DE-PART | Linear DE now integrates polynomial RHS and shows IF lines. |
| PMT-CP1-2021-Q8 | PASS-PARTIAL | DE + follow-up solve covers model solution. |
| PMT-CP2-2020-Q3 | GAP-HONEST | Second-order DE no longer fakes working; route still needed. |
| PMT-P1-2020O-Q9 | PASS-DE-PART | Separable DE with `k` and two boundary values works. |
| PMT-CP1-2022-Q3 | PASS-DE-PART | Trig linear DE works with `p=tan(x)` and `mu=sec(x)`. |
| PMT-CP1-2024-Q5 | PASS-DE-PART | Symbolic `k`, IF cancellation, and `C=8-4k` route works. |
| PMT-FP1-2024-Q10 | GAP-HONEST | Transformed second-order DE unsupported. |
| PMT-CP2-2024-Q6 | GAP-HONEST | Constant-coefficient second-order DE unsupported. |

Next high-value implementation:
1. Add second-order linear constant-coefficient DE route.
2. Add transformed second-order route only if compact and general.
3. Continue paper audit in batches, prioritising DE, integration, implicit/parametric, trig solves, stats tests.
