# P3-BL-RTL-B — 1x4 Single-Hop STREAMING EARLY implementation

```text
P3BLRTLB_STATUS: COMPLETE
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_STREAMING_EARLY
LAYOUT: LINE_1X4
TOPOLOGY: NEIGHBOR_SHARING
RS: 2
CS: 2
SHARE_M: 1
POLICY: ONE_BY_FOUR_SINGLE_HOP_EARLY_V1
```

## Candidate table and physical-resource freeze

```text
RS2_CS2_M1_CANDIDATE_TABLE_FROZEN: YES
M1_SEMANTICS_FROZEN: YES
```

The authoritative C++ source is `capacityOptions()` and
`findOneByFourEarlyChoice()` in `src/DynamicRepairSimulator.cpp`, with the
physical allocation order in `PhysicalResourceLedger::allocateSequential()`.
For the frozen `(RS, CS, m) = (2, 2, 1)` point, columns are always local and
each SA owns one private and one shareable physical row.

| SA | Capacity attempts, in order | PatternID capacity | Candidate slots |
|---|---|---:|---:|
| A | 2R2C; 3R2C | 6; 10 | 16 |
| B | 2R2C; 3R2C; 4R2C | 6; 10; 15 | 31 |
| C | 2R2C; 3R2C; 4R2C | 6; 10; 15 | 31 |
| D | 2R2C; 3R2C | 6; 10 | 16 |

Thus the producer performs ten analyzer requests (`A0,A1,B0,B1,B2,C0,C1,C2,D0,D1`),
and has 94 architecturally usable PatternID positions.  It keeps a regular
180-slot representation (four SAs × three attempt fields × 15 PatternID
positions); unsupported endpoint attempt 2 positions are zero.

`PatternID` is C++ `candidateIndex + 1`. `attemptIndex` is the vector index
above. `mapping_ref = {attemptIndex, PatternID}`. `usedRows` and `usedColumns`
are not nominal capacity values: they are the numbers of populated row and
column dictionary entries selected by the PatternID, matching the C++ remap
decode. The policy rank is attempt first, then ascending PatternID.

```text
CAPACITY_ATTEMPT_COUNT: A=2; B=3; C=3; D=2
ATTEMPT_INDEX_WIDTH: 2
CANDIDATE_TOKEN_WIDTH: 6 (attemptIndex[1:0] + PatternID[3:0])
LOCAL_DEMAND_WIDTH: 5 (usedRows[2:0] + usedColumns[1:0])
DONOR_ID_WIDTH: 2
ROW_OWNER_ID_WIDTH: 2
ROW_ASSIGNMENT_WIDTH: 3 (A/B/C/D/unassigned)
MAPPING_REF_WIDTH: 6
```

The `m=1` row ledger has eight fixed physical rows. Each SA's line 0 is
private and line 1 is shareable. A donor scan is physical owner order, so B
chooses A before C and C chooses B before D. A line is eligible only when it
is unassigned, shareable, adjacent to the borrower, and still owned by its
original SA. This rejects A↔C, A↔D, B↔D, and forwarding of an already borrowed
line.

## Snapshot interface revision and analyzer decision

The frozen 2×2 snapshot has five pivot entries and seven hybrid entries. It
cannot represent the legal 4R2C B/C attempt: that attempt has six matrix
entries and a `4*(2-1)+2*(4-1)=10` Hybrid-CAM capacity. SYN-C therefore has a
topology-specific, same-family collector interface with six pivot entries,
threshold buses through `gt4`, and ten hybrid entries.

```text
COLLECTOR_SNAPSHOT_INTERFACE_REVISION: REQUIRED_AND_IMPLEMENTED
SYN_C_MAX_REQUIRED_PIVOTS: 6
SYN_C_HYBRID_ENTRIES: 10
SYN_A_B_MAX_PIVOTS: 5
SYN_A_B_INTERFACE_REUSED_UNCHANGED: YES
SYN_A_B_INTERFACE_CHANGED: NO
SYN_C_INTERFACE_EXTENSION_REASON: legal 4R2C capacity attempt in LINE1X4 Single-Hop RS2/CS2/m=1
SNAPSHOT_BITS_PER_SA: 279
GROUP_SNAPSHOT_STORAGE_BITS: 1116
LOCAL_RECAM_ENGINE_REUSE: ADAPT
ANALYZER_INSTANCE_COUNT: 1
CANDIDATE_PRODUCTION_REQUESTS: 10
CANDIDATE_GENERATION_CYCLES: 10
```

The adapted analyzer preserves the existing matrix/dictionary method and
C++ PatternID ordering, while extending the fixed maximum from 3R2C (five
entries) to 4R2C (six entries). SYN-A and SYN-B analyzer source and interfaces
remain unchanged.

The four-point comparison retains the same external functional boundary.
Topology-specific legal capacity classes may nevertheless require different
internal analyzer and snapshot widths. The 6-pivot, 10-hybrid SYN-C state is
therefore physical architecture cost, not state that may be normalized away.

## RTL and state boundary

```text
4 distinct collector snapshots
-> one shared 6-entry analyzer
-> serial 10-request candidate producer
-> 180 valid bits + 900 local-demand bits
-> streaming A->B->C->D controller
-> immediate persistent eight-row ownership ledger
```

| State class | Bits | Basis |
|---|---:|---|
| Four collector snapshots | 1116 | 4 × 279 bits |
| Producer control and table | 1087 | 7 control + 180 valid + 540 rows + 360 columns |
| EARLY control, result, ledger | 110 | controller, selected record, 24-bit assignment ledger |
| Top handoff control | 4 | state and start pulses |
| **Total logical state** | **2317** | registered architectural storage, not area |

```text
ROW_LEDGER_STATE_BITS: 24
ROW_OWNER_STATE_BITS: 16 (eight static two-bit owner identities)
EARLY_CONTROLLER_BITS: 20
SELECTED_RESULT_BITS: 64
TOTAL_LOGICAL_STATE_BITS: 2317
PHYSICAL_ROW_LEDGER_ENTRIES: 8
```

Columns are retained in each selected record but do not need a sharing ledger:
they are local-only and no prior transaction can consume another SA's columns.
Each ledger entry is a three-bit current assignee (`A/B/C/D/unassigned`). Its
original two-bit owner and private/shareable nature are structural: entry
`2*SA` is private, entry `2*SA+1` is shareable. A consumed entry is simply not
`unassigned`; a borrowed entry retains its structural original owner, so it
cannot become forwardable.

## Timing and verification evidence

Candidate production is fixed at ten capture edges. Candidate evaluation and
total failure latency are data-dependent because the controller tests each
PatternID until the first ledger-legal candidate or an exhausted attempt set.
Commit is one controller edge per accepted candidate and is immediately
persistent. The zero-snapshot witness measures 18 edges after accepted start;
the directed C-overflow failure measures 61. Neither value is a universal
latency claim.

```text
CANDIDATE_TABLE_MISMATCHES: 0
LOCAL_RECAM_CANDIDATE_MISMATCHES: 0
ADJACENCY_MISMATCHES: 0
MIDDLE_DONOR_PRIORITY_MISMATCHES: 0
BORROWED_LINE_FORWARDING_VIOLATIONS: 0
EARLY_PRIORITY_MISMATCHES: 0
ROLLBACK_EVENTS: 0
CPP_RTL_SHARED_CORPUS: YES
CPP_ORACLE_RANDOM_CASES: 1000
CPP_ORACLE_SEED: 20260920
CPP_ORACLE_MISMATCHES: 0
FINAL_OWNER_MISMATCHES: 0
FAILURE_POSITION_MISMATCHES: 0
ORACLE_CLASS_A_TO_I_MISMATCHES: 0,0,0,0,0,0,0,0,0
INTEGRATED_DIRECTED_SUCCESS_CYCLES: 18
INTEGRATED_FAILURE_A_CYCLES: 44
INTEGRATED_FAILURE_B_CYCLES: 60
INTEGRATED_FAILURE_C_CYCLES: 61
INTEGRATED_FAILURE_D_CYCLES: 47
```

`tests/p3blrtlb_cpp_oracle_corpus.cpp` creates one deterministic candidate
record corpus through the actual C++ `DynamicRepairSimulator` configured as
`OneByFourSingleHopEarlyV1` and `NeighborSharing`. The Verilator policy/ledger
test consumes that same producer-to-policy boundary file; the separately
integrated top test validates producer snapshots and its candidate table. The
shared-corpus comparison checks group result, failure SA, chosen
attempt/PatternID, used R/C, donor slots, borrow count, all eight row assignees,
and retained partial commits. The canonical encodings are A/B/C/D=`0/1/2/3`,
unassigned=`4`, and a no-donor two-slot nibble=`4'b1111`.

The initial shared-corpus run exposed an RTL adjacency defect: two-bit
arithmetic allowed the D borrower to wrap and observe A as adjacent. The
ledger now uses the explicit A↔B↔C↔D relation. The rerun has zero mismatch in
all A–I classifications; this is a fixed semantic issue, not an adapter-only
encoding difference.

The following focused regressions passed after SYN-C was added:

```text
make test_line1x4_single_hop_rs2_cs2_m1_normalized_streaming_early
make test_solution_take_policy test_dynamic_spare_sharing_layout
make test_canonical_directional_early
make test_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer
make test_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated
```

## Scope boundary

```text
INTEGRATED_TOP_IMPLEMENTED: YES
COMMON_INTEGRATED_BOUNDARY_REACHED: YES
SYN_C_SYNTHESIS_READY: YES
VERILATOR_LINT: PASS
STRICT_READABLE_VERILOG_GATE: BLOCKED_BY_LOCAL_RUNTIME
SYN_A_REGRESSION: PASS
SYN_B_REGRESSION: PASS
DC_SYNTHESIS_STARTED: NO
1X4_GLOBAL_RTL_STARTED: NO
OPT2_STARTED: NO
OPT3_STARTED: NO
```

No SYN-A or SYN-B production RTL was modified. This design is not a SYN-D,
WithScratch, RS3, device-simulation, formal-100k, or DC-synthesis result.
