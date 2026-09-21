# 2x2 Directional GROUP-GLOBAL NoScratch

## Case Identity

```text
CASE_NAME: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch
TOP: recam_dss_hyp02_static_global_top
TOPOLOGY: GRID2X2_DIRECTIONAL
DIRECTIONAL_SEMANTICS: FIXED_FOUR_EDGE
RS/CS/SHARE_M: 2/2/1
NORMALIZATION: NORMALIZED
POLICY: GROUP_GLOBAL
SCRATCH: NO
```

## Architecture

Shared RECAM analyzer, 80-bit candidate store, 81 fixed legal static paths
with P0-to-P80 priority, combinational Config decode, four PatternID muxes,
and atomic group commit.

## Authoritative Source Provenance

```text
CANONICAL_COMMIT: 35a60728642e73f5d2c428d2a345ef8c6817bdaa
FUNCTIONAL_REFERENCE: 740a2b777d310d74ded6c50b065e9bff53d788ce
SYNTHESIS_RECORD: 03046e4
```

The canonical functional source snapshot is hash-equivalent to the source used
for HYP02-D reference synthesis.  The eight source files and their hashes are
listed in `synthesis/source_manifest.txt`.

## Functional Verification

The archived summary records the exact-81-path proof and selector/core/top RTL
regression: 256 raw configurations, 81 legal paths, 65,536 RTL validity maps,
and zero reported path, decode, priority, analyzer, or C++ repairability
mismatches.  See `verification/regression_summary.txt`.

## Synthesis Methodology

Reference characterization used Synopsys Design Compiler W-2024.09-SP2,
TSMC018 `slow.db`, slow corner, and a 20.0 ns clock with zero input/output
delay.  Requested low map effort was effectively medium following DC OPT-1303.
See `synthesis/methodology.txt`.

## Area / GE

```text
TOTAL_AREA_UM2: 106341.682630
TOTAL_GE: 10656.33
```

## Timing

```text
CLOCK_PERIOD_NS: 20.0
WNS_NS: 0.00
TNS_NS: 0.00
TIMING_MET: YES
CRITICAL_PATH_DELAY_NS: 19.82
```

## Critical Path

`ANALYZER_TO_CANDIDATE_STORE_WRITE` from analyzer input processing to the
candidate-store register endpoint.  The authoritative path detail is in
`synthesis/timing.rpt`.

## Known Caveats

The historical Phase4F static-oracle P1 generalized-release monotonicity probe
has 60 documented counterexamples and an expected nonzero exit.  This archive
does not reinterpret that conditional proof as a functional RTL regression
failure or as synthesis evidence.

## Final Status

```text
FINAL_CASE_STATUS: VALIDATED_AND_ARCHIVED
```
