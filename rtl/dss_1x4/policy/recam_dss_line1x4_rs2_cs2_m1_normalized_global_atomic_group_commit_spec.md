# 1x4 Single-Hop GLOBAL atomic-commit specification

The commit unit accepts one complete selected A→B→C→D tuple only when its
persistent eight-row ledger is pristine.  It copies that ledger into shadow,
replays one SA per stage with explicit A-B-C-D adjacency, compares the computed
donor and final assignment against the search result, then publishes once.

```text
TIMING: start initialization -> four stage edges -> one publish edge
PERSISTENT_VISIBILITY: publish edge only
ERROR_BEHAVIOR: discard shadow and retain persistent rows
VERIFICATION: atomic success, donor mismatch, no-forwarding, no partial visibility
```
