# R1B — 1×4 Row-Only Policy Implementation

> Group-level C++ implementation only. No RTL changed, no synthesis ran, and
> no formal repair-rate sweep was run.

## 1. Frozen resource model

For `RS=CS=N`, `share_row=m`, and `share_col=0`, every SA retains `N-m`
private rows, `m` shareable rows, and `N` local columns. The ledger constructs
exactly `4N` physical rows plus `4N` physical columns: `8N` lines per group.
`SimulationConfig::validate` already enforces positive RS/CS, `m<=RS`, and
zero shared columns for 1×4. R1B result metadata exposes private/shareable
counts and static/observed row resources in `paired_policy_results_v1.csv`.

## 2. Policy identifiers

| Identifier | Layout/topology | Selector |
|---|---|---|
| `one_by_four_two_pairwise_early_v1` | 1×4 / `pair` | immediate A→B then C→D commitment |
| `one_by_four_two_pairwise_pair_global_v1` | 1×4 / `pair` | independent AB and CD exhaustive pair search |
| `one_by_four_single_hop_early_v1` | 1×4 / `neighbor` | immediate A→B→C→D commitment |
| `one_by_four_single_hop_global_v1` | 1×4 / `neighbor` | full four-SA PatternID tuple search |

Mis-matched policy/layout/topology combinations fail validation. Existing 2×2
policy identities and behavior were not changed.

## 3. Two-Pairwise accessibility

`PairSharing` permits row transfers only A↔B and C↔D. The shared-line ledger
does not create B↔C access, forwarding, or a transitive donor. A line has one
physical owner and one final assignee at most.

## 4. Two-Pairwise EARLY

`findOneByFourEarlyChoice` uses the frozen v1 priority: **local-capacity
attempt first, then ascending extra-row capacity; within each retained attempt,
ascending PatternID; first ledger-legal plan commits.** This is deterministic,
immediate, and has no rollback. The sequence is A, B, C, D; pair independence
comes from the ledger boundary, not from a second topology implementation.

## 5–6. Pair-Global search and oracle

`findPairGlobalChoice` enumerates AB and CD separately. Each pair requires a
legal two-SA full repair and ranks candidates by: minimum borrowed rows,
minimum used rows, pair PatternID tuple, then pair attempt tuple. It combines
the two independent winners and performs one full-ledger verification/commit.
It never runs a four-SA Cartesian search.

The independent test oracle enumerates retained PatternID candidates, checks
joint ledger legality, and compares repairability, selected tuple, resource
counts, and final ledger owners. It passed 384 deterministic vectors across
`(RS,m)=(2,1),(3,1),(3,2)` with zero mismatches.

## 7–9. Single-Hop policies

`NeighborSharing` permits only adjacent row ownership boundaries AB, BC, and
CD. It forbids A→C/D and D→A/B, and borrowing does not expose a subsequent
hop. Single-Hop EARLY uses the same versioned attempt/Pattern priority above;
the physical line order resolves middle-SA donors left before right (B: A then
C; C: B then D). This is explicitly the `v1` experimental priority.

`one_by_four_single_hop_global_v1` reuses the verified PatternID-level global
selector and the neighbor ledger. Its bounded 128-vector oracle comparison had
zero mismatches.

## 10–11. Corpus and budget

All policies consume `const FaultGroup&` and use the existing
`dss_paired_corpus_v1` sidecar. The SA-local fault addresses are unchanged
between 2×2 and 1×4; only ledger accessibility changes. Tests confirm
`FAULT_CORPUS_CHANGED_BY_POLICY=NO` and exact 16-line total for N=2, including
`share_row=0` and `share_row=RS` edge cases.

## 12–14. Regression evidence

Directed coverage includes no borrowing; each legal pair-direction transfer;
over-m borrow failure; B/C isolation; independent AB/CD activity; local-only
degeneracy; full-share edge; fixed budget; and impossible allocations.
`GREEDY_FAIL_PAIR_GLOBAL_PASS` is a confirmed directed candidate-state fixture
where EARLY fails at B after A commits PatternID 0 while Pair-Global selects
PatternID 1. It is not a repair-rate statistic.

`make test_solution_take_policy` reports pair-global oracle mismatch 0 over
384 vectors and single-hop global mismatch 0 over 128 vectors. The mandated
2×2 regression commands also passed.

## 15–16. Remaining scope and CAM

Device-level 1×4 scheduling is not implemented. Existing CAM accounting is
reused because topology changes resource accessibility after candidate
generation; no new topology-specific CAM formula was invented. Its final
experiment calibration remains **PARTIAL** before R2.
