# N2 Continuous-Analysis contract

```text
PRIMARY_CA_FAMILY:
dss_final/continuous_analysis_live/
```

| Family | Status | Final CA PPA? | Purpose |
|---|---|---:|---|
| `NON_CA_CANONICAL` | valid reference | No | CA-overhead baseline |
| `CA_SB` | historical prototype | No | functional/prototype history |
| `CA_LIVE` | canonical | Yes | final Continuous-Analysis implementation |

`CA_SB` stores four duplicated 272-bit analyzer-input snapshots, requires a
wide state selector, and has a synthesis-boundary mismatch. It is preserved,
not deleted, but is `SUPERSEDED_FOR_FINAL_CA_PPA`.

`CA_LIVE` takes upstream authoritative fault/CAM state through one 272-bit live
active-SA interface, invokes one canonical analyzer, and has no duplicated
analyzer-state bank. Its matched PPA boundary excludes upstream storage
consistently.

The six CA-LIVE packages are:

- `dss_final/continuous_analysis_live/G2X2_N2_RC_EARLY_CA_LIVE/`
- `dss_final/continuous_analysis_live/G2X2_N2_RC_GROUP_CA_LIVE/`
- `dss_final/continuous_analysis_live/G2X2_N2_R_EARLY_CA_LIVE/`
- `dss_final/continuous_analysis_live/G2X2_N2_R_GROUP_CA_LIVE/`
- `dss_final/continuous_analysis_live/L1X4_N2_R_EARLY_CA_LIVE/`
- `dss_final/continuous_analysis_live/L1X4_N2_R_GROUP_CA_LIVE/`
