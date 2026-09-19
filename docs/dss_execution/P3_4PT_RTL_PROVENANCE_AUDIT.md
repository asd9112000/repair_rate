# P3-4PT RTL Provenance Audit

## Scope

```text
P3_4PT_RTL_PROVENANCE_AUDIT_STATUS: COMPLETE
BASELINE: P3-4PT-SYNTH closure c81e733; synthesis source revision 93f6384
AUDIT: read-only RTL, source manifest, elaboration, and mapped hierarchy
RTL_MODIFIED: NO
SYNTH_MANIFEST_MODIFIED: NO
SYNTHESIS_RERUN: NO
```

Source inclusion is not hardware evidence. This audit separately records:

```text
A. present in source manifest
B. referenced by RTL text
C. instantiated after elaboration
D. survives optimization
E. appears in final mapped hierarchy
```

The readable-Verilog task class is `analyze`. Public evidence is
`compile=PASS` (closure exact-top Verilator lint), `toolchain=PASS` (frozen
DC), and `ast/readability/comment/naming/profile/testbench=NOT_RERUN`. The
strict skill gate remains blocked only by the known local Python 3.8 runtime.

## Five-state evidence table

| File/module and point | A | B | C | D | E | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| RS3 config table in SYN-A | Yes | Yes, only false generate branch | No | No | No | `early_core.v:90-157`; absent from `SYN_A/hierarchy.rpt`. |
| RS3 topology in SYN-A | Yes | Yes, only false generate branch | No | No | No | Same. |
| Both RS3 modules in SYN-B | No | No | No | No | No | `p3_4pt_syn_b_opt1_sources.tcl`. |
| RS2 `dss_v2_group_slot_decode` in SYN-A | Yes | Yes | Yes | Yes | Yes | `generate_rs2_decode`; named in hierarchy. |
| RS2 feasibility and 2x2 topology in SYN-A | Yes | Yes | Yes | Yes | Yes | `early_core.v:102-117`; named in hierarchy. |
| RS2 ledger and diagnostic adapter in SYN-A | Yes | Yes | Yes | Yes | Yes | `early_core.v:166-192`; named in hierarchy. |
| `recam_shared_config_analyzer` in SYN-A/B | Yes | Yes | Yes | Yes | Yes | SYN-A direct child; SYN-B producer child. |
| 1x4 producer/analyzer in SYN-C/D | Yes | Yes | Yes | Yes | Yes | Both appear in the 1x4 hierarchy reports. |

Packages are source/type definitions, not mapped hardware instances.

## RS3CS3 module audit

| Module | Actual definition | Actual RTL consumers | SYN-A / SYN-B finding |
| --- | --- | --- | --- |
| `dss_v2_rs3cs3m1_config_table` | Not parameterized; hard-coded RS=CS=3/m=1 table T0–T6. Baseline is 3R3C; entries encode 2R3C, 3R4C, 2R4C, 3R2C, 4R3C, 4R2C, ConfigID, transpose, and semantic action. | Isolated `recam_dss_v2_rs3cs3m1_{early,group}_{core,top}`. It is textually named by the generic canonical EARLY core's non-RS2 generate branch. | SYN-A parses it but cannot elaborate it because its top fixes `RESOURCE_POINT=2`; SYN-B does not read it. |
| `dss_v2_rs3cs3m1_topology` | Not parameterized; RS3/m=1 four-resource legality helper. It shares the frozen donor graph but consumes the RS3 table's action semantics. | Isolated RS3 EARLY/GROUP modules and the same false canonical-core branch. | SYN-A inactive at elaboration; SYN-B absent. |

`SYN_A/hierarchy.rpt` is post-optimization evidence: neither module exists in
the mapped hierarchy. Neither can contribute RS3-only area to SYN-A.

```text
SYN_A_ACTUALLY_INSTANTIATES_RS3CS3_LOGIC: NO
SYN_B_ACTUALLY_INSTANTIATES_RS3CS3_LOGIC: NO
ACCIDENTAL_RS3_HARDWARE_IN_RS2: NO
RS3CS3_SOURCE_INCLUSION_CLASSIFICATION: REQUIRED_DEFINITION_BUT_CONSTANT_FOLDED (SYN-A only); absent in SYN-B
```

## Elaborated SYN-A instance graph

The SYN-A top parameters are `ROW_ADDR_W=9`, `COL_ADDR_W=5`,
`DIFF_ADDR_W=9`, `HYBRID_ENTRIES=7`; it instantiates the policy with
`RESOURCE_POINT=2` and `PATTERN_ID_W=6`.

```text
recam_dss_canonical_rs2_streaming_early_top
├── analyzer: recam_shared_config_analyzer
│   └── ROW_ADDR_W=9, COL_ADDR_W=5, DIFF_ADDR_W=9, HYBRID_ENTRIES=7
└── policy_core: recam_dss_canonical_streaming_early_core
    ├── generate_rs2_decode/config_decode: dss_v2_group_slot_decode
    ├── generate_rs2_decode/feasibility: dss_v2_resource_feasibility
    │   └── topology: dss_topology_2x2_directional
    ├── resource_ledger: dss_v2_resource_ledger
    └── ledger_diagnostic: dss_v2_legacy_ledger_diagnostic_adapter
```

The alternative `generate_rs3_decode` body is pruned by the elaboration-time
constant. The source manifest comment correctly describes a generic,
parameterized definition closure; it does not assert that RS3 RTL is active.

## Elaborated SYN-B OPT1 instance graph

```text
recam_dss_grid2x2_directional_rs2_cs2_m1_..._opt1_classcollapsed_integrated_top
└── integrated_shell (USE_OPT1=1)
    ├── candidate_map_producer
    │   ├── slot_decode: dss_v2_group_slot_decode
    │   ├── shared_analyzer: recam_shared_config_analyzer
    │   │   └── ROW_ADDR_W=9, COL_ADDR_W=5, DIFF_ADDR_W=9, HYBRID_ENTRIES=7
    │   └── transition_adapter
    ├── generate_opt1_search/search_core: ...opt1_classcollapsed_core
    │   └── canonical_global_core: recam_dss_canonical_global_noscratch_core
    └── atomic_group_commit
```

Only `generate_opt1_search` survives because `USE_OPT1=1`. The SYN-B manifest
does not include either RS3 file; its mapped hierarchy independently confirms
no RS3 instance.

## RS2 ConfigID and topology provenance

RS2 uses `dss_v2_group_slot_decode`, not an RS3 subset/mask or translated RS3
ConfigID. Its semantic slots are `{LOCAL, RELEASE_ONLY, BORROW_ONLY,
RELEASE_AND_BORROW}` = `{0,1,2,3}`. Canonical policy rank is `R,L,RB,B`, i.e.
semantic slots `{1,0,3,2}`; numeric ConfigID order is not policy order.

| SA role | Slot 0 / 1 / 2 / 3 ConfigID | Descriptor series | Frozen resource map |
| --- | --- | --- | --- |
| A / D (row roles) | 0 / 4 / 5 / 6 | 2R2C / 1R2C / 2R3C / 1R3C | A release A_ROW, borrow B_COL then C_COL. D release D_ROW, borrow C_COL then B_COL. |
| B / C (column roles) | 0 / 1 / 2 / 3 | 2R2C / 2R1C / 3R2C / 3R1C | B release B_COL, borrow A_ROW then D_ROW. C release C_COL, borrow D_ROW then A_ROW. |

The decoder is `dss_v2_group_slot_decode.sv:13-40`; static role and donor
mapping is `dss_topology_2x2_directional.sv:37-90`; availability requires a
released, unborrowed donor in `dss_v2_resource_feasibility.sv:41-64`. SYN-B's
transition adapter uses the same physical resource IDs and descriptor tests.
The independently frozen R1A ConfigID registry records these same mappings.

```text
RS2_CONFIG_SEMANTICS_MATCH_FROZEN_CONTRACT: YES
RS2_DIRECTIONAL_TOPOLOGY_MATCH_FROZEN_CONTRACT: YES
```

`dss_topology_2x2_directional` is the active RS2 topology. The RS3 topology
file is an isolated helper which happens to reuse the same four-resource
donor graph at m=1. They are not concurrent, duplicate mapped topologies in
SYN-A or SYN-B.

## Why the 2×2 and 1×4 organizations differ

| Difference | Classification | Evidence |
| --- | --- | --- |
| 2×2 uses packages, typed decoder, feasibility, topology, ledger, and adapter | `CODE_REUSE_DECISION` + `ARCHITECTURE_REQUIRED` | The frozen 2×2 directional contract matches established V2 modules exactly. |
| 2×2 analyzer is five-pivot/seven-hybrid and has ≤10 candidates | `ARCHITECTURE_REQUIRED` | Existing analyzer matches the RS2 2×2 envelope and ConfigID semantics. |
| 1×4 has separate analyzer and producer | `ARCHITECTURE_REQUIRED` | Legal B/C 4R2C requires six pivots and ten hybrids, and candidate identity is `{attemptIndex, PatternID}`, not L/R/B/RB. |
| 1×4 policy/search/commit contains neighbor owner/donor logic | `ARCHITECTURE_REQUIRED` + `IMPLEMENTATION_SHORTCUT` | A–B–C–D row adjacency, middle-SA donor order, and no forwarding cannot use 2×2 release/borrow effect bits. The correctness-first implementation remains self-contained rather than defining a general 1×4 topology package. |
| SYN-B has producer/map/DFS/commit modules | `HISTORICAL_EVOLUTION` + `CODE_REUSE_DECISION` | Canonical 2×2 GLOBAL grew from V2's action-map/DFS boundary. |
| 1×4 is mostly producer/analyzer/policy/top | `HISTORICAL_EVOLUTION` | Pre-P3-BL documents recorded no synthesizable `rtl/dss_1x4/`; it was later built as an isolated path. |

The separate 1×4 analyzer is principally an architectural-interface necessity:

| Attribute | SYN-A/B 2×2 | SYN-C/D 1×4 |
| --- | --- | --- |
| Analyzer | `recam_shared_config_analyzer` | dedicated line1x4 shared analyzer |
| Pivots / hybrids | 5 / 7 | 6 / 10 |
| Candidate forms | RS2 ConfigID universe; ≤10 candidates | 2R2C / 3R2C / 4R2C; 6 / 10 / 15 candidates |
| Candidate metadata | action/ConfigID plus PatternID | attempt/PatternID plus used-row/used-column demand |

The serial 1×4 producer and its flattened candidate table are implementation
choices, not evidence of RS3 reuse; SYNTH-REVIEW-A separately flags them as
the major cost-review hotspot.

## Functional-boundary comparison

| Responsibility | SYN-A | SYN-B | SYN-C | SYN-D |
| --- | --- | --- | --- | --- |
| Collector snapshot | direct 2×2 analyzer input; no four-SA retained producer bank | retained shell snapshot | four 1×4 snapshots | four 1×4 snapshots |
| Candidate generation | live analyzer request | 160-entry action map producer | 10-request capacity producer | same producer |
| Analyzer | shared 2×2 | shared 2×2 | adapted 1×4 | adapted 1×4 |
| Feasibility/ledger | V2 modules | producer/search/commit representation | immediate neighbor allocation | DFS + atomic neighbor commit |
| Selection/publication | streaming/live ledger | GLOBAL DFS/atomic commit | streaming/persistent row ledger | GLOBAL DFS/atomic commit |

All four implement their frozen analyzer and policy responsibilities, but
their internal state and candidate boundaries intentionally differ. SYN-A
does not retain the four-SA candidate table required by SYN-C/D; SYN-B/D add
GLOBAL search and atomic commit. Therefore the architecture boundaries are
comparable only at the declared functional-contract level:

```text
FOUR_POINT_FUNCTIONAL_BOUNDARY_COMPARABLE: PARTIAL
2X2_STRUCTURE_CLASSIFICATION: MIXED
1X4_STRUCTURE_CLASSIFICATION: MIXED
```

This does not invalidate the four frozen measurements. It prevents claiming a
topology-only or module-count-only area comparison.

## Historical evidence

`git log` records `ff4e9a7` (2026-09-14), *Add isolated RS3 CS3 M1 H5O
representation baseline*. `H3_RS3_CS3_M1_RTL_IMPLEMENTATION.md` states it is
isolated because frozen 2×2 topology rejects RS=CS=3. Commits `788d69d` and
`3e98865` (2026-09-19) close the separate 1×4 EARLY and GLOBAL baselines;
pre-P3-BL interface documents explicitly recorded no synthesizable 1×4
family. The following is inference: generic `RESOURCE_POINT` support caused
the SYN-A definition manifest to retain RS2 and RS3 definitions, but the RS2
elaboration proves the RS3 branch is constant-folded.

## Required conclusions

```text
RS3CS3_CONFIG_TABLE_ROLE: isolated hard-coded RS3/CS3/m1 T0..T6 table; inactive in SYN-A and absent from SYN-B
RS3CS3_TOPOLOGY_ROLE: isolated RS3/m1 four-resource legality helper; inactive in SYN-A and absent from SYN-B
SYN_A_ANALYZER: recam_shared_config_analyzer, five pivots / seven hybrids / RS2 ConfigID universe
SYN_C_ANALYZER: dedicated 1x4 six-pivot / ten-hybrid 2R2C-3R2C-4R2C analyzer
WHY_2X2_USES_DSS_V2_INFRASTRUCTURE: its frozen contract directly matches reusable V2 modules
WHY_1X4_USES_ISOLATED_IMPLEMENTATION: capacity-attempt and neighbor-row semantics are not represented by 2x2 ConfigID/action/resource bits
NEXT_ACTION: HUMAN_RTL_REVIEW
```
