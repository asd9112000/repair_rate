# Phase 4I — Final Post-BIST DSS Decision-Latency Characterization

## Scope and frozen metric

This is the authoritative characterization for the frozen 2×2 Directional
DSS decision/resource-management boundary.  It combines the Phase 4I-2
10,000-trace paired C++ event-model corpus with Phase 4I-3 cycle-exact
production RTL validation.

```text
G      = T_BIST_end - T_last_fault
L_post = max(0, T_decision_ready - T_BIST_end)
```

`T_decision_ready` is the registered point where the DSS decision is final:
repairability and selected ConfigID/PatternID, donor/release action, and
resource ledger are available.  It is not final repair reconstruction,
Hybrid/pivot remapping, eFuse programming, or runtime remapping setup.

The frozen corpus is `TRACE_COUNT = 10000`, `SEED = 20260910`.  Fault maps,
arrival order, and BIST-end gap are paired between EARLY and GROUP-NoScratch;
only the frozen SolutionTakePolicy differs.

## Final post-BIST results

| G | EARLY mean / P95 / zero % | GROUP mean / P95 / zero % |
| -: | ---: | ---: |
| 0 | 4.0095 / 4 / 0.00% | 20.3156 / 21 / 0.00% |
| 4 | 0.0095 / 0 / 99.49% | 16.3156 / 17 / 0.00% |
| 8 | 0.0000 / 0 / 100.00% | 12.3156 / 13 / 0.00% |
| 16 | 0.0000 / 0 / 100.00% | 4.3156 / 5 / 0.00% |
| 20 | 0.0000 / 0 / 100.00% | 0.3156 / 1 / 72.18% |
| 21 | 0.0000 / 0 / 100.00% | 0.0374 / 0 / 96.49% |
| 22 | 0.0000 / 0 / 100.00% | 0.0023 / 0 / 99.79% |
| 24 | 0.0000 / 0 / 100.00% | 0.0000 / 0 / 100.00% |
| 32 | 0.0000 / 0 / 100.00% | 0.0000 / 0 / 100.00% |

The complete `G=0..32` plotting inputs are
[`phase4i4_latency_plot_data.csv`](../results/phase4i/phase4i4_latency_plot_data.csv)
for Figure A (`G` versus mean/P95 `L_post`) and
[`phase4i4_zero_latency_plot_data.csv`](../results/phase4i/phase4i4_zero_latency_plot_data.csv)
for Figure B (`G` versus zero-latency fraction).  No paper figure is generated
here.

## Zero-latency threshold

| Policy | Mean G_zero | P50 | P95 | Max |
|---|---:|---:|---:|---:|
| EARLY | 4.0095 | 4 | 4 | 7 |
| GROUP-NoScratch | 20.3156 | 20 | 21 | 24 |
| GROUP − EARLY, paired | 16.3061 | 16 | 17 | 19 |

Thus GROUP requires 16 additional median post-last-fault slack cycles to
fully hide DSS decision work, and 17 additional paired P95 cycles.  These are
cycle counts; they are not converted to time units in this record.

## Paired policy result

At `G=0`, `Delta_Lpost = L_post_GROUP - L_post_EARLY` has mean 16.3061,
P50 16, P95 17, and maximum 19 cycles.  Across all 33 swept gap points:

```text
PAIRED_OBSERVATIONS = 330000
EARLY_LESS_GROUP_OBSERVATIONS = 203156 (61.562424%)
EARLY_EQUAL_GROUP_OBSERVATIONS = 126844 (38.437576%)
EARLY_GREATER_GROUP_OBSERVATIONS = 0
```

This is a post-BIST decision-latency observation.  It does not imply GROUP
repairability dominance or a repair-result mismatch: GROUP's fixed 16-cycle
candidate collection and deferred allocation are the execution-structure
source of the added latency.

## Execution latency versus exposed post-BIST latency

These metrics remain distinct:

| Metric | Definition | Representative value |
|---|---|---|
| DSS execution latency | `start_i` accepted → `done_o` | EARLY 4 cycles; GROUP 20+ cycles |
| Exposed post-BIST latency | `T_BIST_end` → decision ready | Can be zero when `G` hides the remaining work |

Therefore EARLY does not have zero analysis latency.  Rather, it reaches zero
*additional post-BIST DSS decision latency* when sufficient post-last-fault
slack exists.  GROUP likewise reaches zero exposed latency at sufficient gap.

## Repairability stratification

Unrepairable cases are rare and have a longer threshold tail; GROUP retains
the same median threshold while EARLY's rare unrepairable subset shifts from
four to five cycles:

| Policy | Class | N | Mean G_zero | P50 | P95 | Max |
|---|---|---:|---:|---:|---:|---:|
| EARLY | repairable | 9,951 | 4.0020 | 4 | 4 | 6 |
| EARLY | unrepairable | 49 | 5.5306 | 5 | 7 | 7 |
| GROUP | repairable | 9,988 | 20.3152 | 20 | 21 | 23 |
| GROUP | unrepairable | 12 | 20.6667 | 20 | 24 | 24 |

The unrepairable subsets are 0.49% (EARLY) and 0.12% (GROUP).  They do not
change the median conclusion or justify a separate primary plot, but their
statistics are retained in
[`phase4i2_stratified_summary.csv`](../results/phase4i/phase4i2_stratified_summary.csv).

## RTL validation evidence

| Case | Policy | C++ T_ready | RTL T_ready | C++ / RTL L_post |
|---|---|---:|---:|---:|
| EARLY G=0 | EARLY | 24 | 24 | 4 / 4 |
| EARLY same-edge zero | EARLY | 24 | 24 | 0 / 0 |
| EARLY failure-B | EARLY | 25 | 25 | 5 / 5 |
| GROUP G=0 | GROUP | 40 | 40 | 20 / 20 |
| GROUP same-edge zero | GROUP | 40 | 40 | 0 / 0 |
| GROUP failure-B | GROUP | 41 | 41 | 21 / 21 |

```text
RTL_EVENT_MODEL_MISMATCHES = 0
POST_BIST_EVENT_MODEL_VALIDATED_BY_RTL = YES
```

The full eight-case comparison and cycle traces remain in
[`PHASE4I_RTL_TIMING_VALIDATION.md`](PHASE4I_RTL_TIMING_VALIDATION.md) and
[`phase4i4_rtl_validation_summary.csv`](../results/phase4i/phase4i4_rtl_validation_summary.csv).

## Sanity evidence and conclusions

```text
MONOTONICITY_VIOLATIONS = 0
NEGATIVE_L_POST_COUNT = 0
INVALID_TIMESTAMP_ORDERING = 0
G0_DIRECTED_INTERPRETATION_MISMATCHES = 0
```

1. EARLY exposes substantially less post-BIST DSS decision latency.
2. Most EARLY traces need about four post-last-fault slack cycles to hide DSS
   decision work completely.
3. GROUP generally needs 20–21 cycles because it first collects all 16
   canonical candidates, then allocates sequentially.
4. With sufficient slack, both policies incur zero additional post-BIST DSS
   decision latency.
5. The latency difference is due to SolutionTakePolicy execution structure,
   not a functional repair-result mismatch.
6. The large-scale timing model was validated cycle-exactly against the
   production RTL decision boundary.

## Artifact provenance and phase state

The final plot and threshold CSVs are derived without a new corpus from:

- [`phase4i2_summary.csv`](../results/phase4i/phase4i2_summary.csv)
- [`phase4i2_zero_thresholds.csv`](../results/phase4i/phase4i2_zero_thresholds.csv)
- [`phase4i2_paired_delta.csv`](../results/phase4i/phase4i2_paired_delta.csv)
- [`phase4i3_rtl_timing.csv`](../results/phase4i/phase4i3_rtl_timing.csv)

The Phase 4I-4 deliverables are:

- [`phase4i4_latency_plot_data.csv`](../results/phase4i/phase4i4_latency_plot_data.csv)
- [`phase4i4_zero_latency_plot_data.csv`](../results/phase4i/phase4i4_zero_latency_plot_data.csv)
- [`phase4i4_threshold_summary.csv`](../results/phase4i/phase4i4_threshold_summary.csv)
- [`phase4i4_rtl_validation_summary.csv`](../results/phase4i/phase4i4_rtl_validation_summary.csv)

```text
PHASE_4I_4_FINAL_CHARACTERIZATION = PASS
PHASE_4I = COMPLETE
POST_BIST_EVENT_MODEL_VALIDATED_BY_RTL = YES
TRACE_COUNT = 10000
SEED = 20260910

V2_PHASE = PHASE_4I_COMPLETE
V2_NEXT_PHASE_AUTHORIZED = NO
```

No new architecture phase is started by this characterization.
