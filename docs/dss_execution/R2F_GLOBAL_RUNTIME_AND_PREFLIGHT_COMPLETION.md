# R2F — Single-Hop GLOBAL Runtime Bound and R2 Completion

> Status: COMPLETE. Characterization is non-formal; completed R2 results are
> group-scope preflight data, not DATE formal evidence.

## 1. Root-cause profile

The old Single-Hop GLOBAL representation retained every PatternID over every
capacity attempt, then enumerated the full `A×B×C×D` product and called the
ledger only at leaves. On the fixed N=2, m=1, F=8 ten-group corpus, candidates
were A=15.5, B=27.4, C=26.5, D=14.3 on average: 160,334 leaf tuples/group
(maximum 246,016), 99.0% legal. Runtime was 73,022 us/group mean, 111,930 us
maximum. Thus PatternID/capacity multiplicity and leaf-only checking—not an
illegal donor path—were dominant.

## 2. Old search complexity

The previous exact selector had `O(|A||B||C||D|)` complete ledger calls.
Middle donors remained topology-distinct in the ledger, but donor branching
was not the source of the large product.

## 3. Optimization performed

GLOBAL now retains one representative per `(usedRows, usedColumns)` demand per
SA: the lexicographically lowest `(PatternID, attemptIndex)`. Ledger legality
depends only on the demand vector, while the frozen objective ranks borrowed
lines, used lines, PatternID tuple, then attempt tuple. All other equal-demand
candidates are therefore dominated. Reversible A→B→C→D DFS invokes the ledger
on each partial demand vector and prunes monotonic failures. No search cutoff
or heuristic fallback was added.

## 4. Semantic equivalence

`make test_solution_take_policy` now runs 1,000 randomized Single-Hop GLOBAL
oracle vectors plus directed tests: repairability, selected canonical candidate
tuple, and final ledger owner mismatches are zero. R1B Pair-Global and existing
2x2 GLOBAL oracle checks also remain zero-mismatch.

## 5. Runtime before/after

For the identical N=2, m=1, F=8 ten-group corpus, mean GLOBAL runtime changed
from 73,022 us to 319 us/group (maximum 347 us): about 229× faster. The 100-
group characterization p95 was 343 us for N=2/F=8 and 6,603 us for N=3/F=12.

## 6. Search nodes before/after

N=2/F=8 changed from 172,154 mean nodes and 160,334 complete checks/group to
46 mean nodes and 19 complete checks/group. Partial pruning is small at low
load because most reduced tuples are legal; demand-state reduction provides the
bound.

## 7. Worst-case observed group

The largest 100-group characterization runtime was N=3, m=1, F=12, group 71:
6,728 us, candidate demand counts `4|3|4|6`, and 313 visited nodes. This is
bounded characterization-only evidence, not a formal repair-rate datum.

## 8. Imbalance output

`dss_r2_group_preflight_v1` raw records now contain `fault_A..D`, mean,
population standard deviation, min, max, and range. The formula is
`sqrt(sum((F_i-mean)^2)/4)`. `imbalance/fault_stddev_policy_repairable.csv`
is generated solely from the frozen corpus counts.

## 9. Completed preflight matrix

R2 completed 120 policy/configuration aggregates and 120,000 raw records:
N=2 at F={8,12,16,20}; N=3 at F={12,18,24,30}; 1,000 groups each. All 120,000
records are `RESOURCE_MATCHED`, all group totals equal F_GROUP, and eight
distinct corpora are replayed policy/topology-identically.

## 10. Unsupported combinations

Device-level 1x4 remains unimplemented. CAM is `GENERIC_CPP_CAPACITY` for
2x2 and `NOT_PROVEN`/`NA` for 1x4; neither is used as a hardware-cost claim.

## 11. Characterization-only counterexamples

The completed paired output found 9 `GREEDY_FAIL_GLOBAL_PASS` and 73
`TWO_PAIRWISE_FAIL_SINGLE_HOP_PASS` groups out of 8,000 paired group rows.
These are preflight characterization counts, not formal claims.

## 12. Formal sweep readiness decision

Exact GLOBAL is now bounded and observable without truncation. R2 is complete
and the next authorized phase is a separately launched R3 formal group sweep;
this R2F work does not launch it.
