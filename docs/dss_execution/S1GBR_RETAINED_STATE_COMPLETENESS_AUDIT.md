# S1G-BR — EARLY_OVERLAP Retained-State Completeness Audit

`S1GBR_STATUS: COMPLETE`
`Scope: read-only, DATE 2x2 directional CAM, RS=2, CS=2, SHARE_M=1`

## Conclusion

The current 204-bit value is a complete representation of the **frozen V2
analyzer input boundary**, but it is not a lossless representation of every
historical RECAM collector state.  This distinction is source-defined, not an
inference: `docs_verilog/PHASE3B_ANALYZER_INTERFACE.md` calls the boundary
“Address CAM logical state + Hybrid CAM logical state + Must state -> Matrix
Builder -> candidate evaluation -> PatternID” and explicitly excludes
collector generation/storage and reusable-CAM accounting.

Consequently, no source supports an Option-1 proof that the 204-bit value is
complete for a raw-fault implementation of all historical RECAM semantics.
It can be complete only under the already-defined restricted V2
candidate-analysis boundary.  That boundary does not carry CAM-reuse repair
payload, so it cannot by itself close S1G-B's requested raw-fault end-to-end
proof.

## State trace

| State | Width / SA | Source | Populated | Consumed before Config/Pattern evaluation | In 204 bits |
| --- | ---: | --- | --- | --- | --- |
| Pivot valid/row/ColumnWord | 5 + 45 + 25 | `shared_pivot_cam` | New independent pivot | Yes | Yes, 75 bits |
| Row/column counters | 168 + 120 | `shared_fault_counter` | Every accepted stored fault | Yes, through Must thresholds | Condensed to 30 threshold bits |
| Hybrid state | 7 x 26 = 182 with frozen 7-entry instance | `tagged_hybrid_store` | Related fault | Yes | Condensed to 98 logical bits; no raw row/column pair or cfg tag |
| Overflow | collector subblocks | pivot/counter/hybrid/reuse blocks | Capacity event | Yes, only as analyzer overflow input | One bit |
| CAM-reuse state | 12 x (valid + row[8:0] + col[4:0] + cfg[6:0]) = 264 | `cam_reuse_temp_buffer` | Additional pivot relevant to a smaller Config | No, in the Phase-3B Config/Pattern evaluator | **No** |
| Pending repair payload | includes retained CAM-reuse entries | `pending_repair_buffer` | After group selection | After Config/Pattern selection, for repair-program payload | **No** |

The 204-bit packing is exactly 75 pivot bits + 30 Must bits + 98 Hybrid bits
+ 1 overflow bit.  It is therefore an analyzer snapshot, not a raw collector
snapshot.

## Historical CAM-reuse behavior

`shared_fault_collector` classifies an accepted fault as an additional
CAM-reuse entry when `pivot_occupancy_o >= k(config)` for one or more Configs.
This happens **before** physical pivot storage is globally full: after four
independent pivots, ConfigIDs with `k=3` already mark the fourth pivot for
reuse; after five pivots, the fifth is similarly relevant to smaller Configs.
The existing collector regression proves this directly: five independent
faults produce two CAM-reuse entries and the expected ConfigID mask.

CAM reuse is frozen historical RECAM collection semantics and is reachable by
legal DATE addresses.  The smallest witness is four mutually row- and
column-distinct faults, for example:

```text
(row,col) = (0,0), (1,1), (2,2), (3,3)
```

For `k=3` configurations, the fourth pivot is recorded in
`cam_reuse_temp_buffer` with its Config membership; the 204-bit summary has
no bit range for that record.  This is a concrete **collector-state loss**.

It is not, however, a valid ConfigID/PatternID mismatch counterexample for
the frozen V2 analyzer: the historical `multi_config_analyzer_bank` consumes
the Pivot/Must/Hybrid view and does not read `cam_reuse_temp_buffer`; the
later `pending_repair_buffer` retains CAM-reuse payload only after selection.
Therefore the requested “historical Config/Pattern result != 204-bit
reconstructed result” counterexample is not established by source evidence.
Claiming one would conflate candidate analysis with later repair-program
payload.  The missing state nevertheless prevents a lossless *full RECAM
collection* reconstruction.

## DATE reachability

The frozen E0-L DATE corpus has seven faults per SA, mixed spatial placement,
and no contract restricting pivots to three or fewer.  Its fault model is
therefore not proof of non-reachability; four independent faults are legal.

```text
CAM_REUSE_REACHABILITY_IN_DATE_2X2: REACHABLE
```

The historical collector's entry limit (`MAX_FAULTS=12`) is not reached by
the seven-fault E0-L per-SA count, but CAM reuse can occur well below that
limit.  Previous random regressions do not alter this conclusion.

## Option and implementation impact

Option 1 is valid only for the restricted, already-frozen statement:

```text
204 bits completely represent the V2 matrix/candidate analyzer input.
```

It is not valid for the stronger S1G-B statement “raw faults -> complete
historical RECAM collection semantics -> overlap.”  A complete retained
snapshot for the latter minimally needs the existing 204 bits plus the
historical CAM-reuse records: **264 additional bits per SA**, yielding 468
bits per SA and 1872 bits for four SAs before any existing S1G-A control or
producer control state.  This is a semantic-state lower bound, not a
synthesis result.

That extension requires a retained-state and post-selection repair-payload
interface extension.  Source evidence does **not** show that the current
V2 Config/Pattern analyzer datapath must change, because it does not consume
CAM reuse.  It does require reopening S1G-A's snapshot/interface contract,
generation test, state accounting, and equivalence definition if S1G-A is to
claim complete raw RECAM semantics.  It does not justify any repair-algorithm
change or early-overflow rule.

```text
S1GBR_STATUS: COMPLETE
CAM_REUSE_IS_FROZEN_RECAM_SEMANTICS: YES
CAM_REUSE_REACHABLE_IN_DATE_2X2: YES
CURRENT_204BIT_SUMMARY_COMPLETE: CONDITIONAL
OPTION_1_204BIT_PROOF: CONDITIONAL
OPTION_2_CONTRACT_EXTENSION_REQUIRED: YES
OPTION_3_EARLY_OVERFLOW_WOULD_CHANGE_SEMANTICS: YES
MINIMUM_COMPLETE_BITS_PER_SA: 468 (204 + 264 CAM-reuse records)
ANALYZER_DATAPATH_CHANGE_REQUIRED: NO for current Config/Pattern evaluation; PARTIAL for complete repair payload
REPAIR_ALGORITHM_CHANGE_REQUIRED: NO
S1GA_MUST_REOPEN: YES
S1GB_MAY_RESUME: NO
NEXT_PHASE_AUTHORIZED: NONE
```

`GIT_DIFF_CHECK: FAIL — current worktree reports Makefile:453 blank line at EOF; this audit did not modify Makefile.`
