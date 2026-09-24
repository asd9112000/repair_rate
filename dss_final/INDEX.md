# DATE2026 N2 final hardware index

## Canonical FINAL packages

| Case | Address contract | PPA cell area (µm²) | WNS (ns) | Status |
|---|---|---:|---:|---|
| `RECAM_N2_2R2C` | row 9, physical column 13, word column 5, hybrid 13 | 72116.352610 | 0.00 | FINAL |
| `G2X2_N2_RC_EARLY` | row 9, physical column 13, word column 5, hybrid 13; 5 DSS slots/SA | 102296.779959 | +0.03 | FINAL |
| `G2X2_N2_RC_GROUP_reg` | row 9, physical column 13, word column 5, hybrid 13; 5 DSS slots/SA | 166140.376634 | +0.02 | FINAL |
| `G2X2_N2_R_EARLY` | row 9, physical column 13, word column 5, hybrid 13; 5 DSS slots/SA | 99688.882299 | 0.00 | FINAL |
| `G2X2_N2_R_GROUP_reg` | row 9, physical column 13, word column 5, hybrid 13; 5 DSS slots/SA | 158230.197352 | 0.00 | FINAL |
| `L1X4_N2_R_EARLY` | row 9, physical column 13, word column 5, hybrid 13; 5 DSS slots/SA | 99695.535153 | 0.00 | FINAL |
| `L1X4_N2_R_GROUP_reg` | row 9, physical column 13, word column 5, hybrid 13; 5 DSS slots/SA | 154388.205311 | 0.00 | FINAL |

Each name above matches its archive directory, `CASE.md`, `README.md`, source
manifest, and synthesis report identity. See `N2_HARDWARE_SUMMARY.md` for the
methodology, detailed PPA, and matched ratios.

## Architecture boundary

EARLY is immediate commit with streaming selected repair lines and no four-SA
accumulated final-solution warehouse. GROUP delays selection and retains a
440-bit raw pivot-address payload (four SAs × five slots × (9-bit row + 13-bit
column)); this raw state alone does not explain the GROUP-versus-EARLY PPA
delta.

## Historical and superseded packages

The entries below are preserved evidence, not current matched N2 packages:

- Historical RECAM Phase-3A: policy evidence remains valid; 5-bit physical
  repair-column RTL and its PPA are `SUPERSEDED_FOR_MATCHED_COMPARISON` by
  `RECAM_N2_2R2C`.
- `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch`,
  `L1X4_R_GROUP_V1`, and the old G2X2 R GROUP record are superseded by the
  corresponding corrected physical-column GROUP packages.
- The historical HYP02 / Model-B2 EARLY packages remain archived for provenance
  and must not be read as current canonical N2 results.
