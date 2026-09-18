# S1C — DATE 2x2 DSS Timeline Correlation

## Scope

`S1C_STATUS: COMPLETE`

S1C adds a reporting-only C++ correlation trace for one 2x2 directional-CAM
transaction (`RS=2`, `CS=2`, `SHARE_M=1`).  It links the S1B-2 serial-BIST
timeline with the verified S1B-1 controller-relative decision edges.  No
production RTL, interface, BIST algorithm, candidate selection, policy,
ledger, repair semantics, device scheduler, or repair-rate experiment changed.

Modified files:

- `Makefile`
- `inc/DssTimelineCorrelation.hpp`
- `src/DssTimelineCorrelation.cpp`
- `tests/dss_timeline_correlation_test.cpp`
- this document

## Unified timebase and same-corpus linkage

The sole timebase is an integer transaction cycle:

```text
cycle 0 = start of the serial A->B->C->D BIST transaction
```

`DssTimelineCorrelationRequest` stores transaction ID, policy, fault corpus
ID, optional seed, one owned `FaultGroup`, per-SA fault counts, candidate
contract ID, ConfigID/PatternID contract, BIST trace, DSS edges, repairability,
and optional failure position.  The utility rejects a trace unless its four
stored counts match that exact `FaultGroup`.

The frozen DSS cores receive analyzer results rather than raw physical faults.
`candidate_contract_id` therefore records the S1B-1 candidate
availability/ConfigID/PatternID contract for the same directed corpus; S1C
does not claim to derive candidates again from fault coordinates.  This is
`SAME_CORPUS_LINKAGE: PASS` for the auditable reporting contract.

## BIST-to-DSS handoff

The old BIST and core harnesses were separate.  The S1C wrapper presents
unmodified `start_i` on the next transaction edge after `GROUP_TEST_DONE`:

```text
DSS_DECISION_START = GROUP_TEST_DONE + 1
```

Thus the observed correlation relation is `NEXT_CYCLE`, with gap one.  This
one-cycle offset is explicitly `HARNESS_ARTIFACT`, not an inferred
architectural wait state and not a newly inserted RTL state.

## Correlated traces

The late-D directed corpus has A zero faults, B one fault, C two faults, and D
one fault at row 511/word-column 31.  Its D final accepted fault and D test
completion are both cycle 65536.

| Event | EARLY | GROUP_NO_SCRATCH_V2 |
| --- | ---: | ---: |
| `GROUP_LAST_FAULT_ACCEPT` | 65536 | 65536 |
| `GROUP_TEST_DONE` | 65536 | 65536 |
| `DSS_DECISION_START` | 65537 | 65537 |
| A ready/finalize | 65538 | 65555 |
| B ready/finalize | 65539 | 65557 |
| C ready/finalize | 65540 | 65559 |
| D ready/finalize | 65541 | 65561 |
| `GROUP_DECISION_READY` | 65541 | 65561 |

Relative to `DSS_DECISION_START`, the EARLY trace remains `1/2/3/4/4` and the
GROUP trace remains `18/20/22/24/24`; both match S1B-1.  GROUP per-SA values
remain internal finalization edges, while group `done_o` remains the only
externally visible group decision completion.

The EARLY failure-C trace has `GROUP_TEST_DONE=65536`,
`DSS_DECISION_START=65537`, A/B ready at 65539/65540, and terminal group done
at 65544.  S1B-1's core regressions independently retain first-failure and
no-rollback behavior.  The all-zero-fault trace retains
`SA_TEST_DONE[A]=16384` and an S1B-1 terminal decision, but both A and group
last-fault values are N/A; no timestamp is synthesized.

## Ordering and raw decomposition

The utility checks these invariants for every applicable trace:

```text
SA_LAST_FAULT_ACCEPT[i] <= SA_TEST_DONE[i]
GROUP_LAST_FAULT_ACCEPT <= GROUP_TEST_DONE
GROUP_TEST_DONE <= DSS_DECISION_START
DSS_DECISION_START <= GROUP_DECISION_READY
```

All pass.  For the late-D positive-fault traces, the raw components below are
timeline evidence only, not formal latency metric names or statistics.

| Trace | BIST tail | Handoff | DSS decision | Fault-to-group-done | Identity |
| --- | ---: | ---: | ---: | ---: | --- |
| EARLY | 0 | 1 | 4 | 5 | PASS |
| GROUP_NO_SCRATCH_V2 | 0 | 1 | 24 | 25 | PASS |

The checked identity is:

```text
GROUP_DECISION_READY - GROUP_LAST_FAULT_ACCEPT
= (GROUP_TEST_DONE - GROUP_LAST_FAULT_ACCEPT)
+ (DSS_DECISION_START - GROUP_TEST_DONE)
+ (GROUP_DECISION_READY - DSS_DECISION_START)
```

For a zero-fault group the fault-based total and identity are N/A.  EARLY
SA-level decomposition remains partial because a shared group handoff has no
independent per-SA DSS launch.  The target fault-accept source remains
`MODEL_DERIVED`; no ready/backpressure is modeled in the frozen V2 path.

## Regression evidence and readiness

Commands run after S1C:

```text
make test_dss_timeline_correlation
make test_fault_address_bist
bash scripts/simulation/run_dss_v2_phase4f3_group.sh
bash scripts/simulation/run_recam_phase4e3_random.sh
make test_solution_take_policy
```

All passed.  The frozen EARLY and GROUP RTL-vs-golden regressions each retain
0 mismatches for 50 and 1000 vectors.  The S0R 2x2x1 1000-vector corpus reports
0 ConfigID, PatternID, action, ledger, and failure-position mismatches.

Mathematical readiness is intentionally qualified: group intervals are
available in one directed trace, but target fault acceptance is model-derived
and the handoff is verification glue.  No formal latency distribution,
repair-rate, synthesis, or device timing result was run.

## Required status report

```text
S1C_STATUS:
COMPLETE

SOURCE_FILES_MODIFIED:
Makefile
inc/DssTimelineCorrelation.hpp
src/DssTimelineCorrelation.cpp
tests/dss_timeline_correlation_test.cpp
docs/dss_execution/S1C_DATE_2X2_TIMELINE_CORRELATION.md

PRODUCTION_RTL_CHANGED:
NO

REPAIR_SEMANTICS_CHANGED:
NO

BIST_ALGORITHM_CHANGED:
NO

UNIFIED_TIMEBASE:
Integer cycle index relative to one serial-BIST/DSS transaction.

CYCLE_ZERO:
Start of the 2x2 serial-BIST transaction.

SAME_CORPUS_LINKAGE:
PASS

--------------------------------
BIST TO DSS HANDOFF
--------------------------------

GROUP_TEST_DONE_TO_DSS_START:
NEXT_CYCLE

GAP_CYCLES:
1

ARCHITECTURAL_OR_HARNESS_GAP:
HARNESS_ARTIFACT

--------------------------------
EARLY CORRELATED TRACE
--------------------------------

group_test_done: 65536
group_last_fault_accept: 65536
dss_start: 65537
A_ready: 65538
B_ready: 65539
C_ready: 65540
D_ready: 65541
group_done: 65541

relative_DSS_cycles_match_S1B1:
PASS

--------------------------------
GROUP CORRELATED TRACE
--------------------------------

group_test_done: 65536
group_last_fault_accept: 65536
dss_start: 65537
A_internal_finalize: 65555
B_internal_finalize: 65557
C_internal_finalize: 65559
D_internal_finalize: 65561
group_done: 65561

relative_DSS_cycles_match_S1B1:
PASS

--------------------------------
FAILURE TRACE
--------------------------------

status: TERMINAL_FAILURE
failure_position: C
group_test_done: 65536
dss_start: 65537
group_done: 65544
no_rollback_preserved:
YES

--------------------------------
ZERO FAULT TRACE
--------------------------------

status: PASS
SA_test_done_defined:
YES

SA_last_fault:
N/A

group_last_fault:
N/A

--------------------------------
LATE FAULT TRACE
--------------------------------

status: PASS
last_fault_cycle: 65536
test_done_cycle: 65536
gap_cycles: 0

--------------------------------
ORDERING INVARIANTS
--------------------------------

last_fault_le_test_done:
PASS

group_last_fault_le_group_test_done:
PASS

group_test_done_le_dss_start:
PASS

dss_start_le_group_done:
PASS

--------------------------------
DECOMPOSITION
--------------------------------

BIST_TAIL:
EARLY=0; GROUP_NO_SCRATCH_V2=0

HANDOFF_GAP:
EARLY=1; GROUP_NO_SCRATCH_V2=1; HARNESS_ARTIFACT

DSS_DECISION_LATENCY:
EARLY=4; GROUP_NO_SCRATCH_V2=24

FAULT_TO_GROUP_DONE_TOTAL:
EARLY=5; GROUP_NO_SCRATCH_V2=25

IDENTITY_CHECK:
PASS

--------------------------------
FUNCTIONAL NON-INTERFERENCE
--------------------------------

CONFIGID_MISMATCHES:
0

PATTERNID_MISMATCHES:
0

ACTION_MISMATCHES:
0

LEDGER_MISMATCHES:
0

FAILURE_POSITION_MISMATCHES:
0

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

GIT_DIFF_CHECK:
PASS
```
