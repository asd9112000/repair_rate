# P3-4PT-SYNTH / SYN-B only: the frozen OPT1 class-collapsed 2x2 GLOBAL
# integrated boundary.  The OPT0 public wrapper is deliberately excluded.
set p3_4pt_synth_sources [list \
    [file join $repo_root rtl dss_v2 common dss_v2_params_pkg.sv] \
    [file join $repo_root rtl dss_v2 common dss_v2_types_pkg.sv] \
    [file join $repo_root rtl dss_v2 group dss_v2_group_slot_decode.sv] \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl dss_canonical policy global recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_transition_adapter.v] \
    [file join $repo_root rtl dss_canonical policy global recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_candidate_map_producer.v] \
    [file join $repo_root rtl dss_canonical policy global recam_dss_canonical_global_noscratch_core.v] \
    [file join $repo_root rtl dss_canonical policy global recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_core.v] \
    [file join $repo_root rtl dss_canonical policy global recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_atomic_group_commit.v] \
    [file join $repo_root rtl dss_canonical top recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_integrated_shell.v] \
    [file join $repo_root rtl dss_canonical top recam_dss_grid2x2_directional_rs2_cs2_m1_normalized_group_global_noscratch_opt1_classcollapsed_integrated_top.v]]
