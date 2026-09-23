# DATE2026 DSS Final Hardware Cases

| Case | Top | Canonical RTL Commit | Synthesis Record | Area um2 | GE | WNS ns | Status |
|---|---|---|---|---:|---:|---:|---|
| 2x2 Directional GROUP-GLOBAL NoScratch | `recam_dss_hyp02_static_global_top` | `35a60728642e73f5d2c428d2a345ef8c6817bdaa` | `03046e4` | 106341.682630 | 10656.33 | 0.00 | FINAL HYP02 mother |
| 2x2 G2X2_R row-only GROUP NoScratch | `recam_dss_g2x2_r_static_global_top` | `766268698c983ce5ddb6fa916e4a65b2183cb0ad` | Track A N=2 checkpoint | 97140.859997 | 9734.33 | 0.00 | PROVISIONAL; SYNTHESIS_COMPLETE; SUMMARY_BOUNDARY_VERIFICATION_DEFERRED |
| 2x2 Directional HYP02-compatible EARLY NoScratch | `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top` | `46e1d3a82bf948dc3e357823cbc50869be964023` | DATE2026 Path-C2 closure | 78216.970295 | 7838.00 | +0.01 | FINAL; HYP02_STATIC_81_PATH_CONTRACT |
| 2x2 Directional corrected Model-B2 EARLY NoScratch | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` | `7fdaf0ad86700d40588f31735d4eb2513b77478f` | current DATE2026 closure | 200824.749004 | 20124.33 | 0.00 | EXPLORATORY; GROUP_B2_COMPATIBILITY_FAIL |

## Boundary note

The HYP02-compatible EARLY and GROUP entries are the matched hardware PPA
pair: one H=7 shared analyzer and the fixed HYP02 81-path abstraction.  The
Model-B2 EARLY archive remains intact but is intentionally not a matched
comparator.  Neither HYP02 hardware result establishes equivalence to the
existing C++ PhysicalResourceLedger demand model.

## Planned Cases

Future 1x4 cases are `NOT_YET_FINAL` and are intentionally absent from the
final-case table.
