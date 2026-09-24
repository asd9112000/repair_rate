# G2X2_N2_RC_GROUP_reg

`CASE_ID: G2X2_N2_RC_GROUP_reg`
`LIFECYCLE: FINAL`

This is the delayed-decision directional HYP02 GROUP/static-global N2 case.
It uses `RS=2`, `CS=2`, `m=1`, with 9-bit rows, 13-bit physical repair
columns, 5-bit logical BIST word columns, and 13-bit hybrid lines. Full pivot
retention is present: four SAs × five pivots × (9 + 13) = 440 raw retained
address bits; this payload is not the complete synthesized GROUP overhead.

Physical-column and pivot-retention verification is PASS; matched DC synthesis
and working-to-synthesis/archive hash gates are PASS. The canonical archive is
`dss_final/G2X2_N2_RC_GROUP_reg/`; use
`dss_final/N2_HARDWARE_SUMMARY.md` for final PPA. Final closure commit:
`6351a82`.
