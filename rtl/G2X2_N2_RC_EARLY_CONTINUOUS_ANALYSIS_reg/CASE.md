# G2X2 N2 RC EARLY Continuous Analysis StateBank prototype

`CASE_ID: G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_reg`

```text
FAMILY: CA_SB
STATUS: HISTORICAL_CA_SB_PROTOTYPE
FINAL_CA_IMPLEMENTATION: NO
FINAL_MATCHED_PPA: PROHIBITED
SUPERSEDED_BY: rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg
```

This isolated timing/storage variant retains one analyzer-state bank per SA and
restarts only the updated SA's four-config priority scan. The analyzer and
selected-address mux are reused from the non-CA canonical case. It is
preserved as functional/prototype evidence, but is not the final CA
implementation and must not be used for final matched PPA or latency reporting.
