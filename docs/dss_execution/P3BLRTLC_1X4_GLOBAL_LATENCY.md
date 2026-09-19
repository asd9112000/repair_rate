# P3BLRTLC 1x4 GLOBAL latency

```text
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
CANDIDATE_REQUEST_CYCLES: 10
GROUP_CANDIDATE_REQUEST_CYCLES: 40
GROUP_GLOBAL_SEARCH_CYCLES: DATA_DEPENDENT
SYN_D_LEGACY_COMMIT_CYCLES: 6
SYN_D_LEGACY_DEFINITION: initialization included; init + A+B+C+D stages + publish counted
SYN_D_COMMIT_INIT_CYCLES: 1
SYN_D_COMMIT_APPLY_CYCLES: 4
SYN_D_COMMIT_PUBLISH_CYCLES: 1
SYN_D_COMMIT_TOTAL_CYCLES: 6
SYN_D_SUCCESSFUL_COMMIT_TOTAL_CYCLES: 6
SYN_D_COMMIT_ERROR_LATENCY: DATA_DEPENDENT / MAY_TERMINATE_EARLY
SYN_D_PERSISTENT_LEDGER_UPDATE: publish edge
SYN_D_ATOMIC_VISIBILITY: persistent state changes only at publish
COMMIT_LATENCY_CONVENTION_NORMALIZED: YES
```

The reused producer executes ten registered requests for each SA.  Candidate
generation therefore has a fixed 40-request group component.  The DFS walks
the captured candidates and is data-dependent; the C++-matched 1,000-case core
corpus used 555831 total search cycles, rather than defining a fixed per-case
latency.

Atomic commit consists of one shadow initialization cycle, four A/B/C/D
staging cycles, and one publish cycle.  The integrated directed tests observed
202 cycles for a successful group and 150 cycles for an unsuccessful group.

**IMPORTANT:** historical `SYN-B COMMIT_CYCLES = 5` excluded initialization,
while historical `SYN-D COMMIT_CYCLES = 6` included it; the values must not be
directly compared. Under the normalized transaction-level definition, from
commit initialization/start through persistent-ledger publish:

```text
SYN-B COMMIT_TOTAL_CYCLES: 6
SYN-D COMMIT_TOTAL_CYCLES: 6
SUCCESSFUL_PATH_SHAPE: 1 initialization + 4 per-SA shadow/application + 1 atomic publish
```

This statement applies only to successful commit paths. A commit error may
terminate early, and a search failure does not enter atomic commit.

```text
GLOBAL_DFS_TOTAL_CYCLES_FOR_1000_CASES: 555831
GLOBAL_CANDIDATE_VISITS_FOR_1000_CASES: 555831
AVERAGE_CANDIDATE_VISITS_PER_CASE: 555.831
INTEGRATED_GLOBAL_SUCCESS_CYCLES: 202
INTEGRATED_GLOBAL_FAILURE_CYCLES: 150
DIRECTED_SUCCESS_WITNESS_CYCLES: 202
DIRECTED_SUCCESS_IS_FIXED_GLOBAL_LATENCY: NO
```

The aggregate and average search values are descriptive corpus evidence, not
worst-case, fixed, or silicon-latency claims. These values are functional
latency evidence, not synthesis timing claims.
