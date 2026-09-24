# DATE2026 canonical N2 hardware summary

## Scope and method

This is the authoritative matched hardware summary for the seven FINAL N2
packages. All reported results use DC W-2024.09-SP2, TSMC018 `slow.db`, slow
corner, 20.0 ns virtual/register-boundary timing, zero I/O delay, and
`NAND2X1 = 9.979200` µm². Cell area, rather than DC's wire-load net area, is
used for GE conversion.

The shared address contract is `ROW_ADDR_W=9`, `PHYS_COL_ADDR_W=13`,
`WORD_COL_ADDR_W=5`, and `HYBRID_LINE_ADDR_W=13`. Physical repair columns and
hybrid column payloads are 13-bit. The 5-bit word column remains a logical
BIST-word field; it is not a physical repair-column substitute.

## Canonical PPA

| Case | Cell area (µm²) | GE | Comb / seq area (µm²) | Cells comb / seq | WNS / TNS (ns) | Status |
|---|---:|---:|---:|---:|---:|---|
| `RECAM_N2_2R2C` | 72116.352610 | 7226.67 | 72116.352610 / 0.000000 | 3364 / 0 | 0.00 / 0.00 | FINAL |
| `G2X2_N2_RC_EARLY` | 102296.779959 | 10251.00 | 101518.402346 / 778.377613 | 4879 / 13 | +0.03 / 0.00 | FINAL |
| `G2X2_N2_RC_GROUP_reg` | 166140.376634 | 16648.67 | 133448.516656 / 32691.859978 | 7039 / 581 | +0.02 / 0.00 | FINAL |
| `G2X2_N2_R_EARLY` | 99688.882299 | 9989.67 | 98937.115885 / 751.766415 | 4798 / 13 | 0.00 / 0.00 | FINAL |
| `G2X2_N2_R_GROUP_reg` | 158230.197352 | 15856.00 | 126722.535804 / 31507.661549 | 6681 / 561 | 0.00 / 0.00 | FINAL |
| `L1X4_N2_R_EARLY` | 99695.535153 | 9990.33 | 99016.949543 / 678.585609 | 4822 / 12 | 0.00 / 0.00 | FINAL |
| `L1X4_N2_R_GROUP_reg` | 154388.205311 | 15471.00 | 122867.238163 / 31520.967148 | 6580 / 561 | 0.00 / 0.00 | FINAL |

For corrected RECAM, the critical path is `pivot_valid_i[0]` to
`pattern_id_o[1]`, with 20.00 ns path delay. Full endpoint evidence is in its
archive timing report.

## Corrected versus historical RECAM

| Baseline | Cell area (µm²) | GE | Comb / seq area (µm²) | WNS / TNS (ns) |
|---|---:|---:|---:|---:|
| Historical Phase-3A 5-bit physical-column evidence | 50877.288438 | 5098.33 | 50877.288438 / 0.000000 | 0.00 / 0.00 |
| `RECAM_N2_2R2C` physical-column corrected | 72116.352610 | 7226.67 | 72116.352610 / 0.000000 | 0.00 / 0.00 |

The corrected-minus-historical cell-area delta is 21239.064172 µm²
(41.745668%); the GE delta is 2128.333351 (41.745668%). This comparison is
limited to the historical Phase-3A analyzer under the same 20 ns methodology.
It quantifies the physical-column representation correction and must not be
extrapolated as a DSS architectural effect.

## DSS relative to corrected RECAM

| Case | Area delta (µm²) | Area ratio | GE delta | GE ratio |
|---|---:|---:|---:|---:|
| `G2X2_N2_RC_EARLY` | 30180.427349 | 1.418496 | 3024.33 | 1.418496 |
| `G2X2_N2_RC_GROUP_reg` | 94024.024024 | 2.303782 | 9422.00 | 2.303782 |
| `G2X2_N2_R_EARLY` | 27572.529689 | 1.382334 | 2763.00 | 1.382334 |
| `G2X2_N2_R_GROUP_reg` | 86113.844742 | 2.194096 | 8629.33 | 2.194096 |
| `L1X4_N2_R_EARLY` | 27579.182543 | 1.382426 | 2763.67 | 1.382426 |
| `L1X4_N2_R_GROUP_reg` | 82271.852701 | 2.140821 | 8244.33 | 2.140821 |

## EARLY and GROUP boundary

EARLY commits a selected repair solution immediately and streams it through
`dss_early_selected_address_mux.sv`; it has no four-SA accumulated final
solution warehouse. GROUP delays the decision and retains full pivot row and
column address payloads. For N2's four SAs and five slots, this is 440 raw
address-state bits. The GROUP-minus-EARLY cell-area deltas are 63843.596675
µm² (G2X2 RC), 58541.315053 µm² (G2X2 R), and 54692.670158 µm² (L1X4 R).
Those deltas must not be attributed solely to the 440 raw retained bits.

## Provenance

Historical RECAM in `rtl/recam/` remains valid functional-policy evidence.
Its physical-column RTL and PPA are `SUPERSEDED_FOR_MATCHED_COMPARISON` by
`RECAM_N2_2R2C`; it is preserved rather than deleted. Pre-storage-audit EARLY
reports are likewise preserved only as `PRE_STORAGE_AUDIT` /
`NON_CANONICAL_PPA`; current streaming reports are canonical.
