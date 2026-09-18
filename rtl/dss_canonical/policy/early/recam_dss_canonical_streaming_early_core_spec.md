# `recam_dss_canonical_streaming_early_core` specification

## Purpose and ancestry

This is the canonical decision/resource core for
`NORMALIZED_STREAMING_EARLY`.  It is adapted from the historical
`recam_dss_v2_early_core`, while reusing the verified V2 configuration tables,
directional topology/feasibility blocks, physical resource ledger, and ledger
diagnostic adapter.  The policy change is limited to the frozen semantic
priority `RELEASE_ONLY`, `LOCAL`, `RELEASE_AND_BORROW`, `BORROW_ONLY` for every
SA.

## Interface contract

- `clk_i`: rising-edge clock.
- `rst_ni`: synchronous active-low reset for the controller and reused ledger.
- `start_i`: accepted only while `busy_o=0`; starts SA A at RELEASE_ONLY.
- `candidate_solution_valid_i`, `candidate_repairable_i`: current analyzer
  result qualifiers.
- `candidate_pattern_id_i`: one-based canonical PatternID for the current
  action; zero is invalid by contract.
- `current_sa_o`, `current_action_o`, `current_config_id_o`: request presented
  to the external analyzer.
- `done_o`: one-cycle terminal pulse.
- `group_repairable_o`: terminal result, retained until the next start.
- selected/action/ledger outputs: deterministic decision trace and final live
  ledger state.

`RESOURCE_POINT=2` binds the frozen DATE RS2 table and topology.
`RESOURCE_POINT=3` binds the isolated RS3/CS3/m1 table and topology.
`PATTERN_ID_W=6` is the common width; RS2 inputs occupy values 1 through 10.

## Cycle behavior

On the edge accepting `start_i`, the controller enters SEARCH.  During SEARCH,
one action is evaluated per cycle.  A locally valid and ledger-legal candidate
is committed atomically on the next rising edge.  The next SA then starts at
RELEASE_ONLY.  Four rejected actions terminate the group at the current SA.
Previously committed ledger state is retained on failure; there is no rollback
or backtracking.

## Constraints and corner cases

- Exactly four SAs are traversed in A/B/C/D order.
- Action codes are semantic slot IDs: `0=L`, `1=R`, `2=B`, `3=RB`.
- PatternID order is supplied by the analyzer; this core receives the lowest
  valid PatternID for the current action.
- A donor field is meaningful only when the corresponding `borrow_flat_o` bit
  is set; otherwise the stored donor is zero.
- The reused ledger must be reset before an independent group transaction.

## Verification cases

- E1: RELEASE_ONLY and LOCAL valid; RELEASE_ONLY commits.
- E2: RELEASE_ONLY invalid and LOCAL valid; LOCAL commits.
- E3: RELEASE_AND_BORROW and BORROW_ONLY valid; RB commits.
- E4: later-SA exhaustion retains the committed prefix.
- E5: two donors available; frozen primary donor wins.
- W1: reconstructed N2/F16/group172 candidate map fails at C and retains the
  committed A/B prefix.
- Seeded random candidate maps compare action, ConfigID, PatternID, donor,
  release/borrow, every post-commit ledger state, failure position, and result.

## Timing diagram

```text
id: early_release_reject_local_accept
title: RELEASE_ONLY fallback to LOCAL
scenario: SA A rejects RELEASE_ONLY, accepts LOCAL, and advances to SA B
description: One analyzer result is consumed per cycle; the LOCAL decision is
             committed on the rising edge and is immediately visible to B.
```

```json
{
  "signal": [
    {"name": "clk_i", "wave": "p....."},
    {"name": "start_i", "wave": "010..."},
    {"name": "busy_o", "wave": "01...."},
    {"name": "current_sa_o", "wave": "x=.=..", "data": ["A", "A", "B"]},
    {"name": "current_action_o", "wave": "x=.=..", "data": ["R", "L", "R"]},
    {"name": "candidate_solution_valid_i", "wave": "0.10.."},
    {"name": "sa_commit_valid_o[A]", "wave": "0..1.."}
  ]
}
```
