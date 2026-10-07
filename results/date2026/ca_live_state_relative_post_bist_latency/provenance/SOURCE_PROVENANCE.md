# Source provenance

## Final CA-LIVE sources

The current files match the frozen values in
`dss_latency/N2_CONTINUOUS_ANALYSIS/provenance/CA_LIVE_SOURCE_FREEZE.md`.

| Policy | Source | SHA-256 |
| --- | --- | --- |
| EARLY | `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_core.sv` | `ad9abfbe5ee2ed31b63d0bcc46998a002bceae48360e684c300f02a27cd563b6` |
| EARLY | `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_top.sv` | `706e90b26b98a90f3b26951932755eb80d2007092e0861f0339782fc80722adc` |
| GROUP | `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_hyp02_static_global_live_state_core.sv` | `745cf6a87a94ba82a658d1252ecdf2f69f6f007de5740803afd9719c5779bb90` |
| GROUP | `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_hyp02_static_global_live_state_top.sv` | `d2ddfeda78f9c3eaf6054a1c38ff2e50220bf5669f18928535bb4fbf6e6f29cf` |

`CA_LIVE_SOURCE_MATCH: PASS`

## Test-only infrastructure

| File | Purpose | SHA-256 |
| --- | --- | --- |
| `tb/n2_continuous_analysis/tb_n2_ca_live_state_relative_top.sv` | combined EARLY/GROUP lockstep composition | `109692cccde2c36e5acf1bb87a6840150bf984bcc69c5b3fc6ede49c4e1d599b` |
| `tb/n2_continuous_analysis/tb_n2_ca_live_state_relative.cpp` | timestamped state-relative harness | `dd1473988c94506c5410e2b04e4f8ce9b1a7436be34845ac6fb23af473bf1dcc` |
| `tb/n2_continuous_analysis/ca_live_state_relative_recompute.cpp` | independent raw-CSV summary recomputation | `480500325d4931d8c4e4e01f9665268e46e2ce269811ba9d736f79b9d3be68fb` |

The harness drives the same candidate-state image and update/test-done event
sequence to both final policy cores.  It compares EARLY and GROUP separately
against their established frozen canonical policy references.

## Boundary exclusion

No raw-fault collector, collector-to-state adapter, or BIST-end RTL port is
introduced.  Historical Phase4I (`3304c86`) is a V2 raw-fault-origin model and
is `HISTORICAL_REFERENCE_ONLY` for this final CA-LIVE result.
