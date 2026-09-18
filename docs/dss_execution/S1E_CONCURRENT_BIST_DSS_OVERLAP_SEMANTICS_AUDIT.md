# S1E — Concurrent BIST / DSS Overlap Semantics Audit

`S1E_STATUS: COMPLETE`
`SOURCE_MODIFIED: NO` (implementation source; this audit document is new)
`E0-R: PAUSED`

## Scope and method

This is a read-only source-semantics audit of the historical 2x2 collector,
the frozen V2 EARLY and GROUP-NoScratch boundaries, their test-side timing
chain, and the primary C++ simulator.  No RTL, policy, BIST, fault generator,
latency experiment, or result artifact was changed.  The only repository
addition for S1E is this audit record.

The key distinction used throughout is:

| Term | Meaning in this audit |
| --- | --- |
| Fault-state update | Accepting a raw fault and changing CAM/pivot/counter/hybrid state. |
| Candidate evaluation | Computing Config feasibility and its representative PatternID from the state currently presented. |
| Final policy decision | Selecting/committing a resource action that is valid for the complete tested fault set. |

An evaluator can be combinationally active while an input snapshot changes,
without making its result a final or safely committable DSS decision.

## Evidence map and fault-arrival paths

### Historical RTL collector/analyzer path

`rtl/dss_2x2/analyzer/dss_analyzer_top.sv` is a historical 2x2 boundary, not
the frozen V2 production top.  It accepts raw `fault_valid_i` / `fault_row_i` /
`fault_col_i` and instantiates `shared_fault_collector`.  In
`rtl/dss_2x2/analyzer/shared_fault_collector.sv`, `accept` is
`fault_valid_i && fault_ready_o`; on that clock edge it increments the fault
count and writes the pivot CAM, row/column counters, hybrid store, and/or CAM
reuse store.  Therefore its stored analyzer inputs change on accepted faults,
without a `test_done` condition.

The parallel `multi_config_analyzer_bank` in `dss_analyzer_top.sv` consumes
those current collector outputs combinationally.  Its Config/Pattern map is
captured into `config_result_bank` only on `subarray_commit_i` (the area
orientation instead triggers an area engine at that same commit).  Thus:

```text
raw fault accepted -> collector state updates at the edge
                 -> parallel candidate view changes from that state
subarray_commit_i -> current per-SA map is retained for policy use
```

The older `rtl/dss_2x2/cam/dss_cam_top.sv` / `common/fault_collector.sv`
provide the same kind of raw fault acceptance and storage.  They intentionally
accept `analysis_start_i` only while there is no concurrent `fault_valid_i`.
That is an explicit collection-to-analysis handoff for that top, not evidence
that a final solution is valid during continuing BIST.

### Frozen V2 boundary

`rtl/dss_v2/top/recam_dss_v2_early_top.sv` and
`rtl/dss_v2/top/recam_dss_v2_group_top.sv` have no raw fault valid/ready,
fault address, BIST progress, `SA_TEST_DONE`, or `GROUP_TEST_DONE` port.
Their analyzer inputs are already prepared summaries:

```text
pivot_valid / pivot rows / pivot columns
row_gt{1,2,3} / col_gt{1,2,3}
hybrid valid / pointer / descriptor / differing address
conventional_overflow
```

`rtl/recam/recam_shared_config_analyzer.sv` is one fully combinational
single-Config evaluator (`always @*`).  It reconstructs a bounded matrix from
the supplied snapshot, evaluates fixed PatternIDs, and emits the first valid
PatternID plus `solution_valid_o` / `repairable_o`.  It neither captures raw
faults nor contains a completion/finality qualifier.  It can re-evaluate when
an external producer changes its summary inputs, but the frozen V2 top does
not contain that producer or a raw-BIST connection.

Consequently, the only supported V2 integration contract is:

```text
prepared, caller-owned analyzer snapshot -> V2 analyzer -> policy core
```

It is not:

```text
BIST detects a fault -> frozen V2 accepts it -> frozen V2 updates state
```

### C++ and timing-harness path

The primary C++ adapter has an equally important sequential boundary.
`RECAMSolverAdapter::solve` in `src/RECAMSolverAdapter.cpp` first appends the
entire supplied vector to a fresh `FaultList`; only after that loop does it
call `FaultList::classifyFaults()`, `RECAM_PE::loadFaultsToCAMs()`,
`genFaultAnalyzeMatrix()`, and `genValidSolList()`.  The
`DirectionalMultiConfigAnalyzer` invokes that full-vector operation for each
SA/configuration.  It has no incremental transaction state across arriving
faults.

`DssBistFaultTimeline` is explicitly a reporting/model-derived replay:
`SerialBistSchedule::arrivalCycle()` supplies a cycle for each already-owned
fault, but it does not feed a V2 raw-fault interface.  `DssTimelineCorrelation`
then constructs the existing one-cycle post-`GROUP_TEST_DONE` start handoff.
Therefore it does not establish a concurrent hardware schedule.  The older
`DssPostBistLatency` restart-on-fault model and the optional SRAM
`analyzeAfterEachFault` mode are separate historical/simulator models; neither
is evidence of a frozen V2 integration path.

## RECAM concurrent-analysis finding

The answer differs by layer and must not be conflated.

| Question | Historical 2x2 collector/analyzer RTL | Primary C++ RECAM adapter |
| --- | --- | --- |
| Does an arriving fault update stored state? | Yes, on `accept`. | The `FaultList` is populated as a vector, but `solve()` does not analyze until the complete supplied vector has been appended. |
| Can current matrix/candidate logic change during collection? | Yes in parallel-orientation RTL: collector outputs feed the combinational analyzer bank.  A retained result is captured only at `subarray_commit_i`. | No implemented incremental analysis pass. |
| Is a current ConfigID/PatternID observable before test completion? | Conditionally as a current combinational candidate value; it is not final or committed absent an externally asserted commit. | No: results appear only after full-vector classification/analysis. |
| Can a current solution be invalidated by another accepted fault? | Yes. | Yes if a prefix is independently re-solved; the production adapter does not maintain such a prefix result. |

The historical RTL therefore supports **concurrent state update and current
candidate evaluation**, but not proof that the current candidate is final
before BIST completion.  The primary C++ experiment path is
**collection then analysis**.  Any wording that treats the latter as an
implemented continuously restarting analyzer would be inaccurate.

## V2 `start_i` and candidate timing

`start_i` does **not** trigger computation inside
`recam_shared_config_analyzer`: it has no clock or start port and is `always
@*`.  In EARLY, the current role/rank produces a ConfigID continuously and the
analyzer tracks its supplied snapshot.  In GROUP, one current Config result is
also combinationally visible, but all 16 `[SA][slot]` results are sampled only
after `start_i` enters the `COLLECT` state.

`start_i` therefore begins policy-control consumption:

- EARLY: rank-by-rank candidate consumption and immediate resource-ledger
  commits (`recam_dss_v2_early_core.sv`).
- GROUP: a 16-cycle candidate-store sweep, then A -> B -> C -> D allocation
  (`recam_dss_v2_group_core.sv`).

It does not qualify the supplied snapshot as final.  The limited current
combinational evaluation before `start_i` is not useful complete GROUP
precomputation: no all-Config map is retained and no candidate history is
updated outside `COLLECT`.

## EARLY overlap, invalidation, and commit safety

The frozen EARLY core contains no fault-state update path.  It receives only
the analyzer result for its current rank and commits the resource ledger as
soon as `candidate_solution_valid_i`, `candidate_repairable_i`, descriptor,
and feasibility checks pass.  There is no undo or re-evaluation state.
It accepts `start_i && !busy_o`; there is no `SA_TEST_DONE` or
`GROUP_TEST_DONE` gate.  Thus the RTL does not mechanically prevent an
environment from starting and committing too early, but that use is outside
the supported final-snapshot contract and is semantically unsafe.

For an SA whose test is still in progress, a later fault in that **same SA**
can make its currently selected Config infeasible, change the first valid
PatternID, or change the release/borrow action required by the selected
Config.  The existing commit would then require a ledger rollback and policy
replay.  Neither is implemented, and such a commit is therefore unsafe.

A later fault in a *different* SA does not retroactively make an already
valid A allocation invalid: A's committed resource reservation stays owned by
A.  It can, however, alter the later SA's candidate set and make the final
group outcome fail.  This does not rescue early A commit before A itself is
complete; the unsafe case is the late fault in the committed SA.  The current
test harness supplies final analyzer-derived candidates before asserting
`start_i`; it does not exercise BIST-time EARLY commits.

For the required cycle-15/70/100 example, no V2 raw fault state exists at
cycle 15.  A caller could externally change the summary inputs and observe a
current combinational candidate, but no supported provisional state is
stored.  At a fault at cycle 70, EARLY has no restart, invalidation, or
rollback mechanism.  At test completion, the caller must supply the final
snapshot and assert `start_i`; then the existing rank/ledger work remains.
The source does not support a more precise count for this arbitrary trace.

## GROUP-NoScratch overlap, invalidation, and precompute safety

GROUP has safer *policy timing* than EARLY because it delays allocation, but
the frozen implementation still cannot overlap BIST collection with a
maintained candidate map.

`recam_dss_v2_group_core.sv` writes its 80-bit candidate store only when
`state_q == COLLECT`.  `start_i` enters that state; the core then samples the
16 fixed `[SA][slot]` entries and only after the sweep enters `ALLOCATE`.  The
store has no input to invalidate, version, or rewrite a candidate due to a
subsequent raw fault outside `COLLECT`.  Final A -> B -> C -> D allocation
uses that snapshot and its ledger updates, so it requires all four candidate
sets to be frozen.  `done_o` is terminal decision completion, not a
repair-table/runtime-remap release event.

As with EARLY, there is no `GROUP_TEST_DONE` gate on `start_i`: an environment
can syntactically start the core early, but it would sample a non-final
snapshot and hence does not make early final allocation legal.

A separate external collector could continuously update V2 summary inputs, so
one current Config's combinational result is *derivable* while BIST runs.  It
does not make a 16-entry `ConfigValidMap`-equivalent state persistently
available.  To make GROUP precompute useful and correct, a new integration
would need an incremental producer, candidate-map versioning or replacement
on each accepted fault, a final `GROUP_TEST_DONE` qualification, and a rule
that final allocation starts only from the final version.  These are RTL and
scheduling changes, not a harness-only change.

For the cycle-15/70/100 example, the frozen GROUP has no internal fault or
candidate-map state at cycle 15 before `start_i`.  A caller may observe one
current combinational Config evaluation, but no stored complete map or
provisional-ready state exists.  The fault at cycle 70 has no V2 update path.
At cycle 100 a final snapshot must be provided; a new start performs the
16-cycle collection sweep and then the variable A -> B -> C -> D allocation
work.  Exact terminal cycles depend on candidate availability and failure
rank, so no unsupported fixed count is claimed.

## Provisional state and monotonicity

No frozen production boundary exposes `PROVISIONAL_SOLUTION_READY`:

| Layer | Classification | Reason |
| --- | --- | --- |
| Historical collector/analyzer | IMPLICIT current candidate; not final/retained without `subarray_commit_i`. | Current state feeds a combinational bank during collection. |
| Primary C++ RECAM | DERIVABLE only by separately solving a prefix; no live prefix state. | `solve()` receives a complete vector and creates fresh state. |
| V2 EARLY | NOT_SUPPORTED. | No raw state, provisional flag, or rollback. |
| V2 GROUP | DERIVABLE for a caller-provided snapshot, but NOT_SUPPORTED as a maintained internal map. | Candidate store only samples after `start_i` during `COLLECT`; no invalidation/version. |

For a fixed arrival order under the supported RECAM collector contract,
candidate feasibility is **MONOTONIC_DECREASING**: accepted faults only add
pivot/counter/hybrid/reuse constraints or overflow; the candidate matrix logic
rejects a Pattern when a required matrix edge is present and does not erase
such a constraint.  The C++ `FaultList::classifyFaults()` likewise processes
the vector in arrival order and only appends pivot/nonpivot/buffer/overflow
classification.  This statement is about a fixed configuration and fixed
arrival order; it is not a claim that a policy-selected Config or a group
repair outcome is monotonic.

The representative (smallest valid) PatternID is **CAN_CHANGE**.  A lower
numbered currently valid Pattern can become infeasible while a later Pattern
remains valid; the first-valid priority encoder consequently chooses a new
PatternID.  Thus monotonic feasibility does not eliminate the requirement to
invalidate/replace a provisional representative.

## E0-L interpretation

`E0L_DSS_DECISION_LATENCY_CLASS:
POLICY_CONTROL_LATENCY_AFTER_PRECOMPUTED_ANALYZER`

E0-L materialized a full final-fault candidate corpus with
`DirectionalMultiConfigAnalyzer` before either RTL policy test.  It then
drove the core with the resulting candidates and used the S1C one-cycle
post-BIST harness correlation.  Therefore:

- EARLY mean **4.6517 cycles** is terminal rank/priority consumption,
  descriptor/ledger feasibility, immediate commits, and first-failure or
  fourth-SA completion from accepted `start_i`; it excludes raw-fault
  collection and candidate/matrix construction.
- GROUP mean **20.9871 cycles** is the mandatory 16-entry candidate-store
  sweep plus terminal A -> B -> C -> D priority/ledger allocation from
  accepted `start_i`; it likewise excludes raw-fault collection and
  candidate/matrix construction.

The current `GROUP_POST_BIST_ARCH` metric remains **VALID**, with its frozen
scope: it is post-collection *policy-control* latency after the explicit
one-cycle harness handoff has been normalized away.  It is not a physical
end-to-end BIST-to-runtime-remap latency and must not be re-labelled as one.
It is not over-pessimistic based on a supported concurrent V2 model, because
there is no such model/integration today.

## Final release and exposed latency

The physically meaningful future quantity is:

```text
POST_BIST_EXPOSED_LATENCY = FINAL_SOLUTION_RELEASE - GROUP_TEST_DONE
```

Current `done_o` is an internal terminal DSS decision edge.  The V2 tops
export selected Config/Pattern and commit indications, but expose no explicit
final-solution-release, repair-table-reconstructed, or runtime-remap-ready
event.  The minimum future observation point is therefore a valid-qualified
handoff after terminal decision and after any required repair-table/decode
consumer accepts the final selected data.  It must remain distinct from
`done_o` unless the downstream contract proves they are identical.

The shorthand
`max(0, internal_solution_ready - group_test_done)` is not valid for the
current provisional/current-combinational result: that result lacks final
fault-set qualification, may have an obsolete PatternID, and may not include
final policy allocation.  It becomes valid only when `internal_solution_ready`
means a versioned, final-qualified `FINAL_SOLUTION_RELEASE` with all required
post-finalization work included.  Zero exposed latency is consequently
**REQUIRES_ARCHITECTURE_CHANGE** for both frozen policies, not merely a new
measurement schedule.

## Required classifications

```text
================================
RECAM
================================

FAULT_STATE_UPDATE_DURING_BIST: PARTIAL
  Historical RTL collector: YES on fault_valid_i && fault_ready_o.
  Primary C++ adapter: no live BIST event path; collection is a complete vector.

CANDIDATE_ANALYSIS_DURING_BIST: PARTIAL
  Historical parallel analyzer can evaluate current collector state.
  Primary C++ adapter analyzes only after supplied-vector collection.

PROVISIONAL_SOLUTION_BEFORE_TEST_DONE: CONDITIONAL
  A historical current candidate is implicit/derivable, not final or retained
  as a solution without an external commit.

FINAL_SOLUTION_BEFORE_TEST_DONE: NO

CONCURRENCY_CLASS: CONCURRENT_STATE_UPDATE_ONLY
  This describes the historical RTL collector/analyzer boundary, not the
  primary C++ experiment path.

================================
EARLY
================================

FAULT_STATE_UPDATE_DURING_BIST: NO
CANDIDATE_ANALYSIS_DURING_BIST: PARTIAL
  One supplied-snapshot/current-Config cone is combinational, but there is no
  raw-fault path, finality signal, or maintained precompute state.
EARLY_COMMIT_BEFORE_SA_TEST_DONE_SAFE: NO
ROLLBACK_REQUIRED_IF_LATE_FAULT: YES
ROLLBACK_SUPPORTED: NO
PROVISIONAL_SOLUTION_SUPPORTED: NOT_SUPPORTED
POST_BIST_EXPOSED_LATENCY_CAN_BE_ZERO: NO
CONCURRENCY_CLASS: COLLECTION_THEN_ANALYSIS

================================
GROUP_NO_SCRATCH_V2
================================

FAULT_STATE_UPDATE_DURING_BIST: NO
CANDIDATE_PRECOMPUTE_DURING_BIST: PARTIAL
  Current combinational snapshot evaluation is possible, but no live,
  all-config, invalidatable candidate map is supported before start_i.
FINAL_GROUP_DECISION_BEFORE_TEST_DONE_SAFE: NO
PROVISIONAL_SOLUTION_SUPPORTED: DERIVABLE
  Only for a caller-provided snapshot; not an explicit maintained core state.
POST_BIST_EXPOSED_LATENCY_CAN_BE_ZERO: NO
CONCURRENCY_CLASS: COLLECTION_THEN_ANALYSIS

================================
START_I INTERPRETATION
================================

ANALYZER_COMPUTE_TRIGGERED_BY_START: NO
ANALYZER_PRECOMPUTED_BEFORE_START: PARTIAL
  A current single Config cone is combinational; a complete GROUP candidate
  store/map is not precomputed.
POLICY_CONTROL_TRIGGERED_BY_START: YES

================================
E0-L REINTERPRETATION
================================

E0L_DSS_DECISION_LATENCY_CLASS:
POLICY_CONTROL_LATENCY_AFTER_PRECOMPUTED_ANALYZER

EARLY_4P6517_CYCLES_MEANS:
  Final-snapshot EARLY policy/ledger terminal latency, excluding BIST and
  candidate/matrix derivation.

GROUP_20P9871_CYCLES_MEANS:
  Final-snapshot 16-entry collection plus GROUP allocation terminal latency,
  excluding BIST and candidate/matrix derivation.

CURRENT_GROUP_POST_BIST_ARCH_CLAIM: VALID
  Valid only with its existing post-collection policy-control qualification.

================================
ADDRESS_1_TO_100 EXAMPLE
================================

LAST_FAULT: 15
TEST_DONE: 100

EARLY:
  state_at_15: No internal V2 raw-fault or provisional state; only an external
    prepared snapshot could drive one combinational current-Config result.
  precompute_before_100: PARTIAL, externally observable current evaluation only.
  provisional_ready_before_100: Not supported as an EARLY state.
  safe_to_commit_before_100: NO.
  work_remaining_at_100: Final snapshot presentation plus normal rank/ledger
    control; trace-dependent terminal cycles.

GROUP:
  state_at_15: No V2 raw-fault/candidate-map state before start_i.
  precompute_before_100: PARTIAL, externally observable one-Config evaluation;
    no stored all-config map.
  provisional_ready_before_100: DERIVABLE for an external snapshot only.
  safe_to_finalize_before_100: NO.
  work_remaining_at_100: Final snapshot, 16-entry collection, then
    trace-dependent A -> B -> C -> D allocation.

================================
POST_BIST EXPOSED LATENCY
================================

FORMAL_CANDIDATE_DEFINITION:
  FINAL_SOLUTION_RELEASE - GROUP_TEST_DONE, where FINAL_SOLUTION_RELEASE is a
  final-fault-qualified, externally accepted solution handoff.

ZERO_LATENCY_CASE_SUPPORTED: NO
REQUIRED_FUTURE_MODEL_CHANGE:
  Versioned incremental candidate production plus test-done finality and an
  explicit final-release/handoff event; EARLY additionally needs a safe
  no-rollback commitment rule or rollback architecture.

================================
MONOTONICITY
================================

CONFIG_FEASIBILITY_UNDER_NEW_FAULTS: MONOTONIC_DECREASING
  For fixed configuration and fixed accepted-fault order under the supported
  collector model.
PATTERN_REPRESENTATIVE_STABILITY: CAN_CHANGE

================================
SIMULATOR FUTURE WORK
================================

EARLY: POLICY_CONTROL_MODEL_EXTENSION
  A faithful early-commit overlap model needs finality/rollback or a changed
  safe-commit contract; timing reporting alone cannot supply it.

GROUP: NONFUNCTIONAL_TIMELINE_EXTENSION
  A model can retain policy semantics if it adds an incremental/versioned
  candidate producer and only releases final allocation at test completion.
  It must not be represented as a mere reporting field.

================================
RTL FUTURE WORK
================================

EARLY: RTL_CHANGE_REQUIRED
GROUP: RTL_CHANGE_REQUIRED

================================
RECOMMENDATION
================================

KEEP_CURRENT_E0L_POST_BIST_MODEL: YES
NEW_CONCURRENT_LATENCY_MODEL_REQUIRED: YES
E0R_MAY_RESUME: NO
NEXT_PHASE_AUTHORIZED: NONE
```

## Design consequence, without implementation

The available evidence does not authorize an inference that E0-L latency is
already hidden by BIST.  A future concurrent model must first specify the
fault-state producer, candidate-map version/invalidation rule, finality
qualification, and final release event.  For EARLY it must additionally solve
the semantic issue of an irreversible shared-resource ledger commit.  None of
those mechanisms is introduced by this audit.
