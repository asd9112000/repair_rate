# G2X2 N2 R EARLY

Status: FUNCTIONAL_FINAL_PENDING_PPA

- Policy: `SolutionTakePolicy::Grid2x2RowStaticEarly`
- Geometry: RS=CS=2, m=1, four SAs, five pivot slots per SA
- Address contract: row=9, physical-column=13, word-column=5, hybrid=13
- Timing: immediate sequential prefix commit; no deferred GROUP selection
- Row-only slot mapping: LOCAL/RB=2R2C, RELEASE=1R2C, BORROW=3R2C
- GROUP 440-bit row+column retention: absent
