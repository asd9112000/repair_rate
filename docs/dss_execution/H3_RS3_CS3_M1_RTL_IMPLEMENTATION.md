# H3 — RS=CS=3, SHARE_M=1 RTL Implementation

> 文件狀態：Complete
> 適用範圍：isolated 2x2 Directional CAM DSS V2-derived 3,3,1 RTL implementation
> 建立時間：2026-09-13T15:44:24+08:00
> 最後修改時間：2026-09-13T15:58:53+08:00

## Pre-edit reuse and implementation plan

This plan was recorded before the first H3 RTL edit. The repository matches the
H0/H1/H2 assumptions: the existing V2 point remains fixed at 2,2,1; its
analyzer has a 5x5 matrix, at most ten candidates, four-bit PatternID, five
pivot inputs, and seven Hybrid inputs. Its ledger remains a four-resource,
single-borrower m=1 implementation.

| Existing module/file | Reuse classification | Target module/file | Planned modification | Reason | 2,2,1 regression risk | H4 obligation |
|---|---|---|---|---|---|---|
| rtl/recam/recam_shared_config_analyzer.sv | NEW_MODULE_JUSTIFIED | rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_shared_config_analyzer.sv | New isolated 7x7, 35-candidate analyzer | Frozen source cannot represent target bounds | None; source untouched | all classes, candidates, transpose, Must, pivot/Hybrid limits |
| rtl/dss_v2/adapter/dss_legacy_config_adapter.sv | NEW_MODULE_JUSTIFIED | rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_config_table.sv | New architecture-point table | Preserve historical ConfigID meanings | None | seven table entries and role slots |
| rtl/dss_v2/topology/dss_topology_2x2_directional.sv | EXTEND_EXISTING, isolated target copy | rtl/dss_v2/rs3cs3m1/dss_v2_rs3cs3m1_topology.sv | Target descriptor/action legality | Existing assertion rejects RS/CS=3 | None | actions, donor order, resource legality |
| rtl/dss_v2/resource/dss_v2_resource_ledger.sv | REUSE_AS_IS | unchanged | Instantiate unchanged four-resource atomic ledger | m=1 resource semantics identical | Low | conservation, no double allocation |
| rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv | REUSE_AS_IS | unchanged | Instantiate unchanged diagnostic adapter | canonical resource state unchanged | Low | ledger observability |
| recam_dss_v2_early_core/top.sv | EXTEND_EXISTING, isolated target copy | target early core/top | Four target slots, six-bit PatternID, target analyzer/table | preserve EARLY traversal/commit | None | EARLY golden/action/ledger trace |
| group store/core/top/reader/decoder | EXTEND_EXISTING, isolated target copy | target GROUP modules | 112-bit valid+PatternID history, target priority/decode | preserve greedy GROUP-NoScratch | None | store packing, rank, GROUP golden |
| existing Verilator scripts/tests | EXTEND_EXISTING | H3 target smoke script/test | compile/lint and bounded structural smoke only | H4 owns independent golden closure | Low | later full directed/random regression |

Frozen policy boundary: EARLY and GROUP-NoScratch remain separate greedy
policies. H0S0-CL-004 remains open; DynamicSpareSharing GroupCompressed is not
a target golden source. H2-CL-005 remains a stale execution gate.

## Planned target namespace

All new implementation files will be under rtl/dss_v2/rs3cs3m1/ and use the
dss_v2_rs3cs3m1 prefix. This makes the architecture point explicit and avoids
modifying frozen 2,2,1 sources.

## H3 implementation limits

No full independent target golden, randomized closure, synthesis, simulator
changes, experiment changes, or Scratch work are in scope. H3 smoke will test
compile/elaboration, all target table/action classes, target analyzer maximum
interfaces, and the unchanged frozen 2,2,1 focused regression.


## Implemented target RTL

New isolated target namespace: rtl/dss_v2/rs3cs3m1/.

| File | Role |
|---|---|
| dss_v2_rs3cs3m1_config_table.sv | Versioned T0..T6 table: physical R/C, canonical class, transpose, semantic action. |
| dss_v2_rs3cs3m1_shared_config_analyzer.sv | 7 pivots, 17 Hybrids, 7x7 active-masked matrix, deterministic 35-candidate bitmap and six-bit PatternID. |
| dss_v2_rs3cs3m1_topology.sv | Target legality/action mapping with frozen four-resource directional donor order. |
| dss_v2_rs3cs3m1_group_candidate_store.sv | 16 x {valid, PatternID[5:0]} = 112 registered bits. |
| recam_dss_v2_rs3cs3m1_early_core.sv / early_top.sv | Streaming EARLY traversal, reused atomic ledger, isolated target analyzer/table binding. |
| recam_dss_v2_rs3cs3m1_group_core.sv / group_top.sv | Collect-then-greedy GROUP-NoScratch traversal, reused atomic ledger, isolated target analyzer/table binding. |

No frozen 2,2,1 RTL source was modified. The existing resource ledger and
legacy diagnostic adapter are instantiated unchanged. The target topology is
isolated because the frozen source intentionally rejects RS=CS=3; its resource
ownership and donor semantics are unchanged.

The table implements all seven frozen physical configurations. It separates
physical descriptor/transpose information from action semantics. The analyzer
canonicalizes R/C under transpose, swaps Must threshold sources under transpose,
inverts the frozen Hybrid descriptor relation, evaluates exactly C(K,R)
candidates, and drives inactive bits of candidate_valid[34:0] low. PatternID
zero is invalid; valid IDs are one-based and six bits wide.

## Retained state and observability

The target GROUP candidate history implements the H2 minimum exactly:

~~~
4 SA x 4 slots x (valid + PatternID[5:0]) = 112 bits
~~~

No ConfigID, descriptor, action, or candidate bitmap is stored per record.
On the same inclusive accounting scope as the frozen 157-bit GROUP report,
this RTL has 197 retained bits: 112 history, 16 ledger, 2 ledger-status,
11 controller/lifecycle, 36 selected ConfigID/PatternID, and 20 action
diagnostic bits. This is state accounting only, not synthesis area.

Both target tops expose selected config, selected PatternID, donor,
borrow/release, commit bitmap, final diagnostic ledger, failure position, and
group repairability. H4 can add test-only traces without production state.

## H3 smoke validation

Executed results:

| Check | Result |
|---|---|
| Target EARLY top lint/elaboration | PASS |
| Target GROUP top lint/elaboration | PASS |
| Seven target config-table entries and semantic slots | PASS |
| Seven target analyzer candidate counts | PASS |
| K=7, PatternID[5], pivot index 6, Hybrid index 16 structural input | PASS |
| 112-bit GROUP history read/write | PASS |
| Target EARLY all-local end-to-end smoke | PASS |
| Target GROUP all-local end-to-end smoke | PASS |
| Frozen 2,2,1 Phase 3HI directed matrix | 15/15 PASS |
| Frozen 2,2,1 RTL-vs-golden | EARLY 1000: 0 mismatch; GROUP 1000: 0 mismatch |

Commands used were direct Verilator lint of both target tops;
run_verilator_test.sh for the target config table, analyzer, store, EARLY top,
and GROUP top; and bash scripts/simulation/run_recam_phase3hi_functional.sh.

The bounded H3 smoke does not claim non-local action-flow, Must-threshold, or
transpose functional equivalence. These remain H4 independent-golden tests.

## H4 required tests

- all seven target configs, candidate bitmaps, PatternID ordering/boundaries,
  transpose pairs, selected-envelope Must thresholds, and max pivot/Hybrid;
- EARLY and GROUP directed and randomized independent RTL-vs-golden checks,
  including action/donor, failure, per-commit/final ledger;
- no illegal allocation, no rollback, first failure, latest-ledger behavior;
- frozen 2,2,1 full regression; and
- paired outcome accounting: BOTH_PASS, EARLY_ONLY, GROUP_ONLY, BOTH_FAIL.

## Phase control

~~~
H3 = COMPLETE
READY_FOR_H4 = YES
NEXT_PHASE_AUTHORIZED = NONE
H0S0-CL-004 = OPEN
H2-CL-005 = STALE_EXECUTION_GATE
~~~

No simulator behavior, experiment workflow, synthesis characterization, or
historical evidence was changed.
