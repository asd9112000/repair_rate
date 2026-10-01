# N2 six-case timing-sweep run manifest

```text
BASELINE_COMMIT: a2a61b96280275019ab06f9e120c4951466b9777
BRANCH: integration/date2026-n2-full-timing-sweep
DC_VERSION: W-2024.09-SP2
LIBRARY: TSMC018 slow.db
OPERATING_CONDITION: slow
CLOCK_PERIODS_NS: 10.0, 15.0, 20.0, 25.0
INPUT_DELAY_NS: 0.0
OUTPUT_DELAY_NS: 0.0
WIRE_LOAD_MODEL: tsmc18_wl10
OUTPUT_LOAD: 0.05
MAX_TRANSITION: 3.0
MAX_CAPACITANCE: 0.15
MAX_FANOUT: 10
COMPILE_COMMAND: compile -map_effort low
NAND2X1_AREA_UM2: 9.979200
FROZEN_RTL_CHANGED: NO
```

| New raw case | 10 ns | 15 ns | 25 ns | Exact source-hash check |
| --- | --- | --- | --- | --- |
| G2X2 R EARLY CA-LIVE | PASS | PASS | PASS | PASS before and after each point |
| G2X2 R GROUP CA-LIVE | PASS | PASS | PASS | PASS before and after each point |
| L1X4 R EARLY CA-LIVE | PASS | PASS | PASS | PASS before and after each point |
| L1X4 R GROUP CA-LIVE | PASS | PASS | PASS | PASS before and after each point |

`PASS` in this table means the DC invocation reached report generation and its
metadata contains `STATUS=PASS`; it does not assert setup closure.  The raw
reports and `SUMMARY.csv` retain actual setup and design-rule measurements.

The six 20 ns rows are reused from the committed seven-case 20 ns dataset.
The two RC four-period rows are reused from the committed RC timing sweep.
The four R/L1X4 historical 20 ns rows remain
`PARTIAL_WITH_DOCUMENTED_LIMITATION` for historical run-source provenance.
