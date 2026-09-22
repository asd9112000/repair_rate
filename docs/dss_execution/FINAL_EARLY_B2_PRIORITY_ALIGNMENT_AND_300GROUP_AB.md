# FINAL-EARLY-B2 — Priority Alignment and 300-Group Paired A/B

## Scope and commits

```text
PHASE: FINAL-EARLY-B2
BASELINE_ARCHIVE_HEAD: 1158bea22ba2d45c062fec1856c4411895482324
DOCUMENTATION_CHECKPOINT: 939a4044ea0634d836ce723f636c2079a4626959
RTL_MODIFIED: NO
SYNTHESIS_RUN: NO
FORMAL_100K_SWEEP: NOT RUN
```

The documentation checkpoint contains only the FINAL-EARLY-A/B0/B1 audit
reports. It was created after inspecting all three untracked files and
confirming no functional RTL or simulator source was dirty.

## Authoritative priority and exact simulator edit

```text
R_SLOT_ID: 1
L_SLOT_ID: 0
RB_SLOT_ID: 3
B_SLOT_ID: 2
EXPECTED_SEQUENCE: {1,0,3,2}
```

`inc/V2GroupNoScratchPolicy.hpp` retains the frozen ConfigID/role-slot table;
it was not changed. `src/DynamicRepairSimulator.cpp` changes only the
`DirectionalV2Early` dispatch into `findV2GroupNoScratchChoice` so it requests
the existing canonical rank transformation, previously used only by
`GroupGreedyRtlCanonical`. The resulting Directional EARLY order is
`R,L,RB,B`. No candidate universe, ConfigID, PatternID, topology, ledger,
GLOBAL, LOCAL_FIRST, 1x4, pairwise, seed, or fault-generation behavior changed.

## Independent priority oracle

`tests/directional_v2_group_global_test.cpp` now contains
`independentCanonicalEarlySlot()`: its literal priority is `{1,0,3,2}` and it
does not call a production priority helper, production dispatch condition, or
production role-slot order table.

It covers R+L, L+RB, R+RB, RB+B, all four valid, each only-valid slot, and no
valid slot. The oracle was built and passed before the production edit while
the old simulator behavior was still selected.

```text
INDEPENDENT_PRIORITY_ORACLE: PASS
PRODUCTION_HELPER_REUSED_BY_ORACLE: NO
```

The B2 A/B runner additionally replays the *production-created* candidate
universe with literal old `{0,1,2,3}` and literal new `{1,0,3,2}` orders.
For every one of 1,200 groups it asserts that the literal new replay has the
same repairability and selected slots as the changed production simulator.

## Semantic versioning and history preservation

`DirectionalV2Early` retains the human/runner-facing display
`normalized_local_first` for CLI compatibility. New CSV metadata emitted by
`DynamicCsvReporter` changes its `solution_class` to `EARLY` and its
`priority_class` to the explicit `R_L_RB_B_V2`. Historical sidecars retain
their old `LOCAL_FIRST` priority class and are not edited.

The B2 development output explicitly carries both semantic identities:

```text
OLD_PRIORITY_ID: directional_v2_early_l_r_b_rb
NEW_PRIORITY_ID: directional_v2_early_r_l_rb_b
NEW_METADATA_PRIORITY_CLASS: R_L_RB_B_V2
```

No historical result file is overwritten or reinterpreted. Existing old
Directional EARLY evaluations are invalid for the new priority; raw fault
corpora remain reusable. GLOBAL and all other policy sidecars remain valid.

## Focused validation

| Command | Evidence |
| --- | --- |
| `make test_directional_v2_group_global` | independent oracle PASS (10 cases); V2 GLOBAL known witness, 64 containment vectors, and 16 brute-force vectors PASS |
| `make test_solution_take_policy` | RS2 random 1,000 and RS3 random 1,000 golden replays PASS; resource-budget, first-failure/no-rollback, A→B→C→D, and 1x4 directed/random checks PASS |
| `make test_dynamic_spare_sharing_policy test_dynamic_spare_sharing_layout` | non-directional policy and layout checks PASS |
| `make test_canonical_global_noscratch` | GLOBAL directed 6 + random 1,000 RTL reference vectors, 0 mismatches |
| `make test_canonical_directional_early` | RS2 and RS3 canonical streaming EARLY: each directed 6 + random 1,000 vectors, 0 mismatches |
| `make test_final_early_b2_priority_ab` | all four 300-group paired points PASS; literal new replay equals production on every group |

```text
RS2_FUNCTIONAL_REGRESSION: PASS
RS3_FUNCTIONAL_REGRESSION: PASS
INDEPENDENT_PRIORITY_ORACLE: PASS
GLOBAL_SEMANTICS_UNCHANGED: PASS
1X4_SEMANTICS_UNCHANGED: PASS
NON_DIRECTIONAL_POLICY_UNCHANGED: PASS
```

## Same-corpus development A/B

Development artifacts are written to
`tmp/date2026/final_early_b2_priority_ab/` (`summary.csv` and
`divergence_examples.csv`); `manifest.txt` records timestamp, checkpoint,
seed, corpus recipe, policy IDs, slot sequences, and all point parameters.
Each point uses exactly 300 generated groups from the same deterministic corpus
for both evaluations: RS=CS=2, directional m=1, moderate-imbalance/Mixed fault
model, seed `20260910`, paper CAM capacity, and the production-created
candidate universe. The only varied input is priority.

```text
F_GROUP_POINTS: 16,20,24,28
GROUPS_PER_POINT: 300
TOTAL_GROUPS_PER_PRIORITY: 1200
TOTAL_PAIRED_GROUPS: 1200
```

| F_GROUP | Total | Both pass | Both fail | New only | Old only | Old repairable / rate | New repairable / rate | Delta pp |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 300 | 278 | 10 | 12 | 0 | 278 / 92.666667% | 290 / 96.666667% | +4.000000 |
| 20 | 300 | 220 | 66 | 14 | 0 | 220 / 73.333333% | 234 / 78.000000% | +4.666667 |
| 24 | 300 | 98 | 146 | 56 | 0 | 98 / 32.666667% | 154 / 51.333333% | +18.666667 |
| 28 | 300 | 41 | 202 | 57 | 0 | 41 / 13.666667% | 98 / 32.666667% | +19.000000 |
| **All** | **1200** | **637** | **424** | **139** | **0** | **637 / 53.083333%** | **776 / 64.666667%** | **+11.583333** |

Every row satisfies `both_pass + both_fail + new_only + old_only = 300`.
`NEW_ONLY > OLD_ONLY` on this development corpus. This is not universal
dominance, formal repair-rate evidence, or a paper-ready percentage claim.

## Representative divergences and four-bit observation

All saved divergence examples first differ at A:

| F_GROUP / group | Old slots | New slots | Old failure | New result | Resource consequence |
| --- | --- | --- | --- | --- | --- |
| 16 / 13 | `0:0:0:-1` | `1:1:1:2` | D | pass | old local A/B/C consume owned lines; new R at A/B/C preserves shareable lines and D can select B |
| 20 / 21 | `0:0:0:-1` | `1:1:1:2` | D | pass | same release-first prefix; D succeeds instead of first-failing |
| 24 / 0 | `0:0:0:-1` | `1:0:1:2` | D | pass | A/C release their owned resources; D B consumes the available C-column donor |
| 28 / 13 | `0:0:-1:-1` | `1:1:2:1` | C | pass | A/B releases make A-row available for C B; D then uses R |

```text
RELEASE_FIRST_HYPOTHESIS_SUPPORTED_BY_EXAMPLES: YES (development examples only)
FOUR_BIT_MODEL_COUNTEREXAMPLES: 0
```

The 1,200 traced evaluations did not require historical ConfigID, PatternID,
borrower ID, donor ID, or release-history state to explain later legality; the
differences are explainable by fixed A→B→C→D eligibility and resource
availability. This is observational support for the four-bit architecture, not
a replacement for the future RTL equivalence proof.

## Recommendation

Adopt `R,L,RB,B` as the one production `DirectionalV2Early` default: the
independent literal oracle, focused RS2/RS3 regression, unaffected-policy
checks, and controlled paired development corpus support the alignment. Stop
here for human review. Do not run a 1,000-, 100k-, R3, or other formal sweep
until that review explicitly authorizes it.
