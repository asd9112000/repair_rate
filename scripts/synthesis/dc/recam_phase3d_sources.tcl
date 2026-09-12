set recam_phase3d_sources [list \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl recam recam_role_aware_config_control.sv] \
    [file join $repo_root rtl recam recam_role_aware_config_scheduler.sv] \
    [file join $repo_root rtl recam recam_early_solution_selector.sv] \
    [file join $repo_root rtl recam recam_phase3d_integrated_top.sv]]
