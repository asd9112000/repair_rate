# Frozen L1X4_R_GROUP_V1 functional-closure source boundary.
set recam_dss_l1x4_r_static_global_n2_sources [list \
    [file join $repo_root rtl L1X4_R_GROUP_V1 dss_v2_params_pkg.sv] \
    [file join $repo_root rtl L1X4_R_GROUP_V1 dss_v2_types_pkg.sv] \
    [file join $repo_root rtl L1X4_R_GROUP_V1 recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl L1X4_R_GROUP_V1 dss_v2_group_candidate_store.sv] \
    [file join $repo_root rtl L1X4_R_GROUP_V1 dss_v2_group_slot_decode.sv] \
    [file join $repo_root rtl L1X4_R_GROUP_V1 recam_dss_l1x4_r_static_selector.sv] \
    [file join $repo_root rtl L1X4_R_GROUP_V1 recam_dss_l1x4_r_static_global_core.sv] \
    [file join $repo_root rtl L1X4_R_GROUP_V1 recam_dss_l1x4_r_static_global_top.sv]]
