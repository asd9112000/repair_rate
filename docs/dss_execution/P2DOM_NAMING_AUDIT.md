# P2-DOM naming audit

`ARCHITECTURE: GRID2X2_DIRECTIONAL_RS2_CS2_M1_NORMALIZED_GLOBAL_NOSCRATCH`

## New P2-DOM artifacts

| Artifact | Layout | Topology / SharingPolicy | RS | CS | m | SolutionTakePolicy | ScratchMode | Reusable Common? | Required Explicit Name |
|---|---|---|---:|---:|---:|---|---|---|---|
| `p2dom_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_dfs_audit_test.cpp` | GRID_2X2 | DIRECTIONAL | 2 | 2 | 1 | NORMALIZED_GLOBAL | WITHOUT_SCRATCH | NO | present in identifier |
| `test_p2dom_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_dfs_audit` | GRID_2X2 | DIRECTIONAL | 2 | 2 | 1 | NORMALIZED_GLOBAL | WITHOUT_SCRATCH | NO | present in target |
| required `P2DOM_*.md` reports | documented in each report's architecture header | documented in each report's architecture header | 2 | 2 | 1 | NORMALIZED_GLOBAL | WITHOUT_SCRATCH | N/A: task-mandated report stems | required task filename retained; canonical ID is explicit inside |

No new RTL module, synthesis run, output directory, CSV, or plot was created.

## Existing module classification

| Current name | Architecture dependencies | Name complete under current rule? | Recommended canonical description |
|---|---|---|---|
| `recam_dss_canonical_rs2_streaming_early_top` | GRID_2X2, DIRECTIONAL, RS2/CS2/m1, NORMALIZED_STREAMING_EARLY integrated analyzer/policy/ledger boundary | NO | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_streaming_early_integrated_top` |
| `recam_dss_canonical_streaming_early_core` | policy core selects either frozen RS2 or isolated RS3 decode/topology through `RESOURCE_POINT`; not a topology- or resource-point-generic algorithm | NO | describe an RS2 instance as `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_streaming_early_policy_core`; keep an RS3 instance separately explicit |
| `recam_dss_canonical_global_noscratch_core` | fixed GRID_2X2 directional RS2/CS2/m1 mappings, NORMALIZED_GLOBAL, WITHOUT_SCRATCH | NO | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_global_noscratch_dfs_core` |
| `recam_shared_config_analyzer` | fixed seven-ConfigID, five-pivot analyzer library used by the RS2/CS2/m1 lineage; no donor topology or policy ordering, but the library is not parameterized over its architectural config set | NO | fixed RS2/CS2/m1 ConfigID analyzer; a future rename should expose that frozen library rather than call it universally shared |
| `dss_v2_resource_ledger` | completed transaction intent only; no descriptor, donor mapping, topology, policy order, RS/CS point, m, or scratch behavior is evaluated | YES | common resource ledger; `dss_v2_resource_ledger` is a justified reusable-common exception |

`dss_v2_resource_ledger` is the only listed common-unit exception: its widths
come from the imported V2 interface package, but its transition relation is
solely generic release/borrow bookkeeping over supplied resource IDs.  It is
already used by distinct RS2 and RS3 paths without inspecting their topology.

No existing module was renamed in P2-DOM.  Historical and current names remain
provenance identifiers until a separately authorized cleanup.

```text
NAMING_AUDIT_COMPLETE: YES
ALL_NEW_ARCH_SPECIFIC_NAMES_ENCODE_TOPOLOGY_RS_CS_M_POLICY: YES
COMMON_UNIT_EXCEPTIONS_JUSTIFIED: YES
```
