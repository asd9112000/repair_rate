# S0R — GROUP-NoScratch Semantic Alignment Audit

> Status: IMPLEMENTATION AND EQUIVALENCE COMPLETE (2026-09-14)

## Scope and reuse audit

| Component | Evidence | Classification | Reason |
|---|---|---|---|
| `PhysicalResourceLedger` | `src/PhysicalResourceLedger.cpp` | REUSE_AS_IS | Single physical-spare authority; supports sequential allocation and directional ownership. |
| RECAM candidate analysis / `TileSolutionState` | `src/DynamicRepairSimulator.cpp` | REUSE_AS_IS | Supplies per-SA valid Config/Pattern information. |
| `GroupCompressed` selector | `DynamicRepairSimulator::findCompressedGroupChoice` | REPLACE_POLICY_IMPLEMENTATION | Enumerates cross-SA candidate combinations before a final allocation. |
| EARLY selector | `DynamicRepairSimulator.cpp` | REUSE_AS_IS | Already sequential A→B→C→D and is frozen. |
| topology ownership | `PhysicalResourceLedger` | REUSE_AS_IS | Do not duplicate directional donor rules. |
| Dynamic runners / CSV | `DynamicSpareSharing.cpp`, reporter | EXTEND_EXISTING | Need an unambiguous explicit policy name. |
| `HierarchicalRECAM` interface | `HierarchicalRecamSimulator.cpp` | EXTEND_EXISTING | Consumes group analysis but must retain device-global CAM ownership. |

## Semantic diff

| Policy step | Existing `GroupCompressed` | Frozen V2 GROUP-NoScratch | Status |
|---|---|---|---|
| Candidate generation | Per-SA candidate information | Per-SA candidate information | MATCH |
| Candidate ordering | Inputs to cross-SA product search | Fixed resource-pressure-aware rank | MISMATCH |
| SA order | Combination-wide selection | A→B→C→D | MISMATCH |
| Ledger feasibility | Evaluated for candidate combinations | Checked against current committed ledger | MISMATCH |
| Resource commit | Final logical allocation | Immediate selected-candidate commit | MISMATCH |
| Failure handling | May find another combination | First infeasible SA terminates | MISMATCH |
| Rollback | Implicitly permitted by speculative search | None | MISMATCH |
| Final selection | Feasible best combination | First valid deterministic candidate | MISMATCH |

## Primary mismatch and implementation boundary

`findCompressedGroupChoice` is explicitly documented by the conflict log as a
cross-SA enumeration / best-feasible allocation.  This can succeed when a
prior alternate choice would be needed after a downstream failure.  The frozen
RTL policy commits A→B→C→D immediately and must fail in that case.

S0R shall preserve `GroupCompressed` as the historical policy and add an
explicit `GROUP_NO_SCRATCH_V2` policy.  It will reuse candidate analysis and
`PhysicalResourceLedger`, process A→B→C→D, select the first valid candidate in
the frozen rank, commit immediately, and stop without rollback.  S0R random
corpora are functional-equivalence evidence, never repair-rate results.

## Closure evidence

The new `group_no_scratch_v2` selector is separate from both Early and the
historical `group_compressed_legacy` selector. Its explicit constexpr mapping
uses role-slot order, selects the smallest one-based PatternID in each feasible
configuration, checks the current `PhysicalResourceLedger`, commits immediately,
and terminates at the first SA without a selectable slot. No RTL or persistent
device-CAM implementation was changed.

The focused test suite covers all 16 role-slot mapping entries, eight directed
borrow/release cases spanning A/B/C/D and all four actions, a fixed-priority
Early distinction, and an anti-backtracking case where V2 fails at C while the
legacy combination selector passes. The independent golden re-executes the
mapping and ledger decisions without invoking the production V2 selector.

```text
S0R_DIRECTED_V2 = 8/8 PASS
S0R_RANDOM_2_2_1 = 1000 vectors, seed 20260914, 0 mismatches in all categories
S0R_RANDOM_3_3_1 = 1000 vectors, seed 20260915, 0 mismatches in all categories
EARLY_REGRESSION = PASS
GROUP_COMPRESSED_LEGACY_REGRESSION = PASS
PHYSICAL_RESOURCE_LEDGER = REUSED_AS_IS
H0S0-CL-004 = CLOSED
S1_ELIGIBLE = YES
S1_AUTHORIZED = NO
NEXT_PHASE_AUTHORIZED = NONE
```
