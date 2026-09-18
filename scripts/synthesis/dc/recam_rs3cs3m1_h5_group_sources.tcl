# Isolated H5 target GROUP-NoScratch boundary.  No Scratch or test bridge.
set recam_rs3cs3m1_h5_group_sources [list \
    [file join $repo_root rtl dss_v2 common dss_v2_params_pkg.sv] \
    [file join $repo_root rtl dss_v2 common dss_v2_types_pkg.sv] \
    [file join $repo_root rtl dss_v2 adapter dss_v2_legacy_ledger_diagnostic_adapter.sv] \
    [file join $repo_root rtl dss_v2 resource dss_v2_resource_ledger.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 dss_v2_rs3cs3m1_config_table.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 dss_v2_rs3cs3m1_topology.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 dss_v2_rs3cs3m1_group_candidate_store.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 dss_v2_rs3cs3m1_shared_config_analyzer.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 recam_dss_v2_rs3cs3m1_group_core.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 recam_dss_v2_rs3cs3m1_group_top.sv]]
