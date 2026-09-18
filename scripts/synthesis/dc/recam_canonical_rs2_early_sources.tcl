# Canonical RS2 NORMALIZED_STREAMING_EARLY production boundary.  Packages and
# shared implementation blocks precede the canonical controller and top.
set recam_canonical_rs2_early_sources [list \
    [file join $repo_root rtl dss_v2 common dss_v2_params_pkg.sv] \
    [file join $repo_root rtl dss_v2 common dss_v2_types_pkg.sv] \
    [file join $repo_root rtl dss_v2 group dss_v2_group_slot_decode.sv] \
    [file join $repo_root rtl dss_v2 adapter dss_v2_legacy_ledger_diagnostic_adapter.sv] \
    [file join $repo_root rtl dss_v2 topology dss_topology_2x2_directional.sv] \
    [file join $repo_root rtl dss_v2 resource dss_v2_resource_feasibility.sv] \
    [file join $repo_root rtl dss_v2 resource dss_v2_resource_ledger.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 dss_v2_rs3cs3m1_config_table.sv] \
    [file join $repo_root rtl dss_v2 rs3cs3m1 dss_v2_rs3cs3m1_topology.sv] \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl dss_canonical policy early recam_dss_canonical_streaming_early_core.v] \
    [file join $repo_root rtl dss_canonical top recam_dss_canonical_rs2_streaming_early_top.v]]
