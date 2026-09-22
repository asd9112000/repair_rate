# FINAL-EARLY-C2 — Four-Bit Model-B2 EARLY RTL Implementation

```text
FINAL_EARLY_C2_STATUS: COMPLETE
RTL_MODIFIED: YES
SIMULATOR_SEMANTICS_MODIFIED: NO
DSS_FINAL_MODIFIED: NO
SYNTHESIS_RUN: NO
```

## Planned and implemented architecture

| Item | Frozen plan | Implementation |
| --- | --- | --- |
| Top module | Model-B2 final EARLY boundary | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top` |
| Shared analyzer | Archive-identical `recam_shared_config_analyzer` | Direct unchanged instantiation with `HYBRID_ENTRIES=11` |
| EARLY controller | Streaming `A,B,C,D`; `R,L,RB,B` | `recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core` |
| Controller state | SA + slot rank + four common tokens | `sa_q[1:0]`, `priority_rank_q[1:0]`, `common_spare_available_o[3:0]` |
| Commit | First valid/resource-legal slot, immediate, no rollback | One SA commits per accepted edge; terminal failure leaves prior commits visible |
| Resource action | Fixed 16-entry SA/slot decode | Combinational `claim_mask` table; no demand input or generic ledger |
| Model-B2 source | One retained collector state per SA | Four external 260-bit summary inputs, each with five pivots and eleven Hybrid entries |

```text
TOP_MODULE: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top
SHARED_ANALYZER: recam_shared_config_analyzer (unchanged)
EARLY_CONTROLLER: recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core
FSM: busy + current SA + P0 priority rank; one candidate evaluation per cycle
SA_STATE_BITS: 2
SLOT_STATE_BITS: 2
COMMON_SPARE_STATE_BITS: 4
CLAIM_MASK: 4-bit combinational fixed decode
GROUP_ONLY_STRUCTURES_REMOVED: 80-bit candidate store; 81-path selector; tuple/path search; atomic group controller; generic resource ledger
```

The final top uses `HYBRID_ENTRIES=11`, rather than the historical seven-entry
input. This is the C1R3 Model-B2 retained collector envelope. Its summary is
260 bits per SA: pivot validity/addresses, six Must vectors, eleven
Hybrid-valid/pointer/descriptor/differing-address fields, and overflow. The
top multiplexes the current SA's retained summary into the unchanged analyzer.

## State and payload separation

```text
SHARING_POLICY_PERSISTENT_BITS: 4 (common_spare_available_o only)
REPAIR_DECODE_PAYLOAD_BITS: 32 committed result bits (ConfigID 12 + PatternID 16 + borrow classification 4)
REPAIR_DECODE_PAYLOAD_LIFETIME: acceptance edge through final result/remap handoff
REPAIR_DECODE_PAYLOAD_PURPOSE: preserve selected repair identity and m=1 result classification; not token ownership history
MODEL_B2_COLLECTOR_INPUT_PAYLOAD: 4 x 260 bits, retained outside the controller until its SA analysis is complete
```

`borrow_flat_o` is committed result metadata and is used only to enforce the
already-frozen `m=1` classification on later candidate admission. It is not a
resource-owner table, donor search result, or general ledger. The only
resource availability state is the four-bit monotonic token vector, reset to
`4'b1111` and updated solely as `available & ~claim_mask`.

## Group-mother classification

| Mother source | C2 classification | Treatment |
| --- | --- | --- |
| `dss_v2_params_pkg.sv`, `dss_v2_types_pkg.sv` | `IDENTICAL_TO_GROUP_MOTHER`, not instantiated | No package dependency is needed by the minimal EARLY closure. |
| `recam_shared_config_analyzer.sv` | `IDENTICAL_TO_GROUP_MOTHER` | Reused directly; no port or algorithm change. |
| `dss_v2_group_slot_decode.sv` | `EARLY_SPECIFIC` fixed replacement | The frozen 16-entry SA/slot/ConfigID table is in the controller. |
| `dss_v2_group_candidate_store.sv` | removed | No candidate history is stored. |
| `recam_dss_hyp02_static_selector.sv` | removed | No 81-path/global joint selection. |
| `recam_dss_hyp02_static_global_core.sv`, `..._top.sv` | `EARLY_SPECIFIC` replacement | Streaming controller/top replace atomic group collection/allocation. |

```text
SHARED_SOURCE_MODIFIED: NO
ANALYZER_CHANGED: NO
MODEL_B2_PRESERVED: YES
GROUP_ONLY_LOGIC_RETAINED: NO
```

## Directed C2 validation

`make test_recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_rtl` passes the 16 fixed SA/slot actions, immediate
per-SA commit, selected ConfigID/PatternID capture, token consumption,
first-failure termination, and no rollback.  `make test_final_early_c1r4_static_action`
and `make test_final_early_c1r3_shared_collector` remain the semantic gates for
the fixed action theorem and shared-collector Model-B2 basis.

```text
PLANNED_ARCHITECTURE: CONFORMS
IMPLEMENTED_ARCHITECTURE: CONFORMS
CONFORMS_TO_PLAN: YES
NEXT_ACTION: C3_EQUIVALENCE_AND_PRE_SYNTHESIS_GATE
```
