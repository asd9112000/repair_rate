# N3 RC-GROUP functional closure status

## Closed evidence

- Strict readable-Verilog gate: PASS (candidate-store role metadata only).
- Fixed-mask analyzer directed: PASS, 9 cases, 0 mismatch; physical-column alias: PASS.
- Independent arbitrary-state analyzer oracle: PASS, 10,000 cases, seed `0x4e33524f`, 0 mismatch; all seven N3 capacity classes covered.
- Directed CA-LIVE preemption: Config #1/#2/#3/#4 PASS; same-edge old result/write suppressed; partial finalization forbidden; earlier-SA replay and stale generation mix are both zero.
- N3 distinctions: seventh pivot PASS; Hybrid entry #9 PASS; PatternID >15 full-top PASS.
- Full-top control/reconstruction corpus: 1k and 10k PASS, seed `0x4e335247`, 0 mismatch; this corpus is intentionally T0/PatternID-1 control evidence only.

## Final PatternID trace

`tb_n3_rc_group_high_pattern_full_top.cpp` reuses the deterministic independent-oracle generator.  Witness 2474 is legal 3R3C Config 0, PatternID 20 (`0b010100`).  Analyzer, 112-bit store write/read, selector, registered selected PatternID, and reconstruction each observe 20.  Store bits `[6:1]` retain high bits `[5:4]=01`.  All 4x7 reconstruction outputs match the full PatternID-20 oracle and differ from low-four-bit alias PatternID 4.

```
N3_VS_N2_DISTINCTION: PASS
PATTERN_ID_GT_15_DISTINCTION: PASS
PATTERN_ID_WIDTH_END_TO_END: 6
PATTERN_ID_TRUNCATION: NONE
N3_RC_GROUP_FUNCTIONAL: COMPLETE
LATENCY_CHARACTERIZATION: NOT_STARTED
DC_RUN: NO
COMMIT: NOT_CREATED
N3_R_GROUP: DEFERRED
```
