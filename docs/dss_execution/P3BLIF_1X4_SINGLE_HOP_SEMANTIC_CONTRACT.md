# P3-BL-IF — 1x4 Single-Hop semantic contract

## Provenance and status

This contract is extracted from the C++ `NeighborSharing` ledger and `OneByFourSingleHopEarlyV1` / `OneByFourSingleHopGlobalV1` policies. It freezes required future RTL semantics; it does not claim an existing RTL module, cycle contract, or 2x2-equivalence key.

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED
SA_ORDER: A -> B -> C -> D
SHARING_DIMENSION: rows only
SHARED_COLUMNS: 0
SYNTHESIZABLE_1X4_RTL: MISSING
```

## Resource and donor rules

* Legal row borrowing is adjacent only: A<->B, B<->C, and C<->D; A cannot borrow C/D and D cannot borrow A/B.
* Borrowing is single-hop. A borrowed line cannot become a line its borrower lends onward.
* Middle donor priority is left before right: B chooses A then C; C chooses B then D. This is v1 EARLY topology/ledger behavior.
* Donor ownership, borrower ownership, and final line assignment are physical ledger facts; they cannot be reconstructed from a one-bit borrow flag.
* Columns remain local. A future RTL must implement the frozen row-only configuration rather than directional 2x2 resources.

## Candidate order and policies

For `OneByFourSingleHopEarlyV1`, candidate priority is local-capacity attempt first, then ascending extra-row capacity, then ascending PatternID within an attempt. The first ledger-legal candidate commits immediately; there is no rollback; failure is the first SA without a legal candidate.

For `OneByFourSingleHopGlobalV1`, a complete tuple is evaluated under the same A-to-D sequential ledger allocation semantics. Equal `(usedRows, usedColumns)` candidates reduce to the lowest `(PatternID, attemptIndex)`. The objective is:

```text
minimum borrowed rows
minimum used rows
lexicographic PatternID tuple
lexicographic attempt tuple
```

GLOBAL selects before the all-or-nothing ledger commit. C++ monotonic failed-prefix pruning is evidence for the software policy, but no RTL dominance/state-summary theorem is frozen here.

## Explicit non-equivalences

```text
2X2 explicit_release action semantics: NOT_APPLICABLE
2X2 (explicit_release, actual_release, actual_borrow) key: NOT_APPLICABLE
1X4 donor identity required before final allocation: YES
1X4 middle left/right donor metadata required: YES
1X4 actual_borrow as sole candidate field: NO
1X4 future obligation = 2x2 release_req_mask: NO
```

Evidence: `src/PhysicalResourceLedger.cpp` (`NeighborSharing` adjacency), `src/DynamicRepairSimulator.cpp` (priority, allocation, objective), `R1B_1X4_ROW_ONLY_POLICY_IMPLEMENTATION.md`, and `R2F_GLOBAL_RUNTIME_AND_PREFLIGHT_COMPLETION.md`.
