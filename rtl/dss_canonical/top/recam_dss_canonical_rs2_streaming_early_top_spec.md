# `recam_dss_canonical_rs2_streaming_early_top` specification

## Purpose and ancestry

This module is the RS2 analyzer-integrated boundary for
`NORMALIZED_STREAMING_EARLY`.  It directly reuses
`recam_shared_config_analyzer` and binds its lowest-valid PatternID result to
`recam_dss_canonical_streaming_early_core`.  No analyzer, candidate-universe,
topology, or ledger behavior is reimplemented here.

## Interface contract

The collector/analyzer inputs and address-width parameters are identical to
the historical RS2 V2 EARLY top.  `current_sa_o`, `current_action_o`, and
`current_config_id_o` identify the analyzer request in progress.  The result
and trace outputs expose the selected semantic action, ConfigID, PatternID,
donor, release/borrow flags, failure position, and physical ledger.

Reset is synchronous active-low.  `start_i` is accepted only when `busy_o` is
low.  The external owner must present the analyzer snapshot for
`current_sa_o`; that snapshot must remain stable for the action evaluation
cycle.

## Cycle behavior

The top adds no pipeline stage.  Analyzer outputs are combinational.  The
policy core evaluates one semantic action per cycle and atomically commits a
legal candidate on the rising edge.  `done_o` pulses for one cycle on the
fourth commit or on current-SA exhaustion.

## Constraints and corner cases

- Resource point is frozen to RS=CS=2, directional m=1.
- The existing analyzer's 10-bit valid bitmap remains observability metadata;
  Streaming EARLY consumes its deterministic lowest valid PatternID.
- There is no candidate-history storage and no rollback/backtracking.
- Independent groups require ledger reset before the next `start_i`.

## Verification cases

The policy core is checked by E1–E5 and seeded random candidate maps.  This top
must additionally pass compile/lint with the unchanged historical analyzer and
the historical analyzer regression before synthesis is authorized.

## Timing diagram

```text
id: rs2_analyzer_release_commit
title: RS2 analyzer-to-policy RELEASE_ONLY commit
scenario: analyzer-valid RELEASE_ONLY commits for SA A
description: The combinational analyzer result is qualified and captured by
             the canonical policy/ledger on the next rising edge.
```

```json
{
  "signal": [
    {"name": "clk_i", "wave": "p...."},
    {"name": "start_i", "wave": "010.."},
    {"name": "current_sa_o", "wave": "x=...", "data": ["A"]},
    {"name": "current_action_o", "wave": "x=...", "data": ["R"]},
    {"name": "analyzer_solution_valid", "wave": "0.1.."},
    {"name": "sa_commit_valid_o[A]", "wave": "0..1."},
    {"name": "current_sa_o", "wave": "x..=.", "data": ["B"]}
  ]
}
```
