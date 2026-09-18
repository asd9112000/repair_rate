# S0 — Simulator Schema and Experiment Contract

> 文件狀態：Complete（含 S0R policy closure）
> 適用範圍：future group-scope DSS 2,2,1 and 3,3,1 paired experiment contract
> 建立時間：2026-09-13T06:40:48+08:00
> 最後修改時間：2026-09-14

## Conclusion

~~~
PRIMARY_SIMULATOR_SCOPE = group
PRIMARY_SIMULATOR = DynamicSpareSharing
WORKFLOW_RECOMMENDATION = EXTEND_EXISTING
SIMULATOR_REPAIR_SEMANTICS_CHANGED = YES_ADD_EXPLICIT_POLICY_ONLY
EARLY = PRESERVED
GROUP_COMPRESSED_LEGACY = PRESERVED
GROUP_NO_SCRATCH_V2 = IMPLEMENTED
~~~

The DATE-2X2 runner already provides group scope, symmetric RS/CS sweeps,
deterministic seeds, same generated corpus across its policy invocations,
per-SA raw fault counts, and reproducible run bundles. It does not yet provide
the thesis identity/provenance contract, a hardware join, explicit selected
ConfigID/PatternID trace, or proof that its GROUP behavior equals frozen V2
GROUP-NoScratch. Those were later tasks at S0. S0R now closes the policy-
identity and decision-trace portion without running an experiment;
provenance/schema expansion remains S1 work.

## S0R explicit policy identity

Future thesis configurations must use one of these unambiguous values:

~~~
early
group_no_scratch_v2
group_compressed_legacy
~~~

The deprecated CLI aliases `group` and `group_compressed` still select the
historical `group_compressed_legacy` algorithm so old commands remain
reproducible. They are not valid names on the future main thesis path.

`group_no_scratch_v2` uses A→B→C→D immediate commit and role-slot priority
0→1→2→3. A/D map those slots to ConfigID 0/1/2/3; B/C map them to
ConfigID 0/4/5/6. Slot action identity is respectively LOCAL,
RELEASE_ONLY, BORROW_ONLY, and RELEASE_AND_BORROW. ConfigID is an encoding,
not a sorting key.

## Existing workflow audit

| Component | Existing capability | Contract disposition |
|---|---|---|
| docs/EXPERIMENT_CATALOG.md | formal group workflow authority | retain; do not edit in S0 |
| experiments/date_submission_2x2.json | fixed 2,2, legacy take policy, m=0/1/2 | reference only |
| experiments/date_submission_2x2_spare_sweep.json | symmetric spare points 2,3,4,5 | reusable 3,3 sweep mechanism |
| scripts/group/date_submission/run.py | records config, git status, commands/logs/point metadata | extend later for target identity |
| scripts/group/date_submission/analyze.py | aggregate/plot current DATE policy axes | reuse after target CSV is defined |
| scripts/group/date_submission/paired_analysis.py | verifies identical seed/run-index fault signatures | reuse pairing-validation pattern |
| DynamicSpareSharing | accepts `early`, `group_no_scratch_v2`, and `group_compressed_legacy` | S0R policy identity closed; S1 metadata work remains |
| DynamicCsvReporter::writeRuns | raw counts, outcomes, resources, work, latency | extend missing trace/provenance fields only |

Existing manifests set solution_take=legacy. `findCompressedGroupChoice` still
enumerates cross-SA candidate combinations for the explicitly historical
policy. The separate S0R V2 selector and its independent trace golden establish
the frozen greedy semantics without reinterpreting old output.

## Experiment identity contract

Each future run bundle shall contain run_config.json. Each point shall contain
point.json, command.txt, and run.log. Required run-or-point metadata is:

~~~
experiment_id
run_id
seed
git_revision
simulator_version
architecture_version
experiment_contract_version
layout
sharing_policy
solution_take_policy
memory_impl
RS
CS
sharing_degree_m
fault_model
spatial_model
fault_count
lambda_SA (when applicable)
~~~

A paired observation key is seed + run_index + normalized architecture key.
run_id alone is not a join key. Existing runner git revision/status metadata is
reusable; the target manifest must add explicit architecture/contract versions.

## Physical budget contract

For the current four-SA group:

~~~
physical_spares_per_SA = RS + CS
physical_spares_per_group = 4 * (RS + CS)
~~~

sharing_degree_m, shared_rows, and shared_columns represent redistribution
eligibility, not extra physical redundancy. Future points must record RS, CS,
sharing_degree_m, shared_rows, shared_columns, physical_spares_per_SA, and
physical_spares_per_group. They must not add shareable lines a second time.

## Raw fault and paired comparison contract

Current runs.csv already exposes fault_A, fault_B, fault_C, and fault_D. They
are the primitive source for mean, standard deviation, CV, max-minus-min, and
other imbalance metrics. A pre-binned imbalance repair rate is not a primitive
result.

All paired policies must use the same seed, generated fault map, fault
arrival/order, layout, RS/CS, sharing degree, physical spare budget, and
baseline RECAM semantics. Vary only the intended policy/architecture axis.
Required pairings are No Sharing versus Directional, EARLY versus
GROUP-NoScratch, and separately labelled 2,2,1 versus 3,3,1. Hardware points
remain an analysis axis and cannot be averaged into one curve.

## Outcome contract

Current runs.csv contains group_repair_success, per-SA repair flags, selected
attempt/candidate indices, borrowed rows/columns, selector work, feasible
combination count, early_success, group_compressed_success, and greedy_loss.
A future target raw record must expose or unambiguously derive:

~~~
selected_config_A/B/C/D
selected_pattern_A/B/C/D
borrowed_rows
borrowed_cols
total_spares_used
EARLY success
GROUP-NoScratch success
greedy_loss = (!EARLY success) && GROUP-NoScratch success
~~~

selected_candidate is not automatically a ConfigID or PatternID. First
divergence, resource-conflict detail, combinations_checked, and feasible
combinations are diagnostic fields; add them only if retained raw artifacts
cannot derive required evidence. S0 authorizes no high-volume tracing.

## CAM metadata contract

Simulator geometry values are analytical unless calibrated from RTL. Required
fields are:

~~~
num_configs
max_R_supported
max_C_supported
max_K
address_cam_entries
address_cam_bits_per_entry
address_cam_total_bits
hybrid_cam_entries
hybrid_cam_bits_per_entry
hybrid_cam_total_bits
cam_geometry_source = RTL_CALIBRATED | ANALYTICAL_REFERENCE
~~~

S0 assigns no final 3,3,1 calibrated value. Analytical bit/proxy quantities
must never be labelled as RTL area.

## Hardware characterization join contract

A later hardware_characterization.csv requires:

~~~
hardware_id
layout, sharing_policy, solution_take_policy, memory_impl
RS, CS, sharing_degree_m, num_configs
address_cam_entries, address_cam_bits_per_entry, address_cam_total_bits
hybrid_cam_entries, hybrid_cam_bits_per_entry, hybrid_cam_total_bits
total_area, comb_area, seq_area, cell_count, mapped_leaf_cell_count, GE
clock_constraint, critical_delay, WNS, TNS
analysis_cycles, analysis_latency
rtl_revision, tool, library, corner, synthesis_report_path
~~~

hardware_id derives from:

~~~
layout | sharing_policy | solution_take_policy | memory_impl |
RS | CS | sharing_degree_m | num_configs | RTL revision |
library | corner | clock constraint | synthesis boundary
~~~

A simulator record may join that ID only with
cam_geometry_source=RTL_CALIBRATED. Otherwise it carries no RTL-area claim.

## Metric definitions

| Metric | Frozen definition |
|---|---|
| repair_rate | successful group runs / total paired group runs |
| repair_rate_gain_pp | 100 × (test repair rate − reference repair rate) |
| greedy_loss_rate | count((!EARLY success) && GROUP-NoScratch success) / paired runs |
| fault imbalance | formula explicitly derived from fault_A..fault_D |
| physical spare usage | used_rows + used_columns; bounded by 4×(RS+CS) |
| borrowed line count | borrowed_rows + borrowed_columns |
| analysis cycles | explicitly labelled end-to-end simulator analysis_cycles |
| analysis latency | separately labelled RTL cycle latency or post-BIST exposed latency |

Selector work, RTL cycle latency, and post-BIST exposed latency are separate
metrics and cannot share an unlabeled column or claim.

## Artifact and workflow boundary

Future group runs remain:

~~~
reports/group/<experiment>/<run-id>/
  run_config.json
  raw/
  derived/ or csv/
  plots/
  tables/ (if applicable)
  logs/
  README.md or summary.md
~~~

RTL reports remain under results/. Existing results are not moved. A workflow
may be registered in EXPERIMENT_CATALOG only after a manifest, runner, output
schema, and source-of-truth CSV exist.

## Dependency state

~~~
S1_DEPENDENCIES = S0/S0R contract + H5 accepted actual RTL geometry/provenance
E0_DEPENDENCIES = frozen simulator contract + calibrated geometry + clean RTL
                  functional regression + hardware provenance
READY_FOR_S1 = YES
READY_FOR_E0 = NO
NEXT_PHASE_AUTHORIZED = NONE
~~~

S1 is eligible but not authorized by this document. E0 remains a downstream
dependency state, not an S0R failure.
