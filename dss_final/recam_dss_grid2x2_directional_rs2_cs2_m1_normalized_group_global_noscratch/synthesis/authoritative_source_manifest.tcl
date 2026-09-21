# HYP02-D only: exact fixed-four-edge static-GLOBAL synthesis manifest.
# Packages precede importers.  No expanded-dual-donor topology or resource
# feasibility helper is part of this elaborated hierarchy.
set p3_synb_hyp02_d_reference_sources [list \
    [file join $repo_root rtl dss_v2 common dss_v2_params_pkg.sv] \
    [file join $repo_root rtl dss_v2 common dss_v2_types_pkg.sv] \
    [file join $repo_root rtl dss_v2 group dss_v2_group_candidate_store.sv] \
    [file join $repo_root rtl dss_v2 group dss_v2_group_slot_decode.sv] \
    [file join $repo_root rtl recam recam_shared_config_analyzer.sv] \
    [file join $repo_root rtl dss_hyp02 recam_dss_hyp02_static_selector.sv] \
    [file join $repo_root rtl dss_hyp02 recam_dss_hyp02_static_global_core.sv] \
    [file join $repo_root rtl dss_hyp02 recam_dss_hyp02_static_global_top.sv]]
