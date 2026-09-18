# P1-NAME Canonical Naming Cleanup

> Date: 2026-09-18
> Scope: C++/CLI/CSV display names and active naming documentation only.
> Status: CLOSED

## Git checkpoints

```text
START_BRANCH: DynamicSpareSharing
PRE_CLEANUP_COMMIT: 8d29c71
PRE_CLEANUP_TAG: p1-name-pre-cleanup-20260918
CLEANUP_BRANCH: p1-name-canonical-cleanup
POST_CLEANUP_COMMIT: p1-name-post-cleanup-20260918 (annotated after final commit)
POST_CLEANUP_TAG: p1-name-post-cleanup-20260918
```

The pre-cleanup checkpoint contains only the user-approved canonical source,
active documentation, selected synthesis reports, and compact external-data
provenance.  `tmp/`, `docs/recovery/`, `00_thesis/`, `01_my_note/`, external
raw corpora, and untracked generated results remain outside this checkpoint.

## Naming change

`docs/dss_execution/CANONICAL_NAMING_MAP.md` is the authoritative map.  New
C++/CSV display values are:

| Internal policy | New output | Compatibility inputs retained |
|---|---|---|
| `DirectionalV2Early` | `normalized_local_first` | `directional_v2_early`, `directional_m1_v2_early` |
| `GroupNoScratchV2` | `normalized_early_deferred` | `group_no_scratch_v2` |
| `GroupGreedyRtlCanonical` | `normalized_streaming_early` | `group_greedy_rtl_canonical`, `directional_m1_early` |
| `DirectionalV2GroupGlobal` | `historical_directional_v2_global` | `directional_v2_group_global`, `directional_m1_v2_group_global` |
| `DirectionalV2GroupGlobalCanonical` | `normalized_global` | `directional_v2_group_global_canonical`, `directional_m1_global_canonical` |

The two GLOBAL enums deliberately do not share an output label.  The historical
oracle does not impose the canonical future-donor release obligation, so
collapsing the names would alter the published semantic meaning.  The new
`normalized_global` input selects the canonical release-obligation oracle.

`DynamicSpareSharing` accepts all new labels.  The hierarchical and SRAM
programs accept the normalized labels for the policies they actually expose:
`normalized_streaming_early` and `normalized_early_deferred`.  No legacy
input was removed.

Five one-run, zero-fault CLI smoke cases confirmed parsing and CSV emission:
`normalized_local_first`, `normalized_streaming_early`,
`normalized_early_deferred`, `normalized_global`, and the historical
`directional_v2_group_global`.  The final two emitted, respectively,
`normalized_global` and `historical_directional_v2_global`.

## Escape-path audit

| Location class | Classification | Disposition |
|---|---|---|
| `SimulationConfig::toString`, `DynamicCsvReporter` | CANONICAL_OUTPUT | Emits canonical labels or an explicit historical GLOBAL label. |
| Three CLI `parseSolutionTake` functions and usage text | COMPATIBILITY_INPUT | New aliases added; existing aliases retained. |
| `docs/dss_execution/GROUP_CPP_CANONICAL_FINAL_CLOSURE.md`, `HARDWARE_OPTIMIZATION_LEDGER.md` | CANONICAL_OUTPUT documentation | First active annotation uses canonical name with historical qualification. |
| frozen CSV/corpus/synthesis report fields | HISTORICAL_PROVENANCE | Not rewritten. |
| historical RTL module and result-directory names | HISTORICAL_PROVENANCE | Not renamed or moved. |
| policy-test display-string assertion | TEST_EXPECTATION | Updated to lock the new output contract. |
| historical task notes and prior status records | HISTORICAL_PROVENANCE | Retained without global replacement. |

## Non-regression evidence

The source diff changes only parse aliases, usage strings, `toString()` values,
tests, and documentation.  It does not touch `DynamicRepairSimulator` policy
selection, candidate generation, resource ledger, fault generation, RTL, or
frozen evidence.  Thus policy dispatch, repairability, and selected tuples are
independent of the changed display values.  The test evidence below validates
the still-compiled simulator and RTL paths.

```text
REPAIRABILITY_CHANGED: NO
CANONICAL_SELECTED_TUPLES_CHANGED: NO
POLICY_SEMANTICS_CHANGED: NO
RTL_BEHAVIOR_CHANGED: NO
```

The canonical-global corpus audit has an existing frozen-corpus closure record
of 14,000 groups with zero canonical invariant violations and zero
EARLY-pass/canonical-GLOBAL-fail cases.  A fresh complete replay is not added
to this phase because the local command runner limits one foreground job to
30 seconds; no corpus or simulator change requires a new result.  The required
standard regression below completed in this checkout.

## Regression

| Command | Result |
|---|---|
| `make all` | PASS |
| `make test_directional_v2_group_global` | PASS: 64 containment vectors; brute-force mismatches `0` |
| `make test_solution_take_policy` | PASS: directed/global oracle checks and randomized oracle mismatches `0` |
| `make test_canonical_directional_early` | PASS: RS2 and RS3, each 6 directed + 1,000 random, `0` mismatches |
| `make test_canonical_global_noscratch` | PASS: 6 directed + 1,000 random, `0` mismatches |
| `make test_r3_group_plotting` | PASS |
| `git diff --check` | PASS |

## Evidence and repository safety

```text
FROZEN_1K_RAW_DATA_REWRITTEN: NO
FORMAL_100K_TOUCHED: NO
DEVICE_SWEEP_TOUCHED: NO
SYNTHESIS_REPORTS_MODIFIED: NO
HISTORICAL_DATA_DELETED: NO
HISTORICAL_CODE_DELETED: NO
HISTORICAL_RTL_RENAMED: NO
HISTORICAL_FILES_MOVED: NO
GLOBAL_WITHSCRATCH_STARTED: NO
```

No P1-NAME change was made below `rtl/`.  The only transient artifacts are
ignored `/tmp` CLI-smoke outputs and existing `tmp` derived audit output; they
were not added or deleted.

## Future archival classification (recommendation only)

| Class | Material |
|---|---|
| KEEP_FOREVER_PROVENANCE | historical RTL, historical reports, selected synthesis evidence, manifests, canonical witness summaries |
| ARCHIVE_AFTER_DATE_FREEZE | recovery, thesis, personal-note trees after separate ownership classification |
| SAFE_TO_DELETE_AFTER_REPRODUCIBILITY_CHECK | uniquely non-evidentiary raw tool logs only after a future audit |
| GENERATED_REBUILDABLE | `build/`, EDA work/alib caches, waveforms, compiled artifacts, ordinary plots, ordinary logs |

## Closure

```text
P1_NAME_STATUS: CLOSED
NEXT_AUTHORIZED_PHASE: Normalized GLOBAL-WithScratch
```
