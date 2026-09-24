# L1X4_N2_R_EARLY

`CASE_ID: L1X4_N2_R_EARLY`
`LIFECYCLE: FINAL`

N2 directed `A -> B -> C -> D` row-only EARLY/static implementation with
`RS=2`, `CS=2`, `m=1`, row address 9, physical repair column 13, logical BIST
word column 5, hybrid line 13, and five pivot slots per SA. Policy identity:
`SolutionTakePolicy::Line1x4RowStaticEarly`.

The architecture performs streaming immediate commit and has no four-SA
accumulated final-solution storage or GROUP-style full-pivot retention. The
27-path independent oracle, random vectors, physical-column checks, source
hash gates, and matched synthesis are PASS. Canonical archive:
`dss_final/L1X4_N2_R_EARLY/`; final PPA:
`dss_final/N2_HARDWARE_SUMMARY.md`; final closure commit: `6351a82`.
