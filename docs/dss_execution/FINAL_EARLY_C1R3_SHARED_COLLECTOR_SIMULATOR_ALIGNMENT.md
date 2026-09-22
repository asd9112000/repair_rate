# FINAL-EARLY-C1R3 — Directional EARLY Model-B2 Shared-Collector Alignment

```text
FINAL_EARLY_C1R3_STATUS: COMPLETE
DSS_SIMULATOR_MODEL: B2_SHARED_COLLECTOR
STANDALONE_RECAM_BASELINE_CHANGED: NO
RTL_MODIFIED: NO
DSS_FINAL_MODIFIED: NO
SYNTHESIS_RUN: NO
FORMAL_SWEEP_RUN: NO
```

## Scope and root cause

This change applies only to `SolutionTakePolicy::DirectionalV2Early` in the
2x2 directional DSS group simulator.  Before this change, every capacity role
created a separate `RECAMSolverAdapter`/`FaultList`/`RECAM_PE`; therefore a
2R1C Config could reject a nonpivot because its *hypothetical standalone*
Hybrid CAM was smaller.  Vector 216 C:R was the known example.

The corrected path follows the C1R2 B2 boundary:

```text
one maximum retained collector state per SA
  -> shared pivots, Hybrid records, counts, and pivot-buffer state
  -> all four Config analyses over that same state
```

The collector derives its maximum rows/columns from the four legal V2
requests for that SA.  It uses the retained five-pivot / eleven-reachable-
Hybrid envelope; the old seven-entry Phase-3B analyzer input port is not a
collector capacity.  Config analysis still applies its own `Rs/Cs`, Must
thresholds, active matrix dimension, candidate set, and PatternID.  It no
longer rejects an already retained Hybrid record by reapplying a standalone
Config Hybrid-capacity limit.

## Changed implementation

| File | Change |
| --- | --- |
| `inc/SharedCollectorRecam.hpp`, `src/SharedCollectorRecam.cpp` | New Model-B2 collector/analyzer. `collect()` retains the maximum shared state; `analyze()` projects each request's Must/matrix/candidates. |
| `src/DynamicRepairSimulator.cpp` | `runAttempt` batches all V2 capacity options through the shared collector only for `DirectionalV2Early`. The existing adapter remains the route for every other policy. |
| `inc/RepairResult.hpp` | Adds a row-major shared-matrix diagnostic snapshot used solely by the B2 regression. Legacy adapter results leave it empty. |
| `tests/final_early_c1r3_shared_collector_test.cpp`, `Makefile` | Independent expected-matrix/oracle and C1R3 replay target. |

`RECAMSolverAdapter`, `RECAM_PE`, standalone baselines, Directional GROUP
analyzer, 1x4 paths, and non-Directional policies were not changed.

## Vector 216 and independent oracle

The regression's hand-derived 2R1C shared-state reference has two pivots and
two same-column Hybrid records.  It checks the complete row-major matrix,
validity, lowest PatternID, and RowMust projection without reusing a production
rejection helper.  It also checks a distinct RowMust vector and 128 translated
Hybrid vectors.

```text
MODEL_B2_ORACLE_INDEPENDENT: YES
MODEL_B2_ORACLE: PASS (130 vectors)
VECTOR216_SHARED_SIMULATOR_R: VALID
VECTOR216_SHARED_RTL_R: VALID (C1R2 canonical analyzer cross-check)
VECTOR216_SHARED_R_PATTERN: 2
VECTOR216_SIM_RTL_MATCH: PASS
VECTOR216_REACHES_L: NO
```

The RTL comparison is limited to Vector 216: C1R2 drove the canonical shared
analyzer with the complete shared state and obtained valid PatternID 2.  The
new C++ result is the same.  The 16 replayed cases have independent C++ oracle
coverage but were not individually re-driven through an RTL testbench; thus no
claim of a new 16-vector RTL batch was made.

## Analogous C1R cases

The exact 16 entries from C1R's `low_demand_r_invalid_l_valid.csv` were
regenerated from the same seed/corpus coordinates.  All now have valid R;
fifteen select PatternID 2 and C/16/7 selects PatternID 3.

```text
TOTAL_ANALOGOUS_CASES: 16
BECOME_EARLIER_CONFIG_VALID: 16
REMAIN_INVALID: 0
SIMULATOR_RTL_MISMATCHES: 0 for the Vector216 RTL-equivalent check; 16-case RTL batch not run
```

The temporary per-case evidence is
`tmp/date2026/final_early_c1r3/analogous_16/replay.csv` and is intentionally
not committed.

## Claim-mask and demand decision

The independent four-bit replay observes 23 `(SA, role, higher-priority
failure-history)` contexts across the four-policy corpus.  The same context
never produced two common-line claim masks.  It explicitly encountered both
`B:L:R=I` and `C:L:R=I`; Vector 216 takes C:R and never reaches C:L.

```text
RESOURCE_ACTION_AMBIGUITIES_MODEL_B2: 0 (this C1R3 corpus/replay)
ACTUAL_DEMAND_OUTPUT_REQUIRED_MODEL_B2: NO (for static action selection in this corpus)
USED_ROWS_OUTPUT_REQUIRED_MODEL_B2: NO
USED_COLUMNS_OUTPUT_REQUIRED_MODEL_B2: NO
FOUR_BIT_STATIC_CLAIM_MODEL_VIABLE_MODEL_B2: YES (bounded empirical result)
```

These are bounded simulator observations, not a universal proof that future
topologies or a wider configuration envelope need no diagnostic demand
observability.

## Development-only paired policy study

All cases use the same deterministic Moderate-Imbalance/Mixed groups:
`RS=CS=2`, directional `m=1`, seed `20260922`, F_GROUP `16,20,24,28`, 300
groups/point.  `P0=R,L,RB,B`; `P1=R,RB,L,B`.  The production default remains
P0/ABCD; P1 and ABDC are only test-local replays.

| F_GROUP | Case0 ABCD/P0 | Case1 ABCD/P1 | Case2 ABDC/P0 | Case3 ABDC/P1 | ABCD P1-only / P0-only | ABDC P1-only / P0-only | P0 ABDC-only / ABCD-only | P1 ABDC-only / ABCD-only |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 93.666667% | 93.666667% | 93.666667% | 93.666667% | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| 20 | 78.333333% | 78.333333% | 78.333333% | 78.333333% | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| 24 | 61.666667% | 60.333333% | 61.666667% | 60.333333% | 0 / 4 | 0 / 4 | 0 / 0 | 0 / 0 |
| 28 | 38.333333% | 38.333333% | 38.333333% | 38.333333% | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |

Across 1,200 paired groups, Case0/Case2 repair 816 (68.000000%) and
Case1/Case3 repair 812 (67.666667%).  P1-only is 0 and P0-only is 4; all four
order-only paired counts are 0.

```text
RECOMMENDED_FINAL_PRIORITY: R,L,RB,B
RECOMMENDED_FINAL_SA_ORDER: ABCD
```

This is development evidence only.  It does not change production priority or
SA order and is not a formal repair-rate sweep.

## Validation

| Command | Result |
| --- | --- |
| `make test_final_early_c1r3_shared_collector` | PASS — independent oracle, Vector216, analogous 16, 1,200-group four-policy replay |
| `make test_solution_take_policy` | PASS — existing standalone/policy checks |
| `make test_recam_paper_faithful` | PASS — standalone RECAM baseline |
| `make test_directional_multi_config_analyzer` | PASS — Directional GROUP analyzer |
| `make test_dynamic_spare_sharing_policy test_dynamic_spare_sharing_layout` | PASS — non-Directional and 1x4/layout coverage |

Temporary artifacts are restricted to `tmp/date2026/final_early_c1r3/`.
Historical Directional EARLY outputs remain `PRE_MODEL_B2`; only results from
this implementation are `SHARED_COLLECTOR_MODEL_B2`.

## Stop condition

```text
NEXT_ACTION: STOP FOR HUMAN POLICY FREEZE BEFORE RESUMING FINAL EARLY RTL
```
