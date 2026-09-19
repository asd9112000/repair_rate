# P3 four-point synthesis boundary audit

This record supersedes the pre-P3-BL-RTL-A/B/C readiness snapshot. All four
points now start at integrated collector/analyzer-side inputs and end after
their selected repair result and persistent resource update. Policy timing is
architecture-defined; the external functional boundary is comparable.

| Point | Synthesis top | Candidate / policy boundary | Persistent result boundary | Comparable? |
|---|---|---|---|---|
| SYN-A 2×2 STREAMING_EARLY | `recam_dss_canonical_rs2_streaming_early_top` | integrated collector/analyzer and EARLY policy | selected repair plus committed resources | YES |
| SYN-B 2×2 GROUP_GLOBAL OPT0 | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_top` | four snapshots, shared analyzer, exhaustive GLOBAL DFS | selected group tuple plus atomic committed ledger | YES |
| SYN-B 2×2 GROUP_GLOBAL OPT1 | `recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top` | same boundary; static three-effect candidate collapse before DFS | selected group tuple plus atomic committed ledger | YES |
| SYN-C 1×4 STREAMING_EARLY | `recam_dss_line1x4_rs2_cs2_m1_normalized_streaming_early_top` | integrated snapshots, producer, and EARLY policy | selected repair plus committed resources | YES |
| SYN-D 1×4 GROUP_GLOBAL OPT0 | `recam_dss_line1x4_rs2_cs2_m1_normalized_global_top` | four snapshots, shared producer, exhaustive GLOBAL DFS | selected group tuple plus atomic committed ledger | YES |
| SYN-D 1×4 GROUP_GLOBAL OPT1 | `recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top` | same boundary; static equal-demand collapse before DFS | selected group tuple plus atomic committed ledger | YES |

The comparable boundary is:

```text
collector / RECAM state
-> candidate generation / analyzer
-> policy
-> topology/resource feasibility
-> selected solution
-> committed physical ledger
```

The final fair comparison selects the two OPT1 elaborations. Their class
strength is the same—candidate-equivalence collapse before DFS—but their keys
are architecture-specific. Neither final top has a runtime OPT0/OPT1
selection input or mux.

```text
ALL_BOUNDARIES_COMPARABLE: YES
FINAL_COMMON_GLOBAL_OPT_LEVEL: OPT1
FINAL_SYN_B_GLOBAL_ELABORATION: recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top
FINAL_SYN_D_GLOBAL_ELABORATION: recam_dss_line1x4_rs2_cs2_m1_normalized_global_opt1_classcollapsed_top
P3_4PT_SYNTH: NOT_STARTED
DC_SYNTHESIS_STARTED: NO
```
