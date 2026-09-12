# Phase 4F — Specialized GROUP-NoScratch Characterization

## Scope and boundary

The synthesized production boundary is `recam_dss_v2_group_top`:

```text
shared RECAM analyzer → GROUP collection → compressed candidate store
→ GROUP priority/feasibility → Phase 4C ledger → decision outputs
```

It implements only the frozen GROUP decision/resource-management boundary.
Final pivot/Hybrid-derived repair reconstruction, runtime repair-table encoding,
Scratch storage, and any post-BIST latency metric remain deferred.

## Boundary audit

| Item | Result |
|---|---:|
| Candidate store | 80 bits |
| ConfigPatternMap | absent from V2 production RTL |
| 26-bit CandidateSummary array | absent |
| Pattern-valid bitmap storage | absent |
| Persistent ConfigID map | absent; derived from SA/slot |
| Persistent descriptors | absent; derived combinationally |
| Scratch | absent |
| Reconstruction state | absent |

## Functional evidence

The Phase 4F-4 test-only bridge applies identical canonical candidate maps to
the frozen legacy GROUP allocator and V2 GROUP-NoScratch.  B/C canonical slots
are converted to the legacy map representation only at that bridge; V2
production RTL does not receive a ConfigPatternMap.

```text
seed = 20260910
GROUP 50 vectors:   0 mismatches
GROUP 1000 vectors: 0 mismatches
```

The checked boundary is selected ConfigID/PatternID, donor and release actions,
ledger after each commit, failure position, and group repairability.

`GROUP_PHYSICAL_FEASIBILITY_FALLBACK` is
`N/A_PROVEN_UNREACHABLE`: under the frozen topology, slot3 and slot2 for a
given SA share the same donor pair, so a donor-caused slot3 infeasibility also
makes slot2 infeasible.  This is not a missing coverage item.

## Architectural state audit

| State class | Bits |
|---|---:|
| Compressed GROUP candidate store | 80 |
| Phase 4C released/borrower ledger | 16 |
| Phase 4C status | 2 |
| Collection phase plus shared SA/index counters | 6 |
| Allocation incremental counters | 0 |
| Lifecycle status | 5 |
| Selected ConfigID/PatternID | 28 |
| Commit/donor/borrow/release diagnostics | 20 |
| **GROUP architectural state** | **157** |

The collection and allocation phases reuse the same four-bit SA/index pair.
The 157 logical state bits must not be equated to the mapped sequential-cell
count.

## DSS execution latency

This metric is `start_i` acceptance to `done_o`; it is not post-BIST latency.
The analyzer is combinational, so its use does not add sequential cycles to
the measured GROUP core flow.

| Case | Collection cycles | Allocation cycles | Post-start cycles |
|---|---:|---:|---:|
| All-local success | 16 | 4 | 20 |
| Failure at A | 16 | 4 | 20 |
| Failure at B | 16 | 5 | 21 |
| Failure at C | 16 | 6 | 22 |
| Failure at D | 16 | 7 | 23 |
| Slot2-last success | 16 | 10 | 26 |
| Release+borrow | 16 | 6 | 22 |

## Synthesis characterization

The 20-ns TSMC018 slow-db runner is
`scripts/synthesis/run_recam_phase4f_dc.sh`.  It writes reports under
`results/phase4f/dc_tsmc018_slow/v2_specialized_group_noscratch_20ns/`.

The completed 20-ns DC run used `slow.db`, `clk_i`/`recam_clk`, and a
`9.979200` NAND2X1 reference area.  The primary metric below is **Total cell
area**, not DC's wire-load-inclusive `Total area`.

| Metric | Legacy Phase 3J GROUP | V2 Specialized EARLY | V2 GROUP-NoScratch |
|---|---:|---:|---:|
| Cell area | 117139.176890 | 80871.437539 | 105393.658552 |
| GE | 11738.33 | 8104.00 | 10561.33 |
| WNS @ 20 ns | +0.01 ns | +0.01 ns | +0.00 ns |
| Combinational area | 91256.458163 | 76769.986279 | 96611.962423 |
| Sequential area | 25882.718727 | 4101.451260 | 8781.696129 |
| Total cells | 5689 | 4114 | 5319 |
| Combinational cells | 5204 | 4039 | 5162 |
| Sequential cells | 485 | 75 | 157 |
| Architectural state | legacy reference | 73 bits | 157 bits |
| Candidate-store state | about 400-bit legacy map state | 0 | 80 bits |
| DSS execution latency | not measured here | 4-cycle representative core | 20–26 cycles, case dependent |

The V2 GROUP-NoScratch critical path is
`core/sa_q_reg[1] → core/store/store_q_reg[70]`, crossing slot decode,
the shared analyzer, and the candidate-store write cone.  Arrival and required
time are both 19.74 ns; WNS is +0.00 ns and DC reports no violated constraints.
The compressed candidate store is the endpoint of the worst path, while the
shared analyzer is the largest hierarchy block (73756.2678 area, 70.0%).  The
store is 21129.2930 area (20.0%); the whole GROUP core is 31620.7588 area.

## Cost interpretation

Relative to legacy Phase 3J GROUP:

```text
GROUP area reduction            = 10.0270%
GROUP GE reduction              = 10.0270%
GROUP sequential-area reduction = 66.0712%
```

Relative to V2 Specialized EARLY, deferred GROUP selection adds:

```text
area              = 24522.221013
GE                = 2457.33
architectural state = 84 bits
sequential cells  = 82
sequential area   = 4680.244869
```

H1 is supported by the frozen-equivalence regression: GROUP semantics are
preserved without a generalized ConfigPatternMap.  H2 is **SUPPORTED**: the
80-bit compressed representation reduces total GROUP cell area by about 10%
and sequential area by about 66% versus the validated legacy GROUP boundary,
while still carrying a real 30.32% total-area cost over streaming V2 EARLY.
