# P3BLRTLC 1x4 GLOBAL integrated closure

```text
P3BLRTLC_STATUS: COMPLETE
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
MAX_PIVOTS: 6
HYBRID_ENTRIES: 10
CANDIDATE_PRODUCTION_REQUESTS: 10
CPP_ORACLE_RANDOM_CASES: 1000
CPP_ORACLE_SEED: 20260921
CPP_ORACLE_MISMATCHES: 0
GLOBAL_CANDIDATE_VISITS: 555831
GLOBAL_DFS_TOTAL_CYCLES: 555831
AVERAGE_CANDIDATE_VISITS_PER_CASE: 555.831
GLOBAL_SEARCH_CYCLES: DATA_DEPENDENT
DIRECTED_SUCCESS_WITNESS_CYCLES: 202
DIRECTED_SUCCESS_IS_FIXED_GLOBAL_LATENCY: NO
SELECTED_TUPLE_MISMATCHES: 0
OBJECTIVE_MISMATCHES: 0
FINAL_OWNER_MISMATCHES: 0
GLOBAL_ADJACENCY_MISMATCHES: 0
BORROWED_LINE_FORWARDING_VIOLATIONS: 0
ATOMICITY_VIOLATIONS: 0
PARTIAL_COMMIT_VISIBILITY_ERRORS: 0
CANDIDATE_TABLE_MISMATCHES: 0
END_TO_END_MISMATCHES: 0
GROUP_SNAPSHOT_STORAGE_BITS: 1116
DFS_SPECULATIVE_STATE_BITS: 188 (124 ledger + 64 current-prefix tuple)
BEST_TUPLE_STATE_BITS: 96
COMMIT_SHADOW_STATE_BITS: 32
TOTAL_LOGICAL_STATE_BITS: 3796
SYN_D_LEGACY_COMMIT_CYCLES: 6
SYN_D_COMMIT_INIT_CYCLES: 1
SYN_D_COMMIT_APPLY_CYCLES: 4
SYN_D_COMMIT_PUBLISH_CYCLES: 1
SYN_D_COMMIT_TOTAL_CYCLES: 6
SYN_D_COMMIT_ERROR_LATENCY: DATA_DEPENDENT / MAY_TERMINATE_EARLY
SYN_D_PERSISTENT_LEDGER_UPDATE: publish edge
SYN_D_ATOMIC_VISIBILITY: persistent state changes only at publish
COMMIT_LATENCY_CONVENTION_NORMALIZED: YES
SYN_A_REGRESSION: PASS
SYN_B_REGRESSION: PASS
SYN_C_REGRESSION: PASS
VERILATOR_LINT: PASS
STRICT_READABLE_VERILOG_GATE: BLOCKED_BY_LOCAL_RUNTIME
SYN_D_SYNTHESIS_READY: YES
DC_SYNTHESIS_STARTED: NO
OPT2_STARTED: NO
OPT3_STARTED: NO
P3BLRTL_NEXT_PHASE_STARTED: NO
```

Closure evidence is produced by
`make test_line1x4_single_hop_rs2_cs2_m1_normalized_global`.  Its C++ oracle
corpus uses the actual `OneByFourSingleHopGlobalV1` policy and a deterministic
logical candidate record format.  Core, atomic-commit, and integrated-top tests
all consume the same contract.

The top exposes a result only after atomic publish.  Failed search and failed
commit leave the persistent owner ledger unmodified.

## Cross-architecture commit-latency convention

**IMPORTANT:** the historical `SYN-B COMMIT_CYCLES = 5` and `SYN-D
COMMIT_CYCLES = 6` used different counting conventions and must not be directly
compared. Under the canonical transaction-level definition, commit
initialization/start through persistent-ledger publish, both successful paths
are six cycles:

```text
SYN-B: 1 initialization + 4 per-SA shadow/application + 1 atomic publish
SYN-D: 1 initialization + 4 per-SA shadow/application + 1 atomic publish
SYN-B COMMIT_TOTAL_CYCLES: 6
SYN-D COMMIT_TOTAL_CYCLES: 6
```

This is not an error-path guarantee: a commit error may terminate early, and a
search failure remains distinct from commit failure.
