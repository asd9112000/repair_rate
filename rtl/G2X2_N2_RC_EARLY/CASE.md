# G2X2 N2 RC EARLY

`CASE_ID: G2X2_N2_RC_EARLY`



`LIFECYCLE: FINAL`

- Policy: `SolutionTakePolicy::Hyp02StaticEarly`
- Geometry: RS=CS=2, m=1, four SAs, five pivot slots per SA
- Address contract: row=9, physical-column=13, word-column=5, hybrid=13
- Timing: immediate sequential prefix commit; no deferred GROUP selection
- Final output boundary: immediate streaming `solution_commit` transaction; no accumulated four-SA solution state
- GROUP 440-bit row+column retention: absent

The current analyzer, selected-address mux, and controller have focused
Verilator and independent-oracle evidence. Matched DC synthesis and archive promotion are complete.
