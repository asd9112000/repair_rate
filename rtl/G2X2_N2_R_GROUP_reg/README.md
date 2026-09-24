# G2X2_N2_R_GROUP_reg

`CASE_ID: G2X2_N2_R_GROUP_reg`
`LIFECYCLE: FINAL`

This is the row-only G2X2 N2 GROUP/static-global case with delayed decision.
Its contract is `RS=2`, `CS=2`, `m=1`, `ROW_ADDR_W=9`,
`PHYS_COL_ADDR_W=13`, `WORD_COL_ADDR_W=5`, and `HYBRID_LINE_ADDR_W=13`.
Full pivot retention is present: four SAs × five pivots × (9 + 13) = 440 raw
retained address bits, not the complete synthesized GROUP overhead.

Physical-column and pivot-retention verification is PASS; matched DC synthesis
and working-to-synthesis/archive hash gates are PASS. The canonical archive is
`dss_final/G2X2_N2_R_GROUP_reg/`; final PPA belongs in
`dss_final/N2_HARDWARE_SUMMARY.md`. Final closure commit: `6351a82`.
