# L1X4 N2 R EARLY

`CASE_ID: L1X4_N2_R_EARLY`



`LIFECYCLE: FINAL`

- Policy: `SolutionTakePolicy::Line1x4RowStaticEarly`
- Topology: A -> B -> C -> D, single-hop rows only
- Geometry: RS=CS=2, m=1, four SAs, five pivot slots per SA
- Address contract: row=9, physical-column=13, word-column=5, hybrid=13
- Timing: immediate sequential prefix commit; no GROUP 27-path selector
- Constraints: no reverse sharing, wraparound, transitive borrowing, or re-lending
- GROUP 440-bit row+column retention: absent
