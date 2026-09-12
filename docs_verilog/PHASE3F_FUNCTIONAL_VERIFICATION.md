# Phase 3F Functional Verification

`bash scripts/simulation/run_recam_phase3fg_functional.sh` passes ledger tests
for every physical resource: retained/free, released/free, released/allocated,
impossible allocation, and invalid borrower code. It also passes all legal
A/D and B/C ConfigID action decodes and rejects illegal SA/config pairs.

The tests confirm the snapshot is read-only; no registered ledger storage,
write, reservation, or commit behavior is present.
