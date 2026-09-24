# L1X4_N2_R_GROUP_reg

`CASE_ID: L1X4_N2_R_GROUP_reg`
`LIFECYCLE: FINAL`

This is the directed `A -> B -> C -> D` row-only N2 GROUP/static-global case.
Decision is delayed until all four SAs are available. The contract is `RS=2`,
`CS=2`, `m=1`, row address 9, physical repair column 13, logical BIST word
column 5, and hybrid line 13. Full pivot retention is present: four SAs × five
pivots × (9 + 13) = 440 raw retained address bits, not the full GROUP overhead.

Physical-column and pivot-retention verification is PASS; matched DC synthesis
and working-to-synthesis/archive hash gates are PASS. The canonical archive is
`dss_final/L1X4_N2_R_GROUP_reg/`; final PPA belongs in
`dss_final/N2_HARDWARE_SUMMARY.md`. Final closure commit: `6351a82`.
