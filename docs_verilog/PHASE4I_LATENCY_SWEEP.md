# Phase 4I-2 — Large-Scale Post-BIST DSS Decision-Latency Sweep

## Scope

This is a 10,000-vector paired C++ event-model study of post-BIST **DSS
decision-ready** latency for the frozen 2×2 Directional EARLY and
GROUP-NoScratch policies.  It measures neither final repair reconstruction,
Hybrid/pivot processing, eFuse programming, runtime remapping setup, SRAM,
nor a new topology.

The Phase 4I-1 cycle convention is unchanged:

```text
fault-set-changing event at rising edge N → concurrent analysis restarts
W-cycle decision                              → ready at rising edge N+W
ready at T_BIST_end                           → L_post = 0
L_post                                        = max(0, T_decision_ready - T_BIST_end)
```

## Corpus and paired-input contract

| Item | Value |
|---|---|
| Trace count | 10,000 |
| Seed | 20260910 |
| Fault generator | Existing `DynamicFaultGenerator` |
| Fault-count model | Existing `Uniform`, 20 faults/group |
| Spatial model | Existing `Mixed` |
| Geometry | 1024 rows × 1024 columns/SA |
| Topology / initialization | Frozen 2×2 Directional; A_ROW/D_ROW/B_COL/C_COL initially unreleased and unborrowed |
| Arrival order | Generator order A→B→C→D, then in-SA generated order |
| Arrival cadence | One fault-set-changing event per cycle |
| G sweep | Every integer `G = 0..32` (therefore includes 0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32) |

For a vector, the fault map and event stream are generated once.  Both
policies receive those exact inputs; only the frozen selection policy differs.
`T_last_fault` stays fixed while `T_BIST_end = T_last_fault + G` changes.

The existing `DirectionalMultiConfigAnalyzer` supplies candidate validity and
lowest PatternID.  The sweep adapter then applies the frozen V2 ranks,
directional donor priority, atomic commit, first-failure, and no-rollback
rules to form each policy's decision work plan.  EARLY work is its active-SA
candidate-evaluation sum.  GROUP-NoScratch work is fixed 16-cycle collection
plus its sequential allocation-evaluation sum.

## Aggregate results

`P95` uses nearest-rank percentile.  The full 33-point aggregate table is in
[`phase4i2_summary.csv`](../results/phase4i/phase4i2_summary.csv); selected
transition points are below.

| G | EARLY mean | EARLY P95 | EARLY zero % | GROUP mean | GROUP P95 | GROUP zero % |
| -: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 4.0095 | 4 | 0.00 | 20.3156 | 21 | 0.00 |
| 1 | 3.0095 | 3 | 0.00 | 19.3156 | 20 | 0.00 |
| 2 | 2.0095 | 2 | 0.00 | 18.3156 | 19 | 0.00 |
| 4 | 0.0095 | 0 | 99.49 | 16.3156 | 17 | 0.00 |
| 6 | 0.0013 | 0 | 99.87 | 14.3156 | 15 | 0.00 |
| 8 | 0.0000 | 0 | 100.00 | 12.3156 | 13 | 0.00 |
| 12 | 0.0000 | 0 | 100.00 | 8.3156 | 9 | 0.00 |
| 16 | 0.0000 | 0 | 100.00 | 4.3156 | 5 | 0.00 |
| 20 | 0.0000 | 0 | 100.00 | 0.3156 | 1 | 72.18 |
| 21 | 0.0000 | 0 | 100.00 | 0.0374 | 0 | 96.49 |
| 22 | 0.0000 | 0 | 100.00 | 0.0023 | 0 | 99.79 |
| 24 | 0.0000 | 0 | 100.00 | 0.0000 | 0 | 100.00 |
| 32 | 0.0000 | 0 | 100.00 | 0.0000 | 0 | 100.00 |

Mean `L_last_fault` is 4.0095 cycles for EARLY and 20.3156 cycles for GROUP.
The policy-specific repairable rates are 99.51% (EARLY) and 99.88% (GROUP).
The raw result contains failure position, four selected candidate-evaluation
depths, ConfigIDs, donor IDs, borrow count, and release count for diagnostics.

## Zero-latency slack requirement

| Policy | Mean G_zero | P50 | P95 | Max | N within sweep |
|---|---:|---:|---:|---:|---:|
| EARLY | 4.0095 | 4 | 4 | 7 | 10,000 |
| GROUP-NoScratch | 20.3156 | 20 | 21 | 24 | 10,000 |

`G_zero` is the first integer gap in the complete `0..32` sweep that produces
`L_post = 0` for that vector and policy.

## Paired EARLY versus GROUP interpretation

`Delta_Lpost = L_post_GROUP - L_post_EARLY`.  The full paired table is in
[`phase4i2_paired_delta.csv`](../results/phase4i/phase4i2_paired_delta.csv).

| G | Mean Delta | P50 | P95 | Max | EARLY < GROUP | Equal | EARLY > GROUP |
| -: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 16.3061 | 16 | 17 | 19 | 100.00% | 0.00% | 0.00% |
| 8 | 12.3156 | 12 | 13 | 16 | 100.00% | 0.00% | 0.00% |
| 16 | 4.3156 | 4 | 5 | 8 | 100.00% | 0.00% | 0.00% |
| 20 | 0.3156 | 0 | 1 | 4 | 27.82% | 72.18% | 0.00% |
| 21 | 0.0374 | 0 | 0 | 3 | 3.51% | 96.49% | 0.00% |
| 22 | 0.0023 | 0 | 0 | 2 | 0.21% | 99.79% | 0.00% |
| 24 | 0.0000 | 0 | 0 | 0 | 0.00% | 100.00% | 0.00% |

No `EARLY > GROUP` observation occurred.  This follows the tested work-plan
envelope (EARLY at most 16 active candidate evaluations; GROUP at least its
fixed 16-cycle collection plus allocation); it is a post-BIST latency result,
not a repairability-dominance claim.

## Repairability stratification

The complete repairable/unrepairable output is
[`phase4i2_stratified_summary.csv`](../results/phase4i/phase4i2_stratified_summary.csv).
At `G=0`, EARLY repairable/unrepairable samples were 9,951/49 with mean
`L_post` 4.0020/5.5306 cycles; GROUP samples were 9,988/12 with mean
20.3152/20.6667 cycles.  The rare unrepairable tails therefore have a larger
latency requirement and are retained instead of being folded into the
repairable aggregate.

## Artifacts, sanity checks, and reproduction

| Artifact | Content |
|---|---|
| [`phase4i2_raw_latency.csv`](../results/phase4i/phase4i2_raw_latency.csv) | 660,000 paired policy×vector×G observations plus diagnostics |
| [`phase4i2_summary.csv`](../results/phase4i/phase4i2_summary.csv) | Policy/G aggregate latency statistics |
| [`phase4i2_zero_thresholds.csv`](../results/phase4i/phase4i2_zero_thresholds.csv) | `G_zero` distributions |
| [`phase4i2_paired_delta.csv`](../results/phase4i/phase4i2_paired_delta.csv) | Paired EARLY–GROUP deltas |
| [`phase4i2_stratified_summary.csv`](../results/phase4i/phase4i2_stratified_summary.csv) | Repairability-stratified statistics |

```bash
make test_dss_post_bist_latency_sweep
```

```text
EARLY/GROUP same trace count        = PASS
EARLY/GROUP same vector IDs         = PASS
EARLY/GROUP same G points           = PASS
EARLY/GROUP same fault traces       = PASS
MONOTONICITY_VIOLATIONS             = 0
negative L_post count               = 0
invalid timestamp ordering          = 0
G=0 directed interpretation errors  = 0

PHASE_4I_2_LATENCY_SWEEP = PASS
READY_FOR_PHASE_4I_3 = YES

V2_PHASE = PHASE_4I
V2_NEXT_PHASE_AUTHORIZED = YES
```

Phase 4I-3 was not started.
