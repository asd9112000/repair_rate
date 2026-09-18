# P2-DOM equivalent-effect audit

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

At one fixed DFS depth, two valid candidates are resource-transition equivalent
only when all of the following match:

```text
(explicit_release, actual_release, actual_borrow)
```

Depth is part of the context because it selects the release resource, donor,
and future owner.  PatternID and ConfigID must still be retained for the first
canonical representative so the output tuple remains reconstructible.

For a fixed incoming state, candidates in one such class have identical
legality and identical next `(released, used, obligations, borrow_count)`
state.  If the first candidate in frozen R,L,RB,B then ascending-PatternID
order fails, every later member of that class fails; if it succeeds, it is
already the canonical tuple.  Thus only the first canonical member needs to be
explored.

```text
EQUIVALENT_EFFECT_COLLAPSE_SAFE_FOR_REPAIRABILITY: YES
EQUIVALENT_EFFECT_COLLAPSE_SAFE_FOR_CANONICAL_TUPLE: YES
```

This statement is deliberately narrower than effect-pair collapse.  Its
verification model reports zero repairability, selected-tuple, and per-depth
obligation-trace mismatches on the directed witnesses and 1,000 fixed-seed
candidate maps.

The current RTL does not perform this collapse.
