# N2 architecture contract

```text
RS = 2, CS = 2, m = 1
ROW_ADDR_W = 9
PHYS_COL_ADDR_W = 13
WORD_COL_ADDR_W = 5
HYBRID_LINE_ADDR_W = 13
MAX_K = 5
PIVOT_SLOTS_PER_SA = 5
```

`PHYS_COL_ADDR_W` identifies a physical repair column; the five-bit
`WORD_COL_ADDR_W` remains a logical BIST field and is not interchangeable with
a repair-line address. GROUP retains four SAs times five pivots times 22 bits,
or 440 raw pivot-address bits.

## Continuous-Analysis CA-LIVE contract

```text
AUTHORITATIVE_FAULT_STATE_OWNERSHIP: upstream collector
CA_LIVE_INTERFACE: 272-bit live active-SA state
ANALYZER_COUNT: 1
FAULT_UPDATE_PRIORITY: FAULT_UPDATE_WINS
DUPLICATED_PER_SA_ANALYZER_SNAPSHOT_BANK: NO
```

EARLY commits selected repair state immediately; GROUP defers selection and
retains canonical candidate/pivot reconstruction state. CA-LIVE changes neither
policy contract: it replaces prototype analyzer-input duplication with a live
authoritative-state interface.
