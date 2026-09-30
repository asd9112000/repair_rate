# N3 RC-GROUP N2-to-N3 production delta audit

## Scope and disposition

This revision supersedes the earlier production-readiness conclusions in this
file.  It preserves the recovered historical RS3 evidence, but distinguishes
the historical 400-bit interface from the authorized production 524-bit
CA-LIVE semantic payload.  No N2 source, N3 RTL, production TB, DC result, or
commit is created here.

```
N2_TEMPLATE: G2X2_N2_RC_GROUP_CA_LIVE
N3_TARGET: G2X2_N3_RC_GROUP_CA_LIVE
HISTORICAL_RS3_ANALYZER_SEMANTIC_BUNDLE: 400
N3_PRODUCTION_ANALYZER_PAYLOAD: 524
```

## Frozen production geometry

| Parameter | N2 production | N3 production | Historical RS3 bundle | Production disposition |
| --- | ---: | ---: | ---: | --- |
| `ROW_ADDR_W` | 9 | 9 | 9 | unchanged |
| `PHYS_COL_ADDR_W` | 13 | 13 | 5 pivot-coordinate bits | physical production domain unchanged |
| `WORD_COL_ADDR_W` | 5 | 5 | 5 | unchanged |
| `HYBRID_LINE_ADDR_W` | 13 | 13 | 9 differing-coordinate bits | physical production domain unchanged |

RS/CS/m changes 2/2/1 -> 3/3/1 scale capacity, not physical geometry.
Historical 9/5/9 address semantics remain reference evidence only.

```
PHYSICAL_ADDRESS_CONTRACT: 9 / 13 / 13
PHYSICAL_ADDRESS_CONTRACT_STATUS: PASS
```

## Payloads and exact delta

| Field | N2 production | Historical RS3 | N3 production | Production delta |
| --- | ---: | ---: | ---: | ---: |
| pivot valid | 5 | 7 | 7 | +2 |
| pivot rows | 5x9 = 45 | 7x9 = 63 | 7x9 = 63 | +18 |
| pivot cols | 5x13 = 65 | 7x5 = 35 | 7x13 = 91 | +26 |
| threshold state | 30 | 56 | 56 | +26 |
| Hybrid valid | 7 | 17 | 17 | +10 |
| Hybrid pointer | 7x3 = 21 | 17x3 = 51 | 17x3 = 51 | +30 |
| Hybrid descriptor | 7 | 17 | 17 | +10 |
| Hybrid differing | 7x13 = 91 | 17x9 = 153 | 17x13 = 221 | +130 |
| conventional overflow | 1 | 1 | 1 | 0 |
| **total** | **272** | **400** | **524** | **+252** |

```
HISTORICAL_400BIT_ARITHMETIC: 7 + 63 + 35 + 56 + 17 + 51 + 17 + 153 + 1 = 400
PRODUCTION_524BIT_ARITHMETIC: 7 + 63 + 91 + 56 + 17 + 51 + 17 + 221 + 1 = 524
N2_TO_HISTORICAL_RS3: 272 -> 400 (+128)
N2_TO_N3_PRODUCTION: 272 -> 524 (+252)
2 + 18 + 26 + 26 + 10 + 30 + 10 + 130 = 252
```

## N3 capacity, candidate store, and control

| Property | N2 | N3 | Disposition |
| --- | ---: | ---: | --- |
| topology / SA count | G2X2 / 4 | G2X2 / 4 | unchanged |
| configs per active SA | 4 | 4 | unchanged |
| MAX_K / pivot slots | 5 | 7 | scaled |
| Hybrid entries | 7 | 17 | scaled |
| PatternID width | 4 | 6 | scaled |
| candidate store | 16x5 = 80 | 16x7 = 112 | scaled only by PatternID widening |
| analyzer snapshot bank | none | none | prohibited |

The N3 fixed table has 80 legal capacity-class/PatternID rows.  It matches the
historical descending-mask `nth_pattern` mapping on every row (0 mismatch).
The production implementation uses the fixed table; historical runtime
`nth_pattern` remains reference-only.

```
N3_CONFIGS_PER_ROLE: 4
N3_CONFIG_EVALUATIONS_PER_ACTIVE_SA: 4
N3_GROUP_STORE_WIDTH: 112
FIXED_MASK_EQUIVALENCE: PASS
DUPLICATED_ANALYZER_STATEBANK: NO
N3_CA_LIVE_CONTROL_ARCHITECTURE: UNCHANGED_FROM_N2
```

## Frozen reconstruction derivation

Frozen N2 `dss_group_pivot_address_regs` has private state only for four SA
sets of pivot row and physical-column registers.  The registered core outputs
needed by its reconstruction combinational logic are commit-valid, ConfigID,
and PatternID.  There is no Hybrid reconstruction register in N2.

| Dependency | N2 bits | N3 bits | Mechanical rule |
| --- | ---: | ---: | --- |
| private captured pivots | 4x5x(9+13) = 440 | 4x7x(9+13) = 616 | pivot count 5 -> 7 |
| commit valid | 4 | 4 | unchanged |
| ConfigID | 4x3 = 12 | 4x3 = 12 | unchanged |
| PatternID | 4x4 = 16 | 4x6 = 24 | 4 -> 6 bits |
| Hybrid reconstruction retention | 0 | 0 | absent from frozen source |
| **total retained dependency** | **472** | **656** | **mechanically derived** |

N3 retains the N2 capture lifecycle and output semantics.  Output count widens
from 4x5 to 4x7 while the physical address remains 13 bits:
`address 260->364`, `is_row 20->28`, and `line_valid 20->28`.

```
N2_RECONSTRUCTION_RETAINED_WIDTH: 472
N3_RECONSTRUCTION_RETAINED_WIDTH: 656
RECONSTRUCTION_ARCHITECTURE: SAME_ARCHITECTURE_AS_N2_RC_GROUP_CA_LIVE
RECONSTRUCTION_WIDTH_DERIVATION: PASS
```

## Future proof boundaries

1. **Pattern-table equivalence:** capacity class + PatternID -> row/column
   mask; historical enumerator versus fixed table; PASS 80/80, 0 mismatch.
2. **Analyzer decision equivalence:** a future independent golden oracle
   receives the full 524-bit production semantic state plus N3 config/capacity
   and yields feasible plus PatternID.  No 524->400 truncation is permitted.
3. **Full GROUP equivalence:** future full-top compares repairable, selected
   external Config/action, PatternID, donor/release/borrow behavior, and full
   physical repair addresses against an independent full-width golden model.

```
PRODUCTION_ANALYZER_ORACLE_BOUNDARY: DEFINED
FULL_GROUP_ORACLE_BOUNDARY: DEFINED
N3_RC_GROUP_READY_FOR_RTL: YES
N3_R_GROUP: DEFERRED
RTL_CREATED: NO
PRODUCTION_TB_CREATED: NO
DC_RUN: NO
COMMIT: NOT_CREATED
```
