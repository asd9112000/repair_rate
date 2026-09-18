# P2DOM RTL challenge and defense log

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

## C1 — 480 to 224 logical-summary claim

```text
CLAIM: Downstream search can use an upper-bound 224-bit representative summary.
EVIDENCE: P2DOM equivalence proof; 4 SAs * 8 classes * (presence + 6-bit identity).
ASSUMPTIONS: Existing map boundary; identity is canonical-rank/action-Pattern
bijective; ConfigID and donor remain derivable.
POSSIBLE_CHALLENGE: 224 is a minimum or immediate area reduction.
DEFENSE: It is sufficient only; S2 peak state still includes 480 raw bits.
SAFE_TO_CLAIM: CONDITIONAL
```

## C2 — 2,625,640 to 4,680 visits

```text
CLAIM: Safe collapse bounds semantic candidate evaluations by 4,680.
EVIDENCE: 8 + 8^2 + 8^3 + 8^4 and safe three-field key.
ASSUMPTIONS: Only first canonical representatives are evaluated.
POSSIBLE_CHALLENGE: RTL cycles fall by the same factor.
DEFENSE: Slot inspection, summary construction, and cycles are measured separately.
SAFE_TO_CLAIM: YES for evaluation bound; NO for cycle reduction.
```

## C3 — dominance frontier benefit

```text
CLAIM: A depth-local failed-state frontier may reduce average search.
EVIDENCE: P2DOM model: mean 19 to 10, P99 184 to 46, and 1,154 prunes.
ASSUMPTIONS: RTL implements only earlier-fully-failed cache entries.
POSSIBLE_CHALLENGE: Benefit survives comparator cost and all workloads.
DEFENSE: D records PPA; E starts only if D benefits.
SAFE_TO_CLAIM: CONDITIONAL
```

## C4 — summary-build overhead

```text
CLAIM: Multi-cycle construction may trade latency for a shorter critical path.
EVIDENCE: Architectural hypothesis only; no RTL/PPA evidence exists.
ASSUMPTIONS: Raw inputs are captured before they may change after start.
POSSIBLE_CHALLENGE: It reduces total latency or state in all modes.
DEFENSE: S2 records peak state and construction/DFS/total latency separately.
SAFE_TO_CLAIM: NO
```
