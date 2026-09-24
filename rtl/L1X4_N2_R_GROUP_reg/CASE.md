# L1X4_N2_R_GROUP_reg working case

`CASE_ID: L1X4_N2_R_GROUP_reg`
`LIFECYCLE: FINAL`

Architecture: directed `A -> B -> C -> D` row-only N2 GROUP/static-global.
Decision is delayed until all four SAs are available and full pivot retention
is present. The corrected contract is `RS=2`, `CS=2`, `m=1`,
`ROW_ADDR_W=9`, `PHYS_COL_ADDR_W=13`, `WORD_COL_ADDR_W=5`,
`HYBRID_LINE_ADDR_W=13`, `MAX_K=5`, and five pivot slots per SA. The 440 raw
pivot-address bits are not the full synthesized GROUP overhead.

Physical-column and pivot-retention verification is PASS; matched DC synthesis
and working-to-synthesis/archive hash gates are PASS. Canonical package:
`dss_final/L1X4_N2_R_GROUP_reg/`; final PPA is in
`dss_final/N2_HARDWARE_SUMMARY.md`; final closure commit: `6351a82`.
