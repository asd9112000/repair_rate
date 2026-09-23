# Frozen G2X2_R N=2 summary-to-solution source boundary.
set recam_dss_g2x2_r_static_global_n2_sources [list \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src dss_v2_params_pkg.sv] \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src dss_v2_types_pkg.sv] \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src dss_v2_group_candidate_store.sv] \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src dss_v2_group_slot_decode.sv] \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src recam_shared_config_analyzer.sv] \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src recam_dss_g2x2_r_static_selector.sv] \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src recam_dss_g2x2_r_static_global_core.sv] \
    [file join $repo_root dss_final recam_dss_grid2x2_row_rs2_cs2_m1_static_group_global_noscratch src recam_dss_g2x2_r_static_global_top.sv]]
