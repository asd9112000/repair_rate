# P3-BL-IF — committed-ledger interface

## Commit unit

The committed-ledger boundary accepts:

```text
commit_valid
transaction_kind = EARLY_SINGLE | GLOBAL_GROUP
selected_count
selected candidate records / topology transition records
expected pre-commit ledger version or pristine-group indication
```

It returns:

```text
commit_accepted | commit_error
committed released/borrowed/owner state
committed selected tuple
```

Existing `dss_v2_resource_ledger` is an EARLY single-candidate ledger. It validates one transaction and atomically updates release/borrower state on one rising edge. It is not a four-candidate GLOBAL group commit implementation.

## Frozen semantics

| Question | EARLY | GROUP_GLOBAL |
|---|---|---|
| Commit point | Immediately after each accepted SA candidate. | Only after a complete legal A/B/C/D tuple. |
| Atomic unit | Selected candidate's release/borrow transaction. | Entire selected group tuple. |
| External intermediate state | Earlier accepted SA state is visible and permanent. | Forbidden; search and partial tuple state are not committed state. |
| Commit failure | Report `commit_error`; controller cannot advance. | Search/commit divergence; report failure/error with physical ledger unchanged. |
| Rollback | No, after successful immediate commit. | Required internally if entries are staged; externally all-or-nothing. |
| Stable data until acknowledgement | Selected candidate. | Full tuple, mapping references, transition metadata, final obligation state. |

For normal GLOBAL success, failure must be unreachable when search and adapter use identical topology/ledger semantics. `commit_error` remains explicit so RTL cannot silently claim a repairable result after a failed physical commit.

## Per-topology tuple

### 2x2 Directional

Each slot retains `sa_id`, selected-valid, action, ConfigID, PatternID, actual release, actual borrow, selected donor resource, release resource, and mapping reference.
The future 2x2 adapter uses the `A_ROW, D_ROW, B_COL, C_COL` resource map and validates its tuple against a pristine group ledger before publishing output.

### 1x4 Single-Hop

Each slot retains `sa_id`, capacity-attempt token, PatternID, usedRows, usedColumns, selected adjacent donor identity if borrowed, final row allocation reference, and mapping reference.
The adapter allocates only rows under the A-B-C-D neighbor rule; columns remain local. It must distinguish B/C left and right donors, reject a transitive transfer, and never make a borrowed line forwardable.

## Required invariant

For GLOBAL, `group_repairable_o=1` is observable only with `group_commit_accepted=1` and a ledger equal to atomically applying the selected tuple to the pre-start ledger.
On `group_commit_error`, that pre-start ledger remains committed and no selected success tuple is published as valid.
