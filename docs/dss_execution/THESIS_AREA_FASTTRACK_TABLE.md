# DATE2026 — Thesis area fast-track table

All figures are mapped total-cell area divided by the same TSMC018 `slow.db`
NAND2X1 area (9.979200 um2), at a 20.0 ns clock and zero I/O delay.  The rows
are deliberately not asserted to be a matched-policy comparison until the
separate GROUP Model-B2 compatibility audit finishes.

| Record | Architecture / top | Analyzer representation | Area (um2) | GE | Timing | Evidence status | Thesis use |
| --- | --- | --- | ---: | ---: | --- | --- | --- |
| RECAM baseline | `recam_2r2c_analyzer` | baseline component | 34105.579462 | 3417.67 | WNS/TNS 0.00/0.00 ns | historical phase3a reports | baseline component only; not an integrated DSS comparison |
| FINAL corrected Model-B2 EARLY | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` | 4 x 260-bit / 11 Hybrid entries | 200824.749004 | 20124.33 | critical 19.72 ns; WNS/TNS 0.00/0.00 ns | current DC closure at `7fdaf0a` | thesis-ready EARLY area for the actual corrected Model-B2 decision/resource-management boundary |
| Archived GROUP/GLOBAL | `recam_dss_hyp02_static_global_top` | 204-bit / 7 Hybrid entries | 106341.682630 | 10656.33 | critical 19.82 ns; WNS/TNS 0.00/0.00 ns | immutable final archive | `MODEL_B2_COMPATIBILITY_FAIL`; a read-only audit found 259/1000 full-Model-B2 semantic differences, so it is not a matched PPA comparator |

The EARLY-to-GROUP area difference is not interpreted here as a policy-only
cost: their retained analyzer representations differ (260/11 versus 204/7).
The completed read-only audit found a compatibility failure; no GROUP RTL
change is authorized by this table.
