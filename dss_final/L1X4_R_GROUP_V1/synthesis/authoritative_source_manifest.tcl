# Self-contained FINAL archive source boundary.
# Source directly in a DC flow; this manifest never resolves mutable working-tree sources.
set l1x4_r_group_v1_package_root [file normalize [file join [file dirname [info script]] ..]]
set recam_dss_l1x4_r_static_global_n2_sources [list \
    [file join $l1x4_r_group_v1_package_root src dss_v2_params_pkg.sv] \
    [file join $l1x4_r_group_v1_package_root src dss_v2_types_pkg.sv] \
    [file join $l1x4_r_group_v1_package_root src recam_shared_config_analyzer.sv] \
    [file join $l1x4_r_group_v1_package_root src dss_v2_group_candidate_store.sv] \
    [file join $l1x4_r_group_v1_package_root src dss_v2_group_slot_decode.sv] \
    [file join $l1x4_r_group_v1_package_root src recam_dss_l1x4_r_static_selector.sv] \
    [file join $l1x4_r_group_v1_package_root src recam_dss_l1x4_r_static_global_core.sv] \
    [file join $l1x4_r_group_v1_package_root src recam_dss_l1x4_r_static_global_top.sv]]
