# P3-4PT-SYNTH — Formal Four-Point Integrated Synthesis Closure

## Closure status

```text
P3_4PT_SYNTH_STATUS:
COMPLETE

SYNTHESIS_GIT_REVISION:
93f6384002029f97fb47106847367ad379316a59

DC_VERSION:
W-2024.09-SP2

LIBRARY:
slow.db

CORNER:
slow

CLOCK_PERIOD_NS:
20.0

INPUT_DELAY_NS:
0.0

OUTPUT_DELAY_NS:
0.0

NAND2X1_AREA:
9.979200

COMMON_GLOBAL_OPT_LEVEL:
OPT1
```

The four integrated points completed with their own immutable top/source
closures under `results/p3_4pt_synth/`. The initial environment block was
resolved by rerunning SYN-C with a longer execution timeout; it was a timeout
guard event, not an RTL or architecture-specific synthesis failure.

## Frozen synthesis boundaries

| Point | Elaborated top | Topology | Policy / class |
| --- | --- | --- | --- |
| SYN-A | `recam_dss_canonical_rs2_streaming_early_top` | `GRID2X2_DIRECTIONAL` | `NORMALIZED_STREAMING_EARLY` |
| SYN-B OPT1 | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top` | `GRID2X2_DIRECTIONAL` | `NORMALIZED_GROUP_GLOBAL_NOSCRATCH`, OPT1 |
| SYN-C | `recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top` | `LINE1X4_SINGLE_HOP` | `NORMALIZED_STREAMING_EARLY` |
| SYN-D OPT1 | `recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top` | `LINE1X4_SINGLE_HOP` | `NORMALIZED_GLOBAL`, OPT1 |

SYN-B deliberately excludes the OPT0 public wrapper. SYN-D uses its public
wrapper with `ENABLE_OPT1_CLASS_COLLAPSE=1` fixed at elaboration. No OPT2,
OPT3, one-sided pruning, policy change, topology change, RTL change, or
constraint relaxation was introduced for this synthesis comparison.

## Common methodology and tool behavior

| Item | Value |
| --- | --- |
| Technology / PVT | TSMC018 `slow.db` / slow |
| Clock / IO delay | 20.0 ns / 0.0 ns input and output |
| Requested compile command | `compile -map_effort low` |
| NAND2X1 reference area | 9.979200 |
| Source manifests | `scripts/synthesis/dc/p3_4pt_syn*_sources.tcl` |

All four runs issue the frozen requested compile command. DC W-2024.09-SP2
reports `OPT-1303`: this command is obsolete and DC defaults its mapper to
medium effort. This is a tool-version behavior notice, not a script or
methodology change; it occurred uniformly for all four points and is retained
as a reproducibility caveat.

Gate equivalents below use only mapped total cell area divided by the reported
NAND2X1 area. They do not use net area or any historical core-only result.

## Mapped four-point results

| Point | Total cell area | GE | Comb. area | Noncomb. area | Leaf cells | Seq. cells | WNS / TNS (ns) | Timing met | Critical path dominant block |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| SYN-A | 83,831.933613 | 8,400.67 | 78,819.048729 | 5,012.884884 | 4,231 | 91 | 0.00 / 0.00 | YES | `policy_core/resource_ledger` |
| SYN-B OPT1 | 395,369.256316 | 39,619.33 | 280,601.800658 | 114,767.455658 | 16,952 | 2,027 | 0.00 / 0.00 | YES | `integrated_shell/candidate_map_producer` |
| SYN-C | 1,080,926.994646 | 108,318.00 | 956,366.618429 | 124,560.376217 | 59,289 | 2,315 | 0.00 / 0.00 | YES | `producer` |
| SYN-D OPT1 | 1,267,025.771037 | 126,966.67 | 1,064,085.430530 | 202,940.340508 | 67,461 | 3,778 | 0.00 / 0.00 | YES | `impl/producer` |

The timing reports show critical-path lengths of 19.79 ns, 19.88 ns, 19.81 ns,
and 19.72 ns for SYN-A through SYN-D respectively. Every run reports zero
setup violating paths and zero total negative slack at the frozen 20 ns clock.

## Formal pairwise area comparisons

| Comparison | Area delta | Percent |
| --- | ---: | ---: |
| SYN-B OPT1 vs SYN-A | +311,537.322703 | +371.6213% |
| SYN-D OPT1 vs SYN-C | +186,098.776391 | +17.2166% |
| SYN-C vs SYN-A | +997,095.061033 | +1189.3977% |
| SYN-D OPT1 vs SYN-B OPT1 | +871,656.514721 | +220.4664% |

```text
HISTORICAL_SYN_A_AREA:
83831.933613

HISTORICAL_SYN_A_COMPARISON:
MATCHES_CURRENT_SYN_A_TOTAL_CELL_AREA

OLD_GLOBAL_CORE_ONLY_AREA:
61751.290432

OLD_GLOBAL_CORE_ONLY_USED_IN_FORMAL_4PT_TABLE:
NO
```

## Design/reference and constraint audit

All four runs produced post-compile `check_design`, area, timing, QoR,
reference, cell, hierarchy, constraint, mapped Verilog, DDC, and SDC artifacts.
Each `references.rpt` has zero black-box rows and each QoR report has
`Macro/Black Box Area: 0.000000`. `check_design` reports only retained lint
warnings; it reports no errors or unresolved references.

The timing constraint is met for all four points. Non-timing design-rule
violations are retained as measured evidence, not hidden by this closure:

| Point | Non-timing DRC reported by QoR / constraints |
| --- | --- |
| SYN-A | none |
| SYN-B OPT1 | 166 max-cap and 102 max-fanout violating nets; 268 violation lines in `constraints.rpt` |
| SYN-C | 16 max-cap violating nets |
| SYN-D OPT1 | 17 max-cap violating nets |

No DRC, timing, area, or RTL optimization is performed in this phase. These
figures establish the frozen baseline for any separately authorized follow-up.

## Functional and static evidence

Before DC, the frozen regression set passed: canonical directional early;
2x2 OPT1 class-collapsed and integrated GLOBAL; 1x4 streaming-early C++/RTL
oracle; and 1x4 GLOBAL OPT1 class-collapsed C++/RTL oracle. The two oracle
corpora each covered 1000 cases with the recorded mismatch counters at zero.
Independent `verilator --lint-only -Wall -Wno-fatal` checks passed for every
exact production top/source closure.

The strict readable-Verilog generated-deliverable gate remains blocked before
RTL analysis by the local Python 3.8 skill runtime's unsupported
`dict[str, ...]` annotation. This is a local tooling limitation, not a
functional, lint, or DC synthesis failure.

## Closure gate

```text
FOUR_POINT_SYNTHESIS_BOUNDARY_AUDITED:
YES

SYN_A_DC_COMPLETED:
YES

SYN_B_DC_COMPLETED:
YES

SYN_C_DC_COMPLETED:
YES

SYN_D_DC_COMPLETED:
YES

ALL_FOUR_REPORT_AREA_AVAILABLE:
YES

ALL_FOUR_REPORT_TIMING_AVAILABLE:
YES

ALL_FOUR_BLACK_BOX_COUNT:
0

GE_METHODOLOGY_UNIFORM:
YES

CLOCK_CONSTRAINT_UNIFORM:
YES

LIBRARY_UNIFORM:
YES

FOUR_POINT_SUMMARY_TABLE_COMPLETE:
YES

CRITICAL_PATH_AUDIT_COMPLETE:
YES

DC_SYNTHESIS_COMPLETED:
YES

POST_SYNTH_OPTIMIZATION_STARTED:
NO

RS3_STARTED:
NO

WITHSCRATCH_STARTED:
NO

OPT2_STARTED:
NO

OPT3_STARTED:
NO

NEXT_PROPOSED_PHASE:
P3-4PT-SYNTH-REVIEW
```

Raw reports remain under `results/p3_4pt_synth/` and are intentionally not
staged. Stop here: no review, optimization, RS3, WithScratch, OPT2, or OPT3
work starts automatically.
