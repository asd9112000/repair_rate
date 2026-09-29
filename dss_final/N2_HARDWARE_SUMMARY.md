# DATE2026 N2 hardware summary

All rows use the documented 20 ns matched methodology: DC W-2024.09-SP2,
TSMC018 `slow.db`, slow corner, zero I/O delay, and `NAND2X1 = 9.979200` um2.
The families below are intentionally separate and must not be silently
substituted for one another.

## NON-CA canonical PPA

`VALID_NON_CA_CANONICAL_REFERENCE`: valid CA-overhead baselines and historical
architecture comparators, not final Continuous-Analysis PPA.

| Case | Cell area (um2) | GE |
|---|---:|---:|
| `RECAM_N2_2R2C` | 72116.352610 | 7226.6667 |
| `G2X2_N2_RC_EARLY` | 102296.779959 | 10251.0001 |
| `G2X2_N2_RC_GROUP_reg` | 166140.376634 | 16648.6668 |
| `G2X2_N2_R_EARLY` | 99688.882299 | 9989.6668 |
| `G2X2_N2_R_GROUP_reg` | 158230.197352 | 15856.0001 |
| `L1X4_N2_R_EARLY` | 99695.535153 | 9990.3334 |
| `L1X4_N2_R_GROUP_reg` | 154388.205311 | 15471.0001 |

## CA-LIVE seven-case PPA

`N2_SEVEN_CASE_20NS_CA_LIVE_MATCHED` is the authoritative final
Continuous-Analysis dataset. The PPA/latency source is
`dss_latency/N2_CONTINUOUS_ANALYSIS/seven_case_20ns/`.

| Case | Cell area (um2) | GE | Historical provenance |
|---|---:|---:|---|
| `RECAM_N2_2R2C` | 72116.352610 | 7226.6667 | FULL baseline archive |
| `G2X2_RC_EARLY_CA_LIVE` | 104668.503253 | 10488.6668 | FULL |
| `G2X2_RC_GROUP_CA_LIVE` | 171399.415025 | 17175.6669 | FULL |
| `G2X2_R_EARLY_CA_LIVE` | 99236.491949 | 9944.3334 | PARTIAL; source hash unknown |
| `G2X2_R_GROUP_CA_LIVE` | 160086.328417 | 16042.0002 | PARTIAL; source hash unknown |
| `L1X4_R_EARLY_CA_LIVE` | 102486.384861 | 10270.0001 | PARTIAL; source hash unknown |
| `L1X4_R_GROUP_CA_LIVE` | 157295.478829 | 15762.3335 | PARTIAL; source hash unknown |

For CA-LIVE timing, cite raw report fields as `Critical Path Slack` and
`Design WNS` separately; do not relabel positive critical-path slack as Design
WNS. All seven meet the 20 ns timing target with zero timing-violating paths.
The GROUP cases are not fully electrical-constraint-clean: max-cap violations
are G2X2-RC GROUP = 9, G2X2-R GROUP = 1, and L1X4-R GROUP = 2.
