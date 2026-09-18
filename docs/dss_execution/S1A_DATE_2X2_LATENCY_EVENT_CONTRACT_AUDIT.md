# S1A — DATE 2×2 DSS Latency Event Contract Audit

> Status: COMPLETE (read-only evidence audit)
>
> Target: 2×2 Directional CAM, RS=2, CS=2, SHARE_M=1; RECAM baseline,
> V2 EARLY, and V2 GROUP_NO_SCRATCH_V2.
>
> Boundary: No RTL, testbench, C++ simulator, scheduler, timing model,
> instrumentation, policy, or experiment was modified or run.

## Scope and evidence boundary

This audit distinguishes three source boundaries that cannot be silently
combined into a single event trace:

| Boundary | What it contains | What it does not contain |
|---|---|---|
| Frozen V2 production decision tops | `start_i`, candidate evaluation, ledger commit, selected ConfigID/PatternID, and `done_o` | raw fault stream, BIST start/end, per-SA test lifecycle, or reconstruction completion |
| Historical/alternate CAM collector RTL | `fault_valid_i && fault_ready_o` storage acceptance, with a subarray tag | the frozen V2 GROUP-NoScratch production decision boundary |
| C++ DynamicSpareSharing | batch `FaultGroup` analysis and returned result/trace | a clocked BIST or per-fault acceptance event |

The Phase-4I timing harness is observation-only. Its `T_last_fault` and
`T_BIST_end` are testbench/model timestamps; they are not production-top ports
or collector handshakes. This is confirmed in
`docs_verilog/PHASE4I_RTL_TIMING_VALIDATION.md` and by
`tb/dss_v2/recam_dss_v2_phase4i_rtl_timing_top.sv:5-7`.

### RTL-review public gate state

This is an `analyze` task under the RTL review workflow. No generated or
modified RTL exists, so no quality gate was run.

```text
compile/readability/comment/naming/profile/testbench/toolchain = NOT_RUN
ast = SOURCE_REVIEWED
```

## Frozen event interpretation

`SA_DECISION_READY` means a terminal DSS decision at the decision/resource
management boundary. It does not mean repair-address reconstruction,
repair-table programming, eFuse programming, or runtime-remap readiness.

For a successful V2 SA, the assertion edge of `sa_commit_valid_o[i]` records
the selected ConfigID, PatternID, action bits, and same-edge ledger commit.
For a failed V2 group, `done_o` with `group_repairable_o=0` and
`failure_position_o` is the terminal failure decision; no explicit per-SA
failure-ready strobe exists.

```text
S1A_STATUS: COMPLETE
SOURCE_MODIFIED: NO
TARGET: 2x2 Directional CAM RS=2 CS=2 m=1
```

================================
## SA_TEST_START
================================

### RTL

```text
Frozen V2 EARLY/GROUP:
  status: NOT_DEFINED
  existing signal: start_i is DSS decision start, not BIST/subarray test start
  source: rtl/dss_v2/top/recam_dss_v2_early_top.sv:4-11;
          rtl/dss_v2/top/recam_dss_v2_group_top.sv:8-71
  clock/event semantics: start_i is sampled on clk_i rising edge by each core
  per-SA identifiable: NO
  independent A/B/C/D: NO

Historical CAM collector:
  status: NOT_DEFINED for BIST test start
  closest name: clear_i / collector_enable; subarray_start_i in dss_analyzer_top
  source: rtl/dss_2x2/cam/dss_cam_top.sv:99-104;
          rtl/dss_2x2/analyzer/dss_analyzer_top.sv:11-15,39
  meaning: begins or clears a fault-collection session, not an address-domain
           BIST-start event
  per-SA identifiable: PARTIAL only through fault_subarray_i or subarray_i
```

### Testbench

```text
status: IMPLICIT / APPROXIMATE only
existing action: dss_analyzer_top golden TB pulses subarray_start_i for A..D
source: tb/dss_2x2/dss_analyzer_top_golden_test.cpp:25-28
clock/event semantics: one testbench-generated clk_i rising edge pulse
per-SA identifiable: YES
independent A/B/C/D: NO; the TB drives a serial loop
qualification: this is collection-session setup, not proof of BIST testing
```

The V2 Phase-4I TB does not provide a per-SA start. It drives either
`early_start_i` or `group_start_i` once at the model's final-fault marker
(`tb/dss_v2/recam_dss_v2_phase4i_rtl_timing_test.cpp:144-150`).

### C++ simulator

```text
Primary DynamicSpareSharing status: NOT_DEFINED
existing function: DynamicRepairSimulator::run(FaultGroup,...)
source: src/DynamicRepairSimulator.cpp:1164-1252
meaning: receives already-built per-SA fault vectors; no BIST traversal occurs
per-SA identifiable: fault vector index only
independent A/B/C/D: NO timing event

Optional HierarchicalRECAM timeline status: DERIVABLE
existing type: SerialBistSchedule::groupStartCycle and scan-index formula
source: inc/FaultAddress.hpp:55-67; src/FaultAddress.cpp:168-201
meaning: SA i starts at groupStartCycle + i * rows * wordsPerRow * cyclesPerWord
per-SA identifiable: YES, by derivation only
independent A/B/C/D: NO; schedule is serial A->B->C->D
```

```text
EXISTING_NAME: NONE
NAME_MATCH: NONE
CROSS_LAYER_STATUS: MISSING_IN_ONE_LAYER
```

================================
## SA_TEST_DONE
================================

### RTL

```text
Frozen V2 EARLY/GROUP:
  status: NOT_DEFINED
  evidence: no address/BIST or per-SA test-complete port exists in either top
  source: rtl/dss_v2/top/recam_dss_v2_early_top.sv:4-11;
          rtl/dss_v2/top/recam_dss_v2_group_top.sv:8-55
  final tested address implies done: NOT REPRESENTED
  zero-fault SA: no test-done signal exists
  different A/B/C/D completion times: NOT OBSERVABLE

Historical CAM collector:
  status: NOT_DEFINED
  closest signal: subarray_commit_i is an external collection/result-capture
                  control, not a BIST completion signal
  source: rtl/dss_2x2/analyzer/dss_analyzer_top.sv:100-123,145-169
```

### Testbench

```text
status: IMPLICIT / APPROXIMATE only
existing action: TB pulses subarray_commit_i after each supplied SA fault list
source: tb/dss_2x2/dss_analyzer_top_golden_test.cpp:27-28
per-SA identifiable: YES
zero-fault SA: YES, the loop still pulses subarray_commit_i
qualification: it is a TB collection-close/capture convention, not address-scan
               completion and not an RTL-generated done event
```

### C++ simulator

```text
Primary DynamicSpareSharing status: NOT_DEFINED
source: src/DynamicRepairSimulator.cpp:1164-1383

Optional HierarchicalRECAM timeline status: DERIVABLE
source: src/FaultAddress.cpp:203-220
SA i conceptual completion: groupStartCycle + (i + 1) * rows * wordsPerRow *
                            cyclesPerWord
materialized field: completionCycle() is group-only, after SA D
zero-fault SA: the derived scan boundary still exists
independent A/B/C/D: NO; serial schedule
```

```text
EXISTING_NAME: completionCycle (group only)
NAME_MATCH: APPROXIMATE
CROSS_LAYER_STATUS: MISSING_IN_ONE_LAYER
```

================================
## SA_LAST_FAULT_ACCEPT
================================

### RTL

```text
Frozen V2 EARLY/GROUP:
  status: NOT_DEFINED
  FAULT_ACCEPT_EVENT existing name: NONE
  exact event: N/A; production tops have no fault_valid_i/fault_ready_o input
  synchronous: N/A
  handshake required: N/A
  per-SA identifiable: NO
  source: rtl/dss_v2/top/recam_dss_v2_early_top.sv:4-11;
          rtl/dss_v2/top/recam_dss_v2_group_top.sv:14-55

Historical CAM collector:
  status: EXPLICIT, but not the frozen V2 production boundary
  FAULT_ACCEPT_EVENT existing name: accepted_fault / accept
  exact event: fault_valid_i && fault_ready_o for selected fault_subarray_i,
               stored on the following clk_i rising edge
  handshake required: YES, valid-ready
  per-SA identifiable: YES, fault_subarray_i selects one of four fault_cam
  source: rtl/dss_2x2/common/fault_collector.sv:28-68;
          rtl/dss_2x2/common/fault_cam.sv:40-54,84-93

Alternate shared collector:
  exact event: accept = fault_valid_i && fault_ready_o, sampled at clk_i edge
  source: rtl/dss_2x2/analyzer/shared_fault_collector.sv:66-99
```

The last accepted fault of an SA is only derivable by observing the final
valid-ready transfer bearing that SA tag. It is not stored as an RTL event.
For zero accepted faults, `SA_LAST_FAULT_ACCEPT` is undefined/N/A; no synthetic
event is implied by test completion.

### Testbench

```text
Historical CAM TB status: EXPLICIT stimulus-side valid-ready transaction
existing action: pushFault checks fault_ready_o, drives fault_valid_i for one tick
source: tb/dss_2x2/dss_cam_top_test.cpp:54-64
per-SA identifiable: YES, Fault::subarray drives fault_subarray_i

V2 Phase-4I TB status: NOT_DEFINED as accept
source: tb/dss_v2/recam_dss_v2_phase4i_rtl_timing_test.cpp:144-150
qualification: its "final-fault / start-acceptance" label marks a modelled
               fault-set event and decision start, not a DUT fault handshake
```

### C++ simulator

```text
Primary DynamicSpareSharing status: NOT_DEFINED
source: src/DynamicRepairSimulator.cpp:1204-1241
meaning: solver_->solve receives the full faults[subarray] vector at once;
         no per-fault insertion/handshake timestamp is retained

RECAM batch collector status: IMPLICIT batch load, not an accept event
source: src/RECAM_PE.cpp:5-23

Optional HierarchicalRECAM status: PARTIAL / different event
existing fields: firstFaultArrivalCycle, lastFaultArrivalCycle,
                 faultCollectionCompletionCycle
source: inc/BiraLatency.hpp:40-51; src/BiraLatency.cpp:241-268
meaning: BIST arrival and aggregate FIFO completion, not per-fault accepted
         service or per-SA last acceptance
```

```text
EXISTING_NAME: accepted_fault / accept (alternate RTL); lastFaultCycle and
               lastFaultArrivalCycle are approximate model names only
ZERO_FAULT_BEHAVIOR: LAST_FAULT_ACCEPT = N/A
CROSS_LAYER_STATUS: MISSING_IN_ONE_LAYER
```

================================
## SA_DECISION_READY
================================

### RECAM baseline

```text
RTL status: EXPLICIT group-level only
existing names: analysis_done_o, solution_valid_o
source: rtl/dss_2x2/cam/dss_cam_top.sv:99-116,240-310;
        rtl/dss_2x2/common/solution_store.sv:32-58
exact event: selector_done is captured into solution_store; after that rising
             edge analysis_done_o = stored_solution_valid
per-SA final decision: NOT_DEFINED; selected fields are a four-SA group image
zero-fault group: defined if analysis_start_i is asserted; the historical TB
                  verifies normal_empty
source for zero-fault case: tb/dss_2x2/dss_cam_top_test.cpp:66-76,133-137

C++ status: DERIVABLE at DynamicRepairSimulator::run return only
source: src/DynamicRepairSimulator.cpp:1360-1383
```

### EARLY

```text
RTL status: EXPLICIT per-SA commit and explicit group completion
existing names: sa_commit_valid_o[i], selected_config_flat_o[i],
                selected_pattern_flat_o[i], done_o, group_repairable_o
source: rtl/dss_v2/top/recam_dss_v2_early_core.sv:28-31
exact event: clk_i rising edge with accept for SA i; that same edge sets the
             commit bit and selected ConfigID/PatternID while the ledger sees
             commit_i = busy_o && accept
ledger relation: same registered edge; outputs are stable after that edge
before next SA starts: YES. The next SA is selected only after this commit edge.
per-SA observable: YES in RTL output vector, as an assertion edge of a
                   sticky-per-run bit
independent A/B/C/D: PARTIAL. Commits are sequential A->B->C->D, not parallel.

C++ status: DERIVABLE, not timestamped
source: src/DynamicRepairSimulator.cpp:1312-1360
meaning: findEarlyChoice commits semantic choices in order, but only the final
         GroupRepairResult is externally returned from the batch call
```

### GROUP_NO_SCRATCH_V2

```text
GROUP_CANDIDATE_READY:
  status: IMPLICIT_INTERNAL
  existing event: rising edge that writes slot 3 of SA i into candidate store
  source: rtl/dss_v2/top/recam_dss_v2_group_core.sv:35-50;
          rtl/dss_v2/group/dss_v2_group_candidate_store.sv:51-63
  semantics: all four local {valid, PatternID} records for SA i are stored;
             static ConfigID decoding is derivable, but no final resource
             decision exists yet

GROUP_FINAL_DECISION_READY_A/B/C/D:
  status: EXPLICIT_INTERNAL / externally observable at the production top
  existing event: respective assertion edge of sa_commit_valid_o[i]
  source: rtl/dss_v2/top/recam_dss_v2_group_core.sv:29-51
  ConfigID final: YES at commit edge
  PatternID final: YES at commit edge
  release/borrow and ledger commit: same edge
  ordering: A, then B, then C, then D; earlier commits are not changed by later
            decisions under the no-rollback core

GROUP_DECISION_EVENT:
  existing event: done_o rising edge
  source: rtl/dss_v2/top/recam_dss_v2_group_core.sv:46-51
  scope: per-group DSS decision-ready
  successful group: coincides with D commit; group_repairable_o=1
  failed group: terminal at the first exhausted SA rank; group_repairable_o=0
                and failure_position_o identify the failure

C++ status: DERIVABLE, not timestamped
source: src/DynamicRepairSimulator.cpp:930-1003,1339-1357
existing records: V2DecisionTrace, selectedConfigIds, selectedPatternIds,
                  selectedV2Actions, firstFailureSubarray
source of records: inc/RepairResult.hpp:236-256
```

`done_o` is therefore a per-group DSS decision-ready event, not full
repair-table-ready. Its guaranteed successful outputs are group repairability,
all selected ConfigIDs/PatternIDs, donor/release actions, and ledger state.
Pivot/Hybrid reconstruction and repair-address generation remain downstream
of this frozen V2 top boundary.

```text
DONE_O_RELATION:
  EARLY: group decision-ready; successful D commit is on same edge
  GROUP: group decision-ready; successful D commit is on same edge after COLLECT
  not: per-SA test-done, raw-fault acceptance, full repair-table-ready

EXISTING_NAME: sa_commit_valid_o / done_o
NAME_MATCH: APPROXIMATE for per-SA finality, EXACT for group DSS decision-ready
CROSS_LAYER_STATUS: SEMANTICALLY_EQUIVALENT
```

================================
## EARLY PER-SA COMPLETION
================================

| SA | Final decision event | When it occurs | Observable? |
|---|---|---|---|
| A | `sa_commit_valid_o[0]` assertion | first accepted candidate/ledger commit edge | Yes, top output bit; C++ only derivable |
| B | `sa_commit_valid_o[1]` assertion | after A commit, at B commit edge | Yes, top output bit; C++ only derivable |
| C | `sa_commit_valid_o[2]` assertion | after B commit, at C commit edge | Yes, top output bit; C++ only derivable |
| D | `sa_commit_valid_o[3]` assertion | after C commit; same edge as successful `done_o` | Yes, top output bit |

```text
INDEPENDENT_COMPLETION_OBSERVABLE: PARTIAL
reason: each bit is observable, but the implementation is intentionally
        sequential and has no independent SA engines or per-SA done pulse.
```

================================
## GROUP PER-SA COMPLETION
================================

| SA | Candidate-ready | Final decision-ready | Observable? |
|---|---|---|---|
| A | slot-3 candidate-store write | `sa_commit_valid_o[0]` allocation commit | candidate internal; commit top-visible |
| B | slot-3 candidate-store write | `sa_commit_valid_o[1]` allocation commit | candidate internal; commit top-visible |
| C | slot-3 candidate-store write | `sa_commit_valid_o[2]` allocation commit | candidate internal; commit top-visible |
| D | slot-3 candidate-store write | `sa_commit_valid_o[3]` allocation commit | candidate internal; commit top-visible and successful `done_o` coincident |

```text
CANDIDATE_READY_VS_FINAL_READY:
  Candidate ready occurs during COLLECT and records only local eligibility and
  smallest PatternID for a canonical slot.
  Final ready occurs during ALLOCATE only after priority, live-ledger
  feasibility, and immediate commit are resolved.

GROUP_DECISION_EVENT:
  done_o, not candidate-store completion and not an earlier SA commit.
```

================================
## RTL / TB / C++ cross-layer consistency
================================

| Event | RTL | Testbench | C++ simulator | Classification |
|---|---|---|---|---|
| SA_TEST_START | missing in frozen V2 | only a collection-start approximation in alternate TB | missing in primary simulator; derivable serial schedule in hierarchy | MISSING_IN_ONE_LAYER |
| SA_TEST_DONE | missing in frozen V2 | `subarray_commit_i` is TB convention, not BIST done | missing in primary; derivable serial boundary only | MISSING_IN_ONE_LAYER |
| SA_LAST_FAULT_ACCEPT | explicit only in alternate CAM collector; missing in frozen V2 | explicit alternate valid-ready drive; Phase-4I marker is not accept | batch input / arrival-only metrics, no accept | MISSING_IN_ONE_LAYER |
| SA_DECISION_READY | V2 commit bits plus group `done_o` | existing V2 TB can observe both; Phase-4I records `done_o` | batch V2 trace/result is semantically ordered but untimestamped | SEMANTICALLY_EQUIVALENT |

================================
## GROUP DERIVATION
================================

```text
GROUP_TEST_DONE_FROM_MAX_SA:
  PARTIAL
  The optional SerialBistSchedule permits mathematical SA-boundary derivation,
  and its maximum is the existing group completionCycle(). Neither frozen V2
  RTL nor the primary C++ simulator materializes test_done[SA].

GROUP_LAST_FAULT_FROM_MAX_SA:
  INVALID for the requested accept definition
  No per-SA accepted-fault timestamps exist. max(lastFaultArrivalCycle) would
  describe BIST arrivals, not acceptance; a zero-fault SA has no value.

GROUP_DECISION_FROM_MAX_SA:
  PARTIAL
  On a successful V2 run, D commit and done_o coincide, so done_o equals the
  maximum committed-SA decision edge. On failure, done_o is the correct
  terminal group decision event, while the failed SA has no commit bit; taking
  a maximum over successful commit bits is therefore invalid.
```

================================
## FAILURE SEMANTICS
================================

```text
DECISION_READY_ON_FAILURE: PARTIAL

V2 RTL: YES at group scope. done_o asserts with group_repairable_o=0 and
        failure_position_o when the first SA exhausts its ranked candidates.
        There is no dedicated per-SA failure-ready event; the failed SA lacks
        a selected ConfigID/PatternID commit.
C++ V2: YES at batch-result scope through firstFailureSubarray and the returned
        failed GroupRepairResult; no time event exists.
RECAM baseline: group analysis_done_o may capture repairable_o=0, but does not
        identify a time-resolved per-SA terminal decision.
```

Future latency definition must choose whether it measures successful decisions
only or both successful and terminal-failure group decisions. This audit does
not make that choice.

================================
## CLOCK SEMANTICS
================================

| Event | Existing edge convention |
|---|---|
| Alternate CAM fault accept | `fault_valid_i && fault_ready_o` is sampled at `posedge clk_i`; CAM storage/count update occurs on that edge. |
| V2 `start_i` | sampled at `posedge clk_i`; starts only decision control over pre-collected analyzer state. |
| V2 SA commit | `sa_commit_valid_o[i]`, selected values, and ledger transaction update on the same `posedge clk_i`; observe the asserted/stable outputs immediately after that edge. |
| V2 `done_o` | registered, one-cycle completion indication at the terminal `posedge clk_i`; observe at its assertion edge, not one later stable cycle. |
| Phase-4I BIST end | testbench/model marker only; it has no DUT state effect. |

================================
## Zero-fault semantics
================================

| Event | Existing behavior |
|---|---|
| SA_TEST_START | Defined only in the optional derived serial schedule or an external TB convention; absent in frozen V2. |
| SA_TEST_DONE | Derivable in the serial schedule regardless of fault count; absent in frozen V2. |
| SA_LAST_FAULT_ACCEPT | N/A; no accepted fault exists. |
| SA_DECISION_READY | A decision can be made for an empty collected state when a decision start is supplied; no fault-tail timestamp is thereby created. Historical CAM `normal_empty` coverage demonstrates group analysis completion. |

================================
## EVENTS_MISSING_FOR_S1B
================================

```text
1. Per-SA BIST test-start and test-done at the frozen V2 production boundary.
2. Per-SA raw-fault valid-ready acceptance at that boundary.
3. A stored/exported timestamp for each SA's last accepted fault.
4. A per-SA terminal-failure-ready event, if failed SA latency is required.
5. A common physical clock mapping between batch C++ policy traces and RTL
   decision edges.
```

================================
## RECOMMENDED_MINIMUM_INSTRUMENTATION
================================

These are recommendations only; none is implemented by S1A.

| Missing event | Layer | Insertion point | Semantic meaning | Estimated invasiveness |
|---|---|---|---|---|
| SA BIST start/done | C++ hierarchy/reporting | derive and report four serial schedule boundaries from `SerialBistSchedule` | address-domain lifecycle in the existing optional serial model | REPORTING_ONLY |
| SA BIST start/done at V2 boundary | RTL integration | BIST/collector wrapper above V2 tops, not inside the decision cores | physical per-SA test lifecycle | INTERFACE_EXTENSION |
| SA fault accept at V2 boundary | RTL integration + TB | raw fault collector to analyzer-state handoff | valid-ready accepted input belonging to an SA | INTERFACE_EXTENSION |
| V2 per-SA commit timestamp | TB reporting | sample existing `sa_commit_valid_o` assertion edges | final successful per-SA DSS decision | REPORTING_ONLY |
| V2 group decision timestamp | TB reporting | sample existing `done_o` assertion edge | terminal group DSS decision, success or failure | REPORTING_ONLY |
| C++ decision event timestamp | C++ simulator | an explicitly specified nonfunctional event trace, only after a shared clock contract exists | simulator-side policy decision order/time | NONFUNCTIONAL_STATE |
| Failed-SA terminal decision | RTL/TB | derive from existing `done_o` plus `failure_position_o`, or expose a reporting strobe if required | terminal failed decision for the first failing SA | REPORTING_ONLY initially |

================================
## S1B_ELIGIBILITY
================================

```text
S1B_LATENCY_INSTRUMENTATION_ELIGIBLE: PARTIAL
reason: decision-ready endpoints already exist in V2, but the requested
        per-SA test lifecycle and per-SA fault acceptance contract do not.

S1B_AUTHORIZED: NO
DEVICE_LATENCY_AUTHORIZED: NO
NEXT_PHASE_AUTHORIZED: NONE
```
