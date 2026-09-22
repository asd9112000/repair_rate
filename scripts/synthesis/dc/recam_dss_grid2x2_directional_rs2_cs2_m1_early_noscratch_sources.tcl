# Corrected Model-B2 EARLY decision/resource-management boundary.
set recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_sources [list \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_core.sv] \
    [file join $repo_root rtl recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch recam_dss_grid2x2_directional_rs2_cs2_m1_early_noscratch_top.sv]]
