# Directional V2 Group-Global Oracle Implementation

## Status

```text
STATUS: COMPLETE — correctness validation complete; not added to Matrix V2 canonical policy list
NEW_POLICY_NAME: directional_m1_v2_group_global
INTERNAL_SOLUTION_POLICY: directional_v2_group_global
CANDIDATE_CONTRACT: V2
USES_V2_CAPACITY_OPTIONS: YES
USES_SAME_PHYSICAL_LEDGER_AS_GREEDY: YES
KNOWN_N2_F16_G51_REPLAY: PASS
GREEDY_PASS_V2_GLOBAL_FAIL: 0
V2_GLOBAL_PASS_GREEDY_FAIL: 46
BOTH_PASS: 12215
BOTH_FAIL: 1739
FIRST_TRUE_GREEDY_SUBOPTIMAL_CASE: N2 / F_GROUP=16 / group_id=172
PRODUCTION_VS_BRUTE_FORCE_MISMATCHES: 0
SAFE_TO_USE_AS_DIRECTIONAL_V2_GLOBAL_ORACLE: YES
SAFE_TO_ADD_TO_MATRIX_V2_CANONICAL: NO — correctness is established, but the task explicitly leaves policy-list adoption to a later decision
GENERIC_GLOBAL_SEMANTICS_CHANGED: NO
FORMAL_100K_DATA_TOUCHED: NO
HISTORICAL_SWEEP_REWRITTEN: NO
```

The 14k validation source is classified `QUICK_SWEEP / DEVELOPMENT /
NON-FORMAL`; it is not a new formal result and does not modify the existing
quick sidecars.

## Implementation boundary

`SolutionTakePolicy::DirectionalV2GroupGlobal` is a new, separate policy.
The CLI spelling is `--solution-take directional_v2_group_global`; the
explicit R3 descriptor name for a future policy matrix is
`directional_m1_v2_group_global`.

It reuses exactly the components used by frozen DATE V2 GREEDY:

- `v2CapacityOptions`, including local, release-only, borrow-only, and
  release-and-borrow capacity slots;
- frozen V2 `roleSlot -> ConfigID -> action` mapping;
- existing RECAM local analysis, valid PatternID generation, and retained
  matrix solution state;
- `PhysicalResourceLedger::allocateSequential` and its directional physical
  ownership/borrowing constraints.

The only changed variable is solution selection.  GREEDY commits A-to-D in
its frozen rank order.  V2 GLOBAL enumerates every V2 attempt/PatternID local
candidate in A/B/C/D order, applies the same ledger to each prefix, and stops
at the first legal complete tuple.  Its stable enumeration order is role slot
`0,1,2,3`, then ascending PatternID.  It deliberately does **not** reuse
historical generic `GroupGlobal` capacity generation or collapse V2 entries
by `(usedRows, usedColumns)`.

Historical `directional_m1_group_global` remains unchanged.  It is still a
generic RECAM global search, not an oracle for the frozen V2 candidate
contract.

## Instrumentation and output identity

`paired_policy_results_v1.csv` now carries these global search fields for
fresh outputs:

```text
global_candidate_count_A..D
global_raw_cartesian_product_size
global_search_nodes_visited
global_partial_assignments_pruned
global_complete_assignments_checked
global_legal_complete_assignments
global_stopped_at_first_legal
global_exhaustive_enumeration
global_termination_reason
```

For the production V2 policy, `global_stopped_at_first_legal=1`,
`global_exhaustive_enumeration=0`, and the normal successful termination is
`FIRST_LEGAL_COMPLETE_TUPLE`.  Independent diagnostic brute-force enumeration
remains in `tests/directional_v2_group_global_test.cpp`; it fully enumerates
the known witness and compares repairability independently for random N2/N3
groups.

New policy outputs are distinguishable by all of:

```text
policy_id=directional_v2_group_global
solution_policy=directional_v2_group_global
config_contract_version=frozen_date_2x2_m1 (N2)
config_contract_version=rs3_cs3_m1 (N3)
```

The temporary validation manifest, sidecars, logs, and replay corpus are under
[`tmp/directional_v2_global_validation/quick14k_20260918`](../../tmp/directional_v2_global_validation/quick14k_20260918).

## Known audit-witness replay

The original audit witness N2/F16/group51 now has:

```text
V2 GROUP_GREEDY: PASS
historical generic GROUP_GLOBAL: FAIL
V2 GROUP_GLOBAL: PASS
V2 GLOBAL legal tuple found: YES
V2 GLOBAL matches full brute-force oracle: YES
```

The dedicated regression full-enumerates its V2 candidate product:

```text
candidate counts: A=3, B=15, C=23, D=23
complete tuples: 23,805
legal tuples: 6,693
```

This confirms the new policy contains the GREEDY candidate universe and fixes
the prior cross-contract oracle gap without changing historical generic
GLOBAL behavior.

## Paired 14k quick-corpus validation

The validator replays only the already materialized N2/N3 corpus, writes a
fresh temporary V2 GLOBAL sidecar, and fails immediately if a GREEDY-pass / V2
GLOBAL-fail row occurs:

```bash
python3 scripts/group/validate_directional_v2_group_global.py \
  --output-root tmp/directional_v2_global_validation/quick14k_20260918
```

Its reusable `--resume` mode accepts only already completed new-policy
sidecars in that same temporary root; it never reads or overwrites a formal
root.  Results:

| point | both pass | GREEDY pass / V2 GLOBAL fail | V2 GLOBAL pass / GREEDY fail | both fail |
|---|---:|---:|---:|---:|
| N2 F8 | 1000 | 0 | 0 | 0 |
| N2 F12 | 992 | 0 | 0 | 8 |
| N2 F16 | 972 | 0 | 1 | 27 |
| N2 F20 | 902 | 0 | 5 | 93 |
| N2 F24 | 772 | 0 | 7 | 221 |
| N2 F28 | 575 | 0 | 8 | 417 |
| N2 F32 | 353 | 0 | 19 | 628 |
| N3 F8 | 1000 | 0 | 0 | 0 |
| N3 F12 | 1000 | 0 | 0 | 0 |
| N3 F16 | 997 | 0 | 0 | 3 |
| N3 F20 | 993 | 0 | 0 | 7 |
| N3 F24 | 965 | 0 | 0 | 35 |
| N3 F28 | 904 | 0 | 3 | 93 |
| N3 F32 | 790 | 0 | 3 | 207 |
| **total** | **12215** | **0** | **46** | **1739** |

Thus the required hard invariant holds over every existing quick group:

```text
if V2 GREEDY == PASS, then V2 GROUP GLOBAL == PASS
```

## First true greedy-suboptimal case

The first observed V2 GLOBAL-only case is N2/F16/group172.  The complete
fault input and decision/ledger trace are retained at:

- [`n2_f16_group_172.simplified_faults`](../../tmp/directional_v2_global_validation/quick14k_20260918/traces/n2_f16_group_172.simplified_faults)
- [`n2_f16_group_172_trace.txt`](../../tmp/directional_v2_global_validation/quick14k_20260918/traces/n2_f16_group_172_trace.txt)

GREEDY commits A local (`2R,2C`) and B release-only (`0R,0C`), then reaches C
where only the borrow-only V2 candidate `3R,2C` is locally valid but the
previous commitment prevents a legal allocation.  V2 GLOBAL instead selects:

```text
A: slot 2 / Config 5 / Pattern 7 / 1R,3C
B: slot 0 / Config 0 / Pattern 1 / 0R,0C
C: slot 2 / Config 2 / Pattern 3 / 3R,2C
D: slot 0 / Config 0 / Pattern 1 / 2R,0C

transfers: B -> A column; A -> C row
final ledger: legal, uses 6R and 5C
```

This is the canonical observed example that frozen sequential commitment is
not a joint optimum.  It is a quick-development result only.

## Scaling check

Production stops at the first legal complete tuple, but counts the full raw
candidate product before DFS.  Maximum values over the 14k validation are:

| contract | max local candidates / SA | max raw tuple space | max DFS nodes | max complete tuples checked |
|---|---:|---:|---:|---:|
| N2 | 23 | 279,841 | 7,814 | 1 |
| N3 | 80 | 40,960,000 | 145,864 | 1 |
| N4 | unsupported | unsupported | unsupported | unsupported |

Frozen V2 ConfigID semantics are defined only for N2 and N3:
`canonicalV2ConfigContract()` rejects N4.  No generic/global substitution was
made.  If an N4 V2 contract is later frozen, its potentially large raw tuple
space requires exact memoization/DP or proven-safe branch-and-bound before a
formal exhaustive sweep; semantics must not be relaxed.

## Tests run

```bash
make test_directional_v2_group_global
make test_solution_take_policy
make test_dynamic_spare_sharing_layout
```

All passed.  The new test emits:

```text
test_directional_v2_global_known_witness PASS
test_directional_v2_global_contains_greedy PASS vectors=64
test_directional_v2_global_vs_bruteforce PASS mismatches=0
```

The existing `S0R_ANTI_BACKTRACKING v2=FAIL` characterization remains
unchanged; it is a test of frozen GREEDY's no-rollback behavior, not a failure
of the new V2 GLOBAL oracle.

## Matrix V2 integration decision

Matrix V2 now uses per-N policy membership.  For N2/N3 the directional m=1
ladder is `directional_v2_early`, `group_greedy_rtl_canonical`, and
`directional_v2_group_global`; historical `directional_m1_group_global` is
retained only as `GENERIC_GROUP_GLOBAL_LEGACY`.  N4 explicitly excludes this
V2 ladder because it has no frozen V2 ConfigID/candidate contract.  See
`DIRECTIONAL_V2_EARLY_VS_GREEDY_CASE.md` for the companion semantic audit.
