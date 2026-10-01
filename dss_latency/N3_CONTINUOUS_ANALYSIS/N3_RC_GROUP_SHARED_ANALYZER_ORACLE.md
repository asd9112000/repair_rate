# N3 RC-GROUP shared analyzer oracle

## Scope and API

Layer A extracts only the production-independent analyzer reference model into:

- `tb/n3_continuous_analysis/n3_rc_analyzer_oracle.hpp`
- `tb/n3_continuous_analysis/n3_rc_analyzer_oracle.cpp`

The public model types are `AnalyzerState`, `CapacityRequest`, `AnalyzerResult`, and `FixedMaskMap`.  `evaluate()` maps the full semantic state plus a requested capacity/transpose class to `{repairable, PatternID[5:0]}`.  `generate_random_state()` preserves the closed deterministic corpus generator.

The state-width assertion is:

```
7 + 7*9 + 7*13 + 4*7 + 4*7 + 17 + 17*3 + 17 + 17*13 + 1 = 524
```

The fixed-mask source remains `N3_RC_GROUP_FIXED_MASK_EQUIVALENCE.csv`.  Loading requires exactly 80 unique rows.  Supported normalized counts are 3R3C=20, 3R2C/2R3C=10, 4R3C/3R4C=35, and 4R2C/2R4C=15; transpose maps the paired capacity classes without runtime pattern enumeration.

## Refactored tests and regression

`tb_n3_rc_analyzer_random_lockstep.cpp` and `tb_n3_rc_analyzer_distinctions.cpp` now use normal header inclusion and source linkage.  Neither includes another test executable or renames `main`.

```
ANALYZER_RANDOM_REGRESSION: PASS
ANALYZER_RANDOM_CASES: 10000
ANALYZER_RANDOM_MISMATCHES: 0
SEED: 0x4e33524f
FEASIBLE: 16
UNREPAIRABLE: 9984
ORACLE_SEMANTIC_DRIFT: NONE

SEVENTH_PIVOT_DISTINCTION: PASS
HYBRID_GT_7_DISTINCTION: PASS
PATTERN_ID_GT_15_ANALYZER_WITNESS: PASS
PHYSICAL_COLUMN_ALIAS_1_VS_257: PASS
```

The random result and complete pattern histogram match the pre-refactor run exactly.

## Main and integrity audit

| Source | Role | `main` count |
|---|---|---:|
| `n3_rc_analyzer_oracle.hpp` | shared declaration | 0 |
| `n3_rc_analyzer_oracle.cpp` | shared implementation | 0 |
| `tb_n3_rc_analyzer_random_lockstep.cpp` | executable | 1 |
| `tb_n3_rc_analyzer_distinctions.cpp` | executable | 1 |

`DUPLICATE_MAIN=NO`.  All seven production RTL SHA-256 values are byte-identical before and after Layer A.  No GROUP selector, reconstruction, full-top generator, latency harness, source freeze, or DC work is part of this layer.
