# DATE2026 latency evidence index

This index keeps architecture-specific latency evidence separate.  A result
may only be interpreted within the source/model family named below.

| Evidence | Architecture/model family | Measurement origin | Status | Interpretation boundary |
| --- | --- | --- | --- | --- |
| [Phase4I post-BIST provenance audit](PHASE4I_POST_BIST_LATENCY_PROVENANCE_AUDIT.md) | **HISTORICAL V2** Specialized-EARLY / GROUP-NoScratch | raw-fault-origin historical model | audit complete | Historical reference only; not final CA-LIVE primary evidence |
| [Final CA-LIVE raw-fault post-BIST characterization](CA_LIVE_POST_BIST_LATENCY_CHARACTERIZATION.md) | Final N2 CA-LIVE policy boundary | raw/BIST markers absent at policy boundary | **BLOCKED_BY_INTERFACE_BOUNDARY** | No raw-fault/BIST result exists |
| [Final CA-LIVE state-relative characterization](CA_LIVE_STATE_RELATIVE_POST_BIST_LATENCY.md) | Final N2 CA-LIVE policy boundary | accepted state update to policy ready; synthetic `G_state` observation | **FINAL_POLICY_BOUNDARY** | Paper/thesis usable with the collector exclusion stated |
| `dss_latency/N2_CONTINUOUS_ANALYSIS/N2_CA_LIVE_EXPERIMENT_RECORD.md` | Final N2 CA-LIVE policy boundary | accepted state update to policy ready | frozen closure | Raw-fault end-to-end is not implemented |

The Phase4I raw-fault-origin model and final CA-LIVE state-relative model are
intentionally not combined in a single latency distribution or direct numerical
comparison.  They have different source interfaces, origin markers, and timing
contracts.
