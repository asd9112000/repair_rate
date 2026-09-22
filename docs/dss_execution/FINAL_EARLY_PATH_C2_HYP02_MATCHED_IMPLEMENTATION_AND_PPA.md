# FINAL-EARLY-PATH-C2 — HYP02-matched EARLY implementation and PPA

## Result

The thesis-primary EARLY hardware point is now
`recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_top`.
The implementation commit is `46e1d3a82bf948dc3e357823cbc50869be964023`.  It
implements `HYP02_STATIC_81_PATH_CONTRACT`, passes an independent exhaustive
prefix oracle, and closes the same DC characterization as the final HYP02
GROUP/GLOBAL mother.

| Architecture | Semantic boundary | Area (um2) | GE | Delta vs GROUP | WNS / TNS (ns) | Status |
| --- | --- | ---: | ---: | ---: | --- | --- |
| HYP02 GROUP/GLOBAL | fixed 81 paths; deferred atomic selection | 106341.682630 | 10656.33 | baseline | 0.00 / 0.00 | final mother |
| HYP02-compatible EARLY | fixed 81 paths; immediate A→B→C→D prefix selection | 78216.970295 | 7838.00 | -26.447496% | +0.01 / 0.00 | matched primary EARLY |

The former Model-B2 EARLY (`200824.749004 um2`, H=11, four expanded
summaries) remains an exploratory result only.  It is not used above because
the completed compatibility audit found it differs from final HYP02 GROUP
semantics; it must not be presented as the primary EARLY-versus-GROUP PPA
comparison.

## Frozen HYP02 mother architecture

The immutable mother top is `recam_dss_hyp02_static_global_top`.  It has one
`recam_shared_config_analyzer` configured as H=7 (204-bit input boundary).
For each `{SA, slot}`, the analyzer returns `solution_valid`, `repairable`, and
a 4-bit PatternID.  The GROUP core stores all 16 `{valid, PatternID}` records
in an 80-bit candidate store, then the static selector chooses the first
valid tuple among 81 elaboration-fixed paths and atomically publishes all four
results.

Slot 0 is local, slot 1 releases, slot 2 borrows, and slot 3 both releases
and borrows.  ConfigID mapping is A/D: `0,4,5,6` and B/C: `0,1,2,3` for slots
`0,1,2,3`.  PatternID is passed through unchanged from the shared analyzer.
One static path is a tuple `(A-slot,B-slot,C-slot,D-slot)` satisfying all four
relations: `C.borrow -> A.release`, `B.borrow -> D.release`,
`A.borrow -> B.release`, and `D.borrow -> C.release`.  Enumerating the 256
tuples under these relations yields exactly 81 legal paths.

Thus a partial prefix `A`, `A+B`, or `A+B+C` is usable precisely when the
prefix plus proposed next slot appears in at least one of the 81 legal tuples.
This is the hardware contract; it is not a physical usedRows/usedColumns
demand model.

## EARLY derivation and provenance

The new top retains one byte-identical shared analyzer:
`b4aa07117af410e84c809ae833f7e926ef440a880523c78e11cd1a7df3dd5b6c` in
both archives.  It has no H=11 Model-B2 expansion and no analyzer duplication.

GROUP-only candidate history, the 81-way deferred final selector, and atomic
group commit are removed.  The added EARLY core scans SAs A, B, C, D; every SA
uses frozen priority slots `1,0,3,2` (`R,L,RB,B`).  On a valid compatible
slot, it commits ConfigID and PatternID immediately; a later failure retains
the previous commit without rollback.

The compact prefix state is four bits:
`A.release`, `A.borrow`, `B.borrow`, `C.release`.  They are not four resource
tokens.  They are the only previous-prefix facts referenced by the four HYP02
implications, and their transitions are directly equivalent to filtering the
81-path universe.  At B the controller requires `!A.borrow || B.release`; at
C it requires `!C.borrow || A.release`; at D it requires both
`!D.borrow || C.release` and `!B.borrow || D.release`.

| Source / structure | Class |
| --- | --- |
| `recam_shared_config_analyzer.sv` | identical shared analyzer/candidate producer |
| `dss_v2_group_candidate_store.sv` | GROUP-only, removed |
| `recam_dss_hyp02_static_selector.sv` | GROUP-only deferred selector, removed |
| `recam_dss_hyp02_static_global_core.sv` | GROUP-only collect/decide/atomic core, removed |
| `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core.sv` | EARLY-only prefix controller |
| new top | wrapper change: analyzer directly feeds EARLY core |

No `PhysicalResourceLedger`, physical-demand reconstruction, generic runtime
resource indexing, variable part-select, candidate-history store, or GROUP
final 81-way selection network is instantiated in the production EARLY path.
The historical four-token model is not reused.

## Independent proof and regression evidence

`tb/.../recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch_core_test.cpp`
is an independent oracle.  It enumerates all 256 raw four-slot tuples,
filters them with the four static HYP02 relations, and uses only surviving
tuples to answer prefix-extension queries.  It shares neither an RTL legality
helper nor the C++ PhysicalResourceLedger implementation.

```text
HYP02_EARLY_ORACLE_INDEPENDENT=YES
HYP02_LEGAL_PATH_COUNT=81
EXHAUSTIVE_VALIDITY_MAPS=65536
PREFIX_LEGALITY_MISMATCHES=0
RANDOMIZED_VECTORS=1000
1000_VECTOR_MISMATCHES=0
HYP02_SHARED_ANALYZER_ALL_LOCAL=PASS
HYP02_SHARED_ANALYZER_OVERFLOW_REJECTION=PASS
```

The boundary fixture labelled Vector216 in this branch is valid-map `0x5a6c`
with deterministic PatternIDs and passes under the HYP02 oracle.  Separately,
the historical fault-level Vector216 (`load=16`, index=216) already documents
the semantic split: archived GROUP chooses ConfigIDs `4:1:0:5`, while the full
Model-B2 physical oracle chooses `4:1:1:5`; both repair.  They are not
expected to match.  The exact H=7 analyzer input fixture for replaying that
fault-level vector through this new RTL is not present in the historical
corpus, so this phase does not claim such a replay.  This does not weaken the
static-HYP02 oracle proof, which is the selected hardware boundary.

## Same-methodology PPA

DC W-2024.09-SP2 used TSMC018 `slow.db`, slow corner, 20.0 ns clock, zero I/O
delay, NAND2X1 area `9.979200 um2`, and requested `compile -map_effort low`.
DC issued OPT-1303 and used effective/default medium effort, as it did for the
accepted reference flow.

```text
TOTAL_CELL_AREA: 78216.970295 um2
GE: 7838.00
COMBINATIONAL_AREA: 75748.781464 um2
SEQUENTIAL_AREA: 2468.188831 um2
TOTAL_CELL_COUNT: 3913
COMBINATIONAL_CELL_COUNT: 3866
SEQUENTIAL_CELL_COUNT: 45
CRITICAL_PATH_NS: 19.72
WNS_NS: +0.01
TNS_NS: 0.00
TIMING_MET: YES
```

The critical path is a register-to-register EARLY-core path: `core/sa_q_reg[1]`
to `core/selected_pattern_flat_o_reg[15]` in `timing.rpt`.  Its class is
`EARLY_CONTROL_TO_RESULT_REGISTER`, not candidate-store write as in GROUP.

| Hierarchy | EARLY area (um2) | GROUP area (um2) |
| --- | ---: | ---: |
| shared H=7 analyzer/front end | 73154.1894 | 73144.2102 |
| EARLY core, including prefix compatibility and results | 5046.1489 | 33180.8404 |
| EARLY sequential area | 2468.1888 | 7843.6514 |

The EARLY core is reported as one synthesis hierarchy, so DC does not split a
separate prefix-compatibility cell area.  Its fixed prefix tests are contained
within its `5046.1489 um2` core figure.  The near-identical analyzer area
confirms the H=7 frontend was retained; the 28,124.712335 um2 total reduction
comes from replacing deferred collect/store/select machinery with immediate
prefix control.

## Archive and limits

The immutable archive is
`dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/`.
It contains exact source hashes, oracle source/summary, methodology, and the
generated DC reports.  Existing GROUP and Model-B2 archives were not changed.

HYP02 static hardware-resource semantics and the existing physical-demand C++
resource model are not proven equivalent.  No repair-rate or latency result
may silently reuse the ledger model for this hardware case.  The next allowed
phase is `DATE2026-LAT0-FINAL-HYP02-RTL-CONTRACT-AUDIT`; no latency sweep was
started here.
