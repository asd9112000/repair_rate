# H0 — Reuse and Parameterization Audit

> 文件狀態：Complete
> 適用範圍：DSS V2-derived 2x2 Directional CAM RS=CS=3, SHARE_M=1 pre-implementation audit
> 建立時間：2026-09-13T06:40:48+08:00
> 最後修改時間：2026-09-13T06:40:48+08:00

## Conclusion

The frozen DATE V2 implementation is a verified 2,2,1 point, not arbitrary
RS/CS/SHARE_M support. Its four directional resources and one-owner ledger
remain suitable for 3,3,1. Its analyzer, ConfigID tables, candidate count, and
controller/store widths do not.

~~~
H0_RECOMMENDATION = NEW_MODULE_JUSTIFIED for the target analyzer/configuration
                    envelope; EXTEND_EXISTING for V2 m=1 topology/action;
                    REUSE_AS_IS for the four-resource ledger.
RTL_IMPLEMENTATION_AUTHORIZED = NO
~~~

No RTL, test, golden fixture, simulator behavior, or synthesis was modified or
run. H1 must first derive the legal 3,3,1 configuration space.

## Actual V2 module graph

| Module / file | Role | EARLY | GROUP | State | Reuse finding |
|---|---|---:|---:|---|---|
| rtl/dss_v2/top/recam_dss_v2_early_top.sv | production top/analyzer binding | Yes | No | core below top | REQUIRES_EXTENSION |
| rtl/dss_v2/top/recam_dss_v2_early_core.sv | A→B→C→D EARLY select/commit | Yes | No | SA/rank/output/ledger | REQUIRES_EXTENSION |
| rtl/dss_v2/top/recam_dss_v2_group_top.sv | production GROUP-NoScratch top | No | Yes | core below top | REQUIRES_EXTENSION |
| rtl/dss_v2/top/recam_dss_v2_group_core.sv | collect then greedy allocation | No | Yes | controller/store/ledger | REQUIRES_EXTENSION |
| rtl/dss_v2/group/dss_v2_group_candidate_store.sv | retained candidate history | No | Yes | 80 bits | REQUIRES_EXTENSION |
| rtl/dss_v2/group/dss_v2_group_priority_reader.sv | GROUP rank reader | No | Yes | combinational | EXTEND_EXISTING |
| rtl/dss_v2/group/dss_v2_group_slot_decode.sv | slot→ConfigID/descriptor | No | Yes | combinational | REQUIRES_EXTENSION |
| rtl/dss_v2/adapter/dss_legacy_config_adapter.sv | ConfigID↔descriptor | Yes | indirect | combinational | FIXED_CONFIG_TABLE |
| rtl/dss_v2/topology/dss_topology_2x2_directional.sv | legality/release/donor priority | Yes | Yes | combinational | EXTEND_EXISTING |
| rtl/dss_v2/resource/dss_v2_resource_feasibility.sv | feasibility/donor selection | Yes | Yes | combinational | REUSE_AS_IS for m=1 resource semantics |
| rtl/dss_v2/resource/dss_v2_resource_ledger.sv | atomic ledger update | Yes | Yes | registered | REUSE_AS_IS |
| rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv | 12-bit diagnostic ledger | Yes | Yes | combinational | REUSE_AS_IS |
| rtl/recam/recam_shared_config_analyzer.sv | shared ConfigID analyzer | Yes | Yes | combinational | FIXED_CONFIG_TABLE / NEW_MODULE_JUSTIFIED |
| rtl/recam/recam_dss_group_allocator.sv | frozen Phase-3 comparator oracle | test only | test only | registered | REUSABLE_REFERENCE_ONLY |

EARLY graph: early_top → early_core → config adapter, feasibility/topology,
ledger, diagnostic adapter; early_top binds recam_shared_config_analyzer.
GROUP graph: group_top → group_core → candidate store, priority reader, slot
decoder, feasibility/topology, ledger, diagnostic adapter; group_top binds the
same analyzer. Neither production V2 path instantiates ConfigPatternMap.

## Parameterization audit

| Subject | Evidence | Classification | Target implication |
|---|---|---|---|
| SA_NUM | package default 4; fixed two-bit A/B/C/D role logic | FIXED_TO_2_2_1 | unchanged for 2x2 |
| RS / CS | topology arithmetic exists but initial block rejects non-2 values | PARTIALLY_PARAMETERIZED | extend envelope after H1 |
| SHARE_M | default 1 and topology rejects any other value | FIXED_TO_2_2_1 | m=1 remains fixed |
| RESOURCE_NUM | 4-resource enum, ledger and 12-bit adapter | FIXED_TO_2_2_1 | unchanged only for directional m=1 |
| MAX_K | package 5; analyzer loops/dictionaries/matrix fixed 5 | FIXED_TO_2_2_1 | H2 derives target sizing |
| NUM_CONFIGS | seven fixed IDs/four role slots | FIXED_CONFIG_TABLE | H1 derives target strategy |
| candidate count | four ranks; GROUP store asserts exactly 80 bits | FIXED_TO_2_2_1 | target count/store derive later |
| PatternID / bitmap | 4-bit ID, 10-bit valid bitmap | FIXED_TO_2_2_1 | H1/H2 derive widths |
| Address / Hybrid entries | 5 pivots, 7 Hybrid default; only payload width parameters | PARAMETERIZED_BUT_UNVERIFIED_FOR_TARGET | H2 must size/verify |
| donor / borrower encoding | two donors and one owner per of 4 resources | FIXED_TO_2_2_1 | reuse only while m=1 |
| C++ maximum borrows | Dynamic simulator has max_borrows | REUSABLE_REFERENCE_ONLY | do not import into V2 ledger |

A parameter declaration is not evidence of target support: case tables, arrays,
elaboration assertions, candidate tables, tests, and goldens remain fixed.

## Analyzer and older-resource audit

recam_shared_config_analyzer accepts a three-bit runtime ConfigID but decodes
only 2R2C, 2R1C, 3R2C, 3R1C, 1R2C, 2R3C, and 1R3C. Candidate-pattern/count
functions encode four canonical classes; the maximum is ten. Matrix and
dictionaries are fixed 5×5, valid bitmap is 10 bits, PatternID is four bits,
and IDs 4–6 use hard-coded transpose. Must thresholds select fixed gt1/gt2/gt3
signals. This is not a generic 3,3 analyzer.

rtl/dss_2x2/analyzer exposes MAX_K, NUM_CFG, HYBRID_ENTRY_NUM, SHARED_ROWS,
SHARED_COLS, and MAX_BORROWS. Its dss_pkg, multi_config_analyzer_bank, and
must_threshold_decode still hard-code the same seven IDs/four canonical
classes/threshold mapping. It is reusable reference logic only, not verified
generic V2 integration.

## Topology / ledger / action audit

A resource is one physical eligible directional spare line: A_ROW, D_ROW,
B_COL, C_COL. RESOURCE_NUM=4 because each of four SAs provides one eligible
line. SHARE_M=1 means each resource has one released bit and one borrower owner;
there are no capacity counters.

Increasing local RS/CS to 3 does not increase shareable resources when m=1.
The ledger, atomic release+borrow edge, donor priority, and latest committed
state remain identical. The descriptor-legality envelope, ConfigID/action
mapping, four-slot representation, and EARLY/GROUP orders are fixed to 2,2,1
and need target derivation. m>1 would need a new ledger/topology audit.

## Verification reuse

| Evidence / test | Classification | Target use |
|---|---|---|
| scripts/simulation/run_recam_phase3hi_functional.sh | REUSE_UNCHANGED | frozen 2,2,1 protection |
| tb/recam/recam_dss_group_allocator_random_test.cpp | REUSE_WITH_PARAMETER_UPDATE | independent golden policy/ledger structure |
| run_recam_phase4e3_random.sh + EARLY equivalence harness | REUSE_WITH_PARAMETER_UPDATE | 50+1000 comparison pattern |
| run_dss_v2_phase4f4_group_random.sh + GROUP harness | REUSE_WITH_PARAMETER_UPDATE | 50+1000 comparison pattern |
| V2 adapter, feasibility, ledger, single-SA/core tests | REUSE_WITH_PARAMETER_UPDATE | conservation, donor, atomic commit |
| candidate-store / Phase4I timing tests | REUSE_WITH_PARAMETER_UPDATE | history and timing validation |
| Phase4J/4K C++ tests | REUSE_WITH_PARAMETER_UPDATE | paired outcome/mechanism analysis pattern |
| EARLY success implies GROUP success | NOT_APPLICABLE | frozen evidence establishes no dominance |

The independent golden is tb/recam/recam_dss_group_allocator_random_test.cpp:
it independently ranks, tests physical feasibility/donor priority, traverses
A→B→C→D, atomically commits a software ledger, and stops on first failure
without rollback. V2 equivalence harnesses also compare through test-only
bridges to the frozen Phase-3 allocator; bridges are not production state.

## Accepted synthesis reuse

| Policy | Entry script | Top / Tcl | Accepted result root |
|---|---|---|---|
| EARLY | scripts/synthesis/run_recam_phase4e_dc.sh | recam_dss_v2_early_top; dc/run_recam_phase4e.tcl | results/phase4e/dc_tsmc018_slow/v2_specialized_early_20ns/ |
| GROUP-NoScratch | scripts/synthesis/run_recam_phase4f_dc.sh | recam_dss_v2_group_top; dc/run_recam_phase4f.tcl | results/phase4f/dc_tsmc018_slow/v2_specialized_group_noscratch_20ns/ |

Both use DC W-2024.09-SP2, TSMC018 slow.db/slow corner, 20.0-ns clk_i,
zero IO delay, tsmc18_wl10, timing-driven low-map-effort compile,
recam_phase3j_constraints.tcl, hierarchy/timing/QoR reports, and NAND2X1
normalization. These and the metadata/report schema remain identical for a
fair future 2,2,1 versus 3,3,1 comparison. Historical Phase-2 structural
reports are not the H5 baseline.

## Minimal delta recommendation

| Area | Preferred recommendation | Reason |
|---|---|---|
| Analyzer | NEW_MODULE_JUSTIFIED | fixed seven-ID/five-entry/ten-pattern implementation; isolate frozen baseline |
| Config representation | NEW_MODULE_JUSTIFIED | H1 must derive descriptors, IDs, slots, PatternID/candidate widths |
| Scheduler / scan | EXTEND_EXISTING | preserve A→B→C→D and commit protocol; resize iteration after H1 |
| V2 action adapter | EXTEND_EXISTING | retain descriptor/action boundary; replace fixed cases |
| Topology | EXTEND_EXISTING | retain four m=1 resources/donor priority; extend legality only |
| Ledger | REUSE_AS_IS | four single-line resource semantics unchanged |
| EARLY policy | EXTEND_EXISTING | preserve first feasible ranking after H1 order freeze |
| GROUP-NoScratch | EXTEND_EXISTING | preserve collect-then-greedy/no rollback; resize store |
| Verification | EXTEND_EXISTING | reuse golden/regression patterns with target vectors |
| Synthesis | REUSE_AS_IS | reuse accepted DC environment; new source manifest only after H3 |

## Entry state

~~~
READY_FOR_H1 = YES
READY_FOR_H2 = NO
H1_BLOCKERS = NONE
H2_DEPENDENCY = H1 legal configuration-space freeze
NEXT_PHASE_AUTHORIZED = NONE
~~~

The simulator-to-V2 GROUP semantic issue is recorded in the conflict log. It
does not block H1 but blocks later simulator calibration claims.
