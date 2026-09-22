# 2x2 Directional corrected Model-B2 EARLY NoScratch

## Case identity

```text
CASE_NAME: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch
TOP: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top
TOPOLOGY: GRID2X2_DIRECTIONAL
DIRECTIONAL_SEMANTICS: FIXED_FOUR_EDGE
RS/CS/SHARE_M: 2/2/1
POLICY: MODEL_B2_DIRECTIONAL_STREAMING_EARLY
SCRATCH: NO
BOUNDARY: DECISION_RESOURCE_MANAGEMENT_METADATA
```

## Architecture and provenance

This is a separate final EARLY case derived from the archived GROUP mother's
shared RECAM analyzer lineage.  It preserves ConfigID, PatternID, topology,
and candidate interpretation semantics while intentionally replacing the
GROUP candidate-store/81-path/atomic-global controller with fixed Model-B2
EARLY control: SA order `A,B,C,D`, slot order `R,L,RB,B`, and four persistent
common-spare tokens.  The 260-bit / 11-Hybrid analyzer representation is the
authorized Model-B2 state expansion; it is not a provenance failure.

```text
RTL_COMMIT: 7fdaf0ad86700d40588f31735d4eb2513b77478f
SHARED_ANALYZER_ALGORITHM_DERIVED_FROM_GROUP_LINEAGE: YES
CONFIG_SEMANTICS_PRESERVED: YES
PATTERN_SEMANTICS_PRESERVED: YES
TOPOLOGY_PRESERVED: YES
CANDIDATE_INTERPRETATION_PRESERVED: YES
EARLY_CLEAN_SHEET_REDESIGN: NO
```

Exact archived source hashes are in `synthesis/source_manifest.txt`.

## Functional verification

Directed static-action tests, immediate commit/first-failure tests, C1R3/C1R4
closure, and a deterministic 1000-vector corrected Model-B2 RTL/C++ lockstep
completed with zero lockstep mismatches.  The summary is under
`verification/regression_summary.txt`.  End-to-end physical remap is outside
this decision/resource-management synthesis boundary and remains deferred.

## Synthesis result

DC W-2024.09-SP2, TSMC018 `slow.db`, slow corner, 20.0 ns clock and zero I/O
delay produced:

```text
TOTAL_AREA_UM2: 200824.749004
TOTAL_GE: 20124.33
WNS_NS: 0.00
TNS_NS: 0.00
CRITICAL_PATH_DELAY_NS: 19.72
TIMING_MET: YES
```

Requested `compile -map_effort low` was effectively medium after DC `OPT-1303`.
The copied reports are in `synthesis/`.

## Archive scope

This new sibling archive does not modify or reinterpret the existing
`recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch`
case.  Its relationship to corrected Model-B2 GROUP behavior remains subject
to the separately requested read-only compatibility audit.
