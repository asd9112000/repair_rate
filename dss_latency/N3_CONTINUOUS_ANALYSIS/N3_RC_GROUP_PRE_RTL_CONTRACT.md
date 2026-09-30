# N3 RC-GROUP final pre-RTL contract

## Authority and scope

This document is the final pre-RTL authority for
`G2X2_N3_RC_GROUP_CA_LIVE`.  It resolves only the production physical-address
domain, analyzer semantic payload, reconstruction scaling, and fixed-mask
equivalence boundaries.  It does not create production RTL, a production TB,
or synthesis evidence.

```
WORKTREE: /home/asd9112000/repair_rate_date2026_n3
BRANCH: integration/date2026-n3-ca-live-scaling
N3_R_GROUP: DEFERRED
```

## 1. Frozen production physical-address contract

RS/CS scaling changes redundancy capacity; it does not change the production
physical repair-address domain inherited from frozen N2 RC-GROUP CA-LIVE.

| Production parameter | Value | Status |
| --- | ---: | --- |
| `ROW_ADDR_W` | 9 | PASS |
| `PHYS_COL_ADDR_W` | 13 | PASS |
| `WORD_COL_ADDR_W` | 5 | PASS |
| `HYBRID_LINE_ADDR_W` | 13 | PASS |

Consequently, production N3 pivot-column addresses are 13 bits and production
N3 Hybrid-differing addresses are 13 bits.  Neither field may regress to the
historical RS3 5-bit/9-bit coordinate representation.

```
PHYSICAL_ADDRESS_CONTRACT: 9 / 13 / 13
PHYSICAL_ADDRESS_CONTRACT_STATUS: PASS
```

## 2. Historical and production analyzer bundles are distinct

The historical RS3 top in
`rtl/dss_v2/rs3cs3m1/recam_dss_v2_rs3cs3m1_group_top.sv` remains valid
historical evidence.  Its bundle is named:

```
HISTORICAL_RS3_ANALYZER_SEMANTIC_BUNDLE: 400
```

Its arithmetic is preserved, rather than overwritten:

```
7 + (7 x 9) + (7 x 5) + (4 x 7) + (4 x 7)
  + 17 + (17 x 3) + 17 + (17 x 9) + 1 = 400
```

That historical bundle is reference-only for recovered RS3 field semantics,
capacity/config behavior, legal pattern universe, and PatternID mapping.  It
is not authority for production physical-column width, production Hybrid-line
width, or the CA-LIVE production analyzer boundary.

Production N3 uses the frozen physical domains above:

```
pivot_valid                                      7
+ pivot_rows                         7 x 9  =  63
+ pivot_cols                         7 x 13 =  91
+ threshold_state                                56
+ hybrid_valid                                   17
+ hybrid_pointer                    17 x 3 =  51
+ hybrid_descriptor                             17
+ hybrid_differing                 17 x 13 = 221
+ conventional_overflow                         1
=                                                 524
```

```
HISTORICAL_N3_ANALYZER_PAYLOAD: 400
HISTORICAL_400BIT_ROLE: REFERENCE_ONLY
N3_PRODUCTION_ANALYZER_PAYLOAD: 524
PRODUCTION_524BIT_ARITHMETIC: PASS
```

## 3. N2-to-N3 production payload delta

The frozen N2 production analyzer is 272 bits.  Its production N3 successor
is 524 bits, a +252-bit capacity scaling with no new semantic field class.

| Field | N2 bits | N3 production bits | Delta |
| --- | ---: | ---: | ---: |
| pivot valid | 5 | 7 | +2 |
| pivot rows | 5 x 9 = 45 | 7 x 9 = 63 | +18 |
| pivot cols | 5 x 13 = 65 | 7 x 13 = 91 | +26 |
| threshold state | 30 | 56 | +26 |
| Hybrid valid | 7 | 17 | +10 |
| Hybrid pointer | 7 x 3 = 21 | 17 x 3 = 51 | +30 |
| Hybrid descriptor | 7 | 17 | +10 |
| Hybrid differing | 7 x 13 = 91 | 17 x 13 = 221 | +130 |
| overflow | 1 | 1 | 0 |
| **total** | **272** | **524** | **+252** |

```
2 + 18 + 26 + 26 + 10 + 30 + 10 + 130 = 252
N2_PRODUCTION_TO_HISTORICAL_RS3: 272 -> 400 (+128)
N2_PRODUCTION_TO_N3_PRODUCTION: 272 -> 524 (+252)
N2_TO_N3_PRODUCTION_DELTA: PASS
```

## 4. Live projection and no duplicate analyzer state

Production data flow is:

```
authoritative live collector state
    -> combinational 524-bit semantic projection
    -> one shared analyzer
```

Four 524-bit snapshot banks and any new 524-bit registered analyzer StateBank
are prohibited.  The analyzer consumes the current live projection; it is not
an owner of a duplicated collector payload.

```
DUPLICATED_ANALYZER_STATEBANK: NO
NEW_524BIT_REGISTERED_STATEBANK: NO
```

## 5. Reconstruction: exact frozen N2 source and mechanical N3 scaling

The source is frozen N2
`dss_final/continuous_analysis_live/G2X2_N2_RC_GROUP_CA_LIVE/src/`:

* `dss_group_pivot_address_regs` declares only `pivot_row_q[0:3][0:4]` and
  `pivot_col_q[0:3][0:4]` as reconstruction-private registers.
* The CA-LIVE core registers `sa_commit_valid_o`, `selected_config_flat_o`,
  and `selected_pattern_flat_o`; the reconstruction module consumes these
  three values combinationally with its private pivot registers.
* No Hybrid field is retained by this reconstruction module.  Hybrid data is
  analyzer input only in the frozen N2 topology, so its reconstruction-private
  retained width is exactly zero, not an unknown field.

| Retained reconstruction dependency | N2 count x width | N2 bits | N3 count x width | N3 bits | Scaling rule |
| --- | --- | ---: | --- | ---: | --- |
| captured pivot rows (`pivot_row_q`) | 4 SA x 5 x 9 | 180 | 4 SA x 7 x 9 | 252 | pivot count 5 -> 7; row width unchanged |
| captured physical pivot columns (`pivot_col_q`) | 4 SA x 5 x 13 | 260 | 4 SA x 7 x 13 | 364 | pivot count 5 -> 7; physical column width unchanged |
| commit-valid (`group_commit_valid_i`) | 4 x 1 | 4 | 4 x 1 | 4 | control unchanged |
| selected ConfigID (`selected_config_flat_i`) | 4 x 3 | 12 | 4 x 3 | 12 | four configs/SA and IDs 0..6 fit 3 bits |
| selected PatternID (`selected_pattern_flat_i`) | 4 x 4 | 16 | 4 x 6 | 24 | PatternID 4 -> 6 |
| Hybrid reconstruction retention | 0 | 0 | 0 | 0 | absent from actual frozen reconstruction source |
| **total dependency set** |  | **472** |  | **656** | **mechanically derived** |

For audit clarity, the private pivot capture bank alone is:

```
N2_PRIVATE_PIVOT_CAPTURE: 4 x 5 x (9 + 13) = 440
N3_PRIVATE_PIVOT_CAPTURE: 4 x 7 x (9 + 13) = 616
N2_RECONSTRUCTION_RETAINED_WIDTH: 472
N3_RECONSTRUCTION_RETAINED_WIDTH: 656
RECONSTRUCTION_DELTA: MECHANICALLY_DERIVED_FROM_N2
RECONSTRUCTION_WIDTH_DERIVATION: PASS
N3_RECONSTRUCTION_ARCHITECTURE: SAME_ARCHITECTURE_AS_N2_RC_GROUP_CA_LIVE
```

The N2 capture lifecycle is retained: capture the active SA after its final
canonical scan slot only when no active state update wins, then reconstruct
combinationally from captured pivots plus registered group selection.

The exact N2 final-output ports are:

| Port | N2 width | Production N3 width | Semantic status |
| --- | ---: | ---: | --- |
| `final_repair_address_flat_o` | 4 x 5 x 13 = 260 | 4 x 7 x 13 = 364 | same physical repair-address domain; count widened |
| `final_repair_is_row_flat_o` | 4 x 5 = 20 | 4 x 7 = 28 | same row/column identity; count widened |
| `final_repair_line_valid_flat_o` | 4 x 5 = 20 | 4 x 7 = 28 | same repair-line-valid meaning; count widened |

## 6. Fixed-mask source policy and equivalence layers

Historical runtime `nth_pattern` remains `REFERENCE_ONLY`.  Production N3
uses compile-time/elaboration-time fixed masks.  The preserved
`N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv` establishes the following first
layer with 80/80 rows passing and zero mismatches.

| Layer | Boundary | Status |
| --- | --- | --- |
| 1. Pattern-table equivalence | historical `nth_pattern` versus fixed table: capacity class + PatternID -> row/column mask | PASS; 80/80, 0 mismatch |
| 2. Analyzer decision equivalence | future independent golden oracle: full 524-bit production semantic state + requested N3 capacity/config -> feasible + PatternID | DEFINED |
| 3. Full GROUP solution equivalence | future full-width golden: repairable, selected external Config/action, PatternID, donor/release/borrow, and physical repair addresses | DEFINED |

Layer 2 must never truncate 524-bit production state to the historical
400-bit bundle: that aliases production physical addresses.  Layer 3 begins
only after RTL exists.

```
FIXED_MASK_PATTERN_EQUIVALENCE: PASS
FIXED_MASK_ROWS: 80
FIXED_MASK_MISMATCHES: 0
PRODUCTION_ANALYZER_ORACLE_BOUNDARY: DEFINED
FULL_GROUP_ORACLE_BOUNDARY: DEFINED
```

## 7. Frozen CA-LIVE control and latency boundary

The production N3 controller inherits the frozen N2 RC-GROUP CA-LIVE policy:
one shared analyzer; fault update wins and suppresses the same-edge old result;
restart at the first canonical config; earlier-SA candidates persist without
replay; a partial current-SA scan cannot finalize; BIST_DONE does not start an
analysis; and the GROUP result is registered.  Four configs are evaluated for
each active SA.  Scaling MAX_K from 5 to 7 and Hybrid entries from 7 to 17
does not add scheduler states.

```
N3_CA_LIVE_CONTROL_ARCHITECTURE: UNCHANGED_FROM_N2
CONFIG_EVALUATIONS_PER_ACTIVE_SA: 4
MEASURED_N3_LATENCY: NOT_CLAIMED
FUTURE_MEASUREMENT_BOUNDARY: final active-SA state update -> registered GROUP solution ready
```

## 8. Readiness decision

```
PHYSICAL_ADDRESS_CONTRACT: PASS
HISTORICAL_400BIT_CLASSIFICATION: PASS
PRODUCTION_524BIT_ARITHMETIC: PASS
N2_TO_N3_PRODUCTION_DELTA: PASS
DUPLICATED_STATEBANK: NO
RECONSTRUCTION_ARCHITECTURE: SAME_AS_N2
RECONSTRUCTION_WIDTH_DERIVATION: PASS
FIXED_MASK_PATTERN_EQUIVALENCE: PASS
PRODUCTION_ANALYZER_ORACLE_BOUNDARY: DEFINED
FULL_GROUP_ORACLE_BOUNDARY: DEFINED
CA_LIVE_CONTROL: UNCHANGED_FROM_N2
REVIEW_REQUIRED_PARAMETERS: NONE
N3_RC_GROUP_READY_FOR_RTL: YES
```

`READY_FOR_RTL` means the contract is technically closed.  This phase remains
evidence-only: a separate explicit authorization/review is required before
creating the isolated N2-to-N3 RTL scaling implementation.

```
HUMAN_REVIEW_REQUIRED: YES (authorization before implementation, not an unresolved parameter)
RTL_CREATED: NO
PRODUCTION_TB_CREATED: NO
DC_RUN: NO
COMMIT: NOT_CREATED
```
