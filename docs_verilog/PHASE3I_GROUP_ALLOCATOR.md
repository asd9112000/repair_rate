# Phase 3I Group Allocator

`recam_dss_group_allocator` executes the Phase 3H registered-ledger protocol
with frozen EARLY or GROUP ranking. It exposes registered per-SA ConfigID and
donor fields, commit-valid bits, final repairability, and the final 12-bit
ledger for verification.

The Phase 3G selector correction in
`recam_resource_aware_fallback_selector.sv` restores the frozen GROUP role
order for B/C to `2 -> 0 -> 3 -> 1`. A/D remains `1 -> 0 -> 3 -> 2`.
The prior condition accidentally applied the A/D ordering to B. This is an
implementation bug fix restoring pre-existing Phase 3G semantics; it is not a
new policy or allocation-semantic change.

No synthesis was run for Phase 3H/3I.
