# Run manifest

## Experiment identity

```text
PHASE: DATE2026-CA-LIVE-STATE-RELATIVE-POST-BIST-LATENCY
ARCHITECTURE: final N2 / G2X2 / RS=2 / CS=2 / m=1 / Row+Column sharing
CORPUS: 10000 matched four-SA candidate-state images
SEED: 20260910
RAW_FAULT_TO_STATE_UPDATE_LATENCY: NOT_MEASURED
RAW_FAULT_RELATIVE_POST_BIST_LATENCY: NOT_AVAILABLE_FROM_FINAL_POLICY_RTL
```

## Timing convention

For every group-policy row, `T_state_update` is the cycle of the accepted
update that initiates the recorded group decision: the decisive active-SA
update for EARLY, and the final-SA update for GROUP.  `T_decision_ready` is
the registered `done_o` (EARLY) or `solution_ready_o` (GROUP) cycle.

For each integer `G_state` in 0--8, the synthetic observation is
`T_BIST_end = T_state_update + G_state`; the raw CSV-derived check verifies
`max(0, T_decision_ready - T_BIST_end) == max(0, L_policy - G_state)`.

## Executed gates

| Gate | Command | Result |
| --- | --- | --- |
| L1 smoke | `make test_ca_live_state_relative CA_STATE_RELATIVE_CASES=100 CA_STATE_RELATIVE_SEED=20260910 CA_STATE_RELATIVE_ROOT=results/date2026/ca_live_state_relative_post_bist_latency/smoke` | PASS; mismatches 0 |
| L1 recompute | `make recompute_ca_live_state_relative CA_STATE_RELATIVE_ROOT=results/date2026/ca_live_state_relative_post_bist_latency/smoke` | PASS; 200 rows / 100 pairs |
| L2 formal | `make test_ca_live_state_relative CA_STATE_RELATIVE_CASES=10000 CA_STATE_RELATIVE_SEED=20260910 CA_STATE_RELATIVE_ROOT=results/date2026/ca_live_state_relative_post_bist_latency` | PASS; mismatches 0 |
| L3 independent recompute | `make recompute_ca_live_state_relative CA_STATE_RELATIVE_ROOT=results/date2026/ca_live_state_relative_post_bist_latency` | PASS; 20,000 rows / 10,000 pairs |

The RTL compile emitted existing `PINCONNECTEMPTY` warnings in the reused
lockstep wrappers.  They are unconnected optional-observation pins, did not
prevent compilation, and no wrapper or RTL semantics were changed.

## Gate results

```text
EARLY_GROUP_STATE_CORPUS_MATCH: PASS
FUNCTIONAL_MISMATCHES: 0
EARLY_RANK_LATENCY_MAPPING: PASS
GROUP_FIXED_5: PASS
STATE_RELATIVE_FORMULA_MATCH: PASS
G_STATE_ZERO_EQUALS_POLICY_LATENCY: PASS
SUMMARY_RECOMPUTE_MATCH: PASS
RTL_CHANGED: NO
CA_LIVE_SEMANTICS_CHANGED: NO
PHASE4I_CHANGED: NO
```
