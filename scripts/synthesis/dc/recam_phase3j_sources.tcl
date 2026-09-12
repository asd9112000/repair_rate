set recam_phase3j_sources [list \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl recam recam_role_aware_config_scheduler.sv] \
    [file join $repo_root rtl recam recam_config_resource_action_decoder.sv] \
    [file join $repo_root rtl recam recam_physical_resource_ledger_checker.sv] \
    [file join $repo_root rtl recam recam_resource_aware_fallback_selector.sv] \
    [file join $repo_root rtl recam recam_dss_group_allocator.sv] \
    [file join $repo_root rtl recam recam_phase3j_full_dss_top.sv]]
