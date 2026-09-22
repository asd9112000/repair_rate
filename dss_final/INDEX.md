# DATE2026 DSS Final Hardware Cases

| Case | Top | Canonical RTL Commit | Synthesis Record | Area um2 | GE | WNS ns | Status |
|---|---|---|---|---:|---:|---:|---|
| 2x2 Directional GROUP-GLOBAL NoScratch | `recam_dss_hyp02_static_global_top` | `35a60728642e73f5d2c428d2a345ef8c6817bdaa` | `03046e4` | 106341.682630 | 10656.33 | 0.00 | FINAL |
| 2x2 Directional corrected Model-B2 EARLY NoScratch | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` | `7fdaf0ad86700d40588f31735d4eb2513b77478f` | current DATE2026 closure | 200824.749004 | 20124.33 | 0.00 | FINAL_EARLY; GROUP_B2_COMPATIBILITY_FAIL |

## Planned Cases

Future 1x4 cases are `NOT_YET_FINAL` and are intentionally absent from the
final-case table.  The EARLY entry is a Model-B2 final decision/resource-
management characterization; its presence does not modify the archived GROUP
case or resolve the separate GROUP compatibility failure.
