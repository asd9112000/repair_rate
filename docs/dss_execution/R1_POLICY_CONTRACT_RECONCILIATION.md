# R1 Policy Contract Reconciliation

> Status: PARTIAL. Scope: group-level `DynamicRepairSimulator` policy
> semantics. This is not a formal repair-rate result. No RTL was changed and
> no synthesis or repair-rate sweep was run.

## Decision

Two explicit C++ policy identities were added without changing historical
ones:

| CLI policy | Identity and contract |
|---|---|
| `group_no_scratch_v2` | historical C++ V2 GROUP behavior, slot order `0,1,2,3` |
| `group_greedy_rtl_canonical` | canonical GROUP-GREEDY control order, RTL rank `1,0,3,2` |
| `group_compressed_legacy` (and historic `group`) | retained historical joint-search policy |
| `group_global` | canonical name for the retained joint-search engine and the frozen C3 contract below |

The canonical name was selected before implementation. The historical name and
its aliases retain their former behavior.

## Evidence table

| Item | Active RTL | Current C++ / R1 canonical | Historical C++ | Match? | Evidence |
|---|---|---|---|---|---|
| SA allocation order | A→B→C→D after collection | A→B→C→D after all attempts are retained | same for V2 | PASS | `recam_dss_v2_group_core.sv` collection/allocation states; `findV2GroupNoScratchChoice` |
| role-slot order | ranks `1,0,3,2` | `v2RtlGroupRoleSlotMappings` returns `1,0,3,2` | `v2RoleSlotMappings`: `0,1,2,3` | canonical PASS; legacy intentionally differs | `dss_v2_group_priority_reader.sv:26-37` |
| Pattern priority | stored PatternID for a selected slot | lowest valid `solutionId` within the role slot | same | PARTIAL | C++ `compressedPlansForSubarray` order; RTL reader exposes one stored PatternID. Multi-pattern-in-slot hardware arbitration is not present in the inspected RTL. |
| candidate retention | four `{valid, PatternID}` records/SA, all collected first | all compressed candidates retained; V2 policy selects from a role slot | same | PARTIAL | RTL candidate store; `compressedPlansForSubarray` |
| ledger legality | control core gates allocation with `accept` | `PhysicalResourceLedger::allocateSequential` | same | PARTIAL | RTL top core has no inspected line-level ledger implementation; C++ topology ledger is the simulator model. |
| rollback | none after an accepted SA | none | none | PASS | RTL allocation flow; sequential C++ commitment |
| backtracking | none | none | none | PASS | same sources |
| joint combination search | none | none for canonical greedy | only C3 uses it | PASS | `findCompressedGroupChoice` is not called by canonical greedy |
| C3 tie-break | not an RTL function | defined below | existing deterministic ranking retained | PASS | `compressedChoiceIsBetter` |

`DATA_INSUFFICIENT`: the generic frozen-Date 2×2 slot decoder maps ConfigIDs
oppositely to the RS3 target config table for A/D versus B/C
(`rtl/dss_v2/group/dss_v2_group_slot_decode.sv:15-39` versus
`rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_config_table.sv:31-47`). The control
priority is unambiguous, but one universal numeric ConfigID-output contract
cannot truthfully be claimed without identifying the selected RTL target/table.
The R1 C++ mapping follows the RS3 target table and existing C++ V2 encoding.

## C1 EARLY and C2 GROUP-GREEDY contracts

`Early` in `findEarlyChoice` considers candidate plans for A, then B, C, D.
At each SA it ranks legal choices by borrowed lines, used physical lines,
PatternID, and attempt index, commits through `allocateSequential`, and never
revisits a prior SA.

The active GROUP reader rank order is exactly slot `1,0,3,2`:

| Role | slot 1 | slot 0 | slot 3 | slot 2 |
|---|---|---|---|---|
| A/D, RS=CS=3,m=1 | Config 1: 2R3C release | Config 0: 3R3C local | Config 3: 2R4C release+borrow | Config 2: 3R4C borrow |
| B/C, RS=CS=3,m=1 | Config 4: 3R2C release | Config 0: 3R3C local | Config 6: 4R2C release+borrow | Config 5: 4R3C borrow |

Thus R0's mismatch was not ConfigID sorting: the historical C++ V2 scans
`0,1,2,3`, while RTL ranks `1,0,3,2`. A/D changes Config order
`0,1,2,3 → 1,0,3,2`; B/C changes `0,4,5,6 → 4,0,6,5`.
`group_greedy_rtl_canonical` retains all candidates first, starts allocation
only after collection, scans the RTL rank per A→B→C→D, selects the first
ledger-legal candidate, commits immediately, and has no rollback,
backtracking, or tuple search. The directed regression demonstrates that this
priority can change selected ConfigID/PatternID and ledger state. Historical
results remain reachable under `group_no_scratch_v2`.

## C3 audit and frozen GROUP-GLOBAL contract

`findCompressedGroupChoice` builds `compressedPlansForSubarray` for each SA.
It recursively forms the Cartesian product in A/B/C/D vector order
(`DynamicRepairSimulator.cpp:789-843`). It does not prune partial tuples;
every full tuple increments `solutionSelectionWork`, becomes four
`SpareDemand`s, and receives `PhysicalResourceLedger::allocate`. The ledger
checks physical-line ownership, topology accessibility, row/column conflicts,
reserve rules, and multiple consumers. Tuples exceeding `max-borrows` are
rejected. The first empty SA candidate set causes full-group failure.

Repair success is: **there exists one legal tuple with exactly one valid
PatternID-level candidate for every A/B/C/D SA.** No partial repair tuple is a
GROUP-GLOBAL success.

The existing deterministic objective is now frozen for `group_global`:

1. minimize borrowed rows for row-only layouts, otherwise total transfers;
2. minimize used rows for row-only layouts, otherwise total used physical lines;
3. lexicographically minimize A/B/C/D PatternID (`solutionId`);
4. lexicographically minimize A/B/C/D attempt-vector index.

This preserves useful existing behavior and provides a deterministic selected
output. `applyAllocation` performs the only commit, after selection. On
failure, output carries no selected tuple and reports
`NO_FEASIBLE_GROUP_COMBINATION`.

The implementation is exhaustive over its actual candidate vectors, not merely
"more global" than greedy. For the generic 2×2 directional RS=CS=2,m=1 bound,
each SA has at most `C(4,2)+C(5,2)=16` candidates and at most `16^4=65,536`
full tuples. For RS=CS=3,m=1, the implementation bound depends on every
configured capacity attempt: it is the product of the retained valid bitmap
memberships, bounded by the product of their `C(R+C,R)` candidate spaces.
There is no fixed RS3 number in code because candidate vectors are data driven.

PatternID-level search is required. `CandidatePlan` is produced per valid
bitmap position and uses `decodeSolution` to reconstruct `usedRows` and
`usedColumns`. The existing `GreedyLossSolver` regression has two valid
PatternIDs for A's same 2R2C attempt: Pattern 0 consumes 2R/0C and Pattern 1
consumes 1R/1C. Therefore Config-only search can reject a legal global tuple.
The representation contains enough ledger-relevant demand information for the
current ledger, but not a hardware-specific per-line identity beyond the
topology model.

The C3 engine can be reused for m=2 and for 1×4/pairwise row-only because it
delegates legality to `PhysicalResourceLedger`; this is source support, not
formal validation for every matrix. Device scheduling uses the same
`DynamicRepairSimulator` core through `HierarchicalRecamSimulator`, rather
than an independently implemented C1/C2/C3 selector.

## Same-corpus and output contract

`DynamicSpareSharing --repair-rate-sweep` generates a corpus once and replays
it by policy; `DynamicRepairSimulator::run` receives `const FaultGroup&` and
does not own fault RNG. R1 replayed the same four-SA fixture across C2/C3 and
verified it was not mutated. The hierarchical path can write a corpus artifact
with `--write-fault-corpus`. Policy evaluation does not advance generator RNG.

R2/R3 must add a versioned sidecar rather than rename existing CSV columns.

| Destination | Required fields |
|---|---|
| raw per-group | corpus_id, seed, group_index, topology, sharing/solution policy, RS, CS, m, F_GROUP, A/B/C/D counts, repair success, selected Config/Pattern A-D, borrowed/released row/column counts, remaining row/column spares |
| aggregate summary | group/device repair rate (separate scopes), imbalance repair rate, remaining-spare distribution, policy counts |
| debug trace | all C2 role trials; C3 candidate tuple count, legal tuple count, selected tuple and ledger trace |

Existing `runs.csv` already has seed/run index, topology, policy, per-SA fault
counts, resource usage, selected C2 IDs, remaining resources, selection work,
and feasible combinations. It lacks a corpus signature, explicit m,
fault-stddev, released counts, and a versioned C1/C2/C3 comparison schema.
CAM provisioned/active/peak fields remain accounting-specific; no new CAM
semantics are inferred here.

## Characterization and regression evidence

All characterization is **NOT FORMAL REPAIR-RATE RESULT**. The focused test
uses a directed all-feasible priority case, the existing GreedyLoss fixture,
and 128 deterministic synthetic replay vectors (`runIndex` 5000–5127).

| Check | Result |
|---|---|
| historical V2 vs canonical, 128 vectors | success mismatch 0; Config mismatch 128; Pattern mismatch 128; final ledger mismatch 128; first counterexample 5000 |
| canonical replay, same 128 vectors | 0 mismatch |
| directed global vs canonical greedy | FOUND: greedy fails; global succeeds |
| global wrapper vs `group_compressed_legacy` | identical success, selected candidates, resource counts, work, and legal-count output |

The global-vs-greedy fixture is a synthetic candidate-state regression, not
fault-coordinate hardware evidence. It preserves candidate sets and ledger
trace in `tests/solution_take_policy_test.cpp` and must not be cited as a
repair-rate delta.

## Feature readiness update

| Area | Status | Evidence / limitation | Minimum next action |
|---|---|---|---|
| 2×2 group | READY | common core and ledger | versioned reporting |
| 1×4 group | PARTIAL | row-only neighbor source support; C2/C3 contract not focused-tested | focused policy matrix |
| directional m=1 | READY for group C1/C2/C3 control | RS3 numeric ConfigID target ambiguity above | select target RTL table |
| directional m=2 | PARTIAL | ledger support; no canonical policy regression | focused regression |
| pairwise row-only | PARTIAL | ledger support; no canonical policy regression | focused regression |
| group C1 EARLY | READY | `findEarlyChoice` | schema fields |
| group C2 canonical | PARTIAL | control rank matches; generic/base ConfigID table conflict | identify RTL target |
| group C3 global | READY at group-core semantics | output schema is specified, not yet emitted as sidecar | implement R2 schema |
| device policies | PARTIAL | common core plus device scheduler; no paired device campaign | paired corpus preflight |
| CAM accounting | PARTIAL | existing provisioned/active fields are not a unified policy schema | versioned accounting fields |
| SRAM accounting | PARTIAL | separate SRAM executable/model | keep separate scope and schema |
| fixed F_GROUP/natural allocation/spatial controls | READY/PARTIAL | fixed total and spatial models exist; no multinomial allocator | add only if R2 requires it |
| same-corpus replay | READY group / PARTIAL device | group const input verified; device corpus artifact exists | corpus_id in all outputs |

## Changes and blockers

Implemented: new enum/CLI names; RTL-rank mapping; canonical greedy dispatch;
global canonical wrapper; compatibility preservation; focused regressions.

R1A resolved the apparent conflict as architecture-point-specific ConfigID
contracts, added RS3 lockstep evidence, and implemented the paired corpus
sidecars. See `R1A_CONFIGID_AND_CORPUS_CONTRACT_CLOSURE.md`. R1 remains
PARTIAL only because R1B's topology-specific 1×4 policy implementation is not
authorized yet. No historical raw file was written or replaced.
