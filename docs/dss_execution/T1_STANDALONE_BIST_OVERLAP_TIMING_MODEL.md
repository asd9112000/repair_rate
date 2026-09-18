# T1 — Standalone C++ BIST Overlap Timing Model

> Status: COMPLETE after T1R. The following original BLOCKED record is
> preserved as historical evidence; the authoritative resumed closure begins
> after that record. T1 adds a standalone timing module and tests only. It
> does not modify repair RTL, repair semantics, the BIST scheduler, or
> HierarchicalRECAM.

## Historical record — pre-T1R blocker

## 1. Objective

T1 was authorized to create an independently testable, O(number of faults)
model for BIST fault arrivals, last-fault timing, candidate/provisional-ready
timing, hidden analysis slack, and exposed post-BIST latency. The model must
use physical Fault::r and Fault::c only, without mapping BIST word columns to
V2 repair ColumnWord values.

Implementation is blocked because the required last-fault-to-provisional-
candidate delay has no established numerical rule.

## 2. T0/T0B inherited contracts

- T0 and T0B are closed; T0B selected Branch C.
- Fault::r is a 9-bit row and Fault::c is a 13-bit physical-cell column.
- The BIST-only timing mapping is bistWordColumn = floor(Fault::c / 256).
- 512 physical rows and 8192 physical cell columns give 32 BIST words per
  row.
- BIST wordColumn is not assumed equal to V2 ColumnWord[4:0].

## 3. BIST schedule semantics

SerialBistSchedule is explicit in inc/FaultAddress.hpp and
src/FaultAddress.cpp. It scans exactly four SAs, strictly A then B then C then
D. Each SA is row-major and then BIST-word-column-major. It uses a completed
word event; it has no distinct issue/complete pipeline.

    BIST_SA_SCHEDULING: SERIAL
    wordsPerRow: 8192 / 256 = 32
    addressesPerSA: 512 * 32 = 16384
    cyclesPerWord: 1
    groupStartCycle: 0

    SA_START_CYCLE[A]: 0
    SA_START_CYCLE[B]: 16384
    SA_START_CYCLE[C]: 32768
    SA_START_CYCLE[D]: 49152

The values are confirmed by tests/fault_address_bist_test.cpp and the existing
S1B-2 timeline materializer.

## 4. Cycle convention

Cycle zero is the start of the group-relative serial BIST transaction and of
SA A's first word interval. A word is accepted at the completed-word event:

    FIRST_ADDRESS_ACCEPT:
    SA i start cycle + cyclesPerWord; for A at default geometry, cycle 1.

For SA i and a local scan index in 0..16383:

    LAST_ADDRESS_ACCEPT:
    saStartCycle + (localScanIndex + 1) * cyclesPerWord.

The final local scan index is 16383, so the last accepted word and test-done
boundary are identical:

    TEST_DONE:
    saStartCycle + 16384 * cyclesPerWord.

Thus A/B/C/D test-done cycles are 16384/32768/49152/65536, and group test done
is SerialBistSchedule::completionCycle() = 65536. This is an actual maximum
of serial SA completion boundaries, not an unverified new group formula.

## 5. Physical-to-BIST timing mapping

The only permitted mapping is already implemented:

    physicalCellColumn = Fault::c
    bistWordColumn = physicalCellColumn / 256
    localBistScanIndex = Fault::r * 32 + bistWordColumn
    faultArrivalCycle = saStartCycle + (localBistScanIndex + 1)

The existing SerialBistSchedule::arrivalCycle() is the authoritative,
overflow-checked form of this computation. It operates in the physical/BIST
domain and neither consumes nor produces a V2 repair address.

## 6. Candidate-delay source

No valid source exists for the requested per-SA
SA_LAST_FAULT_ACCEPT -> LATEST_CANDIDATE_READY delay.

S1F defines the requested quantity as:

    CANDIDATE_CATCHUP_LATENCY[i]
      = LATEST_CANDIDATE_READY[i] - SA_LAST_FAULT_ACCEPT[i]

but expressly states that its numerical cycle is intentionally not invented.
S1E further establishes that frozen V2 has no maintained
PROVISIONAL_SOLUTION_READY state. These are direct blockers, not missing
implementation detail.

The existing Phase-4I DssPostBistLatency model is not a substitute. Its
4-cycle EARLY and 20-plus-cycle GROUP rules describe final group
decision/resource-ledger work after an externally supplied functional decision
plan. S1E explicitly says that those measurements exclude raw-fault
collection and candidate/matrix construction. Using them as a per-SA
provisional-candidate delay would mislabel final policy work and would invent
the missing contract.

    CANDIDATE_DELAY_SOURCE: NOT_AVAILABLE
    CANDIDATE_DELAY_UNIT: cycles
    CANDIDATE_DELAY_VALUE_OR_RULE: NOT_ESTABLISHED

## 7. Timing equations

The BIST-side terms are established:

    lastFaultCycle = max(SerialBistSchedule::arrivalCycle(fault_i))

The remaining requested equations are intentionally not evaluated:

    candidateReadyCycle = lastFaultCycle + candidateDelay
    hiddenAnalysisSlack = testDoneCycle - candidateReadyCycle
    provisionalPostBistLatency = max(0, candidateReadyCycle - testDoneCycle)

They require the unavailable candidateDelay. T1 does not insert zero, a
Phase-4I final-decision value, or a V2 ColumnWord-based rule in its place.

## 8. Zero-fault semantics

Existing DssBistFaultTimeline semantics are retained:

- Each SA still has deterministic testStartCycle and testDoneCycle.
- A zero-fault SA has hasAcceptedFault=false and lastFaultAcceptCycle=N/A.
- A zero-fault group likewise has group last-fault=N/A.

Candidate/provisional-ready timing is also N/A in T1 because the initial or
trivial-ready contract has not been established. It is not silently set to
cycle zero or to test done.

## 9. C++ module design

No new C++ module is implemented while this gate is blocked. Existing
SerialBistSchedule and DssBistFaultTimeline already provide the reusable
physical-to-BIST, per-SA boundary, and zero-fault primitives.

After an explicit candidate-catchup rule is accepted, a standalone
BistOverlapTimingModel may consume those existing types and expose separate
physicalCellColumn, bistWordColumn, and v2RepairColumnWord names. The last
name must remain absent from this T1 model unless a later repair-domain
contract authorizes it.

## 10. Directed tests

The requested T1-D1 through T1-D10 test suite is NOT_RUN: constructing
candidate-ready expected values would require fabricating the missing delay.

The existing fault-address/BIST test independently already covers first word,
middle position, final word, same-256-bit-word arrivals, adjacent word
boundaries, zero faults, and A/B/C/D serial boundaries. It is evidence for the
BIST portion only, not a completed T1 candidate-overlap test suite.

## 11. Analytical-vs-reference proof

NOT_RUN. The analytical BIST mapping itself is already cross-checked by
SerialBistSchedule tests, but an analytical-versus-brute-force proof of
candidate-ready, slack, and exposed candidate latency cannot be formed until
candidateDelay is defined.

    ANALYTICAL_VS_REFERENCE: NOT_RUN
    MISMATCH_COUNT: N/A

## 12. Known limitations

1. Frozen V2 does not retain or expose a provisional-ready event.
2. S1F intentionally leaves the numerical catch-up latency open.
3. Phase-4I final decision latency cannot be relabelled as candidate latency.
4. T1 must not infer a repair mapping from BIST word columns.
5. Zero-fault candidate-ready semantics are also not frozen.

## 13. T2 integration plan

T2 remains unauthorized. Before T2, a review must freeze a candidate-catchup
contract with an event edge, cycle convention, zero-fault behavior, and either
a fixed validated delay or a validated data/config-dependent rule. Only then
may T1 implement and validate its standalone model; only a closed T1 may be
considered for HierarchicalRECAM integration.

## 14. Closure decision

T1 is BLOCKED by the explicit mandatory-stop condition:

    candidate-delay source cannot be identified

No source or timing behavior was changed to work around that absence.

## T1 final status

    T1_STATUS:
    BLOCKED

    SELECTED_T0B_BRANCH:
    C

    BIST_TIMING_DOMAIN:
    PHYSICAL_FAULT_TO_256BIT_WORD

    V2_REPAIR_COLUMN_EQUIVALENCE_ASSUMED:
    NO

    BIST_ROWS:
    512

    PHYSICAL_CELL_COLUMNS:
    8192

    BIST_WORD_BITS:
    256

    BIST_WORD_COLUMNS:
    32

    BIST_SA_SCHEDULING:
    SERIAL

    SA_START_CYCLE_A:
    0

    SA_START_CYCLE_B:
    16384

    SA_START_CYCLE_C:
    32768

    SA_START_CYCLE_D:
    49152

    FIRST_ADDRESS_ACCEPT_CONVENTION:
    completed first-word event at saStartCycle + 1; A is cycle 1

    LAST_ADDRESS_ACCEPT_CONVENTION:
    saStartCycle + (localBistScanIndex + 1) * cyclesPerWord

    TEST_DONE_CONVENTION:
    completed final-word event = saStartCycle + 16384 * cyclesPerWord

    CANDIDATE_DELAY_SOURCE:
    NOT_AVAILABLE; S1F explicitly leaves numerical candidate catchup un-invented

    CANDIDATE_DELAY_RULE:
    NOT_ESTABLISHED

    ZERO_FAULT_SEMANTICS:
    last fault and candidate-ready are N/A; BIST start/done boundaries remain defined

    TIMING_IMPLEMENTATION_COMPLEXITY:
    NOT_IMPLEMENTED; required future model remains O(NUM_FAULTS)

    DIRECTED_TESTS:
    NOT_RUN

    ANALYTICAL_VS_REFERENCE:
    NOT_RUN

    MISMATCH_COUNT:
    N/A

    HIERACHIRECAM_MODIFIED:
    NO

    REPAIR_SEMANTICS_MODIFIED:
    NO

    RTL_MODIFIED:
    NO

    EARLY_MODIFIED:
    NO

    GROUP_MODIFIED:
    NO

    S1GB_RESUMED:
    NO

    SYNTHESIS_RUN:
    NO

    GIT_DIFF_CHECK:
    PASS

    T1_DOCUMENT:
    docs/dss_execution/T1_STANDALONE_BIST_OVERLAP_TIMING_MODEL.md

    NEXT_RECOMMENDED_PHASE:
    Review and freeze the per-SA candidate-catchup latency and zero-fault
    provisional-ready contract; T2_HIERACHIRECAM_TIMING_INTEGRATION remains unauthorized

## Resumed T1 closure — post-T1R

### 1. Original blocker

The original T1 record above was correctly BLOCKED: it had the physical BIST
schedule but no proven `SA_LAST_FAULT_ACCEPT -> LATEST_CANDIDATE_READY` rule.
It did not invent a Phase-4I final-decision delay or equate BIST word columns
with V2 repair `ColumnWord` values.

### 2. T1R resolution

T1R closed the missing contract from the frozen S1G-A2G RTL and its
Verilator harness:

```text
LAST_FAULT_ACCEPT      = retained_update_event_o[i]
LATEST_CANDIDATE_READY = latest_candidate_ready_o[i]
                         with analysis-valid and matching generation
current owner          = four uninterrupted sweep edges
non-owner              = ownership_wait + four sweep edges
same edge update       = update wins over old completion
```

### 3. Frozen timing contracts

The C++ model uses the exact completed-word convention from
`SerialBistSchedule`:

```text
A/B/C/D start:     0 / 16384 / 32768 / 49152
A/B/C/D test done: 16384 / 32768 / 49152 / 65536
first accepted word for A: cycle 1
```

`Fault::r` remains a physical 9-bit row and `Fault::c` a physical 13-bit
cell column. The only BIST mapping is `Fault::c / 256`; no V2
`ColumnWord[4:0]` equivalence is assumed.

### 4. BIST event generation

`BistOverlapTimingModel::evaluateGroup()` consumes a `FaultGroup` and the
existing `DssBistFaultTimeline`/`SerialBistSchedule` domain. It orders events
by completed-word cycle and SA. Multiple physical fault bits in the same
tested BIST word remain counted as multiple physical faults, but become one
timing-level selected-SA update event for that cycle. This preserves the
frozen `<= 1` global accepted update per cycle rather than manufacturing an
impossible multi-update collector edge.

This aggregation is timing-only. It does not claim that a one-event replay is
sufficient to reproduce the repair payload of multiple physical bits; T1 does
not perform repair selection.

### 5. Ownership model

`BistOverlapTimingModel` tracks the frozen A -> B -> C -> D owner and these
control states:

```text
SWEEP -> WAIT_TEST_DONE -> FINAL_SELECT -> next owner
```

The model advances an owner only on the successful control-path abstraction:
a matching latest-generation candidate is ready, that SA's `test_done` has
been observed, and the one final-select control edge is reached. It does not
evaluate candidate feasibility, ledger allocation, or failure policy. This is
the minimum scheduler state needed to preserve T1R candidate timing while
keeping final repair semantics outside T1.

### 6. Generation semantics

Each accepted timing event increments the selected SA's generation and clears
its previous ready generation. An accepted event for a non-owner updates only
that SA's retained timing state. An accepted event for the current owner
resets sweep progress to zero; partial progress is not retained.

### 7. Same-edge new-fault-wins rule

At a would-be fourth sweep edge, a selected owner update is processed before
completion. No old-generation candidate-ready event is emitted; the new
generation requires four further uninterrupted owner sweep edges. T1-D7 and
the direct event-stream reference test exercise this boundary.

### 8. Zero-fault behavior

Generation zero is explicitly modeled. It becomes candidate-ready only after
the respective SA receives its four owner sweep edges. It has no
`lastFaultAcceptCycle`, ownership-wait value, or candidate-catchup latency;
those fields are N/A rather than zero. Candidate-ready remains distinct from
final commit.

### 9. Event-driven implementation

New source files:

```text
inc/BistOverlapTimingModel.hpp
src/BistOverlapTimingModel.cpp
```

The production path jumps between external BIST events and internal ownership
milestones. During an idle interval it analytically accounts for the sweep
edges crossed before the next event, while retaining update priority on the
event edge. Its cost is O(accepted BIST-word events + ownership/control
milestones), not O(65,536 memory addresses).

### 10. Cycle reference model

`evaluateEventStreamCycleReference()` is a test-only cycle-by-cycle replay of
the same frozen controller contract. It advances one edge at a time through
the event horizon and provides the oracle for the event-driven implementation.
It is not used by any simulator path.

### 11. Directed verification

`tests/bist_overlap_timing_model_test.cpp` covers all required T1 directed
conditions:

| Cases | Evidence |
| --- | --- |
| T1-D1 | zero-fault generation-0 A/B/C/D readiness |
| T1-D2–D3 | early and final-word current-owner A updates |
| T1-D4–D5 | non-owner and equivalent A/B/C/D location ownership effects |
| T1-D6–D7 | back-to-back restart and same-edge update-wins |
| T1-D8–D9 | B update just before and on A ownership release |
| T1-D10 | exact 12 physical-fault SA bound |
| T1-D11–D12 | same BIST-word aggregation and adjacent-word separation |
| T1-D13–D15 | hidden/exposed group arithmetic and mixed late-D trace |

`make test_bist_overlap_timing_model` passes all 15 directed checks.

### 12. Randomized verification

The same test generates 1,000 deterministic randomized legal `FaultGroup`
traces. Each SA has at most 12 physical faults; physical faults may share a
BIST word and are aggregated only for the timing event stream. Event-driven
and cycle-reference outputs are compared for every per-SA and group metric.

```text
randomized traces: 1000
mismatches: 0
```

### 13. RTL-harness comparison

The existing retained-overlap Verilator harness now replays a successful
A/B/C/D trace into both the frozen RTL and `BistOverlapTimingModel`. It
compares each SA's `latest_candidate_ready_o` cycle and final generation.

```text
compared trace: 1 multi-SA A/B/C/D trace
candidate-ready cycle mismatches: 0
generation mismatches: 0
```

The T1R D7 Verilator evidence remains the RTL proof for the same-edge
completion/update priority; T1's event-stream D7 independently matches the
cycle reference.

### 14. Output metrics

For every SA the result exposes physical fault count, accepted update count,
BIST start/done, first/last accept when present, latest/ready generation,
candidate-ready cycle, ownership wait, candidate catchup, hidden slack,
post-BIST provisional latency, and `fullyHidden`. Group output exposes group
test done, latest candidate-ready, group hidden slack, provisional post-BIST
latency, and whether all final generations were ready by group test done.

For nonzero-fault SAs:

```text
candidateCatchupLatency = candidateReadyCycle - lastFaultAcceptCycle
ownershipWaitCycles     = candidateCatchupLatency - 4
```

### 15. Complexity

The event-driven model does not scan empty BIST addresses. It processes sorted
accepted word events and a bounded number of A/B/C/D ownership milestones. The
cycle model is intentionally retained only as a test oracle.

### 16. Limitations

1. T1 models provisional candidate readiness, not final candidate selection,
   ledger allocation, commit, repairability, or terminal failure.
2. The A->B->C->D ownership release is the successful S1G-A2G control-path
   abstraction. A failure-aware integration needs the actual repair result and
   remains T2 scope.
3. Same-word aggregation preserves the one-update timing interface and
   physical fault count, but is not a multiple-bit repair-payload model.
4. No device-level experiment, synthesis, S1G-B, or HierarchicalRECAM
   integration was performed.

#### Readable-RTL workflow evidence

T1 changed no production RTL. The readable-RTL public matrix is therefore
`compile/ast/readability/comment/naming/profile = NOT_RUN` for a new RTL
deliverable; no such deliverable exists. `testbench = PASS` and `toolchain =
PASS` apply to the focused Verilator build/execution of the frozen retained
overlap core plus the C++ timing-model comparison. This does not claim a
strict generated-RTL gate or synthesis run.

### 17. T2 readiness decision

T1 is independently closed. T2 may be reviewed for authorization to connect
this validated standalone timing module to the existing simulator fault corpus;
T2 has not started in this change.

```text
T1_STATUS:
COMPLETE

T1_ORIGINAL_BLOCKER_RESOLVED:
YES

TIMING_MODEL_TYPE:
OWNERSHIP_AWARE_EVENT_DRIVEN

BIST_SCHEDULING:
SERIAL_A_B_C_D

SA_START_CYCLES:
A=0
B=16384
C=32768
D=49152

GROUP_TEST_DONE:
65536

V2_REPAIR_COLUMN_EQUIVALENCE_ASSUMED:
NO

CURRENT_OWNER_CANDIDATE_DELAY:
4_UNINTERRUPTED_SWEEP_EDGES

NON_OWNER_DELAY_RULE:
OWNERSHIP_WAIT_PLUS_4

CANDIDATE_DELAY_CLASS:
VARIABLE

SAME_EDGE_NEW_FAULT_WINS:
YES

ZERO_FAULT_GENERATION0_MODELED:
YES

MAX_GLOBAL_ACCEPTED_FAULT_EVENTS_PER_CYCLE:
1

MAX_FAULTS_PER_SA:
12

EVENT_DRIVEN_MODEL:
PASS

CYCLE_REFERENCE_MODEL:
PASS

DIRECTED_TESTS:
PASS

RANDOMIZED_TRACES:
1000

RANDOMIZED_MISMATCHES:
0

RTL_HARNESS_COMPARISON:
PASS

RTL_HARNESS_MISMATCHES:
0

HIERACHIRECAM_MODIFIED:
NO

REPAIR_SEMANTICS_MODIFIED:
NO

RTL_MODIFIED:
NO

EARLY_MODIFIED:
NO

GROUP_MODIFIED:
NO

S1GB_RESUMED:
NO

SYNTHESIS_RUN:
NO

GIT_DIFF_CHECK:
PASS

T1_DOCUMENT:
docs/dss_execution/T1_STANDALONE_BIST_OVERLAP_TIMING_MODEL.md

T2_READY:
YES

NEXT_RECOMMENDED_PHASE:
T2_HIERACHIRECAM_TIMING_INTEGRATION
```
