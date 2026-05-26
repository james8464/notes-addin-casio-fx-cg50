# A-Level Audit Flow

```mermaid
graph TD
  E1["Pearson 9MA0 public"] --> D["download_a_level_audit_sources.py --scope all --clean"]
  E2["Pearson SAM/model answers"] --> D
  E3["2025 official probes"] --> D
  M1["MadAsMaths broad packs"] --> D
  O1["online A-level corpus"] --> OD["download_online_paper_corpus.py --clean"]
  D --> P["PDF cache in Downloads"]
  D --> X["manifest_latest.jsonl"]
  OD --> OX["online manifest"]
  X --> XC["972 rows: Pearson 67 + Edexcel scrape 48 + MadAs 857"]
  OX --> OC["1221 indexed PDFs: 13 online sources"]
  X --> V["check_a_level_source_downloads.py"]
  OX --> OV["check_online_paper_corpus_inventory.py"]
  P --> E["Edexcel corpus checks"]
  E --> EQ["question-audit coverage"]
  P --> S["Madas scan checks"]
  S --> M["standard question corpus"]
  S --> MC["download coverage report"]
  P --> R["render_audit_pdf_pages.py --format jpeg"]
  R --> I["manual-review images"]
  I --> Q["manual Q review"]
  Q -->|supported| J["tracker/manual JSONL"]
  Q -->|unsupported/manual| U["tracker unsupported-ok"]
  J --> H["casio_host"]
  J --> K["check_a_level_audit_tracker.py"]
  H --> C["mark-scheme judgement"]
  C -->|pass| G["golden case"]
  C -->|fail| F["fix route family"]
  F --> T["focused + core gates"]
  T --> G
  G --> Z["delete audited PDFs/images"]
```

Latest refresh: 2026-05-25.

- `check_a_level_source_downloads.py`: `rows=972`, no failures.
- `check_edexcel_public_paper_corpus.py`: `54` official Pearson 9MA0 PDFs, `27` question papers + `27` mark schemes.
- `check_edexcel_question_audit_coverage.py`: `27` official question papers have tracker rows for all inferred questions.
- `check_online_paper_corpus_inventory.py`: `1222` indexed PDFs, `0` cached PDF files after cleanup, `1157` text extracts, `6766` question-marker hits, `11` skipped known-dead/non-paper links.
- `check_a_level_audit_tracker.py`: `1344` reviewed rows, `1159` host-pass, `185` unsupported-ok, `3009` host runs.
- `check_madasmaths_standard_question_corpus.py`: `4554` rows, `5896` manual cases, no failures.
- `check_madasmaths_download_coverage.py`: `462` downloaded MadAsMaths question PDFs, `226` covered, `236` still gap-listed for future manual audit.
- Current working help/templates are external in `c++/prizm/help/*.HLP/*.TPL` and packed to `CASIOCAS.PAK`; keep verbose help out of `.g3a`.
