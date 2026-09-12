# Phase 4I-3 — RTL Representative Timing Validation

## Scope

This phase validates the Phase 4I C++ post-BIST DSS event model against
representative Verilator simulations of the unmodified production V2 tops:

```text
recam_dss_v2_early_top
recam_dss_v2_group_top
```

The endpoint is DSS decision ready: selected decision state, resource ledger,
and `group_repairable` are final at the registered `done_o` edge.  This is not
validation of reconstruction, Hybrid/pivot remapping, eFuse programming, or
runtime remapping setup.

## Production-boundary stimulus mapping

Neither production top exposes a raw fault-stream input nor `bist_end_i`.
The test-only observer therefore maps the frozen event-model boundary as
follows:

| Event-model item | Production-equivalent RTL stimulus / observation |
|---|---|
| `T_last_fault = 20` | Rising edge accepting `start_i` at cycle 20, with the current SA's already-collected analyzer state applied. |
| Per-SA collected analyzer state | Existing `conventional_overflow_i`, switched only when the production controller advances to that SA. `0` gives the actual zero-payload valid analyzer state; `1` gives the actual conventional-overflow invalid state. |
| Candidate/config activity | Observation-only hierarchical references to each production core's current SA/slot/config outputs. |
| `T_BIST_end` | Testbench marker only; it does not alter either DUT. |
| `T_decision_ready` | Rising edge asserting the production top's `done_o`. |

This does not bypass any production candidate selection, rank, feasibility,
ledger, commit, or completion logic.  The tester uses two valid analyzer input
states to represent different A/B/C/D BIST results because the current top
has one time-multiplexed analyzer input port rather than four raw fault-stream
ports.

The phase uses the exact Phase 4I-1 `DssPostBistLatency` implementation to
calculate every C++ expected result, rather than duplicating its timing
formula.  The frozen cycle convention remains:

```text
event at rising edge N → decision restart at N
W-cycle plan           → decision-ready at N+W
ready on T_BIST_end    → L_post = 0
```

## Exact RTL/C++ comparison

| Case | Policy | T_last_fault | T_BIST_end | C++ T_ready | RTL T_ready | C++ L_post | RTL L_post | Match |
|---|---|---:|---:|---:|---:|---:|---:|---|
| E1: all-local G=0 | EARLY | 20 | 20 | 24 | 24 | 4 | 4 | PASS |
| E2: all-local same-edge boundary | EARLY | 20 | 24 | 24 | 24 | 0 | 0 | PASS |
| E3: all-local G=2 | EARLY | 20 | 22 | 24 | 24 | 2 | 2 | PASS |
| E4: failure-B, 5-cycle path | EARLY | 20 | 20 | 25 | 25 | 5 | 5 | PASS |
| G1: all-local G=0 | GROUP-NoScratch | 20 | 20 | 40 | 40 | 20 | 20 | PASS |
| G2: all-local same-edge boundary | GROUP-NoScratch | 20 | 40 | 40 | 40 | 0 | 0 | PASS |
| G3: all-local G=16 | GROUP-NoScratch | 20 | 36 | 40 | 40 | 4 | 4 | PASS |
| G4: failure-B, 21-cycle path | GROUP-NoScratch | 20 | 20 | 41 | 41 | 21 | 21 | PASS |

E4 uses A valid and B conventionally overflowed.  The production EARLY core
commits A, evaluates all four B ranked candidates, and ends at cycle 25;
therefore it validates an actual `G_zero = 5` non-dominant path from the
Phase 4I-2 work-plan envelope.  G4 applies the same A-valid/B-overflowed
per-SA state to production GROUP: 16 collection cycles, A allocation, then
four B rank attempts terminate at cycle 41.  It validates a genuine long
GROUP path rather than assuming all GROUP decisions take 20 cycles.

## Trace evidence

[`phase4i3_rtl_cycle_trace.csv`](../results/phase4i/phase4i3_rtl_cycle_trace.csv)
records every relevant cycle: final-fault marker, BIST-end marker, current
candidate SA/slot/config, local candidate validity, accumulated commit bitmap,
ledger diagnostic state, and `done_o`.

For example, E2 marks BIST end at cycle 24, where its fourth commit and
`done_o` occur on the same edge.  G1 records GROUP collection at cycles
21–36, commits at 37–40, and `done_o` at cycle 40.

## Regression and reproduction

```bash
bash scripts/simulation/run_dss_v2_phase4i_rtl_timing.sh
make test_dss_post_bist_latency
make test_dss_post_bist_latency_sweep
```

Phase 4I-1 remained PASS.  The 10,000-trace Phase 4I-2 sweep remained PASS
with zero monotonicity, negative-latency, invalid-timestamp, and G=0
interpretation violations.

Artifacts:

- [`phase4i3_rtl_timing.csv`](../results/phase4i/phase4i3_rtl_timing.csv)
- [`phase4i3_rtl_cycle_trace.csv`](../results/phase4i/phase4i3_rtl_cycle_trace.csv)
- [`recam_dss_v2_phase4i_rtl_timing_test.cpp`](../tb/dss_v2/recam_dss_v2_phase4i_rtl_timing_test.cpp)

```text
EARLY_RTL_CASES = 4
EARLY_RTL_MISMATCHES = 0
GROUP_RTL_CASES = 4
GROUP_RTL_MISMATCHES = 0
RTL_EVENT_MODEL_MISMATCHES = 0

SAME_EDGE_ZERO_LATENCY_VALIDATED = YES
NON_DOMINANT_EARLY_PATH_VALIDATED = YES
LONG_GROUP_PATH_VALIDATED = YES

PHASE_4I_3_RTL_VALIDATION = PASS
READY_FOR_PHASE_4I_4 = YES

V2_PHASE = PHASE_4I
V2_NEXT_PHASE_AUTHORIZED = YES
```

Phase 4I-4 was not started.
