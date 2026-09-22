# DATE2026 GROUP-B2-COMPAT-AUDIT

## Verdict

```text
GROUP_MODEL_B2_COMPATIBILITY: FAIL
GROUP_RTL_MODIFIED: NO
DSS_FINAL_GROUP_MODIFIED: NO
SYNTHESIS: NO
LATENCY: NO
```

The archived 204-bit / 7-Hybrid GROUP/GLOBAL semantics are not equivalent to a
full Model-B2 shared-state GROUP oracle.  This is an evidence result only; it
does not authorize a GROUP RTL change.

## Method

The read-only comparator uses the current C++ canonical GROUP policy
(`DirectionalV2GroupGlobalCanonical`), which is the semantic reference for
the archived fixed-four-edge GROUP controller.  It is run with its existing
standalone 204/7 candidate representation.  The independent side obtains all
four per-SA V2 configurations from the full Model-B2 shared collector
(5 pivots, 11 reachable Hybrid records), then applies a test-local exact
`A -> B -> C -> D`, `R -> L -> RB -> B`, `m=1` DFS with the existing physical
ledger as the topology authority.  It does not call a production GROUP
selector.

The deterministic corpus is the corrected C3 generator contract: seed
`20260922`, 2R/2C, directional 2x2, `m=1`, moderate imbalance and mixed
spatial faults; loads 16/20/24/28 with 250 run indices each.  Vector216 is
load 16/run index 216.  The C1R directed set is the established 16 coordinate
list.  The detailed CSV is generated at
`tmp/date2026/group_b2_compat_audit/model_b2_group_comparison.csv` by
`make test_group_model_b2_compat_audit`.

This is a semantic C++ audit rather than a new GROUP RTL simulation: the
archived GROUP RTL accepts already-retained 204-bit analyzer metadata, while
the question here is whether that truncated representation preserves the
full-collector candidate semantics.  The immutable GROUP archive was not
written or synthesized.

## Results

| Field | Result |
| --- | ---: |
| GROUP_MODEL_B2_MISMATCHES_1000 | 259 distinct vectors |
| BOTH_PASS | 627 |
| MODEL_B2_ONLY | 41 |
| OLD_GROUP_ONLY | 0 |
| BOTH_FAIL | 332 |
| SELECTED_CONFIG_DIFFERENCES | 218 (not necessarily distinct from pattern differences) |
| SELECTED_PATTERN_DIFFERENCES | 152 (not necessarily distinct from config differences) |
| FINAL_REPAIRABILITY_DIFFERENCES | 41 |
| VECTOR216_GROUP_DIFFERENCE | YES |
| C1R16_GROUP_DIFFERENCES | 13 of 16 directed vectors |

For the C1R16 subset, 13 cases pass on both sides but differ in selected
configuration (two also differ in PatternID); the remaining three fail on
both sides.  There are no old-GROUP-only repairable cases in the 1000-vector
corpus.

## Minimum counterexamples

1. Vector216: load 16, run index 216.  Both sides repair, with the same
   PatternID tuple `1:2:2:3`; however the selected ConfigID tuple is
   `4:1:0:5` for the archived/current GROUP semantics and `4:1:1:5` for the
   full Model-B2 oracle.  The C subarray therefore differs from local
   ConfigID 0 to release ConfigID 1.
2. Repairability: load 20, run index 15.  The archived/current GROUP fails
   (`-:-:-:-` selected config and pattern tuple), while the full Model-B2
   oracle repairs with ConfigIDs `4:1:0:4` and PatternIDs `1:2:5:1`.
3. Earliest corpus tuple difference: load 16, run index 7.  Both repair and
   retain PatternIDs `1:1:3:2`; the GROUP ConfigID tuple is `4:1:0:4` while
   the Model-B2 oracle selects `4:1:1:4`.

## Stop condition

The requested compatibility condition is not met, so LAT0 and every GROUP
modification remain blocked pending human review of the above counterexamples.
