# RECAM_N2_2R2C

`CASE_ID: RECAM_N2_2R2C`
`LIFECYCLE: FINAL`

Canonical combinational N2 RECAM 2R2C baseline. The historical 2R2C policy,
PatternID ordering, and first-legal solution semantics remain unchanged. Repair
columns are 13-bit physical identities; the 5-bit logical BIST word-column is
a distinct field.

Verification is PASS for physical `1 != 257`, boundary columns, 1000-vector
low-column old/new equivalence, Verilator width lint, and historical regressions.
Matched 20 ns DC synthesis and source hash gates are PASS. The canonical archive
is `dss_final/RECAM_N2_2R2C/`; final PPA is authoritative only in
`dss_final/N2_HARDWARE_SUMMARY.md`. Final closure commit: `6351a82`.
