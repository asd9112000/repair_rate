# Passive GROUP Pivot Address Retention

`dss_group_pivot_address_regs` retains the five fixed pivot row/physical-column
pairs for each of four SAs. The payload is 20 × (9 + 13) = 440 bits. Capture
is enabled only by the parent at the existing final collection slot; retained
state has no connection to analyzer or selector inputs.

At GROUP commit, the block reconstructs each active fixed pivot slot from the
already selected ConfigID and PatternID. Canonical transpose configurations
invert the canonical row-selection bit so that the emitted address remains a
physical row or physical column. No valid bits are stored: the existing GROUP
commit-valid and selected configuration define output validity.
