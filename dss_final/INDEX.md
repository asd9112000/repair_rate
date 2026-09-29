# DATE2026 N2 final hardware index

## CURRENT N2 HARDWARE FAMILIES

| Family | Path | Status | Default use |
|---|---|---|---|
| RECAM baseline | `RECAM_N2_2R2C` | `VALID_NON_CA_CANONICAL_REFERENCE` | matched baseline |
| NON-CA canonical DSS | `G2X2_N2_*`, `L1X4_N2_*` packages | `VALID_NON_CA_CANONICAL_REFERENCE` | CA-overhead baseline |
| CA-LIVE canonical DSS | [continuous_analysis_live/](continuous_analysis_live/INDEX.md) | `CANONICAL_CA_LIVE` | final CA PPA and latency |
| Historical/prototype/ablation | `rtl/*_CONTINUOUS_ANALYSIS_reg`, `ablation/` | non-default | preserved evidence only |

## NON-CA CANONICAL REFERENCE

- `RECAM_N2_2R2C`
- `G2X2_N2_RC_EARLY`, `G2X2_N2_RC_GROUP_reg`
- `G2X2_N2_R_EARLY`, `G2X2_N2_R_GROUP_reg`
- `L1X4_N2_R_EARLY`, `L1X4_N2_R_GROUP_reg`

These remain valid hardware references; they are not the primary CA family.

## CA-LIVE CANONICAL DSS

The six primary CA implementations are in
[continuous_analysis_live/INDEX.md](continuous_analysis_live/INDEX.md). They
use one 272-bit live active-SA interface and no duplicated four-by-272-bit
analyzer-state bank.

## HISTORICAL / ABLATION

- `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_reg` and
  `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_reg` are `CA_SB` historical
  prototypes, `NOT_FOR_FINAL_MATCHED_CA_PPA`.
- `ablation/G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg` is supporting evidence, not an
  eighth canonical N2 case.
