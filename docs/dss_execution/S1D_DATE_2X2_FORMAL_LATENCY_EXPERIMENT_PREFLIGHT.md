# S1D / E0-L — DATE 2x2 DSS Formal Latency Experiment Preflight

## Scope and result

`S1D_STATUS: COMPLETE`

S1D freezes the measurement contract for a future, paired DATE latency
experiment on the 2x2 directional-CAM configuration (`RS=2`, `CS=2`,
`SHARE_M=1`).  It covers `EARLY` and `GROUP_NO_SCRATCH_V2` only.  This is not
a formal latency experiment, repair-rate sweep, synthesis rerun, device-timing
study, production-RTL change, or repair-policy change.

The preflight adds only a C++ test-side CSV serializer/checker and this
contract document.  The generated CSV is written below `build/tests/`; it is
intentionally a transient validation artifact, not a results dataset.

## Frozen terminology and timebase

The formal names are:

- **DSS Decision Latency**
- **Group Post-BIST Latency** (`RAW` or `ARCHITECTURE_NORMALIZED`)
- **Group Fault-Tail Latency** (`RAW` or `ARCHITECTURE_NORMALIZED`)

Every CSV timestamp is an integer transaction-cycle index.  Cycle zero is the
start of the serial A -> B -> C -> D BIST transaction.  Absolute RTL simulator
time is not emitted in this CSV.

The S1B-1 events remain frozen:

| Policy | DSS decision start | Per-SA diagnostic edge | Group decision ready |
| --- | --- | --- | --- |
| `EARLY` | accepted `start_i && !busy_o` | `sa_commit_valid_o[i]` assertion | `done_o` assertion |
| `GROUP_NO_SCRATCH_V2` | accepted `start_i` while `state_q == IDLE` | `sa_commit_valid_o[i]` assertion, **internal finalization** only | `done_o` assertion |

`done_o` means the DSS group decision is terminal.  It is not called a
repair-table-ready event.  Group per-SA fields are diagnostic sequential
internal-finalization edges, not externally available per-SA results.

## Clock-domain interpretation

`BIST_VS_DSS_CLOCK_RELATION: ABSTRACT_COMMON_CYCLE`

`SerialBistSchedule` supplies an integer `cyclesPerWord` schedule and has no
physical period or explicit relationship to the V2 DSS clock.  The S1C
correlator places a start event on the next shared *transaction index*, but
does not establish a physical shared clock.  Therefore:

```text
PHYSICAL_NS_CONVERSION_ALLOWED: PARTIAL
```

The primary formal output is cycles only.  A later appendix may show the
policy-local, clearly qualified sensitivity calculation
`DSS Decision Latency cycles x mapped critical delay` (EARLY 19.70 ns,
GROUP 19.74 ns).  It is not a system physical latency and it must not be used
to convert Group Post-BIST or Group Fault-Tail cycles into ns.  No ns sum that
includes serial-BIST cycles is authorized by this contract.

## Frozen metric contract

All subtraction uses terminal `GROUP_DECISION_READY` (`done_o`) and only rows
that reach it are emitted.

| Field | Definition | Eligibility | Units |
| --- | --- | --- | --- |
| `dss_decision_cycles` | `group_done_cycle - dss_start_cycle` | every terminal transaction | cycles |
| `group_post_bist_raw` | `group_done_cycle - group_test_done_cycle` | every terminal transaction, including zero-fault and failures | cycles |
| `group_post_bist_arch` | `group_post_bist_raw - handoff_gap_cycles` | same as Post-BIST RAW | cycles |
| `group_fault_tail_raw` | `group_done_cycle - group_last_fault_accept_cycle` | terminal positive-fault transactions only | cycles |
| `group_fault_tail_arch` | `group_fault_tail_raw - handoff_gap_cycles` | same as Fault-Tail RAW | cycles |

The primary architectural metric is `dss_decision_cycles`.  S1B-1's directed
all-local reference remains EARLY A/B/C/D/group = `1/2/3/4/4` and GROUP
internal-finalize A/B/C/D plus group = `18/20/22/24/24`, relative to accepted
start at cycle zero.

S1C established the current correlation relation:

```text
dss_start_cycle = group_test_done_cycle + 1
handoff_gap_cycles = 1
handoff_gap_class = HARNESS_ARTIFACT
```

The one-cycle handoff is retained in RAW columns for trace reproducibility and
subtracted only in the `*_arch` columns.  It is not architectural overhead.

For each positive-fault row the following must hold:

```text
group_fault_tail_raw
  = bist_tail_cycles + handoff_gap_cycles + dss_decision_cycles

group_fault_tail_arch
  = bist_tail_cycles + dss_decision_cycles
```

For every terminal row:

```text
group_post_bist_raw
  = group_done_cycle - group_test_done_cycle

group_post_bist_arch
  = group_post_bist_raw - handoff_gap_cycles
```

## Zero-fault and failure rules

For a zero-fault group, `group_last_fault_accept_cycle`, `bist_tail_cycles`,
`group_fault_tail_raw`, and `group_fault_tail_arch` are literal `N/A`.  They
are never set to zero or inferred from test start/test done.  Zero-fault rows
remain eligible for DSS Decision and Group Post-BIST metrics, whether the
terminal decision is repairable or failed.

Every terminal row records `repairable` and `failure_position` (`A`, `B`,
`C`, `D`, or `N/A`).  Each metric is summarized in three explicitly labelled
views where eligible: `ALL_TERMINAL`, `SUCCESS_ONLY`, and `FAILURE_ONLY`.
The report must give total, success, failure, positive-fault, and zero-fault
counts, and the A/B/C/D failure-position distribution.

The primary policy-completion and DSS comparisons use `ALL_TERMINAL`: both
policies' terminal `done_o` edges are meaningful decision endpoints, including
first-failure termination.  A `BOTH_PASS` subset is also mandatory as the
repair-success sensitivity view.  `SUCCESS_ONLY` per-policy and
`FAILURE_ONLY` per-policy views are reported separately, never merged without
their labels.

The paired outcome class is one of:

```text
BOTH_PASS  = EARLY success, GROUP success
EARLY_ONLY = EARLY success, GROUP failure
GROUP_ONLY = EARLY failure, GROUP success
BOTH_FAIL  = EARLY failure, GROUP failure
```

Pairing is by `transaction_id`, not by row order.  For all terminal paired
rows, report `GROUP - EARLY` deltas for DSS Decision and Group Post-BIST
architecture-normalized cycles.  Fault-tail paired deltas are additionally
eligible only if that shared fault group is positive-fault.  Report mean,
median, and p95 paired deltas; include `BOTH_PASS` deltas separately.

## Formal corpus contract (not run in S1D)

The old `dynamic_sharing_seed_20260820` fixture is a frozen repairability
regression corpus, so it is not silently reused as a latency corpus.  The
future formal corpus is instead frozen as the independently identifiable
recipe below.  S1D does **not** materialize or execute its 10,000 groups.

```text
corpus_id: E0L_DATE_2X2_LATENCY_V1
generator: DynamicFaultGenerator
seed: 20260914
target_group_count: 10000
layout: Grid2x2
RS / CS / SHARE_M: 2 / 2 / 1
memory geometry: 512 rows x 8192 columns, 256-bit BIST word
fault_count_model: Uniform
fault_count_group: 28 fixed
lambda_sa: 7 fixed faults per SA (not a Poisson draw)
fault_spatial_model: Mixed
prob_cluster: 0.20
prob_same_line: 0.30 effective unconditional probability
same_line_threshold: 0.50 (source-model threshold; includes the 0.20 cluster branch)
D0: NOT_APPLICABLE
serial BIST: A -> B -> C -> D, cycles_per_word=1
fault_timeline_source: MODEL_DERIVED_FAULT_TIMELINE
```

`DynamicFaultGenerator` deterministically derives each group from the seed and
its zero-based run index.  The future experiment must persist the generated
fault corpus and record its SHA-256 before either policy runs.  Until that
authorized generation occurs, `E0L_DATE_2X2_LATENCY_V1` is the deterministic
identifier, not a claimed corpus-content hash.

For each run index, EARLY and GROUP must consume the same owned `FaultGroup`,
same A/B/C/D assignments, same serial-BIST schedule, same model-derived replay
cycles, and same `transaction_id`; only the policy changes.

## BIST provenance and scope boundary

`SA_TEST_DONE` is the completed final-word boundary returned by
`SerialBistSchedule::subarrayCompletionCycle()`.  `GROUP_TEST_DONE` is
`schedule.completionCycle()`.  The current schedule has no distinct
issue/complete pipeline model.

Fault timing is explicitly `MODEL_DERIVED_FAULT_TIMELINE`: a `FaultGroup`
replay element is assigned `SerialBistSchedule::arrivalCycle()`.  It is not a
hardware-observed valid/ready handshake and must never be described as one.

SA-level Post-BIST and Fault-Tail metrics remain `PARTIAL`; they are not DATE
headline metrics.  No cross-group scheduler, device completion, global-pool,
Tier-2 CAM, device timing, or repair-rate metric is in scope.

## CSV contract

The formal CSV has one row per policy per transaction and this fixed header:

```text
experiment_id,transaction_id,seed,policy,RS,CS,SHARE_M,
lambda_sa,prob_cluster,prob_same_line,D0,
fault_count_group,fault_count_A,fault_count_B,fault_count_C,fault_count_D,
group_has_accepted_fault,group_last_fault_accept_cycle,
group_test_done_cycle,dss_start_cycle,group_done_cycle,
bist_tail_cycles,handoff_gap_cycles,dss_decision_cycles,
group_post_bist_raw,group_post_bist_arch,
group_fault_tail_raw,group_fault_tail_arch,
repairable,failure_position,fault_timeline_source,handoff_gap_class,
A_commit_or_internal_finalize_cycle,B_commit_or_internal_finalize_cycle,
C_commit_or_internal_finalize_cycle,D_commit_or_internal_finalize_cycle
```

`N/A` is the only null encoding.  For GROUP, the four optional final columns
are internal-finalize cycles; for EARLY they are commit cycles.  Future formal
runs populate the fixed corpus metadata on every row.  The S1D preflight-only
rows deliberately use `N/A` for generator parameters because they are directed
serializer fixtures, not a materialized formal corpus.

For each policy and each of the three views, report count, mean, median, min,
max, and p95 for `dss_decision_cycles`, `group_post_bist_arch`, and (positive
fault only) `group_fault_tail_arch`.  Use nearest-rank p95
`ceil(0.95 * N)` after ascending sort; emit `N/A` when `N=0`.  Standard
deviation and p99 are optional and only meaningful after the formal corpus is
run.

## Frozen synthesis context

No synthesis was rerun.  The frozen 2x2 mapped record in
`results/dss_v2_rs3cs3m1/h5_hardware_retry2/hardware_characterization.csv`
is retained as a separate synthesis comparison:

| Policy | Total area | GE | Critical delay | WNS |
| --- | ---: | ---: | ---: | ---: |
| EARLY | 80871.437539 | 8104.00 | 19.70 ns | 0.01 ns |
| GROUP_NO_SCRATCH_V2 | 105393.658552 | 10561.33 | 19.74 ns | 0.00 ns |

Publication recommendation, without changing paper text:

- Main table: GE, critical delay, and all-terminal DSS Decision Latency.
- Main figure: paired DSS Decision and Group Post-BIST architecture-normalized
  distributions, with terminal outcome counts shown alongside.
- Supplement/appendix: raw values, handoff field, Fault-Tail
  architecture-normalized distribution, positive/zero-fault exclusions,
  `BOTH_PASS` sensitivity, failure-position distribution, per-SA diagnostic
  edges, and any mapped-delay-scaled DSS-only sensitivity calculation.

## Preflight-only CSV sanity check

`tests/dss_formal_latency_preflight_test.cpp` uses ten deterministic paired,
directed serializer fixtures sourced from frozen S1B-1 edge shapes and the S1C
correlator.  It does not recompute candidates from fault coordinates and is not
a repairability or latency population experiment.

Command:

```text
make test_dss_formal_latency_preflight
```

Observed output:

```text
S1D_PREFLIGHT rows=20 pairs=10 positive_fault_pairs=9 zero_fault_pairs=1
both_pass_pairs=6 both_fail_pairs=4 csv_schema=PASS na_handling=PASS
paired_alignment=PASS metric_identities=PASS summary_statistics=PASS
dss_formal_latency_preflight_test PASS
```

The output is `build/tests/s1d_date_2x2_preflight_only.csv`, marked in every
row as `S1D_PREFLIGHT_ONLY_NOT_FOR_PUBLICATION`.  It verifies header and field
population, zero-fault `N/A`, paired IDs/replay linkage, aligned success/failure
classification, and the Post-BIST/Fault-Tail identities.  It reports no
research statistic and does not materialize the frozen 10,000-group corpus.

## Required status report

```text
S1D_STATUS:
COMPLETE

SOURCE_FILES_MODIFIED:
Makefile
tests/dss_formal_latency_preflight_test.cpp
docs/dss_execution/S1D_DATE_2X2_FORMAL_LATENCY_EXPERIMENT_PREFLIGHT.md

PRODUCTION_RTL_CHANGED:
NO

REPAIR_SEMANTICS_CHANGED:
NO

FORMAL_METRIC_NAMES:
DSS Decision Latency
Group Post-BIST Latency (RAW / ARCHITECTURE_NORMALIZED)
Group Fault-Tail Latency (RAW / ARCHITECTURE_NORMALIZED)

--------------------------------
CLOCK / TIMEBASE
--------------------------------

TIMEBASE:
Integer transaction-cycle index; cycle 0 is serial 2x2 BIST start.

BIST_VS_DSS_CLOCK_RELATION:
ABSTRACT_COMMON_CYCLE

PHYSICAL_NS_CONVERSION_ALLOWED:
PARTIAL

--------------------------------
METRIC CONTRACT
--------------------------------

DSS_DECISION_LATENCY:
definition: group_done_cycle - dss_start_cycle
eligibility: every terminal transaction
units: cycles

GROUP_POST_BIST_RAW:
definition: group_done_cycle - group_test_done_cycle

GROUP_POST_BIST_ARCH:
definition: group_post_bist_raw - handoff_gap_cycles

GROUP_FAULT_TAIL_RAW:
definition: group_done_cycle - group_last_fault_accept_cycle

GROUP_FAULT_TAIL_ARCH:
definition: group_fault_tail_raw - handoff_gap_cycles

--------------------------------
ZERO-FAULT RULE
--------------------------------

POST_BIST:
Eligible for all terminal zero-fault and positive-fault rows.

FAULT_TAIL:
N/A; excluded from Fault-Tail aggregates and counted.

--------------------------------
PREFLIGHT SANITY RUN
--------------------------------

ROWS:
20 (10 paired groups)

OUTPUT:
build/tests/s1d_date_2x2_preflight_only.csv

CSV_SCHEMA:
PASS

N/A_HANDLING:
PASS

PAIRED_ALIGNMENT:
PASS

IDENTITY_CHECKS:
PASS

REGRESSION:
PASS

--------------------------------
METRIC READINESS
--------------------------------

SA_POST_BIST:
PARTIAL

GROUP_POST_BIST:
READY

SA_FAULT_TAIL:
PARTIAL

GROUP_FAULT_TAIL:
READY for positive-fault traces

FORMAL_LATENCY_EXPERIMENT_ELIGIBLE:
PARTIAL

FORMAL_LATENCY_EXPERIMENT_AUTHORIZED:
NO

REPAIR_RATE_EXPERIMENT_AUTHORIZED:
NO

DEVICE_LATENCY_AUTHORIZED:
NO

NEXT_PHASE_AUTHORIZED:
NONE
```
