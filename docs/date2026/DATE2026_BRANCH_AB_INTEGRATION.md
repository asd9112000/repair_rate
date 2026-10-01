# DATE2026 controlled Branch-A + Branch-B integration

## Merge identity

| Field | Value |
| --- | --- |
| Primary worktree | `/home/asd9112000/repair_rate_date2026_canonical` |
| Primary branch | `integration/date2026-directional-canonical-v1` |
| Primary HEAD before integration | `a2a61b96280275019ab06f9e120c4951466b9777` |
| Branch-A milestone | `7af7e4eed6747f64ae515f1964f58bd7ea59a825` |
| Branch-A merge commit | `8b516f601d470e112a7de4fd1addddac795003d0` |
| Branch-B milestone | `22a1186720d0092f5f40840e80c392b146aafb56` |
| Branch-B merge commit / integrated primary head | `65e6b5393024ea0cde1c9e90d15fbd346699adac` |

Both merges used `--no-ff` in the primary worktree, in the required A-then-B
order. Both milestone commits are ancestors of the resulting primary history.
Neither merge reported a conflict.

## Preserved primary state

The primary was dirty before integration. The associated preflight record is
[DATE2026_BRANCH_AB_INTEGRATION_PREFLIGHT.md](DATE2026_BRANCH_AB_INTEGRATION_PREFLIGHT.md).
It classified all 1,407 preexisting entries and recorded a status/content
fingerprint. After each merge, the fingerprint of all 1,407 preexisting
entries matched exactly.

```text
PRIMARY_MERGE_READINESS: READY_WITH_PRESERVED_DIRTY_STATE
PREEXISTING_PRIMARY_WORK_PRESERVED_AFTER_A: PASS
PREEXISTING_PRIMARY_WORK_PRESERVED: PASS
```

No stash, reset, clean, restore, deletion, or force checkout was used.

## Branch-A integration audit

`7af7e4e` is an ancestor of the primary after its merge. Its 26-source
provenance manifest verifies at the primary root. The 24-point timing summary
is readable and preserves the four measured remaining-family results:

| Case | 10 ns | 15 ns | 20 ns | 25 ns |
| --- | --- | --- | --- | --- |
| G2X2 R EARLY | setup fail | setup met | setup met | setup met |
| G2X2 R GROUP | setup fail | setup met | setup met | setup met |
| L1X4 R EARLY | setup fail | setup met | setup met | setup met |
| L1X4 R GROUP | setup fail | setup met | setup met | setup met |

Thus, 15 ns is the tightest measured passing point in this archive; it is not
an assertion of a minimum achievable period.

```text
BRANCH_A_TIMING_EVIDENCE_PRESENT: PASS
BRANCH_A_POST_MERGE_REGRESSION: PASS
```

## Branch-B integration audit

`22a1186` is an ancestor of the primary after its merge. The seven frozen N3
production RTL SHA-256 values in `N3_RC_GROUP_SOURCE_SHA256.txt` all verify.
The N3 functional, timing, and 20-ns DC evidence remains:

```text
N3_RC_GROUP_FUNCTIONAL: COMPLETE
N3_POLICY_LATENCY: 5 cycles
N3_LATENCY_FORMAL: 5:10000
SOURCE_FREEZE: PASS
20NS_SETUP: FAILED
N3_TOTAL_AREA_UM2: 685591.004318
N3_GE: 68702.0006
N3_DESIGN_WNS_NS: -24.41
N3_TNS_NS: -2736.25
N3_CRITICAL_PATH_NS: 44.23
N3_SETUP_VIOLATIONS: 119
N3_MAX_CAP_VIOLATIONS: 251
N3_MAX_FANOUT_VIOLATIONS: 155
CRITICAL_PATH_CLASS: MIXED
CRITICAL_PATH: active_sa_q -> slot decode -> analyzer -> candidate store
```

The primary preserves the N2 comparator terminology: area `171399.415025`
um2, GE `17175.6669`, `DESIGN_WNS_NS=0.00`,
`CRITICAL_PATH_SLACK_NS=+0.02`, `CRITICAL_PATH_NS=19.88`, and policy latency
five cycles. No checked N2/N3 evidence text describes `+0.02 ns` as N2
Design WNS.

The committed scaling evidence reports:

```text
AREA_RATIO: 3.99996117x  (3.999961168x before display rounding)
COMB_AREA_RATIO: 4.61807970x
SEQ_AREA_RATIO: 1.38656656x
CRITICAL_PATH_RATIO: 2.22484909x
POLICY_LATENCY_CHANGE: 0 cycles
```

The N3 structural lint passes. It retains physical address widths 9/13/13,
`MAX_K=7`, 17 Hybrid entries, `PATTERN_ID_WIDTH=6`, one 524-bit analyzer,
a 112-bit GROUP store, 656-bit retained reconstruction state, and 616-bit
private pivot capture. Runtime nth-pattern enumeration and duplicated analyzer
StateBank are absent.

```text
BRANCH_B_N3_EVIDENCE_PRESENT: PASS
N2_WNS_TERMINOLOGY: PASS
N2_TO_N3_SCALING_EVIDENCE_PRESENT: PASS
INTEGRATED_N3_STRUCTURAL_CONTRACT: PASS
INTEGRATED_N3_SOURCE_HASH_MATCH: PASS
INTEGRATED_N2_PROVENANCE: PASS
```

`provenance/SHA256SUMS` contains an older hash for the auxiliary
`tb_n3_rc_group_full_top.cpp`; the current file is byte-identical to the
Branch-B milestone and matches the later Layer-B selector/reconstruction
manifest. This is a pre-existing historical-manifest mismatch, not an
integration change. It does not affect the seven frozen production RTL hashes
or the source-freeze result, and was not rewritten during integration.

## Lightweight integration regressions

All runs used the merged frozen source tree and temporary build directories; no
synthesis was rerun.

```text
ANALYZER_DISTINCTION: PASS
GROUP_SELECTOR_ORACLE: PASS (65536 masks, 0 mismatches, 81 legal tuples)
RECONSTRUCTION_ORACLE: PASS (0 mismatches)
LATENCY_SMOKE: PASS (100 cases, 0 functional mismatches, histogram 5:100)
PRODUCTION_RTL_SEMANTIC_CHANGE: NO
INTEGRATION_REPOSITORY_HYGIENE: PASS
```

## Publication boundary

```text
PUSH: NOT_RUN
TAG: NOT_RUN
PRIORITY_STUDY: NOT_STARTED
WORKTREES_AND_RESEARCH_BRANCHES: PRESERVED
```
