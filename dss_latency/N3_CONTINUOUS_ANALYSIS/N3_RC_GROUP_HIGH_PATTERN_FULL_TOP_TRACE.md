# N3 RC-GROUP PatternID >15 Full-Top Trace

Permanent directed test: `tb/n3_continuous_analysis/tb_n3_rc_group_high_pattern_full_top.cpp`.

The test reuses the deterministic independent-oracle state generator.  Witness index 2474 with seed `0x4e33524f` is a legal 3R3C, Config 0 analyzer case selecting PatternID 20 (`0b010100`).  The 4-SA scenario uses this legal Config-0 candidate for every SA, so frozen selector priority chooses its legal T0 tuple without changing priority, config order, or RTL semantics.

| Trace point | Observed PatternID |
|---|---:|
| analyzer output | 20 |
| candidate-store write | 20 |
| candidate-store image/read record | 20 |
| GROUP selector | 20 |
| registered `selected_pattern_flat` field | 20 |
| reconstruction mask selection | 20 |

Each production store record is `{PatternID[5:0], valid}` and contains `0b010100` in bits `[6:1]`; high bits `[5:4]` are preserved as `01`.

Reconstruction is compared for every SA and pivot slot against the full PatternID-20 3R3C fixed mask: line-valid, row/column identity, and 13-bit physical address all match.  The low-four-bit alias is PatternID 4 and has a distinct fixed mask, so the observed reconstruction rejects truncated-alias behavior.

Result: `PATTERN_ID_GT_15_FULL_TOP=PASS`, `PATTERN_ID_GT_15_DISTINCTION=PASS`, and `N3_VS_N2_DISTINCTION=PASS`.  This test changes no production RTL.
