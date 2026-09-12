# Phase 3G Resource-Aware Fallback

`recam_resource_aware_fallback_selector` is pure combinational. It reads a
completed four-entry ConfigPatternMap, exact SA ID, policy (`0=EARLY`,
`1=GROUP_COMPRESSED`), and one read-only Phase 3F ledger snapshot. It performs
no RECAM analysis and does not modify ConfigPatternMap or the ledger.

Phase 3G selects the first policy-ranked entry that is locally qualified
(`valid && solution_valid && repairable`), legal for the exact SA, and
physically feasible. EARLY ranking is map order 0→1→2→3. GROUP ranking is
A/D 1→0→3→2 and B/C 2→0→3→1. It copies ConfigID, PatternID, and bitmap from
the selected record and reports proposed borrow/release actions.

Donor priority is deterministic but not globally optimal: A uses B_COL→C_COL,
D C_COL→B_COL, B A_ROW→D_ROW, and C D_ROW→A_ROW. Physical feasibility uses
the same ledger rules for both policies. Phase 3G does not change the RECAM
local solution space; it filters locally repairable configurations by physical
shared-resource feasibility. EARLY and GROUP differ only in ranking.

With this single-SA immutable-snapshot model, both policies inspect the same
four candidates, so physical-solution existence is also equal; they may select
different feasible configurations. This is an observed property of exhaustive
fallback over one unchanged snapshot, not a permanent invariant for later
commit/arbitration phases.

## Phase 3H unresolved questions

1. How owner releases populate/update the ledger.
2. When donor allocation commits to a borrower.
3. Sequential versus group-wide allocation.
4. Deterministic SA processing/arbitration order.
5. Simultaneous-borrower conflict resolution.
6. Whether fallback recomputes after another commit.
7. Rollback requirements.
8. Ledger reset between repair groups.
9. Need for registered ledger storage.
10. Whether EARLY and GROUP share one group allocation engine.
