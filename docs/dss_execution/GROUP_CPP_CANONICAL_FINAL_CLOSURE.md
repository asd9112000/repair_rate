# Canonical Group C++ Final Closure

> Date: 2026-09-18
> Scope: frozen normalized N2/N3 14×1k group corpus only
> Status: CLOSED

> **P1-NAME annotation (2026-09-18):** Normalized GLOBAL
> (`normalized_global`; historical input
> `directional_v2_group_global_canonical`) is distinct from the Historical
> directional-V2 GLOBAL oracle (`directional_v2_group_global`).  The latter
> stays readable as immutable provenance and must not be silently labelled
> canonical.  See `CANONICAL_NAMING_MAP.md`.

## Contract

The historical `directional_v2_group_global` implementation and all frozen
raw results remain unchanged.  The distinct oracle
`directional_v2_group_global_canonical` freezes:

```text
SA order:       A -> B -> C -> D
action order:   RELEASE_ONLY, LOCAL, RELEASE_AND_BORROW, BORROW_ONLY
PatternID:      ascending within each action
topology:       PhysicalResourceLedger directional ownership/order
search:         complete bounded DFS with backtracking
future borrow:  exact-resource release obligation
commit:         only after a complete legal tuple
```

The speculative state is three four-bit concepts (`released`, `used`, and
`release requirement`) plus a borrow count.  A candidate's actual row/column
demand determines whether it consumes or leaves the physical shareable line;
an outstanding future obligation is stricter and is discharged only by the
future owner's explicit `RELEASE_ONLY` or `RELEASE_AND_BORROW` action.  This
distinction preserves physical spare accounting while making the new
future-owner rule explicit.

## Frozen-corpus audit

`make test_canonical_global_corpus_audit` replays the existing files under
`tmp/repair_rate_matrix_v2_normalized_1k/corpus`; it does not generate faults.
The detailed derived records are under `tmp/canonical_global_closure`.

```text
GROUPS_CHECKED: 14000
CURRENT_GLOBAL_FUTURE_BORROWS: 569
CURRENT_GLOBAL_MATCHED_FUTURE_BORROWS: 326
CURRENT_GLOBAL_UNMATCHED_FUTURE_BORROWS: 243
OLD_VS_CANONICAL_GLOBAL_REPAIRABILITY_DIFF: 0
OLD_VS_CANONICAL_GLOBAL_SELECTED_TUPLE_DIFF: 12236
EARLY_PASS_CANONICAL_GLOBAL_FAIL: 0
CANONICAL_INVARIANT_VIOLATIONS: 0
```

The old audit classifies a future borrow as matched only when the future
owner's selected action explicitly releases that resource.  The 243 unmatched
events therefore establish that the old implicit sequential allocation was
too permissive for the newly frozen explicit-obligation contract.  Search
order changes selected tuples widely, but did not change repairability in the
14k corpus.

For every canonical success the audit also verifies: every actual future
reservation is matched, no physical line has more than one borrower, every
transfer is legal according to `PhysicalResourceLedger::canBorrow`, and the
configured physical spare budget is unchanged.

## N2/F16/group172

The historical tuple was:

```text
A: BORROW_ONLY, Config 5, Pattern 7; reserves B_COL from future B
B: LOCAL, Config 0, Pattern 1; does not explicitly satisfy the reservation
C: BORROW_ONLY, Config 2, Pattern 3
D: LOCAL, Config 0, Pattern 1
```

The canonical first legal tuple is:

```text
A: RELEASE_AND_BORROW, Config 6, Pattern 2; reserves B_COL
B: RELEASE_ONLY,       Config 1, Pattern 1; satisfies B_COL obligation
C: BORROW_ONLY,        Config 2, Pattern 3; uses released A_ROW
D: RELEASE_ONLY,       Config 4, Pattern 1
```

Thus group172 remains repairable, but its selected tuple changes and the
future reservation is explicit and satisfied.

## Closure summary

```text
STATUS: CLOSED
FROZEN_1K_CORPUS_CHANGED: NO
LOCAL_BASELINE: CLOSED
LOCAL_FIRST: CLOSED
NORMALIZED_EARLY: CLOSED
CANONICAL_GLOBAL: CLOSED
SAME_CORPUS: PASS
CURRENT_GLOBAL_FUTURE_BORROWS: 569
CURRENT_GLOBAL_MATCHED_FUTURE_BORROWS: 326
CURRENT_GLOBAL_UNMATCHED_FUTURE_BORROWS: 243
OLD_VS_CANONICAL_GLOBAL_REPAIRABILITY_DIFF: 0
OLD_VS_CANONICAL_GLOBAL_SELECTED_TUPLE_DIFF: 12236
EARLY_PASS_GLOBAL_FAIL: 0
CANONICAL_WITNESSES: PASS
GROUP_CPP_SIMULATOR: CLOSED
FORMAL_100K_TOUCHED: NO
NEW_RANDOM_FAULT_CORPUS: NO
```
