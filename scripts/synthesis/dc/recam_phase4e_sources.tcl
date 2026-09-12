# Production Phase 4E V2 EARLY boundary only; test wrappers are intentionally
# absent.  Packages precede modules which import their canonical types.
set recam_phase4e_sources [list \
    [file join $repo_root rtl dss_v2 common dss_v2_params_pkg.sv] \
    [file join $repo_root rtl dss_v2 common dss_v2_types_pkg.sv] \
    [file join $repo_root rtl dss_v2 adapter dss_legacy_config_adapter.sv] \
    [file join $repo_root rtl dss_v2 adapter dss_v2_legacy_ledger_diagnostic_adapter.sv] \
    [file join $repo_root rtl dss_v2 topology dss_topology_2x2_directional.sv] \
    [file join $repo_root rtl dss_v2 resource dss_v2_resource_feasibility.sv] \
    [file join $repo_root rtl dss_v2 resource dss_v2_resource_ledger.sv] \
    [file join $repo_root rtl dss_v2 top recam_dss_v2_early_core.sv] \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl dss_v2 top recam_dss_v2_early_top.sv]]
