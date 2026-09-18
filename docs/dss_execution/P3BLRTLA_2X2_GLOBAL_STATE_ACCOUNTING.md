# P3-BL-RTL-A — logical state accounting

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
CANDIDATE_MAP_BITS: 480
CANDIDATE_PRODUCER_CONTROL_BITS: 7
GLOBAL_DFS_STATE_BITS: 574
SELECTED_TUPLE_BITS: 112
COMMIT_SHADOW_STATE_BITS: 28
COMMIT_CONTROL_BITS: included above
PERSISTENT_LEDGER_BITS: 16
OTHER_CONTROL_RESULT_BITS: 9
TOTAL_LOGICAL_STATE_BITS: 2042
```

The DFS value includes its required independent 480-bit captured map. Producer and DFS maps coexist during search, so they are deliberately not collapsed in total state accounting.
