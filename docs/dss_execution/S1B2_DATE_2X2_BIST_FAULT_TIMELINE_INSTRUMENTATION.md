# S1B-2 — DATE 2x2 BIST / Fault Timeline Instrumentation

## Scope and status

`S1B2_STATUS: COMPLETE`

This work materializes reporting-only pre-DSS timeline events for the frozen
2x2 directional-CAM configuration (`RS=2`, `CS=2`, `SHARE_M=1`).  It preserves
the S1B-1 V2 decision endpoints exactly and does not calculate or publish a
formal Post-BIST or Fault-tail latency experiment.

Changed sources:

- `inc/FaultAddress.hpp`
- `src/FaultAddress.cpp`
- `inc/DssBistFaultTimeline.hpp`
- `src/DssBistFaultTimeline.cpp`
- `tests/fault_address_bist_test.cpp`

No production V2 RTL, RTL interface, repair policy, BIST algorithm, fault
generation, device scheduler, or architectural repair state was changed.

## Cycle semantics and S1B-1 preservation

The serial BIST schedule has a group-relative `groupStartCycle`.  Its scan is
strictly A, then B, then C, then D; within an SA it is row-major then
word-column-major.  `arrivalCycle()` already assigns the completed word event
as:

```text
groupStartCycle + (scan_index + 1) * cyclesPerWord
```

The schedule does not distinguish address issue from address completion.
S1B-2 therefore uses that existing completed-word event for both a
model-derived accepted fault and the last address completion of the relevant
SA.  It adds no wait cycle.

The preserved S1B-1 decision endpoints remain:

| Policy | DSS start | SA decision/finalize | Group decision ready |
| --- | --- | --- | --- |
| `EARLY` | accepted `start_i && !busy_o` | `sa_commit_valid_o[i]` rising edge | `done_o` rising edge |
| `GROUP_NO_SCRATCH_V2` | accepted `start_i` while `state_q == IDLE` | `sa_commit_valid_o[i]` rising edge, internal sequential finalization | `done_o` rising edge |

The all-local reference remains EARLY A/B/C/D/done = `1/2/3/4/4` and GROUP
A/B/C/D/done = `18/20/22/24/24`, relative to their accepted DSS start edges.

## SA_TEST_DONE

### Implementation

`SerialBistSchedule::subarrayStartCycle()` and
`SerialBistSchedule::subarrayCompletionCycle()` are pure schedule queries.
`materializeDssBistFaultTimeline()` records each per-SA start/done boundary in
a non-functional trace structure.

With the frozen default geometry (512 rows, 8192 cells, 256-bit words,
therefore 32 words/row) and `cyclesPerWord=1`, the completed last address is
row 511 / word-column 31.  The derived serial boundaries are:

| SA | test start | completed last address / `SA_TEST_DONE` |
| --- | ---: | ---: |
| A | 0 | 16384 |
| B | 16384 | 32768 |
| C | 32768 | 49152 |
| D | 49152 | 65536 |

These differing values are intentional: the current model is one serial
A→B→C→D BIST traversal, not four independent BIST engines.  A zero-fault SA
still has both a defined start and a defined test-done event.

## SA_LAST_FAULT_ACCEPT

### Implemented target path: MODEL_DERIVED

The frozen V2 policy paths consume a prebuilt `FaultGroup`; they do not expose
a collector valid/ready interface.  The new C++ trace therefore labels every
implemented target event `MODEL_DERIVED`.

The observer operation defining acceptance is: iterate each existing
`FaultGroup[subarray][faultIndex]` element, validate that its physical
`SubarrayID` matches the vector index, and assign the existing serial schedule
completed-word cycle.  The trace stores SA identity, original fault index,
physical address, and accept cycle; the SA value is the maximum such cycle.
There is no modeled queue or backpressure in this replay path.

```text
has_accepted_fault[SA] = false  => SA_LAST_FAULT_ACCEPT = N/A
has_accepted_fault[SA] = true   => max accepted-fault cycle for that SA
```

Thus no zero-fault SA substitutes test start, test done, or cycle zero for
`SA_LAST_FAULT_ACCEPT`.

### Alternate hardware-observed path

The historical/alternate collector RTL has a genuine hardware acceptance
predicate: `fault_valid_i && fault_ready_o`, sampled on the rising edge (for
example `rtl/dss_2x2/common/fault_cam.sv:40-54,84-93`; the tagged collector
variant is `rtl/dss_2x2/common/fault_collector.sv:28-68`).  The existing
`shared_fault_collector` Verilator test also passes with its hardware
`fault_valid_i && fault_ready_o` accept predicate.

That collector is not in the frozen V2 EARLY or GROUP_NO_SCRATCH_V2 decision
boundary and has no connection to the serial-BIST schedule.  Consequently the
comparison classification is `NOT_DIRECTLY_COMPARABLE`: both paths preserve
the logical meaning “accepted rather than merely presented,” but only the C++
target trace has serial schedule cycles, while only the alternate RTL path has
ready/backpressure behavior.  No legacy collector trace is merged with a V2
policy trace.

## Group-derived events and DSS-start relationship

```text
GROUP_TEST_DONE          = max SA_TEST_DONE = schedule.completionCycle()
GROUP_LAST_FAULT_ACCEPT  = max SA_LAST_FAULT_ACCEPT over SAs with faults
```

For an all-zero-fault group, `GROUP_LAST_FAULT_ACCEPT` is N/A.  For the
directed late-D trace, D's final accepted fault and `GROUP_TEST_DONE` both
occur at cycle 65536; this is timeline evidence only, not a published
Fault-tail metric.

`GROUP_TEST_DONE_TO_DSS_START` is **not observed** in the current repository:
the serial schedule trace and frozen V2 core tests are deliberately separate
harnesses, and no BIST/collector-to-V2 wrapper was authorized.  Therefore:

```text
GROUP_TEST_DONE_TO_DSS_START: UNOBSERVED_IN_SEPARATE_HARNESSES
EXTRA_GAP_CYCLES: N/A
```

No equality was forced and no decision-start delay was invented.

## Directed validation and regression evidence

`make test_fault_address_bist` passed the following small directed timeline
traces:

| Trace | Evidence |
| --- | --- |
| zero-fault A / all-zero group | A test-done exists; A and group last-accept are N/A |
| single fault B | accepted at cycle 16385 |
| multiple faults C | accepts at 32769 and 32801; last is 32801 |
| late fault D | accepted at cycle 65536, no later than D test-done |
| faults in A only | group last-accept is cycle 1 |
| faults in D only | group last-accept is cycle 65536 |
| multiple SAs | accepted-fault trace sorts by serial schedule cycle |

The timeline test labels the directed pre-DSS traces with only the allowed
policy identities: `EARLY` and `GROUP_NO_SCRATCH_V2`.  S1B-1's unchanged core
regressions provide the successful EARLY, successful GROUP_NO_SCRATCH_V2, and
terminal-failure decision cases; their decision endpoints were rerun below.

Commands run after instrumentation:

```text
make test_fault_address_bist
bash scripts/simulation/run_verilator_test.sh shared_fault_collector ...
bash scripts/simulation/run_dss_v2_phase4f3_group.sh
bash scripts/simulation/run_recam_phase4e3_random.sh
make test_solution_take_policy
```

Results:

- focused BIST/timeline test: PASS;
- alternate hardware collector test: PASS (existing lint warnings only);
- frozen V2 GROUP core regression: PASS;
- EARLY RTL-vs-golden: 50 and 1000 vectors, 0 mismatches;
- GROUP RTL-vs-golden frozen regression: 50 and 1000 vectors, 0 mismatches;
- S0R V2 2x2x1 random corpus: `config=0`, `pattern=0`, `action=0`,
  `ledger=0`, `failure=0` mismatches.

## Metric-readiness boundary

The four pre-DSS timestamp fields are now materialized, but they are not yet
joined to a common BIST-to-DSS harness.  Metric calculation is therefore not
authorized and the readiness is partial rather than a result claim.

| Future metric | Readiness | Reason |
| --- | --- | --- |
| SA Post-BIST | PARTIAL | per-SA test-done and S1B-1 SA commit exist, but not in one integrated trace |
| Group Post-BIST | PARTIAL | group test-done and group `done_o` exist, but no observed start relation/gap |
| SA Fault-tail | PARTIAL | last-accept is model-derived and optional for zero-fault SAs |
| Group Fault-tail | PARTIAL | group last-accept exists only if the group has an accepted fault; no integrated decision trace |

## Required status report

```text
S1B2_STATUS:
COMPLETE

SOURCE_FILES_MODIFIED:
inc/FaultAddress.hpp
src/FaultAddress.cpp
inc/DssBistFaultTimeline.hpp
src/DssBistFaultTimeline.cpp
tests/fault_address_bist_test.cpp
docs/dss_execution/S1B2_DATE_2X2_BIST_FAULT_TIMELINE_INSTRUMENTATION.md

PRODUCTION_RTL_CHANGED:
NO

REPAIR_SEMANTICS_CHANGED:
NO

BIST_ALGORITHM_CHANGED:
NO

--------------------------------
SA_TEST_DONE
--------------------------------

IMPLEMENTED:
YES

SOURCE_LAYER:
CXX / SCHEDULER_DERIVED

INSERTION_POINT:
SerialBistSchedule completed last-word boundary for each A->B->C->D SA.

CYCLE_SEMANTICS:
Existing serial completed-word event; issue/complete are not separately modeled.

PER_SA_AVAILABLE:
YES

ZERO_FAULT_SUPPORTED:
YES

--------------------------------
SA_LAST_FAULT_ACCEPT
--------------------------------

IMPLEMENTED:
YES

SOURCE_TYPE:
MODEL_DERIVED

AUTHORITATIVE_EVENT:
FaultGroup replay element accepted at its existing SerialBistSchedule arrivalCycle.

HANDSHAKE_USED:
NO in frozen V2 target path; alternate collector handshake remains separate.

PER_SA_AVAILABLE:
YES

ZERO_FAULT_BEHAVIOR:
N/A, with has_accepted_fault=false and source=NONE.

--------------------------------
GROUP DERIVED EVENTS
--------------------------------

GROUP_TEST_DONE:
  derivation: max SA_TEST_DONE = SerialBistSchedule::completionCycle()
  status: AVAILABLE

GROUP_LAST_FAULT_ACCEPT:
  derivation: maximum accepted-fault cycle among SAs with has_accepted_fault=true
  zero-fault-group_behavior: N/A
  status: AVAILABLE_WHEN_GROUP_HAS_ACCEPTED_FAULT

--------------------------------
DSS START RELATION
--------------------------------

GROUP_TEST_DONE_TO_DSS_START:
UNOBSERVED_IN_SEPARATE_HARNESSES

EXTRA_GAP_CYCLES:
N/A

--------------------------------
DIRECTED_VALIDATION
--------------------------------

ZERO_FAULT_CASE:
PASS

SINGLE_FAULT_CASE:
PASS

MULTI_FAULT_CASE:
PASS

LATE_FAULT_CASE:
PASS

EARLY_SUCCESS_CASE:
PASS

GROUP_SUCCESS_CASE:
PASS

TERMINAL_FAILURE_CASE:
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

INSTRUMENTATION_REGRESSION:
PASS

--------------------------------
FUTURE METRIC READINESS
--------------------------------

SA_POST_BIST:
PARTIAL

GROUP_POST_BIST:
PARTIAL

SA_FAULT_TAIL:
PARTIAL

GROUP_FAULT_TAIL:
PARTIAL

--------------------------------
LIMITATIONS
--------------------------------

Target-path fault acceptance is MODEL_DERIVED from fault-list replay and has
no modeled ready/backpressure.  The alternate hardware collector is a separate
legacy topology, so it is NOT_DIRECTLY_COMPARABLE to the frozen V2 policy path.
The BIST and DSS decision tests remain separate harnesses; their gap is not
observed.  No device scheduler, device timing, repair-rate, Post-BIST, or
Fault-tail experiment was run.

--------------------------------
NEXT PHASE
--------------------------------

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
