# N2 CA-LIVE source freeze

This manifest freezes the source closure used for the CA-LIVE pre-synthesis
functional evidence. SHA256 values are taken relative to the repository root.
The test sources below are the exact directed, lockstep, and full-top
Verilator harnesses used in this closure. The live policy tops retain no raw
fault collector; live analyzer state remains an upstream interface.

## EARLY-CA-LIVE

| Relative path | SHA256 | Role |
| --- | --- | --- |
| `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_core.sv` | `ad9abfbe5ee2ed31b63d0bcc46998a002bceae48360e684c300f02a27cd563b6` | live serial controller |
| `rtl/G2X2_N2_RC_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_grid2x2_directional_rs2_cs2_m1_hyp02_early_live_state_top.sv` | `706e90b26b98a90f3b26951932755eb80d2007092e0861f0339782fc80722adc` | policy top |
| `rtl/G2X2_N2_RC_EARLY/recam_shared_config_analyzer.sv` | `0f443fc86d7b305720367b608ef8b8a1bf640b1845359f8ced58630ee983ceaf` | frozen canonical analyzer |
| `rtl/G2X2_N2_RC_EARLY/dss_early_selected_address_mux.sv` | `24f3b7d4835cfc07f40e2d7d523370151b088479718fd373f1ec97d813f3000e` | frozen selected-address formatter |
| `tb/n2_continuous_analysis/tb_n2_ca_live_early_lockstep_top.sv` | `e68d6aa6c633b1d4ad5ea4c5f14808228cdb18a7965342f79a01735493bd3602` | canonical/live policy lockstep harness |
| `tb/n2_continuous_analysis/tb_n2_ca_live_early_lockstep.cpp` | `16d92d4c67135cbe2e165aa3c8b902cae48e9ecd9e19ebb3359ad01eefed4aaa` | 10k lockstep and latency measurement |
| `tb/n2_continuous_analysis/tb_n2_ca_live_early_directed.cpp` | `108d92e42c0c6da3e5ad334399f7014c39cabdfe985ab3170ba88b6588d3dff5` | vector-27 and same-edge update regression |

## GROUP-CA-LIVE

| Relative path | SHA256 | Role |
| --- | --- | --- |
| `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_hyp02_static_global_live_state_core.sv` | `745cf6a87a94ba82a658d1252ecdf2f69f6f007de5740803afd9719c5779bb90` | live serial controller |
| `rtl/G2X2_N2_RC_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_hyp02_static_global_live_state_top.sv` | `d2ddfeda78f9c3eaf6054a1c38ff2e50220bf5669f18928535bb4fbf6e6f29cf` | policy top |
| `rtl/G2X2_N2_RC_GROUP_reg/dss_v2_params_pkg.sv` | `4027546f2033f5982ef70d326e9f584e966d92ff75b4cb8dad90e3f7d31509b9` | frozen parameter package |
| `rtl/G2X2_N2_RC_GROUP_reg/dss_v2_types_pkg.sv` | `c693582bf6f0236cf75050dfd9a9f2f11abbf477de05f857c35ce9256375edb5` | frozen type package |
| `rtl/G2X2_N2_RC_GROUP_reg/recam_shared_config_analyzer.sv` | `0f443fc86d7b305720367b608ef8b8a1bf640b1845359f8ced58630ee983ceaf` | frozen canonical analyzer |
| `rtl/G2X2_N2_RC_GROUP_reg/dss_v2_group_candidate_store.sv` | `7ca8e4b2219b0d16b0b05f56d32f1c783a8b6480e0a0a78124a08b2cf007e3db` | frozen 80-bit candidate map |
| `rtl/G2X2_N2_RC_GROUP_reg/dss_v2_group_slot_decode.sv` | `69b713b8bf36bcc715ffefedea7b215a04fc47a613012239d83e232fe22d4fd7` | frozen slot/config decode |
| `rtl/G2X2_N2_RC_GROUP_reg/recam_dss_hyp02_static_selector.sv` | `5edc23a40e97da04c5c9e105699ec60821a507bb467cf597890c104df077b740` | frozen 81-path selector |
| `rtl/G2X2_N2_RC_GROUP_reg/dss_group_pivot_address_regs.sv` | `539f3cce34b68ea79a8f6876641233f9cb126b00c18fdfb0a83f5166dadff3dd` | frozen 440-bit reconstruction retention |
| `rtl/G2X2_N2_RC_GROUP_reg/recam_dss_hyp02_static_global_core.sv` | `29d18300d87ef107df64d34702dad288ba76c829f4f8bbbdf8d270dae5ccb64c` | canonical policy comparator |
| `rtl/G2X2_N2_RC_GROUP_reg/recam_dss_hyp02_static_global_top.sv` | `9ea139f981255f82eec18d78d591cfe9658b1a284cf351636e6d2296bc2ffbf2` | canonical full-top comparator |
| `tb/n2_continuous_analysis/tb_n2_ca_live_group_lockstep_top.sv` | `cfa9e05990f4dad0e3bee530c8556672826c911601a3455c96801af7601d3865` | canonical/live policy lockstep harness |
| `tb/n2_continuous_analysis/tb_n2_ca_live_group_lockstep.cpp` | `05faa5a32294817dc21035f30c96de88e094c5662dddab506397acfa5a7fd322` | 10k lockstep and latency measurement |
| `tb/n2_continuous_analysis/tb_n2_ca_live_group_directed.cpp` | `8d57044a2240130eba0428193542533785c1da5ad95df092401cbb8ea69c586b` | same-edge map-write suppression regression |
| `tb/n2_continuous_analysis/tb_n2_ca_live_group_full_top.sv` | `71af837993a84b39639d97ba528c52cfd935d44079936dfc28cd99d012d8593a` | frozen-analyzer full-top harness |
| `tb/n2_continuous_analysis/tb_n2_ca_live_group_full_top.cpp` | `fa1a487521f9115dae73bdf14555b6cb538dabc14b636b6e53ef4574d3ca309e` | physical-address lockstep |

## Frozen hash gates

The EARLY and GROUP analyzer files are byte-identical with SHA256
`0f443fc86d7b305720367b608ef8b8a1bf640b1845359f8ced58630ee983ceaf`.
The GROUP selector SHA256 is
`5edc23a40e97da04c5c9e105699ec60821a507bb467cf597890c104df077b740`.
