#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"

scripts/simulation/run_verilator_test.sh recam_dss_v2_phase4i_rtl_timing_top \
    tb/dss_v2/recam_dss_v2_phase4i_rtl_timing_top.sv \
    tb/dss_v2/recam_dss_v2_phase4i_rtl_timing_test.cpp \
    -Wno-PINCONNECTEMPTY -Wno-UNUSED -Wno-DECLFILENAME \
    -CFLAGS "-std=c++17 -I$root/inc" \
    rtl/dss_v2/common/dss_v2_params_pkg.sv \
    rtl/dss_v2/common/dss_v2_types_pkg.sv \
    rtl/dss_v2/adapter/dss_legacy_config_adapter.sv \
    rtl/dss_v2/adapter/dss_v2_legacy_ledger_diagnostic_adapter.sv \
    rtl/dss_v2/topology/dss_topology_2x2_directional.sv \
    rtl/dss_v2/resource/dss_v2_resource_feasibility.sv \
    rtl/dss_v2/resource/dss_v2_resource_ledger.sv \
    rtl/dss_v2/group/dss_v2_group_candidate_store.sv \
    rtl/dss_v2/group/dss_v2_group_slot_decode.sv \
    rtl/dss_v2/group/dss_v2_group_priority_reader.sv \
    rtl/dss_v2/top/recam_dss_v2_early_core.sv \
    rtl/dss_v2/top/recam_dss_v2_early_top.sv \
    rtl/dss_v2/top/recam_dss_v2_group_core.sv \
    rtl/dss_v2/top/recam_dss_v2_group_top.sv \
    rtl/recam/recam_shared_config_analyzer.sv
