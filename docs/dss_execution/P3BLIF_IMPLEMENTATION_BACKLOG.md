# P3-BL-IF — P3-BL-RTL implementation backlog

No item below was implemented by P3-BL-IF. This is the exact later-phase backlog.

| Architecture | Work item | Class | Reuse/adaptation boundary | Required acceptance evidence |
|---|---|---|---|---|
| SYN-B 2x2 GLOBAL | Candidate-map producer from collector snapshot | **NEW** | May adapt `recam_shared_config_analyzer`; enumerates all SA/action/Pattern candidates and local demand/effects. | Produced-map comparison against frozen reference vectors. |
| SYN-B 2x2 GLOBAL | Candidate-transition adapter | **ADAPT** | Derives actual release/borrow and resources without changing existing GLOBAL semantics. | Directed release/borrow and future-obligation cases. |
| SYN-B 2x2 GLOBAL | Integrated top | **NEW** | Connect snapshot, producer, approved GLOBAL core, commit adapter, result ports. | End-to-end equivalence to candidate-map behavior. |
| SYN-B 2x2 GLOBAL | Atomic selected-tuple commit adapter | **NEW** | Reuse abstract ledger rules only; never expose partial tuple commit. | Success and forced-error atomicity tests. |
| SYN-B 2x2 GLOBAL | Policy core | **REUSE** | Use baseline or explicitly named approved OPT level; OPT1 is the only current safe optional optimization. | Existing core regression plus integrated equivalence. |
| SYN-C 1x4 EARLY | RS2 producer and capacity-attempt table | **NEW** | Reuse snapshot semantics, not 2x2 ConfigID/action mapping. | C++ candidate/PatternID/attempt fixture comparison. |
| SYN-C 1x4 EARLY | Neighbor topology and row ledger | **NEW** | A-B-C-D row adjacency, left-first middle donor, no forwarding. | Directed adjacency, B/C priority, no-forwarding tests. |
| SYN-C 1x4 EARLY | Controller adapter and top | **ADAPT / NEW** | Reuse only generic A-to-D transaction sequencing. | C++ EARLY tuple, failure position, final-owner comparison. |
| SYN-D 1x4 GLOBAL | Candidate record storage/interface | **NEW** | Capacity attempt + PatternID + demand + mapping reference; no 2x2 three-bit map. | Candidate identity and equal-demand reduction tests. |
| SYN-D 1x4 GLOBAL | Four-depth search | **NEW** | 1x4 demand/neighbor partial ledger state and C++ objective. | Oracle comparison for repairability, tuple, final owners. |
| SYN-D 1x4 GLOBAL | Atomic commit adapter and top | **NEW** | 1x4 neighbor ledger, suppressed intermediate state. | Atomicity and end-to-end C++ comparison. |

## Sequencing and gates

1. Freeze packed candidate-table widths from reviewed RS2/CS2/m1 source tables.
2. Build topology/ledger transactions before policy integration.
3. Prove each 1x4 controller/search against the C++ semantic oracle using tuple and final owners, not repairability alone.
4. Build both integrated GLOBAL tops and verify all four top boundaries end at committed ledger.
5. Only then select a common GLOBAL optimization level and request a separate synthesis phase.

DC, WithScratch, RS3, formal 100k, and device sweeps are not P3-BL-RTL acceptance substitutes.
