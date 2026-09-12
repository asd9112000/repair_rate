# Phase 3F PhysicalResourceLedger Snapshot

Phase 3F defines a logical, read-only PhysicalResourceLedger snapshot. Ledger
state mutation, reservation, and commit are deferred to Phase 3H. Its 12-bit
logical format is four resources times `{released, borrower[1:0]}`:

| ID | Resource | Owner | Eligible borrowers |
|---:|---|---|---|
| 0 | A_ROW | A | B, C |
| 1 | D_ROW | D | B, C |
| 2 | B_COL | B | A, D |
| 3 | C_COL | C | A, D |

Each borrower uses resource-local encoding: `00=NONE`, `01/10=valid neighbor`,
`11=INVALID`. A snapshot is illegal if a borrower is invalid or if a non-NONE
borrower appears while `released=0`. A shared physical resource is available
for borrowing only when it has been released by its owner and is not already
allocated to another borrower: `released && borrower==NONE`.

`recam_config_resource_action_decoder` is pure combinational and validates
SA/config pairs. A/D accepts CFG0/4/5/6; B/C accepts CFG0/1/2/3. It reports
proposed borrow/release action, own release resource, and the two permitted
donors; it does not change the ledger.

This format is an external read-only input, not twelve registered ownership
bits. How snapshots are generated, updated, reset, or committed across a group
is intentionally unresolved.
