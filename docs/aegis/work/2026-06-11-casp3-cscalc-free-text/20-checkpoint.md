# Checkpoint

## 2026-06-12 CSCALC SQL Query Trace Slice

Completed:
- Added `sqlselect(...)`, `selectwhere(...)`, and `sqlquery(...)` for SELECT/FROM/WHERE working.
- Added `sqlcount(...)`, `countwhere(...)`, and `countrecords(...)` for COUNT(*) query working.
- Added free-text routing for SQL select/count/where prompts, including "greater than" and "at least" wording.
- Output shows clause order and the final query.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 database/query working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Equilibrium Components Slice

Completed:
- Added `equilibrium(...)`, `forcebalance(...)`, and `balanceforces(...)` for forces supplied as component pairs.
- Added `equilpolar(...)`, `forcepolar(...)`, and `balancepolar(...)` for forces supplied as magnitude/angle pairs.
- Added free-text routing for equilibrium/balance prompts with component or angle wording.
- Added degree-angle snapping for common force resolution angles so 0, 90, 180, and 270 degrees do not suffer from polynomial trig approximation drift.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Variable Acceleration Slice

Completed:
- Added `varacct(...)` / `variableacct(...)` for acceleration as a quadratic function of time.
- Added `varaccx(...)` / `variableaccx(...)` for acceleration as a quadratic function of displacement using `a = v dv/dx`.
- Added label-based free-text routing for variable acceleration coefficient prompts.
- Output shows the selected method, integration setup, substitution, velocity, and displacement where relevant.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Explicit Tree Traversal Slice

Completed:
- Added explicit binary-tree traversal commands using `root,node,left,right` triples.
- Supports `preordertree(...)`, `inordertree(...)`, and `postordertree(...)`.
- Free-text tree prompts with root/left/right/triple wording now route to explicit child-link traversal instead of assuming complete level order.
- Kept existing complete-tree level-order traversal unchanged.
- Added regression tests for direct and free-text diagram-style inputs.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm/trace working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC FSM Trace Slice

Completed:
- Added direct finite state machine tracing with `fsm(start,input,state,symbol,next,...)`.
- Added Mealy-style output tracing with `fsmout(start,input,state,symbol,next,output,...)`.
- Added free-text routing for finite-state-machine prompts.
- Output shows start state, each consumed input symbol, each transition, final state, and output stream where supplied.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 trace-working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Boolean Consensus Slice

Completed:
- Located the archived Python Boolean algebra implementation in `python/src/ComputerScience/booleanProgram.py` from commit `d9a86d0f173a39411a40dd4f150ee874aa251cd1`.
- Compared its documented law coverage with current CSCALC Boolean support.
- Added the missing consensus theorem route for expressions such as `AB + A'C + BC`.
- Kept the existing truth-table equivalence guard before displaying the law step.
- Added direct and free-text regression tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra working support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Grouped Data Slice

Completed:
- Added `groupmean(...)`, `groupedmean(...)`, and `groupstats(...)`.
- Supports grouped mean and standard deviation from midpoint/frequency pairs.
- Added free-text dispatch for grouped-data prompts with midpoints and frequencies.
- Output shows `sum f`, `sum fx`, mean, `sum fx^2`, and standard deviation formula.
- Added a Grouped data help page in CASP3.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics working support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Sort Trace Slice

Completed:
- Added `selectionsort(...)` / `selection(...)` working traces.
- Added `mergesort(...)` / `merge(...)` working traces.
- Added free-text dispatch for selection-sort and merge-sort prompts.
- Output shows the sorting rule and intermediate list states.
- Added sort commands to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Dijkstra Slice

Completed:
- Added `dijkstra(...)`, `shortestpath(...)`, and `shortpath(...)` working traces.
- Supports `dijkstra(start,end,from,to,weight,...)` with compact undirected edge triples.
- Added free-text dispatch for shortest-path route prompts.
- Output shows the Dijkstra rule, fixed nodes, distance updates, final path, and final distance.
- Added Dijkstra line to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Tree Traversal Slice

Completed:
- Added `preorder(...)`, `inorder(...)`, and `postorder(...)` working traces for complete binary trees supplied in level order.
- Added aliases `treepre(...)`, `treein(...)`, `treepost(...)`, `pretraverse(...)`, `intraverse(...)`, and `posttraverse(...)`.
- Added free-text dispatch for tree traversal prompts.
- Output states the level-order assumption, traversal rule, and final visit order.
- Added tree traversal lines to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 data structure traversal support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CASP3 Spearman Slice

Completed:
- Added `spearman(...)` / `spearmanrank(...)` / `rankcorr(...)` working.
- Added free-text/labeled dispatch for Spearman rank correlation using `n` and `sumd2`.
- Output shows the formula, substitution, and final `r_s`.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 statistics support.
- Did not touch CAS Pure behavior, CSCALC behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Stack Queue Trace Slice

Completed:
- Added `stack(...)` / `stacktrace(...)` / `pushpop(...)` working traces.
- Added `queue(...)` / `queuetrace(...)` / `enqueue(...)` working traces.
- Added free-text dispatch for stack push/pop and queue enqueue/dequeue prompts.
- Output shows LIFO/FIFO rule, each operation, current state, and underflow/overflow guards.
- Added stack/queue commands to the Algorithms help page and supported-command summary.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed after replacing calculator-incompatible `snprintf` with `sprintf`.

Drift check:
- Still inside CSCALC AQA Paper 2 data structure trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Search Sort Trace Slice

Completed:
- Added `binarysearch(...)`, `linearsearch(...)`, `bubblesort(...)`, and `insertionsort(...)` working traces.
- Added free-text dispatch for binary/linear search and bubble/insertion sort prompts.
- Output shows comparisons, found position, sort passes, or insertion steps.
- Added an Algorithms help page in the CSCALC menu.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 algorithm trace support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC ASCII Unicode Slice

Completed:
- Added ASCII/Unicode code conversion working for `ascii(...)`, `charcode(...)`, `unicode(...)`, and free-text prompts.
- Numeric code inputs show character/code point, denary, binary, and hex.
- Free-text prompts such as `ASCII code for A` preserve uppercase case.
- Added character-storage menu help for ASCII code conversion.
- Added regression tests for ASCII numeric, ASCII free-text, and Unicode code point conversion.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 character/data representation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, or shared UI/status code.

## 2026-06-12 CSCALC Hash Linear Probe Slice

Completed:
- Added `hashlinear(...)` / `linearprobe(...)` / `hashprobe(...)` for hash table insertion with linear probing.
- Output shows modulo address, occupied slots/probe count, and final placement.
- Added free-text dispatch for prompts mentioning hash table linear probing.
- Added regression tests for direct and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 hash table calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC RPN Slice

Completed:
- Added `rpn(...)` / `postfix(...)` / `reversepolish(...)` working for Reverse Polish notation.
- Output shows stack pushes, each operation, and final answer.
- Added free-text parsing for prompts such as `evaluate RPN 3 4 + 5 *` and word operators such as `divide` / `plus`.
- Added regression tests for direct and free-text RPN inputs.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Huffman Slice

Completed:
- Added `huffman(...)` / `huff(...)` / `huffmancode(...)` working for frequency tables.
- Output shows lowest-weight merges, code lengths, encoded bit total, and fixed-length comparison.
- Added free-text dispatch for prompts such as `huffman frequencies 45 13 12 16 9 5`.
- Added regression tests for direct command and free-text forms.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 compression calculation support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Boolean Parser Slice

Completed:
- Fixed Boolean expression evaluation so `AND` parsing always consumes the RHS factor.
- This fixes direct SOP forms such as `A&B'+A'&B` and implicit forms such as `A'B+AB'`.
- Added regression coverage for both explicit and implicit XOR/SOP expressions.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Boolean Law Guard Slice

Completed:
- Compared the old Boolean helper with current `CSCALC` support.
- Added a truth-table equivalence guard before Boolean algebra law steps are displayed.
- Added bounded repeated Boolean-law trace output instead of a single unchecked line.
- Fixed a class of bad output where De Morgan's law could be shown for only part of a larger expression.
- Added regression coverage for `bool(not(A and B)+A)` so non-equivalent law fragments are rejected.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure behavior, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Impact Solve Slice

Completed:
- Added `impactsolve(...)` / `collisionsolve(...)` / `restitutionsolve(...)` for final velocities from masses, initial velocities, and coefficient of restitution.
- Output shows conservation of momentum, restitution equation, substitution, and final velocities.
- Added labelled free-text dispatch for `m1=... u1=... m2=... u2=... e=...`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Address Space Slice

Completed:
- Added `addressspace(...)` / `addresses(...)` / `addressbus(...)` working for addressable-location counts.
- Added `memorycapacity(...)` / `addresscapacity(...)` / `memorybus(...)` working for address bus plus word-size capacity.
- Added free-text dispatch for address bus/location/memory capacity prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Normal Variance Slice

Completed:
- Added `normalvar(...)` / `normalzvar(...)` / `zscorevar(...)`.
- Added interval, tail, and inverse-normal variance variants.
- Added free-text dispatch so prompts with `variance` convert to `sigma=sqrt(variance)` before using NormalCD/InvNorm working.
- Added regression tests covering standardising, interval, tail, and inverse-normal prompts with variance.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Projectile Height Slice

Completed:
- Added `projectileh(...)` / `projectileheight(...)` / `projheight(...)` for projectiles launched from a height.
- Added free-text dispatch before the same-height projectile fallback when prompts mention height/above.
- Output now shows resolving, vertical equation, time from the quadratic, and horizontal range.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Conservation Of Momentum Slice

Completed:
- Added `momentum(...)` / `momcons(...)` / `consmomentum(...)` working for two-particle conservation of linear momentum.
- Added `commonvelocity(...)` / `coalesce(...)` / `stick(...)` working for particles moving together after impact.
- Added labelled free-text dispatch for `m1=... u1=... m2=... u2=... v1=...`.
- Fixed `momentum` being misrouted as `moment`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Hash Table Slice

Completed:
- Added `hashmod(...)` / `hashtable(...)` / `modhash(...)` working for hash address calculation.
- Added collision notes when two keys map to the same table address.
- Added free-text dispatch for hash-table prompts such as `hash table size 10 keys 27 18 29 37`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 SUVAT Free-Text Bridge Slice

Completed:
- Broadened the labelled SUVAT parser so constant-acceleration/velocity/distance/time wording works without the literal word `suvat`.
- Added tests for natural labelled constant-acceleration prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 mechanics free-text support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Bitwise Operation Slice

Completed:
- Added `xorbits(...)`, `andbits(...)`, `orbits(...)`, `notbits(...)` / `invertbits(...)`.
- Added free-text dispatch for bitwise XOR/AND/OR/NOT while avoiding broad `and` matches in ordinary wording.
- Added tests for direct commands and natural XOR/NOT prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Setup Formula Slice

Completed:
- Added `resolve(...)` / `componentsfromforce(...)` / `forcecomponents(...)` for force components.
- Added `stratified(...)` / `stratifiedsample(...)` / `stratum(...)` for stratified sample-size working.
- Added free-text dispatch for force-component and stratified-sampling prompts.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 setup/method support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, UI/menu/status code.

## 2026-06-12 CSCALC Error Detection Slice

Completed:
- Added `hamming(...)` / `hammingdistance(...)` / `bitdiff(...)` working.
- Added `checksum(...)` / `checksummod(...)` / `binarychecksum(...)` working.
- Added free-text dispatch for Hamming distance and checksum prompts before generic binary addition.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-12 CASP3 Discrete Random Variable Slice

Completed:
- Added `discrete(...)` / `expectation(...)` / `randomvar(...)` working from `(x,p)` pairs.
- Added free-text parsing for value/probability rows, e.g. `values 0 1 2 probabilities 0.2 0.5 0.3`.
- Output now shows probability sum, `E(X)`, `E(X^2)`, and `Var(X)` lines.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Paper 3 stats support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Bayes / Independence Slice

Completed:
- Added `bayes(...)` / `bayestheorem(...)` / `reverseconditional(...)` working.
- Added `independent(...)` / `independence(...)` / `testindependent(...)` working.
- Ordered free-text dispatch so reverse-conditional/Bayes prompts are not stolen by generic conditional probability.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 probability free-text/method support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 Labelled Stats / Boolean Proof Slice

Completed:
- Added free-text Boolean identity proof parsing, e.g. `prove A+B = B+A`.
- Added labelled PMCC parsing for raw sums: `n=`, `sx=`, `sy=`, `sxy=`, `sx2=`, `sy2=`.
- Added PMCC summary-value route using `Sxy/sqrt(Sxx*Syy)`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py`: passed.
- `./compile`: passed.

Drift check:
- Still serves CASP3/CSCALC arbitrary free-text goal.
- Stayed inside shared engine/test/build files.
- No UI/menu/status changes.

Next:
- Add labelled parsing for more Paper 3 and CS past-paper phrasings.
- Add more Boolean law working, not only truth-table proof.

## 2026-06-11 Distribution Tail Slice

Completed:
- Added generic binomial tail working for `at least`, `more than`, `less than`, and `at most`.
- Added Poisson cumulative/tail working for matching free-text phrasings.
- Updated CASP3 supported-command text and tests.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 stats free-text hardening.
- No CAS pure/menu/UI change.

## 2026-06-11 Inverse Normal Slice

Completed:
- Added `invnormal(area,mu,sigma)` / `normalinv` / `inversenormal` setup route.
- Added free-text parsing for normal critical-value / percentile phrasing.
- Output gives exam setup and fx-CG50 `InvNorm(area, sigma, mu)` input.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 stats free-text/method support.
- No UI/menu/status changes.

## 2026-06-11 Boolean Law Step Slice

Completed:
- Read the old Python Boolean program from git history.
- Added compact Boolean algebra step output before truth-table/minterm simplification for common laws:
  idempotent, complement, identity, dominance, absorption, double complement, and De Morgan.
- Broadened free-text Boolean dispatch so symbolic `+` / `*` expressions route correctly.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC Boolean algebra/free-text goal.
- Did not touch UI/menu/status code.

## 2026-06-11 Floating Point Normalisation Slice

Completed:
- Added CSCALC `floatnorm` / `fpnorm` / `normalise` / `normalize` route for AQA mantissa/exponent normalisation.
- Added `floatprecision` / `fpprecision` / `floatstep` route for representable step-size working.
- Added natural-language triggers for normalisation, decimal-to-floating encode, and precision questions.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation scope.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Normal Tail/Test Slice

Completed:
- Added CASP3 `normaltail` / `normalupper` / `normaltp` route with z-standardisation and fx-CG50 NormalCD setup.
- Added `hypnormal` / `normaltest` / `hypmean` route with H0/H1, standard error, z statistic, alpha comparison, and context conclusion prompt.
- Added natural-language parsing for normal upper/lower tail and normal hypothesis-test wording.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 method support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Regression Summary Slice

Completed:
- Added CASP3 least-squares regression route from summary values:
  `regresscalc(n,Sx,Sy,Sxx,Sxy[,x])`.
- Added route from means and corrected sums:
  `regresss(xbar,ybar,Sxx,Sxy[,x])`.
- Added free-text labelled parsing for regression summaries and predictions.
- Hardened label parsing in CASP3 and CSCALC so short labels do not match inside longer labels, e.g. `x` inside `sx`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3/CSCALC free-text arbitrary-input hardening.
- Did not touch CAS Pure, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC RLE Sequence Slice

Completed:
- Added `rletext(sequence,symbolBits,countBits)` / `rlestring` / `runencode`.
- Added run summary output from raw sequence, original bits, and encoded bits.
- Added free-text extraction for run-length encoding questions containing a repeated-symbol sequence.
- Preserved existing numeric `rle(runs,symbolBits,countBits)` route.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC Character Set Slice

Completed:
- Added `charset(characters,characterSetSize)` / `charsetsize` / `textsymbols`.
- Added working for bits per character from `ceil(log2(character set size))`.
- Added free-text parsing for character-set/alphabet/symbol storage questions.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 storage-calculation support.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Mechanics Free-Text Slice

Completed:
- Added free-text dispatch for impulse/momentum-change questions.
- Added free-text dispatch for variable-acceleration integration questions.
- Hardened connected-particles parsing when the prompt contains spaced words.
- Prevented generic force parsing from stealing connected-particle prompts.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 mechanics support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC Closest Float Slice

Completed:
- Added `floatnearest(value,mantissaBits,exponentBits)` / `fpnearest` / `closestfloat`.
- Added working for normalising the value, finding the step at that exponent, rounding to nearest representable multiple, and emitting mantissa/exponent bits.
- Added free-text parsing for closest/nearest representable floating-point questions.
- Ordered free-text dispatch so `representable` does not get stolen by floating-point encode.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 floating-point calculation support.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 CASP3 Binomial Normal Approx Slice

Completed:
- Added `binomnorm(n,p,lower,upper)` / `normalapproxbinom` / `binomnormal`.
- Added mean, standard deviation, continuity-correction, z-value, and NormalCD setup working.
- Added free-text parsing for normal approximation to binomial prompts.
- Ordered dispatch so normal-between prompts do not steal binomial-normal approximation prompts.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC, NOTES, UI/menu/status code.

## 2026-06-11 CSCALC Boolean Distributive Slice

Completed:
- Added named Boolean algebra working for common-factor/distributive simplification, e.g. `AB+AC -> A(B+C)`.
- Preserved the existing truth-table/minterm simplification fallback.
- Added a golden test for spaced free-text style Boolean input.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure, CASP3, NOTES, UI/menu/status code.

## 2026-06-11 Distribution Approx / Fixed Point Encode Slice

Completed:
- Added CASP3 Poisson approximation to binomial route:
  `poissonapprox(n,p,r,tail)` / free text such as `poisson approximation to binomial n 200 p 0.01 at most 3`.
- Added CSCALC fixed-point encoding routes:
  `fixedenc(value,wholeBits,fractionBits)` and signed `fixedtcenc(value,wholeBits,fractionBits)`.
- Added free-text fixed-point encode parsing before fixed-point decode so decimal prompts do not get stolen by binary decode.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 78048 bytes`
  - `CSCALC.g3a: 80348 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3/CSCALC free-text arbitrary-input hardening.
- Did not touch CAS Pure behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Transfer Unit Slice

Completed:
- Added explicit `transfermb(sizeMB,rateMbitPerSecond)` and `transferkb(sizeKB,rateKbitPerSecond)` routes.
- Added free-text handling for megabyte/megabit and kilobyte/kilobit transfer-time prompts.
- Added tests that catch the common wrong `10/2=5s` route for `10 megabytes at 2 megabits per second`; expected method converts to `80 Mbit`, then `40s`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 78048 bytes`
  - `CSCALC.g3a: 81264 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Boolean Product-of-Sums Slice

Completed:
- Added Boolean algebra law working for product-of-sums common-term simplification:
  `(A+B)(A+C) -> A+BC`.
- Kept truth-table/minterm simplification as the fallback after the algebra line.
- Added a golden test for `bool((A+B)(A+C))`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 78048 bytes`
  - `CSCALC.g3a: 81556 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Coded Statistics Slice

Completed:
- Added coded-statistics transforms:
  `uncode(meanY,sdY,a,b)` for `Y=(X-a)/b`, and `code(meanX,sdX,a,b)`.
- Added free-text route for prompts like `coded data y=(x-20)/5 mean 3 sd 2`.
- Fixed free-text sign handling so `x-20` gives `a=20`, not `a=-20`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 79328 bytes`
  - `CSCALC.g3a: 81556 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Parity Bit Slice

Completed:
- Added `parity(bits,even|odd)` / `paritybit(...)` route.
- Added free-text handling for even/odd parity-bit prompts.
- Output counts one-bits, states parity rule, and gives transmitted bits.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 79328 bytes`
  - `CSCALC.g3a: 81984 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Poisson Normal Approximation Slice

Completed:
- Added `poissonnorm(lambda,lo,hi)` / `normalapproxpoisson(...)` / `poissonnormal(...)`.
- Added free-text route for prompts like `normal approximation to poisson mean 64 between 55 and 70`.
- Output shows model choice, `mu`, `sigma`, continuity correction, both z-values, and the fx-CG50 `NormalCD` input.
- Added `poissonnorm` to the supported-command line.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 80080 bytes`
  - `CSCALC.g3a: 81984 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Poisson Hypothesis Slice

Completed:
- Added shared stable Poisson probability helper using scaled `e^-lambda` and recurrence, replacing the weak short alternating exponential series.
- Added `critpoisson(lambda,alpha,tail)` / `criticalpoisson(...)` / `poissoncrit(...)`.
- Added `hyppoisson(lambda,x,alpha,tail)` / `poissontest(...)` / `poissonhyp(...)`.
- Added free-text routes for Poisson critical-region and hypothesis-test prompts.
- Added regression coverage for large-lambda Poisson probability output.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 81736 bytes`
  - `CSCALC.g3a: 81984 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC Check Digit Slice

Completed:
- Added generic weighted modulo check-digit working:
  `checkdigit(digits,modulus,w1,w2,...)` / `modcheck(...)` / `weightedcheck(...)`.
- Added free-text handling for prompts like `check digit for 12345 weights 6 5 4 3 2 mod 11`.
- Output shows weighted-sum method, remainder, and final check digit.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 81736 bytes`
  - `CSCALC.g3a: 82736 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 calculation support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Grouped Interpolation Slice

Completed:
- Added grouped-data interpolation working for median:
  `groupmedian(L,cfBefore,f,width,n)`.
- Added grouped quantile interpolation:
  `groupquantile(L,cfBefore,f,width,n,q)`.
- Added free-text handling for grouped median/quartile prompts.
- Output shows position, interpolation formula, substitution, and final value.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 83688 bytes`
  - `CSCALC.g3a: 82736 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CASP3 Histogram Density Slice

Completed:
- Added histogram frequency-density working:
  `histdensity(frequency,width)` / `frequencydensity(...)`.
- Added reverse route:
  `histfreq(density,width)` / `frequencyfromdensity(...)`.
- Added free-text handling for simple histogram density prompts.
- Output shows formula, substitution, and value.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_p3_engine.py`: passed.
- `python3 tests/check_cscalc_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 84512 bytes`
  - `CSCALC.g3a: 82736 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CASP3 Edexcel Paper 3 statistics support.
- Did not touch CAS Pure, CSCALC behavior, NOTES, menus, or status/UI code.

## 2026-06-11 CSCALC XOR Algebra Slice

Completed:
- Added Boolean algebra working line for XOR:
  `A xor B -> A'B + AB'`.
- Kept existing truth-table simplification as the final confirmation.
- Added tests for text `xor` and symbolic `^`.
- Rebuilt all calculator files.

Evidence:
- `python3 tests/check_cscalc_engine.py`: passed.
- `python3 tests/check_p3_engine.py && python3 tests/check_multi_app_suite.py`: passed.
- `python3 tools/check_catalog_scope.py && python3 tools/check_removed_features.py && git diff --check`: passed.
- `./compile`: passed.
- Size/hash evidence:
  - `CAS.g3a: 2097100 bytes`
  - `CASP3.g3a: 84512 bytes`
  - `CSCALC.g3a: 82852 bytes`
  - `NOTES.g3a: 46952 bytes`

Drift check:
- Still inside CSCALC AQA Paper 2 Boolean algebra support.
- Did not touch CAS Pure, CASP3 behavior, NOTES, menus, or status/UI code.
