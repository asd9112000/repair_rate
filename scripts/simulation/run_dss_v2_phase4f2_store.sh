#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$root"
scripts/simulation/run_verilator_test.sh dss_v2_group_candidate_store_test_top \
    tb/dss_v2/dss_v2_group_candidate_store_test_top.sv \
    tb/dss_v2/dss_v2_group_candidate_store_test.cpp \
    rtl/dss_v2/common/dss_v2_params_pkg.sv \
    rtl/dss_v2/common/dss_v2_types_pkg.sv \
    rtl/dss_v2/group/dss_v2_group_candidate_store.sv \
    rtl/dss_v2/group/dss_v2_group_slot_decode.sv \
    rtl/dss_v2/group/dss_v2_group_priority_reader.sv
