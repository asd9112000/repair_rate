# Phase 4I post-BIST latency provenance audit

## Question and conclusion

This read-only audit identifies the execution model behind the Phase 4I
`G=0` means of 4.0095 cycles (EARLY) and 20.3156 cycles (GROUP).  It does not
rerun or alter any experiment.

**Conclusion:** Phase 4I is a frozen historical V2 decision execution model,
not the later final N2 G2X2 RC CA-LIVE policy-cycle model.  Its historical
V2 evidence remains valid at its stated decision boundary, but it must not be
used as final CA-LIVE latency evidence.

## Exact artifacts and Git provenance

| Role | Path | First / last relevant commit | Current blob |
|---|---|---|---|
| Primary sweep | `tests/dss_post_bist_latency_sweep_test.cpp` | `3304c86` / `3304c86` | `2be1e8ea2cc716029b7774371fa263552dbb7cc7` |
| Timing model | `inc/DssPostBistLatency.hpp`, `src/DssPostBistLatency.cpp` | `3304c86` / `3304c86` | `87cf081f3ca0a9bb8b7f7bef7fa4a1457c86d103`, `737801dcb2165d54b3295b43ecc42a167e1aba9b` |
| Raw sidecar | `results/phase4i/phase4i2_raw_latency.csv` | `3304c86` / `3304c86` | `dd0e6200a12ef54fc3bfd08c7f61ff88a059dd13` |
| Aggregate sidecar | `results/phase4i/phase4i2_summary.csv` | `3304c86` / `3304c86` | `9bec3a2817781442f8d83a64181067fd0339e61e` |
| Final report | `docs_verilog/PHASE4I_FINAL_CHARACTERIZATION.md` | `3304c86` / `3304c86` | `c3f5c519060dd12874c3fb9b5d9827aed4333124` |

`3304c86` is `DATE 2x2 direction rtl done` (2026-09-13).  `git blame` assigns
the relevant work-plan, timing arithmetic, raw-output, and report lines to
that commit.  The listed primary artifacts are unchanged relative to it.

There is no separate persisted input trace.  The primary sweep creates each
input with `DynamicFaultGenerator`, then constructs the deterministic A→B→C→D
one-changing-fault-per-cycle event trace in
`tests/dss_post_bist_latency_sweep_test.cpp:276-307`.  The persisted raw
output is the `phase4i2_raw_latency.csv` sidecar above.

## Actual timing path

The primary sweep first uses `DirectionalMultiConfigAnalyzer`, then applies
the frozen V2 rank, donor order, atomic ledger, and first-failure rules to
form `DssDecisionWorkPlan` (`tests/dss_post_bist_latency_sweep_test.cpp:165-273`).
It supplies `T_BIST_end = T_last_fault + G` (`:297-307`).

`src/DssPostBistLatency.cpp:100-154` performs the timing arithmetic:

```text
T_last_fault       = last event whose changesFaultSet is true
T_BIST_end         = request.bistEndCycle = T_last_fault + G
W                  = decisionExecutionCycles(finalDecisionPlan)
T_decision_ready   = T_last_fault + W
L_post             = max(0, T_decision_ready - T_BIST_end)
                   = max(0, W - G)
```

The model restarts an attempt for every changing event; only the last one can
reach the observed ready boundary.  `T_BIST_end` is a supplied timestamp, not
a production-DUT input.  At `G=0`, the sweep explicitly checks that
`L_post == latencyFromLastFaultCycles`.

## EARLY provenance and 4.0095

Phase 4I EARLY is **V2 Specialized EARLY**, with the frozen V2 per-SA rank
and sequential active-SA allocation work.  Its execution work is the sum of
one-to-four candidate evaluations for each active SA; it is not the final
CA-LIVE current/final-state ranked-config latency.

Existing `G=0` raw records give this exact base-work distribution:

| EARLY execution / L_last_fault cycles | Count |
|---:|---:|
| 4 | 9,949 |
| 5 | 20 |
| 6 | 18 |
| 7 | 13 |

Thus `sum = 40,095`, `N = 10,000`, and `mean = 4.0095`; the 95 extra cycles
above the 4-cycle dominant case are the rare additional historical ranked
candidate evaluations.  The recovered distribution is min/mean/median/P95/max
`4 / 4.0095 / 4 / 4 / 7` cycles.

```text
L_POST_FORMULA_EARLY = max(0, sum(active-SA candidate evaluations) - G)
PHASE4I_EARLY_MODEL_MATCHES_FINAL_CA_LIVE = NO
```

The later final CA-LIVE record instead measures accepted live state update to
solution ready: `1 / 1.31114 / 1 / 3 / 4` cycles.  Its current/final-state
semantics are therefore different in both timing origin and work retained
from earlier SAs.

## GROUP provenance and 20.3156

Phase 4I GROUP is **V2 GROUP-NoScratch**.  It does not import a literal
20/21/22/23/26 result table.  It indirectly derives the same historical
execution regime by encoding it in `decisionExecutionCycles`:

```text
W_GROUP = 16 fixed canonical candidate-collection cycles
        + sum(active A→D sequential allocation candidate evaluations)
```

The fixed 16 is explicitly documented in `src/DssPostBistLatency.cpp:69-76`;
the sum is accumulated over active A→D SAs in `:43-67`.  For a repairable
all-local case the allocation sum is `1+1+1+1=4`, producing 20 cycles.
Historical V2 RTL latency tests independently encode all-local 20,
failure-B/C/D 21/22/23, slot2-last 26, and release+borrow 22 cycles in
`tb/dss_v2/recam_dss_v2_group_latency_test.cpp:47-70`.

Existing `G=0` raw records give:

| GROUP execution / L_last_fault cycles | Count |
|---:|---:|
| 20 | 7,218 |
| 21 | 2,431 |
| 22 | 330 |
| 23 | 19 |
| 24 | 2 |

Thus `sum = 203,156`, `N = 10,000`, and `mean = 20.3156`.  The recovered
distribution is min/mean/median/P95/max `20 / 20.3156 / 20 / 21 / 24` cycles.

```text
L_POST_FORMULA_GROUP = max(0, 16 + sum(active A→D allocation evaluations) - G)
PHASE4I_USES_HISTORICAL_GROUP_EXECUTION_TABLE = INDIRECT
GROUP_ALL_SA_SERIAL_ACCUMULATION = YES
PHASE4I_GROUP_MODEL_MATCHES_FINAL_CA_LIVE = NO
```

The `INDIRECT` classification means that there is no imported result CSV or
literal outcome table; the code directly implements the historical V2
collection-plus-serial-allocation semantics that produce those values.

## Comparison to final CA-LIVE and generation order

Phase 4I validated its event model against `recam_dss_v2_early_top` and
`recam_dss_v2_group_top`, not CA-LIVE tops
(`docs_verilog/PHASE4I_RTL_TIMING_VALIDATION.md`).  Its 8 representative
V2 RTL cases validate the historical decision boundary but do not validate a
CA-LIVE timing contract.

The final N2 G2X2 RC CA-LIVE closure was committed later at `9bb9566`
(2026-09-28), followed by the seven-case freeze `a2a61b9` (2026-09-29).
Git ancestry proves `3304c86` precedes both.  CA-LIVE accepts the final live
state generation, evaluates four Configs for that state, then registers the
canonical group selection one edge later: fixed five cycles for GROUP.
Completed earlier-SA records are retained rather than replayed.

Therefore the mismatch is not merely a `G` offset.  Phase 4I starts from a
last changing fault and runs historical whole-group V2 work (including 16
candidate-collection cycles and serial A→D allocation).  CA-LIVE starts from
the accepted current/final state update and uses retained earlier-SA state.

```text
PHASE4I_RELATIVE_TO_CA_LIVE = BEFORE
PHASE4I_CLASSIFICATION = B
PHASE4I_AS_HISTORICAL_ARCHITECTURE_EVIDENCE = VALID
PHASE4I_AS_FINAL_CA_LIVE_EVIDENCE = INVALID
PHASE4I_SAFE_FOR_CURRENT_MAIN_PAPER = NO
PHASE4I_SAFE_FOR_BACKUP_ARCHITECTURE_EVOLUTION = WITH_QUALIFICATION
```

Safe backup use requires the labels **historical V2 EARLY** and **historical
V2 GROUP-NoScratch**, the decision-ready boundary, and the supplied
last-fault/BIST-gap event model.  It must not be presented as final CA-LIVE
post-BIST latency.  No historical artifact is changed or superseded here.
