# Production Phase 4F GROUP-NoScratch boundary only.  Packages precede all
# importers; no test bridge, ConfigPatternMap, Scratch, or reconstruction RTL.
set recam_phase4f_sources [list \
    [file join $repo_root rtl dss_v2 common dss_v2_params_pkg.sv] \
    [file join $repo_root rtl dss_v2 common dss_v2_types_pkg.sv] \
    [file join $repo_root rtl dss_v2 adapter dss_v2_legacy_ledger_diagnostic_adapter.sv] \
    [file join $repo_root rtl dss_v2 group dss_v2_group_candidate_store.sv] \
    [file join $repo_root rtl dss_v2 group dss_v2_group_slot_decode.sv] \
    [file join $repo_root rtl dss_v2 group dss_v2_group_priority_reader.sv] \
    [file join $repo_root rtl dss_v2 topology dss_topology_2x2_directional.sv] \
    [file join $repo_root rtl dss_v2 resource dss_v2_resource_feasibility.sv] \
    [file join $repo_root rtl dss_v2 resource dss_v2_resource_ledger.sv] \
    [file join $repo_root rtl dss_v2 top recam_dss_v2_group_core.sv] \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl dss_v2 top recam_dss_v2_group_top.sv]]
