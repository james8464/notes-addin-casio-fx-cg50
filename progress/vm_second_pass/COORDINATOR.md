# VM second-pass coordinator

**Status:** Stand down — do not duplicate work.

The original single-agent second pass (`b377d0a5`) was launched to re-audit all VM sources. That run is superseded by **22 parallel workers** that now own the second pass.

## Ownership

| Workers | Scope |
|--------|--------|
| 01–02 | MadAsMaths papers c1_a–c1_z |
| 03–04 | MadAsMaths papers c2_a–c2_z |
| 05–06 | MadAsMaths papers c3_a–c3_z |
| 07–08 | MadAsMaths papers c4_a–c4_u |
| 09–12 | MadAsMaths papers mp1, mp2 |
| 13–14 | MadAsMaths papers syn |
| 15 | Edexcel Pure Paper 01 + SAMs / support materials |
| 16–19 | MadAsMaths A-level booklets (alphabetical batches) |
| 20–22 | MadAsMaths standard topics (integration, trigonometry, various) |

Each worker writes `progress/vm_second_pass/worker_XX.md` and appends any missed testable inputs to the golden queue.

## Coordinator action

- **Stopped** full-VM scan here to avoid overlap with workers 01–22.
- **Do not** re-run the monolithic second pass from `b377d0a5`.
- Consolidate findings from worker reports when all workers finish.

**Updated:** 2026-05-31
