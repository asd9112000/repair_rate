# 2x2 Directional HYP02-compatible EARLY NoScratch

## Case identity

```text
CASE_NAME: recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch
TOP: recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top
TOPOLOGY: GRID2X2_DIRECTIONAL
DIRECTIONAL_SEMANTICS: HYP02_STATIC_81_PATH
RS/CS/SHARE_M: 2/2/1
POLICY: HYP02_STATIC_81_PATH_CONTRACT_EARLY
SCRATCH: NO
BOUNDARY: H7_204BIT_SHARED_ANALYZER_STATIC_PATH_PREFIX_COMPATIBILITY
RTL_COMMIT: 46e1d3a82bf948dc3e357823cbc50869be964023
```

## Contract

Candidate information is `{slot-valid, PatternID}`.  A slot-valid bit is the
`solution_valid && repairable` result from the shared analyzer for one fixed
SA/configuration slot; PatternID is that analyzer's unchanged 4-bit output.
The final resource-legality source is the 81 fixed four-SA slot paths encoded
by the HYP02 GROUP selector.  This case does not use physical demand,
`PhysicalResourceLedger`, Model-B2 shared-state expansion, or the historical
four-token prototype.

The controller scans A, B, C, D and slot priority `1,0,3,2` (`R,L,RB,B`).  It
commits the first slot whose prefix can extend to at least one legal HYP02
path.  A failure has no rollback; the accepted prefix remains observable.

## Provenance and verification

The shared analyzer is byte-identical to the final HYP02 GROUP mother
(`b4aa07117af410e84c809ae833f7e926ef440a880523c78e11cd1a7df3dd5b6c`).
The independent oracle enumerates all 256 slot tuples, filters the 81 legal
paths using the selector's four fixed implications, and does not call any
physical-demand or RTL legality helper.  Exhaustive 16-slot-validity maps
(65,536) and 1,000 deterministic randomized vectors report zero mismatches.

## Synthesis result

DC W-2024.09-SP2, TSMC018 `slow.db`, slow corner, 20.0 ns clock, zero I/O
delay, and requested `compile -map_effort low` (DC OPT-1303 effective medium):

```text
TOTAL_AREA_UM2: 78216.970295
TOTAL_GE: 7838.00
WNS_NS: 0.01
TNS_NS: 0.00
CRITICAL_PATH_DELAY_NS: 19.72
TIMING_MET: YES
```

This is the thesis-primary HYP02-matched EARLY comparison point with the
final HYP02 GROUP/GLOBAL archive.  The separate H=11 Model-B2 EARLY archive is
exploratory and is not semantically matched to this case.

## Status

```text
FINAL_CASE_STATUS: VALIDATED_AND_ARCHIVED
NEXT_PHASE: DATE2026-LAT0-FINAL-HYP02-RTL-CONTRACT-AUDIT
```
