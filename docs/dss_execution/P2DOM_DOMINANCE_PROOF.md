# P2-DOM dominance proof

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

For two states at the same DFS depth and with the same remaining candidate-map
suffix, define `S1` to dominate `S2` iff:

```text
released(S1) superset released(S2)
used(S1)     subset   used(S2)
req(S1)      subset   req(S2)
count(S1) <= count(S2)
```

For every candidate legal from `S2`, the identical candidate is legal from
`S1`: fewer obligations cannot add an explicit-release requirement; an already
released donor removes rather than adds a future-owner requirement; fewer used
donors and no larger count cannot create the two borrow failures.  Applying the
same candidate preserves all four relations: releases add the same bit,
borrows add the same used bit and increment both counts, and a future-donor
obligation is either added to both states or avoided by the more-released
state.  Induction over the common remaining depths proves:

```text
S2 has a legal completion => S1 has a legal completion.
MORE_RELEASED_MONOTONIC: YES
LESS_USED_MONOTONIC: YES
FEWER_OBLIGATIONS_MONOTONIC: YES
LOWER_BORROW_COUNT_MONOTONIC: YES
```

Canonical first-tuple preservation imposes one extra ordering condition.  A
cache may prune `S2` only after an earlier canonical prefix reaching dominant
`S1` has been completely explored and failed at the same depth.  A later
dominating state cannot prune an earlier state, even if it proves repairability,
because that would replace the frozen first tuple.  The non-production model
implements exactly this failed-subtree cache and observed zero mismatches in
the directed and 1,000-map checks.

## Level relation

For candidates with the same explicit-release meaning and the same incoming
state, `(1,0)` dominates `(0,0)`, `(1,1)`, and `(0,1)`.  Each of `(0,0)` and
`(1,1)` dominates `(0,1)`; neither Level-2 branch dominates the other.

```text
LEVEL1_FAIL_PRUNES_LEVEL2: YES, only after the applicable earlier Level-1 subtree fails
LEVEL1_FAIL_PRUNES_LEVEL3: YES, under the same condition
LEVEL2_FAIL_PRUNES_LEVEL3: YES, only after every applicable earlier L-like and RB-like subtree fails
```

These are failed-subtree implications, not permission to delete a nominal
action class before checking its explicit-release semantics.
