# Remaining-four N2 CA-LIVE source freeze

Freeze gate: `PASS`. The final structural audit passed before this manifest
was written. The frozen analyzer interface is 272 bits for every case; the
corpus is 10,000 vectors with seed `0x20260928`.

## G2X2_R_EARLY

| Role | Path | SHA256 |
| --- | --- | --- |
| CA-LIVE RTL | `rtl/G2X2_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_g2x2_r_static_early_live_state_core.sv` | `2665d152990c109fd00a7630cb54dba46f8a5265c3f3419fa0971db45dc6d69f` |
| CA-LIVE RTL | `rtl/G2X2_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_g2x2_r_static_early_live_state_top.sv` | `66044d4332b9d24bc04224e1129790ea142e09bf2a8ef11c9d9e422c98dc7bd2` |
| Analyzer | `rtl/G2X2_N2_R_EARLY/recam_shared_config_analyzer.sv` | `0f443fc86d7b305720367b608ef8b8a1bf640b1845359f8ced58630ee983ceaf` |
| Path logic | `rtl/G2X2_N2_R_EARLY/dss_early_selected_address_mux.sv` | `24f3b7d4835cfc07f40e2d7d523370151b088479718fd373f1ec97d813f3000e` |
| Full-top TB | `tb/n2_continuous_analysis/tb_g2x2_r_early_live_full_top.sv/.cpp` | `c236d1072b66e7de9187568c1a49dab39163568a5d925bf186b21ef84d6531fe`; `2bdd3ab5eee8a74ff8411451c9300a6ed66f9beffd088d4f4495f6f6d1717157` |
| Directed TB | `tb/n2_continuous_analysis/tb_ca_live_early_directed.sv/.cpp` | `9d09873a4fdf790c236c3b1d39be64a41e7119b071903e117bd2ed525b76fe89`; `71afc6a4890ce7043cf6a1c32d3e89f5549a2216cd715e475e67e6ed557d3079` |
| Latency harness / data | `tb/n2_continuous_analysis/tb_ca_latency_early.cpp`; `EARLY_CONFIG_RANK_DISTRIBUTION.csv` | `0367a41ad4d73a3e23011cd05917dc2fabfc68785a2c97b58f1d6fca6c6ba4a9`; `b15660c0d61e0bc8f61ba692a55c5c48d19ba98792b8c5525af90ab26af006f6` |

## G2X2_R_GROUP

| Role | Path | SHA256 |
| --- | --- | --- |
| CA-LIVE RTL | `rtl/G2X2_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_g2x2_r_static_global_live_state_core.sv` | `8208a43b7bfc22e655336c10d14c17554cc2d781337beccc6aca3a062bb873d7` |
| CA-LIVE RTL | `rtl/G2X2_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_g2x2_r_static_global_live_state_top.sv` | `9b45d0e102151c824d4fb7350a057833c530125c5cf53ad1a21ec6931fda73ac` |
| Analyzer | `rtl/G2X2_N2_R_GROUP_reg/recam_shared_config_analyzer.sv` | `0f443fc86d7b305720367b608ef8b8a1bf640b1845359f8ced58630ee983ceaf` |
| Selector / map | `rtl/G2X2_N2_R_GROUP_reg/recam_dss_g2x2_r_static_selector.sv`; `dss_v2_group_candidate_store.sv` | `034c1063533e17e326235a9dc4eafb698c1e1f867b284fc34cc28121018d95b0`; `391eee943ba1aa1e5cb6e59cc8e64b7643eb979d277595ccfaa7bc961155c0c2` |
| Full-top TB | `tb/n2_continuous_analysis/tb_g2x2_r_group_live_full_top.sv/.cpp` | `a1f3eea44296e90db310dd894e385b90c5f78cc8acfa460f0bf8f13c67fafb3f`; `52c62fe4de8573aa727a4e133b705d002f16026eb84cac6305b09add461a5149` |
| Directed TB | `tb/n2_continuous_analysis/tb_ca_live_group_directed.sv/.cpp` | `435cabed3a34e6470276b52fc498f10aa0a2b6f1219f5ff075b8fcec241c68cb`; `a905e75e15d480461070b8583301619baf7997353df81b1c8cf8f6517e508f5f` |
| Latency harness / data | `tb/n2_continuous_analysis/tb_ca_latency_group.cpp`; `GROUP_LATENCY_DISTRIBUTION.csv` | `abd71ded6ff94b4302071b555217481bc466996366b8a27dbbc0d82c5be7caca`; `0b03d88d7a932bb04e194e9192dcc93b62144461043e50517c920137eff1524f` |

## L1X4_R_EARLY

| Role | Path | SHA256 |
| --- | --- | --- |
| CA-LIVE RTL | `rtl/L1X4_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_l1x4_r_static_early_live_state_core.sv` | `5b8a2165253e0ff63ab4c1520ca6ace346beae64a36268bd19fbe08daa782c20` |
| CA-LIVE RTL | `rtl/L1X4_N2_R_EARLY_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_l1x4_r_static_early_live_state_top.sv` | `40fd334683620892dca32eee5ad582052171565e37aeb8abd4c020ad31cd614f` |
| Analyzer | `rtl/L1X4_N2_R_EARLY/recam_shared_config_analyzer.sv` | `0f443fc86d7b305720367b608ef8b8a1bf640b1845359f8ced58630ee983ceaf` |
| Path logic | `rtl/L1X4_N2_R_EARLY/dss_early_selected_address_mux.sv` | `24f3b7d4835cfc07f40e2d7d523370151b088479718fd373f1ec97d813f3000e` |
| Full-top TB | `tb/n2_continuous_analysis/tb_l1x4_r_early_live_full_top.sv/.cpp` | `9f04fbe3269f3151f59ec1e45521745bd5c024276b733d140740b72c26f5856c`; `e0e71b1f2ed5f5f92bd98846168a200995b350e2d47e5b39802f23600d848894` |
| Directed TB | `tb/n2_continuous_analysis/tb_ca_live_early_directed.sv/.cpp` | `9d09873a4fdf790c236c3b1d39be64a41e7119b071903e117bd2ed525b76fe89`; `71afc6a4890ce7043cf6a1c32d3e89f5549a2216cd715e475e67e6ed557d3079` |
| Latency harness / data | `tb/n2_continuous_analysis/tb_ca_latency_early.cpp`; `EARLY_CONFIG_RANK_DISTRIBUTION.csv` | `0367a41ad4d73a3e23011cd05917dc2fabfc68785a2c97b58f1d6fca6c6ba4a9`; `b15660c0d61e0bc8f61ba692a55c5c48d19ba98792b8c5525af90ab26af006f6` |

## L1X4_R_GROUP

| Role | Path | SHA256 |
| --- | --- | --- |
| CA-LIVE RTL | `rtl/L1X4_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_l1x4_r_static_global_live_state_core.sv` | `8be8d14d9c901a48d4d939d58c8fa87d02af144848f80ca150aac6947cf8f389` |
| CA-LIVE RTL | `rtl/L1X4_N2_R_GROUP_CONTINUOUS_ANALYSIS_LIVE_STATE_reg/recam_dss_l1x4_r_static_global_live_state_top.sv` | `e6a308b08817c2b109de12b98eea44436ca6e9255343e57865effdeacfcc60a0` |
| Analyzer | `rtl/L1X4_N2_R_GROUP_reg/recam_shared_config_analyzer.sv` | `0f443fc86d7b305720367b608ef8b8a1bf640b1845359f8ced58630ee983ceaf` |
| Selector / map | `rtl/L1X4_N2_R_GROUP_reg/recam_dss_l1x4_r_static_selector.sv`; `dss_v2_group_candidate_store.sv` | `7fc4b57b6ee484e1ecf304d3c5ec2a24eb7de8bcd0be73ac8091c81f81ebf489`; `391eee943ba1aa1e5cb6e59cc8e64b7643eb979d277595ccfaa7bc961155c0c2` |
| Full-top TB | `tb/n2_continuous_analysis/tb_l1x4_r_group_live_full_top.sv/.cpp` | `27ed7bca4568594cc7f66dd44bf9786f920054e5dfdd8154f4d461a75f8a58b9`; `5e07ec1b5d9903cb95d6661ea8ff94c0023fc479d8ec4912ed279b7ea3a666d9` |
| Directed TB | `tb/n2_continuous_analysis/tb_ca_live_group_directed.sv/.cpp` | `435cabed3a34e6470276b52fc498f10aa0a2b6f1219f5ff075b8fcec241c68cb`; `a905e75e15d480461070b8583301619baf7997353df81b1c8cf8f6517e508f5f` |
| Latency harness / data | `tb/n2_continuous_analysis/tb_ca_latency_group.cpp`; `GROUP_LATENCY_DISTRIBUTION.csv` | `abd71ded6ff94b4302071b555217481bc466996366b8a27dbbc0d82c5be7caca`; `0b03d88d7a932bb04e194e9192dcc93b62144461043e50517c920137eff1524f` |

Frozen case sources must not be modified before their matched 20 ns synthesis
results are collected.
