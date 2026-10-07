# CA-LIVE post-BIST exposed-latency experiment

## Status

`BLOCKED_AT_L0_PRELIGHT` (2026-10-08).  No smoke or formal simulation was
started, and this directory contains no measured trace or summary data.

## Scope

The requested experiment is restricted to the frozen final N2 CA-LIVE policy
tops (G2X2, RS=2, CS=2, m=1, Row+Column sharing):

- `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/`
- `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/`

It does not reuse the historical Phase4I V2/Specialized-EARLY or
GROUP-NoScratch latency model.

## L0 blocker

The final CA-LIVE source boundary accepts an externally supplied analyzer
state through `state_update_i`, `state_sa_i`, and `test_done_*`.  It has no
raw-fault event interface, persistent fault collector, or BIST-end input.
Consequently the following required markers cannot be captured from the final
RTL without inventing an adapter semantic:

- `T_last_fault`
- the raw-fault-to-accepted-state relation for `T_state_update`
- `T_BIST_end`

The frozen metric contract explicitly describes fault-input-to-ready as a
*separately modeled* one-cycle boundary; it is not a direct RTL measurement.
The requested experiment prohibits assuming that boundary.  This satisfies
the request's stop condition that `T_state_update` must be consistently
defined from the final source before proceeding.

The complete source and contract evidence is in
[`provenance/L0_PREFLIGHT.md`](provenance/L0_PREFLIGHT.md).  The blocked final
characterization record is
[`docs/date2026/latency/CA_LIVE_POST_BIST_LATENCY_CHARACTERIZATION.md`](../../../docs/date2026/latency/CA_LIVE_POST_BIST_LATENCY_CHARACTERIZATION.md).

## Result-state contract

`raw/` and `summary/` intentionally contain no numerical experiment output.
Neither a 100-trace smoke nor a 10,000-trace paired corpus exists for this
phase.  Their presence must not be inferred from the directory layout.
