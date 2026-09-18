# T1R — Per-SA Candidate-Catchup Latency Contract Derivation

## 1. Objective and scope

This audit derives the timing contract of the frozen S1G-A2G retained-state
EARLY overlap controller.  It characterizes the path from an accepted raw
fault to an observable, generation-matching candidate-analysis completion.

It does not change production RTL, repair selection, ledger behavior, the
C++ timing model, `HierarchicalRECAM`, synthesis, or S1G-B.  The only source
changed for this audit is the pre-existing isolated Verilator test harness
`tb/dss_v2/recam_dss_v2_retained_overlap_core_test.cpp`.

Cycle convention throughout this document is edge-to-edge:

```text
candidate_catchup_cycles = ready_edge - accepted_fault_edge
```

The accepting edge is cycle 0.  Therefore a result at the fourth following
active clock edge has a delay of four cycles.

## 2. Historical latency-number audit

The prior S1F contract defined the symbolic quantity
`LATEST_CANDIDATE_READY - SA_LAST_FAULT_ACCEPT`, but explicitly did not
invent a numerical completion cycle.  T1 correctly kept the corresponding
candidate delay unpopulated.  The Phase 4I/E0-L values are final DSS
policy/ledger-decision timing, not retained collector, 232-bit view, or
candidate catch-up timing.  They are not reused here.

Consequently, the only numerical candidate-catchup result in this document is
the result derived from the frozen S1G-A2G controller and observed by the
directed RTL harness below.

## 3. `LAST_FAULT_ACCEPT`

For SA `i`, the authoritative event is the rising-edge pulse:

```text
retained_update_event_o[i] == 1
```

It is emitted when the core accepts:

```text
fault_valid_i && busy_o && bank_fault_ready[fault_sa_i]
```

The same acceptance reaches exactly the selected collector bank.  That bank
updates its retained state, increments `fault_generation_q`, and clears
`analysis_valid_q` on the same edge.  The collector's capacity predicate is
`fault_count_o < 12`; hence a full SA bank cannot accept another event.

Source correlation:

```text
raw fault interface
  -> retained_update_event_o[i]                 retained_overlap_core.sv:247-252
  -> per-SA path fault_valid_i gate              retained_overlap_core.sv:140-157
  -> retained_q update / generation increment    retained_collector_bank.sv:342-356
```

`fault_valid_i && fault_ready_o` is the input-side protocol condition;
`retained_update_event_o[i]` is the unambiguous observable edge used by the
testbench and this contract.

## 4. `LATEST_CANDIDATE_READY`

For SA `i`, the authoritative ready event is the one-cycle pulse:

```text
latest_candidate_ready_o[i] == 1
&& analysis_valid_o[i] == 1
&& analysis_generation_flat_o[i] == fault_generation_flat_o[i]
```

The pulse is asserted only when the shared analyzer completes slot 3 of its
four-slot role sweep for the current owner without an accepted owner fault.
The collector samples `analysis_complete_i` on that same edge, so the
generation-matching `analysis_valid_o` is observable after that edge.

This is an analysis/candidate readiness event.  It is not `sa_commit_valid_o`,
`final_ledger_decision_ready_o`, or `done_o`.

## 5. RTL path attribution

The actual path is:

```text
accepted fault
  -> 821-bit collector retained-state update and generation increment
  -> combinational reconstruction of the selected 232-bit analyzer view
  -> nine-entry post-Must projection
  -> combinational shared ConfigID analyzer result
  -> one registered slot per role (slots 0, 1, 2, 3)
  -> analysis_complete / matching analysis_generation / candidate-ready pulse
```

The collector constructs the 232-bit view combinationally, including the
filtered H0–H10-to-nine-entry post-Must subsequence.  The retained analyzer
path feeds that view directly into `recam_shared_config_analyzer`, whose
candidate evaluation is combinational.  The controller supplies the temporal
structure: it presents one role ConfigID per sweep edge and captures four
results before asserting ready.

Relevant source locations:

```text
232-bit post-Must view                retained_collector_bank.sv:296-340
9-slot projection + analyzer binding  retained_analyzer_path.sv:51-96
fixed role order / owner schedule     retained_overlap_core.sv:101-127, 260-279
```

## 6. Generation semantics and restart

An accepted fault always wins over a same-edge analysis completion:

```text
if (fault_valid_i && fault_ready_o) ...
else if (analysis_complete_i && generation matches) ...
```

Thus the fault updates retained state, increments the fault generation, and
invalidates analysis; it does not require rollback.  In the core, an accepted
fault for the current owner makes `owner_fault_update` true, resets `slot_q`
to zero, and suppresses `analyzer_complete`.  A non-owner fault updates only
that SA's independent retained bank; it cannot invalidate a different
owner's analysis.

## 7. Analyzer timing class

The shared ConfigID analyzer is **purely combinational**.  The externally
visible end-to-end timing class is:

```text
COMBINATIONAL_PLUS_CONTROL_DELAY
```

The combinational result is sampled once for each of four fixed role slots.
There is no hidden multi-cycle analyzer pipeline in this RTL.  The registered
valid/generation boundary is deliberately created by the collector completion
handshake.

## 8. Candidate-catchup latency rule

For a fault accepted for the *current analyzer owner*, the exact rule is:

```text
LATEST_CANDIDATE_READY = accepted_fault_edge + 4
```

provided the next four sweep edges are uninterrupted by another accepted fault
for that same owner.  Those edges evaluate and capture slots 0, 1, 2, and 3.
An owner update on any one of those edges restarts the count at zero.  In
particular, an update on the would-be slot-3 completion edge suppresses the
old ready event and the new generation becomes ready four subsequent clean
edges later.

For a fault accepted for a *non-owner* SA, the complete per-SA rule is:

```text
candidate_delay[i]
  = (edge on which i becomes owner and begins a clean sweep - accepted_fault_edge)
    + 4
```

The ownership-wait term is schedule dependent: it depends on earlier SA
test-done events and final selection/ledger progress in strict A -> B -> C ->
D order.  It has no universal finite constant without an environment that
supplies those events.  The architectural lower bound is four cycles.

Therefore `CANDIDATE_DELAY_CLASS = VARIABLE` for per-SA externally observed
latency, while the owner-local catch-up segment is a proven fixed four cycles.

## 9. Fault-update throughput

The top-level interface can accept one selected-SA fault event per clock edge
while `busy_o` is true and that selected bank has fewer than 12 accepted
faults.  There is one shared input, so this is one global accepted fault per
cycle, not four simultaneous SA updates.  A stream to a non-owner SA is safe:
the selected bank accumulates retained state while the shared analyzer serves
the current owner.  A stream to the owner restarts the owner sweep each time.

## 10. Directed timing tests

The isolated Verilator harness was extended and passed with these directed
cases.  All ready observations require the matching valid/generation condition
from section 4.

| ID | Directed condition | Result |
| --- | --- | --- |
| D1 | first pivot | owner-local ready on edge +4 |
| D2 | update after a ready prior state | new generation ready on edge +4 |
| D3 | additional independent pivot | latest generation ready on edge +4 |
| D4 | same-row Hybrid relation | latest generation ready on edge +4 |
| D5 | row-Must threshold witness | latest generation ready on edge +4 |
| D6 | update immediately after ready | new four-edge sweep observed |
| D7 | update on would-be completion edge | stale ready suppressed; new generation edge +4 |
| D8 | four back-to-back owner faults | all accepted; ready only after four clean edges |
| D9 | 9-entry post-Must ConfigID-2 witness | B ownership reaches its `{0,2,1,3}` sweep; ready edge +4 |
| D10 | raw membership above nine but post-Must at most nine | twelve accepted, then same four-edge control delay |

The harness also drives a successful A/B/C/D transaction, observes every
`latest_candidate_ready_o[i]`, proves matching generations for all four SAs,
and checks that no SA violates the four-cycle lower bound.  It intentionally
does not assert one fixed A/B/C/D delay because ownership wait is real control
state, not measurement noise.

Validation command:

```text
verilator --cc --exe --top-module recam_dss_v2_retained_overlap_core ...
make -C build/t1r_retained_overlap/obj -f Vrecam_dss_v2_retained_overlap_core.mk -j2
build/t1r_retained_overlap/obj/Vrecam_dss_v2_retained_overlap_core

S1GA2G_RETAINED_OVERLAP_CORE_TEST PASS
```

## 11. Same-cycle update/completion

`owner_fault_update` is part of the `analyzer_complete` predicate's negation.
At slot 3, a same-edge accepted owner fault therefore prevents both the core's
ready pulse and the collector completion path.  The collector independently
has update priority over `analysis_complete_i`.  Result:

```text
SAME_CYCLE_NEW_FAULT_WINS = YES
```

No stale result can become valid at that boundary.

## 12. Zero-fault semantics

After `start_i`, the initial owner A begins a normal four-slot sweep with
generation zero.  With no accepted fault it emits the same matching
analysis-ready event after its fourth sweep edge.  This is an **empty-summary
analysis readiness** event, not a claim that a repair candidate is valid or
that a ledger commit occurs.  B/C/D zero-fault readiness remains subject to
the same ownership schedule.

## 13. Candidate readiness versus final solution

Candidate readiness precedes final policy/ledger work.  `commit_now` requires
the matching analysis generation *and* test completion, a selected valid
candidate, descriptor validity, and resource feasibility.  Only the later
`STATE_FINAL_SELECT` path can assert `final_ledger_decision_ready_o`,
`sa_commit_valid_o`, or `done_o`.  Candidate-ready must never be labelled a
final repair-table-ready event.

## 14. BIST-model implications

The BIST/overlap model may schedule one accepted fault per clock for the
selected SA, subject to the per-SA 12-fault collector capacity.  It must use
the event rule in section 8 rather than add a fixed four cycles from every
fault globally:

* current-owner final fault: add four uninterrupted sweep edges;
* later-SA final fault: wait for its actual owner turn, then add four;
* any owner fault before completion: discard the previous provisional result.

This is a controller timing rule only.  It does not introduce device-level
latency, raw-fault/BIST RTL interfaces, or a repair-rate experiment.

## 15. T1 reopening decision

T1R closes the missing event and timing-rule definitions.  A future T1 model
can now represent the shared owner schedule explicitly; it must not replace
the schedule-dependent term with an unexplained constant.

```text
T1R_STATUS: COMPLETE
LAST_FAULT_ACCEPT_EVENT: retained_update_event_o[i] pulse on an accepted selected-SA fault
LATEST_CANDIDATE_READY_EVENT: latest_candidate_ready_o[i] pulse with analysis_valid_o[i] and matching generations
ANALYZER_TIMING_CLASS: COMBINATIONAL_PLUS_CONTROL_DELAY
CANDIDATE_DELAY_CLASS: VARIABLE
CANDIDATE_DELAY_CYCLES: owner-local = 4; per-SA = ownership_wait + 4
CANDIDATE_DELAY_RULE: four uninterrupted owner sweep edges after the latest accepted owner fault; non-owner waits for ownership first
FAULT_UPDATE_ACCEPT_INTERVAL: 1 clock edge globally, while busy and selected bank fault_count < 12
ONE_FAULT_EVENT_PER_CYCLE_SUPPORTED: YES
SAME_CYCLE_NEW_FAULT_WINS: YES
ZERO_FAULT_CANDIDATE_READY: YES, after the current owner's four-slot zero-generation sweep; not a commit guarantee
CANDIDATE_READY_DISTINCT_FROM_FINAL_COMMIT: YES
DIRECTED_TIMING_TESTS: PASS (D1-D10 plus scheduled A/B/C/D observation)
PROVISIONAL_CANDIDATE_DELAY_PROVEN: YES
T1_MAY_RESUME: YES
RTL_MODIFIED: NO
CPP_TIMING_MODEL_MODIFIED: NO
HIERACHIRECAM_MODIFIED: NO
SYNTHESIS_RUN: NO
S1GB_RESUMED: NO
GIT_DIFF_CHECK: PASS
DOCUMENT: docs/dss_execution/T1R_CANDIDATE_CATCHUP_LATENCY_CONTRACT.md
NEXT_RECOMMENDED_PHASE: resume the already-authorized standalone T1 timing model with the explicit shared-owner schedule; do not start a device sweep
```
