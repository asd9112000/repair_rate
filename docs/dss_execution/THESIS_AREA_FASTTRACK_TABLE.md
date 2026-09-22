# DATE2026 — Thesis area fast-track table

All figures are mapped total-cell area divided by the same TSMC018 `slow.db`
NAND2X1 area (9.979200 um2), at a 20.0 ns clock and zero I/O delay.  The
primary table is restricted to the frozen `HYP02_STATIC_81_PATH_CONTRACT`.

| Record | Architecture / top | Semantic boundary | Area (um2) | GE | Delta vs GROUP | Timing | Thesis use |
| --- | --- | --- | ---: | ---: | ---: | --- | --- |
| RECAM baseline | `recam_2r2c_analyzer` | baseline component | 34105.579462 | 3417.67 | n/a | WNS/TNS 0.00/0.00 ns | baseline component only |
| FINAL HYP02 EARLY | `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top` | H=7 204-bit analyzer; immediate 81-path-compatible prefix selection | 78216.970295 | 7838.00 | -26.447496% | critical 19.72 ns; WNS/TNS +0.01/0.00 ns | primary matched EARLY PPA |
| FINAL HYP02 GROUP/GLOBAL | `recam_dss_hyp02_static_global_top` | H=7 204-bit analyzer; deferred static 81-path atomic selection | 106341.682630 | 10656.33 | baseline | critical 19.82 ns; WNS/TNS 0.00/0.00 ns | primary matched GROUP PPA |

## Exploratory Model-B2 — not semantically matched to final HYP02 GROUP

| Record | Architecture / top | Analyzer representation | Area (um2) | GE | Timing | Status |
| --- | --- | --- | ---: | ---: | --- | --- |
| corrected Model-B2 EARLY | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` | 4 x 260-bit / 11 Hybrid entries | 200824.749004 | 20124.33 | critical 19.72 ns; WNS/TNS 0.00/0.00 ns | exploratory only; GROUP_B2_COMPATIBILITY_FAIL |

The HYP02 EARLY/GROUP rows share candidate `{slot-valid, PatternID}` and the
same fixed 81-path resource abstraction.  Their comparison does not establish
equivalence to the existing C++ PhysicalResourceLedger demand semantics.  The
Model-B2 figure is retained for provenance only and must not be used as the
primary EARLY-versus-GROUP thesis comparison.
