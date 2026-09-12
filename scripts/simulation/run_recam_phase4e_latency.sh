#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
scripts/simulation/run_verilator_test.sh recam_dss_v2_early_latency_test_top \
    tb/dss_v2/recam_dss_v2_early_latency_test_top.sv \
    tb/dss_v2/recam_dss_v2_early_latency_test.cpp \
    -Wno-PINCONNECTEMPTY -Wno-UNUSED -Wno-DECLFILENAME \
    rtl/dss_v2/common/dss_v2_params_pkg.sv \
    rtl/dss_v2/common/dss_v2_types_pkg.sv \
    rtl/dss_v2/adapter/dss_legacy_config_adapter.sv \
    rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv \
    rtl/dss_v2/topology/dss_topology_2x2_directional.sv \
    rtl/dss_v2/resource/dss_v2_resource_feasibility.sv \
    rtl/dss_v2/resource/dss_v2_resource_ledger.sv \
    rtl/dss_v2/top/recam_dss_v2_early_core.sv \
    rtl/dss_v2/top/recam_dss_v2_early_top.sv \
    rtl/recam/recam_shared_config_analyzer.sv \
    rtl/recam/recam_role_aware_config_scheduler.sv \
    rtl/recam/recam_config_resource_action_decoder.sv \
    rtl/recam/recam_physical_resource_ledger_checker.sv \
    rtl/recam/recam_resource_aware_fallback_selector.sv \
    rtl/recam/recam_dss_group_allocator.sv \
    rtl/recam/recam_phase3j_full_dss_top.sv
