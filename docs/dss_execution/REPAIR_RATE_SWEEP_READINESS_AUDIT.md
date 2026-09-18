# Repair-Rate Sweep R0 Readiness Audit

> 文件狀態：Current
> 適用範圍：repair-rate program / simulator-policy-output readiness
> 建立時間：2026-09-16T00:45:03+08:00
> 最後修改時間：2026-09-16T00:45:03+08:00
> 本文件權威主題：R0 readiness evidence, gaps, and R1 entry criteria

## Decision

R0 is complete as an audit only. No production RTL was changed, no synthesis
was run, and no formal sweep was run. Group and device results remain separate:
group is a four-SA Tier-0/Tier-1 experiment; device has one persistent
device-wide Tier-2 online-CAM pool.

| Program | Current role | Inputs and engine | Scope / outputs | Unified-core disposition |
|---|---|---|---|---|
| `DynamicSpareSharing.cpp` | primary group repair-rate simulator | generated, count-framed, or simplified faults; `DynamicRepairSimulator` + ledger + RECAM | 2×2/1×4; `attempts.csv`, `runs.csv`, `summary.csv`, remap optional | use for R2/R3 |
| `DynamicSpareSharing_SRAM_RECAM.cpp` | CAM/SRAM group search, latency, and analytical-cost comparison | one generated corpus; CAM baseline and SRAM adapter | 2×2 none/directional/global/edge; metrics CSV and cost JSON | backend comparator, not unified policy executable |
| `HierarchicalRECAM.cpp` | WoW-v1.0 device scheduler | generated or file groups; `DeviceRepairScheduler` and global CAM pool | multiple groups/banks/domains; device/groups/BIRA CSVs | retain for R4/R5 |

## C1/C2/C3 audit

| Required identity | Current C++ path | Finding |
|---|---|---|
| C1 EARLY | `SolutionTakePolicy::Early`, `findEarlyChoice` | A→B→C→D sequential allocation with `allocateSequential`; no prior decision is reconsidered. |
| C2 GROUP-GREEDY | `GroupNoScratchV2`, `findV2GroupNoScratchChoice` | collects all four V2 slots/SA, then greedily commits A→B→C→D; no tuple search, no rollback. |
| C3 GROUP-GLOBAL | `GroupCompressed`, `findCompressedGroupChoice` | full tuple enumeration and noncommitting ledger legality test exist, but it is historical-named and lacks the requested partial-repair objective/counters. |

`group_compressed_legacy` is not an RTL GROUP-NoScratch alias. It recursively
enumerates valid RECAM candidate tuples and only `applyAllocation` commits the
winner. It is a reusable C3 base, but must not silently become the published
C3 policy until an objective is approved.

### Candidate semantics and actual bounds

Generic candidates retain: valid bitmap membership; RECAM candidate/PatternID;
`usedRows`/`usedColumns`; repair-line and buffer-CAM mappings; matrix-address
metadata; and per-choice ledger results. The V2 C2 store instead retains four
valid+Pattern records per SA; its role slot maps to ConfigID/action and the C++
trace records ledger before/after, feasibility, selected PatternID, and demand.

For generic 2×2 directional `Rs=Cs=2,m=1`, A/D analyse `2R2C` and `2R3C`;
B/C analyse `2R2C` and `3R2C`. The maximum is
`C(4,2)+C(5,2)=6+10=16` valid candidates **per SA**, so C3 has at most
`16^4=65,536` tuple checks. This is not 16 group choices. The V2 C2 store has
four slots/SA (`4^4=256` hypothetical tuples) but deliberately searches none.
The older `DirectionalMultiConfigAnalyzer` has seven ConfigIDs/SA (`7^4=2401`)
and is **LEGACY/SEMANTICALLY_INCOMPATIBLE** with the generic candidate contract.

Current C3 ranking is: fewer borrowed lines, fewer total used lines,
lexicographically lower candidate IDs, then lower attempt indices. It requires
all four SAs to have a candidate. Recommended R1 objective, subject to explicit
approval: maximize repaired-SA count; prefer an all-four tuple; minimize shared
then total physical lines; tie-break A/B/C/D PatternID tuple, then ConfigID or
attempt tuple. Search attempts are not rollback; only the selected tuple commits.

### C2 RTL comparison

The C++ C2 policy has C2 timing/commit semantics, but exact active RTL priority
does not match source-level C++ order. `dss_v2_group_priority_reader.sv` orders
slots `1,0,3,2` (release-only, local, release-and-borrow, borrow-only), while
`v2RoleSlotMappings` loops `0,1,2,3` (local, release-only, borrow-only,
release-and-borrow). The focused C++ test proves an internal C++ golden, not an
RTL lockstep of that ordering. Resolve or explicitly version this mismatch in R1.

## Fault corpus, topology, and fairness

All generated models keep `fault_A+fault_B+fault_C+fault_D=faultCount`.
Uniform/moderate/strong/hotspot use deterministic weight apportionment;
`UserDefined` validates an explicit tuple. Thus fixed total is supported but
`FIXED_TOTAL_MULTINOMIAL` is missing. A seeded Dirichlet-multinomial allocator
is an easy extension before address generation.

`DynamicSpareSharing --repair-rate-sweep` generates a corpus once and replays
it across policies, and `DynamicSpareSharing` can replay file inputs.
`HierarchicalRECAM
--write-fault-corpus` supplies a physical replay artifact. The layout test
proves equal local maps for 2×2 and 1×4; only ledger accessibility changes.
`runs.csv` has `fault_A..fault_D`, but mean, population standard deviation,
min, max, and range are derived only in analysis scripts, not core fields.

At `Rs=Cs=2`, every requested policy has 8 row and 8 column lines, 16 total;
shared lines are borrowed eligibility, never added redundancy.

| Policy | Layout/accessibility | share row,col | physical rows / cols / total | Status |
|---|---|---:|---:|---|
| local | 2×2 none | 0,0 | 8/8/16 | matched |
| directional m=1 | 2×2 directional | 1,1 | 8/8/16 | matched |
| directional m=2 | 2×2 directional | 2,2 | 8/8/16 | matched |
| pairwise row=1/2 | 2×2 edge, row-only | 1,0 / 2,0 | 8/8/16 | matched physically; compare to row-only peer |
| 1×4 NN row-only | 1×4 neighbor | 1,0 | 8/8/16 | matched physically; separate accessibility axis |

Proposed R2 smoke range: `F_GROUP=8,12,16,20,24,28,32`. The generator ceiling
is `4*memoryRows*memoryColumns`; practical C3 limits are `C(R+C,R)`, number of
capacity attempts, and the Cartesian product. Do a bounded preflight before a
larger R3 sweep.

## Experiment readiness

| Requirement | Group (`DynamicSpareSharing`) | Device (`HierarchicalRECAM`) |
|---|---|---|
| EXP-A 2×2 vs 1×4 | READY | MISSING: no layout/pair/neighbor CLI |
| EXP-B local | READY | READY |
| EXP-B directional m=1 | READY | READY |
| EXP-B directional m=2 | READY | PARTIAL: core accepts it, no focused device regression |
| EXP-B pairwise row=1 | READY | READY: `edge`, rows=1, cols=0 |
| EXP-B pairwise row=2 | READY | PARTIAL: no focused device regression |
| EXP-B 1×4 row-only NN | READY | MISSING |
| EXP-C EARLY | READY | PARTIAL: no formal paired device workflow |
| EXP-C GROUP-GREEDY | PARTIAL: C2 priority discrepancy | PARTIAL |
| EXP-C GROUP-GLOBAL | PARTIAL: joint full-group search exists | PARTIAL via shared core |

The C++ core accepts positive Rs/Cs under signed-int and combinatorial guards;
that is parameterization, not validation. C++ focused evidence includes 2/2
and 3/3. Active RTL evidence is isolated 2,2,1 and 3,3,1 only.

## CAM accounting audit

Generic RECAM gives `AddressEntries=R+C` and
`HybridEntries=R(C-1)+C(R-1)`. Active RTL envelope counts agree:

| Configuration/envelope | address entries / bits | hybrid entries / bits | Evidence |
|---|---|---|---|
| 2,2,m=1 V2 envelope | 5; logical 17, physical mode-reused 28 | 7; logical 14, physical 20 | V2 params, Phase-3B interface, H2 sizing |
| 3,3,m=1 V2 envelope | 7; logical 17, physical 28 | 17; logical 14, physical 20 | H2/H3 isolated target sizing |
| generic C++ 2R2C baseline | 4; geometry dependent | 4; geometry dependent | paper-faithful generic model, not V2 envelope |

`RecamGeometry` and `HardwareMetrics` already distinguish storage, temporary
buffer, runtime view, matrix, and comparators. Dynamic CSV has active/peak/
provisioned entries and Hybrid bits, but lacks a complete per-run Address-bit,
candidate-history-bit, and ledger-bit contract. Required new categories are
`ONLINE_CAM_BITS`, `ANALYSIS_METADATA_BITS`, `GROUP_CANDIDATE_HISTORY_BITS`,
`GROUP_LEDGER_BITS`, and `OVERLAP_RETAINED_STATE_BITS`; do not call all state
“CAM.”

## Existing output schema matrix

| Program/output | Current fields by category | Keep / R1 disposition |
|---|---|---|
| Dynamic stdout | CONFIG, REPAIR, output paths | progress only |
| `attempts.csv` | CONFIG; FAULT; REPAIR candidate/result; CAM active/peak/provisioned; DEBUG bitmap/index; latency | retain raw diagnostic output |
| `runs.csv` | CONFIG; FAULT A/B/C/D; REPAIR A–D/group/C1-legacy-global fields; RESOURCE use/borrow/lend/remaining; CAM; DEBUG bitmaps/work/reason | retain, add versioned derived C1/C2/C3 and imbalance fields |
| `summary.csv` | CONFIG; repair rates/gain; resource/CAM/matrix/proxy averages; selector aggregates | retain, label proxies analytical and add occupancy quantiles |
| SRAM metrics/JSON | CONFIG; SRAM search/latency/analytical cost; CAM-baseline equivalence | retain separately; not RTL area |
| Hierarchical stdout / device summary | CONFIG/version/scope; DEVICE group/bank/domain result; CAM capacity/occupancy/overflow; BIRA | retain authoritative device summary |
| Hierarchical groups / engines CSV | CONFIG/address; FAULT total; REPAIR tier; RESOURCE borrow; CAM; DEVICE/BIRA timing | retain, add per-SA fields only under new schema version |

No legacy header should be renamed. Add a side-by-side versioned group schema:

```text
CONFIG: experiment_id, simulation_level, topology, sharing_policy,
solution_policy, RS, CS, share_row, share_col, m, group_fault_count,
fault_distribution_mode, imbalance_parameter, seed, sample_count
FAULT: fault_A..fault_D, fault_mean, fault_stddev, fault_min, fault_max, fault_range
REPAIR: repairable, group_repair_rate, device_repair_rate, failure_reason,
EARLY_PASS, GROUP_GREEDY_PASS, GROUP_GLOBAL_PASS, EARLY_FAIL_GREEDY_PASS,
EARLY_FAIL_GLOBAL_PASS, GREEDY_FAIL_GLOBAL_PASS, ALL_PASS, ALL_FAIL
RESOURCE: private/shared row/col used, remaining row/col, borrow_events, release_events
CAM: provisioned Address/Hybrid entries/bits/totals and used entries with mean/median/p95/max
```

## R1 architecture and priority

Use `DynamicRepairSimulator` through `DynamicSpareSharing` as one C++
point-per-invocation executable. A versioned manifest/runner owns sweep
enumeration. Keep SRAM as an explicit backend comparator and hierarchical as a
device wrapper; do not create a fourth simulator.

| Priority | R1 item |
|---|---|
| REQUIRED_FOR_R2 | multinomial allocator; derived imbalance fields; versioned schema; corpus signature/replay; physical-budget metadata; 1×4 workflow |
| REQUIRED_FOR_R3 | approved/named C3 objective; bound/count tuple work; paired C1/C2/C3 counters; resolve C2 C++/RTL priority |
| REQUIRED_FOR_R4/R5 | device 1×4 support; device paired-policy preflight; complete CAM capacity/occupancy and RTL-calibrated geometry join |
| OPTIONAL | controlled Dirichlet-multinomial model; historical analyzer export |

## Validation and final status

Focused C++ evidence passed:

```text
make test_solution_take_policy
make test_dynamic_spare_sharing_layout
make test_hierarchical_recam
make test_directional_multi_config_analyzer
make test_dynamic_spare_sharing_policy
```

```text
REPAIR_RATE_R0_STATUS: COMPLETE
DYNAMIC_SPARE_SHARING_CPP_ROLE: group-scope primary repair/policy core
DYNAMIC_SPARE_SHARING_SRAM_RECAM_CPP_ROLE: group-scope SRAM backend comparator
HIERARCHICAL_RECAM_CPP_ROLE: device WoW scheduler with one persistent global CAM pool
RECOMMENDED_UNIFIED_CORE: DynamicRepairSimulator via DynamicSpareSharing
EXP_A_GROUP_LEVEL: READY
EXP_A_DEVICE_LEVEL: MISSING
EXP_B_LOCAL: READY
EXP_B_DIRECTIONAL_M1: READY
EXP_B_DIRECTIONAL_M2: PARTIAL
EXP_B_PAIRWISE_ROW1: READY
EXP_B_PAIRWISE_ROW2: PARTIAL
EXP_B_1X4_ROW_ONLY: PARTIAL
EXP_C_EARLY: READY
EXP_C_GROUP_GREEDY: PARTIAL
EXP_C_GROUP_GLOBAL: PARTIAL
GROUP_GLOBAL_CPP: PARTIAL
MAX_CANDIDATES_PER_SA: 16
MAX_GROUP_COMBINATIONS: 65536
CPP_GROUP_GREEDY_MATCHES_CURRENT_RTL: NO
FIXED_GROUP_TOTAL_FAULT_COUNT: SUPPORTED
FIXED_TOTAL_MULTINOMIAL: NEEDS_WORK
SA_FAULT_STDDEV_OUTPUT: NEEDS_WORK
CONTROLLED_IMBALANCE_SUPPORT: EASY_EXTENSION
SAME_CORPUS_ACROSS_POLICIES: YES
SAME_CORPUS_ACROSS_TOPOLOGIES: YES
RS_CS_SWEEP: parameterized; only focused values are verified
VERIFIED_RS_CS_VALUES: C++ 2,2 and 3,3; active RTL 2,2,1 and 3,3,1
FAULT_COUNT_SWEEP_RANGE: proposed 8..32 step 4; not run
GROUP_LEVEL_SUPPORT: ready 2x2; 1x4 core ready, workflow/schema pending
DEVICE_LEVEL_SUPPORT: true device model; 1x4 missing
SHARED_ONLINE_CAM_DEVICE_MODEL: GLOBAL_LOGIC_DIE, persistent across groups
CAM_CAPACITY_ACCOUNTING: PARTIAL
CAM_OCCUPANCY_ACCOUNTING: PARTIAL
CAM_ACCOUNTING_MATCHES_ACTIVE_RTL: PARTIAL
EXISTING_OUTPUT_SCHEMA_AUDITED: YES
UNIFIED_OUTPUT_SCHEMA_PROPOSED: YES
RESOURCE_FAIRNESS_MATRIX: COMPLETE
MISSING_R1_FUNCTIONS: multinomial allocator; C3 contract/counters; schema; C2 priority resolution; 1x4 workflow
R1_PRIORITY_ORDER: R2 foundations -> R3 policy contract -> R4/R5 device preflight
RECOMMENDED_SWEEP_EXECUTABLE: DynamicSpareSharing
RECOMMENDED_ORCHESTRATION: versioned manifest plus scope-aware runner
FORMAL_SWEEP_RUN: NO
RTL_MODIFIED: NO
SYNTHESIS_RUN: NO
GIT_DIFF_CHECK: FAIL (pre-existing dirty worktree; R0 adds only the two named audit documents)
AUDIT_DOCUMENT: docs/dss_execution/REPAIR_RATE_SWEEP_READINESS_AUDIT.md
FEATURE_MATRIX: docs/dss_execution/REPAIR_RATE_FEATURE_MATRIX.md
NEXT_RECOMMENDED_PHASE: R1_IMPLEMENTATION_PLAN_REVIEW
```
