# G2X2 N2 RC GROUP Continuous Analysis StateBank prototype

`CASE_ID: G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_reg`

```text
FAMILY: CA_SB
STATUS: HISTORICAL_CA_SB_PROTOTYPE
FINAL_CA_IMPLEMENTATION: NO
FINAL_MATCHED_PPA: PROHIBITED
SUPERSEDED_BY: rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg
```

This isolated variant retains completed SA candidate records and prototype
analyzer-input state for selective rescan. The canonical analyzer, slot decode,
candidate store, selector, and reconstruction register are reused. It is
preserved as functional/prototype evidence, but is not the final CA
implementation and must not be used for final matched PPA or latency reporting.
