# R3 final analysis status

| Class | Contents | Status |
| --- | --- | --- |
| FORMAL_COMPLETE | RS=CS=3, m=1, F8/F12/F16/F20/F24/F28/F32, nine policies, 100000 groups each | 63/63 results complete |
| SUPPORTING_SIDECAR | frozen F24 fault-allocation-variance quintile analysis | derived only from formal raw sidecars; labeled supporting |
| NON_FROZEN_SIDECAR | N2 F8--F32; N3 F18/F30 | 81 policy-point files; excluded from primary aggregates |
| DEV_ONLY / STALE | prior root plots and partial/running report | retained, not paper-final |
| DEFERRED | larger-m gain fractions, 4x1 comparison, old runtime benchmark figures | exact frozen provenance unavailable |

## Closure gates

```text
FROZEN_FORMAL_SCOPE_COMPLETE: YES
FAULT_CORPUS_CHANGED_BY_POLICY: NO
R3_FORMAL_SCOPE_MATCH: PASS
R3_SAMPLE_COUNTS_MATCH: PASS
R3_POLICY_LABELS_MATCH: PASS
R3_REPAIR_RATES_MATCH: PASS
R3_DELTA_PP_MATCH: PASS
R3_PLOTS_MATCH_TABLES: PASS
NON_FROZEN_POINTS_EXCLUDED_FROM_PRIMARY_AGGREGATE: YES
NO_TITLE: PASS
LEGEND_PRESENT: PASS
PAPER_STYLE: PASS
FROZEN_SCOPE_ONLY_OR_EXPLICITLY_LABELED: PASS
NEW_REPAIR_RATE_SIMULATION: NO
```

The final output root is `results/date2026/r3_final/`. The external 14G raw
sidecar root remains authoritative and in place.
