# S1G-B — EARLY_OVERLAP Raw-Fault / BIST Integration

`S1GB_STATUS: BLOCKED`

## Blocking source evidence

S1G-A's frozen 204-bit input has five pivots and seven Hybrid records, but no
CAM-reuse payload.  The closest raw-fault implementation,
`rtl/dss_2x2/analyzer/shared_fault_collector.sv`, is therefore not a direct
producer for that contract:

| Existing block | Classification | Evidence |
| --- | --- | --- |
| `shared_fault_collector` | `REUSE_LOGIC_ONLY` | Its state is cleared by `subarray_start_i` in `dss_analyzer_top`; it also delegates extra pivots to CAM reuse. |
| `shared_pivot_cam` / `shared_fault_counter` / `tagged_hybrid_store` | `REUSE_LOGIC_ONLY` | Their pivot, Must, and Hybrid rules are useful source semantics, but the resulting state is not the 204-bit contract alone. |
| `dss_cam_top` / `fault_collector` | `NOT_SUITABLE` | It stops collection before analysis and is not a V2 summary producer. |
| `SerialBistSchedule` | `REUSE_WITH_WRAPPER`, `MODEL_DRIVEN` | It supplies C++ schedule events only; no V2 RTL valid/ready producer exists. |

For a sixth unique pivot, `shared_fault_collector` does not necessarily assert
its visible overflow.  It records the fault in `cam_reuse_temp_buffer` when a
configuration reaches its pivot capacity.  The S1G-A summary cannot represent
that buffer.  Mapping this state to either “ignore” or immediate
`conventional_overflow` would respectively discard a legal fault or change
the frozen candidate-analysis semantics.  Both are prohibited by S1G-B.

The existing four-collector/summary-wrapper prototype was removed before
closure because it had exactly this lossy mapping.  No production or baseline
RTL was modified.

## Required resolution before a retry

One explicit authority decision is needed:

1. prove that the S1G-A 204-bit summary contract is intentionally restricted
   to a source model with no CAM-reuse semantics, and supply that authoritative
   raw-summary construction; or
2. authorize a revised retained-state/analyzer contract that represents
   CAM-reuse semantics and re-open S1G-A equivalence; or
3. define a semantically justified terminal-overflow rule at the first
   unrepresentable pivot, then re-verify it against the frozen baseline.

Until then, raw valid/ready, per-SA `test_done`, and serialized BIST replay
may be planned only as `MODEL_DRIVEN`; they cannot establish an end-to-end
semantics-preserving path.

```text
S1GB_STATUS: BLOCKED
FILES_ADDED:
  docs/dss_execution/S1GB_EARLY_OVERLAP_RAW_FAULT_BIST_INTEGRATION.md
FILES_MODIFIED: none
BASELINE_EARLY_CHANGED: NO
GROUP_CHANGED: NO
REPAIR_ALGORITHM_CHANGED: NO

RAW_FAULT_INPUT_CONTRACT: NOT_FINALIZED; historical valid/ready exists outside frozen V2
FAULT_ACCEPT_EVENT: fault_valid_i && fault_ready_o (historical collector only)
PRODUCER_REPRESENTATION: UNRESOLVED
PER_SA_STATE_INDEPENDENT: REQUIRED, NOT_IMPLEMENTED
SUMMARY_BITS_PER_SA: 204
SUMMARY_BIT_EXACT_EQUIVALENCE: FAIL (missing CAM-reuse representation)
ATOMIC_SUMMARY_UPDATE: NOT_IMPLEMENTED
PER_SA_TEST_DONE: NOT_IMPLEMENTED
LATER_SA_BIST_INDEPENDENT: UNPROVEN
FINAL_FAULT_TEST_DONE_ORDERING: NOT_IMPLEMENTED
FAULTS_LOST: WOULD_BE_NONZERO with direct 204-bit mapping
CROSS_SA_CONTAMINATION: NOT_APPLICABLE
LATEST_GENERATION_ONLY_COMMIT: S1G-A PASS; S1G-B not connected
GENERATION_WRAP_SAFE: NOT_EVALUATED
S1GA_STATE_BITS: 896
S1GB_PRODUCER_STATE_BITS: N/A
TOTAL_EARLY_OVERLAP_STATE_BITS: N/A
S1H_FUNCTIONAL_CLOSURE_ELIGIBLE: NO
S1H_AUTHORIZED: NO
S1I_AUTHORIZED: NO
S1J_AUTHORIZED: NO
E0R_MAY_RESUME: NO
NEXT_PHASE_AUTHORIZED: NONE
GIT_DIFF_CHECK: FAIL (pre-existing unrelated Makefile:453 blank line at EOF)
```
