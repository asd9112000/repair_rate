# S1G-A — EARLY_OVERLAP Retained-State and Control Implementation

`S1GA_STATUS: COMPLETE`
`Scope: DATE 2x2 directional CAM, RS=2, CS=2, SHARE_M=1`

## Scope and preserved boundary

This change adds `recam_dss_v2_early_overlap_core` as a separate
`EARLY_OVERLAP` implementation.  It does not alter
`recam_dss_v2_early_core` (`EARLY_BASELINE`), the GROUP_NO_SCRATCH_V2 path,
the shared analyzer algorithm, or the canonical resource ledger semantics.
The S1G-A boundary is a verification-facing serialized retained-summary
interface, not a raw-fault or BIST interface:

```text
future raw-fault/BIST producer (S1G-B; not implemented)
    -> summary_update_valid, summary_update_sa, summary_payload
    -> EARLY_OVERLAP retained state / single analyzer sweep
    -> test_done_valid, test_done_sa
```

`summary_update_valid_i` atomically replaces one SA snapshot.  Each
`test_done_valid_i` marks the corresponding snapshot final for the current
transaction.  The interface supports independent updates for A, B, C, and D;
it has no raw-fault input.

## Frozen 204-bit analyzer-ready summary packing

The packing was derived from the ports of
`rtl/recam/recam_shared_config_analyzer.sv`, not assumed from its nominal
width.  `SUMMARY_W` is derived from the analyzer parameters and has an
elaboration-time assertion that it equals 204 for the frozen DATE setting.

| Field | Width | Bits | Analyzer source input | Meaning |
| --- | ---: | --- | --- | --- |
| `pivot_valid` | 5 | `[4:0]` | `pivot_valid_i` | Valid compact pivot-prefix entries. |
| `pivot_rows_flat` | 45 | `[49:5]` | `pivot_rows_flat_i` | Five 9-bit pivot rows. |
| `pivot_cols_flat` | 25 | `[74:50]` | `pivot_cols_flat_i` | Five 5-bit pivot columns. |
| `row_gt1`, `row_gt2`, `row_gt3` | 15 | `[89:75]` | `row_gt*_i` | Row Must predicates. |
| `col_gt1`, `col_gt2`, `col_gt3` | 15 | `[104:90]` | `col_gt*_i` | Column Must predicates. |
| `hybrid_valid` | 7 | `[111:105]` | `hybrid_valid_i` | Valid hybrid entries. |
| `hybrid_pointer_flat` | 21 | `[132:112]` | `hybrid_pointer_flat_i` | Seven 3-bit pivot pointers. |
| `hybrid_descriptor` | 7 | `[139:133]` | `hybrid_descriptor_i` | Hybrid differing-dimension descriptor. |
| `hybrid_differing_flat` | 63 | `[202:140]` | `hybrid_differing_flat_i` | Seven 9-bit differing addresses. |
| `conventional_overflow` | 1 | `[203]` | `conventional_overflow_i` | Conventional-repair overflow constraint. |
| **Total** | **204** | **`[203:0]`** | | **One complete SA snapshot.** |

## Retained state and control

The core retains four 204-bit snapshots.  `GENERATION_W=4` supports the
frozen bounded transaction assumption and increments independently on every
summary replacement.  Dirty is represented without a separate flop:

```text
dirty[SA] = !analysis_valid[SA]
         || analysis_generation[SA] != fault_generation[SA]
```

The owner performs exactly one four-slot role-aware sweep.  The frozen slot
mappings are A/D: `0,4,5,6`; B/C: `0,2,1,3`.  Candidate validity and the
representative PatternID for all four slots are retained only for the active,
generation-matched owner.  A new owner update clears that bank and restarts at
slot zero.  A B/C/D update while A is owner only changes that SA's retained
summary/generation; it cannot touch the ledger or start final selection.

New summary has explicit precedence over slot-three completion.  Thus an old
completion cannot set `analysis_valid` or become commit eligible on the same
edge as an update.  An analysis result can be finalized only when its recorded
generation equals the current retained-summary generation.

The resource-feasibility module is evaluated only after a complete local
candidate bank and `test_done`.  The canonical ledger is driven by
`commit_now` only.  There is no provisional release, borrow, donor
reservation, ledger update, or rollback.  Successful commits alone advance
ownership A -> B -> C -> D.  A failed current SA terminates the group, so
later retained summaries/results are ignored and cannot commit.

### Exact logical-state accounting

| State category | Bits | Accounting |
| --- | ---: | --- |
| Summary storage | 816 | 4 x 204-bit independent summaries. |
| Generation | 32 | 4 x (`fault_generation[3:0]` + `analysis_generation[3:0]`). |
| Candidate bank | 20 | Four valid bits and four 4-bit PatternIDs. |
| Owner/FSM | 7 | Owner SA (2), role slot (2), 3-bit FSM. |
| Other functional control | 8 | `analysis_valid[3:0]` and `test_done[3:0]`; dirty is derived. |
| Functional retained/control total | **883** | Excludes existing normal EARLY outputs and existing ledger state. |
| Debug-event flops | 13 | `SUMMARY_UPDATE[3:0]`, `LATEST_CANDIDATE_READY[3:0]`, `TEST_DONE[3:0]`, `FINAL_LEDGER_DECISION_READY`. |
| **Total additional sequential implementation state** | **896** | Functional state plus testbench/debug event observability. |

The S1F functional estimate was 888 bits, including a separate
`sweep_active` bit and a four-bit captured sweep epoch.  The implementation
does not need either: update priority prevents an old completion and the
per-SA `analysis_generation` records the completed generation.  It is five
functional bits below that estimate.  Including the 13 explicit debug-event
flops makes the physical implementation count 896 bits, eight above the S1F
estimate; those flops are non-functional observability only.

## Verification

Dedicated testbench files:

```text
tb/dss_v2/recam_dss_v2_early_overlap_equivalence_test_top.sv
tb/dss_v2/recam_dss_v2_early_overlap_equivalence_test.cpp
```

The testbench retains the final injected snapshots separately and starts the
untouched `EARLY_BASELINE` only after those final snapshots are present.  It
compares finalized repairability, ConfigID, PatternID, donor/action, canonical
ledger encoding, and terminal failure position.  It intentionally does not
compare provisional sweep choices cycle by cycle.

The directed tests cover all-local/zero-summary success; terminal failure at
each A/B/C/D position; non-owner B/C/D summary retention; test-done-before
analysis; analysis-before-test-done; an owner update while sweeping; the
slot-three update/completion collision; and three consecutive owner updates
(generations 1, 2, and 3) before the final decision.  Deterministic final-state
action witnesses covered release, borrow, and release+borrow and remained
checked against `EARLY_BASELINE`.

```text
S1GA_DIRECTED total=8 pass=8 fail=0
S1GA_RANDOM vectors=1000 seed=20260915
  repairable_mismatches=0
  ConfigID_mismatches=0
  PatternID_mismatches=0
  action_mismatches=0
  ledger_mismatches=0
  failure_position_mismatches=0
S1GA_ACTION_COVERAGE release=56 borrow=7 release_borrow=1
S1GA_TIMING_RANDOM vectors=1000 seed=20260916 final_result_mismatches=0
S1GA_RESTART_STRESS PASS
```

The timing-randomized campaign changes summary-update and `test_done` timing
while retaining the same final state for each transaction.  It makes no
physical-time or latency claim.

The following frozen regressions were also run after the S1G-A addition:

```text
bash scripts/simulation/run_recam_phase4e3_random.sh
bash scripts/simulation/run_dss_v2_phase4f3_group.sh
make test_solution_take_policy
make test_dss_formal_latency_preflight
```

All commands completed successfully.  The EARLY baseline 50 directed + 1000
random vectors reported zero ConfigID, PatternID, action, ledger, and failure
position mismatches.  The GROUP_NO_SCRATCH_V2 and S0R regressions passed; the
E0-L support preflight reported `dss_formal_latency_preflight_test PASS`.

No synthesis, formal-latency experiment, repair-rate experiment, E0-R work,
or S1G-B raw-fault/BIST producer work was performed.

## Phase-control record

```text
S1GA_STATUS: COMPLETE

FILES_ADDED:
  rtl/dss_v2/top/recam_dss_v2_early_overlap_core.sv
  tb/dss_v2/recam_dss_v2_early_overlap_equivalence_test_top.sv
  tb/dss_v2/recam_dss_v2_early_overlap_equivalence_test.cpp
  docs/dss_execution/S1GA_EARLY_OVERLAP_RETAINED_STATE_CONTROL_IMPLEMENTATION.md

FILES_MODIFIED: none
BASELINE_EARLY_CHANGED: NO
GROUP_CHANGED: NO
REPAIR_ALGORITHM_CHANGED: NO

SUMMARY_INPUT_MODEL: serialized per-SA 204-bit analyzer-ready snapshot replacement
SUMMARY_BITS_PER_SA: 204
TEST_DONE_INTERFACE: test_done_valid_i + test_done_sa_i, sticky per transaction

SUMMARY_STORAGE_BITS: 816
GENERATION_BITS: 32
CANDIDATE_BANK_BITS: 20
OWNER_FSM_BITS: 7
OTHER_BITS: 8 functional + 13 debug-event observability
TOTAL_ADDITIONAL_STATE_BITS: 896 (883 functional retained/control)
S1F_ESTIMATE: 888
DELTA_FROM_S1F_ESTIMATE: +8 including debug events; -5 functional-only

NEW_UPDATE_INVALIDATES_OLD_RESULT: PASS
SAME_CYCLE_UPDATE_VS_DONE_PRIORITY: PASS
LATEST_GENERATION_ONLY_COMMIT: PASS

A_TO_B_TO_C_TO_D: PASS
LATER_SA_RETENTION_WHILE_NON_OWNER: PASS
FAILURE_FLUSH_OR_IGNORE: PASS

PROVISIONAL_LEDGER_UPDATE: NO
LEDGER_ROLLBACK: NO
FINAL_COMMIT_ONLY: PASS

DIRECTED_TESTS: total=8 pass=8 fail=0
RANDOM_FUNCTIONAL_EQUIVALENCE: vectors=1000, seed=20260915, all required mismatch counts=0
TIMING_RANDOMIZED_EQUIVALENCE: vectors=1000, seed=20260916, final_result_mismatches=0
RESTART_STRESS: PASS

BASELINE_EARLY: PASS
GROUP_NO_SCRATCH_V2: PASS
S0R: PASS

RAW_FAULT_TO_SUMMARY_INTERFACE_DEFINED: YES
S1GB_ELIGIBLE: YES
S1GB_AUTHORIZED: NO
S1H_AUTHORIZED: NO
S1I_AUTHORIZED: NO
S1J_AUTHORIZED: NO
E0R_MAY_RESUME: NO
NEXT_PHASE_AUTHORIZED: NONE
```

`GIT_DIFF_CHECK: PASS`
