# L0 preflight: CA-LIVE post-BIST exposed latency

## Outcome

`STATUS: BLOCKED`

The requested raw-fault/BIST-relative latency experiment cannot be performed
against the frozen final CA-LIVE policy source without adding or assuming a
non-source-anchored fault-to-state adapter.  No adapter was created and no
simulation was launched.

## Frozen source identity

| Policy | Source | SHA-256 | Tracked |
| --- | --- | --- | --- |
| EARLY CA-LIVE | `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_core.sv` | `ad9abfbe5ee2ed31b63d0bcc46998a002bceae48360e684c300f02a27cd563b6` | yes |
| EARLY CA-LIVE | `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_top.sv` | `706e90b26b98a90f3b26951932755eb80d2007092e0861f0339782fc80722adc` | yes |
| GROUP CA-LIVE | `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_hyp02_static_global_live_state_core.sv` | `745cf6a87a94ba82a658d1252ecdf2f69f6f007de5740803afd9719c5779bb90` | yes |
| GROUP CA-LIVE | `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_hyp02_static_global_live_state_top.sv` | `d2ddfeda78f9c3eaf6054a1c38ff2e50220bf5669f18928535bb4fbf6e6f29cf` | yes |

These match the frozen values in
`dss_latency/N2_CONTINUOUS_ANALYSIS/provenance/CA_LIVE_SOURCE_FREEZE.md`.

## Interface audit

Both final policy tops expose the accepted policy-side interface:

```text
clk_i, rst_ni,
state_update_i, state_sa_i,
test_done_valid_i, test_done_sa_i,
<captured analyzer-state projection>,
<policy result outputs>
```

They do **not** expose a raw-fault valid/data event, fault collector state, or
`BIST_end` input.  See:

- `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_top.sv`, lines 10--44.
- `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_hyp02_static_global_live_state_top.sv`, lines 10--48.
- `tb/n2_continuous_analysis/tb_n2_ca_live_early_lockstep_top.sv`, lines 3--27.
- `tb/n2_continuous_analysis/tb_n2_ca_live_group_lockstep_top.sv`, lines 3--31.

`BIST_end` cannot affect controller execution through either policy-top
interface.  This is an interface observation, not evidence for a raw-fault
timing model.

## Contract audit

The final evidence defines the measured origin as a **registered
state-generation update edge**, explicitly excluding BIST-done waiting:

- `dss_latency/N2_CONTINUOUS_ANALYSIS/METRIC_CONTRACT.md` defines
  `STATE_UPDATE_TO_SOLUTION_READY`, and says fault-input-to-ready adds a
  separately modeled one-cycle fault-to-state boundary.
- `dss_latency/N2_CONTINUOUS_ANALYSIS/CYCLE_SEMANTICS.md` states that the raw
  fault-to-state-update edge is not implemented by these CA top wrappers.
- `dss_latency/N2_CONTINUOUS_ANALYSIS/N2_CA_LIVE_EXPERIMENT_RECORD.md` labels
  `RAW_FAULT_END_TO_END` as `NOT_IMPLEMENTED` and says raw-fault latency is
  not an RTL measurement in this experiment.
- `docs/dss_execution/S1E_CONCURRENT_BIST_DSS_OVERLAP_SEMANTICS_AUDIT.md`
  records that frozen EARLY has no fault-state update path and the current
  harness supplies final analyzer-derived candidates before policy activity.

## Required-marker disposition

| Required marker or check | Disposition | Reason |
| --- | --- | --- |
| `T_last_fault` | `NOT_IMPLEMENTED` | No raw-fault event input/collector in final CA-LIVE policy boundary. |
| `T_state_update` | `POLICY_INPUT_ONLY` | `state_update_i` is observable, but no source-defined link to a raw fault exists. |
| `T_decision_ready` | `IMPLEMENTED` | `done_o` (EARLY) / `solution_ready_o` (GROUP) exist after an accepted state update. |
| `T_BIST_end` | `NOT_IMPLEMENTED` | No BIST-end input/event exists in the final policy tops. |
| `G = T_BIST_end - T_last_fault` | `NOT_EVALUABLE` | Both operands are outside this source boundary. |
| trailing nonchanging raw events | `NOT_EVALUABLE` | No raw event trace is consumed or retained by the final CA-LIVE policy source. |
| exact `G=0..32` sweep | `BLOCKED` | Would require an invented external timing relation. |

## Prohibited substitutes not used

- No one-cycle `T_state_update = T_last_fault + 1` assumption.
- No historical V2/Phase4I timing formula, collector, or raw trace reuse.
- No adapter declared equivalent to RTL without the required raw-event
  semantics and directed proof.
- No smoke, formal corpus, functional comparison, or summary recomputation.

## Phase boundary

Historical Phase4I evidence remains in
`results/phase4i/phase4i2_raw_latency.csv` and
`docs/date2026/latency/PHASE4I_POST_BIST_LATENCY_PROVENANCE_AUDIT.md`.  It is
not an input to this final CA-LIVE experiment.  The historical commit
`3304c86` is an ancestor of the current branch, while the CA-LIVE closure is
recorded by `9bb9566`; ancestry does not make their architecture-specific
latency semantics interchangeable.
