# P2DOM RTL critical-path audit plan

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

Classify every worst mapped path as exactly one of: `summary builder`,
`candidate-class selection`, `DFS cursor/control`, `dominance comparator`,
`failed-state cache lookup`, `speculative state update`, `selected-tuple output`,
or `other`. If hierarchy is insufficient, record `UNRESOLVED` with exact
startpoint, endpoint, hierarchy, and timing-report reference.

| Exp | Retained / expected classification | Evidence | Status |
|---|---|---|---|
| A | selected-tuple output/control boundary; exact classification not redone | ledger path `depth_q_reg[0]` to `selected_release_q_reg[3]`, 41 levels | retained |
| B | candidate-class selection or DFS cursor/control | future timing report | PLANNED |
| C S1 | summary builder or candidate-class selection | future timing report | PLANNED |
| C/D S2 | builder control during build; DFS/control or lookup during search | separate mode reports | PLANNED |
| D/E | dominance comparator/cache lookup is a stop-condition risk | future timing report | PLANNED |

If cache comparison dominates the critical path, stop after recording evidence;
do not grow the cache or claim a PPA win without an explicit architecture decision.
