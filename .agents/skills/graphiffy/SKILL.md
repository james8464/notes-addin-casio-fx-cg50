---
name: graphiffy
description: Use for diagrams, architecture, data flow, dependency maps, state machines, API flows, database schemas, and any explanation that can be compressed into graph syntax.
---

Use Graphiffy/Mermaid-style compact diagrams to save tokens.

Default syntax:

graph TD
  A --> B
  B --> C

Rules:
- Prefer diagrams over paragraphs for structure.
- Use short node names.
- Add prose only when the diagram is ambiguous.
- For workflows use graph TD.
- For sequences use sequenceDiagram.
- For states use stateDiagram-v2.
- For entities use erDiagram.
- Keep labels short.
- Do not over-explain the diagram.
