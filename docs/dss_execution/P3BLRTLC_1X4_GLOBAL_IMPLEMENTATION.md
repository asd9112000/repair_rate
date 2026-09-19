# P3BLRTLC 1x4 GLOBAL implementation

```text
P3BLRTLC_STATUS: COMPLETE
ARCHITECTURE: LINE1X4_SINGLE_HOP_RS2_CS2_M1_NORMALIZED_GROUP_GLOBAL
SYN_C_FRONTEND_REUSED: YES
SYN_C_MAX_PIVOTS: 6
SYN_C_MAX_HYBRID_CANDIDATES: 10
SYN_C_CANDIDATE_REQUESTS_PER_SA: 10
SYN_C_ROW_LEDGER_LINES: 8
CANDIDATE_PRODUCER_REUSED_OR_ADAPTED: YES
GLOBAL_SEARCH_IMPLEMENTED: YES
ATOMIC_GROUP_COMMIT_IMPLEMENTED: YES
INTEGRATED_TOP_IMPLEMENTED: YES
```

The SYN-D baseline reuses the frozen 1x4 analyzer, candidate producer, and four
independent SA snapshots from SYN-C.  The new policy core captures the complete
candidate table before it searches; it never updates persistent physical state
during DFS.

The search and commit boundary is split into the following production RTL:

```text
rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_global_core.v
rtl/dss_1x4/policy/recam_dss_line1x4_rs2_cs2_m1_normalized_global_atomic_group_commit.v
rtl/dss_1x4/top/recam_dss_line1x4_rs2_cs2_m1_normalized_global_top.v
```

The physical graph is explicitly encoded as A-B, B-C, and C-D.  No arithmetic
or modulo-derived SA adjacency is used.  This maintains the correction for the
previous D-to-A wraparound defect.

SYN-D is correctness-first GROUP_GLOBAL only.  OPT2/OPT3, DC synthesis, and
new candidate compression are outside this milestone.

**P3-4PT classification note:** the audit proved the existing equal-demand
canonicalization as COMMON_OPT1 and added separate static OPT0/OPT1 wrappers.
OPT2/OPT3 and DC synthesis remain outside scope.
