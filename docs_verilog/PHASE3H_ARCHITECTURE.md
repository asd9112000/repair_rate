# Phase 3H Registered Ledger Architecture

Phase 3H adds the registered group allocator around the already-frozen Phase
3F ledger and Phase 3G fallback selector. Allocation traverses A, B, C, then D.
For each subarray it evaluates one immutable pre-commit ledger snapshot,
selects the first policy-ranked physically feasible map record, and atomically
commits any release and borrow. The first SA without a feasible record ends the
group with no rollback of earlier commits.

The logical ledger has four resources: `A_ROW`, `D_ROW`, `B_COL`, and `C_COL`.
Each has a released bit and a resource-local two-bit borrower field; exported
state is the 12-bit `{borrowers,released}` value.

Verification correction: `ledger_released_borrower_o` is now a continuous view
of the registered ledger, so the external observation includes the final
atomic commit. This was an observability implementation fix, not a change to
allocation policy, feasibility, ownership, or commit semantics.
