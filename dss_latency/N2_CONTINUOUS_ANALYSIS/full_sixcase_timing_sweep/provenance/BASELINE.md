# Provenance baseline

```text
BASELINE_COMMIT: a2a61b96280275019ab06f9e120c4951466b9777
CANONICAL_LOOKUP: dss_final/CURRENT_N2_CANONICAL.md
FROZEN_CA_LIVE_FAMILY: dss_final/continuous_analysis_live/
WORKING_CA_LIVE_RTL: rtl/*_CONTINUOUS_ANALYSIS_LIVE_STATE_reg
RTL_MODIFIED_BY_THIS_SWEEP: NO
```

The four newly characterized cases retain their historical 20 ns source
provenance classification of `PARTIAL_WITH_DOCUMENTED_LIMITATION`.  This
archive supplies an exact source manifest for the new 10, 15, and 25 ns runs;
it does not retroactively change the historical 20 ns classification.

The matched methodology is DC W-2024.09-SP2 with TSMC018 `slow.db`, slow
corner, zero I/O delay, `tsmc18_wl10`, output load 0.05, and
`compile -map_effort low`.  `NAND2X1 = 9.979200 um^2`.  Only the clock period
varies across 10, 15, 20, and 25 ns.
