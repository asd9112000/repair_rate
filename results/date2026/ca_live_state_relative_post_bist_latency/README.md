# CA-LIVE state-relative post-BIST exposed latency

`STATUS: COMPLETE`

This experiment measures **State-relative Post-BIST exposed latency** only at
the frozen final CA-LIVE policy boundary.  It starts at an accepted policy-side
state update and sets a synthetic observation point
`T_BIST_end = T_state_update + G_state`; it does not model a raw fault,
collector, or raw BIST completion event.

Formal corpus: 10,000 matched four-SA state images, deterministic seed
`20260910`.  Each case has one EARLY and one GROUP row in
`raw/CA_LIVE_STATE_RELATIVE_BASE.csv`; pairwise image identity is verified by
the independent recomputation.

Primary results:

| Policy | `L_policy` mean | first all-zero `G_state` |
| --- | ---: | ---: |
| EARLY | 1.4334 cycles | 4 |
| GROUP | 5.0000 cycles | 5 |

`raw/`, `summary/`, and `provenance/` contain the permanent machine-readable
evidence.  The raw-fault-relative experiment remains separately blocked in
`results/date2026/ca_live_post_bist_latency/` and is not overwritten here.
