# Device-Level Experiment Roadmap

> Current status ledger for device/timing work. This minimal roadmap was
> created because no prior file with this required name exists.

## Closed prerequisites

    T0: CLOSED
    T0B: CLOSED
    T1R: CLOSED
    T1: COMPLETE

## Current review gate

    T2_HIERACHIRECAM_TIMING_INTEGRATION: READY_FOR_REVIEW / NOT_YET_STARTED

T1 now has a standalone ownership-aware event-driven timing model with a
cycle-reference oracle, 15 directed checks, 1,000 randomized legal traces,
and one compared S1G-A2G RTL-harness trace at zero timing mismatches. Its
candidate-ready model remains distinct from final repair/ledger semantics.

## Preserved holds

    S1G-A2G synthesis: PAUSED
    T2_HIERACHIRECAM_TIMING_INTEGRATION: READY_FOR_REVIEW / NOT_YET_STARTED
    device-level formal simulation: NOT_AUTHORIZED
    repair-rate experiment: NOT_AUTHORIZED
