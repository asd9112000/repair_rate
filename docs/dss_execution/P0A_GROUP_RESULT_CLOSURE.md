# P0-A — Thesis-V1 Group Repair-Rate Closure

## Decision

```text
GROUP_RESULT_TABLES: PASS
GROUP_FIGURES: PASS
POLICY_TAXONOMY_FROZEN: YES
CANONICAL_WITNESSES_READY: YES
GROUP_SIMULATION_STATUS: CLOSED
```

The frozen source is `tmp/repair_rate_matrix_v2_normalized_1k`, the validated normalized **1k quick-sweep** dataset. It is sufficient as preliminary Thesis-V1 group-level evidence. It is not a formal 100k result and no P0-A action launched a new sweep or rewrote a raw policy/corpus row.

## Frozen policy contract

The paper-facing ladder is fixed as follows:

| class | execution semantics |
|---|---|
| `LOCAL_FIRST` | Sequential, local-first priority, first legal commit, no rollback or backtracking. |
| `EARLY` | Sequential, release-aware/resource-preserving priority, first legal commit, no rollback or backtracking. |
| `GLOBAL` | Same candidate contract, joint complete-tuple oracle/reference search. |

The two-pairwise reference keeps its canonical ID `two_pairwise_m1_pair_global` and solution class `PAIR_GLOBAL`; it occupies the GLOBAL/reference column only as a display convention and is never relabeled as generic `GLOBAL`. Directional m2 is non-canonical/blocked and is excluded.

## Tables and figure artifacts

All artifacts are reproducible, read-only derivatives at:

```text
tmp/repair_rate_matrix_v2_normalized_1k/thesis_v1/
```

| artifact | content |
|---|---|
| `tables/p0a_local_no_sharing.csv` | 21-point explicit No Sharing / LOCAL baseline table. |
| `tables/p0a_group_policy_ladder.csv` | 77 canonical topology/point rows, with percentage repair rates and all three deltas in pp. |
| `tables/p0a_{directional,pairwise_row,single_hop,two_pairwise}_policy_ladder.csv` | Per-topology clean thesis tables. |
| `tables/p0a_canonical_witness_summary.csv` | Six frozen, exact-corpus witness references. |
| `figures/figure_a_directional_policy_ladder.{png,pdf,svg}` | Figure A: LOCAL, LOCAL_FIRST, EARLY, GLOBAL directional ladder versus `F_GROUP`. |
| `figures/figure_b_within_layout_early_topology_comparison.{png,pdf,svg}` | Figure B: production-oriented EARLY topology curves; explicitly no cross-layout ranking. |
| `figures/figure_c_policy_gain_decomposition.{png,pdf,svg}` | Figure C: `DELTA_PRIORITY` and `DELTA_SEARCH` in pp, by topology/point. |
| `manifest/p0a_artifact_manifest.json` | Input, row counts, calculated maxima, and no-write provenance. |

Each table row has canonical IDs for LOCAL, LOCAL_FIRST, EARLY, and the reference policy, plus `global_solution_class`. Its definitions are:

```text
DELTA_PRIORITY_pp = EARLY - LOCAL_FIRST
DELTA_SEARCH_pp   = GLOBAL/reference - EARLY
DELTA_TOTAL_pp    = GLOBAL/reference - LOCAL_FIRST
```

Rates and deltas are explicitly percentage and percentage-point units, respectively. Directional has frozen support only at `N=2,3` (14 rows); pairwise-row, single-hop, and two-pairwise have `N=2,3,4` support (21 rows each). Figure C labels unsupported directional `N=4` cells as `n/a` rather than manufacturing a comparison.

## Results appropriate for Thesis V1

These are normalized 1k quick-sweep observations, not universal claims.

| topology | max `DELTA_PRIORITY` | max `DELTA_SEARCH` | max `DELTA_TOTAL` | correct reading |
|---|---:|---:|---:|---|
| Directional m1 | +7.9 pp | +1.9 pp | +8.9 pp | Both priority and tuple-search rescue occur in this dataset. |
| Pairwise-row m1 | +1.0 pp | +2.3 pp | +2.5 pp | Search gain is observed at some points. |
| Single-hop m1 | +6.5 pp | +0.0 pp | +6.5 pp | `NO_SEARCH_GAIN_OBSERVED_IN_1K`; not a universal equivalence claim. |
| Two-pairwise m1 | +3.4 pp | +0.0 pp | +3.4 pp | `NO_SEARCH_GAIN_OBSERVED_IN_1K`; reference class is `PAIR_GLOBAL`. |

Figure B separates 2x2 (directional/pairwise-row) and 1x4 (single-hop/two-pairwise) identity in the legend. It must not be used to present a row-only topology as an intrinsic 1x4 advantage, or to make a cross-layout ranking without a separately stated resource/physical comparison contract.

## Canonical witnesses

The compact witness table references isolated, exact-frozen-row replays in `tmp/r3_normalized_1k_witness_replay_seeded_20260918`; these generated no random examples. Detailed candidate order, ledgers, and selected candidates remain in its `attempts.csv` and `runs.csv` files. The thesis-level summary is:

| transition | topology | N / F / group | compact mechanism |
|---|---|---|---|
| LOCAL_FIRST → EARLY | directional | 2 / 12 / 738 | A release-aware choice preserves a legal C configuration. |
| LOCAL_FIRST → EARLY | pairwise-row | 2 / 16 / 475 | A different B commitment preserves one row for D. |
| LOCAL_FIRST → EARLY | single-hop | 2 / 12 / 718 | Release-aware ranking retains row capacity for C. |
| LOCAL_FIRST → EARLY | two-pairwise | 2 / 12 / 49 | The EARLY A choice makes a later B candidate legal. |
| EARLY → GLOBAL | directional | 2 / 16 / 172 | A joint V2 tuple avoids the irreversible sequential A choice. |
| EARLY → GLOBAL | pairwise-row | 2 / 12 / 738 | Joint generic search finds a complete legal tuple. |

The single-hop and two-pairwise witnesses demonstrate priority rescue only; they do not assert search gain.

## Validation and non-mutation evidence

```text
python3 scripts/analysis/r3_group/p0a_thesis_v1_artifacts.py \
  --input-root tmp/repair_rate_matrix_v2_normalized_1k \
  --output-root tmp/repair_rate_matrix_v2_normalized_1k/thesis_v1  PASS

python3 -m py_compile scripts/analysis/r3_group/p0a_thesis_v1_artifacts.py  PASS

P0-A table/witness invariants (21 LOCAL baseline rows, 77 ladder rows,
four canonical topology IDs, delta identity, supported-N coverage,
PAIR_GLOBAL class, six witnesses)                                             PASS

sha256sum -c /tmp/p0a_input_before_report.sha256
  aggregate/r3_repair_rate_summary.csv                                        OK
  aggregate/r3_paired_outcomes.csv                                            OK
```

```text
RAW_1K_POLICY_SIDECARS_REWRITTEN: NO
FROZEN_CORPUS_REWRITTEN: NO
NEW_RANDOM_SIMULATION: NO
FORMAL_100K_DATA_TOUCHED: NO
SIMULATOR_CODE_CHANGED: NO
```
