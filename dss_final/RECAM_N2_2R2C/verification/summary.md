# Verification summary

`VERILATOR_LINT: PASS` — `-Wall -Werror-WIDTH`, no address-width or
signedness warning.

`PHYSICAL_COLUMN_ALIAS_TEST: PASS` — physical columns 1 and 257 produce
distinct matrix behavior; the historical low-five-bit adapter is the negative
control.

`BOUNDARY_TEST: PASS` — 0, 31, 32, 255, 256, 257, 4095, and 8191.

`LOW_COLUMN_EQUIVALENCE: 1000 / 1000 PASS` — complete matrix, six-candidate
vector, repairable result, and PatternID compare identically for columns 0--31.

`RECAM_REGRESSION: PASS` — all-local, row-must, column-must, overflow failure,
and a physical column above 31. Existing historical Phase-3A and Phase-3B
regressions are preserved in adjacent logs.
