# RECAM N2 2R2C corrected physical-column baseline

`CASE_ID: RECAM_N2_2R2C`

`LIFECYCLE: FINAL`

This is the canonical N2 RECAM baseline for `RS=2`, `CS=2`, `m=1`.  The
original Phase-3A policy remains unchanged: four fixed matrix indices,
six `C(4,2)` patterns, historical PatternID order, and first legal candidate.

## Address contract

| Field | Width | Meaning |
|---|---:|---|
| `ROW_ADDR_W` | 9 | physical row address |
| `PHYS_COL_ADDR_W` | 13 | physical repair-column identity |
| `WORD_COL_ADDR_W` | 5 | logical BIST word-column; not a repair-line payload |
| `HYBRID_LINE_ADDR_W` | 13 | hybrid repair-column payload |

The 2R2C matrix has four policy pivots because `RS+CS=4`; this does not alter
the five-slot N2 DSS interface contract. No DSS GROUP or EARLY state is used.

## Evidence

- `PHYSICAL_COLUMN_ALIAS_TEST: PASS` for 1 versus 257.
- `BOUNDARY_TEST: PASS` for 0, 31, 32, 255, 256, 257, 4095, and 8191.
- `LOW_COLUMN_EQUIVALENCE: 1000 / 1000 PASS`, zero mismatches.
- `VERILATOR_LINT: PASS` with width warnings treated as errors.
- `FUNCTIONAL_SOURCE_TO_SYNTH_SOURCE_HASH_MATCH: PASS`.

Historical `rtl/recam/` remains policy evidence. Its 5-bit physical-column RTL
and PPA are superseded only for matched physical-address comparison.
