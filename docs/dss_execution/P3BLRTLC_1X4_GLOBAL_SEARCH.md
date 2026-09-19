# P3BLRTLC 1x4 GLOBAL search

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
SEARCH_ORDER: A_TO_B_TO_C_TO_D
PERSISTENT_STATE_UPDATED_DURING_DFS: NO
OBJECTIVE_1: MINIMIZE_BORROWED_ROWS
OBJECTIVE_2: MINIMIZE_USED_ROWS
OBJECTIVE_3: LEXICOGRAPHIC_PATTERN_TUPLE
OBJECTIVE_4: LEXICOGRAPHIC_ATTEMPT_TUPLE
```

The implementation follows the C++ `findCompressedGroupChoice` policy:
each depth evaluates the candidates for one SA against its private prefix
ledger.  A row uses its local SA line first; a borrowed line must be physically
unassigned and owned by one of the explicit adjacent SAs.  A borrowed line is
therefore unavailable to later depths, which also prevents forwarding.

Candidate reduction is limited to an exact C++-safe equivalence class.  When
two valid candidates have the same `(usedRows, usedColumns)`, the lower
`(PatternID, attemptIndex)` is retained.  No 2x2 action/effect pruning or
other semantic compression is applied.

**P3-4PT classification note:** this closed functional behavior is now exposed
as the separate SYN-D OPT1 elaboration. The same externally visible semantics
remain valid; the current OPT0 top instead evaluates every valid candidate.
The proof, direct OPT0↔OPT1 comparison, and current synthesis selection are in
`P3_4PT_GLOBAL_OPT_AUDIT.md`.

The shared deterministic C++-to-RTL candidate corpus contains 1,000 cases with
seed `20260921`.  It compares repairability, selected attempt/pattern tuple,
objective fields, donors, and final row ownership.  The same corpus requires an
EARLY-fail/GLOBAL-success witness.

```text
CPP_ORACLE_RANDOM_CASES: 1000
CPP_ORACLE_SEED: 20260921
EARLY_GLOBAL_DISTINCTION_TEST: PASS
SELECTED_TUPLE_MISMATCHES: 0
OBJECTIVE_MISMATCHES: 0
FINAL_OWNER_MISMATCHES: 0
GLOBAL_ADJACENCY_MISMATCHES: 0
BORROWED_LINE_FORWARDING_VIOLATIONS: 0
CPP_ORACLE_MISMATCHES: 0
```
