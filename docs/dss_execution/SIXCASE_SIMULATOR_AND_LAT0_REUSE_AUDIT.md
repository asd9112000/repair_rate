# DATE2026 Six-Case Simulator and LAT0 Reuse Audit

> Phase: `SIXCASE-SIM-R0-REV1`
> Status: complete — read-only planning audit
> Scope: existing group-scope simulator, R3 runner/analysis/plot pipeline, and LAT0 reuse inventory
> Out of scope: RTL, C++ production code, runners, analysis, plots, synthesis, repair-rate sweeps, and latency sweeps

## 1. Decision summary

The six thesis cases must be a logical selection/provenance layer over existing
policies, not six duplicated simulator implementations.  The current canonical
group pipeline is reusable:

```text
scripts/run_date2026_group_sweep.sh
  -> scripts/group/r3_formal_group_repair_rate.py
  -> build/bin/DynamicSpareSharing
  -> paired corpus + per-policy CSV sidecars + aggregates
  -> scripts/analysis/r3_group/ -> scripts/plot/r3_group/
```

However, the final HYP02 EARLY and GROUP RTL contract is not implemented by a
production C++ repair-rate policy.  The final RTL uses
`HYP02_STATIC_81_PATH_CONTRACT`; every current C++ group policy allocates
`CandidateRepairOption::usedRows/usedColumns` through
`PhysicalResourceLedger`.  Consequently no existing C++ result may be labeled
as a matched HYP02 hardware repair-rate result, even where a policy name says
`directional`, `early`, or `global`.

The future selection layer should therefore retain the distinction:

```text
thesis_case_id -> existing policy descriptor -> existing implementation ID
                                      \-> explicit semantic-boundary metadata
```

It must never rename or copy the implementation as a six-case policy class.

## 2. Audit method and preserved boundaries

This is a source/document inspection only.  No RTL, simulator, runner,
analysis, plot, synthesis, `dss_final/`, Makefile, or experiment result was
modified.  No repair-rate, formal, synthesis, or latency sweep was run.

The audit traced the current entrypoint, its Python backend, the
`DynamicSpareSharing` policy parser/dispatcher, the `SimulationConfig` and
`PhysicalResourceLedger` contracts, paired-sidecar reporting, R3 derived-data
scripts, existing BIST/event models, and Phase4I RTL/C++ timing evidence.
HYP02 conclusions are also constrained by
[`DATE2026_HYP02_EARLY_PPA_LAT0_HANDOFF.md`](../handoff/DATE2026_HYP02_EARLY_PPA_LAT0_HANDOFF.md)
and
[`FINAL_EARLY_PATH_C2_HYP02_MATCHED_IMPLEMENTATION_AND_PPA.md`](FINAL_EARLY_PATH_C2_HYP02_MATCHED_IMPLEMENTATION_AND_PPA.md).

## 3. Runner call graph

| Layer | Path | Role | Policy selection | Topology selection | RS/CS | F_GROUP | Future modification likely |
|---|---|---|---|---|---|---|---|
| Shell adapter | `scripts/run_date2026_group_sweep.sh` | validates group scope/options and forwards them | No: currently rejects anything except `canonical` | No: currently rejects anything except `canonical` | Yes | Yes (`--f-group-list`) | Yes — accept a named selection preset, but do not create a parallel runner |
| Dispatcher/backend | `scripts/group/r3_formal_group_repair_rate.py` | fixes point seed, materializes one corpus, invokes all selected policies, validates sidecars, aggregates | Yes: `canonical_policies(n)` | Yes: each descriptor carries layout/topology/share rows/columns | Yes: `POINTS`, `selected_points()` | Yes: `POINTS` default or validated runtime override | Yes — add a selection-only registry/preset after mapping approval |
| Executable | `build/bin/DynamicSpareSharing` from `DynamicSpareSharing.cpp` | parses layout/topology/solution policy and runs `DynamicRepairSimulator` | Yes: `--solution-take` aliases resolve to `SolutionTakePolicy` | Yes: `--layout`, `--topology`, shared rows/columns | Yes: positional `Rs Cs` | Yes: `--fault-count` | No for aliases alone; only an approved semantic adapter may alter this layer |
| Sidecars | `src/DynamicCsvReporter.cpp` | writes corpus and result provenance | Emits canonical/implementation IDs | Emits layout/topology/share fields | Emits `N,RS,CS` | Emits `F_GROUP,seed` | Small metadata extension for `thesis_case_id` |
| Analysis | `scripts/analysis/r3_group/` | validates raw/corpus replay and derives CSV tables | Fixed policy registry today | Fixed policy/topology registry today | General raw loader; figures use 2/3 panels | data-driven loader | Yes — allow metadata-driven six-case views |
| Plot | `scripts/plot/r3_group/` | plots derived CSV only | fixed view memberships/styles today | fixed view memberships today | figures hard-code panels 2 and 3 | fixed ticks to 8..32 | Yes — metadata-driven six-case memberships, RS4 panel/format, and 8..48 ticks |

The shell adapter is intentionally an R3 group-scope adapter, not the
repository-wide `scripts/run_date2026_repair_sweep.sh` mentioned in its usage
text.  The actual backend identity stored in its manifest is
`scripts/group/r3_formal_group_repair_rate.py`.

## 4. Existing group policy inventory

All policies below share the four-subarray `DynamicRepairSimulator` and
`PhysicalResourceLedger`; `RS/CS` are generic for the base RECAM/ledger path
unless a frozen V2 ConfigID contract restricts them.  `GLOBAL` means the listed
group-level joint-search policy, never a hierarchical device CAM pool.

| Existing policy ID / primary CLI alias | Layout and sharing graph | Solution policy | Resource semantics | RS=CS support | In current R3 canonical matrix |
|---|---|---|---|---|---|
| `legacy` | configurable 2x2 or 1x4 topology | historical selector | physical ledger | parameterized positive values | only `local_no_sharing` descriptor uses it |
| `local_first` / `pairwise_row_m1_local_first` | 2x2 `edge`; row-only when share columns=0 | sequential first legal | physical ledger | 2/3/4 in R3 | yes |
| `early` / `pairwise_row_m1_early` | 2x2 `edge`; row-only when share columns=0 | sequential ranked commit | physical ledger | 2/3/4 in R3 | yes |
| `group_global` / `pairwise_row_m1_global` | 2x2 `edge`; row-only when share columns=0 | generic four-SA joint search | physical ledger | 2/3/4 in R3 | yes |
| `group_compressed_legacy` / `group` | configured 2x2/1x4 generic candidates | compressed joint search | physical ledger | parameterized | no |
| `normalized_early_deferred` / `group_no_scratch_v2` | historical V2 capacity slots | deferred V2 selector | physical ledger | not a frozen N4 directional contract | no |
| `normalized_local_first` / `directional_v2_early` | 2x2 directional m=1; rows A→C,D→B and columns B→A,C→D | V2 sequential local-first | physical ledger | 2/3 only | yes for 2/3 |
| `normalized_streaming_early` / `group_greedy_rtl_canonical` | same 2x2 directional m=1 graph | V2 `R,L,RB,B` sequential commit | physical ledger | 2/3 only | yes for 2/3 |
| `historical_directional_v2_global` / `directional_v2_group_global` | same 2x2 directional m=1 graph | V2 joint search | physical ledger | 2/3 only | yes for 2/3 |
| `normalized_global` / `directional_v2_group_global_canonical` | same 2x2 directional m=1 graph | canonical V2 joint search with release obligations | physical ledger | 2/3 only | not selected by current `canonical_policies()` |
| `one_by_four_two_pairwise_early_v1` / `two_pairwise_m1_local_first` | 1x4 pair: A↔B and C↔D, rows only | sequential first legal | physical ledger | 2/3/4 in R3 | yes |
| `one_by_four_two_pairwise_release_aware_early_v1` / `two_pairwise_m1_early` | same pair graph | sequential ranked commit | physical ledger | 2/3/4 in R3 | yes |
| `one_by_four_two_pairwise_pair_global_v1` / `two_pairwise_m1_pair_global` | same pair graph | independent pair joint search | physical ledger | 2/3/4 in R3 | yes |
| `one_by_four_single_hop_early_v1` / `single_hop_m1_local_first` | 1x4 neighbor chain A↔B↔C↔D, rows only | sequential first legal | physical ledger | 2/3/4 in R3 | yes |
| `one_by_four_single_hop_release_aware_early_v1` / `single_hop_m1_early` | same neighbor chain; no multi-hop transfer | sequential ranked commit | physical ledger | 2/3/4 in R3 | yes |
| `one_by_four_single_hop_global_v1` / `single_hop_m1_global` | same neighbor chain; no multi-hop transfer | full chain joint search | physical ledger | 2/3/4 in R3 | yes |

`SimulationConfig::validate()` rejects directional/edge on 1x4 and rejects
pair/neighbor on 2x2.  `PhysicalResourceLedger::mayBorrow()` establishes the
listed directed edges, and no policy has transitive physical borrowing: the
neighbor policy allows only immediately adjacent owners.

## 5. Six-case mapping

The two G2X2-R cases have an exact existing physical-ledger definition: use
the 2x2 `edge` topology with `shared_rows=1`, `shared_columns=0`.  The L1X4-R
name is under-specified: both the pair graph and the neighbor/single-hop graph
are valid existing row-only 1x4 implementations, and they have different
reachability and GROUP scopes.  The audit must not select one silently.

| Thesis case | Best existing policy / candidates | Match | RS2 | RS3 | RS4 | Action | Reason |
|---|---|---|---|---|---|---|---|
| `G2X2_RC_EARLY` | `directional_m1_early` → `normalized_streaming_early` | PARTIAL | exact C++ V2 | exact C++ V2 | none | `SMALL_ADAPTER_REQUIRED` | Its V2 slots still allocate physical `usedRows/usedColumns`; it is not HYP02 static-81-path EARLY. |
| `G2X2_RC_GROUP` | `directional_m1_global` → `historical_directional_v2_global`; `normalized_global` is a second candidate | PARTIAL | exact C++ V2 | exact C++ V2 | none | `SMALL_ADAPTER_REQUIRED` | Neither existing C++ global choice is final HYP02 deferred static-81-path GROUP. Human semantic choice is needed before mapping. |
| `G2X2_R_EARLY` | `pairwise_row_m1_early` → `early` | EXACT | exact | exact | exact | `REFERENCE_EXISTING_POLICY_WITH_PRESET_METADATA` | 2x2 edge, row-only physical sharing, A↔C and B↔D, sequential ranked EARLY. |
| `G2X2_R_GROUP` | `pairwise_row_m1_global` → `group_global` | EXACT | exact | exact | exact | `REFERENCE_EXISTING_POLICY_WITH_PRESET_METADATA` | Same graph and resource model; generic joint group search is the existing GROUP/GLOBAL implementation. |
| `L1X4_R_EARLY` | `single_hop_m1_early` or `two_pairwise_m1_early` | PARTIAL | exact | exact | exact | `SMALL_ADAPTER_REQUIRED` | Case ID omits whether its graph is neighbor-chain or disjoint-pairs. Once selected it is a metadata reference, not new policy code. |
| `L1X4_R_GROUP` | `single_hop_m1_global` or `two_pairwise_m1_pair_global` | PARTIAL | exact | exact | exact | `SMALL_ADAPTER_REQUIRED` | The two choices have distinct joint-search scopes and cannot be conflated. |

Therefore:

```text
G2X2_R_CONTRACT = READY
L1X4_R_CONTRACT = AMBIGUOUS
G2X2_RC final-HYP02 simulator contract = MISSING
```

The completed HYP02 archives remain hardware references only:

```text
G2X2_RC_EARLY -> dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_noscratch/
G2X2_RC_GROUP -> dss_final/recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch/
```

They do not establish that a current C++ policy is semantically matched.

## 6. Directional N=4 readiness

The generic RECAM/ledger infrastructure itself supports N=4: `SimulationConfig`
requires positive spares and checks only safe numeric capacity; `RecamGeometry`
and `SolGenerator` derive `K=Rs+Cs` and candidate count `C(K,Rs)` rather than
using an N≤3 fixed table.  The R3 descriptor also admits N=4 for local, 2x2
edge row-only, and 1x4 policies.

The directional V2 family is deliberately different.  `canonicalV2ConfigContract()`
in `inc/V2GroupNoScratchPolicy.hpp` permits only `(2,2)` and `(3,3)` and throws
for other values.  Its frozen four role-slot/ConfigID mappings are defined only
for `FrozenDate2x2M1` and `Rs3Cs3M1`.  `r3_formal_group_repair_rate.py` mirrors
this by adding the directional V2 descriptors only when `n in (2,3)` and records
the N4 contract as unsupported in its manifest.

| Directional thesis case | RS2 | RS3 | RS4 | RS4 blocker |
|---|---|---|---|---|
| `G2X2_RC_EARLY` | PARTIAL: C++ V2, not HYP02 | PARTIAL: C++ V2, not HYP02 | NONE | no frozen directional V2 ConfigID/candidate contract; no final HYP02 simulator policy |
| `G2X2_RC_GROUP` | PARTIAL: C++ V2, not HYP02 | PARTIAL: C++ V2, not HYP02 | NONE | same; final static HYP02 GROUP has no C++ bridge |
| `G2X2_R_EARLY` | EXACT | EXACT | EXACT | none for generic physical-ledger policy |
| `G2X2_R_GROUP` | EXACT | EXACT | EXACT | none for generic physical-ledger policy |

Hard-coded or frozen constraints needing an explicit future decision, not a
silent generalization:

- `V2GroupNoScratchPolicy.hpp`: only RS/CS 2 and 3 have directional frozen
  ConfigID role-slot mappings.
- `DynamicRepairSimulator.cpp`: canonical directional action logic derives
  local capacity as 2 or 3 from that contract; it has no N4 branch.
- `scripts/group/r3_formal_group_repair_rate.py`: N4 deliberately excludes all
  `DIRECTIONAL_V2` members even though `POINTS` includes N4.
- The final HYP02 hardware input is H=7 with a fixed 4-bit PatternID and a
  static 81-path slot universe for the RS2/CS2/m1 hardware point.  It is not a
  parameterized RS4 HYP02 RTL/simulator contract.
- The general candidate count grows from `C(4,2)=6`, `C(6,3)=20`, to
  `C(8,4)=70`; this is dynamically represented by the generic solver, but
  PatternID width, analyzer interface width, and a future static action/path
  contract must be explicitly re-specified for hardware matching.

For L1X4-R, current N4 support is `EXACT` for both existing pair and neighbor
physical-ledger policies, so `L1X4_R_RS4_EXTENSION_COST = NONE` within those
existing semantics.  This is not a claim of an RS4 1x4 RTL counterpart: the
checked RTL inventory is named `rs2_cs2_m1`; it is an RS2 hardware reference.

## 7. F_GROUP audit

The current frozen default is:

```text
CURRENT_F_GROUP_DEFAULT = 8,12,16,20,24,28,32
```

The backend's runtime parser accepts any distinct positive list, so the
required `8,12,16,20,24,28,32,36,40,44,48` can already be passed with
`--f-group-list`.  It is not the no-option default, and `--preflight` does not
inject the proposed thesis list.  Thus:

```text
SUPPORTS_8_TO_48_STEP4 = YES, when explicitly supplied
FROZEN_THESIS_DEFAULT_INSTALLED = NO
```

| Hard-coded location | Current effect | Minimal future change |
|---|---|---|
| `scripts/group/r3_formal_group_repair_rate.py`: `CANONICAL_F_GROUP_LIST`, `POINTS` | default remains 8..32 for N2/N3/N4 | new thesis preset owns its 8..48 list; do not rewrite historical R3 defaults |
| `tests/r3_formal_sweep_policy_contract_test.py` | guards the frozen 8..32 default; separately proves extended runtime list parsing | retain test and add thesis-preset coverage later |
| `scripts/plot/r3_group/common.py`: `COMMON_LOAD_TICKS` | axis ticks/limits end at 32 | choose 8..48 ticks for the thesis view only |
| `scripts/README_DATE2026_SWEEP.md` | examples end at 20 or 30 | documentation update only after preset approval |

The raw loader sorts/discovers F_GROUP from corpus directory names and does
not impose 32.  The analysis computations likewise use observed input rows,
not a fixed endpoint.  Existing R3 plots must be adapted because their x-axis
ticks are fixed, not because the underlying CSV cannot represent 36..48.

## 8. Same-corpus and output-schema findings

```text
SAME_CORPUS_CURRENTLY_GUARANTEED = YES for one R3 point and its selected policy matrix
FAULT_GROUP_GENERATED_ONCE_PER_GROUP = YES
POLICY_CAN_CHANGE_CORPUS = NO after the source corpus is materialized
```

At each `(RS,CS,F_GROUP,seed)` point the backend first invokes
`local_no_sharing` to generate `paired_corpus_v1.csv`; it materializes that
same corpus as a simplified fault file and replays it for every remaining
policy.  Sidecar validation rejects mismatched `corpus_id`, hash, group IDs,
resource parameters, topology, and semantic policy metadata.  `point_seed()`
is explicitly independent of policy.

`DynamicFaultGenerator` uses spare values only through the enclosing
`SimulationConfig` validation; its per-run seed, SA count allocation,
row/column address distributions, spatial model, and `faultCount` are not
derived from RS or CS.  Therefore N4 changes repair resource capacity, not
the physical corpus generator, provided memory geometry, seed, F_GROUP, and
fault/spatial model remain fixed.

The existing sidecar schema already contains:

```text
canonical_policy_id, implementation_policy_id, policy_id,
layout, topology, share_row, share_col, solution_policy,
config_contract_version, candidate_contract, priority_class, search_scope,
N, RS, CS, F_GROUP, seed, corpus_id, corpus_hash
```

It does not contain a separate `thesis_case_id`.  Hence
`OUTPUT_SCHEMA_CAN_REUSE = PARTIAL`: add only that provenance field (or a
manifest-side case-to-policy registry which the analysis carries forward).
Do not overload `canonical_policy_id`; that would erase existing R3 identity.

## 9. R3 analysis and plot reuse

### Analysis

| Path | Reusable as-is | Issue | Minimal future change |
|---|---|---|---|
| `scripts/analysis/r3_group/common.py` | PARTIAL | `POLICIES`, comparisons, labels, and figure traceability are fixed to the current R3 registry; loader requires the known policy sidecars | read a thesis case registry/metadata and retain both logical case and implementation IDs |
| `analyze_group_repair_rate.py` | PARTIAL | derives layout from policy-name prefixes | read layout from sidecar metadata instead of prefixes for thesis output |
| `analyze_fault_imbalance.py` | PARTIAL | consumes fixed `POLICIES` registry | use registry-supplied selected cases |
| `analyze_policy_pairs.py` | PARTIAL | named pair-comparison table is frozen | add only approved six-case comparisons |
| `analyze_search_complexity.py` | PARTIAL | fixed GLOBAL-policy set and group-global interpretation | preserve separate semantic-boundary labels; do not call HYP02 matched |
| `p0a_thesis_v1_artifacts.py` | PARTIAL | thesis-oriented, but retains current R3 policy vocabulary | update from registry after the six-case mapping is frozen |

The raw-data loader itself supports arbitrary RS and F_GROUP values.  Its
policy registry, not CSV parsing, is the blocker to a six-case presentation.

### Plots

| Path | Reusable as-is | Issue | Minimal future change |
|---|---|---|---|
| `scripts/plot/r3_group/common.py` | PARTIAL | fixed R3 style map, `COMMON_LOAD_TICKS=8..32`, and RS2/RS3 panel helpers | add thesis-case style/labels and a thesis 8..48 tick set without changing old R3 figures |
| `plot_group_repair_rate.py` | PARTIAL | five fixed R3 memberships; two panels only `(2,3)` | metadata-driven six-case/architecture/topology views and RS4 output layout |
| `plot_fault_imbalance.py` | PARTIAL | fixed policy memberships and `(2,3)` panels | same registry and RS4 work |
| `plot_policy_pairs.py` | PARTIAL | frozen comparison style map | add only approved case-comparison names |
| `plot_repair_rate_gain.py` | PARTIAL | frozen memberships and `(2,3)` panels | registry-based thesis selections and RS4 work |
| `plot_search_complexity.py` | PARTIAL | fixed GLOBAL set and `(2,3)` panels | only extend if this C++ algorithmic metric is retained |

The existing R3 figures already avoid plot titles and provide legends for
multi-line plots.  New six-case figures must preserve those rules and keep
HYP02 static-hardware claims separated from physical-ledger curve labels.

```text
R3_ANALYSIS_CAN_REUSE = PARTIAL
R3_PLOT_CAN_REUSE = PARTIAL
NEW_ANALYSIS_DIRECTORY_REQUIRED = NO
NEW_PLOT_DIRECTORY_REQUIRED = NO
NEW_PARALLEL_RUNNER_REQUIRED = NO
```

## 10. LAT0 reuse inventory

No second latency engine is warranted.  The reusable layers below are
parameterized event/timestamp infrastructure; final HYP02 EARLY/GROUP cycle
numbers must first be audited from their own RTL and supplied as explicit
per-case parameters.  They must not inherit old V2 EARLY/GROUP constants.

| Need | Existing path(s) | Classification | Audit finding |
|---|---|---|---|
| Serial BIST schedule | `inc/FaultAddress.hpp`, `src/FaultAddress.cpp` (`SerialBistSchedule`) | `REUSE_AS_IS` | serial A→B→C→D word schedule; `arrivalCycle()`, SA boundaries, and completion are parameterized by geometry/cycles per word |
| Fault timestamp materialization | `inc/DssBistFaultTimeline.hpp`, `src/DssBistFaultTimeline.cpp` | `REUSE_AS_IS` | preserves physical-fault to completed BIST-word arrival mapping and per-SA/group boundaries |
| Event-driven overlap model | `inc/BistOverlapTimingModel.hpp`, `src/BistOverlapTimingModel.cpp` | `SMALL_EXTENSION` | existing ownership-aware candidate catch-up model; it needs final-HYP02 event parameters/owner rules, not a replacement engine |
| Post-BIST decision model | `inc/DssPostBistLatency.hpp`, `src/DssPostBistLatency.cpp` | `PARAMETER_UPDATE_ONLY` | already separates trace timestamps from policy work plan and computes `L_post=max(0,T_ready-T_BIST_end)` |
| Candidate catch-up contract | `docs/dss_execution/T1R_CANDIDATE_CATCHUP_LATENCY_CONTRACT.md` | `REUSE_AS_IS` as methodology | documents event ownership, restart, edge convention, and distinction between candidate readiness and final commit; its retained-state controller's four-cycle result is not a HYP02 constant |
| Statistics/CSV/formal runner | `T1DateLatencyExperiment.cpp`, `results/date2026/t1_group_latency/`, `scripts/simulation/run_recam_phase4e_latency.sh` | `SMALL_EXTENSION` | existing nearest-rank percentiles, summaries, CDF/histograms and result layout can be reused after the HYP02 contract is fixed |
| RTL/C++ timing comparison | `tb/dss_v2/recam_dss_v2_phase4i_rtl_timing_test.cpp`, `scripts/simulation/run_dss_v2_phase4i_rtl_timing.sh`, `docs_verilog/PHASE4I_RTL_TIMING_VALIDATION.md` | `SMALL_EXTENSION` | Phase4I proves the method on old V2 production tops; a HYP02-specific observer/trace is needed, but not a new event model |
| Phase4I work-plan sweep | `tests/dss_post_bist_latency_sweep_test.cpp`, `docs_verilog/PHASE4I_EVENT_MODEL.md` | `OBSOLETE` as a numeric HYP02 source; `REUSE_AS_IS` as method | existing 4-cycle EARLY / 20+ cycle GROUP numbers characterize prior V2 decision tops, not final HYP02 RTL |

```text
LAT0_EXISTING_BIST_SCHEDULER = SerialBistSchedule + DssBistFaultTimeline
LAT0_EXISTING_EVENT_MODEL = BistOverlapTimingModel and DssPostBistLatency
LAT0_EXISTING_RTL_VALIDATION = Phase4I representative Verilator C++/RTL trace comparison
LAT0_EXISTING_STATS_PIPELINE = T1DateLatencyExperiment nearest-rank statistics and CSV artifacts
NEW_LATENCY_CORE_REQUIRED = NO
```

The prior HYP02 handoff is decisive: the allowed next LAT0 step is a
cycle-accurate contract audit for the final HYP02 EARLY/GROUP RTL.  It must
extract start, candidate evaluation, commit, failure, and done timing before
any statistical latency execution.

## 11. Minimal future implementation plan (not executed)

1. Obtain human decisions for the L1X4-R graph (pair versus neighbor) and the
   exact intended C++/hardware relationship for G2X2-RC.
2. Freeze a `THESIS_CASE_REGISTRY` that maps a logical case ID to an existing
   policy descriptor and immutable semantic-boundary label.  Preserve the
   current `canonical_policies()` matrix unchanged.
3. Add a selection-only `thesis_6case` preset to the existing backend and
   shell adapter.  It should pass the approved `8..48 step 4` list and permit
   1,000 samples/point; it must not create another runner or policy class.
4. Add `thesis_case_id` provenance while retaining `canonical_policy_id` and
   `implementation_policy_id`; update existing R3 analysis/plot code to select
   views from that metadata and add only necessary RS4/tick/legend layouts.
5. Separately complete `DATE2026-LAT0-FINAL-HYP02-RTL-CONTRACT-AUDIT` and use
   its per-case cycle parameters with the existing event/timestamp/statistics
   stack.  Do not run a repair-rate or latency sweep beforehand.
6. For G2X2-RC only, do not present a C++ result with HYP02 PPA until a
   verified semantic bridge or an explicitly approved new static-path policy
   exists.

## 12. Required final status

```text
SIXCASE_SIM_R0_REV1_STATUS = COMPLETE
MAIN_RUNNER = scripts/run_date2026_group_sweep.sh
GROUP_SWEEP_BACKEND = scripts/group/r3_formal_group_repair_rate.py
CURRENT_CANONICAL_PRESET_LOCATION = scripts/group/r3_formal_group_repair_rate.py:canonical_policies(n)
CURRENT_CANONICAL_CASES = N2/N3: local + directional V2 (3) + 2x2 edge-row (3) + 1x4 pair (3) + 1x4 neighbor (3); N4 excludes directional V2
EXISTING_CASES_PRESERVED = YES
SIX_CASES_ARE_LOGICAL_REFERENCES = YES
NEW_PARALLEL_RUNNER_REQUIRED = NO
NEW_ANALYSIS_DIRECTORY_REQUIRED = NO
NEW_PLOT_DIRECTORY_REQUIRED = NO
THESIS_6CASE_PRESET_RECOMMENDED = YES

CASE_MAPPING_G2X2_RC_EARLY = directional_m1_early / normalized_streaming_early; PARTIAL; final HYP02 bridge missing
CASE_MAPPING_G2X2_RC_GROUP = directional_m1_global or normalized_global; PARTIAL; final HYP02 bridge and choice missing
CASE_MAPPING_G2X2_R_EARLY = pairwise_row_m1_early / early; EXACT physical-ledger reference
CASE_MAPPING_G2X2_R_GROUP = pairwise_row_m1_global / group_global; EXACT physical-ledger reference
CASE_MAPPING_L1X4_R_EARLY = single_hop_m1_early or two_pairwise_m1_early; PARTIAL pending graph choice
CASE_MAPPING_L1X4_R_GROUP = single_hop_m1_global or two_pairwise_m1_pair_global; PARTIAL pending graph choice

G2X2_RC_EARLY_SIM_SUPPORT = PARTIAL
G2X2_RC_GROUP_SIM_SUPPORT = PARTIAL
G2X2_R_CONTRACT = READY
L1X4_R_CONTRACT = AMBIGUOUS

DIRECTIONAL_RS2_SUPPORT = PARTIAL: frozen C++ V2 only, not final HYP02
DIRECTIONAL_RS3_SUPPORT = PARTIAL: frozen C++ V2 only, not final HYP02
DIRECTIONAL_RS4_SUPPORT = NONE for directional V2/HYP02
RS4_HARDCODED_BLOCKERS = V2 contract mapping only covers N2/N3; no N4 HYP02 static-path/analyzer contract

L1X4_R_RS4_CURRENT_SUPPORT = EXACT for existing physical-ledger pair/neighbor policies
F_GROUP_FROZEN_LIST = 8,12,16,20,24,28,32,36,40,44,48 (future thesis preset)
SUPPORTS_F_GROUP_TO_48 = YES via existing explicit --f-group-list parser
F_GROUP_HARDCODED_LOCATIONS = R3 default POINTS, R3 policy-contract test, R3 plot COMMON_LOAD_TICKS, documentation examples

HYP02_STATIC_PATH_SIM_SUPPORT = NONE
PHYSICAL_LEDGER_POLICIES_PRESENT = YES; all current group policies
HYP02_VS_LEDGER_CONFLICTS = static 81-path candidate legality versus usedRows/usedColumns physical-demand allocation; no proven bridge

SAME_CORPUS_CURRENTLY_GUARANTEED = YES
OUTPUT_SCHEMA_CAN_REUSE = PARTIAL; thesis_case_id provenance is missing
R3_ANALYSIS_CAN_REUSE = PARTIAL
R3_PLOT_CAN_REUSE = PARTIAL

LAT0_EXISTING_BIST_SCHEDULER = SerialBistSchedule / DssBistFaultTimeline
LAT0_EXISTING_EVENT_MODEL = BistOverlapTimingModel + DssPostBistLatency
LAT0_EXISTING_RTL_VALIDATION = Phase4I C++/Verilator trace methodology
LAT0_EXISTING_STATS_PIPELINE = T1DateLatencyExperiment and existing summary/percentile writers
NEW_LATENCY_CORE_REQUIRED = NO

MINIMAL_IMPLEMENTATION_PLAN = freeze case mapping, add selection/provenance preset only, then parameterize existing LAT0 infrastructure from final HYP02 RTL audit
RECOMMENDED_NEXT_PHASE = human mapping decision followed by DATE2026-LAT0-FINAL-HYP02-RTL-CONTRACT-AUDIT; no sweep yet

FILES_CREATED = docs/dss_execution/SIXCASE_SIMULATOR_AND_LAT0_REUSE_AUDIT.md
WORKTREE_CLEAN_AFTER_COMMIT = NOT_APPLICABLE: this read-only audit creates an uncommitted report only
```
