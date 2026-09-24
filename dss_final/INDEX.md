# DATE2026 N2 final hardware index

## CURRENT / FINAL

| Case | Architecture | Lifecycle |
|---|---|---|
| `RECAM_N2_2R2C` | corrected physical-column RECAM | FINAL |
| `G2X2_N2_RC_EARLY` | directional RC streaming EARLY | FINAL |
| `G2X2_N2_RC_GROUP_reg` | directional RC retained-state GROUP | FINAL |
| `G2X2_N2_R_EARLY` | G2X2 row-only streaming EARLY | FINAL |
| `G2X2_N2_R_GROUP_reg` | G2X2 row-only retained-state GROUP | FINAL |
| `L1X4_N2_R_EARLY` | directed L1X4 row-only streaming EARLY | FINAL |
| `L1X4_N2_R_GROUP_reg` | directed L1X4 row-only retained-state GROUP | FINAL |

Each name above matches its archive directory, `CASE.md`, `README.md`, source
manifest, and synthesis report identity. The common address contract is defined
in `records/N2_ARCHITECTURE_CONTRACT.md`; final PPA and matched ratios are
authoritative only in `N2_HARDWARE_SUMMARY.md`.

## SUPPORTING ABLATIONS

- `ablation/G2X2_N2_RC_GROUP_7CFG_PAR_DFS_reg` — provenance-backed, seven-parallel-analyzer dense-map/DFS GROUP representation with canonical DATE2026 visible semantics. It is not a canonical eighth N2 case; see `records/N2_OPTIMIZATION_ABLATION.md`.

## Architecture boundary

EARLY is immediate commit with streaming selected repair lines and no four-SA
accumulated final-solution warehouse. GROUP delays selection and retains a
440-bit raw pivot-address payload (four SAs × five slots × (9-bit row + 13-bit
column)); this raw state alone does not explain the GROUP-versus-EARLY PPA
delta.

## SUPERSEDED

- Historical Phase-3A RECAM with 5-bit physical repair-column representation is
  superseded for matched comparison by `RECAM_N2_2R2C`.
- `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch`
  is superseded by `G2X2_N2_RC_GROUP_reg`.
- `recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch` is superseded
  by `G2X2_N2_R_GROUP_reg`.
- `L1X4_R_GROUP_V1` is superseded by `L1X4_N2_R_GROUP_reg`.

## HISTORICAL

- `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch` and
  `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch` are preserved
  EARLY experiment packages, not current canonical N2 results.
- Historical RECAM policy evidence remains preserved under `rtl/recam/`; only
  its former 5-bit physical-column PPA is excluded from canonical comparison.
