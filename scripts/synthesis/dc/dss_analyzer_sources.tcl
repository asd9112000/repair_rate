# Ordered RTL source list for the 2x2 directional multi-config analyzer.
# Paths are resolved from REPO_ROOT by run_dss_analyzer.tcl.
set dss_analyzer_relative_sources [list \
    rtl/dss_2x2/analyzer/dss_analyzer_top.sv \
    rtl/dss_2x2/analyzer/shared_fault_collector.sv \
    rtl/dss_2x2/analyzer/shared_pivot_cam.sv \
    rtl/dss_2x2/analyzer/shared_fault_counter.sv \
    rtl/dss_2x2/analyzer/tagged_hybrid_store.sv \
    rtl/dss_2x2/analyzer/cam_reuse_temp_buffer.sv \
    rtl/dss_2x2/analyzer/config_must_view.sv \
    rtl/dss_2x2/analyzer/hybrid_delta_bank.sv \
    rtl/dss_2x2/analyzer/multi_config_analyzer_bank.sv \
    rtl/dss_2x2/analyzer/multi_config_analyzer_area_engine.sv \
    rtl/dss_2x2/analyzer/config_logical_view.sv \
    rtl/dss_2x2/analyzer/config_analyzer.sv \
    rtl/dss_2x2/analyzer/config_candidate_evaluator.sv \
    rtl/dss_2x2/analyzer/config_result_bank.sv \
    rtl/dss_2x2/analyzer/early_selector.sv \
    rtl/dss_2x2/analyzer/group_selector.sv \
    rtl/dss_2x2/analyzer/resource_ledger.sv \
    rtl/dss_2x2/analyzer/config_resource_decode.sv \
    rtl/dss_2x2/common/resource_tracker.sv \
    rtl/dss_2x2/analyzer/pending_repair_buffer.sv \
    rtl/dss_2x2/analyzer/repair_decode_with_hybrid.sv \
    rtl/dss_2x2/analyzer/repair_decode_area_engine.sv]

set dss_analyzer_sources [list]
foreach relative_source $dss_analyzer_relative_sources {
    lappend dss_analyzer_sources [file join $repo_root $relative_source]
}
