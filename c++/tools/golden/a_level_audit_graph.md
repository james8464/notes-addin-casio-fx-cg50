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
  X --> V["check_a_level_source_downloads.py"]
  OX --> OV["check_online_paper_corpus_inventory.py"]
  P --> E["Edexcel corpus checks"]
  P --> S["Madas scan checks"]
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
```
