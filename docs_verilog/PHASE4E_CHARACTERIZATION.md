# Phase 4E characterization record

## Status

Phase 4E-2 directed closure, Phase 4E-3 randomized EARLY equivalence, and the
final 20-ns characterization are complete.  No Phase 4F work is authorized.

## Implemented production boundary

`recam_dss_v2_early_top` contains the legacy shared RECAM analyzer, V2 EARLY
core, Phase 4B resource feasibility, Phase 4C registered resource ledger, and
the legacy-compatible diagnostic ledger adapter.  It includes top-level
controller/glue only for this decision/resource-management boundary.

| Audit item | Result |
| --- | --- |
| Full ConfigPatternMap | No |
| GROUP logic | No |
| Reconstruction logic | No |
| Test wrapper | No |
| Diagnostic ledger adapter in production boundary | Yes |

The diagnostic adapter is combinational and is included in the eventual
primary V2 synthesis total; it must not be manually subtracted.

## Latency measurement

The production-to-production Verilator test uses identical all-zero analyzer
inputs, then separately an input that forces failure at A.  Values below are
clock edges after the edge accepting `start_i`; they are DSS scheduling and
candidate-selection latency, not RECAM BIST/fault-analysis latency.

| Case | Legacy Phase 3J EARLY | V2 specialized EARLY | Delta |
| --- | ---: | ---: | ---: |
| All-local success | 30 | 4 | -26 |
| Failure at A | 27 | 4 | -23 |

`LATENCY_EQUIVALENT = NO`.

The legacy Phase 3J path always collects four analyzer results for each of
four SAs into ConfigPatternMaps before allocation (16 candidate analyses),
then allocates.  V2 streams and evaluates candidates only until the frozen
EARLY first-feasible choice: one candidate for each SA in all-local success,
or four at A for failure-at-A.  The V2 configuration-check architecture is
therefore specialized/data-dependent; this latency change is disclosed and is
not attributed solely to a storage-area optimization.  The repository has no
reliable BIST-end timing oracle, so no BIST-concurrent latency claim is made.

## Sequential-state audit

| State | Bits | Classification |
| --- | ---: | --- |
| Phase 4C released / borrower-valid / borrower-ID ledger | 16 | architectural persistent state |
| Phase 4C commit-accepted / commit-error status pulses | 2 | transient internal status state |
| SA/rank traversal and busy/done/result/failure control | 9 | transient controller state |
| Selected ConfigID / PatternID | 28 | selected-result state |
| Commit bitmap, donor, borrow and release diagnostics | 20 | output/diagnostic state |
| Shared RECAM analyzer | 0 | pre-existing combinational logic |
| Diagnostic ledger adapter | 0 | combinational visibility logic |
| **V2 production total** | **75** | |

The legacy Phase 3J reference has 485 sequential cells/logical bits, including
about 400 ConfigPatternMap-related bits.  The 75-bit V2 total includes two
Phase 4C internal commit-status flops and demonstrates the logical storage
removal.

## 20-ns synthesis and area comparison

The V2 flow uses `recam_dss_v2_early_top`, TSMC018 `slow.db`, a 20-ns `clk_i`
clock, timing-driven low-effort compile, and NAND2X1 reference area 9.979200.
The valid report is in `results/phase4e/dc_tsmc018_slow`.

| Metric | Legacy Phase 3J EARLY | V2 specialized EARLY | Delta (V2 - legacy) |
| --- | ---: | ---: | ---: |
| Total cell area | 117,219.010491 | 80,871.437539 | -36,347.572952 (-31.0083%) |
| GE | 11,746.33 | 8,104.00 | -3,642.33 (-31.0082%) |
| WNS | +0.01 ns | +0.01 ns | 0.00 ns |
| Critical delay | 19.71 ns | 19.70 ns | -0.01 ns |
| Sequential area | 25,882.718727 | 4,101.451260 | -21,781.267467 (-84.1537%) |
| Sequential cells | 485 | 75 | -410 (-84.5361%) |
| Logical state bits | 485 reference | 75 | -410 (-84.5361%) |

V2 timing met the 20-ns target.  Critical arrival was 19.70 ns, required time
was 19.71 ns, WNS was +0.01 ns, and the critical path was
`c/sa_q_reg[1] -> c/l/borrower_id_q_reg[7]`.  The valid area report contains
zero macro/black-box area and no unresolved-link error.

### V2 hierarchy breakdown

| Component | Area | V2 total |
| --- | ---: | ---: |
| Shared legacy RECAM analyzer | 70,938.8070 | 87.7180% |
| EARLY core total | 9,932.6305 | 12.2820% |
| └ EARLY controller/core-local logic | 3,319.7473 | 4.1050% |
| └ Phase 4B feasibility | 622.0368 | 0.7692% |
| └ Phase 4C ledger | 2,175.4656 | 2.6900% |
| └ Legacy ConfigID adapter | 362.5776 | 0.4483% |
| └ Diagnostic ledger adapter | 332.6400 | 0.4113% |

The storage-removal hypothesis is **STRONGLY SUPPORTED** for the frozen
decision/resource-management boundary: functional equivalence passed, total
cell area fell 31.0083%, and sequential cells fell from 485 to 75.  The shared
RECAM analyzer remains the dominant 87.7180% of V2 cell area.  This conclusion
does not hide the documented streaming-latency difference above.

## Reconstruction boundary limitation

The frozen Phase 3J RTL does not expose a final pivot/Hybrid-derived repair
reconstruction interface.  Phase 4E establishes equivalence and area
comparison at the DSS decision/resource-management boundary.  Final
repair-table reconstruction remains deferred.

## Phase control

```text
V2_PHASE = PHASE_4E_COMPLETE
V2_NEXT_PHASE_AUTHORIZED = NO
READY_FOR_PHASE_4F = YES
```
