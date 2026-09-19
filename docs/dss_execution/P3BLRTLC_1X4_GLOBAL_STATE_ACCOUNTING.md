# P3BLRTLC 1x4 GLOBAL state accounting

This audit counts registered logical state only.  It excludes combinational
candidate evaluation, temporary loop variables, synthesis mapping, and area.

| State block | Bits |
| --- | ---: |
| Four independent input snapshots | 1116 |
| Reused sequential candidate producer | 1087 |
| Captured candidate table | 1080 |
| DFS control, cursors, and counters | 94 |
| Four speculative prefix ledgers | 124 |
| Current prefix selected tuple | 64 |
| Best result tuple and final ledger | 96 |
| Atomic commit shadow state | 32 |
| Persistent physical ledger | 24 |
| Top selected tuple result | 71 |
| Top controller/result state | 8 |
| **Total registered logical state** | **3796** |

```text
GROUP_SNAPSHOT_STATE_BITS: 1116
PRODUCER_STATE_BITS: 1087
CANDIDATE_TABLE_STATE_BITS: 1080
DFS_CONTROL_STATE_BITS: 94
SPECULATIVE_LEDGER_STATE_BITS: 124
CURRENT_PREFIX_TUPLE_STATE_BITS: 64
BEST_TUPLE_STATE_BITS: 96
COMMIT_SHADOW_STATE_BITS: 32
PERSISTENT_LEDGER_STATE_BITS: 24
SELECTED_TUPLE_STATE_BITS: 71
CONTROL_RESULT_BITS: 8
TOTAL_REGISTERED_LOGICAL_STATE_BITS: 3796
```

The 24-bit ledger is eight 3-bit row owners.  Owner `0..3` represents A..D and
owner `4` is unassigned.  The shadow ledger is private through the four staged
commit applications; persistent state changes only at publish.
