# Final CA-LIVE state-relative post-BIST latency

## 1. Why raw-fault-relative measurement was blocked

The final CA-LIVE policy RTL has no raw-fault collector or `BIST_end` input.
The prior [raw-fault characterization](CA_LIVE_POST_BIST_LATENCY_CHARACTERIZATION.md)
correctly stopped at L0.  No collector latency, including a one-cycle adapter,
is assumed here.

## 2. Final CA-LIVE policy boundary

This evidence concerns final N2 G2X2 Row+Column sharing (`RS=2`, `CS=2`,
`m=1`) and starts when the final/current state becomes visible to the
CA-LIVE policy engine.  Each of 10,000 matched candidate-state images drives
the frozen EARLY and GROUP policy cores and their canonical policy oracles.

## 3. Definition of `G_state`

`G_state = T_BIST_end - T_state_update`, where `T_BIST_end` is a synthetic
observation point, `T_state_update + G_state`.  It is not an RTL BIST signal.

For the GROUP row, `T_state_update` is the final-SA accepted update.  For the
EARLY row, it is the accepted update that initiates the recorded group decision
(the final repaired SA or the first unrepaired SA).  These are policy-local
decision origins; the paired corpus has identical four-SA input images.

## 4. Definition of `L_post_state`

`L_policy` is measured from observed timestamps:
`T_decision_ready - T_state_update`.  For every raw row and every integer
`G_state` from 0 to 8, the independent sweep verified
`L_post_state = max(0, T_decision_ready - T_BIST_end) = max(0, L_policy - G_state)`.

## 5. Functional validation

`FUNCTIONAL_MISMATCHES: 0`.  EARLY checks repairability, committed-SA mask,
selected config, and PatternID against its canonical oracle.  GROUP checks
repairability, config, PatternID, donor, borrow, and release against its
canonical oracle.  The raw recomputer also verifies that both rows for every
case use the same state-image fingerprint.

## 6. EARLY policy latency

All 10,000 group-decision transactions: `min/mean/median/p95/max =
1 / 1.433400 / 1 / 4 / 4` cycles.

Selected legal group decisions (`N=9,499`): `1 / 1.298031 / 1 / 3 / 4`.
Their observed priority-rank counts are rank 1--4 = `7247 / 1743 / 439 / 70`;
the rank-to-latency mapping is `1→1`, `2→2`, `3→3`, `4→4` cycles.

Unrepairable/full-search decisions (`N=501`) are all 4 cycles.

## 7. GROUP policy latency

All 10,000 final-SA transactions measure `5 / 5 / 5 / 5 / 5` cycles.
`GROUP_FIXED_5: PASS`.

## 8. `G_state` sweep

At `G_state=0`, EARLY has mean/median/P95/max exposed latency
`1.4334 / 1 / 4 / 4` and GROUP has `5 / 5 / 5 / 5`; neither has a zero-latency
case.  EARLY zero-exposure fractions at gaps 1--4 are
`0.7247, 0.8990, 0.9429, 1.0000`; GROUP remains zero until gap 5, where it is
`1.0000`.  The full single-cycle 0--8 table is
[`CA_LIVE_STATE_RELATIVE_SWEEP.csv`](../../../results/date2026/ca_live_state_relative_post_bist_latency/summary/CA_LIVE_STATE_RELATIVE_SWEEP.csv).

## 9. `G_state_zero`

EARLY `G_state_zero`: `min/mean/median/p95/max = 1 / 1.433400 / 1 / 4 / 4`.
GROUP `G_state_zero`: `5 / 5 / 5 / 5 / 5`.
The measured minimum gap yielding zero exposure equals each row's measured
policy latency: `G_STATE_ZERO_EQUALS_POLICY_LATENCY: PASS`.

## 10. EARLY/GROUP paired comparison

`GROUP - EARLY G_state_zero` is `min/mean/median/p95/max =
1 / 3.566600 / 4 / 4 / 4` cycles.  EARLY has lower exposure in every pair for
gaps 0--4; all pairs are equal at gaps 5--8.  This is a latency comparison,
not a repairability-dominance claim.

## 11. Relation to historical Phase4I

Phase4I (`3304c86`) remains `HISTORICAL_REFERENCE_ONLY`: it uses the V2
Specialized-EARLY/GROUP-NoScratch raw-fault-origin model.  It is not numerically
pooled or directly compared with this final CA-LIVE policy-boundary result.

## 12. Limitation: collector not modeled

The measurement begins when the final fault state becomes visible to the
CA-LIVE policy engine; raw fault collection latency is outside the evaluated
RTL boundary.

```text
RAW_FAULT_TO_STATE_UPDATE_LATENCY: NOT_MEASURED
RAW_FAULT_RELATIVE_POST_BIST_LATENCY: NOT_AVAILABLE_FROM_FINAL_POLICY_RTL
```

## 13. Paper/PPT-safe wording

Use **State-relative Post-BIST exposed latency**, **Policy-visible Post-BIST
exposed latency**, or, after definition, **Post-BIST policy exposure**.
When `L_post_state=0`, say **zero additional post-BIST policy exposure**;
do not call it zero repair, analysis, or end-to-end BIST latency.

## Evidence

- [Raw paired base CSV](../../../results/date2026/ca_live_state_relative_post_bist_latency/raw/CA_LIVE_STATE_RELATIVE_BASE.csv)
- [Sweep summary](../../../results/date2026/ca_live_state_relative_post_bist_latency/summary/CA_LIVE_STATE_RELATIVE_SWEEP.csv)
- [G-zero summary](../../../results/date2026/ca_live_state_relative_post_bist_latency/summary/CA_LIVE_STATE_RELATIVE_G_ZERO.csv)
- [Paired summary](../../../results/date2026/ca_live_state_relative_post_bist_latency/summary/CA_LIVE_STATE_RELATIVE_PAIRED.csv)
- [Run manifest](../../../results/date2026/ca_live_state_relative_post_bist_latency/provenance/RUN_MANIFEST.md)
