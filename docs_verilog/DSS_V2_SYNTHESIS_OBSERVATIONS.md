# DSS V2 — Unexpected / Non-obvious Synthesis Observations

All observations below are from completed 20-ns TSMC018 `slow.db` DC reports;
no new synthesis was run for this document.

| Observation | Measured evidence | Architectural explanation | Paper relevance | Confidence |
|---|---|---|---|---|
| EARLY total-area reduction is much smaller than state reduction | Legacy→V2 EARLY: total area −31.0083%; sequential area −84.1537%; sequential cells 485→75 | Removing ConfigPatternMap/history eliminates flops, but shared combinational RECAM analysis remains | Report total area and state separately; do not equate state reduction with total-area reduction | High |
| Shared analyzer dominates specialized EARLY | Analyzer 70,938.8070 / 80,871.437539 = 87.7% | The analyzer is retained for the frozen decision boundary and dominates after storage removal | Analyzer optimization is the principal route to further area reduction | High |
| GROUP store remains a major cost after compression | Analyzer 73,756.2678 (70.0%); store 21,129.2930 (20.0%); total 105,393.658552 | Deferred GROUP allocation requires 16 `{valid, PatternID}` records = 80 bits | Explains why GROUP's legacy-area reduction is only 10.0270% | High |
| GROUP timing pressure includes store write | Critical path `core/sa_q_reg[1] → core/store/store_q_reg[70]`, 19.74 ns, +0.00 ns | SA/slot decode and analyzer result feed candidate-store write | Store insertion, not allocator readback, is the critical registered cone | High |
| EARLY timing pressure reaches ledger commit | Critical path `c/sa_q_reg[1] → c/l/borrower_id_q_reg[7]`, 19.70 ns, +0.01 ns | Config selection drives analyzer `solution_valid`, then commit/ledger state | Both policies meet 20 ns, but their timing bottlenecks differ | High |
| Scratch gives no benefit at current GROUP boundary | Minimum GROUP history remains 80 bits; recompute needs input-stability contract or about 204-bit snapshot | Candidate lifetime extends to post-collection allocation; placement cannot remove required information | Treat Phase 4G as valid architecture-space pruning | High |
| Streaming specialization changes latency as well as storage | EARLY all-local about 4 cycles versus GROUP all-local 20; V2 EARLY uses no retained candidate history | EARLY consumes the current candidate, commits, then discards it; GROUP collects before allocation | Attribute performance change to specialized execution structure, not only storage removal | High |

## Accounting note

V2 EARLY has 73 architectural bits but 75 mapped sequential cells.  The
two-cell difference is Phase 4C transient commit-status state; maps and papers
must label the metric rather than mixing architectural bits with sequential
cell count.  V2 GROUP has 157 bits and 157 mapped sequential cells under the
reported accounting.

Sources: Phase 4E `hierarchy_area.rpt` and `timing.rpt`, Phase 4F
`hierarchy_area.rpt` and `timing.rpt`, and
[PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md](PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md).
