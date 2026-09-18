# S1B-1 — DATE 2x2 DSS Decision-Latency Instrumentation

## Scope and status

`S1B1_STATUS: COMPLETE`

This is a testbench-only measurement for the fixed 2x2 directional-CAM,
`RS=2`, `CS=2`, `SHARE_M=1` implementation.  It measures only DSS decision
availability for `EARLY` and `GROUP_NO_SCRATCH_V2`.  No production RTL, RTL
interface, simulator policy, repair semantics, BIST/fault interface, or
device-timing model was changed.

Modified sources:

- `tb/dss_v2/recam_dss_v2_early_core_test.cpp`
- `tb/dss_v2/recam_dss_v2_group_core_test.cpp`

The tests retain their functional assertions and add an observer which stores
the rising-edge index at which an existing output first asserts.  The observer
does not drive a new DUT input, alter a DUT signal, or add DUT state.

## Frozen cycle convention

For every transaction, the rising edge that accepts `start_i` is **cycle 0**.
All subsequent rising edges are numbered 1, 2, and so on.  For every reported
event:

```text
latency_cycles = end_cycle - start_cycle
```

Thus the listed event cycle is also its decision latency in this test.  Events
are sampled immediately after the relevant rising edge.  A sticky
`sa_commit_valid_o[i]` output is counted once, on its `0 -> 1` assertion edge.
`done_o` is a one-cycle completion edge.  It means **DSS group decision
complete**; it is not described here as repair-table-ready.

## Event binding

### EARLY

| Measurement | Existing event | Evidence |
| --- | --- | --- |
| `DSS_DECISION_START` | `start_i && !busy_o` accepted on a rising edge | `rtl/dss_v2/top/recam_dss_v2_early_core.sv:31` |
| `EARLY_SA_DECISION_READY[i]` | `sa_commit_valid_o[i]` assertion edge | `recam_dss_v2_early_core.sv:31` |
| `EARLY_GROUP_DECISION_READY` | `done_o` assertion edge | `recam_dss_v2_early_core.sv:31` |

The EARLY test drives `start_i` for one idle edge, records A/B/C/D commit
edges, `done_o`, average committed-SA decision cycles, and the terminal
failure position.  The core's commit branch also commits the live ledger, so
the sampled commit edge is the required per-SA decision-ready edge rather than
a candidate-ready indication.

### GROUP_NO_SCRATCH_V2

| Measurement | Existing event | Evidence |
| --- | --- | --- |
| `DSS_DECISION_START` | `start_i && state_q == IDLE` accepted on a rising edge | `rtl/dss_v2/top/recam_dss_v2_group_core.sv:49` |
| `GROUP_SA_INTERNAL_FINALIZE[i]` | `sa_commit_valid_o[i]` assertion edge in the allocation/ledger-commit branch | `recam_dss_v2_group_core.sv:43,51` |
| `GROUP_DECISION_READY` | `done_o` assertion edge | `recam_dss_v2_group_core.sv:51` |

`GROUP_SA_INTERNAL_FINALIZE[i]` is observable in the core testbench and marks
an irreversible sequential policy/ledger decision for that SA.  It is an
internal allocation trace, not a separate externally meaningful group result:
only `GROUP_DECISION_READY` (`done_o`) reports a complete group decision.
Candidate collection/ready is deliberately not used as a final-decision event.

## Observed decision-cycle traces

These are deterministic directed core-test transactions, not Post-BIST or
fault-tail measurements.

### EARLY

| Case | A commit | B commit | C commit | D commit | `done_o` | Terminal result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| all-local success | 1 | 2 | 3 | 4 | 4 | success; average SA decision = 2.50 |
| release then borrow success | 2 | 4 | 5 | 6 | 6 | success; average SA decision = 4.25 |
| resource consumption at C | 2 | 4 | — | — | 8 | failure at C; average committed-SA = 3.00 |
| terminal failure at A | — | — | — | — | 4 | failure at A |
| terminal failure at B | 2 | — | — | — | 6 | failure at B |
| terminal failure at C | 2 | 3 | — | — | 7 | failure at C |
| terminal failure at D | 2 | 3 | 4 | — | 8 | failure at D |

Therefore both successful and failed EARLY decision timing are measurable.
The success case’s group decision latency is 4 cycles for the all-local vector
and 6 cycles for the directed release/borrow vector; it is intentionally not
represented as a single architecture-independent constant.

### GROUP_NO_SCRATCH_V2

| Case | A finalize | B finalize | C finalize | D finalize | `done_o` | Terminal result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| all-local success | 18 | 20 | 22 | 24 | 24 | success; average finalized-SA = 21.00 |
| resource consumption at C | 17 | 21 | — | — | 25 | failure at C; average finalized-SA = 19.00 |
| terminal failure at A | — | — | — | — | 20 | failure at A |
| terminal failure at B | 18 | — | — | — | 22 | failure at B |
| terminal failure at C | 18 | 20 | — | — | 24 | failure at C |
| terminal failure at D | 18 | 20 | 22 | — | 26 | failure at D |

The all-local GROUP trace includes the frozen 16 candidate collection cycles
before sequential allocation.  Per-SA finalization is measurable as the
internal trace above; group decision completion is always measured at `done_o`.

## Functional non-interference regression

Commands run after adding the observers:

```text
bash scripts/simulation/run_dss_v2_phase4f3_group.sh
bash scripts/simulation/run_recam_phase4e3_random.sh
make test_solution_take_policy
```

Results:

- GROUP core directed regression: PASS, including ranking, resource
  consumption, first-failure termination, and no-rollback checks.
- EARLY RTL-vs-golden equivalence: 50 vectors and 1000 vectors, 0 mismatches.
- GROUP RTL-vs-golden equivalence in the frozen Phase-4I regression: 50 vectors
  and 1000 vectors, 0 mismatches.
- The S0R frozen V2 2x2x1 corpus (1000 vectors) reports `config=0`,
  `pattern=0`, `action=0`, `ledger=0`, and `failure=0` mismatches.

```text
INSTRUMENTATION_REGRESSION: PASS
CONFIGID_MISMATCHES: 0
PATTERNID_MISMATCHES: 0
ACTION_MISMATCHES: 0
LEDGER_MISMATCHES: 0
FAILURE_POSITION_MISMATCHES: 0
REPAIR_SEMANTICS_CHANGED: NO
```

## S1B-2 planning boundary (no implementation)

No `SA_TEST_DONE`, `SA_LAST_FAULT_ACCEPT`, Post-BIST, or fault-tail metric is
implemented or reported by S1B-1.

| Future event | Minimum insertion point | Classification |
| --- | --- | --- |
| `SA_TEST_DONE` | Materialize it at the serial-BIST schedule/address-domain boundary, using the existing schedule only as its specification source. | Requires future hardware-observed event insertion. |
| `SA_LAST_FAULT_ACCEPT` | Prefer the actual collector transfer `fault_valid_i && fault_ready_o` at the collector handshake. | Hardware-observed when the handshake exists.  A path which only replays a fault list is `MODEL_DERIVED`, not hardware-observed. |

The frozen V2 production tops consume precollected analyzer information and do
not currently expose a raw-fault valid/ready interface.  S1B-2 is consequently
only partially prepared by this evidence; it is not authorized here.

## Required status report

```text
S1B1_STATUS:
COMPLETE

SOURCE_FILES_MODIFIED:
tb/dss_v2/recam_dss_v2_early_core_test.cpp
tb/dss_v2/recam_dss_v2_group_core_test.cpp
docs/dss_execution/S1B1_DATE_2X2_DSS_DECISION_LATENCY_INSTRUMENTATION.md

REPAIR_SEMANTICS_CHANGED:
NO

CYCLE_COUNT_CONVENTION:
latency_cycles = end_cycle - start_cycle; accepted start_i edge is cycle 0.

EARLY:
decision_start_event: start_i && !busy_o accepted at a rising edge
A_ready_event: sa_commit_valid_o[0] 0->1 assertion edge
B_ready_event: sa_commit_valid_o[1] 0->1 assertion edge
C_ready_event: sa_commit_valid_o[2] 0->1 assertion edge
D_ready_event: sa_commit_valid_o[3] 0->1 assertion edge
group_done_event: done_o assertion edge (DSS group decision complete)
per_SA_cycles_measurable: YES
group_cycles_measurable: YES

GROUP:
decision_start_event: start_i && state_q == IDLE accepted at a rising edge
A_internal_finalize_event: sa_commit_valid_o[0] 0->1 assertion edge
B_internal_finalize_event: sa_commit_valid_o[1] 0->1 assertion edge
C_internal_finalize_event: sa_commit_valid_o[2] 0->1 assertion edge
D_internal_finalize_event: sa_commit_valid_o[3] 0->1 assertion edge
group_done_event: done_o assertion edge (DSS group decision complete)
per_SA_cycles_measurable: YES, as an internal sequential finalization trace
group_cycles_measurable: YES

FAILURE_TIMING_MEASURABLE:
YES

INSTRUMENTATION_REGRESSION:
PASS

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

FUTURE_SA_TEST_DONE_INSERTION_POINT:
Serial-BIST schedule/address-domain boundary; not yet materialized.

FUTURE_LAST_FAULT_ACCEPT_INSERTION_POINT:
Collector fault_valid_i && fault_ready_o handshake; fault-list replay is MODEL_DERIVED.

S1B2_ELIGIBLE:
PARTIAL

S1B2_AUTHORIZED:
NO

LATENCY_EXPERIMENT_AUTHORIZED:
NO

DEVICE_LATENCY_AUTHORIZED:
NO

NEXT_PHASE_AUTHORIZED:
NONE
```
