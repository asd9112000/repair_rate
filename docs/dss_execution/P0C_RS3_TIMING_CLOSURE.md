# P0-C — RS=CS=3, SHARE_M=1 20-ns Timing Closure Assessment

> Status: **NOT_CLOSED**.  This is an evidence-based stop point, not a
> timing-closure claim.  No RTL, simulator, formal data, or device sweep was
> changed by P0-C.

## Scope and policy identity

The Thesis-V1 target is `RS=CS=3`, `SHARE_M=1`, at a 20.0-ns clock with
`WNS >= 0`.  P0-B's behavioral audit establishes the policy identity needed
before evaluating a timing optimization:

| Historical controller | Normalized policy mapping | Timing-use decision |
|---|---|---|
| `recam_dss_v2_rs3cs3m1_early_core` | **not** normalized directional EARLY | Do not optimize as the production RTL |
| `recam_dss_v2_rs3cs3m1_group_core` / GROUP-NoScratch | normalized directional `EARLY` | Production-oriented timing reference |

The first mismatch in the historical `EARLY` controller occurs for SA B/C
when both LOCAL and RELEASE_ONLY are legal: it chooses LOCAL, whereas
normalized EARLY chooses RELEASE_ONLY.  This audit therefore uses the GROUP
NoScratch column for the production-policy conclusion.  The shared analyzer,
candidate contract, and resource ledger are not thereby reinterpreted.

## Authoritative synthesis provenance

All numbers below use the retained Design Compiler method: DC W-2024.09-SP2,
TSMC018 Arm CBDK `slow.db`, slow corner, 20.0 ns, zero IO delay,
`compile -map_effort low`, and `slow/NAND2X1 = 9.979200`.  The reports were
read from retained artifacts; no synthesis was launched by this task.

| Variant | RTL revision | Candidate representation | Added analysis latency | Area | GE | WNS @20 ns | Critical path | Timing |
|---|---|---|---:|---:|---:|---:|---:|---|
| Original RS3 GROUP-NoScratch | `3304c86` | generic, 35-wide | 0 | 12,053,273.51 | 1,207,839.66 | -44.24 ns | 64.14 ns | FAIL |
| Fixed-table GROUP-NoScratch (P0 representation) | `ff4e9a7` | fixed masks, 35-wide | 0 | 546,667.23 | 54,780.67 | -23.29 ns | 43.20 ns | FAIL |
| One-stage pipeline GROUP-NoScratch | `721d539` | fixed masks, 35-wide | +1 | 527,693.45 | 52,879.33 | -12.52 ns | 32.41 ns | FAIL |
| Iterative-7 | not implemented | 7 candidates/cycle | not established | — | — | — | — | NOT_EVIDENCED |
| Iterative-5 | not implemented | 5 candidates/cycle | not established | — | — | — | — | NOT_EVIDENCED |

`721d539` is the historical one-stage pipeline experiment named in the P0
task; it is included as baseline evidence only, not presented as a solution.
The P0 fixed-table result eliminated the runtime `nth_pattern` enumeration
from the implementation, but did not close timing.

## Critical-path decomposition

### Original 35-wide target

The original GROUP-NoScratch report at
`results/dss_v2_rs3cs3m1/h5_hardware_retry2/GROUP/timing.rpt` reports:

```text
startpoint: core/sa_q_reg[0]
endpoint:   core/store/store_q_reg[5]
path group: recam_clk
arrival:    64.14 ns
slack:      -44.24 ns
```

The path crosses the target configuration decode, shared analyzer/candidate
evaluation, then candidate-store capture.  The original H5 hierarchy report
attributes 11,995,819.93 area units of 12,053,273.51 to the analyzer, so this
classification is based on report hierarchy as well as source structure.

### Fixed-table representation

The P0 fixed-table GROUP result at
`/tmp/repair_rate-h5o/results/dss_v2_rs3cs3m1/h5_hardware_optimized/GROUP/`
reports:

```text
startpoint: core/sa_q_reg[1]
endpoint:   core/store/store_q_reg[105]
path group: recam_clk
arrival:    43.20 ns
slack:      -23.29 ns
```

This is a substantial representation reduction relative to the generic
enumeration, but it remains an analyzer/candidate-evaluation path and is not
20-ns feasible.

### One-stage pipeline diagnostic baseline

The retained P1 GROUP result at
`/tmp/repair_rate-h5p-p1/results/dss_v2_rs3cs3m1/h5p_p1_hardware/GROUP/`
reports:

```text
startpoint: core/sa_q_reg[1]
endpoint:   analyzer/matrix_q_reg[2][5]
path group: recam_clk
arrival:    32.41 ns
slack:      -12.52 ns
logic levels: 169
```

This endpoint is the matrix-preparation register, before the fixed 35-way
coverage check and first-valid PatternID reduction.  Consequently it is
direct evidence that simply changing the later candidate-evaluation width to
7 or 5 candidates per cycle cannot reduce this worst 32.41-ns source-to-matrix
path.  It may reduce a non-worst later stage, but the available reports do not
establish that such a batch stage would itself meet 20 ns.

## Bounded-width iterative candidate evaluation decision

The intended iterative architecture would preserve the finite candidate mask
table, ConfigID and PatternID order, the normalized EARLY priority, and the
final repair result while accumulating candidate-valid bits over five
7-candidate batches (or seven 5-candidate batches).  Its required controller
contract would hold the current SA/slot and ledger state until the final batch
returns; it must not commit a partial result or allow a later SA to overtake
the held transaction.

That design was **not implemented** for P0-C.  There is no evidence-backed
path to the stated 20-ns objective from candidate batching alone because the
already registered matrix-preparation stage is the remaining worst path at
32.41 ns.  Implementing it now would add latency and a scheduler without
removing the demonstrated timing blocker.  A further partition of
matrix/descriptor preparation, or another separately authorized structural
optimization, would be needed before an iterative batch experiment could be
claimed as timing closure.

Accordingly, none of the following is claimed:

```text
FUNCTIONAL_EQUIVALENCE: 0 mismatches
ITERATIVE_7_WNS_AT_20NS: closed
ITERATIVE_5_WNS_AT_20NS: closed
```

The historical P1 functional evidence remains retained separately; it does
not validate an unimplemented iterative variant.

## Closure record

```text
RS3_CS3_M1_TARGET_CLOCK:                 20.0 ns
PRODUCTION_NORMALIZED_POLICY_REFERENCE:  GROUP-NoScratch -> EARLY
HISTORICAL_EARLY_EXACT_MATCH:            NO
AUTHORITATIVE_BASELINE:                  ff4e9a7 fixed-table GROUP-NoScratch
BEST_RETAINED_20NS_WNS_FOR_CORRECT_POLICY:-12.52 ns (721d539 P1 diagnostic)
PRIMARY_REMAINING_CRITICAL_STAGE:        core state -> matrix preparation register
BOUNDED_WIDTH_ITERATION_IMPLEMENTED:     NO
FUNCTIONAL_EQUIVALENCE_FOR_ITERATIVE:    NOT_RUN (no RTL exists)
RS3_20NS_TIMING_CLOSED:                  NO
NEXT_RTL_MODIFICATION:                   NONE (requires separate authorization)
SIMULATION_CODE_CHANGED:                 NO
FORMAL_DATA_TOUCHED:                     NO
DEVICE_SWEEP_STARTED:                    NO
```

## Files examined

- `rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_shared_config_analyzer.sv`
- `rtl/dss_v2/rs3cs3m1/recam_dss_v2_rs3cs3m1_early_core.sv`
- `rtl/dss_v2/rs3cs3m1/recam_dss_v2_rs3cs3m1_group_core.sv`
- `results/dss_v2_rs3cs3m1/h5_hardware_retry2/{EARLY,GROUP}/`
- `/tmp/repair_rate-h5o/results/dss_v2_rs3cs3m1/h5_hardware_optimized/{EARLY,GROUP}/`
- `/tmp/repair_rate-h5p-p1/results/dss_v2_rs3cs3m1/h5p_p1_hardware/{EARLY,GROUP}/`
