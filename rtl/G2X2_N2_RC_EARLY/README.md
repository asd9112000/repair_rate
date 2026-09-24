# G2X2_N2_RC_EARLY

`CASE_ID: G2X2_N2_RC_EARLY`
`LIFECYCLE: FINAL`

N2 directional HYP02 EARLY/static implementation with `RS=2`, `CS=2`, `m=1`,
row address 9, physical repair column 13, logical BIST word column 5, hybrid
line 13, and five pivot slots per SA. Policy identity:
`SolutionTakePolicy::Hyp02StaticEarly`.

The architecture performs streaming immediate commit: each SA decision is
observable through the selected repair-line transaction. Four-SA accumulated
final-solution storage is absent, as is GROUP-style full-pivot retention.
Physical-column, 81-path oracle, directed, source-hash, and matched synthesis
evidence are PASS. Canonical archive: `dss_final/G2X2_N2_RC_EARLY/`; final PPA:
`dss_final/N2_HARDWARE_SUMMARY.md`; final closure commit: `6351a82`.
