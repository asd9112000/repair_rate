# P3-BL-RTL-B — 1x4 EARLY candidate table

The frozen SYN-C candidate table is defined in
`P3BLRTLB_1X4_STREAMING_EARLY_IMPLEMENTATION.md`. It has ten requests:
`A0,A1,B0,B1,B2,C0,C1,C2,D0,D1`; therefore one shared analyzer needs ten
capture cycles. A/D support 2R2C then 3R2C; B/C additionally support 4R2C.

```text
ANALYZER_ENTRY_COUNT: 6
HYBRID_ENTRY_COUNT: 10
ANALYZER_INSTANCE_COUNT: 1
CANDIDATE_PRODUCTION_REQUESTS: 10
CANDIDATE_GENERATION_CYCLES: 10
```

PatternID is one-based and the stable mapping reference is
`{attemptIndex[1:0], PatternID[3:0]}`. The table keeps 180 regular slots but
only 94 are capacity-defined positions; endpoint attempt 2 slots are zero.
