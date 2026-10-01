# DATE2026 Branch-A + Branch-B integration preflight

## Primary identity before integration

| Field | Value |
| --- | --- |
| Primary worktree | `/home/asd9112000/repair_rate_date2026_canonical` |
| Primary branch | `integration/date2026-directional-canonical-v1` |
| Primary HEAD before integration | `a2a61b96280275019ab06f9e120c4951466b9777` |
| Branch-A milestone | `7af7e4eed6747f64ae515f1964f58bd7ea59a825` |
| Branch-B milestone | `22a1186720d0092f5f40840e80c392b146aafb56` |
| Branch-A N2-baseline ancestry | PASS (`a2a61b96280275019ab06f9e120c4951466b9777` is an ancestor) |
| Branch-B merge base with primary | `a2a61b96280275019ab06f9e120c4951466b9777` |
| Branch-B already contains Branch A | NO |

## Dirty-primary classification

The preflight found 1,407 primary worktree entries, all unstaged.  A
status-and-content fingerprint was captured before either merge.  It contains
the status, classification, SHA-256 (or a tracked-file-absent marker), and path
for every entry; it is used only for the post-merge preservation audit.

| Exact path set | Git status | Count | Classification | Touched by A | Touched by B | Merge risk |
| --- | --- | ---: | --- | --- | --- | --- |
| `my_note/DRAM_capacity.md`, `my_note/Estimate_area.md` | ` D` | 2 | PREEXISTING_USER_WORK | NO | NO | none |
| `scripts/README_DATE2026_SWEEP.md`, `scripts/analysis/r3_group/common.py`, `scripts/plot/r3_group/common.py`, `scripts/plot/r3_group/plot_fault_imbalance.py`, `scripts/plot/r3_group/plot_group_repair_rate.py`, `scripts/plot/r3_group/plot_repair_rate_gain.py`, `scripts/run_date2026_group_postprocess.sh` | ` M` | 7 | PREEXISTING_USER_WORK | NO | NO | none |
| `docs/dss_execution/DATE2026 — Transition from Legacy Policy Exploration to Six-Case Evaluation.md` | `??` | 1 | PREEXISTING_USER_WORK | NO | NO | none |
| `scripts/analysis_date2026_group_sweep.sh`, `scripts/plot_date2026_group_sweep.sh`, `scripts/analysis/r3_group/analyze_matched_repair_rate_pairs.py`, `scripts/analysis/r3_group/analyze_transition_boundary.py`, `scripts/plot/r3_group/plot_matched_repair_rate_pairs.py`, `scripts/plot/r3_group/plot_transition_boundary.py` | `??` | 6 | PREEXISTING_USER_WORK | NO | NO | none |
| `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_reg/**` | `??` | 22 | PREEXISTING_USER_WORK | NO | NO | none |
| `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_reg/**` | `??` | 22 | PREEXISTING_USER_WORK | NO | NO | none |
| `tb/n2_continuous_analysis/**` | `??` | 8 | PREEXISTING_USER_WORK | NO | NO | none |
| `results/**` | `??` | 1,339 | GENERATED_OR_DISPOSABLE_TOOL_OUTPUT | NO | NO | none |

No entry was classified `UNKNOWN`.  The rows above partition every preflight
entry: 68 `PREEXISTING_USER_WORK` entries and 1,339
`GENERATED_OR_DISPOSABLE_TOOL_OUTPUT` entries.

## Milestone path sets

| Set | Count | Scope |
| --- | ---: | --- |
| Branch A changed paths | 164 | `dss_latency/N2_CONTINUOUS_ANALYSIS/full_sixcase_timing_sweep/**` |
| Branch B changed paths | 75 | N3 CA-LIVE evidence, N3 RTL/testbench, matched 20-ns reports, and `.gitattributes` |
| Branch-A / Branch-B overlap | 0 | none |
| Primary dirty / Branch-A overlap | 0 | none |
| Primary dirty / Branch-B overlap | 0 | none |

The Branch-A evidence paths are isolated from the N3 evidence paths.  There is
no shared metadata, script, RTL, result, or provenance path to classify as a
potential merge conflict.

## Readiness decision

```text
PRIMARY_MERGE_READINESS: READY_WITH_PRESERVED_DIRTY_STATE
```

The existing primary state must remain untouched.  No stash, restore, reset,
clean, force checkout, or deletion is authorized or used.
