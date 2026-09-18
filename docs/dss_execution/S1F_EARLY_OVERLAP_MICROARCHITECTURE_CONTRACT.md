# S1F — EARLY BIST / Analysis Overlap Microarchitecture Contract

`S1F_STATUS: COMPLETE`
`SOURCE_MODIFIED: NO` (implementation source; this contract document is new)
`E0-R: PAUSED`

## Scope and classification

S1F freezes a future `EARLY_OVERLAP` controller for DATE 2x2 directional-CAM
(`RS=2`, `CS=2`, `SHARE_M=1`). It does not alter `EARLY_BASELINE`, which
remains the E0-L reference. The preserved behavior is A -> B -> C -> D,
role-aware priority, commit only for a final SA, first-failure termination,
no rollback, ConfigID/PatternID semantics, release/borrow rules, and canonical
PhysicalResourceLedger behavior. No RTL, C++ repair semantics, BIST behavior,
experiment, synthesis, or E0-R work was performed.

`rtl/recam/recam_shared_config_analyzer.sv` is stateless combinational logic.
A new complete analyzer snapshot therefore re-evaluates without stale state
inside the analyzer. The frozen V2 EARLY top has no raw-fault, BIST, or
per-SA snapshot interface; the historical single collector clears at
`subarray_start_i`, while legacy `dss_cam_top` stops collection during
analysis. Overlap needs retained per-SA state and control, not a harness-only
change.

```text
EARLY_OVERLAP_CLASS: CONTROL_PLUS_SMALL_STATE_EXTENSION
REPAIR_SEMANTICS_PRESERVED: CONDITIONAL
LEDGER_ROLLBACK_REQUIRED: NO
```

The condition is that only a final, generation-matched candidate result may
commit, and that baseline candidate ordering and failure behavior are retained.

## Fault-dependent versus ledger-dependent partition

| Stage | Current module/boundary | Ledger-dependent | Before test done | Retain safely | Recompute after ledger change |
| --- | --- | --- | --- | --- | --- |
| Fault summary | V2 EARLY top analyzer-input contract | No | Yes | Per-SA, versioned | No |
| Matrix, pivot, Must, hybrid interpretation | `recam_shared_config_analyzer` | No | Yes | Generation matched only | No |
| Config feasibility and PatternID | `recam_shared_config_analyzer` | No | Yes | All role slots | No |
| Role-aware slot / descriptor decode | EARLY core / legacy adapter | No | Yes | Fixed/recomputable | No |
| Resource feasibility, donor, release/borrow | `dss_v2_resource_feasibility` | Yes | Not final | No final action | Yes |
| Final role-slot selection | EARLY rank walk | Yes | Not final | Candidate bank only | Yes |
| Ledger commit | `dss_v2_resource_ledger` | Yes | No | Final persistent state | N/A |

The analyzer must sweep and retain all four role-aware local candidate slots.
The first locally valid Config cannot alone be retained because a later slot
may be necessary when the former is infeasible against committed resources.

## Per-SA retained fault state

The selected minimum is **C: analyzer-ready pivot + threshold + hybrid +
overflow summary**. It exactly covers frozen V2 analyzer inputs and is smaller
than a raw-fault CAM at the analyzer boundary. A future raw-fault producer must
prove candidate equivalence and preserve the compact pivot-prefix contract; no
such production V2 producer currently exists.

| Field | Bits per SA | Existing input purpose |
| --- | ---: | --- |
| `pivot_valid` | 5 | Pivot prefix validity. |
| `pivot_rows_flat` | 45 | Five 9-bit row addresses. |
| `pivot_cols_flat` | 25 | Five 5-bit column addresses. |
| `row_gt1`, `row_gt2`, `row_gt3` | 15 | Row Must predicates. |
| `col_gt1`, `col_gt2`, `col_gt3` | 15 | Column Must predicates. |
| `hybrid_valid` | 7 | Hybrid membership. |
| `hybrid_pointer_flat` | 21 | Seven 3-bit pivot references. |
| `hybrid_descriptor` | 7 | Differing-dimension metadata. |
| `hybrid_differing_flat` | 63 | Seven 9-bit differing addresses. |
| `conventional_overflow` | 1 | Local overflow constraint. |
| **Analyzer-ready summary** | **204** | **All frozen analyzer inputs.** |

Let `G = ceil(log2(MAX_ACCEPTED_FAULTS_PER_SA + 1))`. Per SA add
`fault_generation[G]`, `analysis_generation[G]`, `analysis_valid`, and
`sa_test_done`; `fault_state_dirty` is derived, not stored:

```text
!analysis_valid || analysis_generation != fault_generation
```

For the existing bounded historical default of 12 accepted entries, `G=4`:

```text
bits_per_sa: 204 + 4 + 4 + 1 + 1 = 214
four_SAs: 856
```

The shared state estimate is 32 bits: four valid bits plus four 4-bit
PatternIDs (20), owner (2), role slot (2), sweep-active (1), captured epoch
(4), and FSM (3). Total estimated retained state is **888 bits** at `G=4`,
excluding producer-internal raw-fault storage and existing ledger/output
registers. General form:

```text
4 * (204 + 2G + 2) + (28 + G) bits
```

## Generation, restart, and ownership contract

Candidate data for SA `i` are valid only when:

```text
analysis_valid[i]
&& analysis_generation[i] == fault_generation[i]
&& analyzer_owner_sa == i
```

On every accepted fault for SA `i`: update that SA’s summary, increment
`fault_generation[i]`, clear `analysis_valid[i]`, and, if it is owner, discard
the partial/current sweep and restart from role slot zero. Restart is a **full
four-slot role-aware sweep** because the frozen analyzer evaluates one Config
at a time; it is not merely observing a changed combinational output.

New fault wins on a same-edge fault-accept/sweep-complete collision: update the
generation and clear validity, discard the old completion, then restart from
the new snapshot. `G` must not wrap between sweep launch and completion.

```text
IDLE/owner A --A commit--> owner B --B commit--> owner C
             --C commit--> owner D --D commit--> GROUP_DECISION_READY

previous_sa_committed(i) = (i == A) || sa_commit_valid[i - 1]
```

B/C/D BIST may progress independently while A owns the analyzer, but each
must update its own retained summary. A terminal failure at SA `i` discards or
ignores all later provisional state, issues no later commit, and never advances
owner state. This preserves first-failure semantics.

## Commit contract

For owner `i`, role slot `r`:

```text
commit_allowed(i,r) =
    previous_sa_committed(i)
 && sa_test_done[i]
 && analysis_valid[i]
 && analysis_generation[i] == fault_generation[i]
 && candidate_local_valid[i,r]
 && descriptor_valid(i,r)
 && resource_state_valid
 && resource_feasible(i,r)
```

`dss_v2_resource_feasibility` is combinational from the committed ledger. It
derives donor and action before the edge; `dss_v2_resource_ledger` samples the
same transaction at the commit edge. S1F freezes this same-cycle
feasibility/commit relation. ConfigID, PatternID, donor, and action are
captured only with that accepted commit. Ledger-only rejection advances to the
next frozen slot; it does not restart fault analysis.

## Frozen S1J event and latency contract

| Event | Definition |
| --- | --- |
| `SA_LAST_FAULT_ACCEPT[i]` | Last accepted raw-fault update edge for SA `i`. |
| `LATEST_CANDIDATE_READY[i]` | Edge capturing all four candidate slots with matching generation. |
| `SA_TEST_DONE[i]` | Final BIST-address completion edge for SA `i`. |
| `FINAL_LEDGER_DECISION_READY[i]` | Internal final-slot and committed-ledger action qualification; not remap-ready. |
| `SA_COMMIT[i]` | Ledger-accepted final ConfigID/PatternID/action edge. |
| `GROUP_DECISION_READY` | D commit or terminal first-failure controller-decision edge. |

```text
CANDIDATE_CATCHUP_LATENCY[i]
  = LATEST_CANDIDATE_READY[i] - SA_LAST_FAULT_ACCEPT[i]

HIDDEN_ANALYSIS_SLACK[i]
  = SA_TEST_DONE[i] - LATEST_CANDIDATE_READY[i]

EARLY_CONTROLLER_FINAL_SOLUTION_READY = GROUP_DECISION_READY
EARLY_GROUP_EXPOSED_POST_BIST_LATENCY
  = GROUP_DECISION_READY - GROUP_TEST_DONE
```

Hidden slack is signed: positive is hidden work, zero is coincidence, negative
is analysis that extends past test completion. The group formula is neither a
sum nor a maximum of SA values: it includes the A -> B -> C -> D dependency.
It remains an internal controller decision metric, distinct from repair-table
or runtime-remap release.

For an SA scanned from address 1 through 100 with faults at 5, 10, and 15,
each acceptance invalidates and restarts the sweep. The first no-new-fault
completion after address 15 becomes `LATEST_CANDIDATE_READY`; its numerical
cycle is intentionally not invented. At address 100, final commit waits for
the matching result and final ledger control. A late fault at 98 invalidates
the prior result; a replacement ready after 100 yields negative hidden slack
and nonzero exposed latency.

## Future verification and phase control

S1G/S1H must prove exact `EARLY_OVERLAP` versus `EARLY_BASELINE` agreement for
identical complete fault groups: repairability, committed ConfigID/PatternID,
release/borrow and donor action, ledger state, failure position, and no commits
after first failure. Required directed cases include all role priorities,
ledger-rejected earlier slots, late faults, same-edge races, and A/B/C/D
failures. Candidate equivalence of the new producer to this 204-bit contract
is a separate prerequisite gate.

The retained summary/generation/test-done structure has
`GROUP_REUSE_POTENTIAL: MEDIUM`; GROUP still needs distinct candidate history
and allocation behavior.

```text
LATER_SA_BIST_INDEPENDENT: YES
FAULT_RETENTION_WHILE_NOT_OWNER: REQUIRES_NEW_STATE
ANALYZER_RESTART_MODEL: full four-slot sweep from newest generation-matched summary

additional_state_bits: 888 at G=4, plus producer-internal raw-fault state
additional_control: per-SA update routing, snapshot mux, generation compare,
  restartable sweep, owner FSM, test-done gating, and terminal-failure flush
synthesis_required_later: YES

S1G_IMPLEMENTATION_ELIGIBLE: PARTIAL
S1G_AUTHORIZED: NO
E0R_MAY_RESUME: NO
NEXT_PHASE_AUTHORIZED: NONE
```
