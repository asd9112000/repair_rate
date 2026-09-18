# P3-BL-RTL-A — logical state accounting

```text
P3BLRTLA_STATUS: COMPLETE
ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
CLOSURE_COMMIT: ffe2a97
```

| State class | Bits | Basis |
|---|---:|---|
| Group snapshot capture | 816 | 4 × 204-bit logical collector snapshots |
| Candidate producer control | 7 | FSM, SA, action, done |
| Producer candidate map | 480 | valid/release/borrow, 160 bits each |
| GLOBAL DFS captured candidate map | 480 | existing OPT0/OPT1 core raw history |
| GLOBAL DFS control/stack | 94 | cursors, depth/FSM, four resource snapshots, result flags/masks |
| Selected tuple state | 112 | 56 search-core bits plus 56 post-commit publication bits |
| Commit shadow/control | 28 | shadow ledger, obligation/count, stage/FSM, acknowledgements |
| Persistent ledger | 16 | released, borrowed, borrower IDs |
| Other top control/result | 9 | integration state/pulses/result flags |
| **Total logical state** | **2042** | architectural state; not synthesized area |

```text
GROUP_SNAPSHOT_CAPTURE_BITS: 816
CANDIDATE_PRODUCER_STATE_BITS: 7
CANDIDATE_MAP_STATE_BITS: 480
GLOBAL_DFS_STATE_BITS: 574
SELECTED_TUPLE_STATE_BITS: 112
COMMIT_SHADOW_STATE_BITS: 28
PERSISTENT_LEDGER_BITS: 16
CONTROL_RESULT_BITS: 9
TOTAL_LOGICAL_STATE_BITS: 2042

816 + 7 + 480 + 574 + 112 + 28 + 16 + 9 = 2042
```

The DFS value includes its required independent 480-bit captured map. Producer and DFS maps coexist during search, so they are deliberately not collapsed in total state accounting.

This is registered sequential logical storage only: the 2042-bit total excludes
transient combinational analyzer, transition, DFS-legality, and shadow-next
signals. It is architectural accounting, not synthesized area.
