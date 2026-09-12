# Phase 4I-1 — Post-BIST DSS Decision-Latency Event Model

## Scope and boundary

This phase defines a C++ event model for the frozen 2×2 DSS
decision/resource-management boundary only.  It measures post-BIST **DSS
decision-ready** time, not `start_i → done_o` execution time, final repair-map
reconstruction time, Scratch, SRAM, 1×4, or a new topology.

The implementation is intentionally separate from the project-wide CAM/SRAM
BIRA models:

```text
inc/DssPostBistLatency.hpp
src/DssPostBistLatency.cpp
tests/dss_post_bist_latency_test.cpp
```

It consumes an ordered fault-event trace plus a functional DSS decision work
plan.  The plan is an already-derived frozen decision trace; the timing model
does not derive ConfigID, PatternID, donors, ledger actions, or repairability
from fault coordinates.  This keeps the timing experiment separate from the
accepted Phase 4E/4F functional-equivalence models.

## Frozen timestamp convention

```text
T_last_fault     = last BIST event that changes the collected fault set
T_BIST_end       = BIST completion edge
G                = T_BIST_end - T_last_fault
T_decision_ready = frozen DSS decision boundary
L_last_fault     = T_decision_ready - T_last_fault
L_post           = max(0, T_decision_ready - T_BIST_end)
```

A fault-set-changing event at rising edge `N` is visible immediately after
that edge.  It invalidates/restarts concurrent DSS analysis in interval
`[N,N+1)`.  A decision plan requiring `W` cycles is available at rising edge
`N+W`.  If that edge is also `T_BIST_end`, `L_post = 0`.

The model never defers analysis until BIST completion: every changing event
launches a new concurrent attempt, and only the attempt launched by
`T_last_fault` can supply the final observable decision.  Duplicate events
marked `changesFaultSet = false` do not restart the decision.

Decision ready means that `group_repairable` is known and, on a successful
decision, selected ConfigID/PatternID, donor/release actions, and the
resource ledger are final.  Pivot/Hybrid reconstruction is explicitly outside
this boundary.

## Work-plan basis

For a functional trace, `candidateEvaluations[A..D]` records the number of
ranked candidate evaluations for each active SA.  The model enforces one to
four evaluations per active SA and stops at the first failing SA.

| Policy | Event-model execution work |
|---|---|
| EARLY | Sum of active SA candidate evaluations.  It therefore remains candidate-depth and failure-position sensitive. |
| GROUP-NoScratch | Fixed 16-cycle candidate collection plus the same sequential A→B→C→D allocation evaluation sum. |

This preserves the accepted representative GROUP behavior: all-local = 20,
failure-A/B/C/D = 20/21/22/23, and deeper rank paths can reach 26 cycles.

## Directed validation

The common fixed input trace contains two changing fault events at cycles 10
and 20 (`T_last_fault = 20`).  EARLY and GROUP receive this identical event
trace and BIST-end sweep; only the policy-specific frozen decision plan
differs.  The CSV artifact is
[`phase4i1_directed_event_model.csv`](../results/phase4i/phase4i1_directed_event_model.csv).

| Representative case | Policy | T_last_fault | T_BIST_end | G | T_decision_ready | L_post |
|---|---|---:|---:|---:|---:|---:|
| G=0, all-local | EARLY | 20 | 20 | 0 | 24 | 4 |
| earliest zero-exposed-latency all-local | EARLY | 20 | 24 | 4 | 24 | 0 |
| G=0, all-local | GROUP-NoScratch | 20 | 20 | 0 | 40 | 20 |
| earliest zero-exposed-latency all-local | GROUP-NoScratch | 20 | 40 | 20 | 40 | 0 |

For the fixed trace, the model also sweeps
`G = 0, 1, 2, 3, 4, 8, 16, 32`.  `L_post` is nonincreasing for both policies.
The directed test additionally checks an EARLY depth-sensitive 5-cycle plan,
an EARLY failure-at-B 4-cycle plan, and a GROUP failure-at-B 21-cycle plan.

## RTL-validation preparation

The CSV fields and C++ `DssFaultSetEvent` input form the Phase 4I-2 trace
interface.  Representative future RTL checks are limited to the same fixed
trace at EARLY `G=0`/`G=4` and GROUP `G=0`/`G=20`; they must compare measured
`T_decision_ready` with the C++ prediction using the frozen edge convention.
No RTL validation or statistical sweep was run in Phase 4I-1.

## Reproduction and phase state

```bash
make test_dss_post_bist_latency
```

The Makefile target also belongs to the aggregate C++ `make test` suite.

```text
EVENT_MODEL_IMPLEMENTED = YES
EARLY_DIRECTED_G0 = PASS
EARLY_ZERO_LATENCY_CASE = PASS
GROUP_DIRECTED_G0 = PASS
GROUP_ZERO_LATENCY_CASE = PASS
POST_BIST_LATENCY_MONOTONIC = PASS

PHASE_4I_1_EVENT_MODEL = PASS
READY_FOR_PHASE_4I_2 = YES

V2_PHASE = PHASE_4I
V2_NEXT_PHASE_AUTHORIZED = YES
```
