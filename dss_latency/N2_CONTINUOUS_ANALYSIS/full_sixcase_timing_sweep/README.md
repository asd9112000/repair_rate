# N2 six-case CA-LIVE timing sweep

This archive closes the 10/15/20/25 ns timing characterization for the frozen
N2 CA-LIVE family.  The 20 ns points are reused from the committed matched
seven-case dataset; only the four remaining R-policy CA-LIVE cases require
new 10, 15, and 25 ns DC runs.

| Case | 10 ns | 15 ns | 20 ns | 25 ns | Evidence source |
| --- | --- | --- | --- | --- | --- |
| G2X2 RC EARLY CA-LIVE | committed | committed | committed | committed | `../synthesis_timing_sweep/` |
| G2X2 RC GROUP CA-LIVE | committed | committed | committed | committed | `../synthesis_timing_sweep/` |
| G2X2 R EARLY CA-LIVE | new | new | reused | new | `raw/G2X2_R_EARLY/` |
| G2X2 R GROUP CA-LIVE | new | new | reused | new | `raw/G2X2_R_GROUP/` |
| L1X4 R EARLY CA-LIVE | new | new | reused | new | `raw/L1X4_R_EARLY/` |
| L1X4 R GROUP CA-LIVE | new | new | reused | new | `raw/L1X4_R_GROUP/` |

All new points use DC W-2024.09-SP2, TSMC018 `slow.db`, slow corner, zero
I/O delay, `tsmc18_wl10`, output load 0.05, and `compile -map_effort low`.
Only clock period varies.  The runner checks the exact source manifest before
and after each point.  No frozen CA-LIVE RTL is changed by this phase.

`SUMMARY.csv` and `SUMMARY.md` are populated only from successfully generated
raw reports and the committed 20 ns rows.  A timing constraint violation is a
measured result, not a runner failure; DC-run completion is indicated by each
raw `metadata.txt` containing `STATUS=PASS`.
