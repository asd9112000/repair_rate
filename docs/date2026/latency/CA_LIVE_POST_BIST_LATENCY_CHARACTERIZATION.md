# Final CA-LIVE post-BIST exposed-latency characterization

## Status

`DATE2026-CA-LIVE-POST-BIST-LATENCY: BLOCKED_AT_L0_PREFLIGHT`

Date: 2026-10-08

RTL changed: `NO`

CA-LIVE semantics changed: `NO`

Simulation started: `NO`

This is a blocked-characterization record, not a numerical latency result.

## Architecture identity

| Property | Final CA-LIVE scope |
| --- | --- |
| family | N2 / G2X2 / RS=2 / CS=2 / m=1 / Row+Column sharing |
| policies | EARLY CA-LIVE and GROUP CA-LIVE |
| source family | `dss_final/continuous_analysis_live` frozen policy boundary |
| EARLY controller | `recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_core.sv` |
| GROUP controller | `recam_dss_hyp02_static_global_live_state_core.sv` |

The separate historical Phase4I record concerns V2 Specialized-EARLY and
GROUP-NoScratch.  Its collector/model and latency formula are not reused
here; see [the Phase4I provenance audit](PHASE4I_POST_BIST_LATENCY_PROVENANCE_AUDIT.md).

## L0 measurement-boundary result

The final policy tops begin at an accepted analyzer-state generation:
`state_update_i`, `state_sa_i`, `test_done_valid_i`, and `test_done_sa_i`.
They have no raw-fault event, persistent fault collector, or BIST-end input.
The final metric contract confirms that its direct RTL metric is
state-update-to-solution-ready; raw-fault-to-state-update is separately
modeled and is not implemented by the CA top wrappers.

Therefore the required marker set cannot be defined from the frozen final
RTL:

| Marker | Status |
| --- | --- |
| `T_last_fault` | `NOT_IMPLEMENTED` at this boundary |
| `T_state_update` | policy-side input only; no raw-fault provenance |
| `T_decision_ready` | implemented (`done_o` / `solution_ready_o`) |
| `T_BIST_end` | `NOT_IMPLEMENTED` at this boundary |
| `G = T_BIST_end - T_last_fault` | `NOT_EVALUABLE` |

It would be unsound to introduce a one-cycle raw-fault-to-state assumption or
to backfill those events from Phase4I.  The requested stop condition is thus
met before L1.

## Required phase disposition

| Phase | Status | Evidence |
| --- | --- | --- |
| L0 provenance/architecture audit | `PASS_WITH_BLOCKER` | frozen sources and contracts audited |
| L1 100-trace paired smoke | `NOT_RUN` | L0 marker contract absent |
| L2 10,000-trace paired formal corpus | `NOT_RUN` | L1 cannot be authorized |
| L3 independent summary recomputation | `NOT_RUN` | no raw corpus |
| L4 documentation closure | `BLOCKED_RECORD_COMPLETE` | this record and preflight provenance |

## What remains valid, but is not this measurement

Existing frozen CA-LIVE evidence supports a policy-side timing statement:

- EARLY has selected legal-solution rank latency 1--4 cycles from accepted
  state update.
- GROUP has fixed 5-cycle latency from accepted state update.

Those are **not** `T_last_fault`-relative or BIST-relative measurements and
must not be relabeled as exposed post-BIST latency.

## Required evidence to unblock

An authorized future implementation would need a final-architecture,
source-anchored raw fault/CAM collector boundary that defines all of:

1. a raw fault change event and `T_last_fault`, including nonchanging events;
2. the exact accepted state-generation update causally produced by each event;
3. a BIST-end event in the same cycle domain; and
4. preservation of the final CA-LIVE policy outcome under the requested
   directed adapter-versus-RTL checks.

Only after that evidence exists may an exact `G=0..32` sweep be run.  Such an
implementation would be a new approved architecture/measurement effort, not
a reinterpretation of the present final CA-LIVE policy evidence.

## Artifact locations

- Preflight provenance:
  [`results/date2026/ca_live_post_bist_latency/provenance/L0_PREFLIGHT.md`](../../../results/date2026/ca_live_post_bist_latency/provenance/L0_PREFLIGHT.md)
- Result-root status:
  [`results/date2026/ca_live_post_bist_latency/README.md`](../../../results/date2026/ca_live_post_bist_latency/README.md)
- Evidence index: [LATENCY_EVIDENCE_INDEX.md](LATENCY_EVIDENCE_INDEX.md)
