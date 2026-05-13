# Past Paper Audit

Sources:
- PMT A Level Edexcel papers: https://www.physicsandmathstutor.com/maths-revision/a-level-edexcel/papers/
- PMT Further Maths Edexcel papers: https://www.physicsandmathstutor.com/maths-revision/a-level-edexcel/papers-further/
- MadAsMaths Integration: https://www.madasmaths.com/archive_maths_booklets_standard_topics_integration.html
- MadAsMaths Trigonometry: https://www.madasmaths.com/archive_maths_booklets_standard_topics_trigonometry.html
- MadAsMaths Various: https://www.madasmaths.com/archive_maths_booklets_standard_topics_various.html

Corpus:
- PDFs/text are ignored under `c++/tests/reports/pmt_pdf_corpus/`.
- MadAsMaths topic PDFs/text are ignored under `c++/tests/reports/madasmaths_topic_corpus/`.
- Audit rows live in `past_paper_audit.csv`.

Status:
- 217 PDFs downloaded/extracted.
- 62 MadAsMaths topic PDFs downloaded/extracted from the 3 requested pages.
- 18 question-parts manually checked against answer/model text.
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
| MADAS-ST-TRIGEQ-Q1D | PASS | Generic tan/cot power substitution fixed `8*tan(phi)=cot(phi)^2`. |
| MADAS-ST-TRIGEQ-Q2D | GAP | Need generic `sin/cosec` power substitution. |
| MADAS-ST-TRIGEQ-Q3D | GAP | Need `sec(2x)` with `cos(2x)=2cos(x)^2-1`. |
| MADAS-ST-TRIGEQ-Q5A-D | GAP | Need common-denominator reciprocal trig polynomial route. |

Next high-value implementation:
1. Add reciprocal trig polynomial route for `sin/cosec/sec/cot/tan` mixed equations.
2. Add second-order linear constant-coefficient DE route.
3. Add transformed second-order route only if compact and general.
4. Continue paper audit in batches, prioritising DE, integration, implicit/parametric, trig solves, stats tests.
