# P0-D Tier-2 CAM fallback instrumentation

## Contract

Future `DynamicSpareSharing` group outputs now include per-group fields:

```text
tier2_cam_required
tier2_unique_tag_count
tier2_tag_contract_version
```

The per-group fields are emitted in `paired_policy_results_v1.csv` and
`runs.csv`. `summary.csv` retains the matching aggregates:
`tier2_cam_required_groups`, mean/max unique-tag count, and the tag-contract
version. Historical normalized-1k raw rows were not changed.

`tier2_cam_required=1` exactly when the existing hierarchical fallback sequence
would reach a Tier-2-successful selected repair with at least one unique
`GlobalRepairTag`. The probe follows the device model without allocating a
global pool:

```text
Tier-0: no sharing, zero functional CAM capacity
Tier-1: configured in-group sharing, zero functional CAM capacity
Tier-2: configured group policy, capacity=fault count, selected mappings kept
        -> current GlobalRepairTag canonicalization -> unique tag count
```

The tag identity is unchanged from `GlobalOnlineRepairPool`:

```text
(domain, bank, group, subarray, row, repair_column)
```

The default contract is `GLOBAL_REPAIR_TAG_DATA_WORD_V1`, with
`repair_column = cell_column / 256`; a cell-granularity caller is explicitly
identified as `GLOBAL_REPAIR_TAG_CELL_V1`.

This is non-allocating observability. It does not change the normal group's
candidate universe, selected solution, physical ledger, or repair outcome.

## Directed validation

`tests/hierarchical_recam_simulator_test.cpp` now checks:

| Case | Expected result |
|---|---|
| local line-repair success | `required=0`, `count=0` |
| device Tier-2 fixture | `required=1`, `count=1`; normal repair result unchanged before/after probe |
| repeated cells in one data word | `required=1`, `count=1` after exact tag deduplication |
| three distinct tags | `required=1`, `count=3` |

Validation run:

```text
make test_hierarchical_recam    PASS
make dynamic_sharing_b          PASS
DynamicSpareSharing 1 1, 4-group smoke
  -> paired_policy_results_v1.csv, runs.csv, summary.csv metric schema PASS
```

Smoke output is isolated at `tmp/p0d_tier2_cam_metric_smoke_20260918`.
No device sweep or CAM scope sensitivity sweep was launched.
