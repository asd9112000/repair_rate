# DATE 2026 — 2×2 DSS Latency and Device-Feasibility Audit

> Audit status: COMPLETE
>
> Target: 2×2 Directional CAM, `RS=2`, `CS=2`, `SHARE_M=1`; RECAM baseline,
> V2 EARLY, and frozen V2 GROUP-NoScratch.
>
> Boundary: read-only source, RTL, testbench, existing-result, and document
> audit.  No RTL, simulator, policy, instrumentation, synthesis, or repair-rate
> experiment was changed or run.

## Executive result

The repository has a rigorous **group-level DSS-decision** timestamp convention,
but not a complete SA-to-device timing chain.

- The accepted V2 RTL endpoint is the registered `done_o` edge.  It means
  `group_repairable`, selected ConfigID/PatternID, donor/release decision, and
  ledger state are final.  It explicitly excludes pivot/Hybrid reconstruction,
  repair-table encoding, eFuse programming, and runtime remap setup.
- The Phase 4I model supplies a **concurrent, last-fault-to-decision-ready**
  event contract for the frozen V2 decision boundary.  Its `T_last_fault` is a
  supplied fault-set-changing event, not an RTL fault handshake.
- The production V2 tops have no raw fault-stream, BIST-start, or BIST-end
  interface.  Consequently neither per-SA BIST lifecycle nor per-SA fault
  acceptance is represented in that RTL boundary.
- `HierarchicalRECAM` has an optional, group-relative A→B→C→D BIST/FIFO event
  timeline, but `DeviceRepairScheduler` does not place individual groups on a
  shared device wall clock.  Its per-engine `total_cycles` is accumulated work,
  not completion time.  No `t_solution_device` currently exists.
- The current device simulator exposes `SolutionTakePolicy::GroupCompressed`,
  not frozen V2 `GROUP_NO_SCRATCH_V2`.  The open S0R alignment work is therefore
  a hard policy-provenance blocker for any device-level DATE GROUP-NoScratch
  repairability or latency claim.

`DEVICE_LATENCY_FEASIBILITY = DEVICE_MODEL_PARTIAL` and
`DEVICE_DSS_REPAIRABILITY_READY = BLOCKED_BY_S0R`.

## Scope and evidence rules

Three similar-looking paths have deliberately different meanings and are not
interchanged in this audit:

| Path | What it establishes | What it does not establish |
|---|---|---|
| `rtl/dss_v2/` Phase 4E–4I | frozen V2 2×2 DSS decision/resource-management behavior and `start_i → done_o` decision cycles | fault collection, BIST lifecycle, repair reconstruction, device scheduling |
| `src/DssPostBistLatency.cpp` | a timing-only concurrent event model, checked against representative V2 RTL decision-ready edges | an integrated BIST/collector implementation |
| `HierarchicalRECAM` | device-wide CAM ownership and repairability; optional group-relative BIST/FIFO timing | device-wide wall-clock timing or V2 GROUP-NoScratch simulator semantics |

`docs/dss_execution/S0R_GROUP_SEMANTIC_ALIGNMENT_AUDIT.md` establishes that
the simulator's existing `GroupCompressed` performs cross-SA candidate-product
search and differs from frozen V2 GROUP-NoScratch's A→B→C→D immediate,
no-rollback allocation.  It is not used as a substitute below.

## BIST semantics

### SA test-start and test-done

```text
SA_TEST_START:
  status: IMPLICIT
  name: SerialBistSchedule::arrivalCycle() scan-index origin
  source: inc/FaultAddress.hpp; src/FaultAddress.cpp; docs/HIERARCHICAL_RECAM.md §9.2
  scope: optional HierarchicalRECAM group-relative FIFO timeline only
  semantics: The first address interval of SA S in a fixed serial A→B→C→D
             scan. No start event is stored, reported, or exposed per SA.

SA_TEST_DONE:
  status: IMPLICIT
  name: subarray boundary within SerialBistSchedule; completionCycle() is group-only
  source: src/FaultAddress.cpp; tests/fault_address_bist_test.cpp
  scope: optional HierarchicalRECAM group-relative FIFO timeline only
  semantics: The schedule has deterministic A/B/C/D scan boundaries, but only
             the completion of all four SAs is materialized as
             bistCompletionCycle. There is no test_done[SA] field or signal.
```

`SerialBistSchedule` defines
`scan_index=((SA×R+row)×Q)+word_col`, `arrival=(scan_index+1)×Tw`, and
`completion=4×R×Q×Tw`.  Therefore the modeled scan order is serial, not four
independent BIST engines.  The test fixes the A→B boundary and confirms that
the final D word equals the group completion edge
(`tests/fault_address_bist_test.cpp:52–65`).

```text
INDEPENDENT_SA_BIST_TIMING: NO
```

The Phase 4I V2 decision model is even narrower: `T_BIST_end` is a supplied
timestamp; it does not drive a DUT input.  The Phase 4I RTL-validation record
states explicitly that neither production V2 top exposes raw faults or
`bist_end_i` (`docs_verilog/PHASE4I_RTL_TIMING_VALIDATION.md`,
"Production-boundary stimulus mapping").

### Fault collection / acceptance

```text
FAULT_ACCEPT_EVENT:
  signal/event: NONE at the V2 production-top boundary
  source: rtl/dss_v2/top/recam_dss_v2_early_top.sv;
          rtl/dss_v2/top/recam_dss_v2_group_top.sv
  synchronous: N/A
  handshake: N/A; neither top has fault_valid/fault_ready inputs
  per-SA identifiable: NO

LAST_FAULT_EVENT:
  measurable: PARTIAL
  current_name: DssFaultSetEvent::changesFaultSet / lastFaultCycle; and,
                separately, BiraLatencyMetrics::lastFaultArrivalCycle
  source: inc/DssPostBistLatency.hpp; src/DssPostBistLatency.cpp:108–155;
          inc/BiraLatency.hpp; src/BiraLatency.cpp:241–268
  exact_semantics: last *fault-set-changing model event* in Phase 4I, or last
                   BIST arrival to the hierarchical FIFO. Neither is a
                   recorded per-fault FIFO-service acceptance/completion.
```

The old `rtl/dss_2x2/analyzer/shared_fault_collector.sv` does have
`fault_valid_i && fault_ready_o` (`accept`) and updates its collection state on
that acceptance.  It is historical/alternate `dss_2x2` collector evidence,
not the frozen V2 production top and not a legal source for assigning V2
latency semantics.  The C++ RECAM implementation is also batch-oriented:
`RECAM_PE::loadFaultsToCAMs()` copies an already-classified `FaultList` into
Address/Hybrid/Buffer CAMs (`src/RECAM_PE.cpp:5–20`), so it does not expose a
clocked per-fault accept event either.

For the optional hierarchical timeline, an arrival is enqueued conceptually
at `arrivalCycle`; `simulateDecoupledFifo()` computes service start and
collection completion locally, but exports only aggregate
`faultCollectionCompletionCycle`, not `t_fault_accept(SA,fault_n)`.
An arrival address or the last address presented to BIST must therefore **not**
be relabeled as a repair-hardware accept event.

## Analysis behavior

### RECAM baseline

```text
classification: OTHER
during_fault_collection: The RECAM specification describes concurrent fault
                         collection and repair analysis; an accepted fault
                         updates Address/Hybrid/must/buffer state.
post_test_work: The paper-semantic baseline reports zero additional analysis
                cycles, but this repository's C++ implementation is batch
                collection followed by matrix/solution calls, and its active
                analyzer RTL starts from already-collected state.
explicit_analysis_start: NO hardware event represented in the current
                         analyzer-only RTL or C++ API.
```

`docs/RECAM_SPEC.md` §3 calls RECAM dynamic BIRA and says collection and
analysis proceed concurrently; §18 cautions that C++ loops are not hardware
cycles and preserves the paper statement of zero *additional* repair-analysis
cycles.  This is a semantic RECAM claim, not evidence that this repository has
an integrated cycle-accurate collector-to-analyzer RTL implementation.

`rtl/recam/recam_2r2c_analyzer.sv` and the shared analyzer used by V2 consume
stored pivot/Hybrid/must/overflow inputs.  They have no fault interface or
test-done event.  Thus the source supports `FULLY_CONCURRENT` only as the
RECAM architecture specification; its concrete analysis RTL boundary is
post-collection combinational state.

### V2 EARLY

```text
classification: CONCURRENT_COLLECTION_AND_PREPROCESSING_WITH_POST_TEST_FINAL_DECISION
                in the Phase 4I timing model; OTHER in production RTL alone
during_fault_collection: Each fault-set-changing DssFaultSetEvent invalidates
                         and restarts the modelled decision attempt.
post_test_work: Remaining first-feasible candidate/ledger work to registered
                done_o; no reconstruction work is included.
explicit_analysis_start: No production signal. The event model starts/restarts
                         at each supplied changing event; RTL starts only at
                         start_i with stable, pre-collected analyzer inputs.
```

The model's exact rule is in `src/DssPostBistLatency.cpp:108–110`: each
changing event launches an attempt and only the attempt at the last changing
event can be observable.  Its frozen execution work is the sum of active-SA
candidate evaluations.  The production core then accepts the first feasible
candidate, clocks the ledger commit, and advances A→B→C→D
(`rtl/dss_v2/top/recam_dss_v2_early_core.sv:25–31`).

1. Every *modelled changing* fault restarts decision work; the production V2
   top itself has no incoming fault to modify state.
2. In the model, only decision-attempt validity changes while BIST runs.  In
   the RTL decision run, selected state and ledger change on accepted
   candidates.
3. Candidate feasibility is combinational for the current ConfigID; no
   candidate map is retained by EARLY.
4. `T_BIST_end` causes no computation in RTL; it only changes the reported
   `L_post` subtraction.
5. There is no `analysis_start` signal.
6. The event model treats analysis as continuously restartable; the RTL is
   active only after `start_i`.
7. The observable final decision phase ends at registered `done_o`.

### V2 GROUP-NoScratch

```text
classification: CONCURRENT_COLLECTION_AND_PREPROCESSING_WITH_POST_TEST_FINAL_DECISION
                in the Phase 4I model; OTHER in production RTL alone
during_fault_collection: The timing model restarts after each changing event.
                         The RTL GROUP COLLECT state subsequently samples all
                         16 already-derived candidate results.
post_test_work: 16-cycle candidate collection plus sequential A→B→C→D
                feasibility/ledger allocation, ending at done_o.
explicit_analysis_start: No fault-driven production signal; start_i begins
                         COLLECT after the input collector state is stable.
```

The GROUP core writes one `{valid, PatternID}` record per canonical `[SA][slot]`
while `state_q==COLLECT`, then enters `ALLOCATE` only after SA D slot 3
(`rtl/dss_v2/top/recam_dss_v2_group_core.sv:35–50`).  The Phase 4I work model
faithfully represents this as 16 collection cycles plus active-SA allocation
evaluations (`src/DssPostBistLatency.cpp`, `decisionExecutionCycles`).  It is
not raw BIST fault collection.

## Solution events

### EARLY SA solution

```text
EARLY_SA_SOLUTION_EVENT:
  exists: YES, for the decision/resource-management boundary
  signal/event: rising edge that sets sa_commit_valid_o[i]
  source: rtl/dss_v2/top/recam_dss_v2_early_core.sv:31
  ConfigID_final: YES; selected_config_flat_o[i]
  PatternID_final: YES; selected_pattern_flat_o[i]
  selected repair-address information: NO
  externally_usable: PARTIAL; irrevocable under frozen no-rollback traversal,
                     but not a completed repair table
  additional_work_after_event: remaining SAs and, outside this boundary,
                               pivot/Hybrid reconstruction and repair-table
                               encoding
```

EARLY's ledger transaction and selected ConfigID/PatternID are committed on
the same registered edge.  Later SAs cannot alter that selected record under
the frozen no-rollback policy.  `done_o` remains the only group outcome event;
on a later failure, prior SA commits remain diagnostic state but there is no
repairable four-SA group.

### GROUP candidate readiness and finality

```text
GROUP_SA_CANDIDATE_READY_EVENT:
  exists: IMPLICIT_INTERNAL
  signal/event: COLLECT edge that writes slot 3 for a given SA into
                dss_v2_group_candidate_store
  source: rtl/dss_v2/top/recam_dss_v2_group_core.sv:35–37,49–50
  semantics: all four local canonical {valid, PatternID} records for that SA
             have been stored; this is candidate availability only.

GROUP_SA_FINAL_SOLUTION_EVENT:
  exists: YES internally; group-usability is deferred
  signal/event: ALLOCATE edge that sets sa_commit_valid_o[i]
  source: rtl/dss_v2/top/recam_dss_v2_group_core.sv:51
  ConfigID_final: YES internally, from selected_config_flat_o[i]
  PatternID_final: YES internally, from selected_pattern_flat_o[i]
  externally_usable: Only with a later successful group done_o. The design
                     has no rollback, but a prior commit is not a usable
                     four-SA repair outcome if a later SA fails.
  additional_work_after_event: later SA allocation; reconstruction remains
                               outside the production boundary.

ALL_FOUR_FINAL_AT_GROUP_DECISION:
  YES for the externally usable four-SA GROUP solution: success is fixed only
  at done_o after D commits. Internally, the four irrevocable SA commit edges
  occur sequentially and do not coincide.
```

The required distinction is material: candidate-store completion is not final
selection, and an internal early SA commit is not proof that the entire shared
resource group repairs successfully.

```text
EARLY_GROUP_SOLUTION_EVENT:
  signal/event: done_o rising edge with group_repairable_o=1, simultaneous
                with SA D's final commit; max of the four committed SA
                decision events for a successful run.
  source: rtl/dss_v2/top/recam_dss_v2_early_core.sv:31.

GROUP_GROUP_SOLUTION_EVENT:
  signal/event: done_o rising edge with group_repairable_o=1, after COLLECT
                and the final D ALLOCATE commit.
  source: rtl/dss_v2/top/recam_dss_v2_group_core.sv:49–51.

GROUP_COMPLETION_EQUAL_MAX_SA_COMPLETION:
  EARLY: YES for successful four-SA decision completion.
  GROUP: YES for successful externally usable group completion; the internal
         candidate-ready and earlier SA-commit edges are not substitutes.
```

### Device solution

```text
DEVICE_SOLUTION:
  exists: NO timing event
  current functional outcome: DeviceRepairResult::deviceRepairSuccess after
                              DeviceRepairScheduler::run() returns
  source: inc/HierarchicalRecamSimulator.hpp; src/HierarchicalRecamSimulator.cpp:788–1000
  semantics: all supplied groups' repairability aggregate; no timestamp and
             no max over group completion times.
```

## Level semantics

```text
SA_COMPLETION_DEFINED: PARTIAL
GROUP_COMPLETION_DEFINED: YES for V2 DSS decision-ready only; PARTIAL for
                          complete repair-table usability
DEVICE_COMPLETION_DEFINED: NO
```

The exact boundary is important: Phase 4I calls `done_o` **decision ready**,
not repair reconstruction.  The V2 characterization explicitly excludes final
pivot/Hybrid reconstruction, eFuse programming, and runtime setup
(`docs_verilog/PHASE4I_FINAL_CHARACTERIZATION.md`, scope and frozen metric).

## Latency measurability

### Strict proposed quantities

| Concept | Current support | Reason |
|---|---|---|
| `t_solution_sa(i)-t_last_fault_accept_sa(i)` | NO | SA commit exists in V2 decision RTL, but there is no per-SA fault accept or SA BIST done. |
| `t_solution_group-max_i(t_last_fault_accept_sa(i))` | PARTIAL | Phase 4I has group decision-ready and a supplied last changing event; hierarchical timing has last arrival, not acceptance; neither connects to frozen V2 policy end-to-end. |
| `t_solution_device-max_all_SA(t_last_fault_accept_sa)` | NO | no device wall-clock completion and no per-fault accepts. |
| `t_solution_sa(i)-t_test_done_sa(i)` | NO | no materialized `test_done[SA]`. |
| `t_solution_group-max_i(t_test_done_sa(i))` | PARTIAL | a four-SA serial group BIST completion and group-relative solution-ready exist in the hierarchical FIFO model, but not for V2 GROUP-NoScratch and not per-SA. |
| `t_solution_device-max_all_SA(t_test_done_sa)` | NO | device timeline and per-SA done fields are missing. |

```text
SA_FAULT_TAIL: NO
GROUP_FAULT_TAIL: PARTIAL
DEVICE_FAULT_TAIL: NO

SA_POST_BIST: NO
GROUP_POST_BIST: PARTIAL
DEVICE_POST_BIST: NO
```

The existing Phase 4I quantity is a valid, narrower group-decision candidate:

```text
T_last_fault     = last supplied fault-set-changing event
T_BIST_end       = supplied BIST completion edge
T_decision_ready = V2 done_o decision boundary
L_last_fault     = T_decision_ready - T_last_fault
L_post           = max(0, T_decision_ready - T_BIST_end)
```

This is source-anchored in `docs_verilog/PHASE4I_EVENT_MODEL.md` and
`src/DssPostBistLatency.cpp:138–155`; it is not yet the requested physical
`fault_accept` metric.  The RTL validation confirms the endpoint and edge
convention for representative cases, not BIST integration.

## Analysis-cycle terminology and provenance

```text
RECAM_ANALYSIS_CYCLES:
  existing_value: 0 additional repair-analysis cycles (paper-semantic claim);
                  bira_repair_analysis_work_cycles may also be nonzero in the
                  C++ work proxy.
  scope: RECAM architecture semantics / C++ aggregate work, respectively
  provenance: docs/RECAM_SPEC.md §3 and §18; inc/BiraLatency.hpp

EARLY_ANALYSIS_CYCLES:
  existing_value: representative 4 cycles from accepted start_i to done_o;
                  data dependent by candidate depth
  scope: one V2 2×2 group DSS decision/resource-management boundary
  provenance: docs_verilog/PHASE4H_UNIFIED_HARDWARE_CHARACTERIZATION.md;
              docs_verilog/PHASE4I_FINAL_CHARACTERIZATION.md

GROUP_ANALYSIS_CYCLES:
  existing_value: 20–26 cycles from accepted start_i to done_o;
                  16 COLLECT cycles plus sequential allocation evaluations
  scope: one V2 2×2 group DSS decision/resource-management boundary
  provenance: docs_verilog/PHASE4F_CHARACTERIZATION.md; src/DssPostBistLatency.cpp
```

`bira_fault_collection_work_cycles`, `bira_repair_analysis_work_cycles`,
`bira_sharing_allocation_work_cycles`, and `bira_total_work_cycles` are
technology-neutral **operation-work accounting**.  They are not RTL critical
delay, not V2 `start_i → done_o` decision cycles, and not device wall-clock
cycles.  `BiraLatencyMetrics` makes this explicit with separate
`AggregateWorkOnly` and `EventDrivenQueue` modes (`inc/BiraLatency.hpp`).

## Critical delay

Existing accepted 2×2 technology-mapped observations are:

| Design | Existing critical delay | Source / scope |
|---|---:|---|
| RECAM baseline | 20.00 ns | corrected Phase 3A combinational analyzer only |
| V2 EARLY | 19.70 ns | complete V2 EARLY decision/resource-management top |
| V2 GROUP-NoScratch | 19.74 ns | complete V2 GROUP-NoScratch decision/resource-management top |

The common technology setup is Synopsys Design Compiler W-2024.09-SP2 with
TSMC018 `slow.db`, slow corner, `tsmc18_wl10`, a 20.0-ns `clk_i` constraint,
0.1-ns uncertainty, 0 input/output delay, 0.5 input transition, 0.05 library
unit output load, and `rst_ni` false path.  No memory macro is inferred or
linked.  Authority: `docs_verilog/TSMC018_SYNTHESIS_FLOW.md` and
`docs_verilog/PHASE3_SYNTHESIS_27BIT.md`.

```text
CRITICAL_DELAY:
  RECAM: 20.00 ns (Phase 3A analyzer-only, WNS 0.00 ns)
  EARLY: 19.70 ns (V2 decision top, WNS +0.01 ns)
  GROUP: 19.74 ns (V2 decision top, WNS +0.00 ns)
  DIRECTLY_COMPARABLE: PARTIAL
```

They share technology, corner, clock, and I/O assumptions, so the existing
numbers are valid mapped-block observations.  They are not a fully symmetric
three-way architecture-delay comparison: RECAM Phase 3A excludes collector,
scheduling, and policy controller, whereas EARLY/GROUP include their V2
decision/resource-management controllers.  None is a BIST-to-final-repair
latency.

## Device feasibility

### Current modeled device and scheduling

The reference hierarchy is 8 domains × 4 banks/domain × 64 repair groups/bank
= **2,048 repair groups/device**, each with four SAs
(`docs/HIERARCHICAL_RECAM.md` §2).  `--groups N` may deliberately model only a
subset; it does not imply a full 2,048-group device result.

`DeviceRepairScheduler::run()` iterates the supplied `groups` vector in input
order and assigns an engine by `groupIndex % biraEngineCount`
(`src/HierarchicalRecamSimulator.cpp:787–814`).  This is a functional
sequential loop plus static engine attribution, not an event schedule with
group start/end timestamps.  Per-engine `totalCycles` is incremented by each
group's aggregate work (`:933–937`).

The optional BIST/FIFO model is only group-relative: its schedule begins at
the group's origin and `applyDecoupledFifoTimeline()` records group fields.
`docs/HIERARCHICAL_RECAM.md` §10.2 and `docs/ARCHITECTURE.md` §10 explicitly
state that this timeline has no device-wide wall-clock backlog across groups
sharing an engine.

```text
GROUP_TIMING_PROPAGATED_TO_DEVICE: NO
DEVICE_MODEL_TYPE: OTHER (group-relative event timeline plus device-level
                   repairability and aggregate-work accounting)
GLOBAL_CAM_TIMING_MODELED: NO
```

### Global CAM and Tier-2

`GlobalOnlineRepairPool` is genuinely persistent for one scheduler/device:
one pool is constructed before the group loop, `reserve()` is invoked for
Tier-2 mappings, and pool occupancy is not cleared after a group
(`src/HierarchicalRecamSimulator.cpp:776–780, 860–923`).  Scratch is separately
cleared per engine/group only after work accounting (`:933–937`).  This proves
the required ownership semantics, but allocation itself has no cycle/time
cost; it is a synchronous simulator operation and contributes no global-CAM
event or delay field.

There is additional functional Tier-2 processing after a group Tier-0/Tier-1
failure: run Tier-2 analysis, derive/deduplicate mappings, reserve the global
pool transactionally, then commit persistent tags only on success.  It is
accounted as functional repairability/work, not as device timeline time.

```text
DEVICE_DSS_REPAIRABILITY_READY: BLOCKED_BY_S0R
DEVICE_LATENCY_FEASIBILITY: DEVICE_MODEL_PARTIAL
DEVICE_LATENCY_REQUIRED_CHANGE: SCHEDULER_TIMING_MODEL_EXTENSION
```

The first status is policy-specific: hierarchy/CAM ownership is ready, but
`SimulationConfig` and `HierarchicalRECAM` currently parse only `legacy`,
`early`, and `group`, where `group` maps to `GroupCompressed`
(`inc/SimulationConfig.hpp`; `src/SimulationConfig.cpp:282–290`; and
`HierarchicalRECAM.cpp:131–140`).  The S0R audit records the mismatch and the
needed explicit V2 policy.  No claim of device GROUP-NoScratch readiness is
valid until that policy exists and is equivalence-checked in the simulator.

## Terminology search

```text
FAULT_TAIL_EXISTING_NAME: NONE
POST_BIST_EXISTING_NAME: latency_after_bist_cycles / L_post
SA_COMPLETION_EXISTING_NAME: NONE
GROUP_COMPLETION_EXISTING_NAME: solution_ready_cycle (hierarchical FIFO model)
                                 and done_o (V2 DSS decision boundary)
DEVICE_COMPLETION_EXISTING_NAME: NONE; total_bira_completion_cycles is a
                                  work-only completion proxy, not an event
```

`latency_after_bist_cycles` in the hierarchical CSV has the explicit meaning
`max(0, solution_ready_cycle - bist_completion_cycle)`, group-relative only.
The Phase 4I `L_post` uses the same subtraction shape but a different frozen
V2 decision work plan.  They must remain separately named/provenanced until an
integrated policy-compatible model exists.

## Recommended final event definitions

No metric name needs changing now.  The following are recommendations for the
later instrumentation/timing-model phase; they are not current implementation
claims.

| Quantity | Recommended start | Recommended end | Current gap |
|---|---|---|---|
| SA fault-tail | recorded `fault_accept[SA][n]`: successful collector enqueue/service acceptance | final per-SA repair-table-ready event | needs explicit accept, per-SA lifecycle, and reconstruction endpoint |
| Group fault-tail | max accepted-fault time across four SAs | successful group repair-table-ready event, after policy decision and Tier-2 commit when applicable | V2 has decision `done_o`; hierarchy has group `solution_ready_cycle`; neither is the integrated full endpoint |
| Device fault-tail | max accepted-fault time across all modeled SAs | device schedule's final group/Tier-2 persistent-commit event | needs device engine schedule and global-CAM timing |
| SA post-BIST | explicit `test_done[SA]` | same SA repair-table-ready event | no SA done or endpoint |
| Group post-BIST | explicit `test_done_group` after D scan | group repair-table-ready event | group BIST end exists only in optional hierarchy timeline; V2 policy integration missing |
| Device post-BIST | final BIST completion across modeled SAs/groups under a specified scheduler | device completion event | neither device BIST schedule nor completion event exists |

For the already accepted narrow V2 study, preserve the exact Phase 4I names
and endpoints rather than broadening them: `T_last_fault`, `T_BIST_end`,
`T_decision_ready=done_o`, and `L_post`.  State in every result that it is
**post-BIST DSS decision latency**, not repair-table latency or device latency.

## Required minimum next step

```text
LATENCY_EXPERIMENT_READY: PARTIAL
DEVICE_EXPERIMENT_READY: BLOCKED_BY_S0R
REQUIRED_MINIMUM_CHANGE:
  1. Finish S0R with explicit GROUP_NO_SCRATCH_V2 simulator policy and
     equivalence evidence. Do not substitute GroupCompressed.
  2. Add nonfunctional timing metadata for per-SA BIST start/done and actual
     collector acceptance, retaining the current fault-address contract.
  3. Define and expose a repair-table-ready/reconstruction completion boundary
     separately from V2 done_o.
  4. Extend DeviceRepairScheduler with an explicit per-engine event schedule,
     group start/end times, and a specified Tier-2/global-CAM allocation-time
     policy before calculating any device maximum/completion latency.
  5. Only then bind the existing Phase 4I decision model or its successor to
     the integrated SA→group→device trace and run a separately authorized
     experiment.
```

## Requested audit summary

```text
AUDIT_STATUS: COMPLETE
SOURCE_MODIFIED: NO
DOCUMENT_CREATED: docs/dss_execution/DATE_2X2_DSS_LATENCY_AND_DEVICE_FEASIBILITY_AUDIT.md
TARGET: 2x2 Directional CAM RS=2 CS=2 m=1

--------------------------------
BIST SEMANTICS
--------------------------------
SA_TEST_START:
  status: IMPLICIT
  name: SerialBistSchedule scan-index origin
  source: src/FaultAddress.cpp
  semantics: serial A→B→C→D schedule; no emitted per-SA event

SA_TEST_DONE:
  status: IMPLICIT
  name: serial scan boundary; only completionCycle() is materialized
  source: src/FaultAddress.cpp
  semantics: four-SA group BIST completion, not per-SA done

INDEPENDENT_SA_BIST_TIMING: NO

--------------------------------
FAULT COLLECTION
--------------------------------
FAULT_ACCEPT_EVENT:
  V2 production top: NOT_DEFINED
  hierarchical timeline: BIST arrival exists; collector acceptance is not exported

LAST_FAULT_EVENT:
  measurable: PARTIAL
  current_name: lastFaultCycle / lastFaultArrivalCycle
  exact_semantics: last changing model event or last BIST arrival, not accept

--------------------------------
ANALYSIS BEHAVIOR
--------------------------------
RECAM:
  classification: OTHER (specification is concurrent; source RTL boundary is post-collection)
  during_fault_collection: semantic Address/Hybrid/must/buffer updates
  post_test_work: zero additional cycles is paper-semantic; no integrated RTL event
  explicit_analysis_start: NO

EARLY:
  classification: CONCURRENT_COLLECTION_AND_PREPROCESSING_WITH_POST_TEST_FINAL_DECISION
  during_fault_collection: Phase 4I restarts a decision attempt per changing event
  post_test_work: remaining candidate/ledger work to done_o
  explicit_analysis_start: NO

GROUP:
  classification: CONCURRENT_COLLECTION_AND_PREPROCESSING_WITH_POST_TEST_FINAL_DECISION
  during_fault_collection: Phase 4I restarts a decision attempt per changing event
  post_test_work: 16 candidate-store cycles plus allocation to done_o
  explicit_analysis_start: NO

--------------------------------
SOLUTION EVENTS
--------------------------------
EARLY_SA_SOLUTION: sa_commit_valid_o[i]; ConfigID/PatternID and ledger final, reconstruction absent
GROUP_SA_CANDIDATE_READY: internal write of fourth canonical slot for SA i
GROUP_SA_FINAL_SOLUTION: internal sa_commit_valid_o[i]; externally usable only after successful group done_o
EARLY_GROUP_SOLUTION: successful done_o, equal max committed-SA decision edge
GROUP_GROUP_SOLUTION: successful done_o after collection and final allocation
DEVICE_SOLUTION: NOT_DEFINED as a timing event

--------------------------------
LEVEL SEMANTICS
--------------------------------
SA_COMPLETION_DEFINED: PARTIAL
GROUP_COMPLETION_DEFINED: YES for decision-ready; PARTIAL for repair-table-ready
DEVICE_COMPLETION_DEFINED: NO

--------------------------------
LATENCY MEASURABILITY
--------------------------------
SA_FAULT_TAIL: NO
GROUP_FAULT_TAIL: PARTIAL
DEVICE_FAULT_TAIL: NO
SA_POST_BIST: NO
GROUP_POST_BIST: PARTIAL
DEVICE_POST_BIST: NO

--------------------------------
ANALYSIS CYCLES
--------------------------------
RECAM_ANALYSIS_CYCLES:
  existing_value: 0 additional paper-semantic cycles; C++ work proxy separate
  scope: RECAM semantic / aggregate work
  provenance: docs/RECAM_SPEC.md
EARLY_ANALYSIS_CYCLES:
  existing_value: representative 4 start_i→done_o cycles
  scope: V2 group decision boundary
  provenance: Phase 4H/4I
GROUP_ANALYSIS_CYCLES:
  existing_value: 20–26 start_i→done_o cycles
  scope: V2 group decision boundary
  provenance: Phase 4F/4I

--------------------------------
CRITICAL DELAY
--------------------------------
RECAM: 20.00 ns, analyzer-only
EARLY: 19.70 ns, V2 decision top
GROUP: 19.74 ns, V2 decision top
DIRECTLY_COMPARABLE: PARTIAL

--------------------------------
DEVICE FEASIBILITY
--------------------------------
DEVICE_MODEL_TYPE: OTHER (group event timeline + repairability/work model)
GROUP_TIMING_PROPAGATED_TO_DEVICE: NO
GLOBAL_CAM_TIMING_MODELED: NO
DEVICE_DSS_REPAIRABILITY_READY: BLOCKED_BY_S0R
DEVICE_LATENCY_FEASIBILITY: DEVICE_MODEL_PARTIAL
DEVICE_LATENCY_REQUIRED_CHANGE: SCHEDULER_TIMING_MODEL_EXTENSION

--------------------------------
TERMINOLOGY
--------------------------------
FAULT_TAIL_EXISTING_NAME: NONE
POST_BIST_EXISTING_NAME: latency_after_bist_cycles / L_post
SA_COMPLETION_EXISTING_NAME: NONE
GROUP_COMPLETION_EXISTING_NAME: solution_ready_cycle / done_o
DEVICE_COMPLETION_EXISTING_NAME: NONE

--------------------------------
AMBIGUITIES
--------------------------------
1. Phase 4I uses a supplied changing-fault event, whereas the hierarchy exposes BIST arrivals, not accepts.
2. V2 done_o is decision-ready, not repair-table-ready.
3. The hierarchy's group-relative timeline cannot be composed into device time without an engine schedule.
4. Current simulator GROUP is historical GroupCompressed, not frozen V2 GROUP-NoScratch.

--------------------------------
RECOMMENDED_FINAL_EVENT_DEFINITIONS
--------------------------------
Use explicit collector accept, explicit SA/group BIST done, repair-table-ready,
and device scheduler completion events as detailed in the recommendation table.
Preserve Phase 4I's existing decision-ready nomenclature for its narrower scope.

--------------------------------
NEXT_STEP
--------------------------------
LATENCY_EXPERIMENT_READY: PARTIAL
DEVICE_EXPERIMENT_READY: BLOCKED_BY_S0R
REQUIRED_MINIMUM_CHANGE: S0R policy completion, then nonfunctional SA/group timing
metadata and a device scheduler timing-model extension; no experiment yet.

STOP.
```

## T0 address-geometry reconciliation addendum

> Addendum status: COMPLETE — read-only reconciliation.  It authorizes no
> timing-model, simulator, collector, or RTL change.

### Authority and active scopes

The following sources establish the values but not a universal identity between
every field named "column":

| Priority | Source | Symbol / statement | Value | Scope | Active |
| --- | --- | --- | ---: | --- | --- |
| 1 | `rtl/dss_v2/common/dss_v2_params_pkg.sv` | `ROW_ADDR_W`, `COL_ADDR_W`, `DIFF_ADDR_W` | 9, 5, 9 | frozen V2 analyzer package | Yes |
| 1 | `rtl/dss_v2/top/recam_dss_v2_{early,group}_top.sv` | default parameters and `pivot_*_flat_i` port widths | 9-bit row, 5-bit column | frozen V2 production analyzer boundary | Yes |
| 2 | `docs_verilog/PHASE3B_ANALYZER_INTERFACE.md` | frozen Domain+Bank+Group+SA+Row+ColumnWord address | 3+2+6+2+9+5 = 27 | frozen analyzer/interface and reusable-CAM accounting | Yes, as interface authority |
| 3 | `inc/FaultAddress.hpp` | `FaultAddressGeometry` defaults | rows=512, cellColumns=8192, wordBits=256 | C++ BIST packet and serial schedule | Yes |
| 3 | `tests/e0l_generate_candidate_corpus.cpp` | `frozenConfig()` | 512 rows, 8192 cell columns, 256-bit data word | E0-L/S1D timing corpus | Yes |
| 4 | `docs/HIERARCHICAL_RECAM.md` §2.1/§5 | data-word `word_col=floor(cell_col/256)` | 32 word columns, 27-bit tag | Hierarchical-RECAM global CAM model | Yes for that simulator |
| 4 | `docs_verilog/PHASE3B_ANALYZER_INTERFACE.md` | `WORD_W=16` online replacement word | 16 bits | reusable-CAM payload, not an analyzer input | Yes, distinct scope |

The analyzer-specific `ColumnWord[4:0]` is explicitly packed into a 9-bit
Hybrid differing-address field by zero extension.  This confirms its
five-bit representation, but the frozen V2 production tops consume retained
`pivot_cols_flat_i`; they do not consume a raw physical fault or a BIST word
address.

### C++ fault and BIST address domains

`Fault` in `inc/Fault.hpp` stores `r` and `c`.  The associated loader,
RECAM CAM, and analyzer paths use `c` directly as a physical cell-column
coordinate.  `HierarchicalRECAM.cpp` writes the resolved corpus with the
explicit header `Row CellCol`; `FaultAddress.cpp` validates that coordinate
against `FaultAddressGeometry::cellColumns`.

The separate BIST-facing type is `BistWordAddress7`:

```text
physical Fault(r, c)
  -> BistWordAddress7(row=r, wordColumn=floor(c / wordBits))
  -> SerialBistSchedule scan address
```

For the current S1D/E0-L configuration, `wordBits=256`, so
`8192 / 256 = 32` and the BIST `wordColumn` has an effective five-bit range.
The schedule does not reinterpret or truncate the stored `Fault::c`; it
derives the word column at `arrivalCycle()`.

```text
CPP_ROW_FIELD: Fault::r
CPP_COL_FIELD: Fault::c
CPP_ROW_BITS: 9 (validated 0..511 in the frozen timing recipe)
CPP_COL_BITS: 13 (validated 0..8191 physical cell column)
CPP_BIST_COL_FIELD: BistWordAddress7::wordColumn = Fault::c / wordBits
CPP_BIST_COL_BITS: 5 for the current 8192/256 recipe
RTL_ROW_BITS: 9
RTL_COL_BITS: 5
CPP_RTL_ADDRESS_DOMAIN_MATCH: NOT_PROVEN
ADDRESS_DOMAIN_MISMATCH: YES
```

The direct mismatch is real: `Fault::c` has 13 physical-cell bits whereas the
RTL analyzer port has five ColumnWord bits.  The numerical equality of the
current C++ BIST range (32) and the frozen RTL range (32) is insufficient to
prove semantic identity because the Phase-3B document separately names a
16-bit online replacement payload, while S1D/E0-L and the hierarchical BIST
schedule use 256-bit data words.  No V2 production collector bridge maps a
physical C++ cell fault into `pivot_cols_flat_i`.

### Existing timing scan and integration boundary

The existing serial BIST scan is already defined, without a new timing index:

```text
words_per_row = cellColumns / wordBits
scan_index = ((subarray_id * rows + Fault::r) * words_per_row)
             + floor(Fault::c / wordBits)
arrival_cycle = group_start_cycle + (scan_index + 1) * cycles_per_word
```

For the frozen current recipe this gives 512 rows and 32 BIST word addresses
per row, but those are currently supplied by `FaultAddressGeometry` and the
E0-L `SimulationConfig`, not imported from the RTL parameter package.  The
existing C++ shared geometry is therefore `FaultAddressGeometry`; no second
`DramAddressGeometry`, duplicate constants, assertions, or configuration
metadata were added by this audit.

```text
RTL_ROW_ADDR_W: 9
RTL_COL_ADDR_W: 5
RTL_LOGICAL_WORD_BITS: 16 (Phase-3B online replacement payload)
RTL_PHYSICAL_PAGE_BITS: 8192 (physical cell columns per modeled row)
CPP_ROW_ADDR_W: 9 (current timing recipe)
CPP_COL_ADDR_W: 13 physical / 5 derived BIST-word bits at wordBits=256
CPP_FAULT_COL_SEMANTICS: physical cell column; BIST derives wordColumn
RTL_CPP_ADDRESS_DOMAIN_MATCH: NOT_PROVEN
TIMING_MODEL_ROW_ADDR_W: 9 (current schedule)
TIMING_MODEL_COL_ADDR_W: 5 effective BIST-word bits; not proven to be the V2 analyzer ColumnWord contract
TIMING_SCAN_INDEX: ((SA * rows + row) * (cellColumns / wordBits)) + floor(cell_col / wordBits)
```

### Gate and stop condition

This is a read-only RTL/C++ evidence audit.  No RTL source was generated or
modified, so the readable-RTL public review matrix is:

```text
compile: NOT_RUN
ast: NOT_RUN
readability: NOT_RUN
comment: NOT_APPLICABLE
naming: NOT_RUN
profile: NOT_RUN
testbench: NOT_RUN
toolchain: NOT_RUN
```

No timing index or C++ geometry refactor is authorized from this evidence.
Before formal timing integration, an explicitly reviewed conversion contract
must identify whether the V2 analyzer `ColumnWord[4:0]` denotes the same
`floor(cell_col/256)` unit used by the S1D BIST schedule, or a different
five-bit repair-analysis abstraction.  It must also state how the Phase-3B
16-bit replacement payload relates to that unit.  Until then, do not force
the C++ simulator's physical `Fault::c` into five bits.

## T0B ColumnWord / BIST-word reconciliation

> T0B status: COMPLETE — read-only. Detailed evidence is in
> [T0B_V2_COLUMNWORD_BIST_WORD_SEMANTIC_RECONCILIATION.md](T0B_V2_COLUMNWORD_BIST_WORD_SEMANTIC_RECONCILIATION.md).

T0B traced the frozen analyzer consumer and the newer retained
collector-to-analyzer view. The analyzer treats `ColumnWord[4:0]` as a
zero-extended, equality-compared dictionary coordinate. It has no BIST bus or
raw physical-fault port. The retained path visibly projects
`pivot_col[9:0]` to `[4:0]`; it does not implement `/256`.

The physical/BIST mapping remains independently explicit:
`BistWordAddress7::wordColumn = floor(Fault::c / 256)`. No bridge ties the
C++ 13-bit physical column to either V2 producer. Hence T0B selects Branch C:
the BIST timing domain is independently usable only after separate
authorization, while no V2/BIST ColumnWord equality or repair mapper is
authorized from this result.
