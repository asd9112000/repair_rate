# R2 — Group-Level Repair-Rate Sweep Preflight

> Status: COMPLETE. This is non-formal group-scope evidence only. No RTL was
> changed, no synthesis ran, and no device-level experiment was started.

## 1. Scope

R2 exercises `DynamicSpareSharing` only. Group results remain independent
four-SA Tier-0/Tier-1 analyses and do not represent device-wide CAM occupancy.

## 2. Policy matrix

`scripts/group/r2_repair_rate_preflight.py` materializes one physical corpus
per `(RS=CS, F_GROUP)` point and replays it through local/no-sharing,
directional EARLY m=1/2, pairwise-row EARLY m=1/2, directional canonical
GROUP-GREEDY and GROUP-GLOBAL m=1, Two-Pairwise EARLY/Pair-Global m=1/2, and
Single-Hop EARLY/GLOBAL m=1/2. Policy identifiers are never merged across
topologies.

## 3. Parameter points

The manifest is bounded to the R1-verified values: `RS=CS=2,3` and
`share_row=0,1,2` subject to `share_row <= RS`. Diagnostic fault points are
`{8,12,16,20}` for N=2 and `{12,18,24,30}` for N=3, corresponding to
`4N, 6N, 8N, 10N`.

## 4. Fault-generation contract

R2 adds `multinomial_uniform`: sequential categorical assignment is exactly
`Multinomial(F_GROUP; 0.25, 0.25, 0.25, 0.25)`, and each assignment increments
one SA count. The total is therefore exact by construction. The focused
generator regression checks both exact total and fixed-seed determinism.

## 5. Same-corpus validation

The runner first generates the corpus once, writes a seven-field simplified
fault replay file, and passes that file to every policy invocation. It checks
one `corpus_id` across all replayed policies and validates every A/B/C/D total
against `F_GROUP`. Its raw schema includes the four counts and derived mean,
population standard deviation, minimum, maximum, and range.

## 6. Spare-budget validation

Every raw record carries private/shareable rows and group row/column/physical
totals from `paired_policy_results_v1.csv`. The runner marks a record only when
`total_physical_group == 4 * (RS + CS)`; mismatches are explicitly
`RESOURCE_UNMATCHED`.

## 7. Output schema

The additive R2 schema is `dss_r2_group_preflight_v1`. It writes raw group
records, aggregate summaries, paired outcome rows, imbalance plot data,
commands, logs, and a JSON manifest. The paired sidecar now labels generic 2x2
CAM capacity/occupancy fields `GENERIC_CPP_CAPACITY`; 1x4 fields are emitted as
`NA` with `NOT_PROVEN`, rather than inventing a topology-specific CAM formula.

## 8. Repair-rate preflight results

The repaired matrix completed 120 policy/configuration aggregates and 120,000
raw policy-group records: 1,000 groups for every R2 point. Results remain
non-formal and must not enter the DATE evidence index.

## 9. GREEDY vs GLOBAL paired outcomes

The runner has a paired `EARLY_PASS`, `GROUP_GREEDY_PASS`, `GROUP_GLOBAL_PASS`,
and discordance schema, but no complete aggregate is available while the
Single-Hop GLOBAL runtime gate remains unresolved.

## 10. Two-Pairwise vs Single-Hop outcomes

The runner has schema fields for Pair-Global-vs-EARLY, Single-Hop-GLOBAL-vs-
EARLY, and Two-Pairwise-vs-Single-Hop outcomes. R1B directed/oracle evidence
remains valid; no R2 statistical comparison is claimed.

## 11. Fault-imbalance observations

`fault_stddev, policy, repairable` output is implemented. No correlation or
preflight trend is claimed without a completed common sample set.

## 12. CAM-accounting status

2x2 records report generic C++ provisioned/observed fields, explicitly not
RTL-calibrated. 1x4 capacity and runtime occupancy remain `NOT_PROVEN`; all
such fields are `NA`. This does not block functional R1B semantics but blocks
a CAM-complete comparison claim.

## 13. Runtime

The misplaced 2x2 generic dominance assertion initially rejected a legal
`1x4 / neighbor / Single-Hop EARLY` sample. It was limited to its intended
2x2 scope and verified on the same 1,000-group replay (957 successful groups,
16 seconds wall time). A second correction avoids calculating generic full
four-SA compressed search for Two-Pairwise and Single-Hop EARLY policies when
that result cannot affect the selected policy. The required Single-Hop GLOBAL
full tuple search still exceeded the compact-preflight runtime window at
1,000 groups, so no complete R2 table is available.

## 14. Recommended formal sweep range

The completed transition diagnostics support the R3 candidate ranges N=2:
`8,12,16,20`; N=3: `12,18,24,30`. R3 must be separately authorized.

## 15. Remaining blockers

The prior runtime blocker is closed by R2F; its oracle-preserving reduction and
characterization are recorded separately. Historical interrupted outputs remain
retained for diagnosis:

- `results/date2026/repair_rate/r2_preflight_interrupted_pre_fix/`
- `results/date2026/repair_rate/r2_preflight_interrupted_runtime_fix/`
- `results/date2026/repair_rate/r2_preflight_interrupted_global_runtime/`

## 16. Formal R2/R3 recommendation

R2 is complete. Do not automatically launch R3; its formal group sweep needs
separate authorization.
