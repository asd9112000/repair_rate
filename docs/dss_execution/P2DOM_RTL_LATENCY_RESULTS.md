# P2DOM RTL latency accounting

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

No optimized RTL is implemented, simulated, or synthesized in this phase. This
document freezes the later measurement contract.

| Event | Definition |
|---|---|
| candidate-ready | accepted `start_i` edge in IDLE; maps become core-owned |
| summary-ready | first edge where retained representatives are ready for DFS |
| search-start | first DFS evaluation after candidate/summary ownership |
| solution-ready | registered `done_o` edge with stable tuple or failure |

Every experiment reports:

```text
CANDIDATE_GENERATION_LATENCY: NOT_INCLUDED_IN_THIS_BOUNDARY
SUMMARY_CONSTRUCTION_LATENCY: candidate-ready -> summary-ready
DFS_SEARCH_LATENCY: search-start -> solution-ready
FINAL_TUPLE_PREPARATION_LATENCY: explicit value or 0 with source evidence
TOTAL_SOLUTION_LATENCY: candidate-ready -> solution-ready
```

Candidate evaluations are not RTL cycles. Baseline may advance across invalid
raw slots; class collapse can skip equivalent valid slots. Publish theoretical
worst, observed maximum/mean/median/P95/P99 evaluations, backtracks, equivalent
skips, dominance prunes, and the separate RTL-cycle distribution.

| Exp | Summary construction | DFS/search | Total latency | Status |
|---|---|---|---|---|
| A | 0 | NOT_MEASURED_BY_P2DOM | NOT_MEASURED_BY_P2DOM | retained semantic/PPA baseline |
| B | 0 retained summary | NOT_RUN | NOT_RUN | PLANNED |
| C S1 | combinational capture; cycles to measure | NOT_RUN | NOT_RUN | PLANNED |
| C/D S2 | sequential 160-slot scan plus control; exact cycles NOT_FROZEN | NOT_RUN | NOT_RUN | PLANNED |
| S3 | crosses producer boundary | NOT_RUN | NOT_RUN | PLANNED_LAST |
