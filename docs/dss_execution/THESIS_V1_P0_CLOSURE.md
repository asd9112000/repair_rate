# Thesis V1 P0 closure

## P0 summary

```text
P0_STATUS: PARTIALLY_CLOSED — P0-A, P0-B, and P0-D complete; RS3 20-ns closure remains open

# Group
GROUP_SIMULATION_STATUS: CLOSED (normalized 1k quick-sweep preliminary evidence)
GROUP_RESULT_TABLES: PASS
GROUP_FIGURES: PASS
POLICY_TAXONOMY_FROZEN: YES
CANONICAL_WITNESSES_READY: YES

# RTL semantics
OLD_RTL_EARLY_NORMALIZED_MAPPING: NO_EXACT_CANONICAL_MAPPING
OLD_RTL_GROUP_NORMALIZED_MAPPING: EARLY (GROUP-NoScratch decision/resource boundary)
RTL_REUSE_DECISION: GROUP-NoScratch=EXACT_REUSE for normalized EARLY; old EARLY=CONTROLLER_ONLY_UPDATE; GLOBAL=RTL_REWORK_REQUIRED
HISTORICAL_RS2_SYNTHESIS_STILL_USABLE: YES, only GROUP/GROUP-NoScratch as normalized-EARLY boundary evidence
HISTORICAL_RS3_SYNTHESIS_STILL_USABLE: YES for GROUP-NoScratch semantic boundary; NO as a 20-ns timing-closed implementation

# RS3 timing
RS3_TARGET_PERIOD: 20 ns
RS3_SELECTED_ARCHITECTURE: no timing-feasible selection; correct-policy reference is fixed-table GROUP-NoScratch / normalized EARLY
RS3_AREA: 527,693.45 (best retained correct-policy P1 diagnostic)
RS3_GE: 52,879.33
RS3_WNS_20NS: -12.52 ns
RS3_TIMING_MET: NO
RS3_ANALYSIS_CYCLES: +1 for retained P1 diagnostic
RS3_CRITICAL_PATH: core state -> analyzer/matrix_q_reg[2][5] matrix-preparation register

# Hardware optimization
SHARED_ANALYZER_ABLATION_AVAILABLE: NO
ITERATIVE_ANALYZER_ABLATION_AVAILABLE: NO
AREA_REDUCTION: NOT_CLAIMED; no comparable A/B/C synthesis ablation
LATENCY_COST: retained one-stage diagnostic +1 cycle; no iterative implementation

# CAM metrics
TIER2_CAM_REQUIRED_IMPLEMENTED: YES
TIER2_UNIQUE_TAG_COUNT_IMPLEMENTED: YES
CAM_METRIC_TESTS: PASS
DEVICE_SWEEP_STARTED: NO

# Protection
FORMAL_100K_DATA_TOUCHED: NO
HISTORICAL_1K_RAW_DATA_REWRITTEN: NO
```

P0 establishes the Thesis-V1 evidence chain for the group-level result,
canonical policy names, historical RTL interpretation, and future CAM-demand
measurement. It does **not** establish a 20-ns RS3 hardware implementation.
The Thesis V1 text must retain that timing limitation explicitly.

## P0-A: group-level evidence

The frozen source is `tmp/repair_rate_matrix_v2_normalized_1k`: 252,000
policy-group evaluations under same-corpus replay, canonical membership, and
zero EARLY-pass/GLOBAL-fail violations. It remains a normalized **1k
quick-sweep**, sufficient for preliminary Thesis V1 evidence but not formal
100k publication data.

Read-only derivative artifacts are at
`tmp/repair_rate_matrix_v2_normalized_1k/thesis_v1/`:

| Artifact | Result |
|---|---|
| `tables/p0a_local_no_sharing.csv` | 21-point LOCAL / No Sharing baseline |
| `tables/p0a_group_policy_ladder.csv` | 77 canonical topology/point rows with LOCAL_FIRST, EARLY, reference, and pp deltas |
| Per-topology ladder CSVs | directional, pairwise-row, single-hop, two-pairwise |
| `tables/p0a_canonical_witness_summary.csv` | six exact frozen-corpus witnesses |
| Figure A | directional LOCAL / LOCAL_FIRST / EARLY / GLOBAL ladder |
| Figure B | production EARLY topology comparison, separated by layout |
| Figure C | priority versus joint-search gain decomposition |

The fixed paper-facing taxonomy is:

```text
LOCAL_FIRST: sequential, local-first first-legal commit, no rollback
EARLY:       sequential, release-aware first-legal commit, no rollback
GLOBAL:      same candidate contract, joint complete-tuple oracle/reference
```

Directional m2 is `NON-CANONICAL / BLOCKED`. Two-pairwise retains
`PAIR_GLOBAL` rather than being relabeled generic GLOBAL.

The valid quick-sweep observations are:

| Topology | max priority gain | max search gain | Required interpretation |
|---|---:|---:|---|
| Directional m1 | +7.9 pp | +1.9 pp | normalized 1k observation |
| Pairwise-row m1 | +1.0 pp | +2.3 pp | normalized 1k observation |
| Single-hop m1 | +6.5 pp | +0.0 pp | `NO_SEARCH_GAIN_OBSERVED_IN_1K` only |
| Two-pairwise m1 | +3.4 pp | +0.0 pp | `NO_SEARCH_GAIN_OBSERVED_IN_1K` only |

No cross-layout ranking is claimed from Figure B. The detailed closure,
artifact manifest, source hashes, and witness mechanisms are in
`P0A_GROUP_RESULT_CLOSURE.md`.

## P0-B: historical RTL semantic audit

Names were not used as semantic evidence. The read-only audit found:

| Historical controller | Normalized mapping | Reuse disposition |
|---|---|---|
| historical/V2 `EARLY` | no exact canonical mapping | `CONTROLLER_ONLY_UPDATE` |
| historical/V2 `GROUP` / `GROUP-NoScratch` | directional normalized `EARLY` | `EXACT_REUSE` at controller/resource boundary |
| no historical implementation | normalized `GLOBAL` | `RTL_REWORK_REQUIRED` |

The first old-EARLY mismatch is at B/C: it ranks
`LOCAL -> BORROW_ONLY -> RELEASE_ONLY -> RELEASE_AND_BORROW`, while normalized
EARLY requires release preservation. GROUP-NoScratch instead uses
`RELEASE_ONLY -> LOCAL -> RELEASE_AND_BORROW -> BORROW_ONLY`, commits in
A->B->C->D order, and has no rollback; this is the normalized EARLY boundary.
None of the old designs performs GLOBAL's joint tuple search/backtracking.

Existing directed/random candidate-map regressions support that controller and
ledger boundary, but no end-to-end raw-fault replay of all six canonical
witnesses was run. The Verilog review runtime was blocked by its Python 3.9+
requirement on the installed Python 3.8 environment; no misleading lint or
synthesis claim is made. Full evidence and source/synthesis provenance appear
in `P0B_RTL_SEMANTIC_AUDIT.md`.

## P0-C: RS=CS=3 timing

The production-oriented semantic reference is GROUP-NoScratch, not the
historically named EARLY core. Existing Design Compiler evidence gives:

| Correct-policy variant | Area | GE | critical arrival | WNS at 20 ns | Result |
|---|---:|---:|---:|---:|---|
| original GROUP-NoScratch | 12,053,273.51 | 1,207,839.66 | 64.14 ns | -44.24 ns | fail |
| fixed-table GROUP-NoScratch | 546,667.23 | 54,780.67 | 43.20 ns | -23.29 ns | fail |
| one-stage P1 diagnostic | 527,693.45 | 52,879.33 | 32.41 ns | -12.52 ns | fail |

The best retained P1 path ends at `analyzer/matrix_q_reg[2][5]`, before the
candidate coverage / first-valid reduction. Therefore 7- or 5-wide candidate
batching cannot repair the demonstrated worst matrix-preparation path. No RTL
was altered and no synthesis was launched. A further matrix/descriptor
preparation partition is a separately authorized next hardware change, not a
P0 timing-closure claim. See `P0C_RS3_TIMING_CLOSURE.md`.

## P0-D: future CAM-demand metrics

`DynamicSpareSharing` now emits the following per-group observability fields
in future `paired_policy_results_v1.csv` and `runs.csv`:

```text
tier2_cam_required
tier2_unique_tag_count
tier2_tag_contract_version
```

They reproduce the hierarchical device fallback boundary without allocating a
device pool: Tier-0 no-sharing/zero-CAM, Tier-1 configured-sharing/zero-CAM,
then Tier-2 functional analysis and exact `GlobalRepairTag` deduplication.
The tag is `(domain, bank, group, subarray, row, repair_column)` and the
default contract is `GLOBAL_REPAIR_TAG_DATA_WORD_V1`.

Directed validation covers zero demand, one tag, repeated same-word tags,
multiple tags, and unchanged normal repair outcome. `make
test_hierarchical_recam`, `make dynamic_sharing_b`, and a four-group isolated
smoke passed. No historical 1k raw row was rewritten and no device or CAM-scope
sweep was launched. Details are in `P0D_TIER2_CAM_METRICS.md`.

## Thesis V1 conclusion and hold

The group-level thesis evidence is ready, and the historical hardware policy
names are now safely mapped. Thesis V1 may use the group figures/tables,
canonical witnesses, and qualified RS2 normalized-EARLY boundary numbers.
It must state that RS3 20-ns closure remains open and must not claim a
timing-feasible iterative candidate evaluator. A full device repair-rate or
CAM-scope sweep remains out of scope until a groups/domains-per-CAM sensitivity
contract is explicitly selected.
