# R3 — DATE 2026 Formal Group-Level Repair-Rate Results

> Status: PARTIAL / RUNNING. This report is not a device-level result and
> contains no RTL or synthesis claim.

## Contract

R3 uses exactly 100,000 groups for each of the eight `(RS=CS,F_GROUP)` points
and replays one materialized `multinomial_uniform` corpus across the nine
canonical policy identities. The planned formal workload is 7,200,000
policy-group evaluations (800,000 unique physical groups). Seeds are frozen in
`results/date2026/repair_rate/r3_formal_group/manifest/r3_manifest.json`.

The raw output root is deliberately separate from R2 and has a completion
marker only after every point, same-corpus check, fixed-total check, and
applicable GLOBAL-dominance check passes.

## Correctness correction before formal collection

The R3 1,000-group launch gate exposed that GLOBAL was checking candidate
tuples with a different allocation order than EARLY. GLOBAL now checks its
reversible DFS prefixes and complete tuples with the same A→B→C→D sequential
ledger contract as EARLY. The independent directed/randomized GLOBAL oracles
were updated and pass. This is a correctness repair, not a policy objective or
search-cutoff change.

The canonical 2x2 `GROUP-GREEDY` uses the frozen `frozen_date_2x2_m1`
ConfigID candidate contract while generic `GROUP-GLOBAL` uses
`generic_recam_candidate_v1`; their paired outcomes are measured but are not
presented as a mathematical dominance assertion. The 1x4 Pair-Global and
Single-Hop GLOBAL comparisons do receive the per-group subset check.

Pre-fix GLOBAL rows in the retained R2 preflight and R2F characterization
directories are historical artifacts only: their GLOBAL repairability and
runtime observations must not be combined with post-fix R3 results. They are
not deleted. R3 N2/F8, and every subsequent formal point in this root, was
generated after the ledger-order repair.

## Policy Comparison Semantics

The policy label alone is insufficient to establish a dominance relation. A
comparison is Type A only when topology, physical-resource contract, candidate
universe, and final ledger legality contract are the same; it is Type B when a
candidate-generation or topology contract differs. Both types use identical
materialized corpora and may be reported as paired empirical comparisons.

| Comparison | Class | Same candidate universe | Dominance assertion | Paired statistic |
|---|---|---:|---:|---:|
| Two-Pairwise EARLY vs Pair-GLOBAL | A | yes | yes | yes |
| Single-Hop EARLY vs GLOBAL | A | yes | yes | yes |
| 2x2 GROUP-GREEDY vs generic GROUP-GLOBAL | B | no | no | yes |
| Two-Pairwise vs Single-Hop | B | no; topology differs | no | yes |
| Directional EARLY vs GROUP-GREEDY | B | no; policy contract differs | no | yes |

For Type A comparisons, a pass under EARLY must also pass under its exact
GLOBAL counterpart. For Type B comparisons, `A_PASS_B_FAIL` and
`A_FAIL_B_PASS` are descriptive paired outcomes, not correctness failures.

### 2x2 GROUP-GREEDY versus generic GROUP-GLOBAL

This is intentionally a Type B comparison in the current implementation.
`GROUP-GREEDY_RTL_CANONICAL` selects among four frozen V2 role slots per SA in
RTL rank `1,0,3,2`. For `RS=CS=2,m=1`, A/D expose `(2R,2C)`, `(1R,2C)`,
`(2R,3C)`, `(1R,3C)` and B/C expose `(2R,2C)`, `(2R,1C)`, `(3R,2C)`,
`(3R,1C)`. The second and fourth slots are release-capacity candidates.

Generic GLOBAL instead calls `capacityOptions`: A/D retain `(2R,2C)` and
`(2R,3C)`, while B/C retain `(2R,2C)` and `(3R,2C)`. It has no release-only
or release-and-borrow capacity attempts. It retains all valid PatternIDs for
its positive-capacity attempts before demand-equivalence canonicalization;
that canonicalization does not cause the contract difference. Conversely,
the V2 GREEDY contract has no generic GLOBAL's separate candidate-generation
identity—it stores/chooses the four frozen role slots.

Thus the observed N2/F8 ordering (GREEDY `0.999940`, generic GLOBAL
`0.999930`) is possible without a GLOBAL search defect. Current generic GLOBAL
is **a distinct global optimizer over the generic capacity contract**, not a
global optimizer over exactly the canonical V2 GREEDY candidate universe. A
paper claim of “canonical GREEDY versus its exact GLOBAL optimizer” requires a
separately authorized candidate-contract unification; R3 does not change this
semantic boundary.

## Completed point: RS=CS=2, F_GROUP=8

All nine policy replays completed 100,000 groups and passed the applicable
GLOBAL dominance gate. These are an incomplete portion of R3, not final R3
statistics.

| Policy | Successes | Repair rate |
|---|---:|---:|
| LOCAL | 99,867 | 0.998670 |
| Directional EARLY | 99,989 | 0.999890 |
| Directional GROUP-GREEDY | 99,994 | 0.999940 |
| Directional GROUP-GLOBAL | 99,993 | 0.999930 |
| Pairwise-row EARLY (2x2/edge) | 99,987 | 0.999870 |
| Two-Pairwise EARLY | 99,983 | 0.999830 |
| Two-Pairwise Pair-GLOBAL | 99,992 | 0.999920 |
| Single-Hop EARLY | 99,993 | 0.999930 |
| Single-Hop GLOBAL | 99,997 | 0.999970 |

## Scope limits

This work establishes only group-level repairability measurements. 2x2 CAM
fields remain `GENERIC_CPP_CAPACITY` and not RTL-calibrated; 1x4 CAM fields
remain `NOT_PROVEN`. Neither is a PPA claim.
