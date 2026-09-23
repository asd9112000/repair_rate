# G2X2 N2 RC EARLY

Status: FUNCTIONAL_FINAL_PENDING_PPA

- Policy: `SolutionTakePolicy::Hyp02StaticEarly`
- Geometry: RS=CS=2, m=1, four SAs, five pivot slots per SA
- Address contract: row=9, physical-column=13, word-column=5, hybrid=13
- Timing: immediate sequential prefix commit; no deferred GROUP selection
- Final output state: selected line only, 20 × (13-bit address + row flag + valid)
- GROUP 440-bit row+column retention: absent

The current analyzer, selected-address register, and controller have focused
Verilator and independent-oracle evidence. Matched DC and archive promotion
remain pending.
