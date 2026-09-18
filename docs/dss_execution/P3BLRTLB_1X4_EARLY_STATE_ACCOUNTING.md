# P3-BL-RTL-B — 1x4 EARLY state accounting

| Registered state | Bits |
|---|---:|
| Four six-pivot, ten-hybrid snapshots | 1116 |
| Shared-producer control and candidate/demand table | 1087 |
| EARLY controller, selected result, and row ledger | 110 |
| Top handoff control | 4 |
| **Total logical retained state** | **2317** |

```text
GROUP_SNAPSHOT_STORAGE_BITS: 1116
ANALYZER_STATE_BITS: 0 (combinational)
CANDIDATE_PRODUCER_STATE_BITS: 1087
EARLY_CONTROLLER_STATE_BITS: 20
ROW_LEDGER_STATE_BITS: 24
ROW_OWNER_STATE_BITS: 16 (structural, not additional registers)
SELECTED_RESULT_STATE_BITS: 64
CONTROL_RESULT_BITS: 4
TOTAL_LOGICAL_STATE_BITS: 2317
```

This is architectural sequential storage, excluding combinational analyzer,
legality, and next-state wires. It is not a synthesis-cell-area claim.
