# Phase 3G Functional Verification

The Phase 3FG regression passes EARLY first-choice/no-donor/order cases,
GROUP release preference and borrow fallback, A/D/B/C mirrored donor priorities,
already-allocated donor protection, and invalid-ledger all-zero behavior. It
also verifies selected PatternID, bitmap, ConfigID, and resource action come
from the same selected map record.

The local EARLY/GROUP solution-valid invariant remains covered by Phase 3E.
Physical validity is intentionally not constrained to equality between policies:
it is policy-ranking dependent once a frozen resource snapshot filters choices.
