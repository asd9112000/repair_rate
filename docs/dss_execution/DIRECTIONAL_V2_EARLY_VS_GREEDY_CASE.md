# EARLY versus GROUP_GREEDY semantic audit

## Decision

`EARLY_GREEDY_SEMANTIC_AUDIT: PASS` for the conclusion that the historical
comparison is cross-contract, and therefore is **not** evidence of an intended
pure commitment-time advantage.

The old R3 policy ID `directional_m1_early` invoked solution-take `early`.
That selector uses generic `capacityOptions()` and generic compressed RECAM
candidates.  `directional_m1_group_greedy` invokes
`group_greedy_rtl_canonical`, uses `v2CapacityOptions()`, the frozen DATE V2
ConfigID table, and V2 role-slot priority `1,0,3,2`.  They were not the same
algorithm with different commit timing.

| dimension | historical `early` | `group_greedy_rtl_canonical` |
| --- | --- | --- |
| candidate generator | generic `capacityOptions` + compressed candidates | `v2CapacityOptions` + frozen V2 slots |
| V2 capacity slots / ConfigID mapping | no | yes, `FrozenDate2x2M1` |
| PatternID generation | generic valid options | V2 valid options indexed by role slot |
| candidate priority | ranks all currently legal candidates by borrow/used-lines/PatternID/attempt | slot order `1,0,3,2`, first ledger-legal |
| SA order | A→B→C→D | A→B→C→D |
| other-SA/future feasibility before commit | no | no |
| ledger/reservation/release/borrow | same `PhysicalResourceLedger`, prefix reallocation | same ledger, prefix reallocation |
| commit / rollback / first failure | per-SA selected rank, no rollback, stop on first failed SA | first legal slot, no rollback, stop on first failed SA |
| retained trace | generic selected attempt/pattern only; no V2 ConfigID trace | V2 slot, ConfigID, action and ledger before/after trace |

```text
EARLY_AND_GREEDY_SAME_CANDIDATE_UNIVERSE: NO
EARLY_AND_GREEDY_SAME_CONFIG_SEMANTICS: NO
EARLY_AND_GREEDY_SAME_LEDGER: YES
EARLY_AND_GREEDY_SAME_SA_ORDER: YES
EARLY_AND_GREEDY_SAME_CANDIDATE_PRIORITY: NO
EARLY_AND_GREEDY_DIFFER_ONLY_IN_COMMITMENT_TIME: NO
EARLY_GREEDY_THEORETICAL_EQUIVALENCE_EXPECTED: NO
DOES_GROUP_GREEDY_EVALUATE_FUTURE_SA_FEASIBILITY_BEFORE_COMMIT: NO
```

The concrete differences breaking equivalence are candidate universe,
ConfigID/role-slot semantics, and candidate priority—not future-SA look-ahead
or delayed allocation.

## Existing historical 14k corpus result

All rows were joined on the same `(N,F_GROUP,group_id)` corpus identity.

| N | F_GROUP | EARLY pass / GREEDY pass | EARLY pass / GREEDY fail | EARLY fail / GREEDY pass | both fail |
| --- | ---: | ---: | ---: | ---: | ---: |
| 2 | 8 | 1000 | 0 | 0 | 0 |
| 2 | 12 | 991 | 0 | 1 | 8 |
| 2 | 16 | 962 | 0 | 10 | 28 |
| 2 | 20 | 865 | 0 | 37 | 98 |
| 2 | 24 | 649 | 0 | 123 | 228 |
| 2 | 28 | 432 | 0 | 143 | 425 |
| 2 | 32 | 213 | 0 | 140 | 647 |
| 3 | 8 | 1000 | 0 | 0 | 0 |
| 3 | 12 | 1000 | 0 | 0 | 0 |
| 3 | 16 | 996 | 0 | 1 | 3 |
| 3 | 20 | 993 | 0 | 0 | 7 |
| 3 | 24 | 953 | 0 | 12 | 35 |
| 3 | 28 | 880 | 0 | 24 | 96 |
| 3 | 32 | 713 | 0 | 77 | 210 |
| **total** | | **11647** | **0** | **568** | **1785** |

```text
TOTAL_GROUPS_CHECKED: 14000
EARLY_PASS_GREEDY_PASS: 11647
EARLY_PASS_GREEDY_FAIL: 0
EARLY_FAIL_GREEDY_PASS: 568
EARLY_FAIL_GREEDY_FAIL: 1785
FIRST_EARLY_FAIL_GREEDY_PASS_CASE: N=2, F_GROUP=12, group_id=738
EARLY_GREEDY_UNEXPECTED_DIVERGENCE: NO
```

Case 738 has SA fault counts A=2, B=1, C=6, D=3.  Replaying its exact faults
confirms generic EARLY fails and V2 GROUP_GREEDY passes (selected ConfigIDs
`4,1,2,4`, PatternIDs `1,1,3,1`).  The generic selector has no ConfigID or
role-slot trace, so a requested ConfigID-by-ConfigID EARLY trace is
**unsupported for this historical policy**, rather than fabricated.  The
first differing decision is already at candidate construction/priority, before
a shared commitment can be compared.

## Matrix V2 closure

New solution-take `directional_v2_early` is explicit and does not alter the
historical `early` policy.  It uses the frozen V2 capacity slots and the same
ledger, A→B→C→D order, first-legal commitment, and no rollback as the V2
ladder, but priority `0,1,2,3`; GROUP_GREEDY retains its frozen `1,0,3,2`
priority.  A future V2 EARLY→GREEDY difference is therefore attributable to
**DIFFERENT_CANDIDATE_PRIORITY**, not to future-SA feasibility look-ahead.

The two gains must remain distinct:

- Gap A, EARLY→GROUP_GREEDY: effect of the two frozen V2 priorities.
- Gap B, GROUP_GREEDY→V2 GROUP_GLOBAL: exhaustive joint tuple search versus
  sequential first-legal commitment under the same V2 candidate/ledger contract.

No V2-EARLY 1k/14k sweep was started by this closure task.  Therefore the 568
historical cross-contract cases must not be plotted or reported as a Matrix V2
algorithmic gain.

```text
ACTUAL_SEMANTIC_DIFFERENCE: GENERIC_RECAM versus DIRECTIONAL_V2 candidate contract and priority
FIRST_DIFFERING_DECISION: candidate universe / priority construction before SA-A commit
EARLY_FAILURE_MECHANISM: historical generic selector has no V2-equivalent candidate contract
GREEDY_RESCUE_MECHANISM: frozen V2 candidate/slot contract selects a legal tuple
SAFE_TO_INTERPRET_GREEDY_GAIN_AS_ALGORITHMIC: NO (for historical EARLY data)
```
