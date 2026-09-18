# P2DOM RTL architecture variants

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

All variants retain candidate-map boundary, A->B->C->D traversal, R,L,RB,B
then ascending PatternID order, directional mapping, future-owner obligations,
selected-tuple semantics, and speculative final state. The only safe key is:

```text
(explicit_release, actual_release, actual_borrow)
```

`(actual_release, actual_borrow)` alone is prohibited by the P2-DOM
counterexample.

## EXP-A — raw exhaustive baseline

Capture all 480 raw map bits and search 40 positions per depth. This is golden.

## EXP-B — raw-history class collapse

Retain 480 raw bits. Evaluate a candidate only when no earlier canonical slot
at that depth has the same safe key. The cursor may still inspect raw positions,
so candidate evaluations and RTL cycles are separately reported. B has no
retained summary: this isolates deterministic frontier reduction.

## EXP-C — fixed summary plus class collapse

Store at most eight entries per SA, each `presence + first six-bit canonical
identity`, for a maximum 224 bits. Canonical rank 0..39 is preferred because it
is bijective with action/PatternID and directly captures search order. DFS
selects the lowest remaining present rank. Raw history is not retained after
summary capture. 224 is a sufficient upper bound, not a proven minimum.

## EXP-D / EXP-E — failed-state dominance frontier

D adds one valid-plus-14-bit failed state per useful B/C/D depth. State is
`{released[3:0], used[3:0], release_req[3:0], borrow_count[1:0]}`. It becomes
valid only after an earlier same-depth subtree fully fails, and only prunes a
later dominated state. It clears on accepted start. E compares 2 then 4 entries
only if D has a positive measured trade-off; replacement policy is not frozen.

## Summary construction alternatives

| ID | Architecture | State implication | Status |
|---|---|---|---|
| S1 | start-time combinational extractor | no builder state; priority path risk | PLANNED |
| S2 | capture raw maps, scan 160 slots, launch summary DFS | peak has raw + summary + builder state | preferred first implementation; PLANNED |
| S3 | producer streams first representatives to summary | crosses producer/GLOBAL boundary | PLANNED_LAST |

S2 does not claim scratch reuse. S3 must not insert GLOBAL-specific class logic
into the generic analyzer without a separate reusable-common proof.
