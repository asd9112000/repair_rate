# FINAL-EARLY-C1R4 — Static Resource-Action Closure

```text
FINAL_EARLY_C1R4_STATUS: PASS
SCOPE: MODEL_B2_SIMULATOR_STATIC_PROOF_REGRESSION
RTL_MODIFIED: NO
PRODUCTION_SIMULATOR_MODIFIED: NO
DSS_FINAL_MODIFIED: NO
SYNTHESIS_RUN: NO
```

This phase tests the fixed resource-action theorem for the recommended final
directional EARLY policy only: SA order `A,B,C,D` and slot priority
`R,L,RB,B`. It follows C1R3's Model-B2 shared-collector implementation. The
new test has an independent fixed claim-mask oracle; it does not invoke a
production slot/action helper or reuse the production replay path to derive a
mask.

## Theorem and token contract

For each accepted repair, `(current SA, accepted slot)` uniquely determines
the physical common-spare action:

```text
claim_mask = fixed_decode(current_sa, accepted_slot)
available_common: 4'b1111 at transaction start
available_common_next = available_common & ~claim_mask
```

The token encoding is `[3:0]={C-column, B-column, D-row, A-row}`. A bit only
falls from 1 to 0. Admission of an `RB` or `B` action also uses its fixed slot
classification to enforce the existing `m=1` borrow limit; it does not need
`usedRows` or `usedColumns` to decide the action.

| SA | Config IDs: R / L / RB / B | R | L | RB | B |
| --- | --- | --- | --- | --- | --- |
| A | 4 / 0 / 6 / 5 | `0000` | `0001` A-row | `0100` B-column | `0101` A-row+B-column |
| B | 1 / 0 / 3 / 2 | `0000` | `0100` B-column | `0010` D-row | `0110` B-column+D-row |
| C | 1 / 0 / 3 / 2 | `0000` | `1000` C-column | `0001` A-row | `1001` C-column+A-row |
| D | 4 / 0 / 6 / 5 | `0000` | `0010` D-row | `1000` C-column | `1010` D-row+C-column |

Thus the final RTL action decision can be a fixed 16-entry decode. Candidate
demand fields remain diagnostics/model inputs, not an action-selection
interface. The accepted slot/config identity must be retained through the
analyzer-result/remap handoff, while the claim mask can update the token state
at the acceptance edge. `ABCD` and `R,L,RB,B` are fixed policy control, not
per-repair decode payloads.

## Independent replay result

The bounded corpus is the same C1R3 deterministic set: `Rs=Cs=2`, directional
`m=1`, Moderate-Imbalance/Mixed, seed `20260922`, F_GROUP `16,20,24,28`, and
300 groups per load (1,200 groups). For every production-selected candidate,
the test independently derives the physical action from its actual row/column
use and compares it with the table above.

| SA | R reachable/match | L reachable/match | RB reachable/match | B reachable/match |
| --- | ---: | ---: | ---: | ---: |
| A | 1,200 / 1,200 | 0 / 0 | 0 / 0 | 0 / 0 |
| B | 1,110 / 1,110 | 68 / 68 | 5 / 5 | 17 / 17 |
| C | 1,028 / 1,028 | 108 / 108 | 3 / 3 | 40 / 40 |
| D | 658 / 658 | 111 / 111 | 11 / 11 | 36 / 36 |

All reachable selections match. Zero-count entries were not selected by this
policy/corpus; they remain covered by the complete fixed table, rather than
being presented as an empirical reachability claim.

```text
THEOREM_COUNTEREXAMPLES: 0
P0_PRODUCTION_REPLAY_MATCHES: 1200 / 1200
VECTOR216: PASS (C selects R; C:L is not reached)
C1R_ANALOGOUS_16: PASS (16 / 16 R static actions)
```

## Priority sensitivity retained as development evidence

The test-local P1 alternate priority is `R,RB,L,B`. It reproduces exactly the
four C1R3 P0-only results at F_GROUP=24; P1-only remains zero. P0 is the
frozen recommended order, so this is an explanation of why policy control
must be retained, not a production priority change.

| F_GROUP / group | P0 slots A:B:C:D | P1 slots A:B:C:D | Explanation |
| --- | --- | --- | --- |
| 24 / 35 | `R:L:R:L` | `R:RB:R:fail` | P1 borrows D-row at B, blocking D local-L's D-row. |
| 24 / 69 | `R:L:B:L` | `R:RB:fail:fail` | P1 consumes the sole borrow at B; C:B's A-row borrow is no longer admissible. |
| 24 / 192 | `R:L:R:L` | `R:RB:R:fail` | Same D-row conflict as group 35. |
| 24 / 290 | `R:L:B:R` | `R:RB:fail:fail` | Same single-borrow conflict as group 69. |

The point is structural: P1's earlier `RB` is a different fixed physical
action from P0's `L`; it consumes either a token that a later local action
needs or the sole permitted borrow. A generic demand-driven ledger is not
needed to express either fact in final RTL.

## Reproducibility

| Command | Result |
| --- | --- |
| `make test_final_early_c1r4_static_action` | PASS — theorem counterexamples 0, 16 C1R matches, Vector216, and four P1 losses reproduced. |

The test writes non-versioned evidence to
`tmp/date2026/final_early_c1r4/static_action_table.csv` and
`tmp/date2026/final_early_c1r4/p1_loss_cases.csv`. The reusable test source
and Makefile target are committed with this report; temporary evidence is not.

## Hard stop

```text
C1R4_STATIC_RESOURCE_ACTION_THEOREM: PASS
NEXT_ACTION: STOP_FOR_HUMAN_APPROVAL_BEFORE_C2
AREA_BLOCKED_BY_THIS_PHASE: YES
AREA_NEXT_REQUIRED_STEP: HUMAN_APPROVAL_FOR_C2_THEN_C2_C3_AND_FRESH_DC
EXISTING_SYNTHESIS_REUSABLE: NO
NEW_SYNTHESIS_REQUIRED: YES
```
