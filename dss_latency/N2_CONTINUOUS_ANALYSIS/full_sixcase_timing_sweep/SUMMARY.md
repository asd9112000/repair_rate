# N2 CA-LIVE six-case timing summary

`SUMMARY.csv` is the authoritative 24-point table: six canonical N2 CA-LIVE
cases at 10, 15, 20, and 25 ns.  Its
`critical_path_slack_ns` field is copied from the DC QoR `Critical Path Slack`
line; it is deliberately not called Design WNS.  Negative slack means setup
failure.  GE is total cell area divided by `NAND2X1 = 9.979200 um^2`.

## New remaining-four DC points

| Case | 10 ns | 15 ns | 25 ns |
| --- | --- | --- | --- |
| G2X2 R EARLY | setup fail, -3.75 ns | setup met, 0.00 ns | setup met, +0.01 ns |
| G2X2 R GROUP | setup fail, -3.56 ns | setup met, 0.00 ns | setup met, 0.00 ns |
| L1X4 R EARLY | setup fail, -3.65 ns | setup met, +0.01 ns | setup met, 0.00 ns |
| L1X4 R GROUP | setup fail, -4.01 ns | setup met, 0.00 ns | setup met, 0.00 ns |

All twelve DC runs completed successfully (`metadata.txt: STATUS=PASS`).  The
four 10 ns points report setup violations; this is a measured result, not a
run failure.  The four 15 ns and four 25 ns points meet setup.  Max-capacitance
violations remain reportable independent of setup: G2X2 R GROUP has 19/1/1 at
10/15/25 ns and L1X4 R GROUP has 10/2/0.  No new point has a max-transition
violation.

## Method and provenance

The new points use the frozen CA-LIVE source manifest in
`provenance/SOURCE_SHA256SUMS`; all 26 files verify both before and after each
DC invocation.  No CA-LIVE RTL was modified.  DC is W-2024.09-SP2 using
TSMC018 `slow.db`, slow corner, zero I/O delay, `tsmc18_wl10`, output load
0.05, and the accepted literal command `compile -map_effort low` (this DC
release reports OPT-1303 and maps that command to medium effort).  Clock period
is the only swept constraint.

The RC rows reuse the committed four-period archive in
`../synthesis_timing_sweep/`.  Every 20 ns row reuses the committed
seven-case dataset.  The historical 20 ns source provenance for the four
R/L1X4 rows remains `PARTIAL_WITH_DOCUMENTED_LIMITATION`; the exact manifest
for these new points does not retroactively change that classification.
